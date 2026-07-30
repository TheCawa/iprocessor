// ------------------------------------------------------------------------------
//          default-text.c - Text-only video card
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
// default - Generic 80x25 text-mode video card
// 8x8 font, 16-color VGA palette.
// Reads character/attribute cells from CPU memory at cpu->backend->vbuffer_base.
// -----------------------------------------------------------------------------

#include "videocard.h"
#include "font8x8.h"
#include <string.h>

#define DEFAULT_COLS 80
#define DEFAULT_ROWS 25
#define DEFAULT_FONT_W FONT8X8_WIDTH
#define DEFAULT_FONT_H FONT8X8_HEIGHT
#define DEFAULT_WIDTH  (DEFAULT_COLS * DEFAULT_FONT_W)  // 640
#define DEFAULT_HEIGHT (DEFAULT_ROWS * DEFAULT_FONT_H)  // 200

// 16-color VGA palette (ARGB)
static const uint32_t default_text_palette[16] = {
    0xFF000000, 0xFF0000AA, 0xFF00AA00, 0xFF00AAAA,
    0xFFAA0000, 0xFFAA00AA, 0xFFAA5500, 0xFFAAAAAA,
    0xFF555555, 0xFF5555FF, 0xFF55FF55, 0xFF55FFFF,
    0xFFFF5555, 0xFFFF55FF, 0xFFFFFF55, 0xFFFFFFFF
};

static SDL_Texture* default_text_texture = NULL;
static uint32_t default_text_prev_screen[DEFAULT_COLS * DEFAULT_ROWS];
static int default_text_initialized = 0;

static inline uint32_t default_text_get_color(uint8_t attr, int is_fg) {
    uint8_t idx = is_fg ? (attr & 0x0F) : ((attr >> 4) & 0x0F);
    return default_text_palette[idx];
}

static int default_text_init(SDL_Renderer* renderer) {
    if (default_text_initialized) return 0;
    default_text_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                       SDL_TEXTUREACCESS_STREAMING,
                                       DEFAULT_WIDTH, DEFAULT_HEIGHT);
    if (!default_text_texture) return -1;

    memset(default_text_prev_screen, 0xFF, sizeof(default_text_prev_screen));
    SDL_SetTextureBlendMode(default_text_texture, SDL_BLENDMODE_NONE);
    default_text_initialized = 1;
    return 0;
}

static void default_text_shutdown(void) {
    if (default_text_texture) {
        SDL_DestroyTexture(default_text_texture);
        default_text_texture = NULL;
    }
    default_text_initialized = 0;
}

static void default_text_reset(Cpu* cpu) {
    if (!cpu || !cpu->vram) return;

    uint32_t size = (uint32_t)(DEFAULT_COLS * DEFAULT_ROWS * 2);
    if (size > cpu->vram_size) size = (uint32_t)cpu->vram_size;
    if (size > 0) {
        for (uint32_t i = 0; i < size; i += 2) {
            cpu->vram[i] = ' ';
            cpu->vram[i + 1] = 0x07;
        }
    }

    memset(default_text_prev_screen, 0xFF, sizeof(default_text_prev_screen));
    cpu->screen_dirty = 1;
}

static void default_text_update(Cpu* cpu) {
    if (!default_text_texture || !cpu->backend) return;

    uint32_t* pixels = NULL;
    int pitch = 0;
    if (SDL_LockTexture(default_text_texture, NULL, (void**)&pixels, &pitch) != 0) return;

    int stride = pitch / sizeof(uint32_t);
    uint32_t base = cpu->term_buffer;
    int dirty = cpu->screen_dirty;
    bool cursor_visible = cpu->term_cursor_visible;
    uint32_t cx = cpu->term_pos_x;
    uint32_t cy = cpu->term_pos_y;
    bool cursor_blink = ((SDL_GetTicks() / 500) & 1) == 0;

    cpu->term_res_x = DEFAULT_COLS;
    cpu->term_res_y = DEFAULT_ROWS;

    for (int y = 0; y < DEFAULT_ROWS; y++) {
        for (int x = 0; x < DEFAULT_COLS; x++) {
            int cell_idx = y * DEFAULT_COLS + x;
            uint32_t addr = base + (uint32_t)cell_idx * 2;

            uint8_t ch   = (cpu->vram && addr + 1 < cpu->vram_size) ? cpu->vram[addr]     : ' ';
            uint8_t attr = (cpu->vram && addr + 1 < cpu->vram_size) ? cpu->vram[addr + 1] : 0x07;

            uint32_t cell_hash = ((uint32_t)ch << 8) | attr;
            bool is_cursor = cursor_visible && cursor_blink && (uint32_t)x == cx && (uint32_t)y == cy;
            if (!dirty && !is_cursor && default_text_prev_screen[cell_idx] == cell_hash) continue;
            default_text_prev_screen[cell_idx] = cell_hash;

            uint32_t fg = default_text_get_color(attr, 1);
            uint32_t bg = default_text_get_color(attr, 0);
            if (is_cursor) {
                uint32_t tmp = fg; fg = bg; bg = tmp;
            }

            uint64_t glyph = font8x8[ch];

            int px0 = x * DEFAULT_FONT_W;
            int py0 = y * DEFAULT_FONT_H;

            for (int bit_y = 0; bit_y < DEFAULT_FONT_H; bit_y++) {
                uint8_t row = (uint8_t)((glyph >> (bit_y * 8)) & 0xFF);
                int py = py0 + bit_y;
                for (int bit_x = 0; bit_x < DEFAULT_FONT_W; bit_x++) {
                    int px = px0 + bit_x;
                    int active = (row >> (7 - bit_x)) & 1;
                    pixels[py * stride + px] = active ? fg : bg;
                }
            }
        }
    }

    SDL_UnlockTexture(default_text_texture);
    cpu->screen_dirty = 0;
}

static SDL_Texture* default_text_get_texture(void) {
    return default_text_texture;
}

static int default_text_get_display_width(void) {
    return DEFAULT_WIDTH;
}

static int default_text_get_display_height(void) {
    return DEFAULT_HEIGHT;
}

const VideoCard g_videocard_default_text = {
    .name        = "default-text",
    .description = "80x25 16-color text mode",
    .init        = default_text_init,
    .shutdown    = default_text_shutdown,
    .reset       = default_text_reset,
    .update      = default_text_update,
    .get_texture = default_text_get_texture,
    .get_display_width  = default_text_get_display_width,
    .get_display_height = default_text_get_display_height,
};
