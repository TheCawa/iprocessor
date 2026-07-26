// ------------------------------------------------------------------------------
//          cpu_i8046.h - i8046 CPU backend header
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

#ifndef CPU_I8046_H
#define CPU_I8046_H

#include "cpu_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define REG_COUNT_I8046   32
#define MAX_IV_SIZE       256

// Register map for i8046
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

// Modes
typedef enum {
    MODE_BYTE  = 0x00,
    MODE_WORD  = 0x01,
    MODE_ADDR  = 0x02,
    MODE_DWORD = 0x03,
    MODE_REG   = 0x04,
    MODE_QWORD = 0x05
} CpuMode_i8046;

typedef CpuMode_i8046 CpuMode;
#define REG_COUNT REG_COUNT_I8046

// Flags
#define FLAG_C 0x01
#define FLAG_B 0x02
#define FLAG_S 0x04
#define FLAG_O 0x08
#define FLAG_Z 0x10
#define FLAG_G 0x20
#define FLAG_E 0x40
#define FLAG_L 0x80

// Condition codes
#define COND_UNC_I8046 0x00
#define COND_CF_I8046  0x01
#define COND_BF_I8046  0x02
#define COND_SF_I8046  0x03
#define COND_OF_I8046  0x04
#define COND_ZF_I8046  0x05
#define COND_NZ_I8046  0x06
#define COND_GR_I8046  0x07
#define COND_GE_I8046  0x08
#define COND_LS_I8046  0x09
#define COND_LE_I8046  0x0A
#define COND_EQ_I8046  0x0B
#define COND_NE_I8046  0x0C

// Mask mappings
#define MASK_NONE_I8046   0x00
#define MASK_ZOS_C_I8046  0x17
#define MASK_ZOS_B_I8046  0x16
#define MASK_ZOS__I8046   0x14
#define MASK_LEG_I8046    0xE0
#define MASK_ALL_I8046    0xFF

// Video parameters for i8046
typedef struct {
    uint64_t regs[REG_COUNT_I8046];
    uint8_t  iv[MAX_IV_SIZE][3];
    uint32_t mmio_base;
    uint32_t mmio_size;
    bool     is_mmio_access;
    bool     imem_locked;
    bool     reg_locked[REG_COUNT_I8046];
} i8046_State;

#define VBUFFER_BASE_I8046   0x010000
#define VBUFFER_SIZE_I8046   0x10000
#define VBUFFER_COLS_I8046   80
#define VBUFFER_ROWS_I8046   25

extern const CpuBackend g_cpu_backend_i8046;

#ifdef __cplusplus
}
#endif

#endif // CPU_I8046_H
