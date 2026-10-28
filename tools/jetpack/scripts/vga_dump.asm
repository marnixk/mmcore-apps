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
    inc word [cs:ticks]
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
    cmp byte [seq_on], 0
    jne .seq_step
    mov dx, seq_name
    mov ax, 3D00h
    int 21h
    jc .single
    mov bx, ax
    mov ah, 3Eh
    int 21h
    mov byte [seq_on], 1
    mov ax, [ticks]
    mov [last_tick], ax
    jmp .out
.single:
    mov dx, go_name
    mov ax, 3D00h
    int 21h
    jc .out
    mov bx, ax
    mov ah, 3Eh
    int 21h
    call write_dump
    mov byte [cs:done], 1
    jmp .out
.seq_step:
    mov ax, [ticks]
    sub ax, [last_tick]
    cmp ax, 3
    jb .out
    mov ax, [ticks]
    mov [last_tick], ax
    cmp byte [frame_i], 80
    jae .seq_done
    call write_frame
    inc byte [frame_i]
    jmp .out
.seq_done:
    call dump_mem
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

; One mode-13h frame: "JPFR", tick, index, mode, 768 DAC bytes, 64000 plane-0 bytes.
write_frame:
    push cs
    pop ds
    call read_regs
    mov al, [mode]
    mov [frm_mode], al
    mov al, [frame_i]
    xor ah, ah
    mov bl, 10
    div bl
    add ax, 3030h
    mov [frm_name+1], al
    mov [frm_name+2], ah
    mov dx, frm_name
    xor cx, cx
    mov ah, 3Ch
    int 21h
    jc .fail
    mov [handle], ax
    mov al, [frame_i]
    mov [frm_index], al
    mov ax, [ticks]
    mov [frm_tick], ax
    mov dx, frm_header
    mov cx, frm_header_len
    call write_cx
    mov dx, dac
    mov cx, 768
    call write_cx
    mov dx, 3CEh
    mov al, 4
    out dx, al
    inc dx
    xor al, al
    out dx, al
    mov ax, 0A000h
    mov ds, ax
    xor dx, dx
    mov cx, 7D00h
    call write_cx_ds
    mov dx, 7D00h
    mov cx, 7D00h
    call write_cx_ds
    push cs
    pop ds
    call capture_state
    mov dx, state_buf
    mov cx, 32
    call write_cx
    mov bx, [handle]
    mov ah, 3Eh
    int 21h
.fail:
    ret

; Trailer after the plane: player frame, 20 enemy frames, player x, player y.
; game DS = EXE load segment + 0x1D03. The load segment comes from the
; signature B8 40 01 2B 46 10 at image offset 0x188CB.
capture_state:
    push ds
    cmp word [cs:game_ds], 0
    jnz .read
    call find_game
    cmp word [cs:game_ds], 0
    jz .miss
.read:
    mov ds, [cs:game_ds]
    mov al, [0x32c7]
    mov [cs:state_buf], al
    mov al, [0x32c6]
    mov [cs:state_buf+26], al
    mov ax, [0x34d3]
    mov [cs:state_buf+21], ax
    mov ax, [0x34d1]
    mov [cs:state_buf+23], ax
    mov si, 0x329e
    mov cx, 20
.copye:
    mov al, [si]
    mov bx, si
    sub bx, 0x329e
    mov [cs:state_buf+1+bx], al
    inc si
    loop .copye
    mov byte [cs:state_buf+25], 1
    pop ds
    ret
.miss:
    mov byte [cs:state_buf+25], 0
    pop ds
    ret

find_game:
    mov ax, 0x700
.segloop:
    add ax, 0xFF0
    cmp ax, 0xA000
    jae .no
    mov es, ax
    xor bx, bx
.byteloop:
    cmp byte [es:bx], 0xB8
    jne .next
    cmp byte [es:bx+1], 0x40
    jne .next
    cmp byte [es:bx+2], 0x01
    jne .next
    cmp byte [es:bx+3], 0x2B
    jne .next
    cmp byte [es:bx+4], 0x46
    jne .next
    cmp byte [es:bx+5], 0x10
    jne .next
    push ax
    mov dx, ax
    mov cl, 4
    shl ax, cl
    mov cl, 12
    shr dx, cl
    add ax, bx
    adc dx, 0
    sub ax, 0x88CB
    sbb dx, 1
    test al, 0x0F
    jnz .notsig
    mov cl, 4
    shr ax, cl
    mov cl, 12
    shl dx, cl
    or ax, dx
    add ax, 0x1D03
    mov [cs:game_ds], ax
    pop ax
    ret
.notsig:
    pop ax
.next:
    inc bx
    jnz .byteloop
    jmp .segloop
.no:
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
seq_on db 0
frame_i db 0
game_ds dw 0
state_buf times 32 db 0
ticks dw 0
last_tick dw 0
handle dw 0
go_name db "GO.DMP", 0
seq_name db "SEQ.GO", 0
frm_name db "F00.BIN", 0
out_name db "VGA.BIN", 0
mem_name db "MEM.BIN", 0
frm_header:
    db "JPFR"
frm_tick dw 0
frm_index db 0
frm_mode db 0
frm_header_len equ $ - frm_header
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
