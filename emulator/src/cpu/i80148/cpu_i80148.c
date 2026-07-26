// ------------------------------------------------------------------------------
//          cpu_i80148.c - i80148 CPU backend implementation
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

#include "cpu_i80148.h"
#include "cpu_api.h"
#include "system.h"
#include "rtc.h"
#include "pit.h"
#include "input.h"
#include "devclass.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Access i80148-specific state stored in cpu->backend_data
static inline i80148_State* state(Cpu* cpu) {
    return (i80148_State*)cpu->backend_data;
}

static void term_clear(Cpu* cpu);

void cpu_init_i80148(Cpu* cpu) {
    if (cpu->backend_data) {
        memset(cpu->backend_data, 0, sizeof(i80148_State));
    }
    cpu->term_attr = 0x07;
    cpu->irq_enabled = false;
    cpu->halted = false;
    cpu->screen_dirty = true;
    input_init(cpu);
    pit_reset_pending(cpu);
    term_clear(cpu);
}

void cpu_reset_i80148(Cpu* cpu) {
    if (cpu->backend_data) {
        memset(cpu->backend_data, 0, sizeof(i80148_State));
    }
    cpu->halted = false;
    cpu->irq_enabled = false;
    cpu->screen_dirty = true;
    cpu->term_attr = 0x07;
    input_reset(cpu);
    pit_reset_pending(cpu);
    term_clear(cpu);
}

static const uint64_t MODE_MASKS[] = {
    0xFF,           // BYTE
    0xFFFF,         // WORD
    0xFFFFFFFF,     // DWORD
    0xFFFFFFFFFFFFFFFF,
    0xFFFFFFFFFFFFFFFF
};

static int mode_size(CpuMode mode) {
    if (mode == MODE_BYTE) return 1;
    if (mode == MODE_WORD) return 2;
    if (mode == MODE_DWORD) return 4;
    return 4;
}

static CpuMode reg_default_mode(uint8_t idx) {
    if (idx == 0) return MODE_DWORD; // r0
    if (idx >= 0x01 && idx <= 0x07) return MODE_DWORD; // EX1-EX7
    if (idx >= 0x08 && idx <= 0x0E) return MODE_WORD;  // X1-X7
    if (idx >= 0x0F && idx <= 0x15) return MODE_BYTE;  // XL1-XL7
    if (idx == REG_FL) return MODE_BYTE;
    return MODE_DWORD;
}

// Register file write with size handling and aliasing:
// X1-X7 are the low words of EX1-EX7, XL1-XL7 are the low bytes of EX1-EX7.
void cpu_set_reg_i80148(Cpu* cpu, uint8_t idx, uint64_t val) {
    if (idx >= REG_COUNT) return;
    CpuMode mode = reg_default_mode(idx);
    val &= MODE_MASKS[mode];

    if (idx >= 0x08 && idx <= 0x0E) {          // X1-X7 -> low word of EX1-EX7
        uint8_t base = idx - 0x07;
        state(cpu)->regs[base] = (state(cpu)->regs[base] & ~0xFFFFULL) | (val & 0xFFFF);
    } else if (idx >= 0x0F && idx <= 0x15) {   // XL1-XL7 -> low byte of EX1-EX7
        uint8_t base = idx - 0x0E;
        state(cpu)->regs[base] = (state(cpu)->regs[base] & ~0xFFULL) | (val & 0xFF);
    } else {
        state(cpu)->regs[idx] = val;
    }
}

uint64_t cpu_get_reg_i80148(Cpu* cpu, uint8_t idx) {
    if (idx >= REG_COUNT) return 0;
    CpuMode mode = reg_default_mode(idx);

    if (idx >= 0x08 && idx <= 0x0E) {          // X1-X7 -> low word of EX1-EX7
        return state(cpu)->regs[idx - 0x07] & 0xFFFF;
    }
    if (idx >= 0x0F && idx <= 0x15) {          // XL1-XL7 -> low byte of EX1-EX7
        return state(cpu)->regs[idx - 0x0E] & 0xFF;
    }
    return state(cpu)->regs[idx] & MODE_MASKS[mode];
}

// Translate a VRAM window address (0x00050000..0x0005FFFF) into the physical
// VRAM address selected by the bank register at 0x00020007.
static inline uint32_t vram_window_translate(Cpu* cpu, uint32_t addr) {
    uint32_t bank = 0;
    if (VRAM_BANK_REG_I80148 < cpu->mem_size) {
        bank = cpu->mem[VRAM_BANK_REG_I80148];
    }
    return VBUFFER_BASE_I80148 + ((uint32_t)bank * VRAM_WINDOW_SIZE) + (addr - VRAM_WINDOW_BASE);
}

