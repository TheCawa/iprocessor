// ------------------------------------------------------------------------------
//          gui_emulator.cpp - GUI emulator rendering and controls
//
//  Copyright (C) 2026  TheCawa <vos80584@gmail.com>
// ------------------------------------------------------------------------------
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program. If not, see <https://gnu.org>.
// ------------------------------------------------------------------------------

#include "emulator.hpp"
#include "system.h"
#include "input.h"
#include "pit.h"
#include "psg.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "videocard.h"
#include "videocards.h"
#include "cpu_api.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <windows.h>
#include <commdlg.h>

// Display configuration: logical size of the screen panel in pixels.
// The physical video-card texture size is controlled by VC_MODE (0x2001A),
// while these values control the host window / panel size and zoom.
static int g_display_width = 640;
static int g_display_height = 400;

// CPU frequency indicator state.
static int g_steps_this_frame = 0;
static int g_steps_last_second = 0;
static int g_steps_accumulator = 0;
static Uint32 g_steps_last_time = 0;

static SDL_Window* g_window = nullptr;
static SDL_Renderer* g_renderer = nullptr;
static const VideoCard* g_vc = nullptr;
static const CpuBackend* g_current_backend = nullptr;

// Display scale factor for the screen panel (like browser zoom).
static float g_display_scale = 1.0f;

// "Open ROM..." state
static char g_rom_path[MAX_PATH] = "";
static char g_load_addr_buf[32] = "0x00050000";
static bool g_open_load_popup = false;
static char g_load_error[128] = "";

// "Open Disk Image..." state
static char g_disk_path[MAX_PATH] = "";
static int g_disk_drive_idx = 0;

// CPU control state
static bool g_cpu_running = true;
static bool g_step_requested = false;

// CPU speed settings
static CpuSpeedMode g_cpu_speed_mode = CPU_SPEED_ADAPTIVE;
static int g_cpu_fixed_steps = 50;
static int g_cpu_max_steps = 100000;
static int g_cpu_budget_ms = 10;

// RAM/VRAM sizes requested by the user (applied on Reset CPU / CPU switch).
static size_t g_desired_ram_kb = 0;
static size_t g_desired_vram_kb = 0;

// Input capture state: when true, keyboard/mouse events go to the emulated CPU.
static bool g_input_captured = false;

// Native Windows file-open dialog
static bool open_file_dialog(char* out_path, size_t max_len, const char* filter, const char* title) {
    char filename[MAX_PATH] = "";
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = sizeof(filename);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameA(&ofn)) return false;
    strncpy(out_path, filename, max_len - 1);
    out_path[max_len - 1] = '\0';
    return true;
}

static bool save_file_dialog(char* out_path, size_t max_len, const char* filter, const char* title, const char* def_ext) {
    char filename[MAX_PATH] = "";
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = sizeof(filename);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = title;
    ofn.lpstrDefExt = def_ext;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
    if (!GetSaveFileNameA(&ofn)) return false;
    strncpy(out_path, filename, max_len - 1);
    out_path[max_len - 1] = '\0';
    return true;
}

bool emulator_is_running() { return g_cpu_running; }
bool emulator_consume_step() {
    if (g_step_requested) {
        g_step_requested = false;
        return true;
    }
    return false;
}

bool emulator_is_input_captured(void) { return g_input_captured; }

static void emulator_set_input_capture(bool capture) {
    g_input_captured = capture;
    if (g_window) {
        SDL_SetWindowGrab(g_window, capture ? SDL_TRUE : SDL_FALSE);
        SDL_SetRelativeMouseMode(capture ? SDL_TRUE : SDL_FALSE);
    }
    SDL_ShowCursor(capture ? SDL_DISABLE : SDL_ENABLE);
}

static void emulator_capture_input(void) {
    emulator_set_input_capture(true);
}

static void emulator_release_input(void) {
    emulator_set_input_capture(false);
}

static void emulator_reset_cpu(Cpu* cpu, std::vector<uint8_t>& memory, std::vector<uint8_t>& vram) {
    // Apply desired RAM/VRAM sizes before reset.
    size_t ram_kb = g_desired_ram_kb ? g_desired_ram_kb
                   : (cpu->backend ? cpu->backend->mem_size_default / 1024 : 16 * 1024);
    size_t vram_kb = g_desired_vram_kb ? g_desired_vram_kb
                    : (cpu->backend ? cpu->backend->vram_size_default / 1024 : 512);
    size_t ram_bytes = ram_kb * 1024;
    size_t vram_bytes = vram_kb * 1024;

    if (memory.size() != ram_bytes || vram.size() != vram_bytes) {
        memory.assign(ram_bytes, 0);
        vram.assign(vram_bytes, 0);
        cpu->mem = memory.data();
        cpu->mem_size = memory.size();
        cpu->vram = vram.data();
        cpu->vram_size = vram.size();
    }

    cpu_reset(cpu);
    if (g_vc && g_vc->reset) {
        g_vc->reset(cpu);
    }
    cpu->update_term_res = (g_vc && g_vc->update_term_res) ? g_vc->update_term_res : NULL;
    // Clear RAM above the BIOS/CMOS reserved region (0x00020000..end).
    const uint32_t ram_start = 0x00020000;
    if (ram_start < cpu->mem_size) {
        memset(cpu->mem + ram_start, 0, cpu->mem_size - ram_start);
    }
    // Clear VRAM.
    if (cpu->vram && cpu->vram_size > 0) {
        memset(cpu->vram, 0, cpu->vram_size);
    }
    // Reset PC48 default video card mode select to 80x25 text mode.
    const uint32_t vc_mode_addr = 0x0002001A;
    if (vc_mode_addr < cpu->mem_size) {
        cpu->mem[vc_mode_addr] = 0;
        cpu->screen_dirty = true;
    }
}

