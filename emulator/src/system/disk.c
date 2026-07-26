// ------------------------------------------------------------------------------
//          disk.c - Disk controller implementation
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

#include "system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#ifndef strcasecmp
static int strcasecmp_win(const char* a, const char* b) {
    while (*a && *b) {
        int ca = tolower((unsigned char)*a);
        int cb = tolower((unsigned char)*b);
        if (ca != cb) return ca - cb;
        a++; b++;
    }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}
#define strcasecmp strcasecmp_win
#endif
#endif

#define DISK_ACTIVE(cpu) (&(cpu)->disk_drives[(cpu)->disk_current_drive])

static uint64_t parse_hex_token(const char* s, int* out_digits) {
    uint64_t val = 0;
    int digits = 0;
    while (*s) {
        int c = *s++;
        int n = -1;
        if (c >= '0' && c <= '9') n = c - '0';
        else if (c >= 'a' && c <= 'f') n = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') n = c - 'A' + 10;
        else break;
        val = (val << 4) | (uint64_t)n;
        digits++;
    }
    *out_digits = digits;
    return val;
}

// Parse a Logisim/plain hex file into a freshly allocated byte buffer.
static uint8_t* load_hex_buffer(const char* filename, size_t* out_size) {
    FILE* f = fopen(filename, "r");
    if (!f) return NULL;

    char line[1024];
    if (!fgets(line, sizeof(line), f)) { fclose(f); return NULL; }
    bool is_logisim = (strncmp(line, "v2.0", 4) == 0);
    if (!is_logisim) fseek(f, 0, SEEK_SET);

    size_t cap = 4096;
    size_t size = 0;
    uint8_t* data = (uint8_t*)malloc(cap);
    if (!data) { fclose(f); return NULL; }

    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;
        if (line[0] == 0) continue;
        if (!is_logisim && (line[0] == '#' || line[0] == '/')) continue;

        char* p = line;
        while (*p) {
            while (*p && isspace((unsigned char)*p)) p++;
            if (!*p) break;
            if (*p == '#' || *p == '/') break;

            char* end = p;
            while (*end && !isspace((unsigned char)*end) && *end != '#' && *end != '/') end++;

            char token[128];
            size_t tok_len = (size_t)(end - p);
            if (tok_len >= sizeof(token)) tok_len = sizeof(token) - 1;
            memcpy(token, p, tok_len);
            token[tok_len] = '\0';
            p = end;

            char* star = strchr(token, '*');
            uint64_t val = 0;
            int digits = 0;
            int repeat = 1;
            if (star) {
                *star = '\0';
                val = parse_hex_token(token, &digits);
                repeat = atoi(star + 1);
                if (repeat < 1) repeat = 1;
            } else {
                val = parse_hex_token(token, &digits);
            }
            if (digits == 0) continue;

            int byte_len = (digits + 1) / 2;
            if (byte_len < 1) byte_len = 1;

            for (int r = 0; r < repeat; r++) {
                for (int i = byte_len - 1; i >= 0; i--) {
                    uint8_t b = (uint8_t)((val >> (i * 8)) & 0xFF);
                    if (size + 1 > cap) {
                        cap *= 2;
                        uint8_t* nd = (uint8_t*)realloc(data, cap);
                        if (!nd) { free(data); fclose(f); return NULL; }
                        data = nd;
                    }
                    data[size++] = b;
                }
            }
        }
    }
    fclose(f);
    *out_size = size;
    return data;
}

static bool disk_drive_ensure_hex_image(DiskDrive* drive) {
    if (drive->image_data) return true;
    size_t sz = 0;
    uint8_t* data = load_hex_buffer(drive->image_path, &sz);
    if (!data) return false;
    drive->image_data = data;
    drive->image_size = sz;
    return true;
}

static void disk_drive_free_image(DiskDrive* drive) {
    if (drive->image_data) {
        free(drive->image_data);
        drive->image_data = NULL;
        drive->image_size = 0;
    }
}

void disk_free_image(Cpu* cpu) {
    for (int i = 0; i < DISK_MAX_DRIVES; i++) {
        disk_drive_free_image(&cpu->disk_drives[i]);
    }
}

// Read one sector from the active drive's image into its sector buffer.
static bool disk_drive_read_sector(DiskDrive* drive) {
    const char* ext = strrchr(drive->image_path, '.');
    bool is_hex = (ext && strcasecmp(ext, ".hex") == 0);
    uint32_t off = drive->lba * DISK_SECTOR_SIZE;

    memset(drive->sector_buffer, 0, DISK_SECTOR_SIZE);

    if (is_hex) {
        if (!disk_drive_ensure_hex_image(drive)) return false;
        if (off < drive->image_size) {
            size_t n = drive->image_size - off;
            if (n > DISK_SECTOR_SIZE) n = DISK_SECTOR_SIZE;
            memcpy(drive->sector_buffer, drive->image_data + off, n);
        }
        return true;
    }

    FILE* f = fopen(drive->image_path, "rb");
    if (!f) return false;
    if (fseek(f, off, SEEK_SET) != 0) { fclose(f); return false; }
    fread(drive->sector_buffer, 1, DISK_SECTOR_SIZE, f);
    fclose(f);
    return true;
}