// Big-endian memory access
uint64_t cpu_read_mem_i80148(Cpu* cpu, uint32_t addr, int mode) {
    // VRAM window read (0x00050000..0x0005FFFF).
    if (addr >= VRAM_WINDOW_BASE && addr < VRAM_WINDOW_BASE + VRAM_WINDOW_SIZE) {
        addr = vram_window_translate(cpu, addr);
    }

    if (mode == MODE_DWORD && addr == 0x0002000C) {
        return (uint32_t)cpu->mem_size;
    }
    if (mode == MODE_BYTE && addr == KBD_ASCII_ADDR_I80148) {
        return kbd_read_ascii((Cpu*)cpu);
    }
    // Mouse MMIO reads (0x20040..0x20053)
    if (mode == MODE_BYTE && input_is_mmio(addr)) {
        return input_read_byte(cpu, addr);
    }
    if (mode == MODE_DWORD && input_is_mmio(addr)) {
        return input_read_dword(cpu, addr);
    }
    // RTC MMIO reads (0x20020..0x2002F)
    if (mode == MODE_BYTE && rtc_is_mmio(addr)) {
        return rtc_read_byte(addr);
    }
    if (mode == MODE_DWORD && addr == RTC_ADDR_UNIX_TIME) {
        return rtc_read_dword(addr);
    }
    // PIT MMIO reads (0x20030..0x2003F)
    if (mode == MODE_BYTE && pit_is_mmio(addr)) {
        return pit_read_byte(cpu, addr);
    }
    if (mode == MODE_DWORD && pit_is_mmio(addr)) {
        return pit_read_dword(cpu, addr);
    }
    // Device class enumeration MMIO reads (0x20200..0x2023F).
    if (mode == MODE_BYTE && devclass_is_mmio(addr)) {
        return devclass_read_byte(cpu, addr);
    }

    // Disk controller reads (0x20110..0x2011F) take precedence over generic XPB.
    if (mode == MODE_DWORD && addr >= 0x00020110 && addr <= 0x0002011F) {
        return disk_read_dword((Cpu*)cpu, addr);
    }
    if (mode == MODE_BYTE && addr == 0x0002011A) {
        return disk_read_byte((Cpu*)cpu);
    }
    // XPB device registers (slot 1 = system/disk for PC48)
    if (mode == MODE_DWORD && addr >= 0x00020100 && addr < 0x00020200) {
        uint32_t slot = (addr - 0x00020000) >> 8;
        uint32_t reg  = addr & 0xFF;
        if (slot == 1 && reg == 0x00) {
            return 0x00000101; // DEV_DATA: class=1 (storage), vendor=1 (system)
        }
        return 0;
    }
    if (addr >= cpu->mem_size) return 0;
    int bytes = mode_size(mode);
    uint64_t val = 0;
    for (int i = 0; i < bytes; i++) {
        uint32_t a = addr + i;
        if (a >= cpu->mem_size) break;
        val = (val << 8) | cpu->mem[a];
    }
    return val;
}

static void term_putchar(Cpu* cpu, char c);
static void term_clear(Cpu* cpu);
static void push(Cpu* cpu, uint64_t val);
static uint64_t pop(Cpu* cpu);

static void cpu_trigger_int(Cpu* cpu, uint8_t vector) {
    uint32_t ic_after = (uint32_t)state(cpu)->regs[REG_IC];
    push(cpu, ic_after);
    push(cpu, state(cpu)->regs[REG_FL]);
    cpu->irq_enabled = false;
    uint32_t vec_addr = (uint32_t)(state(cpu)->regs[REG_IDTR] + ((uint32_t)vector << 2));
    state(cpu)->regs[REG_IC] = cpu_read_mem_i80148(cpu, vec_addr, MODE_DWORD);
}

void cpu_write_mem_i80148(Cpu* cpu, uint32_t addr, uint64_t val, int mode) {
    // VRAM window write (0x00050000..0x0005FFFF).
    if (addr >= VRAM_WINDOW_BASE && addr < VRAM_WINDOW_BASE + VRAM_WINDOW_SIZE) {
        addr = vram_window_translate(cpu, addr);
    }

    // MMIO intercepts (ignore access size, like real hardware does)
    if (addr == TERM_OUT_ADDR_I80148) {
        term_putchar(cpu, (char)(val & 0xFF));
        return;
    }
    if (addr == TERM_RESET_ADDR_I80148) {
        term_clear(cpu);
        return;
    }
    if (addr == VC_MODE_ADDR_I80148) {
        if (addr < cpu->mem_size) {
            cpu->mem[addr] = (uint8_t)(val & 0xFF);
            cpu->screen_dirty = true;
        }
        return;
    }
    if (mode == MODE_BYTE && rtc_is_mmio(addr)) {
        rtc_write_byte(addr, (uint8_t)(val & 0xFF));
        return;
    }
    if (mode == MODE_DWORD && addr == RTC_ADDR_UNIX_TIME) {
        rtc_write_dword(addr, (uint32_t)(val & 0xFFFFFFFF));
        return;
    }
    if (mode == MODE_BYTE && pit_is_mmio(addr)) {
        pit_write_byte(cpu, addr, (uint8_t)(val & 0xFF));
        return;
    }
    if (mode == MODE_DWORD && pit_is_mmio(addr)) {
        pit_write_dword(cpu, addr, (uint32_t)(val & 0xFFFFFFFF));
        return;
    }
    if (mode == MODE_BYTE && devclass_is_mmio(addr)) {
        devclass_write_byte(cpu, addr, (uint8_t)(val & 0xFF));
        return;
    }

    if (mode == MODE_BYTE) {
        if (addr >= 0x00020110 && addr <= 0x0002011F) {
            disk_write_byte(cpu, addr, val & 0xFF);
            return;
        }
    }
    if (mode == MODE_DWORD && addr >= 0x00020110 && addr <= 0x0002011F) {
        disk_write_dword(cpu, addr, (uint32_t)(val & 0xFFFFFFFF));
        return;
    }

    if (addr >= cpu->mem_size) return;
    int bytes = mode_size(mode);
    for (int i = 0; i < bytes; i++) {
        uint32_t a = addr + i;
        if (a >= cpu->mem_size) break;
        cpu->mem[a] = (val >> ((bytes - 1 - i) * 8)) & 0xFF;
    }
}

