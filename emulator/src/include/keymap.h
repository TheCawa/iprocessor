// ------------------------------------------------------------------------------
//          keymap.h - Layout-independent keyboard mapping
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

#ifndef KEYMAP_H
#define KEYMAP_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// The emulator keyboard is defined by PHYSICAL key position, not by the host
// OS keyboard layout. Physical positions use the SDL_Scancode values (USB HID
// order, layout-independent): the key that prints '<' on a US layout always
// prints '<' here, even when the host layout is Russian/German/etc.
// The scancode values are hard-coded below (SDL 2.x constants) so this module
// does not depend on SDL headers.

// Map a physical scancode to the unshifted US-layout ASCII character.
// Special keys map to control bytes: \r \b \t 0x1B(ESC) 0x7F(DEL),
// arrows 0x48/0x50/0x4B/0x4D. Returns 0 for unmapped keys.
char keymap_ascii_from_scancode(int scancode);

// Map a physical scancode to a PC set-1 scancode. Returns 0 for unmapped.
uint8_t keymap_set1_from_scancode(int scancode);

// True if the physical key is a typeable text key (letters, digits,
// punctuation, space). Only these honor Shift/Caps; control keys (arrows,
// DEL, Enter, ...) must pass through unshifted even though their ASCII
// codes (e.g. 0x48 = 'H') collide with printable letters.
bool keymap_is_text_key(int scancode);

// Apply Shift/Caps to a US-layout character:
//  - letters: Shift XOR Caps (Shift+Caps produces lowercase),
//  - punctuation: Shift swaps to the shifted symbol (4 -> $, , -> <),
//  - everything else (digits already handled, control bytes) is unchanged.
char keymap_apply_shift(char c, bool shift, bool caps);

#ifdef __cplusplus
}
#endif

#endif // KEYMAP_H
