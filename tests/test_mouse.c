// test_mouse.c - verify mouse coordinates follow the active resolution.
//
// Bug being tested: MOUSE_X/Y were hard-clamped to 320x200 (MOUSE_MAX_X/Y),
// so coordinates beyond 319x199 were unreachable in 640x480 / 800x600 modes.
// Fix: bounds now follow TERM_RES_X/Y (pixels in gfx modes, char grid in text
// modes), and MOUSE_X/Y are writable (clamped on write).
//
// Build & run (from project root):
//   gcc -Wall -Wextra -std=c11 -Iemulator/src/include tests/test_mouse.c emulator/src/system/input.c emulator/src/system/term_res.c -o /tmp/test_mouse.exe
//   /tmp/test_mouse.exe

#include "input.h"
#include "term_res.h"
#include "cpu_api.h"
#include <stdio.h>
#include <string.h>

#define VC_MODE_ADDR 0x0002001A

static int failures = 0;

static void check(int cond, const char* what) {
    if (!cond) {
        printf("FAIL: %s\n", what);
        failures++;
    }
}

// Fresh machine: 64KB RAM, resolution synced from VC_MODE like the real backends.
static void make_cpu(Cpu* cpu, uint8_t* ram, size_t ram_size, uint8_t vc_mode, bool with_sync) {
    memset(cpu, 0, sizeof(*cpu));
    memset(ram, 0, ram_size);
    cpu->mem = ram;
    cpu->mem_size = (uint32_t)ram_size;
    ram[VC_MODE_ADDR] = vc_mode;
    cpu->update_term_res = with_sync ? term_res_update : NULL;
    // Backend default like cpu_init_i80148.
    cpu->term_res_x = 80;
    cpu->term_res_y = 25;
    input_init(cpu);
}