static void term_scroll(Cpu* cpu) {
    // Scroll text buffer up by one line
    uint32_t base = VBUFFER_BASE_I80148;
    for (int y = 0; y < TERM_ROWS_I80148 - 1; y++) {
        for (int x = 0; x < TERM_COLS_I80148; x++) {
            cpu->mem[base + (y * TERM_COLS_I80148 + x) * 2] = cpu->mem[base + ((y + 1) * TERM_COLS_I80148 + x) * 2];
            cpu->mem[base + (y * TERM_COLS_I80148 + x) * 2 + 1] = cpu->mem[base + ((y + 1) * TERM_COLS_I80148 + x) * 2 + 1];
        }
    }
    // Clear last line
    for (int x = 0; x < TERM_COLS_I80148; x++) {
        cpu->mem[base + ((TERM_ROWS_I80148 - 1) * TERM_COLS_I80148 + x) * 2] = ' ';
        cpu->mem[base + ((TERM_ROWS_I80148 - 1) * TERM_COLS_I80148 + x) * 2 + 1] = cpu->term_attr;
    }
}

static void term_clear(Cpu* cpu) {
    // Clear the whole text buffer and home the cursor (TERM_RESET semantics)
    uint32_t base = VBUFFER_BASE_I80148;
    if (base + (uint32_t)(TERM_COLS_I80148 * TERM_ROWS_I80148 * 2) > cpu->mem_size) return;
    for (int i = 0; i < TERM_COLS_I80148 * TERM_ROWS_I80148; i++) {
        cpu->mem[base + i * 2] = ' ';
        cpu->mem[base + i * 2 + 1] = cpu->term_attr;
    }
    cpu->term_cursor_x = 0;
    cpu->term_cursor_y = 0;
    cpu->screen_dirty = true;
}

static void term_putchar(Cpu* cpu, char c) {
    putchar(c);
    fflush(stdout);
    cpu->screen_dirty = true;

    uint32_t base = VBUFFER_BASE_I80148;
    if (c == '\n' || c == '\r') {
        cpu->term_cursor_x = 0;
        cpu->term_cursor_y++;
        if (cpu->term_cursor_y >= TERM_ROWS_I80148) {
            cpu->term_cursor_y = TERM_ROWS_I80148 - 1;
            term_scroll(cpu);
        }
        return;
    }
    if (c == '\b') {
        if (cpu->term_cursor_x > 0) cpu->term_cursor_x--;
        return;
    }

    int idx = cpu->term_cursor_y * TERM_COLS_I80148 + cpu->term_cursor_x;
    if (idx < TERM_COLS_I80148 * TERM_ROWS_I80148) {
        cpu->mem[base + idx * 2] = (uint8_t)c;
        cpu->mem[base + idx * 2 + 1] = cpu->term_attr;
    }
    cpu->term_cursor_x++;
    if (cpu->term_cursor_x >= TERM_COLS_I80148) {
        cpu->term_cursor_x = 0;
        cpu->term_cursor_y++;
        if (cpu->term_cursor_y >= TERM_ROWS_I80148) {
            cpu->term_cursor_y = TERM_ROWS_I80148 - 1;
            term_scroll(cpu);
        }
    }
}

// Disk controller provided by emulator/src/system/disk.c

// ==================== FILE LOADING ====================

int cpu_load_bin_i80148(Cpu* cpu, const char* filename, uint32_t load_addr) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        printf("[ERR] Cannot open file: %s\n", filename);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (load_addr + fsize > cpu->mem_size) {
        printf("[ERR] File too large\n");
        fclose(f);
        return -1;
    }
    fread(&cpu->mem[load_addr], 1, fsize, f);
    fclose(f);
    printf("[INFO] Loaded %ld bytes to 0x%08X\n", fsize, load_addr);
    return 0;
}

static uint64_t parse_hex_token(const char* s, int* out_digits) {
    uint64_t val = 0;
    int digits = 0;
    while (*s) {
        char c = *s++;
        int n = -1;
        if (c >= '0' && c <= '9') n = c - '0';
        else if (c >= 'a' && c <= 'f') n = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') n = c - 'A' + 10;
        else break;
        val = (val << 4) | (uint64_t)n;
        digits++;
        if (digits >= 16) break;
    }
    *out_digits = digits;
    return val;
}

