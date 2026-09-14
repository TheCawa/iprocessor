// ------------------------------------------------------------------------------
//          input.c - Keyboard and mouse input implementation
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

#include "input.h"
#include "system.h"
#include <string.h>

// Current mouse coordinate bounds, derived from the active terminal/video
// resolution (TERM_RES). In graphics modes TERM_RES is the pixel size; in text
// modes it is the character grid, which is what software draws a mouse cursor
// onto. Falls back to the legacy MOUSE_MAX_X/Y (320x200) when no resolution is
// available yet.
static void input_mouse_bounds(Cpu* cpu, int32_t* max_x, int32_t* max_y) {
    if (cpu->update_term_res) cpu->update_term_res(cpu);
    uint32_t w = cpu->term_res_x;
    uint32_t h = cpu->term_res_y;
    *max_x = (w > 0) ? (int32_t)(w - 1) : MOUSE_MAX_X;
    *max_y = (h > 0) ? (int32_t)(h - 1) : MOUSE_MAX_Y;
}

static int32_t input_clamp_coord(int32_t v, int32_t max) {
    if (v < 0) return 0;
    if (v > max) return max;
    return v;
}

void input_init(Cpu* cpu) {
    if (!cpu) return;
    cpu->kbd_buffer_len = 0;
    cpu->kbd_buffer_pos = 0;
    cpu->kbd_irq_pending = false;
    cpu->kbd_modifiers = 0;
    cpu->kbd_tab_down = false;

    // Center the mouse on the active resolution (or the legacy 320x200 space
    // if the resolution is not known yet).
    int32_t mx, my;
    input_mouse_bounds(cpu, &mx, &my);
    cpu->mouse_x = mx / 2;
    cpu->mouse_y = my / 2;
    cpu->mouse_delta_x = 0;
    cpu->mouse_delta_y = 0;
    cpu->mouse_sens = MOUSE_SENS_DEFAULT;
    cpu->mouse_buttons = 0;
    cpu->mouse_irq_pending = false;
}

void input_reset(Cpu* cpu) {
    input_init(cpu);
}

void input_feed_key(Cpu* cpu, char c) {
    input_feed_key_ex(cpu, c, 0);
}

void input_feed_key_ex(Cpu* cpu, char c, uint8_t scancode) {
    if (!cpu) return;
    if (cpu->kbd_buffer_len < (int)sizeof(cpu->kbd_buffer) - 1) {
        int i = cpu->kbd_buffer_len++;
        cpu->kbd_buffer[i] = c;
        cpu->kbd_scancodes[i] = scancode;
        cpu->kbd_irq_pending = true;
    }
}

void input_set_modifiers(Cpu* cpu, uint8_t mod_bits) {
    if (!cpu) return;
    cpu->kbd_modifiers = mod_bits;
}

void input_mouse_move(Cpu* cpu, int dx, int dy) {
    if (!cpu) return;

    // Apply the software sensitivity multiplier (fixed point, 1/256). It scales
    // the host-relative deltas before they move the pointer and accumulate into
    // MOUSE_DELTA_X/Y, so software can speed up or slow down the cursor.
    uint32_t sens = cpu->mouse_sens ? cpu->mouse_sens : MOUSE_SENS_DEFAULT;
    int sx = (int)(((int64_t)dx * (int64_t)sens) >> 8);
    int sy = (int)(((int64_t)dy * (int64_t)sens) >> 8);

    // Clamp to the current resolution so coordinates can exceed the legacy
    // 320x200 limit (e.g. 640x480 or 800x600 graphics modes).
    int32_t mx, my;
    input_mouse_bounds(cpu, &mx, &my);
    cpu->mouse_x = input_clamp_coord(cpu->mouse_x + sx, mx);
    cpu->mouse_y = input_clamp_coord(cpu->mouse_y + sy, my);

    cpu->mouse_delta_x += sx;
    cpu->mouse_delta_y += sy;

    cpu->mouse_irq_pending = true;
}

void input_mouse_button(Cpu* cpu, uint8_t sdl_button, bool pressed) {
    if (!cpu) return;

    uint8_t bit = 0;
    switch (sdl_button) {
        case 1: bit = MOUSE_BTN_LEFT;   break;
        case 3: bit = MOUSE_BTN_RIGHT;  break;
        case 2: bit = MOUSE_BTN_MIDDLE; break;
        default: return;
    }

    if (pressed) {
        cpu->mouse_buttons |= bit;
    } else {
        cpu->mouse_buttons &= ~bit;
    }

    cpu->mouse_irq_pending = true;
}

