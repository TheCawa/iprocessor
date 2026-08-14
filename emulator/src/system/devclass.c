// ------------------------------------------------------------------------------
//          devclass.c - Device class enumeration implementation
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

#include "devclass.h"
#include <string.h>

static const DeviceClassEntry g_devices[] = {
    { 0, DEV_CLASS_SYSTEM,  DEV_VENDOR_SYSTEM, "System"       },
    { 1, DEV_CLASS_STORAGE, DEV_VENDOR_SYSTEM, "System Disk"  },
    { 2, DEV_CLASS_INPUT,   DEV_VENDOR_SYSTEM, "Keyboard"     },
    { 3, DEV_CLASS_TIMER,   DEV_VENDOR_SYSTEM, "Timer"        },
    { 4, DEV_CLASS_RTC,     DEV_VENDOR_SYSTEM, "RTC"          },
    { 5, DEV_CLASS_VIDEO,   DEV_VENDOR_SYSTEM, "Video"        },
    { 6, DEV_CLASS_RNG,     DEV_VENDOR_SYSTEM, "RNG"          },
    { 7, DEV_CLASS_FPU,     DEV_VENDOR_SYSTEM, "FPU"          },
    { 8, DEV_CLASS_AUDIO,   DEV_VENDOR_SYSTEM, "PSG"          },
};

#define DEV_COUNT (sizeof(g_devices) / sizeof(g_devices[0]))

bool devclass_is_mmio(uint32_t addr) {
    return (addr >= DEV_ADDR_BASE && addr <= DEV_ADDR_END);
}

static int devclass_get_selected(Cpu* cpu) {
    int sel = cpu->devclass_selected;
    if (sel < 0 || sel >= (int)DEV_COUNT) sel = 0;
    return sel;
}

uint8_t devclass_read_byte(Cpu* cpu, uint32_t addr) {
    if (!cpu) return 0;

    if (addr == DEV_ADDR_COUNT) {
        return (uint8_t)DEV_COUNT;
    }

    if (addr == DEV_ADDR_SELECT) {
        return (uint8_t)devclass_get_selected(cpu);
    }

    int sel = devclass_get_selected(cpu);
    const DeviceClassEntry* dev = &g_devices[sel];

    if (addr == DEV_ADDR_CLASS)  return dev->class_id;
    if (addr == DEV_ADDR_VENDOR) return dev->vendor_id;

    if (addr >= DEV_ADDR_NAME && addr < DEV_ADDR_NAME + DEV_MAX_NAME_LEN) {
        int idx = (int)(addr - DEV_ADDR_NAME);
        const char* name = dev->name ? dev->name : "";
        size_t len = strlen(name);
        if ((size_t)idx < len) return (uint8_t)name[idx];
        return 0;
    }

    return 0;
}

void devclass_write_byte(Cpu* cpu, uint32_t addr, uint8_t val) {
    if (!cpu) return;
    if (addr == DEV_ADDR_SELECT) {
        if (val < DEV_COUNT) {
            cpu->devclass_selected = (int)val;
        }
    }
}
