// ------------------------------------------------------------------------------
//          videocards.c - Video card registry
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

#include "videocards.h"
#include <stdio.h>
#include <string.h>

const VideoCard* const g_video_cards[] = {
    &g_videocard_default,
    &g_videocard_default_text,
    &g_videocard_default_mono,
    NULL
};

const VideoCard* videocard_find_by_name(const char* name) {
    if (!name) return NULL;
    for (int i = 0; g_video_cards[i]; i++) {
        if (strcmp(g_video_cards[i]->name, name) == 0) {
            return g_video_cards[i];
        }
    }
    return NULL;
}

const VideoCard* videocard_get_default(void) {
    return g_video_cards[0];
}

void videocard_print_list(void) {
    printf("Available video cards:\n");
    for (int i = 0; g_video_cards[i]; i++) {
        printf("  %-12s - %s\n", g_video_cards[i]->name, g_video_cards[i]->description);
    }
}
