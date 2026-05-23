#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <ctype.h>
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
const uint64_t MODE_MASKS[] = { 0xFF, 0xFFFF, 0xFFFFFF, 0xFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF };

// ==================== REGISTER ACCESS ====================
uint64_t cpu_get_reg(const Cpu *cpu, uint8_t idx) {
    if (idx > 0x0F) return cpu->regs[idx] & 0xFFFFFFFFFFFFFFFF;
    if (idx >= 0x01 && idx <= 0x0D && (idx - 1) % 3 == 0) {
        uint8_t xl = idx + 1;
        uint8_t xh = idx + 2;
        return ((cpu->regs[xh] & 0xFF) << 8) | (cpu->regs[xl] & 0xFF);
    }
    return cpu->regs[idx] & 0xFFFFFFFFFFFFFFFF;
}

void cpu_set_reg(Cpu *cpu, uint8_t idx, uint64_t val) {
    if (cpu->reg_locked[idx] && idx != REG_FL && idx != REG_IC) return;
    
    val &= MODE_MASKS[MODE_QWORD];
    if (idx >= 0x01 && idx <= 0x0D && (idx - 1) % 3 == 0) {
        cpu->regs[idx + 1] = (val & 0xFF);
        cpu->regs[idx + 2] = (val >> 8) & 0xFF;
    } else {
        cpu->regs[idx] = val;
    }
}

#include <ctype.h>  // Для isxdigit()

// ==================== FILE LOADING ====================

int cpu_load_bin(Cpu* cpu, const char* filename, uint32_t load_addr) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        printf("[ERR] Cannot open file: %s\n", filename);
        return -1;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (load_addr + fsize > cpu->mem_size) {
        printf("[ERR] File too large: %ld bytes at 0x%06X (mem_size=%zu)\n", 
               fsize, load_addr, cpu->mem_size);
        fclose(f);
        return -1;
    }

    size_t read = fread(&cpu->mem[load_addr], 1, fsize, f);
    fclose(f);

    if (read != (size_t)fsize) {
        printf("[WARN] Read only %zu of %ld bytes\n", read, fsize);
        return -1;
    }

    printf("[INFO] Loaded %ld bytes from '%s' to 0x%06X\n", fsize, filename, load_addr);
    return 0;
}

// Вспомогательная: парсинг двух шестнадцатеричных символов в байт
static int hex_pair_to_byte(const char* hex) {
    int val = 0;
    if (sscanf(hex, "%2x", &val) != 1) return -1;
    return val;
}

// Вспомогательная: вычисление контрольной суммы для Intel HEX
static uint8_t hex_checksum(const uint8_t* data, size_t len) {
    uint8_t sum = 0;
    for (size_t i = 0; i < len; i++) sum += data[i];
    return (~sum + 1) & 0xFF;
}

int cpu_load_hex(Cpu* cpu, const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        printf("[ERR] Cannot open file: %s\n", filename);
        return -1;
    }

    char line[1024];
    int total_bytes = 0;
    uint32_t addr = 0;
    int is_logisim_raw = 0;
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        printf("[ERR] Empty file: %s\n", filename);
        return -1;
    }
    if (strncmp(line, "v2.0", 4) == 0) {
        is_logisim_raw = 1;
        printf("[INFO] Detected Logisim RAW format\n");
    } else {
        fseek(f, 0, SEEK_SET);
    }

    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '#' || line[0] == '\r') continue;
        line[strcspn(line, "\r\n")] = '\0';
        char* ptr = line;
        while (*ptr) {
            while (*ptr == ' ' || *ptr == '\t' || *ptr == ',') ptr++;
            if (!*ptr) break;
            unsigned int byte_val;
            int consumed = 0;
            if (sscanf(ptr, "%2x%n", &byte_val, &consumed) == 1 && consumed >= 2) {
                if (addr < cpu->mem_size) {
                    cpu->mem[addr++] = (uint8_t)byte_val;
                    total_bytes++;
                }
                ptr += consumed;
            } else {
                while (*ptr && *ptr != ' ' && *ptr != '\t' && *ptr != ',') ptr++;
            }
        }
    }

    fclose(f);
    printf("[INFO] Loaded %d bytes from '%s' (format: %s)\n", 
           total_bytes, filename, is_logisim_raw ? "Logisim RAW" : "Simple HEX");
    return 0;
}

