[bits 16]

;---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
; Bootloader Stage 0
;---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

start: jmp 0x0000:stage0_start

; DATA
boot0_msg db "Stage 0 started", 0x0a, 0x0d, 0x00
boot0_reset_error db "Failed to reset floppy disk to track 0", 0x0a, 0x0d, 0x00
;boot0_reset_okay db "reset okay", 0x0a, 0x0d, 0x00
boot0_floppy_error db "Failed to read stage 1 bootloader from floppy disk", 0x0a, 0x0d, 0x00

BOOT1_SECTOR_COUNT equ 4

STACK_BOTTOM equ 0x500
STACK_TOP equ 0x6ff0

CURRENT_CYLINDER_ADDR equ 0x6ff4
CURRENT_HEAD_ADDR equ 0x6ff5
CURRENT_SECTOR_ADDR equ 0x6ff6

CYLINDER_COUNT equ 80
HEAD_COUNT equ 2
SECTOR_COUNT equ 18

SECTOR_SIZE equ 0x200

FLOPPY_READ_ATTEMPT_COUNT equ 3

STAGE1_LOAD_ADDR equ 0x7e00

; CODE

; Returns in eax ceil(eax / %1)
%macro ceil_div 1

    xor edx, edx            ; Setup edx for division
    mov ecx, %1             ; Need to use a register for division
    div ecx                 ; Divide eax by %1
    cmp edx, 0              ; Check if remainder is 0
    je %%ceil_div_end       ; If not then end

    add eax, 1              ; Otherwise add 1 to result

    %%ceil_div_end:

%endmacro

stage0_start:
    cli         ; Clear interrupts

    ; Set segment registers to 0
    mov ax, 0
    mov ds, ax  ; Data segment register
    mov es, ax  ; Extra segment register
    mov ss, ax  ; Stack segment register
    mov fs, ax  ; f segment register
    mov gs, ax  ; g segment register

    ; Setup stack
    mov sp, STACK_TOP
    mov bp, sp

    mov si, boot0_msg
    call print_string

    ; Reset current stored cylinder, head, and sector positions to sector after boot sector
    mov byte [CURRENT_CYLINDER_ADDR], 0
    mov byte [CURRENT_HEAD_ADDR], 0
    mov byte [CURRENT_SECTOR_ADDR], 2

    ; int 13h - disk - reset disk system
    xor ah, ah                  ; to reset disk system (seek to track 0)
    xor dl, dl                  ; drive number 0

    clc
    int 0x13
    
    jnc boot0_reset_success

    ; On failure print error message and halt
    mov si, boot0_reset_error
    call print_string
    hlt

    boot0_reset_success:
    ;mov si, boot0_reset_okay
    ;call_print_string

    mov bx, STAGE1_LOAD_ADDR                ; Set buffer location to STAGE1_LOAD_ADDR (right after the boot sector)
    call read_next_floppy_sector_repeated   ; Read stage 1 meta data from floppy
    jc boot0_floppy_fail

    mov bx, STAGE1_LOAD_ADDR        ; Set meta data location to newly loaded sector
    call ustar_read_file_size       ; Get size of stage 1 bootloader

    ceil_div SECTOR_SIZE            ; Get the number of sectors to read

    mov bx, STAGE1_LOAD_ADDR        ; Set location for stage 1 to be loaded to
    call read_sectors               ; Read sectors from floppy
    jc boot0_floppy_fail

    jmp boot1_start                 ; Jump to stage 1

    boot0_floppy_fail:

    ; If failed to load from floppy then print error message and stop
    mov si, boot0_floppy_error
    call print_string

    hlt
    
; Prints a character on screen at the current cursor position
; Arguments: al - character to print, bl - text colour, cx - number of times character is repeated
put_char:
    ; int 10h(0Eh, al, 0, bl, 1) - TELETYPE OUTPUT

    mov ah, 0x0E    ; Set to 0Ah for writing character
    ; mov al, al    ; Set character to display
    xor bh, bh      ; Set page number to 0
    ; mov bl, bl    ; Set colour of text
    mov ecx, 1      ; Set number of times to repeat the character to 1
    int 0x10        ; Call the BIOS interrupt

    ret

