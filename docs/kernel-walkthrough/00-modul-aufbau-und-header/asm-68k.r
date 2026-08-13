* Wortwoertlicher Auszug aus src/kernel/kernel.r (eigener byte-exakter
* Nachbau von dker030s), Zeilen 20-97: kompletter Modul-Header (0x00-0x53)
* als Datenkonstante, dann der Einsprung bei 0x54 (M$Exec-Ziel) und der
* eingebettete ID-String, den die BRA.W ueberspringt.

L000000:
	dc.b	$4a,$fc,$00,$01
L000004:
	dc.b	$00,$00,$6f,$3c
L000008:
	dc.b	$00,$00,$00,$00
L00000c:
	dc.b	$00,$00,$6f,$30
L000010:
	dc.b	$05,$55,$0c,$01
L000014:
	dc.b	$a0,$00,$01,$77
L000018:
	dc.b	$00,$00,$00,$00
L00001c:
	dc.b	$00,$00,$00,$00
L000020:
	dc.b	$00,$00,$00,$00
L000024:
	dc.b	$00,$00,$00,$00
L000028:
	dc.b	$00,$00,$00,$00
L00002c:
	dc.b	$00,$00,$1d,$2d
L000030:
	dc.b	$00,$00,$00,$54
L000034:
	dc.b	$00,$00,$00,$00
L000038:
	dc.b	$00,$00,$00,$00
L00003c:
	dc.b	$00,$00,$00,$00
L000040:
	dc.b	$b0,$bd,$b0,$bd
L000044:
	dc.b	$00,$00,$00,$01
L000048:
	dc.b	$00,$00,$00,$01
L00004c:
	dc.b	$00,$00,$00,$00
L000050:
	dc.b	$00,$00,$00,$00

* --- Einsprung bei 0x54 (M$Exec = 0x54 zeigt genau hierher) ---
L000054:
	dc.w	$6000                        * BRA.W-Opcode
	dc.w	Q9_kernel_init_67a0-*        * Displacement = 0x674A -> Ziel 0x67a0 (Q9_kernel_init_67a0), ueberspringt den ID-String
L000058:
	dc.b	$00,$01,$09,$be              * 4 Byte, Zweck noch nicht geklaert (vor dem ID-String)
L00005c:
	dc.b	$36
L00005d:
	dc.b	$38
L00005e:
	dc.b	$30
L00005f:
	dc.b	$33
L000060:
	dc.b	$30
L000061:
	dc.b	$00                          * "68030\0"
L000062:
	dc.b	$20,$4f,$53,$2d,$39,$2f,$36,$38,$4b,$20,$4b,$65,$72,$6e,$65,$6c
	dc.b	$20,$28,$44,$65,$76,$2d,$53,$74,$64,$29,$20,$56,$33,$2e,$32,$2e
	dc.b	$30,$00                      * " OS-9/68K Kernel (Dev-Std) V3.2.0\0"
L000084:
	dc.b	$43,$6f,$70,$79,$72,$69,$67,$68,$74,$20,$28,$63,$29,$20,$31,$39
	dc.b	$39,$39,$20,$62,$79,$20,$4d,$69,$63,$72,$6f,$77,$61,$72,$65,$20
	dc.b	$53,$79,$73,$74,$65,$6d,$73,$20,$43,$6f,$72,$70,$2e,$00
	                                    * "Copyright (c) 1999 by Microware Systems Corp.\0"
* Ab hier (0xb2) beginnt Q9_post_idstring_b2 -- nicht mehr Teil dieses Themas.
