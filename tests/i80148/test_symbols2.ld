ENTRY(_start)
SECTIONS {
    . = 0x00060000;
    .text : { *(.text) }
    __data_start = .;
    .data : { *(.data) }
    __data_end = .;
}
