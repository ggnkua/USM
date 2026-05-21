; prg_loader_compressed.s -- default-mode cart stub for LZSS-compressed
; payloads. Drop-in replacement for prg_loader.s when usm is invoked with
; -z. The PRG file (header + TEXT + DATA + symbols + reloc table) is
; compressed as a single blob; this stub does Mshrink + Malloc to allocate
; a fresh TPA, then inline-decompresses straight into TPA + $100 before
; running the same BSS clear / relocation / basepage-fill / JMP tail as
; the uncompressed stub.
;
; Cart-side layout for one compressed entry:
;     [ CA_HEADER 34 B ]
;     [ this stub      ]
;     [ uncompressed_size LONG (4 B, big-endian) ]
;     [ compressed bytes                          ]
;
; usm.c writes the LONG just before the compressed bytes; the stub locates
; it via PC-relative addressing of the `prg_payload` label at the bottom.
;
; LZSS-12-4 stream format (matches lzss_compress / lzss_decompress in usm.c):
;   blocks of 1 flag byte + up to 8 tokens.
;   flag bit 0 = literal (1 byte); flag bit 1 = back-ref (2 bytes BE,
;   packed (offset-1) << 4 | (length-3); offset 1..4096, length 3..18).
;
; Rebuild via:
;     STCMD_NO_TTY=1 STCMD_QUIET=1 stcmd vasm -quiet -Fbin \
;         -o prg_loader_compressed.bin prg_loader_compressed.s
; then regenerate the prg_loader_compressed[] byte array in usm.c.

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
gemdos_mshrink  EQU $4a

start:
                ;-- Mshrink the TOS-provided basepage to $100, freeing the
                ;   bulk of the TPA. Same reason as the uncompressed stub:
                ;   without this the Malloc below sees no free memory.
                move.l  4(sp),a4
                move.l  #$100,-(sp)
                move.l  a4,-(sp)
                clr.w   -(sp)
                move.w  #gemdos_mshrink,-(sp)
                trap    #1
                lea     12(sp),sp

                ;-- Malloc(-1) to find the largest free chunk size
                moveq.l #-1,d0
                move.l  d0,-(sp)
                move.w  #gemdos_malloc,-(sp)
                trap    #1
                addq.l  #6,sp
                move.l  d0,d1               ; D1 = chunk size

                ;-- Malloc(size) to claim it as our new TPA
                move.l  d0,-(sp)
                move.w  #gemdos_malloc,-(sp)
                trap    #1
                addq.l  #6,sp

                ;-- Zero the 256-byte basepage at the head of the TPA
                movea.l d0,a4
                move.w  #63,d2
.zero_basepage:
                clr.l   (a4)+
                dbra    d2,.zero_basepage

                ;-- Lay out basepage and SP (Pexec convention)
                movea.l d0,a5
                move.l  d0,p_lowtpa(a5)
                movea.l d0,sp
                adda.l  d1,sp
                move.l  sp,p_hitpa(a5)
                subq.l  #8,sp
                move.l  a5,4(sp)
                lea     $80(a5),a4
                move.l  a4,p_dta(a5)
                lea     $30(a5),a4
                move.l  a4,p_env(a5)

                ;-- Locate the compressed payload in ROM (PC-relative).
                ;   `prg_payload` points at the uncompressed_size LONG;
                ;   advancing a4 by 4 leaves it at the compressed bytes.
                lea     prg_payload(pc),a4
                move.l  (a4)+,d0            ; D0 = uncompressed_size
                movea.l a4,a0               ; A0 = source (compressed)
                lea     $100(a5),a1         ; A1 = destination (TPA + $100)

                ;-- Inline LZSS-12-4 decompressor.
                ;   In:  A0 = compressed src, A1 = dst start, D0 = expected size
                ;   Out: A1 = dst end (= start + D0)
                ;   Uses: D2-D5, A2, A3
                lea     (a1,d0.l),a3        ; A3 = end of destination
                cmpa.l  a3,a1
                bge.s   .decompress_done
