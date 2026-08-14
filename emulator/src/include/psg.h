// ------------------------------------------------------------------------------
//          psg.h - Programmable Sound Generator header
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

#ifndef PSG_H
#define PSG_H

#include "cpu_api.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// PC48 PSG MMIO register addresses.
#define PSG_ADDR_BASE        0x000200B0
#define PSG_ADDR_TONE0       0x000200B0  // 16-bit tone period (low byte + high nibble)
#define PSG_ADDR_TONE1       0x000200B2
#define PSG_ADDR_TONE2       0x000200B4
#define PSG_ADDR_NOISE       0x000200B6  // noise period
#define PSG_ADDR_MIXER       0x000200B7  // tone/noise enable bits
#define PSG_ADDR_VOL0        0x000200B8  // 4-bit volume + envelope enable bit
#define PSG_ADDR_VOL1        0x000200B9
#define PSG_ADDR_VOL2        0x000200BA
#define PSG_ADDR_ENV_PERIOD  0x000200BB  // 16-bit envelope period
#define PSG_ADDR_ENV_SHAPE   0x000200BD  // envelope shape
#define PSG_ADDR_ENV_LEVEL   0x000200BE  // current envelope level (read-only-ish)
#define PSG_ADDR_END         0x000200BE

#define PSG_REG_COUNT 15

// Mixer bits.
#define PSG_MIXER_TONE0  0x01
#define PSG_MIXER_TONE1  0x02
#define PSG_MIXER_TONE2  0x04
#define PSG_MIXER_NOISE0 0x08
#define PSG_MIXER_NOISE1 0x10
#define PSG_MIXER_NOISE2 0x20

// Volume envelope enable bit.
#define PSG_VOL_ENV 0x80

// Envelope shapes.
#define PSG_ENV_OFF       0
#define PSG_ENV_ATTACK    1
#define PSG_ENV_DECAY     2
#define PSG_ENV_TRIANGLE  3
#define PSG_ENV_SAWTOOTH  4

// Audio output parameters.
#define PSG_SAMPLE_RATE 44100
#define PSG_AUDIO_BUFFER_SAMPLES 512

// Initialize / shutdown PSG audio subsystem.
bool psg_audio_init(void);
void psg_audio_shutdown(void);

// Bind a Cpu instance to the audio generator.
void psg_bind_cpu(Cpu* cpu);

// Initialize / reset PSG state for the bound CPU.
void psg_init(Cpu* cpu);
void psg_reset(Cpu* cpu);

// MMIO accessors.
uint8_t  psg_read_byte(Cpu* cpu, uint32_t addr);
uint16_t psg_read_word(Cpu* cpu, uint32_t addr);
void     psg_write_byte(Cpu* cpu, uint32_t addr, uint8_t val);
void     psg_write_word(Cpu* cpu, uint32_t addr, uint16_t val);

// Returns true if addr belongs to the PSG MMIO region.
static inline bool psg_is_mmio(uint32_t addr) {
    return (addr >= PSG_ADDR_BASE && addr <= PSG_ADDR_END);
}

#ifdef __cplusplus
}
#endif

#endif // PSG_H
