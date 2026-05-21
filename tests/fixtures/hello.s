; hello.s -- minimal Atari ST PRG used as the tiny test fixture for
; Epic 002 / Story 5 / T5.1. Prints a short message via GEMDOS Cconws
; ($09) and exits with Pterm0 ($00). The whole PRG ends up well under
; 1 KB, so compression will be a net loss -- that's intentional; this
; fixture exercises the auto-fallback path in `usm -z`.
;
; Build (the assembled output is checked in at tests/fixtures/hello.prg):
;     STCMD_NO_TTY=1 STCMD_QUIET=1 stcmd vasm -quiet -Ftos \
;         -o tests/fixtures/hello.prg tests/fixtures/hello.s

                TEXT

                pea     message(pc)
                move.w  #9,-(sp)            ; Cconws
                trap    #1                  ; GEMDOS
                addq.l  #6,sp

                clr.w   -(sp)               ; Pterm0
                trap    #1                  ; doesn't return

message:        dc.b    'Hello from a USM cart!',13,10,0
                even
