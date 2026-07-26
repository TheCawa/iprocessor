; Test program for keyboard IRQ, mouse IRQ and mouse MMIO.
; Load address: 0x00050000 (BIOS/CBIOS can load it from disk).
; In the GUI, click on the emulated screen to capture mouse/keyboard,
; then type or move the mouse.

.org 0x00050000
.text

    CLI
    LDI.DW SP, 0x000FFF00

    ; Setup IDT at 0x00001000.
    LDI.DW IDTR, 0x00001000

    ; PIT IRQ vector (0x20) -> dummy handler so timer does not crash us.
    LDI.DW EX1, pit_handler
    STR.DW EX1, [0x00001080]

    ; Keyboard IRQ vector (0x21) -> kbd_handler.
    LDI.DW EX1, kbd_handler
    STR.DW EX1, [0x00001084]

    ; Mouse IRQ vector (0x22) -> mouse_handler.
    LDI.DW EX1, mouse_handler
    STR.DW EX1, [0x00001088]

    STI

main_loop:
    ; Print typed key if the keyboard IRQ set the flag.
    LOD.B XL1, [key_ready]
    CMP.B XL1, 0
    JMP.EQ no_key
    XOR XL1, XL1
    STR.B XL1, [key_ready]
    LOD.B XL1, [last_key]
    STR.B XL1, [0x00020018]
    LDI.B XL1, 0x20        ; ' '
    STR.B XL1, [0x00020018]
no_key:

    ; Print "M:" marker for mouse line.
    LDI.B XL1, 0x4D        ; 'M'
    STR.B XL1, [0x00020018]
    LDI.B XL1, 0x3A        ; ':'
    STR.B XL1, [0x00020018]

    ; Mouse X
    LOD.DW EX1, [mouse_x]
    CALL print_hex_dw
    LDI.B XL1, 0x2C        ; ','
    STR.B XL1, [0x00020018]

    ; Mouse Y
    LOD.DW EX1, [mouse_y]
    CALL print_hex_dw
    LDI.B XL1, 0x2C        ; ','
    STR.B XL1, [0x00020018]

    ; Mouse buttons
    LOD.B XL1, [mouse_buttons]
    CALL print_hex_b

    ; Newline (LF + CR for terminal compatibility).
    LDI.B XL1, 10
    STR.B XL1, [0x00020018]
    LDI.B XL1, 13
    STR.B XL1, [0x00020018]

    ; Simple delay so the screen is readable.
    LDI.DW EX7, 0x00100000
delay:
    DEC EX7
    JMP.NZ delay

    JMA main_loop

; ---------------------------------------------------------------------------
; Print EX1 as 8 hex digits.
; ---------------------------------------------------------------------------
print_hex_dw:
    PUSH EX4
    PUSH EX3
    COPY EX4, EX1
    LDI.DW EX3, 8
phd_loop:
    COPY EX2, EX4
    LSR EX2, 28
    CALL print_hex_nibble
    LSL EX4, 4
    DEC EX3
    JMP.NZ phd_loop
    POP EX3
    POP EX4
    RET

; ---------------------------------------------------------------------------
; Print low nibble of EX2 as one hex digit.
; ---------------------------------------------------------------------------
print_hex_nibble:
    PUSH EX2
    PUSH EX3
    LDI.B XL3, 0x0F
    AND EX2, EX3
    CMP.B EX2, 10
    JMP.GE phn_letter
    ADD.B EX2, 0x30        ; '0'
    JMP phn_out
phn_letter:
    ADD.B EX2, 0x37        ; 'A' - 10
phn_out:
    STR.B EX2, [0x00020018]
    POP EX3
    POP EX2
    RET

; ---------------------------------------------------------------------------
; Print XL1 as 2 hex digits.
; ---------------------------------------------------------------------------
print_hex_b:
    PUSH EX1
    PUSH EX2
    COPY EX1, XL1
    LSL EX1, 4
    CALL print_hex_nibble
    COPY EX1, XL1
    CALL print_hex_nibble
    POP EX2
    POP EX1
    RET

; ---------------------------------------------------------------------------
; Dummy PIT handler: just acknowledge the interrupt.
; ---------------------------------------------------------------------------
pit_handler:
    IRET

; ---------------------------------------------------------------------------
; Keyboard IRQ handler: store ASCII and set flag.
; ---------------------------------------------------------------------------
kbd_handler:
    PUSH EX1
    LOD.B XL1, [0x0002000B]
    STR.B XL1, [last_key]
    LDI.B XL1, 1
    STR.B XL1, [key_ready]
    POP EX1
    IRET

; ---------------------------------------------------------------------------
; Mouse IRQ handler: store coordinates and button state.
; ---------------------------------------------------------------------------
mouse_handler:
    PUSH EX1
    LOD.DW EX1, [0x00020040]
    STR.DW EX1, [mouse_x]
    LOD.DW EX1, [0x00020044]
    STR.DW EX1, [mouse_y]
    LOD.B XL1, [0x00020048]
    STR.B XL1, [mouse_buttons]
    POP EX1
    IRET

.data
key_ready:     .db 0
last_key:      .db 0
mouse_x:       .dd 0
mouse_y:       .dd 0
mouse_buttons: .db 0
