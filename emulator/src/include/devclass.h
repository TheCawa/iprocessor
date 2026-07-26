// ------------------------------------------------------------------------------
//          devclass.h - Device class enumeration header
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

#ifndef DEVCLASS_H
#define DEVCLASS_H

#include "cpu_api.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Device class enumeration used by the system device table.
#define DEV_CLASS_SYSTEM  0x00
#define DEV_CLASS_STORAGE 0x01
#define DEV_CLASS_INPUT   0x02
#define DEV_CLASS_TIMER   0x03
#define DEV_CLASS_RTC     0x04
#define DEV_CLASS_VIDEO   0x05

// Vendor identifiers.
#define DEV_VENDOR_SYSTEM 0x01

// Device enumeration MMIO range (0x20200 .. 0x2023F).
#define DEV_ADDR_BASE     0x00020200
#define DEV_ADDR_COUNT    0x00020200
#define DEV_ADDR_SELECT   0x00020201
#define DEV_ADDR_CLASS    0x00020202
#define DEV_ADDR_VENDOR   0x00020203
#define DEV_ADDR_NAME     0x00020204
#define DEV_ADDR_END      0x0002023F

#define DEV_MAX_NAME_LEN  32

typedef struct {
    uint8_t slot;
    uint8_t class_id;
    uint8_t vendor_id;
    const char* name;
} DeviceClassEntry;

// Check whether an address belongs to the device-class MMIO range.
bool devclass_is_mmio(uint32_t addr);

// Read a byte from the device-class MMIO range.
uint8_t devclass_read_byte(Cpu* cpu, uint32_t addr);

// Write a byte to the device-class MMIO range (only SELECT is writable).
void devclass_write_byte(Cpu* cpu, uint32_t addr, uint8_t val);

#ifdef __cplusplus
}
#endif

#endif // DEVCLASS_H