int cpu_load_hex_i80148(Cpu* cpu, const char* filename, uint32_t base_addr) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        printf("[ERR] Cannot open file: %s\n", filename);
        return -1;
    }
    char line[1024];
    if (!fgets(line, sizeof(line), f)) { fclose(f); return -1; }
    bool is_logisim = (strncmp(line, "v2.0", 4) == 0);
    if (!is_logisim) fseek(f, 0, SEEK_SET);

    uint32_t addr = base_addr;
    int total_bytes = 0;
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;
        if (line[0] == 0) continue;
        if (!is_logisim && (line[0] == '#' || line[0] == '/')) continue;

        char* p = line;
        while (*p) {
            while (*p && isspace((unsigned char)*p)) p++;
            if (!*p) break;
            if (*p == '#' || *p == '/') break; // comment until end of line

            char* end = p;
            while (*end && !isspace((unsigned char)*end) && *end != '#' && *end != '/') end++;

            char token[128];
            size_t tok_len = (size_t)(end - p);
            if (tok_len >= sizeof(token)) tok_len = sizeof(token) - 1;
            memcpy(token, p, tok_len);
            token[tok_len] = '\0';
            p = end;

            // Logisim supports value*count (e.g. 0*10 or 00000000*4)
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

            // Determine how many bytes this token represents.
            // For Logisim hex each token is one memory word, so width comes from digit count.
            // For plain hex we do the same: a token like DEADBEEF becomes 4 bytes.
            int byte_len = (digits + 1) / 2;
            if (byte_len < 1) byte_len = 1;

            for (int r = 0; r < repeat; r++) {
                for (int i = byte_len - 1; i >= 0; i--) {
                    uint8_t b = (uint8_t)((val >> (i * 8)) & 0xFF);
                    if (addr < cpu->mem_size) {
                        cpu->mem[addr++] = b;
                        total_bytes++;
                    }
                }
            }
        }
    }
    fclose(f);
    printf("[INFO] Loaded %d bytes from hex '%s' to 0x%08X\n", total_bytes, filename, base_addr);
    return 0;
}

int cpu_load_file_i80148(Cpu* cpu, const char* filename, uint32_t load_addr) {
    const char* ext = strrchr(filename, '.');
    if (ext && (strcasecmp(ext, ".hex") == 0)) {
        return cpu_load_hex_i80148(cpu, filename, load_addr);
    }
    return cpu_load_bin_i80148(cpu, filename, load_addr);
}

// ==================== FLAG LOGIC ====================

static void update_flags(Cpu* cpu, uint64_t res, uint64_t a, uint64_t b, uint8_t mask, CpuMode mode, bool is_sub, bool is_signed) {
    uint64_t* fl = &state(cpu)->regs[REG_FL];
    int bits = mode_size(mode) * 8;
    uint64_t sign_bit = (bits < 64) ? (1ULL << (bits - 1)) : 0;

    if (mask & FLAG_Z) {
        *fl = (*fl & ~FLAG_Z) | ((res == 0) ? FLAG_Z : 0);
    }
    if (mask & FLAG_S) {
        if (bits < 64) {
            *fl = (*fl & ~FLAG_S) | ((res & sign_bit) ? FLAG_S : 0);
        } else {
            *fl = (*fl & ~FLAG_S) | ((res & (1ULL<<63)) ? FLAG_S : 0);
        }
    }

    if (is_sub) {
        if (is_signed) {
            int64_t sa = (int64_t)(a << (64 - bits)) >> (64 - bits);
            int64_t sb = (int64_t)(b << (64 - bits)) >> (64 - bits);
            if (mask & FLAG_B) *fl = (*fl & ~FLAG_B) | ((sa < sb) ? FLAG_B : 0);
            if (mask & FLAG_C) *fl = (*fl & ~FLAG_C) | (((uint64_t)sa < (uint64_t)sb) ? FLAG_C : 0);
        } else {
            if (mask & FLAG_B) *fl = (*fl & ~FLAG_B) | ((a < b) ? FLAG_B : 0);
            if (mask & FLAG_C) *fl = (*fl & ~FLAG_C) | ((a < b) ? FLAG_C : 0);
        }
    } else {
        if (mask & FLAG_C) {
            uint64_t max = (bits < 64) ? ((1ULL << bits) - 1) : 0xFFFFFFFFFFFFFFFFULL;
            *fl = (*fl & ~FLAG_C) | (((a + b) > max) ? FLAG_C : 0);
        }
    }

    if (mask & FLAG_O) {
        // Overflow for signed arithmetic
        if (bits < 64) {
            int64_t sa = (int64_t)(a << (64 - bits)) >> (64 - bits);
            int64_t sb = (int64_t)(b << (64 - bits)) >> (64 - bits);
            int64_t sr = (int64_t)(res << (64 - bits)) >> (64 - bits);
            bool ov = ((sa >= 0 && sb >= 0 && sr < 0) || (sa < 0 && sb < 0 && sr >= 0));
            if (is_sub) {
                ov = ((sa >= 0 && sb < 0 && sr < 0) || (sa < 0 && sb >= 0 && sr >= 0));
            }
            *fl = (*fl & ~FLAG_O) | (ov ? FLAG_O : 0);
        }
    }

    if (is_signed) {
        int64_t sa = (int64_t)(a << (64 - bits)) >> (64 - bits);
        int64_t sb = (int64_t)(b << (64 - bits)) >> (64 - bits);
        if (mask & FLAG_G) *fl = (*fl & ~FLAG_G) | ((sa > sb) ? FLAG_G : 0);
        if (mask & FLAG_E) *fl = (*fl & ~FLAG_E) | ((sa == sb) ? FLAG_E : 0);
        if (mask & FLAG_L) *fl = (*fl & ~FLAG_L) | ((sa < sb) ? FLAG_L : 0);
    } else {
        if (mask & FLAG_G) *fl = (*fl & ~FLAG_G) | ((a > b) ? FLAG_G : 0);
        if (mask & FLAG_E) *fl = (*fl & ~FLAG_E) | ((a == b) ? FLAG_E : 0);
        if (mask & FLAG_L) *fl = (*fl & ~FLAG_L) | ((a < b) ? FLAG_L : 0);
    }
}

