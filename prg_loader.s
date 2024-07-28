; Code originally by tIn/Newline, "creatively adjusted" by GGN.
; Obviously in need of a cleanup!

ULTRADEV        EQU 0				
START_PAYLOAD   EQU 01
start:


                move.l  4(sp),a0         ; Obtain pointer to basepage
                move.l  a0,d7            ; Save a copy (start of TPA)
;                add.l #$100,a0           ; add basepage size (end of TPA)
;                move.l  $c(a0),a1        ; Text size
;                adda.l  $14(a0),a1      ; Data size
;                adda.l  $18(a0),a1      ; BSS size
;                adda.l  $1C(a0),a1      ; Add BSS size
;                adda.l #$100,a1         ; Add Basepage size
;                adda.l  #$200,a1         ; Add stack size
                
                ;move.l %a1,%sp            ; Move your stack pointer to
                                           ; your new stack.
                
;                suba.l d7,a1             ; TPA size
;                move.l a1,-(sp)
                move.l #$100,-(sp)      ; new size - just the basepage if you don't mind
                move.l d7,-(sp)         ; start of TPA
                clr.w  -(sp)
                move.w #$4a,-(sp)        ; Mshrink()
                trap   #1
                lea    12(sp),sp        ; Fix up stack

;               ;malloc
;               move.l  #payload-run,D0
;               move.l  #-1,-(sp)       ; Return amount of largest chunk of RAM
;               move.w  #72,-(sp)
;               trap    #1              ; GEMDOS
;               addq.l  #6,sp
;
;               move.l  D0,A0
;               move.l  D0,A3
;               lea     run,A1
;               lea     payload,A2
;.cl: 
;               move.b  (A1)+,(A0)+
;               cmp.l   A2,A1
;               blt.s   .cl
;
;               jmp     (A3)

run:

                IFNE    START_PAYLOAD
;from http://www.bitsavers.org/pdf/atari/ST/Atari_ST_GEM_Programming_1986/GEM_0042.pdf
;TPA structure
p_lowtpa        EQU $00 ;.l p_lowtpa -> base of TPA
p_hitpa         EQU $04 ;.l p_hitpa -> end of TPA
p_tbase         EQU $08 ;.l p_tbase base of text segment
p_tlen          EQU $0c ;.l p_tlen size of text segment
p_dbase         EQU $10 ;.l p_dbase base of data segment
p_dlen          EQU $14 ;.l p_dlen size of data segment
p_bbase         EQU $18 ;.l p_bbase base of BSS segment
p_blen          EQU $1c ;.l p_blen size of BSS segment
p_dta           EQU $20 ;.l p_dta Disk Transfer Address (DTA)
p_parent        EQU $24 ;.l p_parent -> parent's basepage
p_reserved      EQU $28 ;.l (reserved)  
p_env           EQU $2c ;.l p_env -> enviroment string
p_cmdlin        EQU $80 ;.l p_cmdlin commandline image
;PRG file header
prg_magic       EQU $00 ;.w Ox601A (magic number)
prg_tlen        EQU $02 ;.l Size of text segment
prg_dlen        EQU $06 ;.l Size of data segment
prg_blen        EQU $0a ;.l Size of BSS segment
prg_slen        EQU $0e ;.l Size of symbol table
prg_reserved1   EQU $12 ;.l (reserved)
prg_reserved2   EQU $16 ;.l (reserved)
prg_reserved3   EQU $1a ;.l (reserved)
prg_text        EQU $1e ;(start of text segment)

                ;pea   0
                ;pea   0
                ;pea   null_long
                ;move.w   #5,-(sp)       ; PE_BASEPAGE
                ;move.w   #$4B,-(sp)
                ;trap   #1
                ;lea   16(sp),sp


                move.l  #-1,-(sp)       ; Return amount of largest chunk of RAM
                move.w  #72,-(sp)
                trap    #1              ; GEMDOS
                addq.l  #6,sp
                ;@TODO handle not enough memory error
                move.l  D0,-(sp)
                move.w  #72,-(sp)
                trap    #1              ; GEMDOS
                addq.l  #6,sp
                ;@TODO handle malloc error

                move.l  D0,A0
                lea     $100(A0),A0     ;reserve basepage
