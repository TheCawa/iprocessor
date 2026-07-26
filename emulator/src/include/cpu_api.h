// ------------------------------------------------------------------------------
//          cpu_api.h - Common CPU API header
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

#ifndef CPU_API_H
#define CPU_API_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Common memory-mapped device addresses used by multiple CPU backends.
#define TERM_OUT_ADDR       0x00020018
#define TERM_RESET_ADDR     0x00020019

// Forward declaration
struct Cpu;
typedef struct Cpu Cpu;

// Disk controller limits and per-drive state.
#define DISK_MAX_DRIVES 4
#define DISK_SECTOR_SIZE 512

typedef struct {
    char     image_path[512];
    uint32_t lba;
    uint32_t buffer;
    uint32_t count;
    uint32_t status;
    uint32_t ctrl;
    uint8_t  sector_buffer[DISK_SECTOR_SIZE];
    uint32_t buffer_offset;
    uint32_t din_shadow;
    uint8_t* image_data;
    size_t   image_size;
} DiskDrive;

// CPU backend vtable. Each processor core (i80148, i8046, ...) provides one
// of these and registers it via cpu_backend_register().
typedef struct CpuBackend {
    const char* name;                       // short id, e.g. "i80148"
    const char* description;                // human-readable name
    size_t      state_size;                 // bytes of backend-specific state

    // Lifecycle
    void (*init)(Cpu* cpu);
    void (*reset)(Cpu* cpu);

    // Execution
    int  (*step)(Cpu* cpu);

    // Loading
    int  (*load_file)(Cpu* cpu, const char* filename, uint32_t load_addr);

    // Register access
    uint64_t (*get_reg)(Cpu* cpu, uint8_t idx);
    void     (*set_reg)(Cpu* cpu, uint8_t idx, uint64_t val);

    // Memory access (mode is backend-specific; see backend headers)
    uint64_t (*read_mem)(Cpu* cpu, uint32_t addr, int mode);
    void     (*write_mem)(Cpu* cpu, uint32_t addr, uint64_t val, int mode);

    // Flags / conditions
    bool (*check_cond)(Cpu* cpu, uint8_t cond);

    // Keyboard input (optional)
    void (*feed_key)(Cpu* cpu, char c);

    // GUI helpers
    int register_count;
    const char* const* register_names;      // array of names, length = register_count
    void (*render_state)(Cpu* cpu);         // ImGui register panel
    void (*render_memory_buttons)(Cpu* cpu, uint32_t* dump_addr, char* buf, size_t buf_size);

    // Common register indices (used by generic GUI/console code)
    uint8_t pc_register;                    // program counter register index
    uint8_t fl_register;                    // flags register index
    uint8_t sp_register;                    // stack pointer register index

    // System parameters
    uint32_t mem_size_default;
    uint32_t vbuffer_base;
    uint16_t vbuffer_cols;
    uint16_t vbuffer_rows;
    uint32_t kbd_ascii_addr;                // 0 if keyboard MMIO is not supported
    uint32_t user_ram_start;                // first address available to user programs
} CpuBackend;

// Common CPU container. Holds universal subsystems (memory, disk, keyboard,
// terminal state) and a pointer to the active backend + its private state.
struct Cpu {
    uint8_t* mem;
    size_t   mem_size;

    bool halted;
    bool irq_enabled;
    bool screen_dirty;
    bool gui_mode;
    bool term_needs_newline;

    // Terminal text buffer state (used by i80148-style text-mode cards)
    uint8_t term_cursor_x;
    uint8_t term_cursor_y;
    uint8_t term_attr;

    // Keyboard input state
    char kbd_buffer[256];
    int  kbd_buffer_len;
    int  kbd_buffer_pos;
    bool kbd_irq_pending;

    // Mouse input state
    int    mouse_x;
    int    mouse_y;
    int    mouse_delta_x;
    int    mouse_delta_y;
    uint8_t mouse_buttons;
    bool   mouse_irq_pending;

    // Disk controller state
    uint8_t   disk_current_drive;
    DiskDrive disk_drives[DISK_MAX_DRIVES];

    // Device class enumeration state.
    int devclass_selected;

    const CpuBackend* backend;
    void*             backend_data;   // backend-specific state (malloc'd)
    void*             pit_data;       // PIT state (malloc'd)
};

// Backend registry. Backends register themselves at startup.
void cpu_backend_register(const CpuBackend* backend);
const CpuBackend* cpu_backend_find(const char* name);
void cpu_backend_print_list(void);
int cpu_backend_count(void);
const CpuBackend* cpu_backend_at(int idx);

// Generic CPU lifecycle
void cpu_init(Cpu* cpu, const CpuBackend* backend, uint8_t* mem, size_t mem_size);
void cpu_reset(Cpu* cpu);
int  cpu_step(Cpu* cpu);
int  cpu_load_file(Cpu* cpu, const char* filename, uint32_t load_addr);

uint64_t cpu_get_reg(Cpu* cpu, uint8_t idx);
void     cpu_set_reg(Cpu* cpu, uint8_t idx, uint64_t val);
uint64_t cpu_read_mem(Cpu* cpu, uint32_t addr, int mode);
void     cpu_write_mem(Cpu* cpu, uint32_t addr, uint64_t val, int mode);
bool     cpu_check_cond(Cpu* cpu, uint8_t cond);
void     cpu_feed_key(Cpu* cpu, char c);

#ifdef __cplusplus
}
#endif

#endif // CPU_API_H
