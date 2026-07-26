// ------------------------------------------------------------------------------
//          system.h - Common system device helpers header
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

#ifndef SYSTEM_H
#define SYSTEM_H

#include "cpu_api.h"

#ifdef __cplusplus
extern "C" {
#endif

// Common disk controller helpers used by CPU backends.
// The disk state lives in Cpu::disk_drives[DISK_MAX_DRIVES]; these functions
// implement the actual read/write sector logic so that all backends share the
// same behavior.

#define DISK_STATUS_BUSY  0x01
#define DISK_STATUS_ERROR 0x02

// MMIO accessors for the disk controller.
void     disk_write_dword(Cpu* cpu, uint32_t addr, uint32_t val);
void     disk_write_byte(Cpu* cpu, uint32_t addr, uint8_t val);
uint32_t disk_read_dword(Cpu* cpu, uint32_t addr);
uint8_t  disk_read_byte(Cpu* cpu);

// Release dynamically allocated disk image data for all drives.
void disk_free_image(Cpu* cpu);

// Release image data for a single drive (used when ejecting a disk image).
void disk_drive_free_image(DiskDrive* drive);

// Keyboard input helper.
uint8_t kbd_read_ascii(Cpu* cpu);

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_H
