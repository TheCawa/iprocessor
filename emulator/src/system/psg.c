// ------------------------------------------------------------------------------
//          psg.c - Programmable Sound Generator implementation
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

#include "psg.h"
#include <SDL.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// Number of PSG tone channels.
#define PSG_CHANNELS 3

// Maximum amplitude per channel before mixing (keeps headroom).
#define PSG_CHANNEL_AMP 2000

// Simple PSG state: 3 square-wave channels + noise + envelope.
typedef struct {
    // Registers (writable from CPU).
    uint16_t tone_period[PSG_CHANNELS];
    uint8_t  noise_period;
    uint8_t  mixer;
    uint8_t  volume[PSG_CHANNELS];
    uint16_t env_period;
    uint8_t  env_shape;

    // Runtime generators.
    uint16_t tone_counter[PSG_CHANNELS];
    uint8_t  tone_output[PSG_CHANNELS];   // 0 or 1
    uint16_t noise_counter;
    uint32_t noise_lfsr;
    uint8_t  noise_output;                // 0 or 1

    // Envelope generator.
    uint16_t env_counter;
    uint8_t  env_level;                   // 0..15
    int      env_direction;               // +1 or -1
    bool     env_running;

    // Bound CPU (used by audio callback).
    Cpu* cpu;
} PsgState;

static PsgState g_psg;     // Single instance for the GUI emulator.
static bool     g_audio_open = false;

// Return the PSG state bound to a CPU, or NULL.
static PsgState* psg_state(Cpu* cpu) {
    if (cpu && cpu->psg_data) return (PsgState*)cpu->psg_data;
    return NULL;
}

// Advance the noise LFSR once.
static void psg_noise_tick(PsgState* psg) {
    uint32_t lfsr = psg->noise_lfsr;
    uint32_t bit = ((lfsr >> 0) ^ (lfsr >> 3)) & 1;
    psg->noise_lfsr = (lfsr >> 1) | (bit << 16);
    psg->noise_output = (uint8_t)(lfsr & 1);
}

// Advance the envelope generator by one sample tick.
static void psg_env_tick(PsgState* psg) {
    if (psg->env_shape == PSG_ENV_OFF || psg->env_period == 0) {
        psg->env_level = 15;
        return;
    }

    if (--psg->env_counter == 0) {
        psg->env_counter = psg->env_period;

        if (!psg->env_running) {
            // First tick after shape write.
            psg->env_running = true;
            switch (psg->env_shape) {
                case PSG_ENV_ATTACK:
                    psg->env_level = 0;
                    psg->env_direction = 1;
                    break;
                case PSG_ENV_DECAY:
                    psg->env_level = 15;
                    psg->env_direction = -1;
                    break;
                case PSG_ENV_TRIANGLE:
                    psg->env_level = 0;
                    psg->env_direction = 1;
                    break;
                case PSG_ENV_SAWTOOTH:
                default:
                    psg->env_level = 0;
                    psg->env_direction = 1;
                    break;
            }
        } else {
            int lvl = (int)psg->env_level + psg->env_direction;
            switch (psg->env_shape) {
                case PSG_ENV_ATTACK:
                    if (lvl > 15) lvl = 15;
                    break;
                case PSG_ENV_DECAY:
                    if (lvl < 0) lvl = 0;
                    break;
                case PSG_ENV_TRIANGLE:
                    if (lvl > 15) { lvl = 14; psg->env_direction = -1; }
                    else if (lvl < 0) { lvl = 1; psg->env_direction = 1; }
                    break;
                case PSG_ENV_SAWTOOTH:
                default:
                    if (lvl > 15) lvl = 0;
                    break;
            }
            psg->env_level = (uint8_t)lvl;
        }
    }
}

