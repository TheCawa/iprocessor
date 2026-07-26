// ------------------------------------------------------------------------------
//          pit.c - Programmable interval timer implementation
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

#include "pit.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint32_t divisor;      // reload value in ms (0 = disabled)
    uint32_t counter;      // current counter
    bool     irq_pending;
} PitChannel;

typedef struct {
    PitChannel ch[3];
    uint8_t    command;
} PitState;

static PitState* pit_state(Cpu* cpu) {
    return (PitState*)cpu->pit_data;
}

void pit_init(Cpu* cpu) {
    if (!cpu) return;
    if (cpu->pit_data) pit_shutdown(cpu);
    PitState* pit = (PitState*)calloc(1, sizeof(PitState));
    if (!pit) return;
    pit->ch[0].divisor = 1000;  // 1 Hz default (1000 ms)
    pit->ch[0].counter = 1000;
    cpu->pit_data = pit;
}

void pit_shutdown(Cpu* cpu) {
    if (!cpu || !cpu->pit_data) return;
    free(cpu->pit_data);
    cpu->pit_data = NULL;
}

void pit_step(Cpu* cpu, uint32_t elapsed_ms) {
    PitState* pit = pit_state(cpu);
    if (!pit) return;

    for (int c = 0; c < 3; c++) {
        PitChannel* ch = &pit->ch[c];
        if (ch->divisor == 0) continue;

        // Accumulate elapsed time against the counter.
        if (elapsed_ms >= ch->counter) {
            ch->irq_pending = true;
            uint32_t remaining = elapsed_ms - ch->counter;
            // Handle multiple ticks if frame took longer than one period.
            while (remaining >= ch->divisor && ch->divisor > 0) {
                remaining -= ch->divisor;
            }
            ch->counter = ch->divisor - remaining;
        } else {
            ch->counter -= elapsed_ms;
        }
    }
}

static PitChannel* pit_get_channel(Cpu* cpu, uint32_t addr) {
    PitState* pit = pit_state(cpu);
    if (!pit) return NULL;
    int idx = (int)(addr - PIT_ADDR_CH0);
    if (idx < 0 || idx >= 3) return NULL;
    return &pit->ch[idx];
}

uint8_t pit_read_byte(Cpu* cpu, uint32_t addr) {
    if (addr == PIT_ADDR_STATUS) {
        PitState* pit = pit_state(cpu);
        if (!pit) return 0;
        uint8_t status = 0;
        if (pit->ch[0].irq_pending) status |= 0x01;
        return status;
    }
    PitChannel* ch = pit_get_channel(cpu, addr);
    if (!ch) return 0;
    return (uint8_t)(ch->counter & 0xFF);
}

uint32_t pit_read_dword(Cpu* cpu, uint32_t addr) {
    if (addr == PIT_ADDR_STATUS) {
        PitState* pit = pit_state(cpu);
        if (!pit) return 0;
        uint32_t status = 0;
        if (pit->ch[0].irq_pending) status |= 0x01;
        return status;
    }
    PitChannel* ch = pit_get_channel(cpu, addr);
    if (!ch) return 0;
    return ch->counter;
}

void pit_write_byte(Cpu* cpu, uint32_t addr, uint8_t val) {
    if (addr == PIT_ADDR_COMMAND) {
        PitState* pit = pit_state(cpu);
        if (pit) pit->command = val;
        return;
    }
    if (addr == PIT_ADDR_STATUS) {
        PitChannel* ch = pit_get_channel(cpu, PIT_ADDR_CH0);
        if (ch) ch->irq_pending = false;
        return;
    }
    PitChannel* ch = pit_get_channel(cpu, addr);
    if (!ch) return;
    ch->divisor = val;
    if (ch->divisor == 0) ch->divisor = 1;
    ch->counter = ch->divisor;
}

void pit_write_dword(Cpu* cpu, uint32_t addr, uint32_t val) {
    if (addr == PIT_ADDR_COMMAND) {
        PitState* pit = pit_state(cpu);
        if (pit) pit->command = (uint8_t)val;
        return;
    }
    if (addr == PIT_ADDR_STATUS) {
        PitChannel* ch = pit_get_channel(cpu, PIT_ADDR_CH0);
        if (ch) ch->irq_pending = false;
        return;
    }
    PitChannel* ch = pit_get_channel(cpu, addr);
    if (!ch) return;
    ch->divisor = val;
    if (ch->divisor == 0) ch->divisor = 1;
    ch->counter = ch->divisor;
}

int pit_service_irq(Cpu* cpu) {
    PitState* pit = pit_state(cpu);
    if (!pit) return -1;
    if (pit->ch[0].irq_pending) {
        pit->ch[0].irq_pending = false;
        return PIT_IRQ_VECTOR;
    }
    return -1;
}

void pit_reset_pending(Cpu* cpu) {
    PitState* pit = pit_state(cpu);
    if (!pit) return;
    pit->ch[0].irq_pending = false;
}

size_t pit_state_size(void) {
    return sizeof(PitState);
}

void pit_save_state(Cpu* cpu, void* out_buf) {
    PitState* pit = pit_state(cpu);
    if (pit && out_buf) {
        memcpy(out_buf, pit, sizeof(PitState));
    }
}

void pit_load_state(Cpu* cpu, const void* in_buf) {
    PitState* pit = pit_state(cpu);
    if (pit && in_buf) {
        memcpy(pit, in_buf, sizeof(PitState));
    }
}
