ENTRY(_start)
SECTIONS {
    . = 0x00060000;
    .text ALIGN(0x1000) : { *(.text) }
    .data : { *(.data) }
}