bool cpu_check_cond_i80148(Cpu* cpu, uint8_t cond) {
    uint64_t fl = state(cpu)->regs[REG_FL];
    switch (cond) {
        case COND_UNC: return true;
        case COND_CF:  return (fl & FLAG_C) != 0;
        case COND_BF:  return (fl & FLAG_B) != 0;
        case COND_SF:  return (fl & FLAG_S) != 0;
        case COND_OF:  return (fl & FLAG_O) != 0;
        case COND_ZF:  return (fl & FLAG_Z) != 0;
        case COND_NZ:  return (fl & FLAG_Z) == 0;
        case COND_GR:  return (fl & FLAG_G) != 0;
        case COND_GE:  return (fl & FLAG_G) != 0 || (fl & FLAG_E) != 0;
        case COND_LS:  return (fl & FLAG_L) != 0;
        case COND_LE:  return (fl & FLAG_L) != 0 || (fl & FLAG_E) != 0;
        case COND_EQ:  return (fl & FLAG_E) != 0;
        case COND_NE:  return (fl & FLAG_E) == 0;
        default: return false;
    }
}

// ==================== STACK HELPERS ====================

static void push(Cpu* cpu, uint64_t val) {
    uint32_t sp = (uint32_t)state(cpu)->regs[REG_SP];
    sp += 4;
    state(cpu)->regs[REG_SP] = sp;
    cpu_write_mem_i80148(cpu, sp, val & 0xFFFFFFFF, MODE_DWORD);
}

static uint64_t pop(Cpu* cpu) {
    uint32_t sp = (uint32_t)state(cpu)->regs[REG_SP];
    uint64_t val = cpu_read_mem_i80148(cpu, sp, MODE_DWORD);
    state(cpu)->regs[REG_SP] = sp - 4;
    return val;
}

// ==================== INSTRUCTION DECODE & EXECUTE ====================