; Prints a string on screen at the current cursor position updating the cursor position to the end of the string
; Arguments: ds:si - Zero-terminated string
print_string:
    ps_loop_start:
        mov byte al, byte [ds:si]   ; Load character from memory
        cmp al, 0                   ; Compare character to 0
        je ps_loop_end              ; If is 0 then exit loop

        ; put_char(al, 0, 1)
        ; mov al, al
        xor bl, bl
        mov ecx, 1
        call put_char

        add si, 1                   ; Increment address
        jmp ps_loop_start
    ps_loop_end:
    ret

; Reads a sector from a floppy disk to a buffer
; Arguments: [es:bx] buffer address
read_next_floppy_sector:
    push cx

    ; int 13h - disk - read sectors into memory
    mov ah, 2                                   ; for reading sectors
    mov al, 1                                   ; number of sectors to read
    mov ch, byte [CURRENT_CYLINDER_ADDR]        ; load current cylinder
    mov cl, byte [CURRENT_SECTOR_ADDR]          ; load current sector
    mov dh, byte [CURRENT_HEAD_ADDR]            ; load current head
    xor dl, dl                                  ; drive number 0
    ; mov bx, bx                                ; buffer location

    clc                                         ; Used as error signifier so clear before interrupt
    int 0x13                                    ; Read from disk

    jc rnfs_done                                ; If there was an error then we don't want to update position

    call increment_floppy_position

    rnfs_done:
    pop cx
    ret

; Attempts to read next sector N times before returning an error
; Arguments: [es:bx] buffer address
read_next_floppy_sector_repeated:
    ; Save preserved registers
    push cx

    ; Initialize loop counter
    mov cx, FLOPPY_READ_ATTEMPT_COUNT
    rnfsr_loop_start:
        cmp cx, 0                               ; loop condition
        je rnfsr_loop_end                       ; exit if true

        push bx                                 ; save buffer address to stack
        call read_next_floppy_sector            ; attempt to read sector
        pop bx                                  ; restore buffer address from stack

        jnc rnfsr_loop_end                      ; if there wasnt an error then we can stop

        dec cx                                  ; decrement loop counter
        jmp rnfsr_loop_start                    ; loop
    rnfsr_loop_end:
    pop cx
    ret

; Updates the current cylinder, sector, and head values to refer to the next sector
; Arguments: none
increment_floppy_position:
    mov al, byte [CURRENT_SECTOR_ADDR]
    cmp al, SECTOR_COUNT                ; Compare current sector to 18
    jae ifp_update_head                 ; If >= SECTOR_COUNT then need to update head (no -1 since 1-18 inclusive indexing)

    add al, 1
    mov byte [CURRENT_SECTOR_ADDR], al  ; Otherwise increment sector number by 1 and return
    ret
    
    ifp_update_head:
    mov byte [CURRENT_SECTOR_ADDR], 1   ; Reset sector to 1 (0 is invalid)

    mov al, byte [CURRENT_HEAD_ADDR]
    cmp al, HEAD_COUNT - 1
    jae ifp_update_cylinder             ; If >= HEAD_COUNT - 1 then need to update cylinder

    add al, 1
    mov byte [CURRENT_HEAD_ADDR], al    ; Otherwise set head to 1 and return
    ret

    ifp_update_cylinder:
    mov byte [CURRENT_HEAD_ADDR], 0     ; Reset head to 0

    mov al, byte [CURRENT_HEAD_ADDR]
    cmp al, CYLINDER_COUNT              ; Compare current cylinder to CYLINDER_COUNT - 1
    jae ifp_reset_cylinder              ; If >= CYLINDER_COUNT - 1 then need to reset cylinder to 0

    add al, 1
    mov byte [CURRENT_CYLINDER_ADDR], al ; Otherwise increment cylinder number by 1 and return
    ret

    ifp_reset_cylinder:
    mov byte [CURRENT_CYLINDER_ADDR], 0 ; Reset cylinder to 0
    ret

