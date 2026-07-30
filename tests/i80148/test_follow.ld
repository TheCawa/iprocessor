ENTRY(_start)

SECTIONS {
    .text 0x00060000 : { *(.text) }
    .data : { *(.data) }
}
