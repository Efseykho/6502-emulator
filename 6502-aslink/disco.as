; DISCO DISCO
; submitted by Anonymous
; shamelessly stolen from http://www.6502asm.com/

start:
 inx
 txa
 sta $200, y
 sta $300, y
 sta $400, y
 sta $500, y
 iny
 tya
 cmp 16
 bne do
 iny
 jmp start
do:
 iny
 iny
 iny
 iny
jmp start