int main(void) {
    printf("=== test_mouse ===\n");

    uint8_t ram[0x40000];
    Cpu cpu;
    char buf[128];

    // --- 1. Default 80x25 text mode: bounds = 79x24, init centered ---
    make_cpu(&cpu, ram, sizeof(ram), 0x00, true);
    check(cpu.mouse_x == 39 && cpu.mouse_y == 12, "init center in 80x25 = (39,12)");
    input_mouse_move(&cpu, 10000, 10000);
    check(input_read_dword(&cpu, MOUSE_X_ADDR) == 79, "80x25: X clamped to 79");
    check(input_read_dword(&cpu, MOUSE_Y_ADDR) == 24, "80x25: Y clamped to 24");
    input_mouse_move(&cpu, -100000, -100000);
    check(input_read_dword(&cpu, MOUSE_X_ADDR) == 0, "80x25: X clamped to 0");
    check(input_read_dword(&cpu, MOUSE_Y_ADDR) == 0, "80x25: Y clamped to 0");

    // --- 2. 640x480 graphics (mode 0x21): coordinates must exceed 320x200 ---
    make_cpu(&cpu, ram, sizeof(ram), 0x21, true);
    check(cpu.mouse_x == 319 && cpu.mouse_y == 239, "init center in 640x480 = (319,239)");
    input_mouse_move(&cpu, 10000, 10000);
    check(input_read_dword(&cpu, MOUSE_X_ADDR) == 639, "640x480: X reaches 639");
    check(input_read_dword(&cpu, MOUSE_Y_ADDR) == 479, "640x480: Y reaches 479");
    snprintf(buf, sizeof(buf), "640x480: X read via byte = low byte %d", 639 & 0xFF);
    check(input_read_byte(&cpu, MOUSE_X_ADDR) == (uint8_t)(639 & 0xFF), buf);

    // --- 3. 800x600 graphics (mode 0x22) ---
    make_cpu(&cpu, ram, sizeof(ram), 0x22, true);
    input_mouse_move(&cpu, 10000, 10000);
    check(input_read_dword(&cpu, MOUSE_X_ADDR) == 799, "800x600: X reaches 799");
    check(input_read_dword(&cpu, MOUSE_Y_ADDR) == 599, "800x600: Y reaches 599");

    // --- 4. 320x200 legacy graphics (mode 0x01): old behavior preserved ---
    make_cpu(&cpu, ram, sizeof(ram), 0x01, true);
    input_mouse_move(&cpu, 10000, 10000);
    check(input_read_dword(&cpu, MOUSE_X_ADDR) == 319, "320x200: X clamped to 319");
    check(input_read_dword(&cpu, MOUSE_Y_ADDR) == 199, "320x200: Y clamped to 199");

    // --- 5. MOUSE_X/Y are writable (clamped to resolution) ---
    make_cpu(&cpu, ram, sizeof(ram), 0x21, true);
    input_write_dword(&cpu, MOUSE_X_ADDR, 500);
    input_write_dword(&cpu, MOUSE_Y_ADDR, 400);
    check(input_read_dword(&cpu, MOUSE_X_ADDR) == 500, "write MOUSE_X=500 -> 500");
    check(input_read_dword(&cpu, MOUSE_Y_ADDR) == 400, "write MOUSE_Y=400 -> 400");
    input_write_dword(&cpu, MOUSE_X_ADDR, 900);
    input_write_dword(&cpu, MOUSE_Y_ADDR, 1000);
    check(input_read_dword(&cpu, MOUSE_X_ADDR) == 639, "write MOUSE_X=900 clamps to 639");
    check(input_read_dword(&cpu, MOUSE_Y_ADDR) == 479, "write MOUSE_Y=1000 clamps to 479");
    // Signed interpretation: 0xFFFFFFFF = -1 -> clamps to 0.
    input_write_dword(&cpu, MOUSE_X_ADDR, 0xFFFFFFFF);
    check(input_read_dword(&cpu, MOUSE_X_ADDR) == 0, "write 0xFFFFFFFF (-1) clamps to 0");

    // Byte write patches the low byte (like other 32-bit MMIO registers).
    input_write_dword(&cpu, MOUSE_X_ADDR, 0x100);
    input_write_byte(&cpu, MOUSE_X_ADDR, 0x2F);
    check(input_read_dword(&cpu, MOUSE_X_ADDR) == 0x12F, "byte write patches low byte (0x100|0x2F=0x12F)");

    // --- 6. Resolution shrink re-clamps on read ---
    make_cpu(&cpu, ram, sizeof(ram), 0x21, true);
    input_mouse_move(&cpu, 10000, 10000);  // (639,479)
    ram[VC_MODE_ADDR] = 0x00;              // switch to 80x25 text
    check(input_read_dword(&cpu, MOUSE_X_ADDR) == 79, "after mode shrink X re-clamps to 79");
    check(input_read_dword(&cpu, MOUSE_Y_ADDR) == 24, "after mode shrink Y re-clamps to 24");

    // --- 7. MOUSE_BTN writable + button feed ---
    make_cpu(&cpu, ram, sizeof(ram), 0x00, true);
    input_write_byte(&cpu, MOUSE_BTN_ADDR, 0x03);
    check(input_read_byte(&cpu, MOUSE_BTN_ADDR) == 0x03, "write MOUSE_BTN=3 -> 3");
    input_mouse_button(&cpu, 2, true);  // middle = 0x04
    check(input_read_byte(&cpu, MOUSE_BTN_ADDR) == 0x07, "button feed ORs into buttons (3|4=7)");
    input_mouse_button(&cpu, 1, false); // release left
    check(input_read_byte(&cpu, MOUSE_BTN_ADDR) == 0x06, "button release clears bit (7&~1=6)");

    // --- 8. Deltas writable ---
    input_write_dword(&cpu, MOUSE_DELTA_X_ADDR, 1234);
    check(input_read_dword(&cpu, MOUSE_DELTA_X_ADDR) == 1234, "write DELTA_X=1234 -> 1234");
    input_write_byte(&cpu, MOUSE_DELTA_Y_ADDR, 9);
    check(input_read_dword(&cpu, MOUSE_DELTA_Y_ADDR) == 9, "write byte DELTA_Y=9 -> 9");

    // --- 9. MOUSE_SENS: sensitivity scales relative deltas (default = 1.0) ---
    make_cpu(&cpu, ram, sizeof(ram), 0x00, true);
    check(input_read_dword(&cpu, MOUSE_SENS_ADDR) == MOUSE_SENS_DEFAULT,
          "MOUSE_SENS default = 0x100 (1.0)");

    // Default 1.0: moves stay unchanged.
    memset(&cpu, 0, sizeof(cpu));  // recenter via explicit init below
    cpu.term_res_x = 80;
    cpu.term_res_y = 25;
    cpu.update_term_res = term_res_update;
    ram[VC_MODE_ADDR] = 0x00;
    cpu.mem = ram;
    cpu.mem_size = (uint32_t)sizeof(ram);
    input_mouse_move(&cpu, 1, 1);   // from (0,0): stays 1:1 with sens=default
    input_mouse_move(&cpu, 1, 1);
    check(input_read_dword(&cpu, MOUSE_X_ADDR) == 2, "sens=1.0: dx accumulates normally");
    check(input_read_dword(&cpu, MOUSE_DELTA_X_ADDR) == 2, "sens=1.0: delta accumulates normally");

    // Sens 0x200 (2.0): doubles movement.
    input_write_dword(&cpu, MOUSE_SENS_ADDR, 0x200);
    check(input_read_dword(&cpu, MOUSE_SENS_ADDR) == 0x200, "write MOUSE_SENS=0x200 -> 0x200");
    input_mouse_move(&cpu, 5, 0);
    check(input_read_dword(&cpu, MOUSE_X_ADDR) == 2 + 10, "sens=2.0: dx=5 doubles to 10");
    check(input_read_dword(&cpu, MOUSE_DELTA_X_ADDR) == 2 + 10, "sens=2.0: delta doubles");

    // Sens 0x80 (0.5): halves, truncating toward zero.
    input_write_dword(&cpu, MOUSE_SENS_ADDR, 0x80);
    input_write_dword(&cpu, MOUSE_X_ADDR, 40);   // reset pointer position
    input_write_dword(&cpu, MOUSE_DELTA_X_ADDR, 0);
    input_mouse_move(&cpu, 5, 0);
    check(input_read_dword(&cpu, MOUSE_X_ADDR) == 40 + 2, "sens=0.5: dx=5 halves to 2");

    // Sens 0: treated as default (1.0).
    input_write_dword(&cpu, MOUSE_SENS_ADDR, 0);
    input_write_dword(&cpu, MOUSE_X_ADDR, 0);
    input_write_dword(&cpu, MOUSE_DELTA_X_ADDR, 0);
    input_mouse_move(&cpu, 5, 0);
    check(input_read_dword(&cpu, MOUSE_X_ADDR) == 5, "sens=0 treated as 1.0");

    // Byte write patches the low byte of the sensitivity register.
    input_write_dword(&cpu, MOUSE_SENS_ADDR, 0x300);
    input_write_byte(&cpu, MOUSE_SENS_ADDR, 0x01);
    check(input_read_dword(&cpu, MOUSE_SENS_ADDR) == 0x301, "byte write patches low byte of SENS");

    // --- 10. No resolution available: fallback to legacy 320x200 bounds ---
    memset(&cpu, 0, sizeof(cpu));  // update_term_res = NULL, term_res_x/y = 0
    cpu.mem = ram;
    cpu.mem_size = (uint32_t)sizeof(ram);
    input_init(&cpu);
    check(cpu.mouse_x == 159 && cpu.mouse_y == 99, "fallback init center = (159,99)");
    input_mouse_move(&cpu, 10000, 10000);
    check(input_read_dword(&cpu, MOUSE_X_ADDR) == 319, "fallback: X clamped to 319");
    check(input_read_dword(&cpu, MOUSE_Y_ADDR) == 199, "fallback: Y clamped to 199");

    // --- 11. NULL cpu safety ---
    input_mouse_move(NULL, 1, 1);
    input_write_dword(NULL, MOUSE_X_ADDR, 5);
    input_read_dword(NULL, MOUSE_X_ADDR);
    check(1, "NULL cpu calls are safe");

    if (failures) {
        printf("RESULT: %d FAILURE(S)\n", failures);
        return 1;
    }
    printf("RESULT: ALL OK\n");
    return 0;
}
