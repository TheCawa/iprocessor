; Test: INT triggers a handler, IRET must resume at the instruction AFTER the
; INT opcode (not re-execute INT). If the return is wrong, next_addr prints 1;
; if correct, next_addr prints 2.

.org 0x00060000

.text
main:
    CLI
    LDI.DW SP, 0x000FFF00

    ; IDT: vector 0 -> handler.
    LDI.DW IDTR, 0x00001000
    LDI.DW EX1, handler
    STR.DW EX1, [0x00001000]

    ; next_addr starts as 1 (sentinel).
    LDI.DW EX2, 1
    STR.DW EX2, [next_addr]

    INT 0x00

    ; If IRET returned here we are past the INT: confirm by bumping the value.
    LDI.DW EX2, 2
    STR.DW EX2, [next_addr]
    JMP done

handler:
    IRET

done:
    ; Print the flag byte (0x00000002 expected in EX2 after IRET + store).
    STR.DW EX2, [0x00020018]
    ; Append newline so output is readable.
    LDI.B XL1, 13
    STR.B XL1, [0x00020018]
    HALT

.data
next_addr: .dd 1