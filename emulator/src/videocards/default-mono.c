// ------------------------------------------------------------------------------
//          default-mono.c - Monochrome text video card
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

// -----------------------------------------------------------------------------
// default_mono - Generic monochrome 80x25 text-mode video card
// 8x8 font, green-phosphor-on-black palette.
// Reads character/attribute cells from CPU memory at cpu->backend->vbuffer_base.
// -----------------------------------------------------------------------------

#include "videocard.h"
#include "font8x8.h"
#include <string.h>

#define DEFAULT_MONO_COLS 80
#define DEFAULT_MONO_ROWS 25
#define DEFAULT_MONO_FONT_W FONT8X8_WIDTH
#define DEFAULT_MONO_FONT_H FONT8X8_HEIGHT
#define DEFAULT_MONO_WIDTH  (DEFAULT_MONO_COLS * DEFAULT_MONO_FONT_W)  // 640
#define DEFAULT_MONO_HEIGHT (DEFAULT_MONO_ROWS * DEFAULT_MONO_FONT_H)  // 200

// Monochrome green-phosphor palette.
static const uint32_t default_mono_fg[16] = {
    0xFF000000, 0xFF002200, 0xFF003300, 0xFF004400,
    0xFF005500, 0xFF006600, 0xFF007700, 0xFF008800,
    0xFF009900, 0xFF00AA00, 0xFF00BB00, 0xFF00CC00,
    0xFF00DD00, 0xFF00EE00, 0xFF00FF00, 0xFF00FF00
};

static const uint32_t default_mono_bg[16] = {
    0xFF000000, 0xFF001100, 0xFF002200, 0xFF003300,
    0xFF004400, 0xFF005500, 0xFF006600, 0xFF007700,
    0xFF008800, 0xFF009900, 0xFF00AA00, 0xFF00BB00,
    0xFF00CC00, 0xFF00DD00, 0xFF00EE00, 0xFF00FF00
};

static SDL_Texture* default_mono_texture = NULL;
static uint32_t default_mono_prev_screen[DEFAULT_MONO_COLS * DEFAULT_MONO_ROWS];
static int default_mono_initialized = 0;

static inline uint32_t default_mono_get_fg(uint8_t attr) {
    return default_mono_fg[attr & 0x0F];
}

static inline uint32_t default_mono_get_bg(uint8_t attr) {
    return default_mono_bg[(attr >> 4) & 0x0F];
}

static int default_mono_init(SDL_Renderer* renderer) {
    if (default_mono_initialized) return 0;
    default_mono_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                           SDL_TEXTUREACCESS_STREAMING,
                                           DEFAULT_MONO_WIDTH, DEFAULT_MONO_HEIGHT);
    if (!default_mono_texture) return -1;

    memset(default_mono_prev_screen, 0xFF, sizeof(default_mono_prev_screen));
    SDL_SetTextureBlendMode(default_mono_texture, SDL_BLENDMODE_NONE);
    default_mono_initialized = 1;
    return 0;
}

static void default_mono_shutdown(void) {
    if (default_mono_texture) {
        SDL_DestroyTexture(default_mono_texture);
        default_mono_texture = NULL;
    }
    default_mono_initialized = 0;
}

static void default_mono_reset(Cpu* cpu) {
    if (!cpu || !cpu->mem || !cpu->backend) return;

    uint32_t base = cpu->backend->vbuffer_base;
    uint32_t size = (uint32_t)(DEFAULT_MONO_COLS * DEFAULT_MONO_ROWS * 2);
    if (base + size > cpu->mem_size) {
        size = (base < cpu->mem_size) ? (uint32_t)(cpu->mem_size - base) : 0;
    }
    if (size > 0) {
        for (uint32_t i = 0; i < size; i += 2) {
            cpu->mem[base + i] = ' ';
            cpu->mem[base + i + 1] = 0x07;
        }
    }

    memset(default_mono_prev_screen, 0xFF, sizeof(default_mono_prev_screen));
    cpu->screen_dirty = 1;
}

static void default_mono_update(Cpu* cpu) {
    if (!default_mono_texture || !cpu->backend) return;

    uint32_t* pixels = NULL;
    int pitch = 0;
    if (SDL_LockTexture(default_mono_texture, NULL, (void**)&pixels, &pitch) != 0) return;

    int stride = pitch / sizeof(uint32_t);
    uint32_t base = cpu->backend->vbuffer_base;
    int dirty = cpu->screen_dirty;

    for (int y = 0; y < DEFAULT_MONO_ROWS; y++) {
        for (int x = 0; x < DEFAULT_MONO_COLS; x++) {
            int cell_idx = y * DEFAULT_MONO_COLS + x;
            uint32_t addr = base + (uint32_t)cell_idx * 2;

            uint8_t ch   = (addr + 1 < cpu->mem_size) ? cpu->mem[addr]     : ' ';
            uint8_t attr = (addr + 1 < cpu->mem_size) ? cpu->mem[addr + 1] : 0x07;

            uint32_t cell_hash = ((uint32_t)ch << 8) | attr;
            if (!dirty && default_mono_prev_screen[cell_idx] == cell_hash) continue;
            default_mono_prev_screen[cell_idx] = cell_hash;

            uint32_t fg = default_mono_get_fg(attr);
            uint32_t bg = default_mono_get_bg(attr);

            uint64_t glyph = font8x8[ch];

            int px0 = x * DEFAULT_MONO_FONT_W;
            int py0 = y * DEFAULT_MONO_FONT_H;

            for (int bit_y = 0; bit_y < DEFAULT_MONO_FONT_H; bit_y++) {
                uint8_t row = (uint8_t)((glyph >> (bit_y * 8)) & 0xFF);
                int py = py0 + bit_y;
                for (int bit_x = 0; bit_x < DEFAULT_MONO_FONT_W; bit_x++) {
                    int px = px0 + bit_x;
                    int active = (row >> (7 - bit_x)) & 1;
                    pixels[py * stride + px] = active ? fg : bg;
                }
            }
        }
    }

    SDL_UnlockTexture(default_mono_texture);
    cpu->screen_dirty = 0;
}

static SDL_Texture* default_mono_get_texture(void) {
    return default_mono_texture;
}

static int default_mono_get_display_width(void) {
    return DEFAULT_MONO_WIDTH;
}

static int default_mono_get_display_height(void) {
    return DEFAULT_MONO_HEIGHT;
}

const VideoCard g_videocard_default_mono = {
    .name        = "default-mono",
    .description = "80x25 green-phosphor monochrome text mode",
    .init        = default_mono_init,
    .shutdown    = default_mono_shutdown,
    .reset       = default_mono_reset,
    .update      = default_mono_update,
    .get_texture = default_mono_get_texture,
    .get_display_width  = default_mono_get_display_width,
    .get_display_height = default_mono_get_display_height,
};
