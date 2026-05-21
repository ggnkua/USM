; prg_loader.s -- default-mode cart stub for USM-generated carts.
;
; Works both for auto-init (-f 3) at boot and for manual desktop launch
; from the cart's c: drive in user mode. We allocate a fresh TPA from
; the largest free chunk, build a complete Pexec-style basepage from
; scratch (ignoring anything TOS may have already provided), copy the
; appended PRG payload from ROM into the TPA, conditionally zero-fill
; BSS (skip when the PRGFLAGS fastload bit is set), apply PRG
; relocations, then JMP to the program. No privileged instructions, so
; the same stub runs in either supervisor or user mode.
;
; The appended PRG is located via PC-relative addressing of the
; `prg_payload` label at the end of this file, so every stub instance in
; a multi-program cart finds its own payload without any cart-build-time
; patching. The byte-stream output of this file is hand-pasted into the
; `prg_loader[]` array in usm.c.
;
; Rebuild after edits:
;     STCMD_NO_TTY=1 STCMD_QUIET=1 stcmd vasm -quiet -Fbin \
;         -o prg_loader.bin prg_loader.s
; then regenerate the byte array (Python helper in usm.c history / git log).
;
; Inspired by Christian Zietz's CARTLOAD.ABS
; (https://www.chzsoft.de) -- algorithm matches; written from scratch in
; asm so it lives in our build pipeline.

; ---- Basepage offsets (Atari Compendium) -------------------------------
p_lowtpa        EQU $00
p_hitpa         EQU $04
p_tbase         EQU $08
p_tlen          EQU $0c
p_dbase         EQU $10
p_dlen          EQU $14
p_bbase         EQU $18
p_blen          EQU $1c
p_dta           EQU $20
p_env           EQU $2c
p_cmdlen        EQU $80

; ---- PRG header offsets ------------------------------------------------
prg_tlen        EQU $02
prg_dlen        EQU $06
prg_blen        EQU $0a
prg_slen        EQU $0e
prg_flags       EQU $16
prg_absflag     EQU $1a
prg_text        EQU $1c

; ---- GEMDOS function codes --------------------------------------------
gemdos_malloc   EQU $48

start:
                ;-- Mshrink the TOS-provided basepage to $100 first. When
                ;   we're launched via Pexec from the cart's c: drive,
                ;   TOS hands us essentially all of free memory as our
                ;   TPA -- without this Mshrink, the Malloc(-1) below
                ;   would see no free memory and return 0. The trap
                ;   silently no-ops if 4(sp) doesn't actually point at a
                ;   basepage (e.g. an auto-init invocation).
                move.l  4(sp),a4         ; a4 = TOS-provided basepage
                move.l  #$100,-(sp)
                move.l  a4,-(sp)
                clr.w   -(sp)
                move.w  #$4a,-(sp)       ; Mshrink
                trap    #1
                lea     12(sp),sp

                ;-- Ask GEMDOS for the size of the largest free chunk
                moveq.l #-1,d0
                move.l  d0,-(sp)
                move.w  #gemdos_malloc,-(sp)
                trap    #1
                addq.l  #6,sp
                move.l  d0,d1               ; D1 = chunk size (preserved
                                            ; across the next Malloc trap)

                ;-- Allocate it as our new TPA
                move.l  d0,-(sp)
                move.w  #gemdos_malloc,-(sp)
                trap    #1
                addq.l  #6,sp
                ; D0 = base of allocated block

                ;-- Zero-fill the 256-byte basepage at the head of the
                ;   allocated chunk (also zeros p_cmdlen at +$80 to give
                ;   us a valid empty command line).
                movea.l d0,a4
                move.w  #63,d2              ; 64 longs = 256 bytes
.zero_basepage:
                clr.l   (a4)+
                dbra    d2,.zero_basepage

                ;-- (We deliberately do NOT touch SR here. The chzsoft
                ;   loader does `move.w #$0300, sr` to drop to user
                ;   mode at IPL 3, but that instruction is privileged
                ;   and would trap when our cart is launched from the
                ;   c: desktop in user mode via Pexec. Running in
                ;   whatever mode the caller was in -- supervisor for
                ;   auto-init, user for desktop launch -- works for both
                ;   contexts. The Malloc traps above are not privileged.)

                ;-- Lay out the basepage and SP
                movea.l d0,a5               ; A5 = basepage
                move.l  d0,p_lowtpa(a5)
                movea.l d0,sp
                adda.l  d1,sp               ; SP = base + size = TPA top
                move.l  sp,p_hitpa(a5)
                subq.l  #8,sp               ; reserve 8 bytes for Pexec frame
                move.l  a5,4(sp)            ; 4(sp) = basepage  (Pexec
                                            ; convention -- the program's
                                            ; `move.l 4(sp),aN` finds it)

                lea     $80(a5),a4
                move.l  a4,p_dta(a5)        ; default DTA = inline cmdline
                lea     $30(a5),a4
                move.l  a4,p_env(a5)        ; empty environment (zeros)

                ;-- Find the appended PRG payload in ROM. PC-relative, so
                ;   every stub instance in a multi-program cart finds its
                ;   own payload at the same fixed offset.
                lea     prg_payload(pc),a4  ; A4 = ROM PRG header

                ;-- Copy header + TEXT + DATA + symbol table from ROM
                ;   into the TPA at $100(A5). Word-aligned word copy --
                ;   PRG sums are even for all PRGs we've seen.
                lea     prg_text(a4),a0     ; A0 = ROM TEXT start
                lea     $100(a5),a1         ; A1 = RAM dest (past basepage)
                move.l  prg_tlen(a4),d0
                add.l   prg_dlen(a4),d0
                add.l   prg_slen(a4),d0
                lsr.l   #1,d0
                subq.l  #1,d0
.copy_loop:
                move.w  (a0)+,(a1)+
                dbra    d0,.copy_loop

                ;-- Conditionally zero-fill BSS. The PRGFLAGS fastload
                ;   bit (bit 0 of the LONG at PRG header offset $16, so
                ;   bit 0 of the low byte at $19) says "BSS does not need
                ;   to be zeroed"; honor it as the chzsoft loader does.
                btst    #0,prg_flags+3(a4)
                bne.s   .skip_bss
                move.l  prg_blen(a4),d0
                add.l   a1,d0               ; D0 = end of BSS in RAM
.bss_loop:
                move.w  #0,(a1)+
                cmp.l   a1,d0
                bgt.s   .bss_loop
.skip_bss:

                ;-- Apply standard PRG relocations. A0 now points to the
                ;   start of the relocation table in ROM (just past the
                ;   symbol table).
                tst.l   (a0)
                beq.s   .skip_reloc         ; first LONG = 0 -> no fixups
                lea     $100(a5),a1
                move.l  a1,d1               ; D1 = RAM TEXT base (the
                                            ; relocation delta to add)
                moveq.l #0,d0
                adda.l  (a0)+,a1            ; A1 += first 4-byte LONG offset
                add.l   d1,(a1)             ; apply first fixup
.reloc_walk:
                move.b  (a0)+,d0
                beq.s   .skip_reloc         ; 0 = end of table
                cmp.b   #1,d0
                bne.s   .reloc_step
                adda.w  #254,a1             ; 1 = advance 254, no fixup
                bra.s   .reloc_walk
.reloc_step:
                adda.w  d0,a1               ; advance by byte
                add.l   d1,(a1)             ; apply fixup
                bra.s   .reloc_walk
.skip_reloc:

                ;-- Finish the basepage: p_tbase, p_tlen, p_dbase, p_dlen,
                ;   p_bbase, p_blen.
                lea     $100(a5),a1         ; A1 = RAM TEXT start = entry
                move.l  a1,d1
                move.l  d1,p_tbase(a5)
                move.l  prg_tlen(a4),d0
                move.l  d0,p_tlen(a5)
                add.l   d0,d1
                move.l  d1,p_dbase(a5)
                move.l  prg_dlen(a4),d0
                move.l  d0,p_dlen(a5)
                add.l   d0,d1
                move.l  d1,p_bbase(a5)
                move.l  prg_blen(a4),p_blen(a5)

                ;-- Hand off to the program. A0 = 0 matches the convention
                ;   the original USM stub used (some PRGs check it).
                suba.l  a0,a0
                jmp     (a1)

prg_payload:
                ; usm.c appends the verbatim PRG file here. Do not move
                ; this label without also re-measuring the lea above.
