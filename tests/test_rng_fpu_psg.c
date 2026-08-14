#include "cpu_api.h"
#include "rng.h"
#include "fpu.h"
#include "psg.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_E
#define M_E 2.71828182845904523536
#endif

static int g_failures = 0;
static int g_tests = 0;

#define CHECK(cond, ...) do { \
    g_tests++; \
    if (!(cond)) { \
        g_failures++; \
        printf("FAIL: " __VA_ARGS__); \
        printf("\n"); \
    } \
} while(0)

static float bits_to_float(uint32_t u) {
    union { uint32_t u; float f; } b;
    b.u = u;
    return b.f;
}

static uint32_t float_to_bits(float f) {
    union { uint32_t u; float f; } b;
    b.f = f;
    return b.u;
}

static void test_rng(void) {
    printf("\n=== RNG tests ===\n");
    Cpu cpu;
    memset(&cpu, 0, sizeof(cpu));
    rng_init(&cpu);

    CHECK(cpu.rng_state == RNG_DEFAULT_SEED, "default seed");

    uint32_t a = rng_read_dword(&cpu, RNG_ADDR_RAND);
    uint32_t b = rng_read_dword(&cpu, RNG_ADDR_RAND);
    CHECK(a != b, "two dwords differ");

    uint32_t s = RNG_DEFAULT_SEED;
    s = s * 1103515245U + 12345U;
    CHECK(a == s, "LCG dword #1");
    s = s * 1103515245U + 12345U;
    CHECK(b == s, "LCG dword #2");

    rng_write_dword(&cpu, RNG_ADDR_SEED, 0xDEADBEEFU);
    uint32_t x = rng_read_dword(&cpu, RNG_ADDR_RAND);
    rng_write_dword(&cpu, RNG_ADDR_SEED, 0xDEADBEEFU);
    uint32_t y = rng_read_dword(&cpu, RNG_ADDR_RAND);
    CHECK(x == y, "reseed reproducibility");

    rng_write_dword(&cpu, RNG_ADDR_SEED, RNG_DEFAULT_SEED);
    uint8_t byte = rng_read_byte(&cpu, RNG_ADDR_RAND);
    s = RNG_DEFAULT_SEED;
    s = s * 1103515245U + 12345U;
    uint8_t expected = (uint8_t)(s >> 16);
    CHECK(byte == expected, "byte read");

    rng_write_byte(&cpu, RNG_ADDR_SEED, 0x42);
    CHECK((cpu.rng_state & 0xFF) == 0x42, "byte seed write");
}

static void test_fpu(void) {
    printf("\n=== FPU tests ===\n");
    Cpu cpu;
    memset(&cpu, 0, sizeof(cpu));
    fpu_bind_cpu(&cpu);

    // Basic arithmetic.
    fpu_write_dword(&cpu, FPU_ADDR_A, float_to_bits(1.5f));
    fpu_write_dword(&cpu, FPU_ADDR_B, float_to_bits(2.5f));
    fpu_write_byte(&cpu, FPU_ADDR_OP, FPU_OP_ADD);
    float res = bits_to_float(fpu_read_dword(&cpu, FPU_ADDR_RESULT));
    CHECK(res == 4.0f, "add 1.5+2.5 = %f", res);

    fpu_write_byte(&cpu, FPU_ADDR_OP, FPU_OP_SUB);
    res = bits_to_float(fpu_read_dword(&cpu, FPU_ADDR_RESULT));
    CHECK(res == -1.0f, "sub 1.5-2.5 = %f", res);

    fpu_write_byte(&cpu, FPU_ADDR_OP, FPU_OP_MUL);
    res = bits_to_float(fpu_read_dword(&cpu, FPU_ADDR_RESULT));
    CHECK(res == 3.75f, "mul 1.5*2.5 = %f", res);

    fpu_write_dword(&cpu, FPU_ADDR_A, float_to_bits(7.0f));
    fpu_write_dword(&cpu, FPU_ADDR_B, float_to_bits(2.0f));
    fpu_write_byte(&cpu, FPU_ADDR_OP, FPU_OP_DIV);
    res = bits_to_float(fpu_read_dword(&cpu, FPU_ADDR_RESULT));
    CHECK(res == 3.5f, "div 7/2 = %f", res);

    // Division by zero.
    fpu_write_dword(&cpu, FPU_ADDR_A, float_to_bits(5.0f));
    fpu_write_dword(&cpu, FPU_ADDR_B, float_to_bits(0.0f));
    fpu_write_byte(&cpu, FPU_ADDR_OP, FPU_OP_DIV);
    uint8_t flags = fpu_read_byte(&cpu, FPU_ADDR_FLAGS);
    CHECK((flags & FPU_FLAG_DIV0) != 0, "div0 flag set");

    // sqrt.
    fpu_write_dword(&cpu, FPU_ADDR_A, float_to_bits(4.0f));
    fpu_write_byte(&cpu, FPU_ADDR_OP, FPU_OP_SQRT);
    res = bits_to_float(fpu_read_dword(&cpu, FPU_ADDR_RESULT));
    CHECK(res == 2.0f, "sqrt 4 = %f", res);

    fpu_write_dword(&cpu, FPU_ADDR_A, float_to_bits(-1.0f));
    fpu_write_byte(&cpu, FPU_ADDR_OP, FPU_OP_SQRT);
    flags = fpu_read_byte(&cpu, FPU_ADDR_FLAGS);
    CHECK((flags & FPU_FLAG_INVALID) != 0, "sqrt(-1) invalid flag");

    // min/max.
    fpu_write_dword(&cpu, FPU_ADDR_A, float_to_bits(3.0f));
    fpu_write_dword(&cpu, FPU_ADDR_B, float_to_bits(5.0f));
    fpu_write_byte(&cpu, FPU_ADDR_OP, FPU_OP_MIN);
    res = bits_to_float(fpu_read_dword(&cpu, FPU_ADDR_RESULT));
    CHECK(res == 3.0f, "min(3,5) = %f", res);

    fpu_write_byte(&cpu, FPU_ADDR_OP, FPU_OP_MAX);
    res = bits_to_float(fpu_read_dword(&cpu, FPU_ADDR_RESULT));
    CHECK(res == 5.0f, "max(3,5) = %f", res);

    // cmp.
    fpu_write_byte(&cpu, FPU_ADDR_OP, FPU_OP_CMP);
    flags = fpu_read_byte(&cpu, FPU_ADDR_FLAGS);
    CHECK((flags & FPU_FLAG_LT) != 0, "cmp 3<5 lt flag");

    fpu_write_dword(&cpu, FPU_ADDR_A, float_to_bits(5.0f));
    fpu_write_dword(&cpu, FPU_ADDR_B, float_to_bits(5.0f));
    fpu_write_byte(&cpu, FPU_ADDR_OP, FPU_OP_CMP);
    flags = fpu_read_byte(&cpu, FPU_ADDR_FLAGS);
    CHECK((flags & FPU_FLAG_EQ) != 0, "cmp 5==5 eq flag");

    // Conversions.
    fpu_write_dword(&cpu, FPU_ADDR_A, (uint32_t)(int32_t)42);
    fpu_write_byte(&cpu, FPU_ADDR_OP, FPU_OP_ITOF);
    res = bits_to_float(fpu_read_dword(&cpu, FPU_ADDR_RESULT));
    CHECK(res == 42.0f, "itof 42 = %f", res);

    fpu_write_dword(&cpu, FPU_ADDR_A, float_to_bits(-7.0f));
    fpu_write_byte(&cpu, FPU_ADDR_OP, FPU_OP_FTOI);
    uint32_t bits = fpu_read_dword(&cpu, FPU_ADDR_RESULT);
    CHECK((int32_t)bits == -7, "ftoi -7 = %d", (int32_t)bits);

    fpu_write_dword(&cpu, FPU_ADDR_A, (uint32_t)123456789);
    fpu_write_byte(&cpu, FPU_ADDR_OP, FPU_OP_UTOF);
    res = bits_to_float(fpu_read_dword(&cpu, FPU_ADDR_RESULT));
    CHECK(res == 123456789.0f, "utof 123456789 = %f", res);

    // Trig.
    fpu_write_dword(&cpu, FPU_ADDR_A, float_to_bits(0.0f));
    fpu_write_byte(&cpu, FPU_ADDR_OP, FPU_OP_SIN);
    res = bits_to_float(fpu_read_dword(&cpu, FPU_ADDR_RESULT));
    CHECK(fabsf(res) < 0.001f, "sin(0) = %f", res);

    fpu_write_dword(&cpu, FPU_ADDR_A, float_to_bits((float)M_PI / 2.0f));
    fpu_write_byte(&cpu, FPU_ADDR_OP, FPU_OP_SIN);
    res = bits_to_float(fpu_read_dword(&cpu, FPU_ADDR_RESULT));
    CHECK(fabsf(res - 1.0f) < 0.001f, "sin(pi/2) = %f", res);

    fpu_write_dword(&cpu, FPU_ADDR_A, float_to_bits(0.0f));
    fpu_write_byte(&cpu, FPU_ADDR_OP, FPU_OP_COS);
    res = bits_to_float(fpu_read_dword(&cpu, FPU_ADDR_RESULT));
    CHECK(fabsf(res - 1.0f) < 0.001f, "cos(0) = %f", res);

    fpu_write_dword(&cpu, FPU_ADDR_A, float_to_bits(1.0f));
    fpu_write_byte(&cpu, FPU_ADDR_OP, FPU_OP_TAN);
    res = bits_to_float(fpu_read_dword(&cpu, FPU_ADDR_RESULT));
    CHECK(fabsf(res - tanf(1.0f)) < 0.001f, "tan(1) = %f", res);

    fpu_write_dword(&cpu, FPU_ADDR_A, float_to_bits((float)M_PI / 4.0f));
    fpu_write_byte(&cpu, FPU_ADDR_OP, FPU_OP_COT);
    res = bits_to_float(fpu_read_dword(&cpu, FPU_ADDR_RESULT));
    CHECK(fabsf(res - 1.0f) < 0.01f, "cot(pi/4) = %f", res);

    // log/exp.
    fpu_write_dword(&cpu, FPU_ADDR_A, float_to_bits((float)M_E));
    fpu_write_byte(&cpu, FPU_ADDR_OP, FPU_OP_LOG);
    res = bits_to_float(fpu_read_dword(&cpu, FPU_ADDR_RESULT));
    CHECK(fabsf(res - 1.0f) < 0.001f, "log(e) = %f", res);

    fpu_write_dword(&cpu, FPU_ADDR_A, float_to_bits(0.0f));
    fpu_write_byte(&cpu, FPU_ADDR_OP, FPU_OP_LOG);
    flags = fpu_read_byte(&cpu, FPU_ADDR_FLAGS);
    CHECK((flags & FPU_FLAG_INVALID) != 0, "log(0) invalid flag");

    fpu_write_dword(&cpu, FPU_ADDR_A, float_to_bits(1.0f));
    fpu_write_byte(&cpu, FPU_ADDR_OP, FPU_OP_EXP);
    res = bits_to_float(fpu_read_dword(&cpu, FPU_ADDR_RESULT));
    CHECK(fabsf(res - (float)M_E) < 0.001f, "exp(1) = %f", res);
}

