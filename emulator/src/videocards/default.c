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
// Graphics modes (256-color VGA palette):
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

// Standard VGA 256-color palette (6x6x6 color cube + 40 grayscale entries).
static uint32_t vga_palette[256];

static void default_build_vga_palette(void) {
    int idx = 0;
    for (int r = 0; r < 6; r++) {
        for (int g = 0; g < 6; g++) {
            for (int b = 0; b < 6; b++) {
                uint8_t rv = (uint8_t)(r * 51);
                uint8_t gv = (uint8_t)(g * 51);
                uint8_t bv = (uint8_t)(b * 51);
                vga_palette[idx++] = 0xFF000000 | ((uint32_t)rv << 16) | ((uint32_t)gv << 8) | bv;
            }
        }
    }
    // Remaining 40 entries: grayscale ramp.
    for (int i = 0; i < 40; i++) {
        uint8_t v = (uint8_t)(i * 255 / 39);
        vga_palette[idx++] = 0xFF000000 | ((uint32_t)v << 16) | ((uint32_t)v << 8) | v;
    }
}

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
    int       font_h;     // logical font height on screen (8 or 16)
} VideoMode;

static const VideoMode g_modes[] = {
    { 0x00, MODE_TYPE_TEXT, 640, 200, 80, 25, 8,  8 },
    { 0x01, MODE_TYPE_GFX,  320, 200,  0,  0, 0,  0 },
    { 0x10, MODE_TYPE_TEXT, 320, 240, 40, 30, 8,  8 },
    { 0x11, MODE_TYPE_TEXT, 640, 480, 80, 60, 8,  8 },
    { 0x12, MODE_TYPE_TEXT, 640, 480, 80, 30, 8, 16 }, // native 8x16 font
    { 0x20, MODE_TYPE_GFX,  320, 240,  0,  0, 0,  0 },
    { 0x21, MODE_TYPE_GFX,  640, 480,  0,  0, 0,  0 },
    { 0x22, MODE_TYPE_GFX,  800, 600,  0,  0, 0,  0 },
};
#define MODE_COUNT (sizeof(g_modes) / sizeof(g_modes[0]))

static SDL_Texture* default_texture = NULL;
static SDL_Renderer* default_renderer = NULL;
static uint32_t default_prev_text[MAX_TEXT_COLS * MAX_TEXT_ROWS];
static uint8_t  default_prev_gfx[MAX_GFX_WIDTH * MAX_GFX_HEIGHT];
static int default_initialized = 0;
static const VideoMode* default_current_mode = NULL;
static int default_force_redraw = 1;

// Native 8x16 font generated from the 8x8 font by doubling each scanline.
static uint8_t default_font8x16[256][16];

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

    // Build the 8x16 font from the embedded 8x8 font by duplicating rows.
    for (int ch = 0; ch < 256; ch++) {
        uint64_t glyph = font8x8[ch];
        for (int y = 0; y < 8; y++) {
            uint8_t row = (uint8_t)((glyph >> (y * 8)) & 0xFF);
            default_font8x16[ch][y * 2 + 0] = row;
            default_font8x16[ch][y * 2 + 1] = row;
        }
    }

    default_build_vga_palette();
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

            int px0 = col * mode->font_w;
            int py0 = row * mode->font_h;

            if (mode->font_h == 16) {
                for (int bit_y = 0; bit_y < 16; bit_y++) {
                    uint8_t row_bits = default_font8x16[ch][bit_y];
                    int py = py0 + bit_y;
                    for (int bit_x = 0; bit_x < mode->font_w; bit_x++) {
                        int px = px0 + bit_x;
                        int active = (row_bits >> (7 - bit_x)) & 1;
                        if (py < mode->height && px < mode->width) {
                            pixels[py * stride + px] = active ? fg : bg;
                        }
                    }
                }
            } else {
                uint64_t glyph = font8x8[ch];
                for (int bit_y = 0; bit_y < 8; bit_y++) {
                    uint8_t row_bits = (uint8_t)((glyph >> (bit_y * 8)) & 0xFF);
                    int py = py0 + bit_y;
                    for (int bit_x = 0; bit_x < mode->font_w; bit_x++) {
                        int px = px0 + bit_x;
                        int active = (row_bits >> (7 - bit_x)) & 1;
                        if (py < mode->height && px < mode->width) {
                            pixels[py * stride + px] = active ? fg : bg;
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

            // Mode 13h style: standard VGA 256-color palette.
            pixels[y * stride + x] = vga_palette[pix];
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
    .description = "text (80x25,40x30,80x60,80x30-8x16) + graphics (320x200,320x240,640x480,800x600)",
    .init        = default_init,
    .shutdown    = default_shutdown,
    .reset       = default_reset,
    .update      = default_update,
    .get_texture = default_get_texture,
    .get_display_width  = default_get_display_width,
    .get_display_height = default_get_display_height,
};
