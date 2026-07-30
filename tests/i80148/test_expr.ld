ENTRY(_start)
SECTIONS {
    . = 0x00060000;
    .text : { *(.text) }
    .data . + SIZEOF(.text) : { *(.data) }
}
