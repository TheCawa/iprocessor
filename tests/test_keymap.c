// test_keymap.c - verify layout-independent key mapping for ALL keys.
//
// Every physical key (SDL_Scancode, layout-independent) must map to the
// correct US-layout ASCII character and PC set-1 scancode, regardless of the
// host OS keyboard layout (Russian, German, ...). This is what fixes keys
// like '<'/',' (Russian 'Б') producing nothing in the terminal.
//
// Build & run (from project root):
//   gcc -Wall -Wextra -std=c11 -Iemulator/src/include tests/test_keymap.c emulator/src/system/keymap.c -o /tmp/test_keymap.exe
//   /tmp/test_keymap.exe

#include "keymap.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Physical key positions (must match SDL_Scancode values used by keymap.c).
enum {
    SC_A = 4, SC_B, SC_C, SC_D, SC_E, SC_F, SC_G, SC_H, SC_I, SC_J, SC_K, SC_L, SC_M,
    SC_N, SC_O, SC_P, SC_Q, SC_R, SC_S, SC_T, SC_U, SC_V, SC_W, SC_X, SC_Y, SC_Z,
    SC_1 = 30, SC_2, SC_3, SC_4, SC_5, SC_6, SC_7, SC_8, SC_9, SC_0,
    SC_RETURN = 40, SC_ESCAPE = 41, SC_BACKSPACE = 42, SC_TAB = 43, SC_SPACE = 44,
    SC_MINUS = 45, SC_EQUALS = 46, SC_LEFTBRACKET = 47, SC_RIGHTBRACKET = 48,
    SC_BACKSLASH = 49, SC_NONUSHASH = 50, SC_SEMICOLON = 51, SC_APOSTROPHE = 52,
    SC_GRAVE = 53, SC_COMMA = 54, SC_PERIOD = 55, SC_SLASH = 56, SC_CAPSLOCK = 57,
    SC_DELETE = 76, SC_RIGHT = 79, SC_LEFT = 80, SC_DOWN = 81, SC_UP = 82,
    SC_KP_ENTER = 88, SC_KP_PERIOD = 99,
    SC_LCTRL = 224, SC_LSHIFT = 225, SC_LALT = 226, SC_LGUI = 227,
    SC_RCTRL = 228, SC_RSHIFT = 229, SC_RALT = 230, SC_RGUI = 231
};

static int failures = 0;

static void check(int cond, const char* what) {
    if (!cond) {
        printf("FAIL: %s\n", what);
        failures++;
    }
}

// --- 1. Letters: full A-Z row, shift/caps logic, set-1 rows ---
static void test_letters(void) {
    const char* rows[3]   = { "qwertyuiop", "asdfghjkl", "zxcvbnm" };
    const uint8_t bases[3] = { 0x10, 0x1E, 0x2C };
    char buf[128];
    for (int sc = SC_A; sc <= SC_Z; sc++) {
        char c = keymap_ascii_from_scancode(sc);
        snprintf(buf, sizeof(buf), "letter sc=%d -> '%c'", sc, c);
        check(c == 'a' + (sc - SC_A), buf);

        uint8_t s1 = keymap_set1_from_scancode(sc);
        int found = 0;
        for (int r = 0; r < 3 && !found; r++) {
            const char* p = strchr(rows[r], c);
            if (p && *p) {
                found = 1;
                snprintf(buf, sizeof(buf), "set1 letter sc=%d -> 0x%02X", sc, s1);
                check(s1 == bases[r] + (p - rows[r]), buf);
            }
        }
        check(found && s1 != 0, "letter has set-1 code");

        snprintf(buf, sizeof(buf), "shift letter sc=%d -> uppercase", sc);
        check(keymap_apply_shift(c, true, false) == (char)(c - 32), buf);
        snprintf(buf, sizeof(buf), "caps letter sc=%d -> uppercase", sc);
        check(keymap_apply_shift(c, false, true) == (char)(c - 32), buf);
        snprintf(buf, sizeof(buf), "shift+caps letter sc=%d -> lowercase", sc);
        check(keymap_apply_shift(c, true, true) == c, buf);
    }
    printf("letters: 26 keys OK\n");
}

