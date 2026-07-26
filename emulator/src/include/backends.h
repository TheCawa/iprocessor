// ------------------------------------------------------------------------------
//          backends.h - CPU backend registration header
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

#ifndef BACKENDS_H
#define BACKENDS_H

#include "cpu_api.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const CpuBackend g_cpu_backend_i80148;
extern const CpuBackend g_cpu_backend_i8046;

#ifdef __cplusplus
}
#endif

#endif // BACKENDS_H
