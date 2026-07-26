// ------------------------------------------------------------------------------
//          rtc.h - Real-time clock header
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

#ifndef RTC_H
#define RTC_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// PC48 RTC MMIO addresses (0x00020020 - 0x0002002F)
#define RTC_ADDR_BASE       0x00020020
#define RTC_ADDR_SECONDS    0x00020020
#define RTC_ADDR_MINUTES    0x00020021
#define RTC_ADDR_HOURS      0x00020022
#define RTC_ADDR_DAY        0x00020023
#define RTC_ADDR_MONTH      0x00020024
#define RTC_ADDR_YEAR       0x00020025  // years since 2000
#define RTC_ADDR_WEEKDAY    0x00020026
#define RTC_ADDR_CENTURY    0x00020027
#define RTC_ADDR_UNIX_TIME  0x00020028  // 32-bit, little-endian in MMIO

// Read a single RTC register (byte).
uint8_t rtc_read_byte(uint32_t addr);

// Read a 32-bit value from RTC region (used for UNIX time).
uint32_t rtc_read_dword(uint32_t addr);

// Write is ignored for now (RTC is read-only host clock).
void rtc_write_byte(uint32_t addr, uint8_t val);
void rtc_write_dword(uint32_t addr, uint32_t val);

// Helper: return true if addr belongs to RTC MMIO region.
static inline bool rtc_is_mmio(uint32_t addr) {
    return (addr >= RTC_ADDR_BASE && addr <= 0x0002002F);
}

#ifdef __cplusplus
}
#endif

#endif // RTC_H