;               lea     payload,A1
                lea     'STRT',A1       ; will be filled later
;               lea     eop,A2
                lea     'END!',A2        ; will be filled later
.cl: 
                move.b  (A1)+,(A0)+
                cmp.l   A2,A1
                blt.s   .cl

;                lea     pbasepage(PC),A1
;                move.l  D0,(A1)
                move.l  d0,d7           ;save a copy of basepage address
                move.l  D0,A0
                lea     $100(A0),A1     ;prg header

                move.l  A0,p_lowtpa(A0)

                lea     $1c(A1),A1
                move.l  A1,p_tbase(A0)
                move.l  prg_tlen+$100(A0),p_tlen(A0)
                add.l   prg_tlen+$100(A0),A1

                move.l  A1,p_dbase(A0)
                move.l  prg_dlen+$100(A0),p_dlen(A0)
                add.l   prg_dlen+$100(A0),A1

                move.l  A1,p_bbase(A0)
                move.l  prg_blen+$100(A0),p_blen(A0)
                add.l   prg_blen+$100(A0),A1

                add.l   prg_slen+$100(A0),A1

                ;/from ultraDev (TOS1.02 by ultra, blame TOS1 "fix" to tIn)
                lea     emptyEnvStr(PC),A2
                move.l  A2,p_env(A0)
                ;clr.l  p_cmdlin(A0)    ;is this correct? better use an empty commandline just to be sure
                lea     emptyCommandline(PC),A2
                move.l  A2,p_cmdlin(A0)

                pea supervisor_code(pc)
                move.w #$26,-(sp)
                trap #14
                addq.l #6,sp
                bra past_supervisor_code

supervisor_code:
                move.l d7,a0

                movea.l $04F2.w,a2      ;get Sysbase
                movea.l 8(a2),a4        ;get rom start
                move.l  $28(a4),a2      ;this is not present in TOS<1.02!
;/---------------------
;1MB TOS1.0 crash!!!    
;   move.l  (a2),p_parent(A0)  ;<====== this does not work with TOS1.0, ROMSTART+$28 process pointer isn't present
;   move.l  A0,(a2)         ;set act pd 
;1MB TOS1.0 crash!!!    
;\_____________________
                ;START --------------------- TOS 1.0 workaround
                ;check if this is TOS1.0 - unfortunately TOS1.0 does not know the first thing about cookies
                tst.l   $5a0.w              ;cookie jar pointer
                bne.s   .thisisnottos10     ;!=0 ==> TOS did set this, so we're definitively not on TOS 1.0
                cmp.w   #$100,2(A4)
                beq.s   .thisistos10
                ;END ----------------------- TOS 1.0 workaround
.thisisnottos10:
                move.l  (a2),p_parent(A0)  ;<====== this does not work
                move.l  A0,(a2)         ;set act pd 
                bra.s   .no1
.thisistos10:
                ;START --------------------- TOS 1.0 workaround
                ;taken from BUGABOOs "init_all"
                lea     $602C.w,A2                  ;act_pd (vor dem Blitter-TOS)
                move.w  $1C(A4),D0                  ;os_conf holen
                lsr.w   #1,D0                       ;PAL/NTSC-Mode ignorieren
                subq.w  #4,D0                       ;Spanisches TOS 1.0?
                bne.s   .noESTOS10                  ;Nein! =>
                lea     $873C-$602C(A2),A2          ;act_pd des spanischen TOS 1.0
.noESTOS10:
                move.l  A0,(A2)
                ;END ----------------------- TOS 1.0 workaround