// --- 2. Digit row: 1..0 and shifted symbols ---
static void test_digits(void) {
    const char* shifted = "!@#$%^&*()";
    char buf[128];
    for (int sc = SC_1; sc <= SC_0; sc++) {
        char c = keymap_ascii_from_scancode(sc);
        char exp = (sc <= SC_9) ? (char)('1' + (sc - SC_1)) : '0';
        snprintf(buf, sizeof(buf), "digit sc=%d -> '%c'", sc, c);
        check(c == exp, buf);

        uint8_t s1 = keymap_set1_from_scancode(sc);
        uint8_t exp_s1 = (sc <= SC_9) ? (uint8_t)(0x02 + (sc - SC_1)) : 0x0B;
        snprintf(buf, sizeof(buf), "set1 digit sc=%d -> 0x%02X", sc, s1);
        check(s1 == exp_s1, buf);

        snprintf(buf, sizeof(buf), "shift digit sc=%d -> '%c'", sc, shifted[sc - SC_1]);
        check(keymap_apply_shift(c, true, false) == shifted[sc - SC_1], buf);
        // Caps must NOT affect digits.
        snprintf(buf, sizeof(buf), "caps digit sc=%d unchanged", sc);
        check(keymap_apply_shift(c, false, true) == c, buf);
    }
    printf("digits: 10 keys OK (4 -> $, 0 -> ))\n");
}

// --- 3. Punctuation block: explicit expectations ---
static void test_punctuation(void) {
    struct { int sc; char plain; char shifted; uint8_t set1; } tab[] = {
        { SC_GRAVE,        '`', '~', 0x29 },
        { SC_MINUS,        '-', '_', 0x0C },
        { SC_EQUALS,       '=', '+', 0x0D },
        { SC_LEFTBRACKET,  '[', '{', 0x1A },
        { SC_RIGHTBRACKET, ']', '}', 0x1B },
        { SC_BACKSLASH,    '\\', '|', 0x2B },
        { SC_NONUSHASH,    '\\', '|', 0x2B },
        { SC_SEMICOLON,    ';', ':', 0x27 },
        { SC_APOSTROPHE,   '\'', '"', 0x28 },
        { SC_COMMA,        ',', '<', 0x33 },
        { SC_PERIOD,       '.', '>', 0x34 },
        { SC_SLASH,        '/', '?', 0x35 },
        { SC_SPACE,        ' ', ' ', 0x39 },
    };
    char buf[128];
    for (size_t i = 0; i < sizeof(tab) / sizeof(tab[0]); i++) {
        snprintf(buf, sizeof(buf), "punct sc=%d plain '%c'", tab[i].sc, tab[i].plain);
        check(keymap_ascii_from_scancode(tab[i].sc) == tab[i].plain, buf);
        snprintf(buf, sizeof(buf), "punct sc=%d shifted '%c'", tab[i].sc, tab[i].shifted);
        check(keymap_apply_shift(tab[i].plain, true, false) == tab[i].shifted, buf);
        snprintf(buf, sizeof(buf), "punct sc=%d set1 0x%02X", tab[i].sc, tab[i].set1);
        check(keymap_set1_from_scancode(tab[i].sc) == tab[i].set1, buf);
    }
    printf("punctuation: 13 keys OK (Б-key -> ',' / '<')\n");
}