static void test_psg_registers(void) {
    printf("\n=== PSG register tests ===\n");
    Cpu cpu;
    memset(&cpu, 0, sizeof(cpu));
    psg_bind_cpu(&cpu);

    // Tone period word write/read.
    psg_write_word(&cpu, PSG_ADDR_TONE0, 0x1234);
    CHECK(psg_read_word(&cpu, PSG_ADDR_TONE0) == 0x1234, "tone0 word r/w");
    CHECK(psg_read_byte(&cpu, PSG_ADDR_TONE0) == 0x34, "tone0 low byte");
    CHECK(psg_read_byte(&cpu, PSG_ADDR_TONE0 + 1) == 0x12, "tone0 high byte");

    // Byte-wise write.
    psg_write_byte(&cpu, PSG_ADDR_TONE1, 0x56);
    psg_write_byte(&cpu, PSG_ADDR_TONE1 + 1, 0x78);
    CHECK(psg_read_word(&cpu, PSG_ADDR_TONE1) == 0x7856, "tone1 byte r/w");

    // Mixer.
    psg_write_byte(&cpu, PSG_ADDR_MIXER, PSG_MIXER_TONE0 | PSG_MIXER_NOISE1);
    CHECK(psg_read_byte(&cpu, PSG_ADDR_MIXER) == (PSG_MIXER_TONE0 | PSG_MIXER_NOISE1), "mixer");

    // Volume with envelope enable.
    psg_write_byte(&cpu, PSG_ADDR_VOL2, 0x8F);
    CHECK(psg_read_byte(&cpu, PSG_ADDR_VOL2) == 0x8F, "vol2");

    // Envelope period/shape.
    psg_write_word(&cpu, PSG_ADDR_ENV_PERIOD, 1000);
    CHECK(psg_read_word(&cpu, PSG_ADDR_ENV_PERIOD) == 1000, "env period");
    psg_write_byte(&cpu, PSG_ADDR_ENV_SHAPE, PSG_ENV_TRIANGLE);
    CHECK(psg_read_byte(&cpu, PSG_ADDR_ENV_SHAPE) == PSG_ENV_TRIANGLE, "env shape");
}

int main(void) {
    test_rng();
    test_fpu();
    test_psg_registers();

    printf("\n=== Results: %d tests, %d failures ===\n", g_tests, g_failures);
    return g_failures > 0 ? 1 : 0;
}
