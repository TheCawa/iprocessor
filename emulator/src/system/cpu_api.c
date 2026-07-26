// ------------------------------------------------------------------------------
//          cpu_api.c - Common CPU API implementation
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

#include "cpu_api.h"
#include "input.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BACKENDS 16
static const CpuBackend* g_backends[MAX_BACKENDS];
static int g_backend_count = 0;

void cpu_backend_register(const CpuBackend* backend) {
    if (!backend || g_backend_count >= MAX_BACKENDS) return;
    g_backends[g_backend_count++] = backend;
}

const CpuBackend* cpu_backend_find(const char* name) {
    if (!name) return NULL;
    for (int i = 0; i < g_backend_count; i++) {
        if (g_backends[i] && strcmp(g_backends[i]->name, name) == 0) {
            return g_backends[i];
        }
    }
    return NULL;
}

void cpu_backend_print_list(void) {
    printf("Available CPU backends:\n");
    for (int i = 0; i < g_backend_count; i++) {
        if (g_backends[i]) {
            printf("  %s - %s\n", g_backends[i]->name, g_backends[i]->description);
        }
    }
}

int cpu_backend_count(void) { return g_backend_count; }

const CpuBackend* cpu_backend_at(int idx) {
    if (idx < 0 || idx >= g_backend_count) return NULL;
    return g_backends[idx];
}

void cpu_init(Cpu* cpu, const CpuBackend* backend, uint8_t* mem, size_t mem_size) {
    memset(cpu, 0, sizeof(Cpu));
    cpu->mem = mem;
    cpu->mem_size = mem_size;
    cpu->backend = backend;
    if (backend && backend->state_size > 0) {
        cpu->backend_data = calloc(1, backend->state_size);
    }
    if (backend && backend->init) {
        backend->init(cpu);
    }
}

void cpu_reset(Cpu* cpu) {
    if (cpu->backend && cpu->backend->reset) {
        cpu->backend->reset(cpu);
    }
}

int cpu_step(Cpu* cpu) {
    if (!cpu->backend || !cpu->backend->step) return -1;
    return cpu->backend->step(cpu);
}

int cpu_load_file(Cpu* cpu, const char* filename, uint32_t load_addr) {
    if (!cpu->backend || !cpu->backend->load_file) return -1;
    return cpu->backend->load_file(cpu, filename, load_addr);
}

uint64_t cpu_get_reg(Cpu* cpu, uint8_t idx) {
    if (!cpu->backend || !cpu->backend->get_reg) return 0;
    return cpu->backend->get_reg(cpu, idx);
}

void cpu_set_reg(Cpu* cpu, uint8_t idx, uint64_t val) {
    if (!cpu->backend || !cpu->backend->set_reg) return;
    cpu->backend->set_reg(cpu, idx, val);
}

uint64_t cpu_read_mem(Cpu* cpu, uint32_t addr, int mode) {
    if (!cpu->backend || !cpu->backend->read_mem) return 0;
    return cpu->backend->read_mem(cpu, addr, mode);
}

void cpu_write_mem(Cpu* cpu, uint32_t addr, uint64_t val, int mode) {
    if (!cpu->backend || !cpu->backend->write_mem) return;
    cpu->backend->write_mem(cpu, addr, val, mode);
}

bool cpu_check_cond(Cpu* cpu, uint8_t cond) {
    if (!cpu->backend || !cpu->backend->check_cond) return false;
    return cpu->backend->check_cond(cpu, cond);
}

void cpu_feed_key(Cpu* cpu, char c) {
    input_feed_key(cpu, c);
}
