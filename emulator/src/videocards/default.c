// ------------------------------------------------------------------------------
//          default.c - Default combined text/graphics video card
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
// default - Combined text and graphics video card
//
// Mode is selected by writing to DEFAULT_MODE_ADDR (0x0002001A).
//
// Text modes (16-color VGA palette):
//   0x00 - 80x25, 8x8 font  (640x200) legacy
//   0x10 - 40x30, 8x8 font  (320x240)
//   0x11 - 80x60, 8x8 font  (640x480)
//   0x12 - 80x30, 8x8 font stretched to 8x16 (640x480)
//
// Graphics modes (256-color grayscale ramp):
//   0x01 - 320x200 legacy
//   0x20 - 320x240
//   0x21 - 640x480
//   0x22 - 800x600
// -----------------------------------------------------------------------------

#include "videocard.h"
#include "font8x8.h"
#include <string.h>

#define DEFAULT_MODE_ADDR   0x0002001A

#define MAX_TEXT_COLS 80
#define MAX_TEXT_ROWS 60
#define MAX_GFX_WIDTH  800
#define MAX_GFX_HEIGHT 600

// 16-color VGA palette (ARGB)
static const uint32_t default_palette[16] = {
    0xFF000000, 0xFF0000AA, 0xFF00AA00, 0xFF00AAAA,
    0xFFAA0000, 0xFFAA00AA, 0xFFAA5500, 0xFFAAAAAA,
    0xFF555555, 0xFF5555FF, 0xFF55FF55, 0xFF55FFFF,
    0xFFFF5555, 0xFFFF55FF, 0xFFFFFF55, 0xFFFFFFFF
};

typedef enum {
    MODE_TYPE_TEXT,
    MODE_TYPE_GFX
} ModeType;

typedef struct {
    uint8_t   id;
    ModeType  type;
    int       width;
    int       height;
    // text-only
    int       cols;
    int       rows;
    int       font_w;
    int       font_h;     // logical font height on screen
    int       font_src_h; // height in the source 8x8 font (1..8)
} VideoMode;

static const VideoMode g_modes[] = {
    { 0x00, MODE_TYPE_TEXT, 640, 200, 80, 25, 8, 8, 8 },
    { 0x01, MODE_TYPE_GFX,  320, 200,  0,  0, 0, 0, 0 },
    { 0x10, MODE_TYPE_TEXT, 320, 240, 40, 30, 8, 8, 8 },
    { 0x11, MODE_TYPE_TEXT, 640, 480, 80, 60, 8, 8, 8 },
    { 0x12, MODE_TYPE_TEXT, 640, 480, 80, 30, 8, 16, 8 }, // 8x16 by stretching
    { 0x20, MODE_TYPE_GFX,  320, 240,  0,  0, 0, 0, 0 },
    { 0x21, MODE_TYPE_GFX,  640, 480,  0,  0, 0, 0, 0 },
    { 0x22, MODE_TYPE_GFX,  800, 600,  0,  0, 0, 0, 0 },
};
#define MODE_COUNT (sizeof(g_modes) / sizeof(g_modes[0]))

static SDL_Texture* default_texture = NULL;
static SDL_Renderer* default_renderer = NULL;
static uint32_t default_prev_text[MAX_TEXT_COLS * MAX_TEXT_ROWS];
static uint8_t  default_prev_gfx[MAX_GFX_WIDTH * MAX_GFX_HEIGHT];
static int default_initialized = 0;
static const VideoMode* default_current_mode = NULL;
static int default_force_redraw = 1;

static const VideoMode* default_find_mode(uint8_t id) {
    for (int i = 0; i < (int)MODE_COUNT; i++) {
        if (g_modes[i].id == id) return &g_modes[i];
    }
    return &g_modes[0]; // fallback to 80x25 text
}

static inline uint32_t default_get_color(uint8_t attr, int is_fg) {
    uint8_t idx = is_fg ? (attr & 0x0F) : ((attr >> 4) & 0x0F);
    return default_palette[idx];
}

static int default_recreate_texture(const VideoMode* mode, SDL_Renderer* renderer) {
    if (default_texture) {
        SDL_DestroyTexture(default_texture);
        default_texture = NULL;
    }

    default_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                        SDL_TEXTUREACCESS_STREAMING, mode->width, mode->height);
    if (!default_texture) return -1;

    SDL_SetTextureBlendMode(default_texture, SDL_BLENDMODE_NONE);
#if SDL_VERSION_ATLEAST(2, 0, 12)
    SDL_SetTextureScaleMode(default_texture, SDL_ScaleModeNearest);
#endif

    memset(default_prev_text, 0xFF, sizeof(default_prev_text));
    memset(default_prev_gfx,  0xFF, sizeof(default_prev_gfx));

    default_current_mode = mode;
    default_force_redraw = 1;
    return 0;
}

static int default_init(SDL_Renderer* renderer) {
    if (default_initialized) return 0;
    default_renderer = renderer;
    default_current_mode = NULL;
    default_force_redraw = 1;
    default_initialized = 1;
    return 0;
}

static void default_shutdown(void) {
    if (default_texture) {
        SDL_DestroyTexture(default_texture);
        default_texture = NULL;
    }
    default_renderer = NULL;
    default_initialized = 0;
    default_current_mode = NULL;
}

static uint8_t default_get_mode_id(Cpu* cpu) {
    if (!cpu || !cpu->mem) return 0x00;
    if (DEFAULT_MODE_ADDR < cpu->mem_size) {
        return cpu->mem[DEFAULT_MODE_ADDR];
    }
    return 0x00;
}