; Reads n sectors from the floppy disk
; Arguments: [es:bx] buffer location, ax number of sectors to load
; On error carry flag is set
read_sectors:
    mov cx, ax
    rs_loop_start:
        cmp cx, 0           ; Compare loop counter to 0
        je rs_loop_end      ; If = 0 then exit loop

        push cx
        push ax
        push bx
        call read_next_floppy_sector_repeated
        pop bx
        pop ax
        pop cx

        jc rs_loop_end      ; If there was an error then exit loop

        dec cx              ; Decrement loop counter
        add bx, SECTOR_SIZE ; Move bx to end of loaded sector
        jmp rs_loop_start   ; Loop
    rs_loop_end:
    ret

; Reads the file size from the ustar meta data provided
; Return eax as the file size
; Arguments: meta data address [es:bx]
; TODO: check for invalid characters in file size
ustar_read_file_size:
    add bx, 0x7c            ; Add offset to file size's last character
    xor cx, cx              ; Reset loop counter to 0
    xor eax, eax            ; Set result to 0
    urfs_loop_start:
        cmp cx, 11
        jae urfs_loop_end

        movzx edx, byte [es:bx] ; Load character from memory
        sub dl, '0'             ; Subtract ascii offset
        
        ; Add octal digit to front i.e.  3++4 -> 34 (011 ++ 100 -> 011 100)
        shl eax, 3
        or eax, edx

        add cx, 1
        add bx, 1
        jmp urfs_loop_start
    urfs_loop_end:
    ret
    
    

times 510 - ($ - $$) db 0x00
dw 0xAA55

;---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
; Bootloader Stage 1
;---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

; Definitions
MEMORY_INFO_BOTTOM equ 0x7000
MEMORY_INFO_TOP equ 0x7af0
MEMORY_INFO_ENTRY_SIZE equ 24

SECTORS_PER_SEGMENT equ 0x10000 / 0x200

; Data
GDT_DESCRIPTOR_SIZE equ 6
GDT_DESCRIPTOR_PADDING_SIZE equ 2
GDT_ENTRY_SIZE equ 8
GDT_ENTRY_COUNT equ 3

GDT_DESCRIPTOR_ADDR equ $
GDT_ADDR equ (GDT_DESCRIPTOR_ADDR + GDT_DESCRIPTOR_SIZE + GDT_DESCRIPTOR_PADDING_SIZE)
GDT_DESCRIPTOR dw (GDT_ENTRY_SIZE * GDT_ENTRY_COUNT - 1), GDT_ADDR, 0x0000
GDT_DESCRIPTOR_PADDING db 0x00, 0x00
GDT_NULL dw 0x0000, 0x0000, 0x0000, 0x0000
GDT_KERNEL_CODE dw 0xFFFF, 0x0000, 0x9A00, 0x00CF
GDT_KERNEL_DATA dw 0xFFFF, 0x0000, 0x9200, 0x00CF

boot1_msg db "Stage 1 started", 0x0a, 0x0d, 0x00
boot1_floppy_error_msg db "Failed to load data from floppy disk :(", 0x0a, 0x0d, 0x00
boot1_a20_error_msg db "Failed to enable the a20 line :(", 0x0a, 0x0d, 0x00
boot1_invalid_elf_error_msg db "Kernel file is not a valid elf binary :(", 0x0a, 0x0d, 0x00

boot1_a20_success_msg db "Successfully enabled a20 line", 0x0a, 0x0d, 0x00

lsmi_success_msg db "Successfully obtained system memory information", 0x0a, 0x0d, 0x00
lsmi_error_msg db "Failed to get system memory information :(", 0x0a, 0x0d, 0x00


; Code

; Loads the memory information provided by the BIOS into a buffer
; Note: assumes the provided buffer can contain all entries
; TODO: add extra checks and combine adjacent sections of same type
%macro load_system_memory_info 1 

    ; Initial call to int 15h
    mov di, %1              ; Set write location to buffer
    xor ebx, ebx            ; Clear ebx
    mov edx, 0x534D4150     ; Magic number
    mov eax, 0xe820
    mov ecx, 24

    clc
    int 0x15

    jnc %%lsmi_loop_start

    ; If failed then print error message and halt
    mov si, lsmi_error_msg
    call print_string
    hlt

    %%lsmi_loop_start:
        ; If ebx is 0 or carry flag set then we're done
        cmp ebx, 0
        je %%lsmi_loop_end
        jc %%lsmi_loop_end

        ; If type of length of area is 0 then don't increment
        mov eax, [es:(di + 12)]
        cmp eax, 0
        jne %%lsmi_increment_start

        mov eax, [es:(di + 8)]
        cmp eax, 0
        jne %%lsmi_increment_start

        jmp %%lsmi_increment_end

        %%lsmi_increment_start:
        add di, 24
        %%lsmi_increment_end:

        ; Updating values for next call to int 15h
        mov eax, 0xe820
        mov ecx, 24
        int 0x15

        jmp %%lsmi_loop_start

    %%lsmi_loop_end:

    ; Add NULL entry at the end
    mov word [es:(di + 20)], 0
    mov word [es:(di + 16)], 0
    mov word [es:(di + 12)], 0
    mov word [es:(di +  8)], 0
    mov word [es:(di +  4)], 0
    mov word [es:(di +  0)], 0

    ; No return since macro

