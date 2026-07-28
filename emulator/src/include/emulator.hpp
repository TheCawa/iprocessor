#pragma once
#include <SDL.h>
#include <vector>
#include "cpu_api.h"
#include "videocard.h"

// Initialize SDL2, Dear ImGui and the selected video card.
bool emulator_init(SDL_Window** out_window, SDL_Renderer** out_renderer, const VideoCard* vc);

// Main rendering loop (screen + registers + menu)
void emulator_render(Cpu* cpu, SDL_Renderer* renderer, std::vector<uint8_t>& memory);

// Switch the active CPU backend and resize/reinit memory.
bool emulator_switch_cpu(Cpu* cpu, const CpuBackend* new_backend, std::vector<uint8_t>& memory);

// Switch the active video card (reinitializes renderer resources).
bool emulator_switch_vc(Cpu* cpu, const VideoCard* new_vc, SDL_Renderer* renderer);

// Handle system events and ImGui events
bool emulator_handle_events(Cpu* cpu);

// CPU speed modes.
typedef enum {
    CPU_SPEED_ADAPTIVE = 0,   // run as many steps as fit in the frame budget
    CPU_SPEED_FIXED,          // run a fixed number of steps per frame
    CPU_SPEED_UNLIMITED       // run up to max_steps per frame
} CpuSpeedMode;

// CPU control state for the main loop
bool emulator_is_running();
bool emulator_consume_step();

// Input capture state (true when the emulated machine owns keyboard/mouse).
bool emulator_is_input_captured(void);

// CPU speed settings
void emulator_set_speed_mode(CpuSpeedMode mode);
CpuSpeedMode emulator_get_speed_mode(void);
void emulator_set_fixed_steps(int steps);
int  emulator_get_fixed_steps(void);
void emulator_set_max_steps(int steps);
int  emulator_get_max_steps(void);

// Report the number of CPU steps executed this frame for the frequency indicator.
void emulator_report_steps(int steps);

// Properly destroy all contexts
void emulator_shutdown();
