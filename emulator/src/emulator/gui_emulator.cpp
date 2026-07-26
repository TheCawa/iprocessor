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

// Display scale: the native texture is 640x200, but we scale it to 640x400
// for a more typical retro-PC look.
#define DISPLAY_W 640
#define DISPLAY_H 400

static SDL_Window* g_window = nullptr;
static SDL_Renderer* g_renderer = nullptr;
static const VideoCard* g_vc = nullptr;
static const CpuBackend* g_current_backend = nullptr;

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

static void emulator_reset_cpu(Cpu* cpu) {
    cpu_reset(cpu);
    if (g_vc && g_vc->reset) {
        g_vc->reset(cpu);
    }
}

void emulator_set_speed_mode(CpuSpeedMode mode) { g_cpu_speed_mode = mode; }
CpuSpeedMode emulator_get_speed_mode(void) { return g_cpu_speed_mode; }
void emulator_set_fixed_steps(int steps) { g_cpu_fixed_steps = steps > 0 ? steps : 1; }
int  emulator_get_fixed_steps(void) { return g_cpu_fixed_steps; }
void emulator_set_max_steps(int steps) { g_cpu_max_steps = steps > 0 ? steps : 1; }
int  emulator_get_max_steps(void) { return g_cpu_max_steps; }

bool emulator_init(SDL_Window** out_window, SDL_Renderer** out_renderer, const VideoCard* vc) {
    if (!vc) vc = videocard_get_default();
    g_vc = vc;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) return false;

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

bool emulator_switch_cpu(Cpu* cpu, const CpuBackend* new_backend, std::vector<uint8_t>& memory) {
    if (!new_backend || !g_window) return false;

    // Preserve disk image paths across reinit.
    char saved_disk_paths[DISK_MAX_DRIVES][MAX_PATH] = {0};
    for (int i = 0; i < DISK_MAX_DRIVES; i++) {
        if (cpu->disk_drives[i].image_path[0]) {
            strncpy(saved_disk_paths[i], cpu->disk_drives[i].image_path, sizeof(saved_disk_paths[i]) - 1);
        }
    }
    disk_free_image(cpu);

    memory.assign(new_backend->mem_size_default, 0);
    cpu_init(cpu, new_backend, memory.data(), memory.size());
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
    if (g_window) {
        char title[512];
        snprintf(title, sizeof(title), "%s - %s", g_current_backend ? g_current_backend->name : "cpu", g_vc->name);
        SDL_SetWindowTitle(g_window, title);
    }
    return true;
}

void emulator_render(Cpu* cpu, SDL_Renderer* renderer, std::vector<uint8_t>& memory) {
    if (cpu && cpu->backend && !g_current_backend) {
        g_current_backend = cpu->backend;
    }

    if (g_vc) {
        g_vc->update(cpu);
    }

    SDL_Texture* screen_tex = g_vc ? g_vc->get_texture() : nullptr;
    int display_w = DISPLAY_W;

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
                        "Disk images (*.bin;*.hex)\0*.bin;*.hex\0All files (*.*)\0*.*\0",
                        "Open Disk Image")) {
                    if (g_disk_drive_idx >= 0 && g_disk_drive_idx < DISK_MAX_DRIVES) {
                        strncpy(cpu->disk_drives[g_disk_drive_idx].image_path, g_disk_path,
                                sizeof(cpu->disk_drives[g_disk_drive_idx].image_path) - 1);
                        cpu->disk_drives[g_disk_drive_idx].image_path[
                            sizeof(cpu->disk_drives[g_disk_drive_idx].image_path) - 1] = '\0';
                    }
                }
            }
            if (ImGui::MenuItem("Reset CPU")) { emulator_reset_cpu(cpu); g_cpu_running = true; }
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
                        emulator_switch_cpu(cpu, b, memory);
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

            char drive_label[32];
            snprintf(drive_label, sizeof(drive_label), "Drive %d", g_disk_drive_idx);
            if (ImGui::BeginCombo("Disk drive", drive_label)) {
                for (int i = 0; i < DISK_MAX_DRIVES; i++) {
                    char label[32];
                    snprintf(label, sizeof(label), "Drive %d", i);
                    bool selected = (i == g_disk_drive_idx);
                    if (ImGui::Selectable(label, selected)) {
                        g_disk_drive_idx = i;
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
        ImGui::TextDisabled("0x00000000 = BIOS/ROM, 0x00050000 = user program");
        if (g_load_error[0] != '\0') {
            ImGui::TextColored(ImVec4(0.8f, 0.1f, 0.1f, 1.0f), "%s", g_load_error);
        }
        if (ImGui::Button("Load", ImVec2(120, 0))) {
            uint32_t addr = (uint32_t)strtoul(g_load_addr_buf, nullptr, 0);
            if (cpu_load_file(cpu, g_rom_path, addr) == 0) {
                emulator_reset_cpu(cpu);
                cpu_set_reg(cpu, cpu->backend->pc_register, addr);
                if (g_window) {
                    char title[512];
                    snprintf(title, sizeof(title), "%s - %s @ 0x%08X",
                             g_current_backend ? g_current_backend->name : "cpu",
                             g_rom_path, addr);
                    SDL_SetWindowTitle(g_window, title);
                }
                g_cpu_running = true;
                ImGui::CloseCurrentPopup();
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
    if (screen_tex) {
        ImGui::Image((void*)(intptr_t)screen_tex, ImVec2((float)DISPLAY_W, (float)DISPLAY_H));
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
            if (ImGui::Button("Reset", ImVec2(80, 0))) { emulator_reset_cpu(cpu); g_cpu_running = true; }
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

            ImGui::SetNextItemWidth(120.0f);
            ImGui::InputText("Address", mem_addr_buf, sizeof(mem_addr_buf),
                             ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);
            ImGui::SameLine();
            if (ImGui::Button("Go")) {
                dump_addr = (uint32_t)strtoul(mem_addr_buf, nullptr, 0);
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
                dump_addr = 0x00050000;
                snprintf(mem_addr_buf, sizeof(mem_addr_buf), "0x%08X", dump_addr);
            }
            ImGui::Separator();

            if (ImGui::BeginTable("MemDumpTable", 18, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit)) {
                ImGui::TableSetupColumn("Addr", ImGuiTableColumnFlags_WidthFixed, 75.0f);
                for (int i = 0; i < 16; i++) {
                    char col_name[4];
                    snprintf(col_name, sizeof(col_name), "%X", i);
                    ImGui::TableSetupColumn(col_name, ImGuiTableColumnFlags_WidthFixed, 24.0f);
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
                        ImGui::Text("%02X", b);
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

        // Normal (non-captured) mode: events go to ImGui and keyboard is fed
        // to the emulated machine for typing convenience.
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
                case SDLK_ESCAPE: c = 0x1B; break;
                default: break;
            }
            if (c) {
                input_feed_key(cpu, c);
            }
        }
    }
    return true;
}

void emulator_shutdown() {
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    if (g_vc) {
        g_vc->shutdown();
        g_vc = nullptr;
    }
    SDL_Quit();
}
