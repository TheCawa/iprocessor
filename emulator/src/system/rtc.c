// ------------------------------------------------------------------------------
//          rtc.c - Real-time clock implementation
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

#include "rtc.h"
#include <time.h>

static struct tm rtc_get_local_time(void) {
    time_t now = time(NULL);
    struct tm tm_info;
    localtime_s(&tm_info, &now);
    return tm_info;
}

static uint32_t rtc_get_unix_time(void) {
    return (uint32_t)time(NULL);
}

uint8_t rtc_read_byte(uint32_t addr) {
    struct tm tm_info = rtc_get_local_time();
    switch (addr) {
        case RTC_ADDR_SECONDS: return (uint8_t)tm_info.tm_sec;
        case RTC_ADDR_MINUTES: return (uint8_t)tm_info.tm_min;
        case RTC_ADDR_HOURS:   return (uint8_t)tm_info.tm_hour;
        case RTC_ADDR_DAY:     return (uint8_t)tm_info.tm_mday;
        case RTC_ADDR_MONTH:   return (uint8_t)(tm_info.tm_mon + 1);
        case RTC_ADDR_YEAR:    return (uint8_t)(tm_info.tm_year % 100);
        case RTC_ADDR_WEEKDAY: return (uint8_t)tm_info.tm_wday;
        case RTC_ADDR_CENTURY: return (uint8_t)(1900 + tm_info.tm_year) / 100;
        default: return 0;
    }
}

uint32_t rtc_read_dword(uint32_t addr) {
    if (addr == RTC_ADDR_UNIX_TIME) {
        return rtc_get_unix_time();
    }
    return 0;
}

void rtc_write_byte(uint32_t addr, uint8_t val) {
    (void)addr;
    (void)val;
    // RTC is read-only for now.
}

void rtc_write_dword(uint32_t addr, uint32_t val) {
    (void)addr;
    (void)val;
    // RTC is read-only for now.
}
