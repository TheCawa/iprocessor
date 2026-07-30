ENTRY(_start)
SECTIONS {
    . = 0x00060001;
    .text ALIGN(0x10) : { *(.text) }
    .data : { *(.data) }
}