void emulator_set_speed_mode(CpuSpeedMode mode) { g_cpu_speed_mode = mode; }
CpuSpeedMode emulator_get_speed_mode(void) { return g_cpu_speed_mode; }
void emulator_set_fixed_steps(int steps) { g_cpu_fixed_steps = steps > 0 ? steps : 1; }
int  emulator_get_fixed_steps(void) { return g_cpu_fixed_steps; }
void emulator_set_max_steps(int steps) { g_cpu_max_steps = steps > 0 ? steps : 1; }
int  emulator_get_max_steps(void) { return g_cpu_max_steps; }

void emulator_set_desired_ram_vram(size_t ram_kb, size_t vram_kb) {
    g_desired_ram_kb = ram_kb;
    g_desired_vram_kb = vram_kb;
}

void emulator_report_steps(int steps) {
    g_steps_this_frame = steps;
    g_steps_accumulator += steps;
    Uint32 now = SDL_GetTicks();
    if (now - g_steps_last_time >= 1000) {
        g_steps_last_second = g_steps_accumulator;
        g_steps_accumulator = 0;
        g_steps_last_time = now;
    }
}

bool emulator_init(SDL_Window** out_window, SDL_Renderer** out_renderer, const VideoCard* vc) {
    if (!vc) vc = videocard_get_default();
    g_vc = vc;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0) return false;

    psg_audio_init();

    g_window = SDL_CreateWindow("Iprocessor Emulator",
                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                1150, 680, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!g_window) return false;

    SDL_Renderer* renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) return false;
    g_renderer = renderer;

    if (g_vc->init(renderer) != 0) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(g_window);
        SDL_Quit();
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsLight();

    ImGui_ImplSDL2_InitForSDLRenderer(g_window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    *out_window = g_window;
    *out_renderer = renderer;
    return true;
}

bool emulator_switch_cpu(Cpu* cpu, const CpuBackend* new_backend, std::vector<uint8_t>& memory, std::vector<uint8_t>& vram) {
    if (!new_backend || !g_window) return false;

    // Preserve disk image paths across reinit.
    char saved_disk_paths[DISK_MAX_DRIVES][MAX_PATH] = {0};
    for (int i = 0; i < DISK_MAX_DRIVES; i++) {
        if (cpu->disk_drives[i].image_path[0]) {
            strncpy(saved_disk_paths[i], cpu->disk_drives[i].image_path, sizeof(saved_disk_paths[i]) - 1);
        }
    }
    disk_free_image(cpu);

    size_t ram_kb = g_desired_ram_kb ? g_desired_ram_kb : (new_backend->mem_size_default / 1024);
    size_t vram_kb = g_desired_vram_kb ? g_desired_vram_kb : (new_backend->vram_size_default / 1024);
    memory.assign(ram_kb * 1024, 0);
    vram.assign(vram_kb * 1024, 0);
    cpu_init(cpu, new_backend, memory.data(), memory.size(), vram.data(), vram.size());
    g_current_backend = new_backend;

    for (int i = 0; i < DISK_MAX_DRIVES; i++) {
        if (saved_disk_paths[i][0]) {
            strncpy(cpu->disk_drives[i].image_path, saved_disk_paths[i],
                    sizeof(cpu->disk_drives[i].image_path) - 1);
            cpu->disk_drives[i].image_path[sizeof(cpu->disk_drives[i].image_path) - 1] = '\0';
        }
    }

    char title[512];
    snprintf(title, sizeof(title), "%s - %s", new_backend->name, g_vc ? g_vc->name : "no-vc");
    SDL_SetWindowTitle(g_window, title);
    return true;
}

bool emulator_switch_vc(Cpu* cpu, const VideoCard* new_vc, SDL_Renderer* renderer) {
    if (!new_vc || !renderer) return false;
    if (g_vc) g_vc->shutdown();
    g_vc = new_vc;
    if (g_vc->init(renderer) != 0) {
        g_vc = nullptr;
        return false;
    }
    cpu->screen_dirty = true;
    cpu->update_term_res = (g_vc && g_vc->update_term_res) ? g_vc->update_term_res : NULL;
    if (g_window) {
        char title[512];
        snprintf(title, sizeof(title), "%s - %s", g_current_backend ? g_current_backend->name : "cpu", g_vc->name);
        SDL_SetWindowTitle(g_window, title);
    }
    return true;
}

#define STATE_MAGIC   "IPSTATE"
#define STATE_VERSION 1

static bool save_machine_state(Cpu* cpu, const char* filename) {
    FILE* f = fopen(filename, "wb");
    if (!f) return false;

    bool ok = true;
    const char* backend_name = cpu->backend ? cpu->backend->name : "";
    uint32_t backend_name_len = (uint32_t)strlen(backend_name);
    uint32_t backend_state_size = cpu->backend ? (uint32_t)cpu->backend->state_size : 0;
    uint32_t ram_size = (uint32_t)cpu->mem_size;
    uint32_t pit_size = (uint32_t)pit_state_size();
    uint32_t mode_addr = 0x0002001A;
    uint8_t video_mode = (mode_addr < cpu->mem_size) ? cpu->mem[mode_addr] : 0x00;

    ok = ok && fwrite(STATE_MAGIC, 1, 8, f) == 8;
    uint32_t version = STATE_VERSION;
    ok = ok && fwrite(&version, sizeof(version), 1, f) == 1;
    ok = ok && fwrite(&backend_name_len, sizeof(backend_name_len), 1, f) == 1;
    ok = ok && fwrite(backend_name, 1, backend_name_len, f) == backend_name_len;
    ok = ok && fwrite(&backend_state_size, sizeof(backend_state_size), 1, f) == 1;
    ok = ok && fwrite(&ram_size, sizeof(ram_size), 1, f) == 1;
    ok = ok && fwrite(&pit_size, sizeof(pit_size), 1, f) == 1;

    // RAM
    if (cpu->mem && ram_size > 0) {
        ok = ok && fwrite(cpu->mem, 1, ram_size, f) == ram_size;
    }
    // Backend state
    if (cpu->backend_data && backend_state_size > 0) {
        ok = ok && fwrite(cpu->backend_data, 1, backend_state_size, f) == backend_state_size;
    }
    // CPU common state
    ok = ok && fwrite(&cpu->halted, sizeof(cpu->halted), 1, f) == 1;
    ok = ok && fwrite(&cpu->irq_enabled, sizeof(cpu->irq_enabled), 1, f) == 1;
    ok = ok && fwrite(&cpu->screen_dirty, sizeof(cpu->screen_dirty), 1, f) == 1;
    ok = ok && fwrite(&cpu->term_cursor_x, sizeof(cpu->term_cursor_x), 1, f) == 1;
    ok = ok && fwrite(&cpu->term_cursor_y, sizeof(cpu->term_cursor_y), 1, f) == 1;
    ok = ok && fwrite(&cpu->term_attr, sizeof(cpu->term_attr), 1, f) == 1;
    ok = ok && fwrite(&cpu->kbd_buffer_len, sizeof(cpu->kbd_buffer_len), 1, f) == 1;
    ok = ok && fwrite(&cpu->kbd_buffer_pos, sizeof(cpu->kbd_buffer_pos), 1, f) == 1;
    ok = ok && fwrite(&cpu->kbd_irq_pending, sizeof(cpu->kbd_irq_pending), 1, f) == 1;
    ok = ok && fwrite(cpu->kbd_buffer, 1, sizeof(cpu->kbd_buffer), f) == sizeof(cpu->kbd_buffer);
    ok = ok && fwrite(&cpu->mouse_x, sizeof(cpu->mouse_x), 1, f) == 1;
    ok = ok && fwrite(&cpu->mouse_y, sizeof(cpu->mouse_y), 1, f) == 1;
    ok = ok && fwrite(&cpu->mouse_delta_x, sizeof(cpu->mouse_delta_x), 1, f) == 1;
    ok = ok && fwrite(&cpu->mouse_delta_y, sizeof(cpu->mouse_delta_y), 1, f) == 1;
    ok = ok && fwrite(&cpu->mouse_buttons, sizeof(cpu->mouse_buttons), 1, f) == 1;
    ok = ok && fwrite(&cpu->mouse_irq_pending, sizeof(cpu->mouse_irq_pending), 1, f) == 1;
    ok = ok && fwrite(&cpu->disk_current_drive, sizeof(cpu->disk_current_drive), 1, f) == 1;
    ok = ok && fwrite(&cpu->devclass_selected, sizeof(cpu->devclass_selected), 1, f) == 1;
    ok = ok && fwrite(&video_mode, sizeof(video_mode), 1, f) == 1;

    // Disk image paths
    for (int i = 0; i < DISK_MAX_DRIVES; i++) {
        uint32_t len = (uint32_t)strlen(cpu->disk_drives[i].image_path);
        ok = ok && fwrite(&len, sizeof(len), 1, f) == 1;
        if (len > 0) {
            ok = ok && fwrite(cpu->disk_drives[i].image_path, 1, len, f) == len;
        }
    }

    // PIT state
    if (pit_size > 0 && cpu->pit_data) {
        std::vector<uint8_t> pit_buf(pit_size);
        pit_save_state(cpu, pit_buf.data());
        ok = ok && fwrite(pit_buf.data(), 1, pit_size, f) == pit_size;
    }

    fclose(f);
    return ok;
}

