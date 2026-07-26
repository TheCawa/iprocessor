// ------------------------------------------------------------------------------
//          videocards.h - Video card registry header
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

#ifndef VIDEOCARDS_H
#define VIDEOCARDS_H

#include "videocard.h"

#ifdef __cplusplus
extern "C" {
#endif

// Built-in video cards.
extern const VideoCard g_videocard_default;
extern const VideoCard g_videocard_default_text;
extern const VideoCard g_videocard_default_mono;

// Null-terminated array of all registered video cards.
extern const VideoCard* const g_video_cards[];

// Find a video card by name. Returns NULL if not found.
const VideoCard* videocard_find_by_name(const char* name);

// Return the default video card (first in the registry).
const VideoCard* videocard_get_default(void);

// Print a list of available video cards to stdout.
void videocard_print_list(void);

#ifdef __cplusplus
}
#endif

#endif // VIDEOCARDS_H
