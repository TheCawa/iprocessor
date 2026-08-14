// ------------------------------------------------------------------------------
//          fpu.h - Floating-point unit header
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

#ifndef FPU_H
#define FPU_H

#include "cpu_api.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// PC48 FPU MMIO register addresses (IEEE 754 float32).
#define FPU_ADDR_BASE    0x00020090
#define FPU_ADDR_A       0x00020090  // 32-bit operand A
#define FPU_ADDR_B       0x00020094  // 32-bit operand B
#define FPU_ADDR_OP      0x00020098  // 8-bit operation code
#define FPU_ADDR_RESULT  0x0002009C  // 32-bit result
#define FPU_ADDR_FLAGS   0x000200A0  // 8-bit status flags
#define FPU_ADDR_END     0x000200A0

// Operation codes.
#define FPU_OP_ADD  0x00
#define FPU_OP_SUB  0x01
#define FPU_OP_MUL  0x02
#define FPU_OP_DIV  0x03
#define FPU_OP_SQRT 0x04
#define FPU_OP_MIN  0x05
#define FPU_OP_MAX  0x06
#define FPU_OP_CMP  0x07
#define FPU_OP_ITOF 0x10  // int32(A) -> float
#define FPU_OP_FTOI 0x11  // float(A) -> int32
#define FPU_OP_UTOF 0x12  // uint32(A) -> float
#define FPU_OP_FTOU 0x13  // float(A) -> uint32
#define FPU_OP_SIN  0x20
#define FPU_OP_COS  0x21
#define FPU_OP_TAN  0x22
#define FPU_OP_COT  0x23
#define FPU_OP_LOG  0x24
#define FPU_OP_EXP  0x25

// Status flags.
#define FPU_FLAG_ZERO  0x01
#define FPU_FLAG_SIGN  0x02
#define FPU_FLAG_DIV0  0x04
#define FPU_FLAG_INVALID 0x08
#define FPU_FLAG_GT    0x10  // A > B  (cmp)
#define FPU_FLAG_EQ    0x20  // A == B (cmp)
#define FPU_FLAG_LT    0x40  // A < B  (cmp)

// Initialize / reset FPU state.
void fpu_bind_cpu(Cpu* cpu);
void fpu_init(Cpu* cpu);
void fpu_reset(Cpu* cpu);

// MMIO accessors.
uint8_t  fpu_read_byte(Cpu* cpu, uint32_t addr);
uint32_t fpu_read_dword(Cpu* cpu, uint32_t addr);
void     fpu_write_byte(Cpu* cpu, uint32_t addr, uint8_t val);
void     fpu_write_dword(Cpu* cpu, uint32_t addr, uint32_t val);

// Execute the operation currently selected by FPU_OP.
void fpu_execute(Cpu* cpu);

// Returns true if addr belongs to the FPU MMIO region.
static inline bool fpu_is_mmio(uint32_t addr) {
    return (addr >= FPU_ADDR_BASE && addr <= FPU_ADDR_END);
}

#ifdef __cplusplus
}
#endif

#endif // FPU_H