static bool load_machine_state(Cpu* cpu, const char* filename, std::vector<uint8_t>& memory) {
    (void)memory;
    FILE* f = fopen(filename, "rb");
    if (!f) return false;

    char magic[8] = {0};
    if (fread(magic, 1, 8, f) != 8 || strncmp(magic, STATE_MAGIC, 7) != 0) {
        fclose(f);
        return false;
    }

    uint32_t version = 0;
    uint32_t backend_name_len = 0;
    uint32_t backend_state_size = 0;
    uint32_t ram_size = 0;
    uint32_t pit_size = 0;
    if (fread(&version, sizeof(version), 1, f) != 1 || version != STATE_VERSION ||
        fread(&backend_name_len, sizeof(backend_name_len), 1, f) != 1 || backend_name_len >= 256 ||
        fread(&backend_state_size, sizeof(backend_state_size), 1, f) != 1 ||
        fread(&ram_size, sizeof(ram_size), 1, f) != 1 ||
        fread(&pit_size, sizeof(pit_size), 1, f) != 1) {
        fclose(f);
        return false;
    }

    char backend_name[256] = {0};
    if (fread(backend_name, 1, backend_name_len, f) != backend_name_len) {
        fclose(f);
        return false;
    }

    if (!cpu->backend || strcmp(cpu->backend->name, backend_name) != 0) {
        fclose(f);
        return false;
    }

    bool ok = true;
    // RAM
    if (ram_size > 0 && ram_size <= cpu->mem_size && cpu->mem) {
        ok = ok && fread(cpu->mem, 1, ram_size, f) == ram_size;
    } else {
        ok = false;
    }
    // Backend state
    if (backend_state_size > 0 && backend_state_size <= cpu->backend->state_size && cpu->backend_data) {
        ok = ok && fread(cpu->backend_data, 1, backend_state_size, f) == backend_state_size;
    } else if (backend_state_size != 0) {
        ok = false;
    }

    uint8_t video_mode = 0;
    // CPU common state
    ok = ok && fread(&cpu->halted, sizeof(cpu->halted), 1, f) == 1;
    ok = ok && fread(&cpu->irq_enabled, sizeof(cpu->irq_enabled), 1, f) == 1;
    ok = ok && fread(&cpu->screen_dirty, sizeof(cpu->screen_dirty), 1, f) == 1;
    ok = ok && fread(&cpu->term_cursor_x, sizeof(cpu->term_cursor_x), 1, f) == 1;
    ok = ok && fread(&cpu->term_cursor_y, sizeof(cpu->term_cursor_y), 1, f) == 1;
    ok = ok && fread(&cpu->term_attr, sizeof(cpu->term_attr), 1, f) == 1;
    ok = ok && fread(&cpu->kbd_buffer_len, sizeof(cpu->kbd_buffer_len), 1, f) == 1;
    ok = ok && fread(&cpu->kbd_buffer_pos, sizeof(cpu->kbd_buffer_pos), 1, f) == 1;
    ok = ok && fread(&cpu->kbd_irq_pending, sizeof(cpu->kbd_irq_pending), 1, f) == 1;
    ok = ok && fread(cpu->kbd_buffer, 1, sizeof(cpu->kbd_buffer), f) == sizeof(cpu->kbd_buffer);
    ok = ok && fread(&cpu->mouse_x, sizeof(cpu->mouse_x), 1, f) == 1;
    ok = ok && fread(&cpu->mouse_y, sizeof(cpu->mouse_y), 1, f) == 1;
    ok = ok && fread(&cpu->mouse_delta_x, sizeof(cpu->mouse_delta_x), 1, f) == 1;
    ok = ok && fread(&cpu->mouse_delta_y, sizeof(cpu->mouse_delta_y), 1, f) == 1;
    ok = ok && fread(&cpu->mouse_buttons, sizeof(cpu->mouse_buttons), 1, f) == 1;
    ok = ok && fread(&cpu->mouse_irq_pending, sizeof(cpu->mouse_irq_pending), 1, f) == 1;
    ok = ok && fread(&cpu->disk_current_drive, sizeof(cpu->disk_current_drive), 1, f) == 1;
    ok = ok && fread(&cpu->devclass_selected, sizeof(cpu->devclass_selected), 1, f) == 1;
    ok = ok && fread(&video_mode, sizeof(video_mode), 1, f) == 1;

    // Disk image paths
    for (int i = 0; i < DISK_MAX_DRIVES && ok; i++) {
        uint32_t len = 0;
        ok = ok && fread(&len, sizeof(len), 1, f) == 1;
        if (len >= sizeof(cpu->disk_drives[i].image_path)) {
            ok = false;
            break;
        }
        cpu->disk_drives[i].image_path[0] = '\0';
        if (len > 0) {
            ok = ok && fread(cpu->disk_drives[i].image_path, 1, len, f) == len;
            cpu->disk_drives[i].image_path[len] = '\0';
        }
    }

    // PIT state
    if (pit_size > 0 && pit_size == pit_state_size() && cpu->pit_data) {
        std::vector<uint8_t> pit_buf(pit_size);
        ok = ok && fread(pit_buf.data(), 1, pit_size, f) == pit_size;
        if (ok) pit_load_state(cpu, pit_buf.data());
    } else if (pit_size != 0) {
        ok = false;
    }

    fclose(f);

    if (ok) {
        uint32_t mode_addr = 0x0002001A;
        if (mode_addr < cpu->mem_size) {
            cpu->mem[mode_addr] = video_mode;
        }
        cpu->screen_dirty = true;
    }
    return ok;
}

