PHDRS {
    code PT_LOAD;
}

SECTIONS {
    . = 0x7c00;
    .text : { *(.text) } :code  
}
