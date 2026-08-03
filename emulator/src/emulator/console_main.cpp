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
#include <iomanip>
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <windows.h>
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include "backends.h"
#include "pit.h"
#include "term_res.h"

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

static void print_usage(const char* prog) {
    std::cerr << "Usage: " << prog << " [--cpu <name>] [--bios <path>] [--ram <size>] [--vram <size>] <file> [load_addr] [disk_image]\n";
    std::cerr << "\nExamples:\n";
    std::cerr << "  " << prog << " --cpu i80148 program.bin 0x00060000 disk.bin\n";
    std::cerr << "  " << prog << " --cpu i8046  program.bin 0x0000\n";
    std::cerr << "  " << prog << " --bios ../i80148/PC48/Programs/CBIOS/CBIOS.bin program.bin\n";
    std::cerr << "  " << prog << " --ram 16M --vram 1M program.bin\n";
}

static void print_registers(const CpuBackend* backend, Cpu* cpu) {
    if (!backend || !backend->register_names || backend->register_count <= 0) return;
    std::cout << "\n=== Registers ===\n";
    int cols = (backend->register_count <= 16) ? 2 : 3;
    for (int i = 0; i < backend->register_count; i++) {
        const char* name = backend->register_names[i] ? backend->register_names[i] : "?";
        std::cout << std::left << std::setw(6) << name << std::setfill(' ') << "= 0x"
                  << std::right << std::hex << std::setfill('0') << std::setw(8)
                  << (uint32_t)cpu_get_reg(cpu, (uint8_t)i) << std::dec << std::setfill(' ');
        if ((i + 1) % cols == 0) std::cout << "\n";
        else std::cout << "  ";
    }
    if (backend->register_count % cols != 0) std::cout << "\n";
}

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(65001);

    // Register available CPU backends.
    cpu_backend_register(&g_cpu_backend_i80148);
    cpu_backend_register(&g_cpu_backend_i8046);

    const char* cpu_name = "i80148"; // default
    const char* filename = nullptr;
    uint32_t load_addr = 0;
    bool load_addr_set = false;
    const char* disk_image = nullptr;
    const char* bios_path = nullptr;
    const char* ram_size_str = nullptr;
    const char* vram_size_str = nullptr;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--ram") == 0) {
            if (i + 1 < argc) {
                ram_size_str = argv[++i];
            } else {
                std::cerr << "[ERROR] --ram requires a size (e.g. 16M, 512K)\n";
                return 1;
            }
        } else if (strcmp(argv[i], "--vram") == 0) {
            if (i + 1 < argc) {
                vram_size_str = argv[++i];
            } else {
                std::cerr << "[ERROR] --vram requires a size (e.g. 512K, 1M)\n";
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

    size_t ram_size = ram_size_str ? cpu_parse_size(ram_size_str) : backend->mem_size_default;
    if (ram_size == 0) ram_size = backend->mem_size_default;
    size_t vram_size = vram_size_str ? cpu_parse_size(vram_size_str) : backend->vram_size_default;
    if (vram_size == 0) vram_size = backend->vram_size_default;

    std::vector<uint8_t> memory(ram_size, 0);
    std::vector<uint8_t> vram(vram_size, 0);
    Cpu cpu;
    cpu_init(&cpu, backend, memory.data(), ram_size, vram.data(), vram_size);
    cpu.update_term_res = term_res_update;
    pit_init(&cpu);

    if (disk_image) {
        std::strncpy(cpu.disk_drives[0].image_path, disk_image, sizeof(cpu.disk_drives[0].image_path) - 1);
        cpu.disk_drives[0].image_path[sizeof(cpu.disk_drives[0].image_path) - 1] = '\0';
    }

    bool run_program = (filename != nullptr);
    bool rom_override = run_program && load_addr_set && (load_addr == 0x00000000);

    if (!run_program) {
        // No program given: load default BIOS and boot from it.
        std::string resolved_bios = resolve_bios_path(bios_path);
        if (!resolved_bios.empty()) {
            if (cpu_load_file(&cpu, resolved_bios.c_str(), 0x00000000) != 0) {
                std::cerr << "[WARN] Failed to load BIOS from " << resolved_bios << "\n";
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
                return 1;
            }
        } else {
            uint32_t user_ram = backend->user_ram_start;
            if (!load_addr_set) {
                load_addr = user_ram;
            }
            if (load_addr < user_ram) {
                std::cerr << "[ERROR] User programs cannot be loaded below 0x" << std::hex << user_ram
                          << std::dec << " (reserved for system ROM/MMIO/VRAM window).\n";
                return 1;
            }
            if (cpu_load_file(&cpu, filename, load_addr) != 0) {
                std::cerr << "[ERROR] Failed to load binary file!\n";
                return 1;
            }
        }
    }

    // Set execution entry point: BIOS/ROM starts at 0, user programs start at load_addr.
    if (rom_override || !run_program) {
        cpu_set_reg(&cpu, backend->pc_register, 0x00000000);
    } else {
        cpu_set_reg(&cpu, backend->pc_register, load_addr);
    }

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

    print_registers(backend, &cpu);

    return 0;
}
