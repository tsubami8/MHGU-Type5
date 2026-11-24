OUTPUT_FORMAT(elf32-littlearm)
OUTPUT_ARCH(arm)

PHDRS {
    text PT_LOAD FLAGS(5);
    rodata PT_LOAD FLAGS(4);
    data PT_LOAD FLAGS(6);
    dynamic PT_DYNAMIC FLAGS(6);
}

SECTIONS {
    . = 0;
    .text : {
        LONG(0);
        LONG(module_header);
        . = 0x80;
        *(.text .text.*)
    } :text
    .plt : { *(.plt .plt.*) } :text

    . = ALIGN(0x1000);
    .rodata : {
        KEEP(*(.module_name))
        . = ALIGN(4);
        HIDDEN(module_header = .);
        LONG(0x30444f4d);
        LONG(ADDR(.dynamic) - module_header);
        LONG(ADDR(.bss) - module_header);
        LONG(bss_end - module_header);
        LONG(0); LONG(0);
        LONG(module_object - module_header);
        *(.rodata .rodata.*)
    } :rodata
    .hash : { *(.hash) } :rodata
    .dynsym : { *(.dynsym) } :rodata
    .dynstr : { *(.dynstr) } :rodata
    .rel.dyn : { *(.rel.dyn) } :rodata
    .rel.plt : { *(.rel.plt) } :rodata

    . = ALIGN(0x1000);
    .data : { *(.data .data.*) *(.data.rel.ro .data.rel.ro.*) } :data
    .got : { *(.got .got.*) } :data
    .dynamic : { *(.dynamic) } :data :dynamic
    . = ALIGN(0x1000);
    .bss (NOLOAD) : {
        HIDDEN(module_object = .);
        . += 0x100;
        *(.bss .bss.* COMMON)
        . = ALIGN(0x1000);
        HIDDEN(bss_end = .);
    } :data

    /DISCARD/ : { *(.ARM.exidx*) *(.ARM.extab*) *(.comment) *(.eh_frame*) }
}
