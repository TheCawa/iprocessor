#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define REG_COUNT   32
#define MAX_IV_SIZE 256

// ==================== REGISTER MAP ====================
#define REG_R0   0x00
#define REG_X1   0x01
#define REG_XL1  0x02
#define REG_XH1  0x03
#define REG_X2   0x04
#define REG_XL2  0x05
#define REG_XH2  0x06
#define REG_X3   0x07
#define REG_XL3  0x08
#define REG_XH3  0x09
#define REG_X4   0x0A
#define REG_XL4  0x0B
#define REG_XH4  0x0C
#define REG_X5   0x0D
#define REG_XL5  0x0E
#define REG_XH5  0x0F
#define REG_IX   0x10
#define REG_IY   0x11
#define REG_SP   0x12
#define REG_BP   0x13
#define REG_CS   0x14
#define REG_DS   0x15
#define REG_SS   0x16
#define REG_ES   0x17
#define REG_SCS  0x18
#define REG_SDS  0x19
#define REG_SSS  0x1A
#define REG_SES  0x1B
#define REG_A0   0x1C
#define REG_A1   0x1D
#define REG_FL   0x1E
#define REG_IC   0x1F

// ==================== MODES ====================
typedef enum {
    MODE_BYTE  = 0x00,
    MODE_WORD  = 0x01,
    MODE_ADDR  = 0x02,
    MODE_DWORD = 0x03,
    MODE_REG   = 0x04,
    MODE_QWORD = 0x05
} CpuMode;

extern const uint64_t MODE_MASKS[];

// ==================== FLAGS ====================
#define FLAG_C 0x01
#define FLAG_B 0x02
#define FLAG_S 0x04
#define FLAG_O 0x08
#define FLAG_Z 0x10
#define FLAG_G 0x20
#define FLAG_E 0x40
#define FLAG_L 0x80

// Mask mappings from spec (******** -> bit pattern)
#define MASK_NONE   0x00
#define MASK_ZOS_C  0x17 // ***ZOS*C
#define MASK_ZOS_B  0x16 // ***ZOSB*
#define MASK_ZOS_   0x14 // ***ZOS**
#define MASK_LEG    0xE0 // LEG*****
#define MASK_ALL    0xFF

// Condition codes for JMP
typedef enum {
    COND_UNC = 0x00,
    COND_CF  = 0x01,
    COND_BF  = 0x02,
    COND_SF  = 0x03,
    COND_OF  = 0x04,
    COND_ZF  = 0x05,
    COND_NZ  = 0x06,
    COND_GR  = 0x07,
    COND_GE  = 0x08,
    COND_LS  = 0x09,
    COND_LE  = 0x0A,
    COND_EQ  = 0x0B,
    COND_NE  = 0x0C
} CondCode;

// ==================== CPU STATE ====================
typedef struct {
    uint64_t regs[REG_COUNT];
    uint8_t  *mem;
    size_t   mem_size;
    uint8_t  iv[MAX_IV_SIZE][3]; // Interrupt Vector (24-bit addresses)
    uint32_t mmio_base;  // Начало MMIO-диапазона
    uint32_t mmio_size;  // Размер MMIO-диапазона
    bool     is_mmio_access;
    bool     halted;
    bool     irq_enabled;
    bool     imem_locked;
    bool     reg_locked[REG_COUNT];
} Cpu;

// ==================== API ====================

#ifdef __cplusplus
extern "C" {
#endif

void cpu_init(Cpu *cpu, uint8_t *mem, size_t size);
void cpu_reset(Cpu *cpu);
int  cpu_step(Cpu *cpu);
void cpu_run(Cpu *cpu);
void cpu_interrupt(Cpu *cpu, uint8_t int_id);

// Internal helpers (exposed for debugging/testing)
uint64_t cpu_get_reg(const Cpu *cpu, uint8_t idx);
void     cpu_set_reg(Cpu *cpu, uint8_t idx, uint64_t val);
uint64_t cpu_read_mem(const Cpu *cpu, uint32_t addr, CpuMode mode);
void     cpu_write_mem(Cpu *cpu, uint32_t addr, uint64_t val, CpuMode mode);

int cpu_load_file(Cpu* cpu, const char* filename, uint32_t load_addr);

#ifdef __cplusplus
}
#endif

#endif // CPU_H