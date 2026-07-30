.entry _start
.extern print_string
.extern __data_start

_start:
    LDI.DW IX, __data_start
    CALL print_string
    HALT

.data
ok_msg:
    .db "ok", 10, 0
