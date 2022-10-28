ENTRY(kernel_main)

PHDRS {
    headers PT_PHDR PHDRS;
    code PT_LOAD FILEHDR PHDRS;
    data PT_LOAD;
    uninitialized_data PT_LOAD;
}

SECTIONS {
    . = 1M + SIZEOF_HEADERS;
    .text BLOCK(4K) : ALIGN(4K) { *(.text) } :code 
    .data BLOCK(4K) : ALIGN(4K) { *(.data) } :data
    .bss BLOCK(4K) : ALIGN(4K) { *(.bss) } :uninitialized_data

    _kernel_end = .;
}
