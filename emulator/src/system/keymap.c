// ------------------------------------------------------------------------------
//          keymap.c - Layout-independent keyboard mapping
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

#include "keymap.h"
#include <string.h>

// Physical key positions (SDL_Scancode values, USB HID order).
// Layout-independent: A is always the physical key at the home-row left,
// regardless of what letter the host layout prints on it.
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

char keymap_ascii_from_scancode(int sc) {
    // Letters: physical A..Z -> 'a'..'z' (US home position).
    if (sc >= SC_A && sc <= SC_Z) {
        return (char)('a' + (sc - SC_A));
    }
    // Digit row: physical 1..0 -> '1'..'9','0'.
    if (sc >= SC_1 && sc <= SC_0) {
        if (sc <= SC_9) return (char)('1' + (sc - SC_1));
        return '0';
    }
    switch (sc) {
        case SC_RETURN:        return '\r';
        case SC_ESCAPE:        return 0x1B;
        case SC_BACKSPACE:     return '\b';
        case SC_TAB:           return '\t';
        case SC_SPACE:         return ' ';
        case SC_MINUS:         return '-';
        case SC_EQUALS:        return '=';
        case SC_LEFTBRACKET:   return '[';
        case SC_RIGHTBRACKET:  return ']';
        case SC_BACKSLASH:
        case SC_NONUSHASH:     return '\\';
        case SC_SEMICOLON:     return ';';
        case SC_APOSTROPHE:    return '\'';
        case SC_GRAVE:         return '`';
        case SC_COMMA:         return ',';
        case SC_PERIOD:        return '.';
        case SC_SLASH:         return '/';
        case SC_DELETE:
        case SC_KP_PERIOD:     return 0x7F;
        case SC_UP:            return 0x48;
        case SC_DOWN:          return 0x50;
        case SC_LEFT:          return 0x4B;
        case SC_RIGHT:         return 0x4D;
        default:               return 0;
    }
}

uint8_t keymap_set1_from_scancode(int sc) {
    // Letters: PC set-1 row layout q..p 0x10..0x19, a..l 0x1E..0x26, z..m 0x2C..0x32.
    if (sc >= SC_A && sc <= SC_Z) {
        char c = (char)('a' + (sc - SC_A));
        static const char* rows[3]   = { "qwertyuiop", "asdfghjkl", "zxcvbnm" };
        static const uint8_t bases[3] = { 0x10, 0x1E, 0x2C };
        for (int r = 0; r < 3; r++) {
            const char* p = strchr(rows[r], c);
            if (p && *p) return (uint8_t)(bases[r] + (p - rows[r]));
        }
        return 0;
    }
    // Digit row: 1..0 -> 0x02..0x0B.
    if (sc >= SC_1 && sc <= SC_0) {
        if (sc <= SC_9) return (uint8_t)(0x02 + (sc - SC_1));
        return 0x0B;
    }
    switch (sc) {
        case SC_ESCAPE:        return 0x01;
        case SC_BACKSPACE:     return 0x0E;
        case SC_TAB:           return 0x0F;
        case SC_RETURN:
        case SC_KP_ENTER:      return 0x1C;
        case SC_SPACE:         return 0x39;
        case SC_CAPSLOCK:      return 0x3A;
        case SC_DELETE:
        case SC_KP_PERIOD:     return 0x53;
        case SC_UP:            return 0x48;
        case SC_DOWN:          return 0x50;
        case SC_LEFT:          return 0x4B;
        case SC_RIGHT:         return 0x4D;
        case SC_LSHIFT:        return 0x2A;
        case SC_RSHIFT:        return 0x36;
        case SC_LCTRL:
        case SC_RCTRL:         return 0x1D;
        case SC_LALT:
        case SC_RALT:          return 0x38;
        case SC_LGUI:          return 0x5B;
        case SC_RGUI:          return 0x5C;
        case SC_GRAVE:         return 0x29;
        case SC_MINUS:         return 0x0C;
        case SC_EQUALS:        return 0x0D;
        case SC_LEFTBRACKET:   return 0x1A;
        case SC_RIGHTBRACKET:  return 0x1B;
        case SC_BACKSLASH:
        case SC_NONUSHASH:     return 0x2B;
        case SC_SEMICOLON:     return 0x27;
        case SC_APOSTROPHE:    return 0x28;
        case SC_COMMA:         return 0x33;
        case SC_PERIOD:        return 0x34;
        case SC_SLASH:         return 0x35;
        default:               return 0;
    }
}

bool keymap_is_text_key(int sc) {
    if (sc >= SC_A && sc <= SC_Z) return true;
    if (sc >= SC_1 && sc <= SC_0) return true;
    switch (sc) {
        case SC_MINUS:
        case SC_EQUALS:
        case SC_LEFTBRACKET:
        case SC_RIGHTBRACKET:
        case SC_BACKSLASH:
        case SC_NONUSHASH:
        case SC_SEMICOLON:
        case SC_APOSTROPHE:
        case SC_GRAVE:
        case SC_COMMA:
        case SC_PERIOD:
        case SC_SLASH:
        case SC_SPACE:
            return true;
        default:
            return false;
    }
}

char keymap_apply_shift(char c, bool shift, bool caps) {
    // Letters: Shift XOR Caps Lock (Shift+Caps yields lowercase).
    if (c >= 'a' && c <= 'z') return (shift ^ caps) ? (char)(c - 32) : c;
    if (c >= 'A' && c <= 'Z') return (shift ^ caps) ? (char)(c + 32) : c;
    if (!shift) return c;
    // US-layout shifted punctuation: 4 -> $, , -> <, etc.
    static const char* plain   = "`1234567890-=[]\\;',./";
    static const char* shifted = "~!@#$%^&*()_+{}|:\"<>?";
    const char* p = strchr(plain, c);
    if (p && *p) return shifted[p - plain];
    return c;
}