int cpu_load_file(Cpu* cpu, const char* filename, uint32_t load_addr) {
    const char* ext = strrchr(filename, '.');
    if (ext && (strcasecmp(ext, ".hex") == 0 || strcasecmp(ext, ".HEX") == 0)) {
        return cpu_load_hex(cpu, filename);
    } else {
        return cpu_load_bin(cpu, filename, load_addr);
    }
}

// ==================== MEMORY ACCESS ====================
static uint32_t calc_addr(const Cpu *cpu, uint8_t base_reg, uint32_t offset) {
    return ((cpu_get_reg(cpu, base_reg) << 8) + offset) & 0xFFFFFF;
}

static uint32_t align_address(uint32_t addr, CpuMode mode) {
    if (mode == MODE_BYTE) return addr;
    if (mode == MODE_WORD) return addr & ~1U;
    return addr;
}

uint64_t cpu_read_mem(const Cpu *cpu, uint32_t addr, CpuMode mode) {
    if (addr >= cpu->mem_size) return 0;
    uint32_t aligned_addr = align_address(addr, mode);
    uint64_t val = 0;
    size_t bytes = (mode == MODE_ADDR) ? 3 : (1 << mode);
    if (bytes > 8) bytes = 8;
    
    // Big-endian: первый байт в памяти — самый старший
    for (size_t i = 0; i < bytes; i++) {
        val |= ((uint64_t)cpu->mem[aligned_addr + i]) << ((bytes - 1 - i) * 8);
    }
    return val & MODE_MASKS[mode];
}

void cpu_write_mem(Cpu *cpu, uint32_t addr, uint64_t val, CpuMode mode) {
    if (addr >= cpu->mem_size) return;
    if (addr >= VBUFFER_BASE && addr < VBUFFER_BASE + VBUFFER_SIZE) {
        cpu->screen_dirty = true;
    }
    size_t bytes = (mode == MODE_ADDR) ? 3 : (1 << mode);
    if (bytes > 8) bytes = 8;
    uint32_t aligned_addr = align_address(addr, mode);
    for (size_t i = 0; i < bytes; i++) {
        if (aligned_addr + i >= cpu->mem_size) continue;
        cpu->mem[aligned_addr + i] = (val >> ((bytes - 1 - i) * 8)) & 0xFF;
    }
}

// ==================== FLAG & CONDITION LOGIC ====================
static void update_flags(Cpu *cpu, uint64_t res, uint64_t a, uint64_t b, uint8_t mask, bool is_sub) {
    uint64_t *fl = &cpu->regs[REG_FL];
    
    if (mask & FLAG_Z) *fl = (*fl & ~FLAG_Z) | ((res == 0) ? FLAG_Z : 0);
    if (mask & FLAG_S) *fl = (*fl & ~FLAG_S) | ((res & 0x80) ? FLAG_S : 0);
    
    if (is_sub) {
        if (mask & FLAG_C) *fl = (*fl & ~FLAG_C) | (a < b ? FLAG_C : 0);
        if (mask & FLAG_B) *fl = (*fl & ~FLAG_B) | (a < b ? FLAG_B : 0);
    } else {
        if (mask & FLAG_C) *fl = (*fl & ~FLAG_C) | (((a + b) >> 32) ? FLAG_C : 0);
    }
    if (mask & FLAG_O) *fl &= ~FLAG_O;
    
    if (mask & FLAG_G) *fl = (*fl & ~FLAG_G) | (a > b ? FLAG_G : 0);
    if (mask & FLAG_E) *fl = (*fl & ~FLAG_E) | (a == b ? FLAG_E : 0);
    if (mask & FLAG_L) *fl = (*fl & ~FLAG_L) | (a < b ? FLAG_L : 0);
}

