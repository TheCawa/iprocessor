// ------------------------------------------------------------------------------
//          cpu_i80148.h - i80148 CPU backend header
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

#ifndef CPU_I80148_H
#define CPU_I80148_H

#include "cpu_api.h"

#ifdef __cplusplus
extern "C" {
#endif

// Register map for i80148
#define REG_COUNT_I80148   37
#define REG_R0      0x00
#define REG_EX1     0x01
#define REG_EX2     0x02
#define REG_EX3     0x03
#define REG_EX4     0x04
#define REG_EX5     0x05
#define REG_EX6     0x06
#define REG_EX7     0x07
#define REG_X1      0x08
#define REG_X2      0x09
#define REG_X3      0x0A
#define REG_X4      0x0B
#define REG_X5      0x0C
#define REG_X6      0x0D
#define REG_X7      0x0E
#define REG_XL1     0x0F
#define REG_XL2     0x10
#define REG_XL3     0x11
#define REG_XL4     0x12
#define REG_XL5     0x13
#define REG_XL6     0x14
#define REG_XL7     0x15
#define REG_IC      0x16
#define REG_FL      0x17
#define REG_SP      0x18
#define REG_BP      0x19
#define REG_IX      0x1A
#define REG_IY      0x1B
#define REG_A0      0x1C
#define REG_A1      0x1D
#define REG_A2      0x1E
#define REG_A3      0x1F
#define REG_A4      0x20
#define REG_A5      0x21
#define REG_A6      0x22
#define REG_A7      0x23
#define REG_IDTR    0x24

// Modes
typedef enum {
    MODE_BYTE  = 0x00,
    MODE_WORD  = 0x01,
    MODE_DWORD = 0x02,
    MODE_REG   = 0x04
} CpuMode_i80148;

typedef CpuMode_i80148 CpuMode;
#define REG_COUNT REG_COUNT_I80148

// Flags
#define FLAG_C  0x01
#define FLAG_B  0x02
#define FLAG_S  0x04
#define FLAG_O  0x08
#define FLAG_Z  0x10
#define FLAG_G  0x20
#define FLAG_E  0x40
#define FLAG_L  0x80

// Conditions
#define COND_UNC 0x00
#define COND_CF  0x01
#define COND_BF  0x02
#define COND_SF  0x03
#define COND_OF  0x04
#define COND_ZF  0x05
#define COND_NZ  0x06
#define COND_GR  0x07
#define COND_GE  0x08
#define COND_LS  0x09
#define COND_LE  0x0A
#define COND_EQ  0x0B
#define COND_NE  0x0C

// i80148-specific memory map constants
#define TERM_OUT_ADDR_I80148    0x00020018
#define TERM_RESET_ADDR_I80148  0x00020019
#define VC_MODE_ADDR_I80148     0x0002001A  // default video card mode select
#define KBD_ASCII_ADDR_I80148   0x0002000B
#define MEM_SIZE_ADDR_I80148    0x0002000C
#define VBUFFER_BASE_I80148     0x00100000
#define TERM_COLS_I80148        80
#define TERM_ROWS_I80148        25

// i80148 backend state stored in cpu->backend_data
typedef struct {
    uint64_t regs[REG_COUNT_I80148];
} i80148_State;

// Backend instance exported from cpu_i80148.c
extern const CpuBackend g_cpu_backend_i80148;

#ifdef __cplusplus
}
#endif

#endif // CPU_I80148_H
