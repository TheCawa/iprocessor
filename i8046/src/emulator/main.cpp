#include <iostream>
#include <vector>
#include <iomanip>
#include <cstdint>
#include "cpu.h"
#include "tests.h"
#include <windows.h>

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(65001);
    
    const size_t RAM_SIZE = 1024 * 1024;
    std::vector<uint8_t> memory(RAM_SIZE, 0);
    Cpu cpu;

    cpu_init(&cpu, memory.data(), RAM_SIZE);
    
    // ===== Загрузка программы =====
    if (argc > 1) {
        const char* filename = argv[1];
        uint32_t load_addr = 0x0000;
        if (argc > 2) {
            load_addr = (uint32_t)strtoul(argv[2], nullptr, 0);
        }
        
        std::cout << "Loading file: " << filename 
                  << " @ 0x" << std::hex << load_addr << std::dec << "\n";
        
        if (cpu_load_file(&cpu, filename, load_addr) != 0) {
            std::cerr << "[ERROR] Failed to load file!\n";
            return 1;
        }

        cpu.regs[REG_IC] = load_addr;
        std::cout << "\n=== Execution Start ===\n";
        int step = 0;
        while (!cpu.halted && step < 1000) {
            int bytes = cpu_step(&cpu);
            if (bytes <= 0) break;
            step++;
        }
        std::cout << "Executed " << step << " steps\n";
        
    } else {
        // Режим: встроенные тесты
        test_setup(&cpu, memory.data(), RAM_SIZE);
        
        std::cout << "╔════════════════════════════════╗\n";
        std::cout << "║   CPU Instruction Test Suite   ║\n";
        std::cout << "╚════════════════════════════════╝\n\n";
        
        TestResult (*all_tests[])() = {
            test_nop_halt, test_ldi_mov, test_arithmetic,
            test_logic, test_unary, test_shift_rotate,
            test_memory_access, test_stack, test_control_flow, test_system,
        };
        
        int passed = 0, total = sizeof(all_tests)/sizeof(all_tests[0]);
        for (int i = 0; i < total; ++i) {
            TestResult res = all_tests[i]();
            std::cout << "[" << (res.passed ? "PASS" : "FAIL") << "] " 
                      << std::setw(25) << std::left << res.name 
                      << ": " << res.message << "\n";
            if (res.passed) passed++;
        }
        
        std::cout << "\n╔════════════════════════════════╗\n";
        std::cout << "║   Results: " << passed << "/" << total << " tests passed";
        std::cout << std::string(30 - (passed>=10?2:1) - (total>=10?2:1), ' ') << "║\n";
        std::cout << "╚════════════════════════════════╝\n";
        
        return (passed == total) ? 0 : 1;
    }
    
    return 0;
}