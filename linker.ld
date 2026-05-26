OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(reset_vector)

SECTIONS
{
    /* База ROM (128 KB) */
    . = 0xFFFE0000;
    
    .text : { *(.text) }
    .rodata : { *(.rodata*) }
    .data : { *(.data) }
    .bss : { *(.bss) *(COMMON) }

    /* Бутблок - последние 256 байт */
    . = 0xFFFFFF00;
    .boot : { *(.boot) }
}