// Generate one audio sample from the current PSG state.
static int16_t psg_generate_sample(PsgState* psg) {
    int32_t mix = 0;

    for (int ch = 0; ch < PSG_CHANNELS; ch++) {
        // Tone square wave.
        if (psg->tone_period[ch] > 0) {
            if (--psg->tone_counter[ch] == 0) {
                psg->tone_counter[ch] = psg->tone_period[ch];
                psg->tone_output[ch] ^= 1;
            }
        }

        bool tone_on  = (psg->mixer & (PSG_MIXER_TONE0  << ch)) != 0;
        bool noise_on = (psg->mixer & (PSG_MIXER_NOISE0 << ch)) != 0;

        uint8_t out = 0;
        if (tone_on)  out |= psg->tone_output[ch];
        if (noise_on) out |= psg->noise_output;

        if (out) {
            uint8_t vol = psg->volume[ch] & 0x0F;
            if (psg->volume[ch] & PSG_VOL_ENV) {
                vol = psg->env_level;
            }
            // Logarithmic-ish amplitude scale (similar to AY volume curve).
            int32_t amp = (int32_t)(PSG_CHANNEL_AMP * (vol / 15.0));
            mix += amp;
        }
    }

    // Noise generator tick.
    if (psg->noise_period == 0 || --psg->noise_counter == 0) {
        psg->noise_counter = psg->noise_period ? psg->noise_period : 1;
        psg_noise_tick(psg);
    }

    psg_env_tick(psg);

    // Clamp.
    if (mix >  32767) mix =  32767;
    if (mix < -32768) mix = -32768;
    return (int16_t)mix;
}

// SDL audio callback.
static void psg_audio_callback(void* userdata, Uint8* stream, int len) {
    (void)userdata;
    PsgState* psg = &g_psg;
    int16_t* out = (int16_t*)stream;
    int samples = len / sizeof(int16_t);

    for (int i = 0; i < samples; i++) {
        out[i] = psg_generate_sample(psg);
    }
}

bool psg_audio_init(void) {
    if (g_audio_open) return true;

    SDL_AudioSpec want, have;
    memset(&want, 0, sizeof(want));
    want.freq = PSG_SAMPLE_RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = PSG_AUDIO_BUFFER_SAMPLES;
    want.callback = psg_audio_callback;

    if (SDL_OpenAudio(&want, &have) < 0) {
        fprintf(stderr, "[WARN] Failed to open PSG audio: %s\n", SDL_GetError());
        return false;
    }
    SDL_PauseAudio(0);
    g_audio_open = true;
    return true;
}

void psg_audio_shutdown(void) {
    if (g_audio_open) {
        SDL_CloseAudio();
        g_audio_open = false;
    }
    memset(&g_psg, 0, sizeof(g_psg));
}

void psg_bind_cpu(Cpu* cpu) {
    g_psg.cpu = cpu;
    if (cpu) {
        cpu->psg_data = &g_psg;
        psg_init(cpu);
    }
}

static void psg_set_defaults(PsgState* psg) {
    memset(psg, 0, sizeof(PsgState));
    for (int i = 0; i < PSG_CHANNELS; i++) {
        psg->tone_period[i] = 0;
        psg->volume[i] = 0;
    }
    psg->noise_period = 0;
    psg->mixer = 0;
    psg->env_period = 0;
    psg->env_shape = PSG_ENV_OFF;
    psg->env_level = 15;
    psg->env_counter = 1;
    psg->noise_lfsr = 0x1FFFF;
    psg->noise_counter = 1;
}

void psg_init(Cpu* cpu) {
    PsgState* psg = psg_state(cpu);
    if (!psg) return;
    psg_set_defaults(psg);
}

void psg_reset(Cpu* cpu) {
    psg_init(cpu);
}

