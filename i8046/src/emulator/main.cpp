#include <iostream>
#include <vector>
#include <cstdint>
#include "cpu.h"
#include "emulator.hpp"
#include <windows.h>

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(65001);
    
    const size_t RAM_SIZE = 1024 * 1024; // 1 MB
    std::vector<uint8_t> memory(RAM_SIZE, 0);
    Cpu cpu;

    // Инициализация структуры процессора
    cpu_init(&cpu, memory.data(), RAM_SIZE);

    // Инициализация графического движка (SDL2 + ImGui)
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (!emulator_init(&window, &renderer)) {
        std::cerr << "[ERROR] Failed to initialize graphics subsystem!\n";
        return 1;
    }

    // ===== Загрузка программы (ROM) =====
    if (argc > 1) {
        const char* filename = argv[1];
        uint32_t load_addr = 0x0000;
        if (argc > 2) {
            load_addr = (uint32_t)strtoul(argv[2], nullptr, 0);
        }
        
        if (cpu_load_file(&cpu, filename, load_addr) != 0) {
            std::cerr << "[ERROR] Failed to load binary file!\n";
            emulator_shutdown();
            return 1;
        }
        cpu.regs[REG_IC] = load_addr;
    } else {
        // Запись тестовой заглушки "HI!" прямо в видеопамять эмулятора
        memory[VBUFFER_BASE + 0] = 'H'; memory[VBUFFER_BASE + 1] = 0x1F;
        memory[VBUFFER_BASE + 2] = 'I'; memory[VBUFFER_BASE + 3] = 0x1F;
        memory[VBUFFER_BASE + 4] = '!'; memory[VBUFFER_BASE + 5] = 0x2F;
        cpu.screen_dirty = true;
    }

    std::cout << "=== Emulator Started successfully ===\n";

    bool running = true;
    
    // Главный игровой/эмуляционный цикл приложения
    while (running) {
        // 1. Обрабатываем системные прерывания интерфейса (закрытие, клики мыши)
        if (!emulator_handle_events()) {
            running = false;
        }

        // 2. Шаг симуляции процессора
        // Вместо выполнения всех 10000 шагов за раз, выполняем, например, по 50 шагов за кадр,
        // чтобы эмуляция шла плавно и интерфейс оставался отзывчивым.
        if (!cpu.halted) {
            for (int i = 0; i < 50; i++) {
                if (cpu_step(&cpu) <= 0) {
                    break;
                }
            }
        }

        // 3. Рендеринг нового кадра интерфейса ImGui
        emulator_render(&cpu, renderer);
        
        // Небольшая разгрузка процессора хост-машины (примерно 60 FPS)
        SDL_Delay(16);
    }

    // Корректно уничтожаем контексты перед выходом
    emulator_shutdown();
    std::cout << "Emulator closed safely.\n";
    return 0;
}