%endmacro

boot1_start:
    ;----- Gather system information -----;
    mov si, boot1_msg
    call print_string

    load_system_memory_info MEMORY_INFO_BOTTOM      ; Load memory info into buffer

    mov si, lsmi_success_msg
    call print_string

    ;----- Start reading in kernel from disk -----;

    mov ax, 0x1000
    mov es, ax                      ; Set extra segment register to point at 0x1000*0x10 = 0x10000

    ; Read in kernel file meta data
    xor bx, bx                      ; Reset bx
    call read_next_floppy_sector_repeated ; TODO: get kernel by filepath

    jc boot1_floppy_fail            ; If failed then jump to error code

    ; Get size of kernel in bytes
    xor bx, bx                      ; Reset bx
    call ustar_read_file_size

    ; Calculate number of sectors to read
    ceil_div SECTOR_SIZE

    ; Read kernel from disk
    xor bx, bx                      ; Reset bx
    call read_sectors_multi_segment

    jc boot1_floppy_fail            ; If failed then jump to error code

    ;----- Switch to protected mode -----;

    ; Enable a20 line

    call enable_a20
    cmp ax, 0
    je boot1_a20_fail

    mov si, boot1_a20_success_msg
    call print_string

    ; Load GDT
    cli
    lgdt [GDT_DESCRIPTOR_ADDR]

    ; Set protected bit
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; Set CS register to point to KERNEL_CODE_SEGMENT and jump to pre_kernel
    jmp 0x08:pre_kernel

    boot1_floppy_fail:
    ; print_string(boot1_floppy_error_msg)
    mov si, boot1_floppy_error_msg
    call print_string
    hlt

    boot1_a20_fail:
    mov si, boot1_a20_error_msg
    call print_string
    hlt

    boot1_invalid_elf:
    mov si, boot1_invalid_elf_error_msg
    call print_string
    hlt


; Reads sectors from floppy disk, if we are reading more than one segment's
; worth of data then update es register and continue reading
; On error the carry flag is set
; Arguments: es - segment to write to, ax - number of sectors to read
read_sectors_multi_segment:
    mov cx, ax                                  ; Copy number of sectors to read to cx

    rsms_loop_start:
        cmp cx, SECTORS_PER_SEGMENT
        jb rsms_remainder_sectors               ; If we have less than a segment's worth of sectors to read then jump to remainder code

        ; Read sectors into current segment
        mov ax, SECTORS_PER_SEGMENT             ; Set number of sectors to read
        xor bx, bx                              ; Reset bx (so no offset)

        push cx
        call read_sectors
        pop cx

        jc rsms_read_fail                       ; Reading failed then jump to error code

        ; Update es register to next segment
        mov ax, es
        add ax, 0x1000                          ; Move to next full segment (next segment that does not overlap the current one)        
        mov es, ax

        sub cx, SECTORS_PER_SEGMENT             ; Decrement cx
        jmp rsms_loop_start                     ; Loop

    rsms_remainder_sectors:
    ; Read the remaining sectors to memory
    mov ax, cx                                  ; Load number of remaining sectors into ax
    xor bx, bx                                  ; Reset bx
    call read_sectors

    jc rsms_read_fail                           ; If failed jump to error code

    ret

    rsms_read_fail:
    ret                                         ; Just return on failure (carry flag will be set)