.flag_loop:
                move.b  (a0)+,d2            ; D2 = flag byte
                moveq.l #7,d3               ; 8 bits in flag (count 7..0)
.bit_loop:
                cmpa.l  a3,a1
                bge.s   .decompress_done
                add.b   d2,d2               ; MSB of D2 -> carry, D2 shifts left
                bcs.s   .back_ref
                move.b  (a0)+,(a1)+         ; literal byte
                dbra    d3,.bit_loop
                bra.s   .flag_loop
.back_ref:
                moveq.l #0,d4
                move.b  (a0)+,d4            ; D4 = high byte of word
                lsl.w   #8,d4
                move.b  (a0)+,d4            ; D4 = full 16-bit word
                move.w  d4,d5
                lsr.w   #4,d4
                addq.w  #1,d4               ; D4 = offset (1..4096)
                andi.w  #$f,d5
                addq.w  #3,d5               ; D5 = length (3..18)
                movea.l a1,a2
                suba.w  d4,a2               ; A2 = A1 - offset (match source)
                subq.w  #1,d5
.copy_match:
                move.b  (a2)+,(a1)+
                dbra    d5,.copy_match
                dbra    d3,.bit_loop
                bra.s   .flag_loop
.decompress_done:

                ;-- After decompression: $100(a5) holds the original PRG file
                ;   contents (header + TEXT + DATA + symbols + reloc table).
                ;   Compute the in-RAM landmarks the BSS / relocation / basepage
                ;   tail expects:
                ;       A4 = $100(a5)                          (PRG header)
                ;       A0 = end of symbol table = reloc start (in RAM)
                ;       A1 = end of symbol table              (BSS-clear anchor;
                ;                                              matches the
                ;                                              uncompressed stub's
                ;                                              post-copy state)
                lea     $100(a5),a4
                movea.l a4,a0
                adda.w  #prg_text,a0
                adda.l  prg_tlen(a4),a0
                adda.l  prg_dlen(a4),a0
                adda.l  prg_slen(a4),a0
                movea.l a0,a1

                ;-- Conditionally zero BSS (skip when PRGFLAGS fastload bit set)
                btst    #0,prg_flags+3(a4)
                bne.s   .skip_bss
                move.l  prg_blen(a4),d0
                add.l   a1,d0
.bss_loop:
                move.w  #0,(a1)+
                cmp.l   a1,d0
                bgt.s   .bss_loop
.skip_bss:

                ;-- Apply PRG relocations. A0 still points at the reloc
                ;   table in RAM, exactly as the uncompressed stub left it
                ;   pointing at the reloc table in ROM.
                ;
                ;   IMPORTANT: TEXT in RAM is at $100(a5)+prg_text = $11C(a5),
                ;   NOT $100(a5). We decompressed the whole PRG file (header
                ;   included) starting at $100(a5), so the 28-byte header
                ;   precedes TEXT in the in-RAM image. The relocation delta
                ;   has to be the TEXT base, hence $11C(a5).
                tst.l   (a0)
                beq.s   .skip_reloc
                lea     $100+prg_text(a5),a1
                move.l  a1,d1
                moveq.l #0,d0
                adda.l  (a0)+,a1
                add.l   d1,(a1)
.reloc_walk:
                move.b  (a0)+,d0
                beq.s   .skip_reloc
                cmp.b   #1,d0
                bne.s   .reloc_step
                adda.w  #254,a1
                bra.s   .reloc_walk
.reloc_step:
                adda.w  d0,a1
                add.l   d1,(a1)
                bra.s   .reloc_walk
.skip_reloc:

                ;-- Fill the remaining basepage fields. TEXT is at
                ;   $100(a5)+prg_text = $11C(a5) in the in-RAM image (see
                ;   note above the relocator).
                lea     $100+prg_text(a5),a1
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

                ;-- Hand off to the program
                suba.l  a0,a0
                jmp     (a1)

prg_payload:
                ; usm.c writes the uncompressed_size LONG here, followed by
                ; the compressed payload bytes. Do not move this label
                ; without re-measuring the lea above.