int cpu_step_i80148(Cpu* cpu) {
    if (cpu->halted) return -1;

    if (cpu->irq_enabled) {
        int irq_vector = input_service_irq(cpu);
        if (irq_vector < 0) {
            irq_vector = pit_service_irq(cpu);
        }
        if (irq_vector >= 0) {
            cpu_trigger_int(cpu, (uint8_t)irq_vector);
            return 1;
        }
    }

    uint32_t ic = (uint32_t)state(cpu)->regs[REG_IC];
    if (ic >= cpu->mem_size) { cpu->halted = true; return 0; }

    uint8_t op = cpu->mem[ic++];
    uint8_t sf = 0;
    int has_sf = (op >= 0x03 && op <= 0x0A) || op == 0x19 || op == 0x1A || op == 0x1B ||
                 op == 0x1C || op == 0x1D || op == 0x21 || op == 0x22 || op == 0x23 ||
                 op == 0x29 || op == 0x2A;
    if (has_sf) sf = cpu->mem[ic++];

    uint8_t rd = 0, rs = 0;
    uint32_t imm32 = 0;
    uint8_t cond = 0;
    int len = has_sf ? 2 : 1;

    // Mode from suffix or register lookup
    CpuMode mode;
    if (has_sf) {
        bool is_a_family = (op == 0x03 || op == 0x04 || op == 0x05 || op == 0x06 || op == 0x07 ||
                            op == 0x08 || op == 0x09 || op == 0x0A || op == 0x19 || op == 0x1A ||
                            op == 0x1B || op == 0x1C || op == 0x1D || op == 0x29 || op == 0x2A);
        bool is_d_family = (op == 0x21);

        if (sf == 0x00 && (is_a_family || is_d_family)) {
            // A0/D0 register-register: use register lookup table
            mode = MODE_REG;
        } else if (is_a_family || is_d_family) {
            // A1/A2/A3 / D1B/D1W/D1DW: sf encodes immediate size
            switch (sf & 0x03) {
                case 0x01: mode = MODE_BYTE; break;
                case 0x02: mode = MODE_WORD; break;
                case 0x03: mode = MODE_DWORD; break;
                default:   mode = MODE_DWORD; break;
            }
        } else {
            mode = (sf & 0x03);
            if (mode == 3) mode = MODE_DWORD;
        }
    } else {
        mode = MODE_DWORD;
    }

    // Decode operands
    switch (op) {
        case 0x00: case 0x01: case 0x02: case 0x13: case 0x26: case 0x27: case 0x28:
            // S0 instructions: NOP, HALT, WAIT, RET, IRET, CLI, STI
            break;

        case 0x0B: case 0x0C: case 0x0D: // S3: NOT, INC, DEC
        case 0x0E: case 0x0F:            // S2/S1: PUSH, POP
        case 0x11:                       // S2: CALLR
            rd = cpu->mem[ic++]; len++;
            break;

        case 0x25: // INT S4
            rd = cpu->mem[ic++]; len++;
            break;

        case 0x10: case 0x12: // CALL, CLABS S6
            imm32 = (cpu->mem[ic] << 24) | (cpu->mem[ic+1] << 16) | (cpu->mem[ic+2] << 8) | cpu->mem[ic+3];
            ic += 4; len += 4;
            break;

        case 0x14: case 0x15: case 0x16: case 0x17: case 0x18: // H0 shifts
            rd = cpu->mem[ic++];
            imm32 = cpu->mem[ic++];
            len += 2;
            break;

        case 0x03: case 0x04: case 0x05: case 0x06: case 0x07: case 0x08:
        case 0x09: case 0x0A: case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D:
        case 0x29: case 0x2A: // A-family
            rd = cpu->mem[ic++]; len++;
            if (sf == 0x00) {
                rs = cpu->mem[ic++]; len++;
            } else {
                int imm_bytes = ((sf & 0x03) == 1) ? 1 : (((sf & 0x03) == 2) ? 2 : 4);
                imm32 = 0;
                for (int i = 0; i < imm_bytes; i++) {
                    imm32 = (imm32 << 8) | cpu->mem[ic++];
                }
                len += imm_bytes;
            }
            break;

        case 0x21: // COPY / LDI
            rd = cpu->mem[ic++]; len++;
            if (sf == 0x00) {
                rs = cpu->mem[ic++]; len++;
            } else {
                int imm_bytes = ((sf & 0x03) == 1) ? 1 : (((sf & 0x03) == 2) ? 2 : 4);
                imm32 = 0;
                for (int i = 0; i < imm_bytes; i++) {
                    imm32 = (imm32 << 8) | cpu->mem[ic++];
                }
                len += imm_bytes;
            }
            break;

        case 0x1E: case 0x20: // JMP, JMA J0
            cond = cpu->mem[ic++]; len++;
            imm32 = (cpu->mem[ic] << 24) | (cpu->mem[ic+1] << 16) | (cpu->mem[ic+2] << 8) | cpu->mem[ic+3];
            ic += 4; len += 4;
            break;

        case 0x1F: // JMPR J1
            cond = cpu->mem[ic++]; len++;
            rs = cpu->mem[ic++]; len++;
            break;

        case 0x22: case 0x23: { // STR / LOD
            rd = cpu->mem[ic++]; len++;
            // Map suffix to mem_mode 0..4 and operand size
            int mem_mode;
            int mode_size_bytes;
            if (sf < 0x05) { mem_mode = sf; mode_size_bytes = 1; }
            else if (sf < 0x0A) { mem_mode = sf - 0x05; mode_size_bytes = 2; }
            else { mem_mode = sf - 0x0A; mode_size_bytes = 4; }

            if (mode_size_bytes == 4) mode = MODE_DWORD;
            else if (mode_size_bytes == 2) mode = MODE_WORD;
            else mode = MODE_BYTE;

            uint8_t rb = 0, ra = 0;
            uint32_t addr = 0;
            uint8_t disp = 0;

            if (mem_mode == 0) { // F absolute
                addr = (cpu->mem[ic] << 24) | (cpu->mem[ic+1] << 16) | (cpu->mem[ic+2] << 8) | cpu->mem[ic+3];
                ic += 4; len += 4;
            } else if (mem_mode == 1) { // S: base:offset (two regs)
                rb = cpu->mem[ic++];
                ra = cpu->mem[ic++];
                len += 2;
                addr = (cpu_get_reg_i80148(cpu, rb) + cpu_get_reg_i80148(cpu, ra)) & 0xFFFFFFFF;
            } else if (mem_mode == 2) { // R: register indirect (one reg)
                rb = cpu->mem[ic++];
                len += 1;
                addr = cpu_get_reg_i80148(cpu, rb) & 0xFFFFFFFF;
            } else if (mem_mode == 3) { // SD: base:offset + disp
                rb = cpu->mem[ic++];
                ra = cpu->mem[ic++];
                disp = cpu->mem[ic++];
                len += 3;
                addr = (cpu_get_reg_i80148(cpu, rb) + cpu_get_reg_i80148(cpu, ra) + (int8_t)disp) & 0xFFFFFFFF;
            } else if (mem_mode == 4) { // RD: reg + disp
                rb = cpu->mem[ic++];
                disp = cpu->mem[ic++];
                len += 2;
                addr = (cpu_get_reg_i80148(cpu, rb) + (int8_t)disp) & 0xFFFFFFFF;
            }

            if (op == 0x22) { // STR
                uint64_t val = cpu_get_reg_i80148(cpu, rd);
                cpu_write_mem_i80148(cpu, addr, val, mode);
            } else { // LOD
                uint64_t val = cpu_read_mem_i80148(cpu, addr, mode);
                cpu_set_reg_i80148(cpu, rd, val);
            }
            state(cpu)->regs[REG_IC] = ic;
            return len;
        }

        default:
            printf("[WARN] Unknown opcode 0x%02X at 0x%08X\n", op, (uint32_t)state(cpu)->regs[REG_IC]);
            cpu->halted = true;
            return 0;
    }

    // Resolve REG lookup mode based on destination register
    if (mode == MODE_REG && rd < REG_COUNT) {
        mode = reg_default_mode(rd);
    }

    // Execute
    uint64_t a, b, res;
    uint8_t mask = 0;
    bool is_sub = false;

    switch (op) {
        case 0x00: // NOP
            break;
        case 0x01: // HALT
            cpu->halted = true;
            break;
        case 0x02: // WAIT
            // Halt until IRQ - for now just continue
            break;

        case 0x03: // ADD
        case 0x04: // ADC
        case 0x05: // SUB
        case 0x06: // SBB
        case 0x07: // MUL
        case 0x08: // IMUL
        case 0x09: // DIV
        case 0x0A: // IDIV
        case 0x19: // AND
        case 0x1A: // OR
        case 0x1B: // XOR
        case 0x1C: // CMP
        case 0x1D: // ICMP
        case 0x29: // REM
        case 0x2A: // IREM
            a = cpu_get_reg_i80148(cpu, rd);
            b = (sf == 0x00) ? cpu_get_reg_i80148(cpu, rs) : (imm32 & MODE_MASKS[mode]);
            res = 0;
            is_sub = false;
            mask = 0;

            switch (op) {
                case 0x03: res = a + b; mask = FLAG_Z | FLAG_O | FLAG_S | FLAG_C; break;
                case 0x04: res = a + b + ((state(cpu)->regs[REG_FL] & FLAG_C) ? 1 : 0); mask = FLAG_Z | FLAG_O | FLAG_S | FLAG_C; break;
                case 0x05: res = a - b; mask = FLAG_Z | FLAG_O | FLAG_S | FLAG_B; is_sub = true; break;
                case 0x06: res = a - b - ((state(cpu)->regs[REG_FL] & FLAG_B) ? 1 : 0); mask = FLAG_Z | FLAG_O | FLAG_S | FLAG_B; is_sub = true; break;
                case 0x07: res = a * b; mask = FLAG_Z | FLAG_O | FLAG_S; break;
                case 0x08: {
                    int64_t sa = (int64_t)(a << (64 - mode_size(mode)*8)) >> (64 - mode_size(mode)*8);
                    int64_t sb = (int64_t)(b << (64 - mode_size(mode)*8)) >> (64 - mode_size(mode)*8);
                    res = (uint64_t)(sa * sb);
                    mask = FLAG_Z | FLAG_O | FLAG_S;
                    break;
                }
                case 0x09: res = (b != 0) ? (a / b) : 0; mask = FLAG_Z | FLAG_O | FLAG_S; break;
                case 0x0A: {
                    int64_t sa = (int64_t)(a << (64 - mode_size(mode)*8)) >> (64 - mode_size(mode)*8);
                    int64_t sb = (int64_t)(b << (64 - mode_size(mode)*8)) >> (64 - mode_size(mode)*8);
                    res = (sb != 0) ? (uint64_t)(sa / sb) : 0;
                    mask = FLAG_Z | FLAG_O | FLAG_S;
                    break;
                }
                case 0x19: res = a & b; mask = FLAG_Z | FLAG_O | FLAG_S; break;
                case 0x1A: res = a | b; mask = FLAG_Z | FLAG_O | FLAG_S; break;
                case 0x1B: res = a ^ b; mask = FLAG_Z | FLAG_O | FLAG_S; break;
                case 0x1C:
                    res = a - b;
                    mask = FLAG_L | FLAG_E | FLAG_G | FLAG_Z | FLAG_O | FLAG_S | FLAG_B;
                    is_sub = true;
                    break;
                case 0x1D: {
                    int64_t sa = (int64_t)(a << (64 - mode_size(mode)*8)) >> (64 - mode_size(mode)*8);
                    int64_t sb = (int64_t)(b << (64 - mode_size(mode)*8)) >> (64 - mode_size(mode)*8);
                    res = (uint64_t)(sa - sb);
                    mask = FLAG_L | FLAG_E | FLAG_G | FLAG_Z | FLAG_O | FLAG_S | FLAG_B;
                    is_sub = true;
                    break;
                }
                case 0x29: res = (b != 0) ? a % b : 0; mask = FLAG_Z | FLAG_O | FLAG_S; break;
                case 0x2A: {
                    int64_t sa = (int64_t)(a << (64 - mode_size(mode)*8)) >> (64 - mode_size(mode)*8);
                    int64_t sb = (int64_t)(b << (64 - mode_size(mode)*8)) >> (64 - mode_size(mode)*8);
                    res = (sb != 0) ? (uint64_t)(sa % sb) : 0;
                    mask = FLAG_Z | FLAG_O | FLAG_S;
                    break;
                }
            }
            res &= MODE_MASKS[mode];
            if (op != 0x1C && op != 0x1D) {
                cpu_set_reg_i80148(cpu, rd, res);
            }
            bool is_signed = (op == 0x1D || op == 0x08 || op == 0x0A || op == 0x2A);
            update_flags(cpu, res, a, b, mask, mode, is_sub, is_signed);
            break;

        case 0x0B: // NOT
            a = cpu_get_reg_i80148(cpu, rd);
            res = (~a) & MODE_MASKS[mode];
            cpu_set_reg_i80148(cpu, rd, res);
            update_flags(cpu, res, a, 0, FLAG_Z | FLAG_O | FLAG_S, mode, false, false);
            break;
        case 0x0C: // INC
            a = cpu_get_reg_i80148(cpu, rd);
            res = (a + 1) & MODE_MASKS[mode];
            cpu_set_reg_i80148(cpu, rd, res);
            update_flags(cpu, res, a, 1, FLAG_Z | FLAG_O | FLAG_S | FLAG_C, mode, false, false);
            break;
        case 0x0D: // DEC
            a = cpu_get_reg_i80148(cpu, rd);
            res = (a - 1) & MODE_MASKS[mode];
            cpu_set_reg_i80148(cpu, rd, res);
            update_flags(cpu, res, a, 1, FLAG_Z | FLAG_O | FLAG_S | FLAG_B, mode, true, false);
            break;

        case 0x0E: // PUSH
            push(cpu, cpu_get_reg_i80148(cpu, rd));
            break;
        case 0x0F: // POP
            cpu_set_reg_i80148(cpu, rd, pop(cpu));
            break;

        case 0x10: { // CALL relative
            uint32_t ic_after = (uint32_t)state(cpu)->regs[REG_IC] + 5;
            push(cpu, ic_after);
            state(cpu)->regs[REG_IC] = (uint32_t)(ic_after + (int32_t)imm32);
            return len;
        }
        case 0x11: // CALLR
            push(cpu, (uint32_t)(state(cpu)->regs[REG_IC] + 2));
            state(cpu)->regs[REG_IC] = (uint32_t)cpu_get_reg_i80148(cpu, rd);
            return len;
        case 0x12: { // CLABS absolute
            uint32_t ic_after = (uint32_t)state(cpu)->regs[REG_IC] + 5;
            push(cpu, ic_after);
            state(cpu)->regs[REG_IC] = imm32;
            return len;
        }
        case 0x13: // RET
            state(cpu)->regs[REG_IC] = (uint32_t)pop(cpu);
            return len;

        case 0x14: // LSL
        case 0x15: // LSR
        case 0x16: // ASR
        case 0x17: // ROL
        case 0x18: { // ROR
            a = cpu_get_reg_i80148(cpu, rd);
            int shift = imm32 & 0x3F;
            res = 0;
            int bits = mode_size(mode) * 8;
            uint64_t mask_val = MODE_MASKS[mode];
            switch (op) {
                case 0x14: res = (a << shift) & mask_val; break;
                case 0x15: res = (a >> shift) & mask_val; break;
                case 0x16: {
                    int64_t sa = (int64_t)(a << (64 - bits)) >> (64 - bits);
                    res = ((uint64_t)(sa >> shift)) & mask_val;
                    break;
                }
                case 0x17: res = ((a << shift) | (a >> ((bits - shift) % bits))) & mask_val; break;
                case 0x18: res = ((a >> shift) | (a << ((bits - shift) % bits))) & mask_val; break;
            }
            cpu_set_reg_i80148(cpu, rd, res);
            update_flags(cpu, res, a, shift, FLAG_Z | FLAG_O | FLAG_S, mode, false, false);
            break;
        }

        case 0x21: // COPY / LDI
            if (sf == 0x00) {
                cpu_set_reg_i80148(cpu, rd, cpu_get_reg_i80148(cpu, rs));
            } else {
                cpu_set_reg_i80148(cpu, rd, imm32 & MODE_MASKS[mode]);
            }
            break;

        case 0x1E: // JMP relative
            if (cpu_check_cond_i80148(cpu, cond)) {
                state(cpu)->regs[REG_IC] = (uint32_t)((int32_t)(state(cpu)->regs[REG_IC] + 6) + (int32_t)imm32);
                return len;
            }
            break;
        case 0x1F: // JMPR
            if (cpu_check_cond_i80148(cpu, cond)) {
                state(cpu)->regs[REG_IC] = (uint32_t)cpu_get_reg_i80148(cpu, rs);
                return len;
            }
            break;
        case 0x20: // JMA absolute
            if (cpu_check_cond_i80148(cpu, cond)) {
                state(cpu)->regs[REG_IC] = imm32;
                return len;
            }
            break;

        case 0x25: { // INT
            cpu_trigger_int(cpu, rd);
            return len;
        }
        case 0x26: // IRET
            state(cpu)->regs[REG_FL] = pop(cpu);
            state(cpu)->regs[REG_IC] = (uint32_t)pop(cpu);
            cpu->irq_enabled = true;
            return len;
        case 0x27: // CLI
            cpu->irq_enabled = false;
            break;
        case 0x28: // STI
            cpu->irq_enabled = true;
            break;
    }

    state(cpu)->regs[REG_IC] = ic;
    return len;
}
// Backend vtable and registration data