; Checks if the a20 line is set
; Does not modify any registers except ax
; Returns 0 in ax if not set, and 1 in ax if it is set
; Arguments: none
;
; Taken from https://wiki.osdev.org/A20_Line
check_a20:
    pushf
    push ds
    push es
    push di
    push si

    cli

    xor ax, ax  ; ax = 0
    mov es, ax

    not ax      ; ax = 0xFFFF
    mov ds, ax

    mov di, 0x0500
    mov si, 0x0510

    mov al, byte [es:di]
    push ax

    mov al, byte [ds:si]
    push ax

    mov byte [es:di], 0x00
    mov byte [ds:si], 0xFF

    cmp byte [es:di], 0xFF

    pop ax
    mov byte [ds:si], al

    pop ax
    mov byte [es:di], al

    mov ax, 0
    je check_a20_exit

    mov ax, 1

    check_a20_exit:
    pop si
    pop di
    pop es
    pop ds
    popf

    ret

; Enables the a20 line (allowing access to higher memory)
; If a20 is already set then does nothing
; Returns 1 on success or 0 on failure
; Arguments: none
enable_a20:
    call check_a20
    cmp ax, 0
    je enable_a20_bios_method
    ret                 ; If already enabled then just return

    enable_a20_bios_method:
    ;----- Try enabling through BIOS first -----;

    ; Check if BIOS function is supported
    mov ax, 0x2403                          ; Query A20 Gate Support
    int 0x15

    jc enable_a20_keyboard_method
    cmp ax, 0
    jne enable_a20_keyboard_method

    ; Try enabling using BIOS function
    mov ax, 0x2401
    int 0x15

    jc enable_a20_keyboard_method
    cmp ax, 0
    jne enable_a20_keyboard_method

    ; Check if it actually worked
    call check_a20
    cmp ax, 0
    je enable_a20_keyboard_method           ; If still not set then try keyboard method

    mov ax, 1
    ret                                     ; If successfully set then return 1

    ;----- Try enabling through keyboard  -----;
    enable_a20_keyboard_method:

    ; Modified from: https://wiki.osdev.org/A20_Line
    call    a20wait
    mov     al,0xAD
    out     0x64,al

    call    a20wait
    mov     al,0xD0
    out     0x64,al

    call    a20wait2
    in      al,0x60
    push    eax

    call    a20wait
    mov     al,0xD1
    out     0x64,al

    call    a20wait
    pop     eax
    or      al,2
    out     0x60,al

    call    a20wait
    mov     al,0xAE
    out     0x64,al

    call    a20wait

    call check_a20
    cmp ax, 0

    ret                                     ; Returns 1 if set or 0 if not set
    
a20wait:
    in al, 0x64
    test al, 2
    jnz a20wait
    ret

a20wait2:
    in al, 0x64
    test al, 1
    jz a20wait2
    ret

;---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
; Pre-kernel protected mode
;---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
[bits 32]

ELF_MAGIC_NUMBER equ 0x464C457F
ELF_HEADER_4_7   equ 0x00010101 ; header values 4-7 (32-bit, little-endian, elf header version 1, os abi version 0)
ELF_HEADER_16_19 equ 0x00030002 ; header values 16-19 (executable, x86)
ELF_HEADER_20_23 equ 0x00000001 ; header value  20-23 (ELF version 1)

; Copies memory from one address to another
; Arguments: EAX source address, EDX destination address, ECX number of bytes to copy
; Returns nothing
memcpy:
    push ecx

    memcpy_loop_start:
        cmp ecx, 0
        je memcpy_loop_end

        ; copy byte
        push ecx
        mov byte cl, [eax]
        mov byte [edx], cl
        pop ecx

        inc eax
        inc edx
        dec ecx
        jmp memcpy_loop_start
    memcpy_loop_end:

    pop ecx
    ret