.no1:
                ;\from ultraDev


 ;@TODO: better stack handling
;                move.l  $44e.w,SP
;                move.l  SP,p_hitpa(A0)  ;SSP
;                lea     -10000(SP),A3   ;USP
;                move    #$700,sr

                rts

past_supervisor_code:
;                move.l  A3,SP


                move.l d7,a0

                ;/from ultraDev
                move.l  A0,-(a7)    ;put code page to stack
                clr.l   -(a7)       ;clear something ;)
                ;\from ultraDev

                ;move.l  pbasepage(PC),A0
                move.l  d7,A0
                lea     $100(A0),A0
                move.l  A0,A1
                bsr     kernel_relocate

                lea     0.w,A0 ;apparently we need to set this to zero? why? TPA address _should_ work?
                ;move.l  pbasepage(PC),A3
                move.l  d7,A3
                jmp     $100+$1c(A3)
;pbasepage:      DC.L    0
;=================================================
;                                       PRG-Header
kernel_prg_text_len:   EQU     2
kernel_prg_data_len:   EQU     2+4
kernel_prg_bss_len:    EQU     2+4+4
kernel_prg_symbol_len: EQU     2+4+4+4
kernel_prg_flags:      EQU     2+4+4+4+4+4 
kernel_prg_relocflag:  EQU     2+4+4+4+4+4+4 
;=================================================
;                                    PRG-Relocator
;=================================================
;A0=>Start PRG
;A1=>where's the PRG actually _started_?
;out: A1=>Start BSS
kernel_relocate:
                tst.w   kernel_prg_relocflag(A0)
                bne.s   .k_noreloc
                lea     $1c(A1),A3      ;address to relocate to
                move.l  A3,D2
                lea     $1c(A0),A3      
                move.l  A3,A2
                adda.l  kernel_prg_text_len(A0),A2
                adda.l  kernel_prg_data_len(A0),A2
                move.l  kernel_prg_bss_len(A0),D3
                move.l  A2,A1
                adda.l  kernel_prg_symbol_len(A0),A2
                move.l  (A2)+,D0
                beq.s   .k_noreloc
                moveq   #0,D1
.k_relocloop:   add.l   D2,0(A3,D0.l)
.k_relocloop2:  move.b  (A2)+,D1
                beq.s   .k_noreloc
                cmp.b   #1,D1
                beq.s   .k_reloclong
                add.l   D1,D0
                bra.s   .k_relocloop
.k_reloclong:   add.l   #254,D0
                bra.s   .k_relocloop2
.k_noreloc:   
                rts             
                ELSE
w:              bra.s   w               
                ENDC

;from ultraDev
emptyEnvStr: dc.b   "PATH=",0
    dc.b    "A:\",0,0
emptyCommandline: dc.b 0    
    even

;null_long:  dc.l 0

;clearScreen:
;                pea     cls(PC)
;                move.w  #9,-(SP)
;                trap    #1
;                addq.l  #6,SP
;                rts
;;A0: text
;;D0: X
;;D1: Y
;printText:
;               lea     gotox(PC),A1
;               add.w   #32,D0
;               add.w   #32,D1
;               move.b  D0,(A1)
;               move.b  D1,gotoy-gotox(A1)
;
;               pea     goto-gotox(A1)  ;move cursor
;               move.w  #9,-(SP)
;               trap    #1
;               addq.l  #6,SP
;
;               pea     (A0)            ;print text
;               move.w  #9,-(SP)
;               trap    #1
;               addq.l  #6,SP
;               rts
;cls:           DC.B 27,'E',0               
;goto:          DC.B 27,'Y'
;gotoy:          DC.B 0
;gotox:             DC.B 0
;               DC.B 0
;               EVEN

;               include "hwinfo.s"
;               EVEN
payload:                
                IFNE START_PAYLOAD
                ;incbin "ikaifill.tos"
                ENDC
eop:
