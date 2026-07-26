// ------------------------------------------------------------------------------
//          gui_main.cpp - GUI emulator main entry point
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

#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include "backends.h"
#include "emulator.hpp"
#include "videocard.h"
#include "videocards.h"
#include "pit.h"
#include <windows.h>

static std::string get_exe_dir() {
    char buf[MAX_PATH];
    DWORD len = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) return "";
    for (int i = (int)len - 1; i >= 0; i--) {
        if (buf[i] == '\\' || buf[i] == '/') {
            buf[i] = '\0';
            break;
        }
    }
    return std::string(buf);
}

static std::string resolve_bios_path(const char* user_path) {
    if (user_path) {
        FILE* f = fopen(user_path, "rb");
        if (f) { fclose(f); return user_path; }
        return user_path;
    }
    std::string exe_dir = get_exe_dir();
    const char* candidates[] = {
        "CBIOS.bin",
        "CBIOS/CBIOS.bin",
        "../i80148/PC48/Programs/CBIOS/CBIOS.bin",
        "../../i80148/PC48/Programs/CBIOS/CBIOS.bin"
    };
    for (const char* cand : candidates) {
        std::string path = exe_dir.empty() ? cand : (exe_dir + "/" + cand);
        FILE* f = fopen(path.c_str(), "rb");
        if (f) { fclose(f); return path; }
    }
    return "";
}

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(65001);

    // Register available CPU backends.
    cpu_backend_register(&g_cpu_backend_i80148);
    cpu_backend_register(&g_cpu_backend_i8046);

    const VideoCard* vc = videocard_get_default();

    // Parse arguments. Usage: emulator [--cpu name] [--bios path] <file> [load_addr] [disk_image] [--vc name]
    const char* filename = nullptr;
    uint32_t load_addr = 0;
    bool load_addr_set = false;
    const char* disk_image = nullptr;
    const char* cpu_name = "i80148";
    const char* bios_path = nullptr;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--vc") == 0) {
            if (i + 1 < argc) {
                const char* name = argv[++i];
                const VideoCard* chosen = videocard_find_by_name(name);
                if (!chosen) {
                    std::cerr << "[ERROR] Unknown video card: " << name << "\n";
                    videocard_print_list();
                    return 1;
                }
                vc = chosen;
            } else {
                std::cerr << "[ERROR] --vc requires a video card name\n";
                videocard_print_list();
                return 1;
            }
        } else if (strcmp(argv[i], "--cpu") == 0) {
            if (i + 1 < argc) {
                cpu_name = argv[++i];
            } else {
                std::cerr << "[ERROR] --cpu requires a CPU name\n";
                cpu_backend_print_list();
                return 1;
            }
        } else if (strcmp(argv[i], "--bios") == 0) {
            if (i + 1 < argc) {
                bios_path = argv[++i];
            } else {
                std::cerr << "[ERROR] --bios requires a path\n";
                return 1;
            }
        } else if (!filename) {
            filename = argv[i];
        } else if (!load_addr_set) {
            load_addr = (uint32_t)strtoul(argv[i], nullptr, 0);
            load_addr_set = true;
        } else {
            disk_image = argv[i];
        }
    }

    const CpuBackend* backend = cpu_backend_find(cpu_name);
    if (!backend) {
        std::cerr << "[ERROR] Unknown CPU backend: " << cpu_name << "\n";
        cpu_backend_print_list();
        return 1;
    }

    size_t ram_size = backend->mem_size_default;
    std::vector<uint8_t> memory(ram_size, 0);
    Cpu cpu;

    cpu_init(&cpu, backend, memory.data(), ram_size);
    cpu.gui_mode = true;
    pit_init(&cpu);

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (!emulator_init(&window, &renderer, vc)) {
        std::cerr << "[ERROR] Failed to initialize graphics subsystem!\n";
        return 1;
    }

    std::cout << "=== Iprocessor Emulator Started ===\n";
    std::cout << "CPU: " << backend->description << "\n";
    std::cout << "Video card: " << vc->name << " - " << vc->description << "\n";

    bool run_program = (filename != nullptr);
    bool rom_override = run_program && load_addr_set && (load_addr == 0x00000000);

    if (!run_program) {
        // No program given: load default BIOS and boot from it.
        std::string resolved_bios = resolve_bios_path(bios_path);
        if (!resolved_bios.empty()) {
            if (cpu_load_file(&cpu, resolved_bios.c_str(), 0x00000000) != 0) {
                std::cerr << "[WARN] Failed to load BIOS from " << resolved_bios << "\n";
                // Continue anyway; user may supply their own ROM via the GUI.
            }
        } else {
            std::cerr << "[WARN] No default BIOS found. Use --bios <path> to specify one.\n";
        }
    }

    if (filename) {
        if (rom_override) {
            // Explicit ROM image replaces the default BIOS.
            if (cpu_load_file(&cpu, filename, 0x00000000) != 0) {
                std::cerr << "[ERROR] Failed to load ROM/BIOS file!\n";
                emulator_shutdown();
                return 1;
            }
        } else {
            // User programs always load into RAM unless an address was explicitly given.
            uint32_t user_ram = backend->user_ram_start;
            if (!load_addr_set) {
                load_addr = user_ram;
            }
            if (load_addr < user_ram) {
                std::cerr << "[ERROR] User programs cannot be loaded below 0x" << std::hex << user_ram
                          << std::dec << " (reserved for system ROM/MMIO/VRAM window).\n";
                emulator_shutdown();
                return 1;
            }
            if (cpu_load_file(&cpu, filename, load_addr) != 0) {
                std::cerr << "[ERROR] Failed to load binary file!\n";
                emulator_shutdown();
                return 1;
            }
        }

        if (disk_image) {
            std::strncpy(cpu.disk_drives[0].image_path, disk_image, sizeof(cpu.disk_drives[0].image_path) - 1);
            cpu.disk_drives[0].image_path[sizeof(cpu.disk_drives[0].image_path) - 1] = '\0';
        }
    }

    // Set execution entry point: BIOS/ROM starts at 0, user programs start at load_addr.
    if (rom_override || !run_program) {
        cpu_set_reg(&cpu, backend->pc_register, 0x00000000);
    } else {
        cpu_set_reg(&cpu, backend->pc_register, load_addr);
    }

    bool running = true;
    Uint32 last_frame_time = SDL_GetTicks();
    while (running) {
        if (!emulator_handle_events(&cpu)) {
            running = false;
        }

        Uint32 now = SDL_GetTicks();
        Uint32 frame_elapsed = now - last_frame_time;
        last_frame_time = now;
        Uint32 frame_start = now;

        pit_step(&cpu, frame_elapsed);

        if (!cpu.halted) {
            if (emulator_is_running()) {
                CpuSpeedMode mode = emulator_get_speed_mode();
                int fixed_steps = emulator_get_fixed_steps();
                int max_steps = emulator_get_max_steps();
                int budget_ms = (mode == CPU_SPEED_ADAPTIVE) ? 10 : 0;
                int steps = 0;

                while (!cpu.halted && steps < max_steps) {
                    if (cpu_step(&cpu) <= 0) break;
                    steps++;

                    if (mode == CPU_SPEED_FIXED && steps >= fixed_steps) break;
                    if (mode == CPU_SPEED_ADAPTIVE && (int)(SDL_GetTicks() - frame_start) >= budget_ms) break;
                }
            } else if (emulator_consume_step()) {
                cpu_step(&cpu);
            }
        }

        emulator_render(&cpu, renderer, memory);
        Uint32 render_elapsed = SDL_GetTicks() - frame_start;
        if (render_elapsed < 16) {
            SDL_Delay(16 - render_elapsed);
        }
    }

    pit_shutdown(&cpu);
    emulator_shutdown();
    std::cout << "Emulator closed safely.\n";
    return 0;
}