// --- 4. Special / control keys ---
static void test_special(void) {
    struct { int sc; char ascii; uint8_t set1; } tab[] = {
        { SC_RETURN,    '\r', 0x1C },
        { SC_KP_ENTER,  0,    0x1C },
        { SC_ESCAPE,    0x1B, 0x01 },
        { SC_BACKSPACE, '\b', 0x0E },
        { SC_TAB,       '\t', 0x0F },
        { SC_DELETE,    0x7F, 0x53 },
        { SC_KP_PERIOD, 0x7F, 0x53 },
        { SC_UP,        0x48, 0x48 },
        { SC_DOWN,      0x50, 0x50 },
        { SC_LEFT,      0x4B, 0x4B },
        { SC_RIGHT,     0x4D, 0x4D },
        { SC_CAPSLOCK,  0,    0x3A },
        { SC_LSHIFT,    0,    0x2A },
        { SC_RSHIFT,    0,    0x36 },
        { SC_LCTRL,     0,    0x1D },
        { SC_RCTRL,     0,    0x1D },
        { SC_LALT,      0,    0x38 },
        { SC_RALT,      0,    0x38 },
        { SC_LGUI,      0,    0x5B },
        { SC_RGUI,      0,    0x5C },
    };
    char buf[128];
    for (size_t i = 0; i < sizeof(tab) / sizeof(tab[0]); i++) {
        snprintf(buf, sizeof(buf), "special sc=%d ascii 0x%02X", tab[i].sc, (unsigned char)tab[i].ascii);
        check((unsigned char)keymap_ascii_from_scancode(tab[i].sc) == (unsigned char)tab[i].ascii, buf);
        snprintf(buf, sizeof(buf), "special sc=%d set1 0x%02X", tab[i].sc, tab[i].set1);
        check(keymap_set1_from_scancode(tab[i].sc) == tab[i].set1, buf);
    }
    // Special/control keys must NOT be "text keys": their ASCII codes
    // (0x48='H', 0x50='P') collide with letters, so Shift must never be applied.
    int non_text[] = { SC_RETURN, SC_KP_ENTER, SC_ESCAPE, SC_BACKSPACE, SC_TAB,
                       SC_DELETE, SC_KP_PERIOD, SC_UP, SC_DOWN, SC_LEFT, SC_RIGHT,
                       SC_CAPSLOCK, SC_LSHIFT, SC_RSHIFT, SC_LCTRL, SC_RCTRL,
                       SC_LALT, SC_RALT, SC_LGUI, SC_RGUI };
    char buf2[128];
    for (size_t i = 0; i < sizeof(non_text) / sizeof(non_text[0]); i++) {
        snprintf(buf2, sizeof(buf2), "sc=%d is not a text key", non_text[i]);
        check(!keymap_is_text_key(non_text[i]), buf2);
    }
    // apply_shift itself leaves control bytes alone (0x7F is not a letter/punct).
    check(keymap_apply_shift('\r', true, false) == '\r', "shift \\r unchanged");
    check(keymap_apply_shift(0x7F, true, false) == 0x7F, "shift DEL unchanged");
    printf("special/control: 20 keys OK\n");
}

// --- 5. Unmapped scancodes return 0 ---
static void test_unmapped(void) {
    int unmapped[] = { 0, 1, 58, 59, 70, 100, 150, 200, 223, 232, 300 };
    char buf[128];
    for (size_t i = 0; i < sizeof(unmapped) / sizeof(unmapped[0]); i++) {
        snprintf(buf, sizeof(buf), "unmapped sc=%d -> ascii 0", unmapped[i]);
        check(keymap_ascii_from_scancode(unmapped[i]) == 0, buf);
        snprintf(buf, sizeof(buf), "unmapped sc=%d -> set1 0", unmapped[i]);
        check(keymap_set1_from_scancode(unmapped[i]) == 0, buf);
    }
    printf("unmapped: %d keys OK\n", (int)(sizeof(unmapped) / sizeof(unmapped[0])));
}

// --- 6. Full US-layout round trip over the whole scancode space ---
// Every printable ASCII (0x20..0x7E) must be reachable from some key, either
// unshifted or shifted. The only allowed duplicate is '\\' (0x5C), produced by
// both SC_BACKSLASH and the ISO SC_NONUSHASH key.
static void test_roundtrip(void) {
    char seen[128] = {0};
    int count = 0;
    for (int sc = 0; sc < 240; sc++) {
        if (keymap_is_text_key(sc)) {
            char c = keymap_ascii_from_scancode(sc);
            if (c >= 0x20 && c <= 0x7E) {
                if (seen[(unsigned char)c] && c != '\\') {
                    printf("FAIL: duplicate printable char '%c' (0x%02X) from sc=%d\n", c, c, sc);
                    failures++;
                }
                seen[(unsigned char)c] = 1;
                count++;
            }
            // Every text key must also reach its shifted variant.
            char s = keymap_apply_shift(c, true, false);
            seen[(unsigned char)s] = 1;
        }
    }
    int expected = 0;
    for (int c = 0x20; c <= 0x7E; c++) {
        if (!seen[c]) {
            printf("FAIL: printable char 0x%02X '%c' unreachable from any key\n", c, c);
            failures++;
        } else {
            expected++;
        }
    }
    printf("roundtrip: %d unique printable chars reachable (%d/%d of ASCII set)\n",
           count, expected, 0x7E - 0x20 + 1);
    check(count == 49, "49 text keys mapped (26 letters + 10 digits + 13 punct/space)");
    check(expected == 95, "all 95 printable ASCII chars reachable (with shift)");
}

int main(void) {
    printf("=== test_keymap ===\n");
    test_letters();
    test_digits();
    test_punctuation();
    test_special();
    test_unmapped();
    test_roundtrip();

    if (failures) {
        printf("RESULT: %d FAILURE(S)\n", failures);
        return 1;
    }
    printf("RESULT: ALL OK\n");
    return 0;
}