uint8_t psg_read_byte(Cpu* cpu, uint32_t addr) {
    PsgState* psg = psg_state(cpu);
    if (!psg) return 0;

    switch (addr) {
        case PSG_ADDR_TONE0:       return (uint8_t)(psg->tone_period[0] & 0xFF);
        case PSG_ADDR_TONE0 + 1:   return (uint8_t)(psg->tone_period[0] >> 8);
        case PSG_ADDR_TONE1:       return (uint8_t)(psg->tone_period[1] & 0xFF);
        case PSG_ADDR_TONE1 + 1:   return (uint8_t)(psg->tone_period[1] >> 8);
        case PSG_ADDR_TONE2:       return (uint8_t)(psg->tone_period[2] & 0xFF);
        case PSG_ADDR_TONE2 + 1:   return (uint8_t)(psg->tone_period[2] >> 8);
        case PSG_ADDR_NOISE:       return psg->noise_period;
        case PSG_ADDR_MIXER:       return psg->mixer;
        case PSG_ADDR_VOL0:        return psg->volume[0];
        case PSG_ADDR_VOL1:        return psg->volume[1];
        case PSG_ADDR_VOL2:        return psg->volume[2];
        case PSG_ADDR_ENV_PERIOD:  return (uint8_t)(psg->env_period & 0xFF);
        case PSG_ADDR_ENV_PERIOD + 1: return (uint8_t)(psg->env_period >> 8);
        case PSG_ADDR_ENV_SHAPE:   return psg->env_shape;
        case PSG_ADDR_ENV_LEVEL:   return psg->env_level;
    }
    return 0;
}

uint16_t psg_read_word(Cpu* cpu, uint32_t addr) {
    if (addr == PSG_ADDR_TONE0 || addr == PSG_ADDR_TONE1 || addr == PSG_ADDR_TONE2 ||
        addr == PSG_ADDR_ENV_PERIOD) {
        return (uint16_t)(psg_read_byte(cpu, addr) | (psg_read_byte(cpu, addr + 1) << 8));
    }
    return psg_read_byte(cpu, addr);
}

void psg_write_byte(Cpu* cpu, uint32_t addr, uint8_t val) {
    PsgState* psg = psg_state(cpu);
    if (!psg) return;

    switch (addr) {
        case PSG_ADDR_TONE0:       psg->tone_period[0] = (psg->tone_period[0] & 0xFF00) | val; break;
        case PSG_ADDR_TONE0 + 1:   psg->tone_period[0] = (psg->tone_period[0] & 0x00FF) | ((uint16_t)val << 8); break;
        case PSG_ADDR_TONE1:       psg->tone_period[1] = (psg->tone_period[1] & 0xFF00) | val; break;
        case PSG_ADDR_TONE1 + 1:   psg->tone_period[1] = (psg->tone_period[1] & 0x00FF) | ((uint16_t)val << 8); break;
        case PSG_ADDR_TONE2:       psg->tone_period[2] = (psg->tone_period[2] & 0xFF00) | val; break;
        case PSG_ADDR_TONE2 + 1:   psg->tone_period[2] = (psg->tone_period[2] & 0x00FF) | ((uint16_t)val << 8); break;
        case PSG_ADDR_NOISE:       psg->noise_period = val; psg->noise_counter = 1; break;
        case PSG_ADDR_MIXER:       psg->mixer = val; break;
        case PSG_ADDR_VOL0:        psg->volume[0] = val; break;
        case PSG_ADDR_VOL1:        psg->volume[1] = val; break;
        case PSG_ADDR_VOL2:        psg->volume[2] = val; break;
        case PSG_ADDR_ENV_PERIOD:  psg->env_period = (psg->env_period & 0xFF00) | val; break;
        case PSG_ADDR_ENV_PERIOD + 1: psg->env_period = (psg->env_period & 0x00FF) | ((uint16_t)val << 8); break;
        case PSG_ADDR_ENV_SHAPE:
            psg->env_shape = val;
            psg->env_running = false;
            psg->env_counter = 1;
            break;
        case PSG_ADDR_ENV_LEVEL:
            psg->env_level = val & 0x0F;
            break;
    }
}

void psg_write_word(Cpu* cpu, uint32_t addr, uint16_t val) {
    if (addr == PSG_ADDR_TONE0 || addr == PSG_ADDR_TONE1 || addr == PSG_ADDR_TONE2 ||
        addr == PSG_ADDR_ENV_PERIOD) {
        psg_write_byte(cpu, addr, (uint8_t)(val & 0xFF));
        psg_write_byte(cpu, addr + 1, (uint8_t)(val >> 8));
        return;
    }
    psg_write_byte(cpu, addr, (uint8_t)val);
}
