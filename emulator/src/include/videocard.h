// ------------------------------------------------------------------------------
//          videocard.h - Video card abstract interface
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

#ifndef VIDEOCARD_H
#define VIDEOCARD_H

#include <SDL.h>
#include "cpu_api.h"

#ifdef __cplusplus
extern "C" {
#endif

// Abstract video card interface.
// A video card owns its own SDL texture, font and palette.
// It reads video memory from the CPU (usually at cpu->backend->vbuffer_base)
// and renders into the texture. The emulator only calls init/update/shutdown
// and blits the resulting texture.

typedef struct VideoCard {
    const char* name;        // Short identifier, e.g. "default"
    const char* description; // Human readable description

    // Initialize the card and create its SDL texture. Return 0 on success.
    int  (*init)(SDL_Renderer* renderer);

    // Destroy the texture and any card-specific state.
    void (*shutdown)(void);

    // Reset card state on CPU reset (switch to default text mode, clear buffer).
    // Optional: may be NULL.
    void (*reset)(Cpu* cpu);

    // Synchronously update cpu->term_res_x/y from the current VC_MODE.
    // Optional: may be NULL.
    void (*update_term_res)(Cpu* cpu);

    // Re-render the texture from CPU video memory if needed.
    void (*update)(Cpu* cpu);

    // Return the SDL texture produced by the card.
    SDL_Texture* (*get_texture)(void);

    // Logical display size in pixels.
    int  (*get_display_width)(void);
    int  (*get_display_height)(void);
} VideoCard;

#ifdef __cplusplus
}
#endif

#endif // VIDEOCARD_H
