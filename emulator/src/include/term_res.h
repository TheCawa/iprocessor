// ------------------------------------------------------------------------------
//          term_res.h - Synchronous terminal resolution helper
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

#ifndef TERM_RES_H
#define TERM_RES_H

#include "cpu_api.h"

#ifdef __cplusplus
extern "C" {
#endif

// Update cpu->term_res_x/y from the current VC_MODE byte in CPU memory.
// This mirrors the resolution table used by the default video card so that
// the CPU backend can report the correct resolution before the first render.
void term_res_update(Cpu* cpu);

#ifdef __cplusplus
}
#endif

#endif // TERM_RES_H
