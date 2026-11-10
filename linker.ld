ENTRY(_start)
OUTPUT_FORMAT("binary")
SECTIONS {
    . = 0x1000;
    .text : {
        kernel_entry.o(.text)
        *(.text)
    }
    .data : { *( .data ) }
    .rodata : { *( .rodata ) }
    .bss : { *( .bss ) }
}