void emulator_render(Cpu* cpu, SDL_Renderer* renderer, std::vector<uint8_t>& memory, std::vector<uint8_t>& vram) {
    (void)vram;  // available for future VRAM viewers
    if (cpu && cpu->backend && !g_current_backend) {
        g_current_backend = cpu->backend;
    }

    if (g_vc) {
        g_vc->update(cpu);
    }

    SDL_Texture* screen_tex = g_vc ? g_vc->get_texture() : nullptr;
    int display_w = (int)(g_display_width * g_display_scale);
    int display_h = (int)(g_display_height * g_display_scale);

    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("FILE")) {
            if (ImGui::MenuItem("Open ROM...")) {
                if (open_file_dialog(g_rom_path, sizeof(g_rom_path),
                        "i80148 binaries (*.bin;*.hex)\0*.bin;*.hex\0All files (*.*)\0*.*\0",
                        "Open ROM / Program")) {
                    g_load_error[0] = '\0';
                    g_open_load_popup = true;
                }
            }
            if (ImGui::MenuItem("Open Disk Image...")) {
                if (open_file_dialog(g_disk_path, sizeof(g_disk_path),
                        "Disk images (*.bin;*.hex;*.img)\0*.bin;*.hex;*.img\0All files (*.*)\0*.*\0",
                        "Open Disk Image")) {
                    if (g_disk_drive_idx >= 0 && g_disk_drive_idx < DISK_MAX_DRIVES) {
                        strncpy(cpu->disk_drives[g_disk_drive_idx].image_path, g_disk_path,
                                sizeof(cpu->disk_drives[g_disk_drive_idx].image_path) - 1);
                        cpu->disk_drives[g_disk_drive_idx].image_path[
                            sizeof(cpu->disk_drives[g_disk_drive_idx].image_path) - 1] = '\0';
                    }
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save state...")) {
                char path[MAX_PATH] = "";
                if (save_file_dialog(path, sizeof(path),
                        "Iprocessor state (*.ips)\0*.ips\0All files (*.*)\0*.*\0",
                        "Save machine state", "ips")) {
                    if (!save_machine_state(cpu, path)) {
                        fprintf(stderr, "[ERROR] Failed to save state to %s\n", path);
                    }
                }
            }
            if (ImGui::MenuItem("Load state...")) {
                char path[MAX_PATH] = "";
                if (open_file_dialog(path, sizeof(path),
                        "Iprocessor state (*.ips)\0*.ips\0All files (*.*)\0*.*\0",
                        "Load machine state")) {
                    if (!load_machine_state(cpu, path, memory)) {
                        fprintf(stderr, "[ERROR] Failed to load state from %s\n", path);
                    }
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Reset CPU")) { emulator_reset_cpu(cpu, memory, vram); g_cpu_running = true; }
            if (ImGui::MenuItem("Exit")) { emulator_shutdown(); exit(0); }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("SYSTEM")) {
            if (ImGui::BeginCombo("CPU", g_current_backend ? g_current_backend->name : "select")) {
                extern const CpuBackend* cpu_backend_at(int idx);
                extern int cpu_backend_count(void);
                for (int i = 0; i < cpu_backend_count(); i++) {
                    const CpuBackend* b = cpu_backend_at(i);
                    if (!b) continue;
                    bool selected = (b == g_current_backend);
                    if (ImGui::Selectable(b->name, selected)) {
                        emulator_switch_cpu(cpu, b, memory, vram);
                    }
                    if (selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            if (ImGui::BeginCombo("Video card", g_vc ? g_vc->name : "select")) {
                for (int i = 0; g_video_cards[i]; i++) {
                    const VideoCard* vc = g_video_cards[i];
                    bool selected = (vc == g_vc);
                    if (ImGui::Selectable(vc->name, selected)) {
                        emulator_switch_vc(cpu, vc, renderer);
                    }
                    if (selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            char drive_label[64];
            const char* drive_name = cpu->disk_drives[g_disk_drive_idx].image_path[0]
                ? strrchr(cpu->disk_drives[g_disk_drive_idx].image_path, '\\')
                : nullptr;
            if (!drive_name) drive_name = cpu->disk_drives[g_disk_drive_idx].image_path[0]
                ? strrchr(cpu->disk_drives[g_disk_drive_idx].image_path, '/')
                : nullptr;
            if (drive_name) drive_name++; else drive_name = "(empty)";
            snprintf(drive_label, sizeof(drive_label), "Drive %d: %s", g_disk_drive_idx, drive_name);
            if (ImGui::BeginCombo("Disk drive", drive_label)) {
                for (int i = 0; i < DISK_MAX_DRIVES; i++) {
                    const char* name = cpu->disk_drives[i].image_path[0]
                        ? strrchr(cpu->disk_drives[i].image_path, '\\')
                        : nullptr;
                    if (!name) name = cpu->disk_drives[i].image_path[0]
                        ? strrchr(cpu->disk_drives[i].image_path, '/')
                        : nullptr;
                    if (name) name++; else name = "(empty)";
                    char label[64];
                    snprintf(label, sizeof(label), "Drive %d: %s", i, name);
                    bool selected = (i == g_disk_drive_idx);
                    if (ImGui::Selectable(label, selected)) {
                        g_disk_drive_idx = i;
                    }
                    if (selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            if (ImGui::Button("Eject")) {
                disk_drive_free_image(&cpu->disk_drives[g_disk_drive_idx]);
                cpu->disk_drives[g_disk_drive_idx].image_path[0] = '\0';
            }
            ImGui::SameLine();
            if (ImGui::Button("Clear RAM")) {
                uint32_t ram_start = cpu->backend ? cpu->backend->user_ram_start : 0x00060000;
                if (ram_start < cpu->mem_size) {
                    memset(&cpu->mem[ram_start], 0, cpu->mem_size - ram_start);
                }
                cpu->screen_dirty = true;
                if (g_vc && g_vc->reset) {
                    g_vc->reset(cpu);
                }
            }

            // RAM / VRAM size editors. Sizes are applied on Reset CPU.
            int ram_kb_editor = (int)(g_desired_ram_kb ? g_desired_ram_kb : (cpu->backend ? cpu->backend->mem_size_default / 1024 : 16 * 1024));
            int vram_kb_editor = (int)(g_desired_vram_kb ? g_desired_vram_kb : (cpu->backend ? cpu->backend->vram_size_default / 1024 : 512));
            ImGui::SetNextItemWidth(120.0f);
            if (ImGui::InputInt("RAM (KB)", &ram_kb_editor, 64, 1024)) {
                if (ram_kb_editor < 64) ram_kb_editor = 64;
                g_desired_ram_kb = (size_t)ram_kb_editor;
            }
            ImGui::SetNextItemWidth(120.0f);
            if (ImGui::InputInt("VRAM (KB)", &vram_kb_editor, 64, 1024)) {
                if (vram_kb_editor < 64) vram_kb_editor = 64;
                g_desired_vram_kb = (size_t)vram_kb_editor;
            }
            ImGui::TextDisabled("Applied on Reset CPU");

            ImGui::Separator();

            // Display settings: host window size and color depth.
            // VC_MODE (0x2001A) remains under program control.
            struct { const char* name; int w; int h; } resolutions[] = {
                { "640x400",  640,  400 },
                { "800x600",  800,  600 },
                { "1024x768", 1024, 768 },
                { "1280x720", 1280, 720 },
            };
            const char* current_res_name = resolutions[0].name;
            for (int i = 0; i < (int)(sizeof(resolutions)/sizeof(resolutions[0])); i++) {
                if (resolutions[i].w == g_display_width && resolutions[i].h == g_display_height) {
                    current_res_name = resolutions[i].name;
                    break;
                }
            }
            if (ImGui::BeginCombo("Display resolution", current_res_name)) {
                for (int i = 0; i < (int)(sizeof(resolutions)/sizeof(resolutions[0])); i++) {
                    bool selected = (resolutions[i].w == g_display_width && resolutions[i].h == g_display_height);
                    if (ImGui::Selectable(resolutions[i].name, selected)) {
                        g_display_width = resolutions[i].w;
                        g_display_height = resolutions[i].h;
                        if (g_window) {
                            SDL_SetWindowSize(g_window, g_display_width + 400, g_display_height + 80);
                        }
                    }
                    if (selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            const char* depth_names[] = { "16 colors", "256 colors (VGA)" };
            static int g_color_depth = 8; // 4 = 16 colors, 8 = 256 colors
            int depth_idx = (g_color_depth == 8) ? 1 : 0;
            if (ImGui::BeginCombo("Color depth", depth_names[depth_idx])) {
                for (int i = 0; i < 2; i++) {
                    bool selected = (i == depth_idx);
                    if (ImGui::Selectable(depth_names[i], selected)) {
                        g_color_depth = (i == 1) ? 8 : 4;
                    }
                    if (selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::Separator();

            const char* speed_names[] = {"Adaptive", "Fixed", "Unlimited"};
            if (ImGui::BeginCombo("CPU speed", speed_names[g_cpu_speed_mode])) {
                for (int i = 0; i < 3; i++) {
                    bool selected = (i == (int)g_cpu_speed_mode);
                    if (ImGui::Selectable(speed_names[i], selected)) {
                        g_cpu_speed_mode = (CpuSpeedMode)i;
                    }
                    if (selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            if (g_cpu_speed_mode == CPU_SPEED_FIXED) {
                ImGui::SetNextItemWidth(120.0f);
                ImGui::InputInt("Steps/frame", &g_cpu_fixed_steps, 1, 100);
                if (g_cpu_fixed_steps < 1) g_cpu_fixed_steps = 1;
            }

            ImGui::SetNextItemWidth(120.0f);
            ImGui::InputInt("Max steps/frame", &g_cpu_max_steps, 100, 1000);
            if (g_cpu_max_steps < 1) g_cpu_max_steps = 1;

            ImGui::SetNextItemWidth(120.0f);
            ImGui::InputInt("Frame budget (ms)", &g_cpu_budget_ms, 1, 2);
            if (g_cpu_budget_ms < 1) g_cpu_budget_ms = 1;
            if (g_cpu_budget_ms > 15) g_cpu_budget_ms = 15;

            ImGui::EndMenu();
        }

        float stats_width = 300.0f;
        ImGui::SameLine(ImGui::GetWindowWidth() - stats_width);
        ImGui::Text("Iprocessor Emulator");

        ImGui::EndMainMenuBar();
    }

    // --- Load ROM modal ---
    if (g_open_load_popup) {
        ImGui::OpenPopup("Load ROM");
        g_open_load_popup = false;
    }
    if (ImGui::BeginPopupModal("Load ROM", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("File: %s", g_rom_path);
        ImGui::InputText("Load address", g_load_addr_buf, sizeof(g_load_addr_buf));
        {
            uint32_t user_ram = cpu->backend ? cpu->backend->user_ram_start : 0x00060000;
            char hint[128];
            snprintf(hint, sizeof(hint), "User programs load at 0x%08X+; BIOS is loaded automatically.", user_ram);
            ImGui::TextDisabled("%s", hint);
        }
        if (g_load_error[0] != '\0') {
            ImGui::TextColored(ImVec4(0.8f, 0.1f, 0.1f, 1.0f), "%s", g_load_error);
        }
        if (ImGui::Button("Load", ImVec2(120, 0))) {
            uint32_t addr = (uint32_t)strtoul(g_load_addr_buf, nullptr, 0);
            uint32_t user_ram = cpu->backend ? cpu->backend->user_ram_start : 0x00060000;
            // 0x00000000 is allowed as an explicit BIOS/ROM override.
            if (addr != 0x00000000 && addr < user_ram) {
                snprintf(g_load_error, sizeof(g_load_error),
                         "Address below 0x%08X is reserved for system ROM/MMIO/VRAM window (except 0x00000000 for ROM override).", user_ram);
            } else if (cpu_load_file(cpu, g_rom_path, addr) == 0) {
                // Set execution entry point: ROM override starts at 0,
                // user programs start at their load address.
                uint32_t pc = (addr == 0x00000000) ? 0x00000000 : addr;
                cpu_set_reg(cpu, cpu->backend->pc_register, pc);
                g_cpu_running = true;
                if (g_window) {
                    char title[512];
                    snprintf(title, sizeof(title), "%s - %s @ 0x%08X",
                             g_current_backend ? g_current_backend->name : "cpu",
                             g_rom_path, addr);
                    SDL_SetWindowTitle(g_window, title);
                }
                ImGui::CloseCurrentPopup();
                g_load_error[0] = '\0';
            } else {
                snprintf(g_load_error, sizeof(g_load_error), "Failed to load file!");
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetFrameHeight()));
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y - ImGui::GetFrameHeight()));

    ImGui::Begin("MasterWindow", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

    // --- LEFT PANEL: screen ---
    ImGui::BeginChild("ScreenPanel", ImVec2((float)(display_w + 30), 0), true);
    ImGui::Text("SCREEN (%s)", g_vc ? g_vc->description : "none");
    ImGui::Separator();
    ImGui::SetNextItemWidth(150.0f);
    ImGui::SliderFloat("Zoom", &g_display_scale, 0.5f, 3.0f, "%.1fx");
    ImGui::Separator();
    if (screen_tex) {
        ImGui::Image((void*)(intptr_t)screen_tex, ImVec2((float)display_w, (float)display_h));
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
            emulator_capture_input();
        }
    } else {
        ImGui::Text("No video card initialized.");
    }
    ImGui::EndChild();
    ImGui::SameLine();

    // --- RIGHT PANEL: registers / state ---
    ImGui::BeginChild("ControlPanel", ImVec2(0, 0), true);

    if (ImGui::BeginTabBar("TabsContainer")) {

        if (ImGui::BeginTabItem("CPU STATE")) {
            ImGui::Spacing();

            // CPU control buttons
            if (g_cpu_running) {
                if (ImGui::Button("Pause", ImVec2(80, 0))) g_cpu_running = false;
            } else {
                if (ImGui::Button("Run", ImVec2(80, 0))) g_cpu_running = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Step", ImVec2(80, 0))) g_step_requested = true;
            ImGui::SameLine();
            if (ImGui::Button("Reset", ImVec2(80, 0))) { emulator_reset_cpu(cpu, memory, vram); g_cpu_running = true; }
            ImGui::SameLine();
            ImGui::Text("Status: %s", cpu->halted ? "HALTED" : (g_cpu_running ? "RUNNING" : "PAUSED"));

            ImGui::Separator();

            if (cpu->backend && cpu->backend->render_state) {
                cpu->backend->render_state(cpu);
            } else if (cpu->backend && cpu->backend->register_names) {
                if (ImGui::BeginTable("RegistersTable", 2, ImGuiTableFlags_NoBordersInBody)) {
                    int half = (cpu->backend->register_count + 1) / 2;
                    for (int col = 0; col < 2; col++) {
                        ImGui::TableNextColumn();
                        for (int i = 0; i < half; i++) {
                            int idx = col * half + i;
                            if (idx >= cpu->backend->register_count) break;
                            const char* name = cpu->backend->register_names[idx];
                            uint64_t val = cpu_get_reg(cpu, (uint8_t)idx);
                            ImGui::Text("%s = %08X", name, (unsigned int)val);
                        }
                    }
                    ImGui::EndTable();
                }
            }

            ImGui::Separator();
            if (cpu->backend) {
                ImGui::Text("IC = %08X", (unsigned int)cpu_get_reg(cpu, cpu->backend->pc_register));
                ImGui::Text("FL = %08X", (unsigned int)cpu_get_reg(cpu, cpu->backend->fl_register));
                ImGui::Text("SP = %08X", (unsigned int)cpu_get_reg(cpu, cpu->backend->sp_register));
            }
            ImGui::Spacing();
            ImGui::Text("Halted = %s", cpu->halted ? "YES" : "NO");
            ImGui::Text("IRQ    = %s", cpu->irq_enabled ? "ON" : "OFF");

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("MEMORY DUMP")) {
            static char mem_addr_buf[32] = "0x00000000";
            static uint32_t dump_addr = 0;
            static uint32_t edit_addr = 0xFFFFFFFF;
            static char edit_buf[4] = "";
            static bool edit_activate = false;
            const uint32_t page_size = 256;

            ImGui::SetNextItemWidth(90.0f);
            ImGui::InputText("Address", mem_addr_buf, sizeof(mem_addr_buf),
                             ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);
            ImGui::SameLine();
            if (ImGui::Button("Go")) {
                dump_addr = (uint32_t)strtoul(mem_addr_buf, nullptr, 0);
            }
            ImGui::SameLine();
            if (ImGui::Button("<")) {
                dump_addr = (dump_addr >= page_size) ? (dump_addr - page_size) : 0;
                snprintf(mem_addr_buf, sizeof(mem_addr_buf), "0x%08X", dump_addr);
            }
            ImGui::SameLine();
            if (ImGui::Button(">")) {
                dump_addr = (dump_addr + page_size < cpu->mem_size) ? (dump_addr + page_size) : dump_addr;
                snprintf(mem_addr_buf, sizeof(mem_addr_buf), "0x%08X", dump_addr);
            }
            ImGui::SameLine();
            if (ImGui::Button("BIOS")) {
                dump_addr = 0x00000000;
                snprintf(mem_addr_buf, sizeof(mem_addr_buf), "0x%08X", dump_addr);
            }
            ImGui::SameLine();
            if (ImGui::Button("VBUFFER")) {
                dump_addr = cpu->backend->vbuffer_base;
                snprintf(mem_addr_buf, sizeof(mem_addr_buf), "0x%08X", dump_addr);
            }
            ImGui::SameLine();
            if (ImGui::Button("USER RAM")) {
                dump_addr = cpu->backend ? cpu->backend->user_ram_start : 0x00060000;
                snprintf(mem_addr_buf, sizeof(mem_addr_buf), "0x%08X", dump_addr);
            }
            ImGui::Separator();

            if (ImGui::BeginTable("MemDumpTable", 18, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit)) {
                ImGui::TableSetupColumn("Addr", ImGuiTableColumnFlags_WidthFixed, 75.0f);
                for (int i = 0; i < 16; i++) {
                    char col_name[4];
                    snprintf(col_name, sizeof(col_name), "%X", i);
                    ImGui::TableSetupColumn(col_name, ImGuiTableColumnFlags_WidthFixed, 28.0f);
                }
                ImGui::TableSetupColumn("ASCII", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();

                for (int row = 0; row < 16; row++) {
                    ImGui::TableNextRow();
                    uint32_t row_addr = dump_addr + (uint32_t)(row * 16);
                    ImGui::TableNextColumn();
                    ImGui::Text("%08X", row_addr);

                    char ascii[17];
                    for (int col = 0; col < 16; col++) {
                        uint32_t a = row_addr + (uint32_t)col;
                        uint8_t b = 0;
                        if (a < cpu->mem_size) {
                            b = cpu->mem[a];
                        }
                        ImGui::TableNextColumn();

                        if (a < cpu->mem_size && edit_addr == a) {
                            ImGui::SetNextItemWidth(-FLT_MIN);
                            if (edit_activate) {
                                ImGui::SetKeyboardFocusHere();
                            }
                            char edit_id[32];
                            snprintf(edit_id, sizeof(edit_id), "##edit_%08X", a);
                            bool entered = ImGui::InputText(edit_id, edit_buf, sizeof(edit_buf),
                                                 ImGuiInputTextFlags_CharsHexadecimal |
                                                 ImGuiInputTextFlags_CharsUppercase |
                                                 ImGuiInputTextFlags_EnterReturnsTrue);
                            bool cancelled = ImGui::IsKeyPressed(ImGuiKey_Escape);
                            if (entered) {
                                uint32_t val = (uint32_t)strtoul(edit_buf, nullptr, 16);
                                cpu->mem[a] = (uint8_t)(val & 0xFF);
                                edit_addr = 0xFFFFFFFF;
                                edit_activate = false;
                            } else if (cancelled) {
                                edit_addr = 0xFFFFFFFF;
                                edit_activate = false;
                            } else if (ImGui::IsItemDeactivated()) {
                                // Focus lost (clicked outside, Tab, etc.): save value.
                                uint32_t val = (uint32_t)strtoul(edit_buf, nullptr, 16);
                                cpu->mem[a] = (uint8_t)(val & 0xFF);
                                edit_addr = 0xFFFFFFFF;
                                edit_activate = false;
                            } else if (ImGui::IsItemActive() || ImGui::IsItemFocused()) {
                                // Input field is being interacted with; keep it open.
                                edit_activate = false;
                            } else if (!edit_activate) {
                                // Input field failed to take focus; close editor to avoid getting stuck.
                                edit_addr = 0xFFFFFFFF;
                            }
                        } else if (a < cpu->mem_size) {
                            char cell_id[32];
                            snprintf(cell_id, sizeof(cell_id), "%02X##cell_%08X", b, a);
                            ImGui::Selectable(cell_id, false, ImGuiSelectableFlags_AllowDoubleClick);
                            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                                edit_addr = a;
                                snprintf(edit_buf, sizeof(edit_buf), "%02X", b);
                                edit_activate = true;
                            }
                        } else {
                            ImGui::Text("--");
                        }
                        ascii[col] = (b >= 32 && b < 127) ? (char)b : '.';
                    }
                    ascii[16] = '\0';
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", ascii);
                }
                ImGui::EndTable();
            }

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::Separator();
    ImGui::Text("CPU clock:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.0f, 0.7f, 0.0f, 1.0f), "%d steps/s", g_steps_last_second);

    ImGui::EndChild();
    ImGui::End();

    ImGui::Render();
    SDL_SetRenderDrawColor(renderer, 200, 255, 0, 255);
    SDL_RenderClear(renderer);

    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);
}

bool emulator_handle_events(Cpu* cpu) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        // When input is captured, route keyboard/mouse to the emulated CPU
        // instead of Dear ImGui.
        if (g_input_captured) {
            if (e.type == SDL_QUIT) return false;

            if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                emulator_release_input();
                continue;
            }

            if (e.type == SDL_KEYDOWN && !e.key.repeat) {
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    emulator_release_input();
                    continue;
                }

                char c = 0;
                if (e.key.keysym.sym >= SDLK_SPACE && e.key.keysym.sym <= SDLK_z) {
                    c = (char)e.key.keysym.sym;
                    if (e.key.keysym.mod & KMOD_SHIFT) {
                        if (c >= 'a' && c <= 'z') c -= 32;
                    }
                }
                switch (e.key.keysym.sym) {
                    case SDLK_RETURN:     c = '\r'; break;
                    case SDLK_BACKSPACE:  c = '\b'; break;
                    case SDLK_TAB:        c = '\t'; break;
                    case SDLK_ESCAPE:     c = 0x1B; break;
                    case SDLK_DELETE:     c = 0x7F; break;
                    case SDLK_KP_PERIOD:  c = 0x7F; break; // keypad Del (NumLock off)
                    case SDLK_UP:         c = 0x48; break;
                    case SDLK_DOWN:       c = 0x50; break;
                    default: break;
                }
                if (c) {
                    input_feed_key(cpu, c);
                }
            }

            if (e.type == SDL_MOUSEMOTION) {
                input_mouse_move(cpu, e.motion.xrel, e.motion.yrel);
            }

            if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {
                bool pressed = (e.type == SDL_MOUSEBUTTONDOWN);
                input_mouse_button(cpu, e.button.button, pressed);
            }

            continue;
        }

        // Normal (non-captured) mode: events go to ImGui and keyboard/mouse
        // are also fed to the emulated machine so test programs work without
        // explicit capture.
        ImGui_ImplSDL2_ProcessEvent(&e);

        if (e.type == SDL_QUIT) return false;

        if (e.type == SDL_KEYDOWN && !e.key.repeat) {
            char c = 0;
            if (e.key.keysym.sym >= SDLK_SPACE && e.key.keysym.sym <= SDLK_z) {
                c = (char)e.key.keysym.sym;
                if (e.key.keysym.mod & KMOD_SHIFT) {
                    if (c >= 'a' && c <= 'z') c -= 32;
                }
            }
            switch (e.key.keysym.sym) {
                case SDLK_RETURN: c = '\r'; break;
                case SDLK_BACKSPACE: c = '\b'; break;
                case SDLK_TAB: c = '\t'; break;
                case SDLK_ESCAPE: c = 0x1B; break;
                case SDLK_DELETE: c = 0x7F; break;
                case SDLK_KP_PERIOD: c = 0x7F; break; // keypad Del (NumLock off)
                case SDLK_UP:     c = 0x48; break;
                case SDLK_DOWN:   c = 0x50; break;
                default: break;
            }
            if (c) {
                input_feed_key(cpu, c);
            }
        }

        if (e.type == SDL_MOUSEMOTION) {
            input_mouse_move(cpu, e.motion.xrel, e.motion.yrel);
        }

        if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {
            bool pressed = (e.type == SDL_MOUSEBUTTONDOWN);
            input_mouse_button(cpu, e.button.button, pressed);
        }
    }
    return true;
}

void emulator_shutdown() {
    psg_audio_shutdown();

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    if (g_vc) {
        g_vc->shutdown();
        g_vc = nullptr;
    }
    SDL_Quit();
}