// Write the active drive's sector buffer to its image at the current LBA.
static bool disk_drive_write_sector(DiskDrive* drive) {
    const char* ext = strrchr(drive->image_path, '.');
    bool is_hex = (ext && strcasecmp(ext, ".hex") == 0);
    if (is_hex) return false;

    FILE* f = fopen(drive->image_path, "rb+");
    if (!f) f = fopen(drive->image_path, "wb");
    if (!f) return false;

    uint32_t off = drive->lba * DISK_SECTOR_SIZE;
    if (fseek(f, off, SEEK_SET) != 0) { fclose(f); return false; }
    size_t written = fwrite(drive->sector_buffer, 1, DISK_SECTOR_SIZE, f);
    fclose(f);
    return written == DISK_SECTOR_SIZE;
}

// Advance to the next sector during a multi-sector operation.
static bool disk_advance_sector(DiskDrive* drive) {
    if (drive->count > 1) {
        drive->count--;
        drive->lba++;
        drive->buffer_offset = 0;
        return disk_drive_read_sector(drive);
    }
    drive->status = 0;
    return true;
}

// Execute a disk command on the active drive.
static void disk_command(Cpu* cpu) {
    DiskDrive* drive = DISK_ACTIVE(cpu);

    switch (drive->ctrl) {
        case 0: // idle / reset
            drive->status &= ~DISK_STATUS_BUSY;
            break;

        case 1: // acknowledge ready
            drive->status = 0;
            break;

        case 2: { // read single sector
            drive->status = DISK_STATUS_BUSY;
            drive->buffer_offset = 0;
            if (drive->image_path[0] == '\0' || !disk_drive_read_sector(drive)) {
                drive->status = DISK_STATUS_ERROR;
            } else {
                drive->status = 0;
            }
            break;
        }

        case 3: { // read multiple sectors (count)
            drive->status = DISK_STATUS_BUSY;
            drive->buffer_offset = 0;
            if (drive->image_path[0] == '\0' || drive->count == 0 || !disk_drive_read_sector(drive)) {
                drive->status = DISK_STATUS_ERROR;
            } else {
                drive->status = 0;
            }
            break;
        }

        case 4: { // write single sector
            drive->status = DISK_STATUS_BUSY;
            if (drive->image_path[0] == '\0' || !disk_drive_write_sector(drive)) {
                drive->status = DISK_STATUS_ERROR;
            } else {
                drive->status = 0;
            }
            break;
        }

        case 5: { // write multiple sectors (count)
            drive->status = DISK_STATUS_BUSY;
            drive->buffer_offset = 0;
            if (drive->image_path[0] == '\0' || drive->count == 0) {
                drive->status = DISK_STATUS_ERROR;
            }
            break;
        }

        case 8: { // write pending DIN dword into sector buffer
            uint32_t off = drive->buffer_offset;
            if (off + 4 <= DISK_SECTOR_SIZE) {
                uint32_t val = drive->din_shadow;
                drive->sector_buffer[off + 0] = (uint8_t)(val >> 24);
                drive->sector_buffer[off + 1] = (uint8_t)(val >> 16);
                drive->sector_buffer[off + 2] = (uint8_t)(val >> 8);
                drive->sector_buffer[off + 3] = (uint8_t)(val);
                drive->buffer_offset = off + 4;
            }

            // During multi-sector writes, flush the buffer and advance
            // automatically when a sector is full.
            if (drive->ctrl == 5 && drive->buffer_offset >= DISK_SECTOR_SIZE) {
                if (!disk_drive_write_sector(drive)) {
                    drive->status = DISK_STATUS_ERROR;
                } else if (drive->count > 1) {
                    drive->count--;
                    drive->lba++;
                    drive->buffer_offset = 0;
                    drive->status = DISK_STATUS_BUSY;
                } else {
                    drive->status = 0;
                }
            } else {
                drive->status = 0;
            }
            break;
        }

        case 0x10: { // format: zero-fill count sectors starting at LBA
            drive->status = DISK_STATUS_BUSY;
            if (drive->image_path[0] == '\0' || drive->count == 0) {
                drive->status = DISK_STATUS_ERROR;
                break;
            }
            memset(drive->sector_buffer, 0, DISK_SECTOR_SIZE);
            bool ok = true;
            uint32_t start_lba = drive->lba;
            for (uint32_t i = 0; i < drive->count && ok; i++) {
                drive->lba = start_lba + i;
                if (!disk_drive_write_sector(drive)) {
                    ok = false;
                }
            }
            drive->lba = start_lba;
            drive->status = ok ? 0 : DISK_STATUS_ERROR;
            break;
        }

        default:
            drive->status = DISK_STATUS_ERROR;
            break;
    }
}