static bool check_cond(const Cpu *cpu, uint8_t cond) {
    uint64_t fl_val = cpu->regs[REG_FL] & 0xFF;
    switch (cond) {
        case COND_UNC: return true;
        case COND_CF:  return (fl_val & FLAG_C) != 0;
        case COND_BF:  return (fl_val & FLAG_B) != 0;
        case COND_SF:  return (fl_val & FLAG_S) != 0;
        case COND_OF:  return (fl_val & FLAG_O) != 0;
        case COND_ZF:  return (fl_val & FLAG_Z) != 0;
        case COND_NZ:  return (fl_val & FLAG_Z) == 0;
        case COND_GR:  return (fl_val & FLAG_G) != 0;
        case COND_GE:  return (fl_val & FLAG_G) || (fl_val & FLAG_E);
        case COND_LS:  return (fl_val & FLAG_L) != 0;
        case COND_LE:  return (fl_val & FLAG_L) || (fl_val & FLAG_E);
        case COND_EQ:  return (fl_val & FLAG_E) != 0;
        case COND_NE:  return (fl_val & FLAG_E) == 0;
        default: return false;
    }
}

// ==================== ALU/BSU DISPATCHER ====================
static uint64_t alu_execute(Cpu *cpu, uint8_t op, uint64_t a, uint64_t b, uint8_t mode) {
    uint64_t res = 0;
    uint8_t mask = (op >= 0x08) ? MASK_ZOS_ : MASK_ALL;
    
    switch (op) {
        case 0x00: res = a + b; break;
        case 0x01: res = a - b; break;
        case 0x02: res = a * b; break;
        case 0x03: res = (b != 0) ? a / b : 0; break;
        case 0x04: res = a & b; break;
        case 0x05: res = a | b; break;
        case 0x06: res = a ^ b; break;
        case 0x07: res = ~a; break;
        case 0x08: res = a << (b & 0x3F); break;
        case 0x09: res = a >> (b & 0x3F); break;
        case 0x0A: res = (int64_t)a >> (b & 0x3F); break;
        case 0x0B: res = (a << (b & 0x3F)) | (a >> ((64 - (b & 0x3F)) & 0x3F)); break;
        case 0x0C: res = (a >> (b & 0x3F)) | (a << ((64 - (b & 0x3F)) & 0x3F)); break;
        default: return a;
    }
    
    update_flags(cpu, res, a, b, mask, op == 0x01 || op == 0x0A);
    return res & MODE_MASKS[mode];
}