static void default_reset(Cpu* cpu) {
    if (!cpu || !cpu->mem) return;

    // Reset to 80x25 text mode on CPU reset.
    if (DEFAULT_MODE_ADDR < cpu->mem_size) {
        cpu->mem[DEFAULT_MODE_ADDR] = 0x00;
    }

    // Clear the largest possible video buffer area (text + graphics).
    uint32_t base = cpu->backend ? cpu->backend->vbuffer_base : 0x00100000;
    uint32_t size = MAX_GFX_WIDTH * MAX_GFX_HEIGHT;
    if (base + size > cpu->mem_size) {
        size = (base < cpu->mem_size) ? (uint32_t)(cpu->mem_size - base) : 0;
    }
    if (size > 0) {
        memset(&cpu->mem[base], 0, size);
    }

    cpu->screen_dirty = 1;
    default_current_mode = NULL; // force texture recreation
    default_force_redraw = 1;
}

static void default_update_text(Cpu* cpu, const VideoMode* mode) {
    uint32_t* pixels = NULL;
    int pitch = 0;
    if (SDL_LockTexture(default_texture, NULL, (void**)&pixels, &pitch) != 0) return;

    int stride = pitch / sizeof(uint32_t);
    uint32_t base = cpu->backend->vbuffer_base;
    int dirty = cpu->screen_dirty;

    for (int row = 0; row < mode->rows; row++) {
        for (int col = 0; col < mode->cols; col++) {
            int cell_idx = row * mode->cols + col;
            uint32_t addr = base + (uint32_t)cell_idx * 2;

            uint8_t ch   = (addr + 1 < cpu->mem_size) ? cpu->mem[addr]     : ' ';
            uint8_t attr = (addr + 1 < cpu->mem_size) ? cpu->mem[addr + 1] : 0x07;

            uint32_t cell_hash = ((uint32_t)ch << 8) | attr;
            if (!dirty && !default_force_redraw && default_prev_text[cell_idx] == cell_hash) continue;
            default_prev_text[cell_idx] = cell_hash;

            uint32_t fg = default_get_color(attr, 1);
            uint32_t bg = default_get_color(attr, 0);

            uint64_t glyph = font8x8[ch];

            int px0 = col * mode->font_w;
            int py0 = row * mode->font_h;

            for (int bit_y = 0; bit_y < mode->font_src_h; bit_y++) {
                uint8_t row_bits = (uint8_t)((glyph >> (bit_y * 8)) & 0xFF);
                int py = py0 + bit_y * (mode->font_h / mode->font_src_h);
                for (int bit_x = 0; bit_x < mode->font_w; bit_x++) {
                    int px = px0 + bit_x;
                    int active = (row_bits >> (7 - bit_x)) & 1;
                    uint32_t color = active ? fg : bg;
                    // For stretched modes duplicate the row.
                    int y_rep = (mode->font_h / mode->font_src_h);
                    for (int r = 0; r < y_rep; r++) {
                        int py_draw = py + r;
                        if (py_draw < mode->height && px < mode->width) {
                            pixels[py_draw * stride + px] = color;
                        }
                    }
                }
            }
        }
    }

    SDL_UnlockTexture(default_texture);
}

static void default_update_gfx(Cpu* cpu, const VideoMode* mode) {
    uint32_t* pixels = NULL;
    int pitch = 0;
    if (SDL_LockTexture(default_texture, NULL, (void**)&pixels, &pitch) != 0) return;

    int stride = pitch / sizeof(uint32_t);
    uint32_t base = cpu->backend->vbuffer_base;
    int dirty = cpu->screen_dirty;

    for (int y = 0; y < mode->height; y++) {
        for (int x = 0; x < mode->width; x++) {
            int idx = y * mode->width + x;
            uint32_t addr = base + (uint32_t)idx;

            uint8_t pix = (addr < cpu->mem_size) ? cpu->mem[addr] : 0;
            if (!dirty && !default_force_redraw && default_prev_gfx[idx] == pix) continue;
            default_prev_gfx[idx] = pix;

            // Mode 13h style: 256-color palette (grayscale ramp for now).
            uint32_t color = 0xFF000000 | (pix << 16) | (pix << 8) | pix;
            pixels[y * stride + x] = color;
        }
    }

    SDL_UnlockTexture(default_texture);
}

static void default_update(Cpu* cpu) {
    if (!cpu || !cpu->backend || !default_renderer) return;

    const VideoMode* mode = default_find_mode(default_get_mode_id(cpu));
    if (mode != default_current_mode) {
        if (default_recreate_texture(mode, default_renderer) != 0) return;
        cpu->screen_dirty = 1;
    }

    if (!default_texture) return;

    if (mode->type == MODE_TYPE_GFX) {
        default_update_gfx(cpu, mode);
    } else {
        default_update_text(cpu, mode);
    }

    cpu->screen_dirty = 0;
    default_force_redraw = 0;
}

static SDL_Texture* default_get_texture(void) {
    return default_texture;
}

static int default_get_display_width(void) {
    return default_current_mode ? default_current_mode->width : 640;
}

static int default_get_display_height(void) {
    return default_current_mode ? default_current_mode->height : 200;
}

const VideoCard g_videocard_default = {
    .name        = "default",
    .description = "text (80x25,40x30,80x60,80x30) + graphics (320x200,320x240,640x480,800x600)",
    .init        = default_init,
    .shutdown    = default_shutdown,
    .reset       = default_reset,
    .update      = default_update,
    .get_texture = default_get_texture,
    .get_display_width  = default_get_display_width,
    .get_display_height = default_get_display_height,
};
