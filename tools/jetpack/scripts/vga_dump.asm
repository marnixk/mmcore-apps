; VGA ground-truth dump TSR. Installed before JETPACK.EXE.
; When C:\GO.DMP exists and BIOS video mode is 13h, write C:\VGA.BIN once:
;   magic "JPVG", bios mode, misc, seq[5], crtc[25], gfx[9], dac[768], 4x64KiB planes.
org 100h

BITS 16

start:
    mov ax, 351Ch
    ; hook IRQ0 (INT 08). Game chains to the previous vector.
    mov ax, 3508h
    int 21h
    mov [old_off], bx
    mov [old_seg], es
    mov ah, 34h
    int 21h
    mov [indos_off], bx
    mov [indos_seg], es
    mov dx, hook
    mov ax, 2508h
    int 21h
    mov dx, resident_end
    add dx, 15
    shr dx, 4
    mov ax, 3100h
    int 21h

hook:
    cmp byte [cs:busy], 0
    jne .chain
    cmp byte [cs:done], 0
    jne .chain
    mov byte [cs:busy], 1
    mov [cs:save_ss], ss
    mov [cs:save_sp], sp
    push cs
    pop ss
    mov sp, stack_top
    sti
    call try_dump
    cli
    mov ss, [cs:save_ss]
    mov sp, [cs:save_sp]
    mov byte [cs:busy], 0
.chain:
    jmp far [cs:old_off]

try_dump:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push ds
    push es
    mov ax, 40h
    mov es, ax
    cmp byte [es:49h], 13h
    jne .out
    les bx, [cs:indos_off]
    cmp byte [es:bx], 0
    jne .out
    cmp byte [es:bx-1], 0
    jne .out
    push cs
    pop ds
    mov dx, go_name
    mov ax, 3D00h
    int 21h
    jc .out
    mov bx, ax
    mov ah, 3Eh
    int 21h
    call write_dump
    mov byte [cs:done], 1
.out:
    pop es
    pop ds
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

write_dump:
    push cs
    pop ds
    call read_regs
    mov dx, out_name
    xor cx, cx
    mov ah, 3Ch
    int 21h
    jc .fail
    mov [handle], ax
    mov dx, header
    mov cx, header_len
    call write_cx
    mov dx, dac
    mov cx, 768
    call write_cx
    xor si, si
.plane:
    mov dx, 3CEh
    mov al, 4
    out dx, al
    inc dx
    mov ax, si
    out dx, al
    mov ax, 0A000h
    mov ds, ax
    xor dx, dx
    mov cx, 8000h
    call write_cx_ds
    mov dx, 8000h
    mov cx, 8000h
    call write_cx_ds
    push cs
    pop ds
    inc si
    cmp si, 4
    jb .plane
    mov bx, [handle]
    mov ah, 3Eh
    int 21h
    call dump_mem
    ret
.fail:
    ret

; Conventional memory 0000:0000 .. A000:0000 (640 KiB), 32 KiB DOS writes.
dump_mem:
    push cs
    pop ds
    mov dx, mem_name
    xor cx, cx
    mov ah, 3Ch
    int 21h
    jc .done
    mov [handle], ax
    xor si, si
.next:
    cmp si, 0A000h
    jae .close
    mov ds, si
    xor dx, dx
    mov cx, 8000h
    call write_cx_ds
    push cs
    pop ds
    add si, 800h
    jmp .next
.close:
    mov bx, [handle]
    mov ah, 3Eh
    int 21h
.done:
    ret

write_cx:
    mov bx, [handle]
    mov ah, 40h
    int 21h
    ret

write_cx_ds:
    mov bx, [cs:handle]
    mov ah, 40h
    int 21h
    ret

read_regs:
    mov ax, 40h
    mov es, ax
    mov al, [es:49h]
    mov [mode], al
    mov dx, 3CCh
    in al, dx
    mov [misc], al
    xor cx, cx
.seq:
    mov dx, 3C4h
    mov al, cl
    out dx, al
    inc dx
    in al, dx
    mov di, seq
    add di, cx
    mov [di], al
    inc cx
    cmp cx, 5
    jb .seq
    xor cx, cx
.crtc:
    mov dx, 3D4h
    mov al, cl
    out dx, al
    inc dx
    in al, dx
    mov di, crtc
    add di, cx
    mov [di], al
    inc cx
    cmp cx, 25
    jb .crtc
    xor cx, cx
.gfx:
    mov dx, 3CEh
    mov al, cl
    out dx, al
    inc dx
    in al, dx
    mov di, gfx
    add di, cx
    mov [di], al
    inc cx
    cmp cx, 9
    jb .gfx
    mov dx, 3C7h
    xor al, al
    out dx, al
    mov dx, 3C9h
    mov di, dac
    mov cx, 768
.dac:
    in al, dx
    mov [di], al
    inc di
    loop .dac
    ret

old_off dw 0
old_seg dw 0
indos_off dw 0
indos_seg dw 0
save_ss dw 0
save_sp dw 0
busy db 0
done db 0
handle dw 0
go_name db "GO.DMP", 0
out_name db "VGA.BIN", 0
mem_name db "MEM.BIN", 0
header:
    db "JPVG"
mode db 0
misc db 0
seq times 5 db 0
crtc times 25 db 0
gfx times 9 db 0
header_len equ $ - header
dac times 768 db 0
    times 128 dw 0
stack_top:
resident_end:
