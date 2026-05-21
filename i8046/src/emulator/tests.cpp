#include "tests.h"
#include <cstring>
#include <cstdio>

static Cpu* g_cpu = nullptr;
static uint8_t* g_mem = nullptr;
static size_t g_mem_size = 0;

void test_setup(Cpu* cpu, uint8_t* mem, size_t mem_size) {
    g_cpu = cpu;
    g_mem = mem;
    g_mem_size = mem_size;
    cpu_init(cpu, mem, mem_size);
}

void test_load_program(Cpu* cpu, const uint8_t* code, size_t len, uint32_t addr) {
    if (addr + len <= g_mem_size) {
        std::memcpy(&g_mem[addr], code, len);
    }
}

void test_reset_regs(Cpu* cpu) {
    std::memset(cpu->regs, 0, sizeof(cpu->regs));
    cpu->regs[REG_SP] = g_mem_size - 1;
    cpu->regs[REG_IC] = 0x0000;
    cpu->halted = false;
}

bool test_check_reg(const Cpu* cpu, uint8_t idx, uint64_t expected, const char* reg_name) {
    uint64_t actual = cpu_get_reg(cpu, idx);
    if (actual != expected) {
        printf("  [FAIL] %s: expected 0x%08llX, got 0x%08llX\n", reg_name, expected, actual);
        return false;
    }
    return true;
}

bool test_check_mem(const Cpu* cpu, uint32_t addr, uint64_t expected, CpuMode mode, const char* desc) {
    uint64_t actual = cpu_read_mem(cpu, addr, mode);
    size_t bytes = (mode == MODE_ADDR) ? 3 : (1 << mode);
    if (actual != (expected & ((1ULL << (bytes * 8)) - 1))) {
        printf("  [FAIL] %s @0x%06X: expected 0x%08llX, got 0x%08llX\n", 
               desc, addr, expected, actual);
        return false;
    }
    return true;
}

bool test_check_flag(const Cpu* cpu, uint8_t flag, bool expected, const char* flag_name) {
    bool actual = (cpu->regs[REG_FL] & flag) != 0;
    if (actual != expected) {
        printf("  [FAIL] Flag %s: expected %d, got %d\n", flag_name, expected, actual);
        return false;
    }
    return true;
}

// ==================== ТЕСТЫ ====================

TestResult test_nop_halt() {
    TestResult result = {"NOP/HALT", true, "OK"};
    test_reset_regs(g_cpu);
    
    uint8_t code[] = {0x00, 0x00, 0x01, 0x00};
    test_load_program(g_cpu, code, sizeof(code));
    
    int steps = 0;
    while (!g_cpu->halted && steps < 10) {
        cpu_step(g_cpu);
        steps++;
    }
    
    if (!g_cpu->halted) {
        result.passed = false;
        result.message = "HALT did not stop execution";
    }
    return result;
}

TestResult test_ldi_mov() {
    TestResult result = {"LDI/MOV", true, "OK"};
    test_reset_regs(g_cpu);
    
    uint8_t code[] = {
        0x17, 0x01, 0x00, 0x42,
        0x17, 0x00, 0x01, 0x00,
        0x01, 0x00
    };
    test_load_program(g_cpu, code, sizeof(code));
    
    while (!g_cpu->halted) cpu_step(g_cpu);
    
    if (!test_check_reg(g_cpu, REG_R0, 0x42, "r0")) result.passed = false;
    if (!test_check_reg(g_cpu, REG_X1, 0x42, "r1")) result.passed = false;
    
    return result;
}

TestResult test_arithmetic() {
    TestResult result = {"Arithmetic", true, "OK"};
    test_reset_regs(g_cpu);
    g_cpu->regs[REG_R0] = 0x10;
    cpu_set_reg(g_cpu, REG_X1, 0x20);
    
    uint8_t code[] = {
        0x02, 0x00, 0x00, 0x01,  // ADD r0, r1  => r0 = 0x30
        0x04, 0x00, 0x00, 0x01,  // SUB r0, r1  => r0 = 0x10
        0x01, 0x00                // HALT
    };
    test_load_program(g_cpu, code, sizeof(code));
    
    while (!g_cpu->halted) cpu_step(g_cpu);
    
    if (!test_check_reg(g_cpu, REG_R0, 0x10, "r0 after ADD/SUB")) result.passed = false;
    if (!test_check_flag(g_cpu, FLAG_Z, false, "Z after non-zero")) result.passed = false;
    
    // Тест флага Z: 0x10 - 0x10 = 0
    test_reset_regs(g_cpu);
    cpu_set_reg(g_cpu, REG_R0, 0x10);
    cpu_set_reg(g_cpu, REG_X1, 0x10);
    uint8_t code2[] = {0x04, 0x00, 0x00, 0x01, 0x01, 0x00}; // SUB r0,r1; HALT
    test_load_program(g_cpu, code2, sizeof(code2));
    while (!g_cpu->halted) cpu_step(g_cpu);
    
    if (!test_check_flag(g_cpu, FLAG_Z, true, "Z after zero result")) result.passed = false;
    
    return result;
}

TestResult test_logic() {
    TestResult result = {"Logic", true, "OK"};
    test_reset_regs(g_cpu);
    
    g_cpu->regs[REG_R0] = 0xF0;
    cpu_set_reg(g_cpu, REG_X1, 0x0F);
    
    uint8_t code[] = {
        0x0A, 0x00, 0x00, 0x01,  // AND r0, r1
        0x0B, 0x00, 0x00, 0x01,  // OR  r0, r1
        0x01, 0x00                // HALT
    };
    test_load_program(g_cpu, code, sizeof(code));
    
    while (!g_cpu->halted) cpu_step(g_cpu);
    
    if (!test_check_reg(g_cpu, REG_R0, 0x0F, "r0 after AND/OR")) result.passed = false;
    
    // NOT r0: ~0x0F = 0xF0 (для байта)
    test_reset_regs(g_cpu);
    cpu_set_reg(g_cpu, REG_R0, 0x0F);
    uint8_t code2[] = {0x0D, 0x00, 0x00, 0x01, 0x00}; // NOT r0; HALT
    test_load_program(g_cpu, code2, sizeof(code2));
    while (!g_cpu->halted) cpu_step(g_cpu);
    
    if (!test_check_reg(g_cpu, REG_R0, 0xF0, "r0 after NOT")) result.passed = false;
    
    return result;
}

TestResult test_unary() {
    TestResult result = {"Unary (INC/DEC/CMP)", true, "OK"};
    test_reset_regs(g_cpu);
    
    cpu_set_reg(g_cpu, REG_R0, 0x41);
    uint8_t code[] = {
        0x0E, 0x00, 0x00,  // INC r0 => 0x42
        0x0F, 0x00, 0x00,  // DEC r0 => 0x41
        0x01, 0x00          // HALT
    };
    test_load_program(g_cpu, code, sizeof(code));
    while (!g_cpu->halted) cpu_step(g_cpu);
    
    if (!test_check_reg(g_cpu, REG_R0, 0x41, "r0 after INC/DEC")) result.passed = false;
    
    // CMP: проверяем флаги
    test_reset_regs(g_cpu);
    cpu_set_reg(g_cpu, REG_R0, 0x10);
    cpu_set_reg(g_cpu, REG_X1, 0x20);
    uint8_t code2[] = {0x10, 0x00, 0x00, 0x01, 0x01, 0x00}; // CMP r0,r1; HALT
    test_load_program(g_cpu, code2, sizeof(code2));
    while (!g_cpu->halted) cpu_step(g_cpu);
    
    if (!test_check_flag(g_cpu, FLAG_L, true, "L (less) after CMP 0x10 < 0x20")) result.passed = false;
    
    return result;
}

TestResult test_shift_rotate() {
    TestResult result = {"Shift/Rotate", true, "OK"};
    test_reset_regs(g_cpu);
    
    cpu_set_reg(g_cpu, REG_R0, 0x01);
    uint8_t code[] = {
        0x1A, 0x00, 0x00, 0x03,  // LSL r0, 3
        0x01, 0x00                // HALT
    };
    test_load_program(g_cpu, code, sizeof(code));
    while (!g_cpu->halted) cpu_step(g_cpu);
    
    if (!test_check_reg(g_cpu, REG_R0, 0x08, "r0 after LSL")) result.passed = false;
    
    test_reset_regs(g_cpu);
    cpu_set_reg(g_cpu, REG_R0, 0x88);
    uint8_t code2[] = {0x1E, 0x00, 0x00, 0x01, 0x01, 0x00}; // ROR r0,1; HALT
    test_load_program(g_cpu, code2, sizeof(code2));
    while (!g_cpu->halted) cpu_step(g_cpu);
    
    if (!test_check_reg(g_cpu, REG_R0, 0x44, "r0 after ROR")) result.passed = false;
    
    return result;
}