// ==================== INSTRUCTION DECODE & EXECUTE ====================
int cpu_step(Cpu *cpu) {
    if (cpu->halted) return -1;
    
    uint32_t ic = (uint32_t)cpu->regs[REG_IC];
    if (ic >= cpu->mem_size) { cpu->halted = true; return 0; }

    uint8_t op  = cpu->mem[ic++];
    uint8_t sf  = cpu->mem[ic++];
    
    uint8_t  rd = 0, rs = 0, rb = 0, ra = 0, cond = 0;
    uint64_t imm8 = 0, imm16 = 0, imm24 = 0, addr24 = 0;
    int len = 2;

    switch (op) {
        case 0x00: // NOP
        case 0x01: // HALT
            break;

        // A-family: ADD, SUB, CMP и т.д.
        case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
        case 0x08: case 0x09: case 0x0A: case 0x0B: case 0x0C:
        case 0x10: case 0x11:
            rd = cpu->mem[ic++]; 
            if (sf == 0x00) {
                rs = cpu->mem[ic++];
                len += 2;
            } else {
                len += (sf & 0x03) + 1;
            }
            break;

        // S3: NOT, INC, DEC
        case 0x0D: case 0x0E: case 0x0F:
            rd = cpu->mem[ic++]; len += 1; break;

        // J0: JMP
        case 0x12: {
            cond = cpu->mem[ic++];
            uint8_t b0 = cpu->mem[ic++];
            uint8_t b1 = cpu->mem[ic++];
            uint8_t b2 = cpu->mem[ic++];
            imm24 = b0 | (b1 << 8) | (b2 << 16);
            len += 4;
            break;
        }

        case 0x13: // PUSH
        case 0x14: // POP
            rd = cpu->mem[ic++];
            len += 1;
            break;

        // CALL - S6
        case 0x15: {
            uint8_t b0 = cpu->mem[ic++];
            uint8_t b1 = cpu->mem[ic++];
            uint8_t b2 = cpu->mem[ic++];
            imm24 = b0 | (b1 << 8) | (b2 << 16);
            len += 3;
            break;
        }

        // S0: RET, CLI, STI, etc.
        case 0x16: case 0x1F: case 0x20: case 0x21: case 0x22: case 0x24:
            break;

        // MOV/LDI family
        case 0x17:
            rd = cpu->mem[ic++]; len += 1; 
            if (sf == 0x01) { imm8 = cpu->mem[ic++]; len++; }
            else if (sf == 0x02) {
                uint8_t b0 = cpu->mem[ic++];
                uint8_t b1 = cpu->mem[ic++];
                imm16 = b0 | (b1 << 8);
                len += 2;
            }
            else if (sf == 0x03) {
                uint8_t b0 = cpu->mem[ic++];
                uint8_t b1 = cpu->mem[ic++];
                uint8_t b2 = cpu->mem[ic++];
                imm24 = b0 | (b1 << 8) | (b2 << 16);
                len += 3;
            }
            else { rs = cpu->mem[ic++]; len++; }
            break;

        // STR/LOD family
        case 0x18: case 0x19:
            rd = cpu->mem[ic++]; len++;
            if (sf & 0x0C) {  // flat address
                uint8_t b0 = cpu->mem[ic++];
                uint8_t b1 = cpu->mem[ic++];
                uint8_t b2 = cpu->mem[ic++];
                addr24 = b0 | (b1 << 8) | (b2 << 16);
                len += 3;
            }
            else if (sf & 0x02) {  // adreg
                ra = cpu->mem[ic++]; len++;
            }
            else if (sf & 0x01) {  // base:offset
                rb = cpu->mem[ic++]; 
                ra = cpu->mem[ic++]; 
                len += 2;
            }
            break;

        // Shift/Rotate
        case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1E:
            rd = cpu->mem[ic++]; 
            imm8 = cpu->mem[ic++];
            len += 2;
            break;

        // INT
        case 0x23:
            imm8 = cpu->mem[ic++]; len++; break;

        // WRINT
        case 0x25: {
            imm8 = cpu->mem[ic++];
            uint8_t b0 = cpu->mem[ic++];
            uint8_t b1 = cpu->mem[ic++];
            uint8_t b2 = cpu->mem[ic++];
            imm24 = b0 | (b1 << 8) | (b2 << 16);
            len += 5;
            break;
        }

        // LOCK/UNLOCK
        case 0x26: case 0x27:
            rd = cpu->mem[ic++]; len++; break;

        default:
            printf("[WARN] Unknown Opcode: 0x%02X\n", op);
            break;
    }

    uint8_t mode = MODE_BYTE;
    if (sf >= 0x40 && sf < 0x80) mode = MODE_WORD;
    else if (sf >= 0x80) mode = MODE_ADDR;
    
    // ===== Маппинг ALU =====
    uint8_t alu_op;
    switch (op) {
        case 0x02: alu_op = 0x00; break; // ADD
        case 0x04: alu_op = 0x01; break; // SUB
        case 0x0A: alu_op = 0x04; break; // AND
        case 0x0B: alu_op = 0x05; break; // OR
        case 0x0D: alu_op = 0x07; break; // NOT
        case 0x0E: alu_op = 0x00; break; // INC (ADD)
        case 0x0F: alu_op = 0x01; break; // DEC (SUB)
        case 0x10: alu_op = 0x01; break; // CMP (SUB)
        case 0x1A: alu_op = 0x08; break; // LSL
        case 0x1E: alu_op = 0x0C; break; // ROR
        default:   alu_op = 0x00; break;
    }

    switch (op) {
        case 0x00: break;
        case 0x01: cpu->halted = true; break;
        case 0x1F: cpu->irq_enabled = false; break;
        case 0x20: cpu->irq_enabled = true; break;
        case 0x21: cpu->imem_locked = true; break;
        case 0x22: cpu->imem_locked = false; break;
        case 0x24: { // IRET
            cpu->regs[REG_SP] -= 1;
            uint8_t ret_fl = cpu->mem[(size_t)cpu->regs[REG_SP]];
            cpu->regs[REG_SP] -= 3;
            uint32_t ret_addr = (uint32_t)cpu_read_mem(cpu, (uint32_t)cpu->regs[REG_SP], MODE_ADDR);
            cpu->regs[REG_FL] = ret_fl;
            cpu->regs[REG_IC] = ret_addr;
            return len;
        }
        case 0x26: cpu->reg_locked[rd] = true; break;
        case 0x27: cpu->reg_locked[rd] = false; break;
        case 0x15: { // CALL
            cpu->regs[REG_SP] -= 3;
            cpu_write_mem(cpu, (uint32_t)cpu->regs[REG_SP], cpu->regs[REG_IC] + len, MODE_ADDR);
            cpu->regs[REG_IC] = (uint32_t)imm24;
            return len;
        }
        case 0x16: { // RET
            uint32_t ret_addr = (uint32_t)cpu_read_mem(cpu, (uint32_t)cpu->regs[REG_SP], MODE_ADDR);
            cpu->regs[REG_SP] += 3;
            cpu->regs[REG_IC] = ret_addr;
            return len;
        }
        case 0x13: { // PUSH
            size_t bytes = (mode == MODE_ADDR) ? 3 : (1 << mode);
            if (bytes > 8) bytes = 8;
            cpu->regs[REG_SP] -= bytes;
            cpu_write_mem(cpu, (uint32_t)cpu->regs[REG_SP], cpu_get_reg(cpu, rd), mode);
            break;
        }
        case 0x14: { // POP
            size_t bytes = (mode == MODE_ADDR) ? 3 : (1 << mode);
            if (bytes > 8) bytes = 8;
            uint64_t val = cpu_read_mem(cpu, (uint32_t)cpu->regs[REG_SP], mode);
            cpu_set_reg(cpu, rd, val);
            cpu->regs[REG_SP] += bytes;
            break;
        }
        case 0x12:
            if (check_cond(cpu, cond)) cpu->regs[REG_IC] = imm24;
            else cpu->regs[REG_IC] += len;
            return len;
        case 0x23:
            if (cpu->irq_enabled) cpu_interrupt(cpu, imm8);
            cpu->regs[REG_IC] += len;
            return len;
        case 0x25: {
            if (imm8 < MAX_IV_SIZE) {
                cpu->iv[imm8][0] = (uint8_t)(imm24 & 0xFF);
                cpu->iv[imm8][1] = (uint8_t)((imm24 >> 8) & 0xFF);
                cpu->iv[imm8][2] = (uint8_t)((imm24 >> 16) & 0xFF);
            }
            break;
        }
        default: {
            uint64_t src = 0;
            bool is_alu_op = false;
            if (op == 0x17) {  // MOV/LDI
                if (sf == 0x00) src = cpu_get_reg(cpu, rs);
                else if (sf == 0x01) src = imm8;
                else if (sf == 0x02) src = imm16;
                else if (sf == 0x03) src = imm24;
            }
            else if (op == 0x18) {  // STR
                src = cpu_get_reg(cpu, rd);
            }
            else if (op >= 0x02 && op <= 0x11) {  // A-family
                is_alu_op = true;
                if (sf == 0x00) src = cpu_get_reg(cpu, rs);
                else if (sf == 0x01) src = imm8;
                else if (sf == 0x02) src = imm16;
                else if (sf == 0x03) src = imm24;
            }
            if (op == 0x0E || op == 0x0F) src = 1;
            if (op == 0x18) {  // STR
                uint32_t addr = (sf & 0x0C) ? (uint32_t)addr24 : 
                            ((sf & 0x01) ? calc_addr(cpu, rb, (uint32_t)cpu_get_reg(cpu, ra)) : 
                            ((sf & 0x02) ? (uint32_t)cpu_get_reg(cpu, ra) : (uint32_t)addr24));
                cpu_write_mem(cpu, addr, src, mode);
            }
            else if (op == 0x19) {  // LOD
                uint32_t addr = (sf & 0x0C) ? (uint32_t)addr24 : 
                            ((sf & 0x01) ? calc_addr(cpu, rb, (uint32_t)cpu_get_reg(cpu, ra)) : 
                            ((sf & 0x02) ? (uint32_t)cpu_get_reg(cpu, ra) : (uint32_t)addr24));
                uint64_t val = cpu_read_mem(cpu, addr, mode);
                cpu_set_reg(cpu, rd, val);
                update_flags(cpu, val, 0, 0, MASK_NONE, false);
            }
            else if (op == 0x17) {  // MOV/LDI
                cpu_set_reg(cpu, rd, src);
            }
            else if (is_alu_op) {
                uint8_t dest_reg = rd;
                uint64_t dest = cpu_get_reg(cpu, dest_reg);
                if (op == 0x03) src |= (cpu->regs[REG_FL] & FLAG_C ? 1 : 0);
                if (op == 0x05) src |= (cpu->regs[REG_FL] & FLAG_B ? 1 : 0);
                uint64_t res = alu_execute(cpu, alu_op, dest, src, mode);
                if (op != 0x10 && op != 0x11) {
                    cpu_set_reg(cpu, dest_reg, res);
                }
            }
            else if (op >= 0x1A && op <= 0x1E) {  // Сдвиги
                uint64_t dest = cpu_get_reg(cpu, rd);
                uint64_t res = alu_execute(cpu, alu_op, dest, imm8, mode);
                cpu_set_reg(cpu, rd, res);
            }
            
            cpu->regs[REG_IC] += len;
            return len;
        }
    }
    
    cpu->regs[REG_IC] += len;
    return len;
}