uint8_t disk_read_byte(Cpu* cpu) {
    DiskDrive* drive = DISK_ACTIVE(cpu);
    uint32_t off = drive->buffer_offset;
    uint8_t val = (off < DISK_SECTOR_SIZE) ? drive->sector_buffer[off] : 0;
    drive->buffer_offset = off + 1;

    // During multi-sector reads, auto-advance when the end of a sector is reached.
    if (cpu->disk_drives[cpu->disk_current_drive].ctrl == 3 && drive->buffer_offset >= DISK_SECTOR_SIZE) {
        disk_advance_sector(drive);
    }

    return val;
}

static uint32_t disk_read_dword_from_image(Cpu* cpu) {
    DiskDrive* drive = DISK_ACTIVE(cpu);
    uint32_t off = drive->buffer_offset;
    uint32_t val = 0;
    for (int i = 0; i < 4; i++) {
        uint8_t b = (off + i < DISK_SECTOR_SIZE) ? drive->sector_buffer[off + i] : 0;
        val = (val << 8) | b;
    }
    drive->buffer_offset = off + 4;

    // During multi-sector reads, auto-advance when the end of a sector is reached.
    if (drive->ctrl == 3 && drive->buffer_offset >= DISK_SECTOR_SIZE) {
        disk_advance_sector(drive);
    }

    return val;
}

void disk_write_dword(Cpu* cpu, uint32_t addr, uint32_t val) {
    DiskDrive* drive = DISK_ACTIVE(cpu);
    switch (addr) {
        case 0x00020110: drive->status = val; break;
        case 0x00020111: drive->ctrl = val; disk_command(cpu); break;
        case 0x00020112: drive->lba = val; break;
        case 0x00020113: drive->lba = val; break;
        case 0x00020114: drive->buffer = val; break;
        case 0x00020115: drive->count = val; break;
        case 0x00020116:
            if (val < DISK_MAX_DRIVES) {
                cpu->disk_current_drive = (uint8_t)val;
            }
            break;
        case 0x00020117: drive->status = val; break;
        case 0x0002011B: drive->din_shadow = val; break; // DISK_DIN
        case 0x0002011C: drive->buffer_offset = val; break;
    }
}

void disk_write_byte(Cpu* cpu, uint32_t addr, uint8_t val) {
    DiskDrive* drive = DISK_ACTIVE(cpu);
    switch (addr) {
        case 0x00020110: drive->status = val; break;
        case 0x00020111: drive->ctrl = val; disk_command(cpu); break;
        case 0x00020112: drive->lba = (drive->lba & 0xFFFFFF00) | val; break;
        case 0x00020113: drive->lba = (drive->lba & 0xFF00FFFF) | (val << 16); break;
        case 0x00020114: drive->buffer = (drive->buffer & 0xFFFFFF00) | val; break;
        case 0x00020115: drive->count = val; break;
        case 0x00020116:
            if (val < DISK_MAX_DRIVES) {
                cpu->disk_current_drive = val;
            }
            break;
        case 0x00020117: drive->status = val; break;
        case 0x0002011B: drive->din_shadow = (drive->din_shadow & 0xFFFFFF00) | val; break;
        case 0x0002011C: drive->buffer_offset = (drive->buffer_offset & 0xFFFFFF00) | val; break;
    }
}

uint32_t disk_read_dword(Cpu* cpu, uint32_t addr) {
    switch (addr) {
        case 0x00020110: return DISK_ACTIVE(cpu)->status;
        case 0x0002011A: return disk_read_dword_from_image(cpu); // DISK_DOUT
        case 0x0002011C: return DISK_ACTIVE(cpu)->buffer_offset;
    }
    return 0;
}

uint8_t kbd_read_ascii(Cpu* cpu) {
    if (cpu->kbd_buffer_pos >= cpu->kbd_buffer_len) {
        if (cpu->gui_mode) {
            return 0;
        }
        if (!fgets(cpu->kbd_buffer, sizeof(cpu->kbd_buffer), stdin)) {
            return 0;
        }
        cpu->kbd_buffer_len = (int)strlen(cpu->kbd_buffer);
        cpu->kbd_buffer_pos = 0;
    }
    uint8_t c = (uint8_t)cpu->kbd_buffer[cpu->kbd_buffer_pos++];
    if (c == '\n') c = '\r';
    return c;
}
