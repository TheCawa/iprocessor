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
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include "backends.h"
#include "emulator.hpp"
#include "videocard.h"
#include "videocards.h"
#include "pit.h"
#include <windows.h>

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(65001);

    // Register available CPU backends.
    cpu_backend_register(&g_cpu_backend_i80148);
    cpu_backend_register(&g_cpu_backend_i8046);

    const VideoCard* vc = videocard_get_default();

    // Parse arguments. Usage: emulator [--cpu name] <file> [load_addr] [disk_image] [--vc name]
    const char* filename = nullptr;
    uint32_t load_addr = 0x00000000;
    bool load_addr_set = false;
    const char* disk_image = nullptr;
    const char* cpu_name = "i80148";

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

    if (filename) {
        if (cpu_load_file(&cpu, filename, load_addr) != 0) {
            std::cerr << "[ERROR] Failed to load binary file!\n";
            emulator_shutdown();
            return 1;
        }
        cpu_set_reg(&cpu, backend->pc_register, load_addr);

        if (disk_image) {
            std::strncpy(cpu.disk_drives[0].image_path, disk_image, sizeof(cpu.disk_drives[0].image_path) - 1);
            cpu.disk_drives[0].image_path[sizeof(cpu.disk_drives[0].image_path) - 1] = '\0';
        }
    } else {
        // Test stub: write "HI!" into video buffer
        memory[backend->vbuffer_base + 0] = 'H'; memory[backend->vbuffer_base + 1] = 0x1F;
        memory[backend->vbuffer_base + 2] = 'I'; memory[backend->vbuffer_base + 3] = 0x1F;
        memory[backend->vbuffer_base + 4] = '!'; memory[backend->vbuffer_base + 5] = 0x2F;
        cpu.screen_dirty = true;
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
