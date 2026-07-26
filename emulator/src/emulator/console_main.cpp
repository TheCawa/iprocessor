// ------------------------------------------------------------------------------
//          console_main.cpp - Console emulator main entry point
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
#include <windows.h>
#include <SDL.h>
#include "backends.h"
#include "pit.h"

static void print_usage(const char* prog) {
    std::cerr << "Usage: " << prog << " [--cpu <name>] <file> [load_addr] [disk_image]\n";
    std::cerr << "\nExamples:\n";
    std::cerr << "  " << prog << " --cpu i80148 program.bin 0x00050000 disk.bin\n";
    std::cerr << "  " << prog << " --cpu i8046  program.bin 0x0000\n";
}

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(65001);

    // Register available CPU backends.
    cpu_backend_register(&g_cpu_backend_i80148);
    cpu_backend_register(&g_cpu_backend_i8046);

    const char* cpu_name = "i80148"; // default
    const char* filename = nullptr;
    uint32_t load_addr = 0x00000000;
    bool load_addr_set = false;
    const char* disk_image = nullptr;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--cpu") == 0) {
            if (i + 1 < argc) {
                cpu_name = argv[++i];
            } else {
                std::cerr << "[ERROR] --cpu requires a CPU name\n";
                cpu_backend_print_list();
                return 1;
            }
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            cpu_backend_print_list();
            return 0;
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

    if (!filename) {
        print_usage(argv[0]);
        cpu_backend_print_list();
        return 1;
    }

    size_t ram_size = backend->mem_size_default;
    std::vector<uint8_t> memory(ram_size, 0);
    Cpu cpu;
    cpu_init(&cpu, backend, memory.data(), ram_size);
    pit_init(&cpu);

    if (disk_image) {
        std::strncpy(cpu.disk_drives[0].image_path, disk_image, sizeof(cpu.disk_drives[0].image_path) - 1);
        cpu.disk_drives[0].image_path[sizeof(cpu.disk_drives[0].image_path) - 1] = '\0';
    }

    if (cpu_load_file(&cpu, filename, load_addr) != 0) {
        std::cerr << "[ERROR] Failed to load binary file!\n";
        return 1;
    }
    cpu_set_reg(&cpu, backend->pc_register, load_addr);

    std::cout << "=== " << backend->description << " Console Emulator Started ===\n";

    const int MAX_STEPS = 100000000;
    int steps = 0;
    Uint64 pit_freq = SDL_GetPerformanceFrequency();
    Uint64 last_pit_counter = SDL_GetPerformanceCounter();
    while (!cpu.halted && steps < MAX_STEPS) {
        Uint64 now = SDL_GetPerformanceCounter();
        Uint64 elapsed_us = (now - last_pit_counter) * 1000000 / pit_freq;
        if (elapsed_us >= 1000) {
            Uint32 elapsed_ms = (Uint32)(elapsed_us / 1000);
            pit_step(&cpu, elapsed_ms);
            last_pit_counter = now;
        }

        if (cpu_step(&cpu) <= 0) {
            if (!cpu.halted) {
                std::cerr << "[ERROR] cpu_step failed at IC=0x" << std::hex << cpu_get_reg(&cpu, backend->pc_register) << std::dec << "\n";
            }
            break;
        }
        steps++;
    }

    pit_shutdown(&cpu);

    if (cpu.halted) {
        std::cout << "\n=== CPU Halted after " << steps << " steps ===\n";
    } else {
        std::cout << "\n=== Step limit reached (" << steps << " steps) ===\n";
    }

    return 0;
}