void cpu_run(Cpu *cpu) {
    while (!cpu->halted) cpu_step(cpu);
}

void cpu_interrupt(Cpu *cpu, uint8_t int_id) {
    if (!cpu->irq_enabled) return;
    cpu->regs[REG_SP] -= 1;
    cpu->mem[(size_t)cpu->regs[REG_SP]] = (uint8_t)(cpu->regs[REG_FL] & 0xFF);
    cpu->regs[REG_SP] -= 3;
    cpu_write_mem(cpu, (uint32_t)cpu->regs[REG_SP], cpu->regs[REG_IC], MODE_ADDR);
    
    cpu->regs[REG_IC] = (cpu->iv[int_id][0]) | (cpu->iv[int_id][1] << 8) | (cpu->iv[int_id][2] << 16);
}

void cpu_init(Cpu *cpu, uint8_t *mem, size_t size) {
    memset(cpu, 0, sizeof(Cpu));
    cpu->mem = mem;
    cpu->mem_size = size;
    cpu->irq_enabled = true;
    cpu->mmio_base = (uint32_t)(size - 256);
    cpu->mmio_size = 256;
    cpu->screen_dirty = false;
    cpu_reset(cpu);
}

void cpu_reset(Cpu *cpu) {
    memset(cpu->regs, 0, sizeof(cpu->regs));
    memset(cpu->iv, 0, sizeof(cpu->iv));
    cpu->halted = false;
    cpu->regs[REG_FL] = 0;
    cpu->regs[REG_IC] = 0;
    cpu->regs[REG_SP] = cpu->mem_size - 1;
}