; Loads an elf program table entry
; Arguments: EAX elf file base address, EDX program header address
; Returns nothing
load_elf_program_table_entry:
    ; Save preserved registers
    push ebx
    push ecx

    mov ebx, [edx]      ; load type of segment
    add edx, 4          ; increment pointer
    cmp ebx, 1          ; check if this is a load segment
    jne lepte_finish    ; if not then do nothing

    mov ebx, [edx]      ; load data offset
    add edx, 4          ; increment pointer
    add eax, ebx        ; calculate segment start address

    mov ebx, [edx]      ; load virtual memory address of segment
    add edx, 4          ; increment pointer

    add edx, 4          ; ignore undefined bytes

    mov ecx, [edx]      ; load size of segment in file
    add edx, 4          ; increment pointer

    push ecx            ; store size of segment in file on stack
    mov ecx, [edx]      ; load size of segment in memory
    add edx, 4          ; increment pointer

    push ebx            ; store vmem address of segment on stack

    ; Zero out memory of segment
    lepte_loop_start:
        cmp ecx, 0              ; loop condition
        je lepte_loop_end       ; exit if true
        
        mov byte [ebx], 0       ; write 0 to current address
    
        dec ecx                 ; decrement loop counter
        inc ebx                 ; increment pointer
        jmp lepte_loop_start    ; loop
    lepte_loop_end:

    pop ebx             ; restore vmem address of segment
    pop ecx             ; restore segment size in file

    ; mov eax, eax      ; set segment in file as source address
    mov edx, ebx        ; set vmem address as destination address
    call memcpy         ; copy segment data to destination address

    lepte_finish:
    pop ecx
    pop ebx
    ret

; Loads the kernel from the ELF binary at EAX and writes it to the address specified within it (should be 0x100000)
; Arguments: EAX ELF address
; Returns nothing
load_kernel_elf:
    ; Save preserved registers
    push ebx
    push esi
    push edi

    ; Save ELF base address
    push eax

    ; copy ELF starting address to preserved registers
    mov ebx, eax

    ; Check the magic number
    mov eax, [ebx]            ; get magic number
    add ebx, 4                ; increment pointer
    cmp eax, ELF_MAGIC_NUMBER ; compare read magic number with ELF magic number
    jne lke_finish            ; if not equal then exit

    ; Check and load ELF header information
    mov eax, [ebx]            ; load next 4 entries
    add ebx, 4                ; increment pointer
    cmp eax, ELF_HEADER_4_7   ; check values match the expected values
    jne lke_finish            ; if not then exit

    add ebx, 8                ; ignore padding bytes

    mov eax, [ebx]            ; load next entries
    add ebx, 4                ; increment pointer
    cmp eax, ELF_HEADER_16_19 ; check values
    jne lke_finish            ; if not equal then exit

    mov eax, [ebx]            ; load next entry
    add ebx, 4                ; increment pointer
    cmp eax, ELF_HEADER_20_23 ; check values
    jne lke_finish            ; if not equal then exit

    mov edx, [ebx]            ; load program entry position
    add ebx, 4                ; increment pointer
    push edx                  ; save entry position to stack

    mov ecx, [ebx]            ; load program header table position
    add ebx, 4                ; increment pointer

    add ebx, 8                ; ignore section header table position and flags

    mov eax, [ebx]            ; load header size and size of entry in program header table
    add ebx, 4                ; increment pointer
    shr eax, 16               ; ignore header size but keep pht entry sizes

    mov edx, [ebx]            ; load number of program header table entries and size of segment headers
    and edx, 0x0000FFFF       ; ignore size of segment headers

    ; Read program header data
    mov ebx, [esp + 0x04]     ; load ELF base address from stack
    add ebx, ecx              ; add program header table offset

    ; Iterate over all program header entries
    mov ecx, edx              ; copy size to loop register
    lke_loop_start:
        cmp ecx, 0                          ; loop check
        je lke_loop_end
        push eax                            ; save program table entry size to stack

        mov eax, [esp + 0x08]               ; copy file base address from stack
        mov edx, ebx                        ; copy header entry address

        call load_elf_program_table_entry   ; load entry

        pop eax                             ; restore program table entry size from stack
        add ebx, eax                        ; update address to next entry
        dec ecx                             ; decrement count
        jmp lke_loop_start                  ; loop
    lke_loop_end:
    pop edx                   ; load program entry position from stack

    lke_finish:
    ; Restore preserved registers and return
    pop eax
    pop edi
    pop esi
    pop ebx
    ret
    

pre_kernel:
    ; Set the rest of the segment registers to point to KERNEL_DATA_SEGMENT
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Load kernel into upper memory
    mov eax, 0x010000
    call load_kernel_elf    ; load kernel to address 0x010000
    nop
    jmp edx                 ; jump to kernel

    hlt
