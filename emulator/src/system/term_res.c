// ------------------------------------------------------------------------------
//          term_res.c - Synchronous terminal resolution helper
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

#include "term_res.h"

#define VC_MODE_ADDR 0x0002001A

void term_res_update(Cpu* cpu) {
    if (!cpu || !cpu->mem) return;

    uint8_t mode_id = 0x00;
    if (VC_MODE_ADDR < cpu->mem_size) {
        mode_id = cpu->mem[VC_MODE_ADDR];
    }

    switch (mode_id) {
        case 0x01:
            cpu->term_res_x = 320;
            cpu->term_res_y = 200;
            break;
        case 0x10:
            cpu->term_res_x = 40;
            cpu->term_res_y = 30;
            break;
        case 0x11:
            cpu->term_res_x = 80;
            cpu->term_res_y = 60;
            break;
        case 0x12:
            cpu->term_res_x = 80;
            cpu->term_res_y = 30;
            break;
        case 0x20:
            cpu->term_res_x = 320;
            cpu->term_res_y = 240;
            break;
        case 0x21:
            cpu->term_res_x = 640;
            cpu->term_res_y = 480;
            break;
        case 0x22:
            cpu->term_res_x = 800;
            cpu->term_res_y = 600;
            break;
        case 0x00:
        default:
            cpu->term_res_x = 80;
            cpu->term_res_y = 25;
            break;
    }
}
