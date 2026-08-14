// ------------------------------------------------------------------------------
//          rng.c - Hardware random number generator implementation
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

#include "rng.h"
#include <stdlib.h>

// Simple 32-bit LCG. Good enough for a retro "hardware" RNG and fully
// deterministic when a seed is set, which is important for reproducible tests.
static uint32_t rng_next(Cpu* cpu) {
    cpu->rng_state = cpu->rng_state * 1103515245U + 12345U;
    return cpu->rng_state;
}

void rng_init(Cpu* cpu) {
    if (!cpu) return;
    cpu->rng_state = RNG_DEFAULT_SEED;
}

void rng_reset(Cpu* cpu) {
    rng_init(cpu);
}

uint8_t rng_read_byte(Cpu* cpu, uint32_t addr) {
    if (!cpu) return 0;
    if (addr == RNG_ADDR_RAND) {
        return (uint8_t)(rng_next(cpu) >> 16);
    }
    return 0;
}

uint32_t rng_read_dword(Cpu* cpu, uint32_t addr) {
    if (!cpu) return 0;
    if (addr == RNG_ADDR_RAND) {
        return rng_next(cpu);
    }
    return 0;
}

void rng_write_byte(Cpu* cpu, uint32_t addr, uint8_t val) {
    if (!cpu) return;
    if (addr == RNG_ADDR_SEED) {
        cpu->rng_state = (cpu->rng_state & 0xFFFFFF00U) | (uint32_t)val;
    }
}

void rng_write_dword(Cpu* cpu, uint32_t addr, uint32_t val) {
    if (!cpu) return;
    if (addr == RNG_ADDR_SEED) {
        cpu->rng_state = val;
    }
}
