#pragma once
#include <SDL.h>
#include "cpu.h"

// Инициализация SDL2 и Dear ImGui
bool emulator_init(SDL_Window** out_window, SDL_Renderer** out_renderer);

// Главный цикл отрисовки интерфейса (Экран + Регистры + Меню)
void emulator_render(Cpu* cpu, SDL_Renderer* renderer);

// Обработка системных событий и событий ImGui
bool emulator_handle_events();

// Корректное закрытие всех контекстов
void emulator_shutdown();