static const char* const g_i80148_register_names[REG_COUNT_I80148] = {
    "R0",  "EX1", "EX2", "EX3", "EX4", "EX5", "EX6", "EX7",
    "X1",  "X2",  "X3",  "X4",  "X5",  "X6",  "X7",
    "XL1", "XL2", "XL3", "XL4", "XL5", "XL6", "XL7",
    "IC",  "FL",  "SP",  "BP",  "IX",  "IY",
    "A0",  "A1",  "A2",  "A3",  "A4",  "A5",  "A6",  "A7"
};

const CpuBackend g_cpu_backend_i80148 = {
    .name               = "i80148",
    .description        = "i80148 processor",
    .state_size         = sizeof(i80148_State),
    .init               = cpu_init_i80148,
    .reset              = cpu_reset_i80148,
    .step               = cpu_step_i80148,
    .load_file          = cpu_load_file_i80148,
    .get_reg            = cpu_get_reg_i80148,
    .set_reg            = cpu_set_reg_i80148,
    .read_mem           = cpu_read_mem_i80148,
    .write_mem          = cpu_write_mem_i80148,
    .check_cond         = cpu_check_cond_i80148,
    .feed_key           = NULL,  // handled by common cpu_feed_key
    .register_count     = REG_COUNT_I80148,
    .register_names     = g_i80148_register_names,
    .render_state       = i80148_render_state,
    .render_memory_buttons = NULL,
    .pc_register        = REG_IC,
    .fl_register        = REG_FL,
    .sp_register        = REG_SP,
    .mem_size_default   = 16 * 1024 * 1024,
    .vbuffer_base       = VBUFFER_BASE_I80148,
    .vbuffer_cols       = TERM_COLS_I80148,
    .vbuffer_rows       = TERM_ROWS_I80148,
    .kbd_ascii_addr     = KBD_ASCII_ADDR_I80148,
    .user_ram_start     = USER_RAM_START_I80148,
};
