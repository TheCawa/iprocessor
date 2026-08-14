// ------------------------------------------------------------------------------
//          fpu.c - Floating-point unit implementation
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

#include "fpu.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

typedef union {
    uint32_t u;
    float    f;
} FpuFloatBits;

static inline float fpu_bits_to_float(uint32_t u) {
    FpuFloatBits bits;
    bits.u = u;
    return bits.f;
}

static inline uint32_t fpu_float_to_bits(float f) {
    FpuFloatBits bits;
    bits.f = f;
    return bits.u;
}

typedef struct {
    float    a;
    float    b;
    uint8_t  op;
    float    result;
    uint8_t  flags;
} FpuState;

static FpuState* fpu_state(Cpu* cpu) {
    return cpu ? (FpuState*)cpu->fpu_data : NULL;
}

static FpuState g_fpu;

void fpu_bind_cpu(Cpu* cpu) {
    g_fpu = (FpuState){0};
    if (cpu) {
        cpu->fpu_data = &g_fpu;
        fpu_init(cpu);
    }
}

void fpu_init(Cpu* cpu) {
    FpuState* fpu = fpu_state(cpu);
    if (!fpu) return;
    memset(fpu, 0, sizeof(FpuState));
}

void fpu_reset(Cpu* cpu) {
    fpu_init(cpu);
}

static uint8_t fpu_pack_status(float res, bool cmp, uint8_t cmp_flags) {
    uint8_t flags = 0;
    if (cmp) {
        flags |= cmp_flags;
    } else {
        if (res == 0.0f) flags |= FPU_FLAG_ZERO;
        if (res < 0.0f)  flags |= FPU_FLAG_SIGN;
    }
    return flags;
}

void fpu_execute(Cpu* cpu) {
    FpuState* fpu = fpu_state(cpu);
    if (!fpu) return;

    float a = fpu->a;
    float b = fpu->b;
    float res = 0.0f;
    uint8_t flags = fpu->flags & FPU_FLAG_INVALID; // keep sticky invalid? no, clear errors
    flags = 0;

    switch (fpu->op) {
        case FPU_OP_ADD: res = a + b; break;
        case FPU_OP_SUB: res = a - b; break;
        case FPU_OP_MUL: res = a * b; break;
        case FPU_OP_DIV:
            if (b == 0.0f) {
                flags |= FPU_FLAG_DIV0;
                res = (a >= 0.0f) ? INFINITY : -INFINITY;
            } else {
                res = a / b;
            }
            break;
        case FPU_OP_SQRT:
            if (a < 0.0f) {
                flags |= FPU_FLAG_INVALID;
                res = NAN;
            } else {
                res = sqrtf(a);
            }
            break;
        case FPU_OP_MIN: res = (a < b) ? a : b; break;
        case FPU_OP_MAX: res = (a > b) ? a : b; break;
        case FPU_OP_CMP:
            if (a > b)  flags |= FPU_FLAG_GT;
            if (a == b) flags |= FPU_FLAG_EQ;
            if (a < b)  flags |= FPU_FLAG_LT;
            res = 0.0f;
            break;
        case FPU_OP_ITOF: {
            uint32_t bits = fpu_float_to_bits(fpu->a);
            res = (float)(int32_t)bits;
            break;
        }
        case FPU_OP_FTOI: {
            int32_t iv = (int32_t)truncf(a);
            fpu->result = fpu_bits_to_float((uint32_t)iv);
            flags |= (iv == 0) ? FPU_FLAG_ZERO : 0;
            flags |= (iv < 0)  ? FPU_FLAG_SIGN : 0;
            fpu->flags = flags;
            return;
        }
        case FPU_OP_UTOF: {
            uint32_t bits = fpu_float_to_bits(fpu->a);
            res = (float)bits;
            break;
        }
        case FPU_OP_FTOU: {
            uint32_t uv = (uint32_t)truncf(a);
            fpu->result = fpu_bits_to_float(uv);
            flags |= (uv == 0) ? FPU_FLAG_ZERO : 0;
            fpu->flags = flags;
            return;
        }
        case FPU_OP_SIN: res = sinf(a); break;
        case FPU_OP_COS: res = cosf(a); break;
        case FPU_OP_TAN: res = tanf(a); break;
        case FPU_OP_COT: {
            float t = tanf(a);
            if (t == 0.0f) {
                flags |= FPU_FLAG_DIV0;
                res = (cosf(a) >= 0.0f) ? INFINITY : -INFINITY;
            } else {
                res = 1.0f / t;
            }
            break;
        }
        case FPU_OP_LOG:
            if (a <= 0.0f) {
                flags |= FPU_FLAG_INVALID;
                res = NAN;
            } else {
                res = logf(a);
            }
            break;
        case FPU_OP_EXP: res = expf(a); break;
        default: res = 0.0f; break;
    }

    if (fpu->op != FPU_OP_CMP) {
        flags |= fpu_pack_status(res, false, 0);
    }

    fpu->result = res;
    fpu->flags = flags;
}

