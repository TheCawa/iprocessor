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

void input_init(Cpu* cpu) {
    if (!cpu) return;
    cpu->kbd_buffer_len = 0;
    cpu->kbd_buffer_pos = 0;
    cpu->kbd_irq_pending = false;

    cpu->mouse_x = MOUSE_MAX_X / 2;
    cpu->mouse_y = MOUSE_MAX_Y / 2;
    cpu->mouse_delta_x = 0;
    cpu->mouse_delta_y = 0;
    cpu->mouse_buttons = 0;
    cpu->mouse_irq_pending = false;
}

void input_reset(Cpu* cpu) {
    input_init(cpu);
}

void input_feed_key(Cpu* cpu, char c) {
    if (!cpu) return;
    if (cpu->kbd_buffer_len < (int)sizeof(cpu->kbd_buffer) - 1) {
        cpu->kbd_buffer[cpu->kbd_buffer_len++] = c;
        cpu->kbd_irq_pending = true;
    }
}

void input_mouse_move(Cpu* cpu, int dx, int dy) {
    if (!cpu) return;

    cpu->mouse_x += dx;
    cpu->mouse_y += dy;

    if (cpu->mouse_x < 0) cpu->mouse_x = 0;
    if (cpu->mouse_x > MOUSE_MAX_X) cpu->mouse_x = MOUSE_MAX_X;
    if (cpu->mouse_y < 0) cpu->mouse_y = 0;
    if (cpu->mouse_y > MOUSE_MAX_Y) cpu->mouse_y = MOUSE_MAX_Y;

    cpu->mouse_delta_x += dx;
    cpu->mouse_delta_y += dy;

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
        case MOUSE_X_ADDR:       return (int32_t)cpu->mouse_x;
        case MOUSE_Y_ADDR:       return (int32_t)cpu->mouse_y;
        case MOUSE_DELTA_X_ADDR: return (int32_t)cpu->mouse_delta_x;
        case MOUSE_DELTA_Y_ADDR: return (int32_t)cpu->mouse_delta_y;
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
