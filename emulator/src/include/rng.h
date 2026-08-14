// ------------------------------------------------------------------------------
//          rng.h - Hardware random number generator header
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

#ifndef RNG_H
#define RNG_H

#include "cpu_api.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// PC48 hardware RNG MMIO addresses.
#define RNG_ADDR_BASE  0x00020080
#define RNG_ADDR_RAND  0x00020080  // read: next random value
#define RNG_ADDR_SEED  0x00020081  // write: reseed the generator

// Default seed used on reset.
#define RNG_DEFAULT_SEED 0x12345678U

// Initialize / reset RNG state.
void rng_init(Cpu* cpu);
void rng_reset(Cpu* cpu);

// Read a random byte or dword from the generator.
uint8_t  rng_read_byte(Cpu* cpu, uint32_t addr);
uint32_t rng_read_dword(Cpu* cpu, uint32_t addr);

// Write a new seed. Writing RAND is ignored.
void rng_write_byte(Cpu* cpu, uint32_t addr, uint8_t val);
void rng_write_dword(Cpu* cpu, uint32_t addr, uint32_t val);

// Returns true if addr belongs to the RNG MMIO region.
static inline bool rng_is_mmio(uint32_t addr) {
    return (addr >= RNG_ADDR_BASE && addr <= RNG_ADDR_SEED);
}

#ifdef __cplusplus
}
#endif

#endif // RNG_H
