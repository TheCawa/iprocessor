// ------------------------------------------------------------------------------
//          pit.h - Programmable interval timer header
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

#ifndef PIT_H
#define PIT_H

#include "cpu_api.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// PC48 PIT MMIO addresses (0x00020030 - 0x0002003F)
#define PIT_ADDR_BASE       0x00020030
#define PIT_ADDR_COMMAND    0x00020030
#define PIT_ADDR_CH0        0x00020031
#define PIT_ADDR_CH1        0x00020032
#define PIT_ADDR_CH2        0x00020033
#define PIT_ADDR_STATUS     0x00020034

// Default PIT frequency: 1 kHz (1 ms per tick)
#define PIT_FREQUENCY_HZ    1000

// Timer IRQ vector used by i80148
#define PIT_IRQ_VECTOR      0x20

// Initialize PIT state for the given CPU.
void pit_init(Cpu* cpu);

// Free PIT state.
void pit_shutdown(Cpu* cpu);

// Advance the PIT by elapsed_ms milliseconds. Should be called once per frame.
void pit_step(Cpu* cpu, uint32_t elapsed_ms);

// Read a PIT register.
uint8_t  pit_read_byte(Cpu* cpu, uint32_t addr);
uint32_t pit_read_dword(Cpu* cpu, uint32_t addr);

// Write a PIT register.
void pit_write_byte(Cpu* cpu, uint32_t addr, uint8_t val);
void pit_write_dword(Cpu* cpu, uint32_t addr, uint32_t val);

// Returns true if addr belongs to PIT MMIO region.
static inline bool pit_is_mmio(uint32_t addr) {
    return (addr >= PIT_ADDR_BASE && addr <= 0x0002003F);
}

// Check for pending timer IRQ. Returns IRQ vector (PIT_IRQ_VECTOR) or -1 if none.
int pit_service_irq(Cpu* cpu);

// Clear any pending IRQ (used on CPU reset).
void pit_reset_pending(Cpu* cpu);

// State serialization helpers (used by save-state/load-state).
size_t pit_state_size(void);
void   pit_save_state(Cpu* cpu, void* out_buf);
void   pit_load_state(Cpu* cpu, const void* in_buf);

#ifdef __cplusplus
}
#endif

#endif // PIT_H