uint8_t fpu_read_byte(Cpu* cpu, uint32_t addr) {
    FpuState* fpu = fpu_state(cpu);
    if (!fpu) return 0;

    if (addr == FPU_ADDR_OP)    return fpu->op;
    if (addr == FPU_ADDR_FLAGS) return fpu->flags;

    uint32_t base = addr & ~3U;
    uint32_t off  = addr - base;
    uint32_t val = 0;
    switch (base) {
        case FPU_ADDR_A:      val = fpu_float_to_bits(fpu->a); break;
        case FPU_ADDR_B:      val = fpu_float_to_bits(fpu->b); break;
        case FPU_ADDR_RESULT: val = fpu_float_to_bits(fpu->result); break;
        default: return 0;
    }
    // Big-endian byte order to match i80148 memory layout.
    return (uint8_t)(val >> ((3 - off) * 8));
}

uint32_t fpu_read_dword(Cpu* cpu, uint32_t addr) {
    FpuState* fpu = fpu_state(cpu);
    if (!fpu) return 0;

    switch (addr) {
        case FPU_ADDR_A:      return fpu_float_to_bits(fpu->a);
        case FPU_ADDR_B:      return fpu_float_to_bits(fpu->b);
        case FPU_ADDR_OP:     return fpu->op;
        case FPU_ADDR_RESULT: return fpu_float_to_bits(fpu->result);
        case FPU_ADDR_FLAGS:  return fpu->flags;
    }
    return 0;
}

void fpu_write_byte(Cpu* cpu, uint32_t addr, uint8_t val) {
    FpuState* fpu = fpu_state(cpu);
    if (!fpu) return;

    if (addr == FPU_ADDR_OP) {
        fpu->op = val;
        fpu_execute(cpu);
        return;
    }
    if (addr == FPU_ADDR_FLAGS) {
        fpu->flags = val;
        return;
    }

    uint32_t base = addr & ~3U;
    uint32_t off  = addr - base;
    uint32_t mask = 0xFFU << ((3 - off) * 8);
    uint32_t nval = (uint32_t)val << ((3 - off) * 8);

    uint32_t bits = 0;
    float* target = NULL;
    switch (base) {
        case FPU_ADDR_A:      bits = fpu_float_to_bits(fpu->a);      target = &fpu->a;      break;
        case FPU_ADDR_B:      bits = fpu_float_to_bits(fpu->b);      target = &fpu->b;      break;
        case FPU_ADDR_RESULT: bits = fpu_float_to_bits(fpu->result); target = &fpu->result; break;
        default: return;
    }
    bits = (bits & ~mask) | nval;
    *target = fpu_bits_to_float(bits);
}

void fpu_write_dword(Cpu* cpu, uint32_t addr, uint32_t val) {
    FpuState* fpu = fpu_state(cpu);
    if (!fpu) return;

    switch (addr) {
        case FPU_ADDR_A:      fpu->a = fpu_bits_to_float(val); break;
        case FPU_ADDR_B:      fpu->b = fpu_bits_to_float(val); break;
        case FPU_ADDR_OP:
            fpu->op = (uint8_t)val;
            fpu_execute(cpu);
            break;
        case FPU_ADDR_RESULT: fpu->result = fpu_bits_to_float(val); break;
        case FPU_ADDR_FLAGS:  fpu->flags = (uint8_t)val; break;
    }
}
