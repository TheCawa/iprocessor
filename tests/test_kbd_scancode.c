// test_kbd_scancode.c - verify kbd scancode FIFO, ascii/scancode sync and modifiers.
#include "cpu_api.h"
#include "input.h"
#include "system.h"
#include <stdio.h>

static int g_fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL: %s\n", msg); g_fail++; } } while (0)

int main(void) {
    Cpu cpu;
    for (int i = 0; i < 256; i++) { cpu.kbd_buffer[i] = 0; cpu.kbd_scancodes[i] = 0; }
    cpu.kbd_buffer_len = 0;
    cpu.kbd_buffer_pos = 0;
    cpu.gui_mode = true;
    cpu.kbd_irq_pending = false;

    input_init(&cpu);

    /* Feed a few keys with scancodes. */
    input_feed_key_ex(&cpu, 'A', 0x1E);   /* a -> scancode 0x1E */
    input_feed_key_ex(&cpu, '$', 0x05);   /* 4 -> 0x05 */
    input_feed_key_ex(&cpu, 0x0D, 0x1C);  /* CR, Enter -> 0x1C */

    /* SCANCODE peeks the head slot, ASCII pops it - they must pair up. */
    CHECK(kbd_read_scancode(&cpu) == 0x1E, "first scancode == 0x1E");
    CHECK(kbd_read_ascii(&cpu) == 'A', "first ascii == 'A'");
    CHECK(kbd_read_scancode(&cpu) == 0x05, "second scancode == 0x05");
    CHECK(kbd_read_ascii(&cpu) == '$', "second ascii == '$'");
    CHECK(kbd_read_scancode(&cpu) == 0x1C, "third scancode == 0x1C (Enter)");
    CHECK(kbd_read_ascii(&cpu) == 0x0D, "third ascii == CR");
    CHECK(kbd_read_scancode(&cpu) == 0, "empty scancode == 0");
    CHECK(kbd_read_ascii(&cpu) == 0, "empty ascii == 0");

    /* After full drain the buffer resets (regression from the 255-char bug). */
    CHECK(cpu.kbd_buffer_len == 0, "buffer len reset after drain");

    /* Peek must have consumed nothing: feed again and pair. */
    input_feed_key_ex(&cpu, 'X', 0x10);   /* q */
    CHECK(kbd_read_scancode(&cpu) == 0x10, "q scancode == 0x10");
    CHECK(kbd_read_ascii(&cpu) == 'X', "q ascii == 'X'");

    /* Modifier latch. */
    input_set_modifiers(&cpu, KBD_MOD_SHIFT | KBD_MOD_CTRL);
    CHECK(cpu.kbd_modifiers == (KBD_MOD_SHIFT | KBD_MOD_CTRL), "modifier latch set");
    cpu.kbd_tab_down = true;
    input_set_modifiers(&cpu, KBD_MOD_TAB | KBD_MOD_SHIFT);
    CHECK(cpu.kbd_modifiers == (KBD_MOD_TAB | KBD_MOD_SHIFT), "modifier with tab");
    input_set_modifiers(&cpu, 0);
    CHECK(cpu.kbd_modifiers == 0, "modifier cleared");

    if (g_fail == 0) printf("ALL OK: kbd scancode/modifier tests\n");
    return g_fail ? 1 : 0;
}