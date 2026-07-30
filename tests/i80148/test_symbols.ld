ENTRY(_start)
SECTIONS {
    . = 0x00060000;
    .text : { *(.text) }
    __data_start = .;
    .data 0x00080000 : { *(.data) }
    __data_end = .;
}