int input_service_irq(Cpu* cpu) {
    if (!cpu) return -1;

    // Keyboard has highest priority.
    if (cpu->kbd_irq_pending) {
        cpu->kbd_irq_pending = false;
        return KBD_IRQ_VECTOR;
    }

    if (cpu->mouse_irq_pending) {
        cpu->mouse_irq_pending = false;
        return MOUSE_IRQ_VECTOR;
    }

    return -1;
}

static int32_t input_read_dword_signed(Cpu* cpu, uint32_t addr) {
    switch (addr) {
        case MOUSE_X_ADDR: {
            int32_t mx, my;
            input_mouse_bounds(cpu, &mx, &my);
            return input_clamp_coord(cpu->mouse_x, mx);
        }
        case MOUSE_Y_ADDR: {
            int32_t mx, my;
            input_mouse_bounds(cpu, &mx, &my);
            return input_clamp_coord(cpu->mouse_y, my);
        }
        case MOUSE_DELTA_X_ADDR: return (int32_t)cpu->mouse_delta_x;
        case MOUSE_DELTA_Y_ADDR: return (int32_t)cpu->mouse_delta_y;
        case MOUSE_SENS_ADDR:    return (int32_t)cpu->mouse_sens;
        default: return 0;
    }
}

uint8_t input_read_byte(Cpu* cpu, uint32_t addr) {
    if (!cpu) return 0;

    if (addr == MOUSE_BTN_ADDR) {
        return cpu->mouse_buttons;
    }

    int32_t val = input_read_dword_signed(cpu, addr);
    return (uint8_t)(val & 0xFF);
}

uint32_t input_read_dword(Cpu* cpu, uint32_t addr) {
    if (!cpu) return 0;

    if (addr == MOUSE_BTN_ADDR) {
        return (uint32_t)cpu->mouse_buttons;
    }

    return (uint32_t)input_read_dword_signed(cpu, addr);
}

// MOUSE_X/Y are writable: software may reposition the pointer (e.g. center it
// on the screen). Writes are clamped to the current resolution. Byte writes
// patch the low byte of the coordinate, like other 32-bit MMIO registers.
void input_write_dword(Cpu* cpu, uint32_t addr, uint32_t val) {
    if (!cpu) return;
    switch (addr) {
        case MOUSE_X_ADDR: {
            int32_t mx, my;
            input_mouse_bounds(cpu, &mx, &my);
            cpu->mouse_x = input_clamp_coord((int32_t)val, mx);
            break;
        }
        case MOUSE_Y_ADDR: {
            int32_t mx, my;
            input_mouse_bounds(cpu, &mx, &my);
            cpu->mouse_y = input_clamp_coord((int32_t)val, my);
            break;
        }
        case MOUSE_BTN_ADDR:      cpu->mouse_buttons = (uint8_t)(val & 0xFF); break;
        case MOUSE_DELTA_X_ADDR:  cpu->mouse_delta_x = (int32_t)val; break;
        case MOUSE_DELTA_Y_ADDR:  cpu->mouse_delta_y = (int32_t)val; break;
        case MOUSE_SENS_ADDR:     cpu->mouse_sens = val; break;
        default: break;
    }
}

void input_write_byte(Cpu* cpu, uint32_t addr, uint8_t val) {
    if (!cpu) return;
    switch (addr) {
        case MOUSE_X_ADDR: {
            int32_t mx, my;
            input_mouse_bounds(cpu, &mx, &my);
            cpu->mouse_x = input_clamp_coord((int32_t)((uint32_t)cpu->mouse_x & ~0xFFU) | val, mx);
            break;
        }
        case MOUSE_Y_ADDR: {
            int32_t mx, my;
            input_mouse_bounds(cpu, &mx, &my);
            cpu->mouse_y = input_clamp_coord((int32_t)((uint32_t)cpu->mouse_y & ~0xFFU) | val, my);
            break;
        }
        case MOUSE_BTN_ADDR:      cpu->mouse_buttons = val; break;
        case MOUSE_DELTA_X_ADDR:  cpu->mouse_delta_x = (int32_t)((uint32_t)cpu->mouse_delta_x & ~0xFFU) | val; break;
        case MOUSE_DELTA_Y_ADDR:  cpu->mouse_delta_y = (int32_t)((uint32_t)cpu->mouse_delta_y & ~0xFFU) | val; break;
        case MOUSE_SENS_ADDR:     cpu->mouse_sens = (uint32_t)((uint32_t)cpu->mouse_sens & ~0xFFU) | val; break;
        default: break;
    }
}
