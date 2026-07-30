.entry _start
.extern print_string
.extern print_char

_start:
    LDI.DW IX, msg
    CALL print_string
    LDI.B XL1, '!'
    CALL print_char
    HALT

.data
msg: .DB "Hello, linked world", 10, 0
