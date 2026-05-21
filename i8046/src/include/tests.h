#ifndef TESTS_H
#define TESTS_H

#include "cpu.h"
#include <cstdint>
#include <cstdio>

struct TestResult {
    const char* name;
    bool passed;
    const char* message;
};

void test_setup(Cpu* cpu, uint8_t* mem, size_t mem_size);
void test_load_program(Cpu* cpu, const uint8_t* code, size_t len, uint32_t addr = 0x0000);
void test_reset_regs(Cpu* cpu);
bool test_check_reg(const Cpu* cpu, uint8_t idx, uint64_t expected, const char* reg_name);
bool test_check_mem(const Cpu* cpu, uint32_t addr, uint64_t expected, CpuMode mode, const char* desc);
bool test_check_flag(const Cpu* cpu, uint8_t flag, bool expected, const char* flag_name);

TestResult test_nop_halt();
TestResult test_ldi_mov();
TestResult test_arithmetic();      // ADD, SUB, ADC, SBB
TestResult test_multiply_divide(); // MUL, IMUL, DIV, IDIV
TestResult test_logic();           // AND, OR, XOR, NOT
TestResult test_unary();           // INC, DEC, CMP, ICMP
TestResult test_shift_rotate();    // LSL, LSR, ASR, ROL, ROR
TestResult test_memory_access();   // STR, LOD (все режимы)
TestResult test_stack();           // PUSH, POP
TestResult test_control_flow();    // JMP, CALL, RET
TestResult test_system();          // CLI, STI, LOCK, UNLOCK

#endif // TESTS_H