TestResult test_memory_access() {
    TestResult result = {"Memory Access (STR/LOD)", true, "OK"};
    test_reset_regs(g_cpu);
    
    cpu_set_reg(g_cpu, REG_R0, 0xAB);
    
    uint8_t code[] = {
        0x18, 0x04, 0x00, 0x00, 0x01, 0x00,  // STR.b r0, [0x0100]
        0x19, 0x04, 0x01, 0x00, 0x01, 0x00,  // LOD.b r1, [0x0100]
        0x01, 0x00                            // HALT
    };
    test_load_program(g_cpu, code, sizeof(code));
    while (!g_cpu->halted) cpu_step(g_cpu);
    
    if (!test_check_mem(g_cpu, 0x0100, 0xAB, MODE_BYTE, "mem[0x100] after STR")) result.passed = false;
    if (!test_check_reg(g_cpu, REG_X1, 0xAB, "r1 after LOD")) result.passed = false;
    
    return result;
}

TestResult test_stack() {
    TestResult result = {"Stack (PUSH/POP)", true, "OK"};
    test_reset_regs(g_cpu);
    
    uint32_t initial_sp = (uint32_t)g_cpu->regs[REG_SP];
    cpu_set_reg(g_cpu, REG_R0, 0x1234);
    
    uint8_t code[] = {
        0x13, 0x40, 0x00,  // PUSH.w r0
        0x14, 0x40, 0x01,  // POP.w r1
        0x01, 0x00          // HALT
    };
    test_load_program(g_cpu, code, sizeof(code));
    while (!g_cpu->halted) cpu_step(g_cpu);
    
    if (g_cpu->regs[REG_SP] != initial_sp) {
        printf("  [FAIL] SP: expected 0x%06X, got 0x%06llX\n", initial_sp, g_cpu->regs[REG_SP]);
        result.passed = false;
    }
    if (!test_check_reg(g_cpu, REG_X1, 0x1234, "r1 after POP")) result.passed = false;
    
    return result;
}

TestResult test_control_flow() {
    TestResult result = {"Control Flow (JMP/CALL/RET)", true, "OK"};
    test_reset_regs(g_cpu);
    
    uint8_t code[] = {
        0x12, 0x00, 0x00, 0x0A, 0x00, 0x00,  // JMP UNC 0x000A
        0x17, 0x01, 0x00, 0xFF,                // (пропускаем) LDI.b r0, 0xFF
        0x17, 0x01, 0x00, 0x42,                // 0x000A: LDI.b r0, 0x42
        0x01, 0x00                             // HALT
    };
    test_load_program(g_cpu, code, sizeof(code));
    while (!g_cpu->halted) cpu_step(g_cpu);
    
    if (!test_check_reg(g_cpu, REG_R0, 0x42, "r0 after JMP")) result.passed = false;
    
    return result;
}

TestResult test_system() {
    TestResult result = {"System (CLI/STI/LOCK)", true, "OK"};
    test_reset_regs(g_cpu);
    
    g_cpu->irq_enabled = false;
    
    uint8_t code[] = {
        0x20, 0x00,  // STI
        0x1F, 0x00,  // CLI
        0x01, 0x00   // HALT
    };
    test_load_program(g_cpu, code, sizeof(code));
    while (!g_cpu->halted) cpu_step(g_cpu);
    
    if (g_cpu->irq_enabled != false) {
        printf("  [FAIL] irq_enabled after CLI: expected false\n");
        result.passed = false;
    }
    
    test_reset_regs(g_cpu);
    cpu_set_reg(g_cpu, REG_R0, 0x10);
    
    uint8_t code2[] = {
        0x26, 0x00, 0x00,       // LOCK r0
        0x17, 0x01, 0x00, 0xFF, // LDI.b r0, 0xFF (должно быть проигнорировано)
        0x27, 0x00, 0x00,       // UNLOCK r0
        0x01, 0x00              // HALT
    };
    test_load_program(g_cpu, code2, sizeof(code2));
    while (!g_cpu->halted) cpu_step(g_cpu);
    
    if (!test_check_reg(g_cpu, REG_R0, 0x10, "r0 after LOCK+LDI")) result.passed = false;
    
    return result;
}