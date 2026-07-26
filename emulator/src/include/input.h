// ------------------------------------------------------------------------------
//          input.h - Keyboard and mouse input header
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

#ifndef INPUT_H
#define INPUT_H

#include "cpu_api.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Input subsystem: keyboard + mouse IRQs and MMIO state.
// The keyboard is also accessible via the legacy ASCII MMIO port
// (KBD_ASCII_ADDR_I80148 in the i80148 backend).

// IRQ vectors used by the input subsystem.
#define KBD_IRQ_VECTOR      0x21
#define MOUSE_IRQ_VECTOR    0x22

// Mouse MMIO registers (relative to any CPU backend's I/O space).
#define MOUSE_X_ADDR        0x00020040
#define MOUSE_Y_ADDR        0x00020044
#define MOUSE_BTN_ADDR      0x00020048
#define MOUSE_DELTA_X_ADDR  0x0002004C
#define MOUSE_DELTA_Y_ADDR  0x00020050
#define MOUSE_ADDR_END      0x00020053

// Mouse button bits written to MOUSE_BTN_ADDR.
#define MOUSE_BTN_LEFT      0x01
#define MOUSE_BTN_RIGHT     0x02
#define MOUSE_BTN_MIDDLE    0x04

// Logical screen limits for absolute mouse coordinates.
#define MOUSE_MAX_X         319
#define MOUSE_MAX_Y         199

// Initialize / reset input state. Called from CPU backend init/reset.
void input_init(Cpu* cpu);
void input_reset(Cpu* cpu);

// Feed a key press.  Generates KBD_IRQ_VECTOR if IRQs are enabled.
void input_feed_key(Cpu* cpu, char c);

// Feed mouse movement in relative mode.  Generates MOUSE_IRQ_VECTOR.
void input_mouse_move(Cpu* cpu, int dx, int dy);

// Feed mouse button change.  Generates MOUSE_IRQ_VECTOR.
void input_mouse_button(Cpu* cpu, uint8_t sdl_button, bool pressed);

// Service pending input IRQs.  Returns a vector or -1 if none pending.
// The caller is responsible for checking cpu->irq_enabled.
int input_service_irq(Cpu* cpu);

// Check whether an address belongs to the mouse MMIO range.
static inline bool input_is_mmio(uint32_t addr) {
    return (addr >= MOUSE_X_ADDR && addr <= MOUSE_ADDR_END);
}

// MMIO accessors.
uint8_t  input_read_byte(Cpu* cpu, uint32_t addr);
uint32_t input_read_dword(Cpu* cpu, uint32_t addr);

#ifdef __cplusplus
}
#endif

#endif // INPUT_H
