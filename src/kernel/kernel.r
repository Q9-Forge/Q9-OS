* Q9-OS eigener Nachbau des OS-9/68K-Kernels (dker030s), 68030,
* Development/Standard-Allocator. Byte-exakt per Disassemblierung
* rekonstruiert, siehe docs/REVERSE_ENGINEERING.md und docs/REBUILD.md.
* Eigenstaendige Quelle, keine Uebernahme von Microware-Originaltext.
*
* Gesamter Modul-Header (0x00-0x53, Standardteil + kernel-spezifische
* Erweiterung) wird hier bewusst 1:1 als Datenkonstante uebernommen --
* das sind reine Strukturfakten (Groesse, Typ/Sprache, Paritaet,
* Einsprungoffset), keine geschuetzte Werksausdruecke, und lassen sich
* wegen der noch ungeklaerten Bedeutung einzelner Erweiterungsfelder
* (siehe REVERSE_ENGINEERING.md, "M$Exec-Erweiterungsblock") derzeit
* nicht zuverlaessig automatisch durch r68/l68 erzeugen. Gebaut wird
* im l68-Rohbinaer-Modus (-r), der die automatische Header-Synthese
* umgeht und uns volle Byte-Kontrolle gibt.
*
* Koerper ab L000054 automatisch aus der Ghidra-Disassemblierung
* generiert (Werkzeug-Syntax-Uebersetzung, keine inhaltliche
* Neuerfindung), Labels vorerst generisch (LxxxxxX).

	nam	kernel
	psect	kernel,3073,40960,375,0,L000000,0

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
L00004a	EQU	L000048+2

* Sprungziele, die mitten in eine groessere Instruktion zeigen
* (klassischer 68k-Ueberlappungs-Trick).
L001b90	EQU	L001b8e+2
L001b9c	EQU	L001b9a+2
L001ba8	EQU	L001ba4+4
L0035ea	EQU	L0035e8+2
L0035f2	EQU	L0035f0+2

* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000054:
	dc.w	$6000
	dc.w	Q9_kernel_init_67a0-*
L000058:
	dc.b	$00,$01,$09,$be
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
	dc.b	$00
L000062:
	dc.b	$20,$4f,$53,$2d,$39,$2f,$36,$38,$4b,$20,$4b,$65,$72,$6e,$65,$6c
	dc.b	$20,$28,$44,$65,$76,$2d,$53,$74,$64,$29,$20,$56,$33,$2e,$32,$2e
	dc.b	$30,$00
L000084:
	dc.b	$43,$6f,$70,$79,$72,$69,$67,$68,$74,$20,$28,$63,$29,$20,$31,$39
	dc.b	$39,$39,$20,$62,$79,$20,$4d,$69,$63,$72,$6f,$77,$61,$72,$65,$20
	dc.b	$53,$79,$73,$74,$65,$6d,$73,$20,$43,$6f,$72,$70,$2e,$00
* TRAPF.L-Padding (6 Byte, Ausrichtung) gefolgt von einem indizierten Trampolin-Dispatch ueber Tabelle (0x8e4,A6).
Q9_post_idstring_b2:
	trapf.l	#$0
L0000b8:
	movem.l	a6/a2/d0,-(sp)
L0000bc:
	movec	vbr,a6
L0000c0:
	movea.l	(a6),a6
L0000c2:
	addq.l	#$1,$8bc(a6)
L0000c6:
	move.w	$e(sp),d0
L0000ca:
	pea	L0000de(pc)
L0000ce:
	movea.l	$8e4(a6),a2
L0000d2:
	lea	$0(a2,d0.w*1),a2
L0000d6:
	move.l	(a2),-(sp)
L0000d8:
	movea.l	$400(a2),a2
L0000dc:
	rts
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0000de:
	dc.w	$6500
	dc.w	L000172-*
L0000e2:
	btst.b	#$4,$16(sp)
L0000e8:
	beq.b	L00012e
L0000ea:
	btst.b	#$7,$16(sp)
L0000f0:
	bne.b	L00012e
L0000f2:
	tst.l	$8c0(a6)
L0000f6:
	bne.b	L000168
L0000f8:
	movec	msp,a2
L0000fc:
	btst.b	#$5,(a2)
L000100:
	ori	#$700,sr
L000104:
	movea.l	$4c(a6),a2
L000108:
	beq.b	L00013c
L00010a:
	cmpi.l	#$1,$8bc(a6)
L000112:
	bne.b	L000132
L000114:
	btst.b	#$5,$1c(a2)
L00011a:
	beq.b	L000132
L00011c:
	tst.l	$3ac(a2)
L000120:
	bne.b	L000132
L000122:
	movec	msp,a2
L000126:
	move.w	(a2),d0
L000128:
	andi.w	#$700,d0
L00012c:
	beq.b	L000158
L00012e:
	ori	#$700,sr
L000132:
	subq.l	#$1,$8bc(a6)
L000136:
	movem.l	(sp)+,d0/a2/a6/sp
L00013a:
	rte
L00013c:
	tst.w	$26(a2)
L000140:
	bne.b	L00014c
L000142:
	move.w	$1c(a2),d0
L000146:
	andi.w	#$7fff,d0
L00014a:
	beq.b	L000132
L00014c:
	movem.l	(sp)+,d0/a2
L000150:
	movem.l	a3/a2/a0/d1/d0,-(sp)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000154:
	dc.w	$6000
	dc.w	L000310-*
L000158:
	movem.l	(sp)+,d0/a2
L00015c:
	movem.l	a3/a2/a0/d1/d0,-(sp)
L000160:
	movea.l	$4c(a6),a3
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000164:
	dc.w	$6000
	dc.w	L000282-*
L000168:
	movem.l	(sp)+,d0/a2
L00016c:
	movem.l	a3/a2/a0/d1/d0,-(sp)
L000170:
	bra.b	L0001e4
L000172:
	movem.l	(sp)+,d0/a2
L000176:
	movem.l	a3/a2/a0/d1/d0,-(sp)
L00017a:
	bra.b	L00018e
L00017c:
	dc.b	$51
L00017d:
	dc.b	$fa
L00017e:
	dc.b	$00
L00017f:
	dc.b	$00
* IRQ-Dispatcher: Autovektoren 1-7 + User-Defined Vectors, verkettete Handler-Deskriptorlisten, Reschedule-Trigger.
Q9_disp_180:
	movem.l	a6/a3/a2/a0/d1/d0,-(sp)
L000184:
	movec	vbr,a6
L000188:
	movea.l	(a6),a6
L00018a:
	addq.l	#$1,$8bc(a6)
L00018e:
	move.w	$1a(sp),d0
L000192:
	cmpi.w	#$80,d0
L000196:
	bcc.b	L0001a2
L000198:
	lea	$0(a6,d0.w*1),a3
L00019c:
	adda.w	#$384,a3
L0001a0:
	bra.b	L0001a6
L0001a2:
	lea	-$5c(a6,d0.w*1),a3
L0001a6:
	movem.l	a6/a3/d0,-(sp)
L0001aa:
	movea.l	$0(a3),a3
L0001ae:
	move.l	a3,$4(sp)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0001b2:
	dc.w	$6700
	dc.w	L0002e4-*
L0001b6:
	movem.l	$4(a3),a0/a2/a3
L0001bc:
	jsr	(a0)
L0001be:
	movem.l	(sp),d0/a3/a6
L0001c2:
	bcs.b	L0001aa
L0001c4:
	move.b	#$0,$3b(a6)
L0001ca:
	adda.l	#$c,sp
L0001d0:
	btst.b	#$4,$22(sp)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0001d6:
	dc.w	$6700
	dc.w	L0002d6-*
L0001da:
	btst.b	#$7,$22(sp)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0001e0:
	dc.w	$6600
	dc.w	L0002d6-*
L0001e4:
	move.l	$8c0(a6),d0
L0001e8:
	beq.b	L00024a
L0001ea:
	movem.l	a6/a3/d0,-(sp)
L0001ee:
	move	sr,$2(sp)
L0001f2:
	movec	msp,a2
L0001f6:
	move.w	(a2),d1
L0001f8:
	andi.w	#$700,d1
L0001fc:
	ori.w	#$2000,d1
L000200:
	move.w	d1,$0(sp)
L000204:
	movea.l	d0,a3
L000206:
	bset.b	#$0,$c(a3)
L00020c:
	beq.b	L000216
L00020e:
	lea	$c(sp),sp
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000212:
	dc.w	$6000
	dc.w	L0002d6-*
L000216:
	movea.l	$8(a3),a6
L00021a:
	movea.l	$4(a3),a0
L00021e:
	move.l	a3,$4(sp)
L000222:
	move	$0(sp),sr
L000226:
	jsr	(a0)
L000228:
	ori	#$700,sr
L00022c:
	movem.l	$4(sp),a3/a6
L000232:
	move.l	$0(a3),d0
L000236:
	bclr.b	#$1,$c(a3)
L00023c:
	move.l	d0,$8c0(a6)
L000240:
	bne.b	L000204
L000242:
	move	$2(sp),sr
L000246:
	lea	$c(sp),sp
L00024a:
	movec	msp,a2
L00024e:
	btst.b	#$5,(a2)
L000252:
	ori	#$700,sr
L000256:
	movea.l	$4c(a6),a3
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00025a:
	dc.w	$6700
	dc.w	L000300-*
L00025e:
	cmpi.l	#$1,$8bc(a6)
L000266:
	bne.b	L0002da
L000268:
	btst.b	#$5,$1c(a3)
L00026e:
	beq.b	L0002da
L000270:
	tst.l	$3ac(a3)
L000274:
	bne.b	L0002da
L000276:
	movec	msp,a2
L00027a:
	move.w	(a2),d1
L00027c:
	andi.w	#$700,d1
L000280:
	bne.b	L0002da
L000282:
	bclr.b	#$5,$1c(a3)
L000288:
	cmpi.b	#$61,$20(a3)
L00028e:
	beq.b	L0002da
L000290:
	lea	$37c(a6),a0
L000294:
	cmpa.l	$30(a0),a0
L000298:
	bne.b	L0002a4
L00029a:
	move.w	$18(a3),d0
L00029e:
	cmp.w	$8a6(a6),d0
L0002a2:
	bcc.b	L0002d6
L0002a4:
	subq.l	#$1,$8bc(a6)
L0002a8:
	bsr.b	L000318
L0002aa:
	movem.l	a5/a4/a3/a2/a1/a0/d7/d6/d5/d4/d3/d2/d1/d0,-(sp)
L0002ae:
	movea.l	$4c(a6),a4
L0002b2:
	movea.l	a4,a0
L0002b4:
	addq.l	#$1,$3ac(a4)
L0002b8:
	andi	#-$701,sr
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0002bc:
	dc.w	$6100
	dc.w	Q9_scheduler_183a-*
L0002c0:
	move.l	sp,$8(a4)
L0002c4:
	move	usp,a0
L0002c6:
	move.l	a0,$c(a4)
L0002ca:
	ori	#$700,sr
L0002ce:
	subq.l	#$1,$3ac(a4)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0002d2:
	dc.w	$6000
	dc.w	Q9_reschedule_trampolin_3140-*
L0002d6:
	ori	#$700,sr
L0002da:
	subq.l	#$1,$8bc(a6)
L0002de:
	movem.l	(sp)+,d0/d1/a0/a2/a3/a6/sp
L0002e2:
	rte
L0002e4:
	addq.b	#$1,$3b(a6)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0002e8:
	dc.w	$6400
	dc.w	L0001ca-*
L0002ec:
	move	sr,d0
L0002ee:
	andi.w	#$700,d0
L0002f2:
	movec	msp,a2
L0002f6:
	andi.w	#-$701,(a2)
L0002fa:
	or.w	d0,(a2)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0002fc:
	dc.w	$6000
	dc.w	L0001ca-*
L000300:
	tst.w	$26(a3)
L000304:
	bne.b	L000310
L000306:
	move.w	$1c(a3),d0
L00030a:
	andi.w	#$7fff,d0
L00030e:
	beq.b	L0002da
L000310:
	subq.l	#$1,$8bc(a6)
L000314:
	pea	L000348(pc)
L000318:
	move.w	#$3700,$20(sp)
L00031e:
	movec	msp,a2
L000322:
	movem.l	$18(sp),d0/d1
L000328:
	movem.l	d1/d0,-(a2)
L00032c:
	move.l	$0(sp),$22(sp)
L000332:
	andi.w	#$fff,$26(sp)
L000338:
	movec	a2,msp
L00033c:
	movem.l	$4(sp),d0/d1/a0/a2/a3
L000342:
	lea	$20(sp),sp
L000346:
	rte
L000348:
	dc.b	$48
L000349:
	dc.b	$e7
L00034a:
	dc.b	$ff
L00034b:
	dc.b	$fc
L00034c:
	dc.b	$02
L00034d:
	dc.b	$7c
L00034e:
	dc.b	$f8
L00034f:
	dc.b	$ff
L000350:
	movea.l	$4c(a6),a4
L000354:
	bclr.b	#$5,$1c(a4)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00035a:
	dc.w	$6600
	dc.w	L000d8e-*
L00035e:
	btst.b	#$1,$1c(a4)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000364:
	dc.w	$6600
	dc.w	L000fc8-*
L000368:
	move.w	$26(a4),d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00036c:
	dc.w	$6600
	dc.w	L000de0-*
L000370:
	move.l	$2ac(a4),d0
L000374:
	beq.b	L0003f2
L000376:
	move.l	$2e8(a4),d2
L00037a:
	bpl.b	L0003f2
L00037c:
	pea	L0003f2(pc)
L000380:
	movea.l	d0,a0
L000382:
	movea.l	$8(a0),a0
L000386:
	movea.l	$20(a0),a0
L00038a:
	lea	$2ec(a4),a3
L00038e:
	bra.b	L0003c4
L000390:
	movea.l	(a0)+,a2
L000392:
	move.w	(a2),d0
L000394:
	cmp.w	(a3)+,d0
L000396:
	bne.b	L0003c4
L000398:
	move.w	#$4afc,(a2)
L00039c:
	move	sr,d0
L00039e:
	move.l	d0,-(sp)
L0003a0:
	ori	#$700,sr
L0003a4:
	movec	caar,d0
L0003a8:
	move.l	d0,-(sp)
L0003aa:
	movec	a2,caar
L0003ae:
	movec	cacr,d0
L0003b2:
	bset.l	#$2,d0
L0003b6:
	movec	d0,cacr
L0003ba:
	move.l	(sp)+,d0
L0003bc:
	movec	d0,caar
L0003c0:
	move.l	(sp)+,d0
L0003c2:
	move	d0,sr
L0003c4:
	dbf	d2,L000390
L0003c8:
	move.b	$3e0(a6),d0
L0003cc:
	btst.l	#$0,d0
L0003d0:
	beq.b	L0003d8
L0003d2:
	btst.l	#$1,d0
L0003d6:
	bne.b	L0003f0
L0003d8:
	moveq	#$44,d0
L0003da:
	move.l	a3,-(sp)
L0003dc:
	movea.l	$3a4(a6),a3
L0003e0:
	pea	L0003ee(pc)
L0003e4:
	move.l	$168(a3),-(sp)
L0003e8:
	movea.l	$568(a3),a3
L0003ec:
	rts
L0003ee:
	dc.b	$26
L0003ef:
	dc.b	$5f
L0003f0:
	rts
L0003f2:
	move.w	$3b0(a4),d0
L0003f6:
	sub.w	$3b2(a4),d0
L0003fa:
	cmpi.l	#$4a696d69,$0(a4,d0.w*1)
L000402:
	beq.b	L000408
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000404:
	dc.w	$6100
	dc.w	L0032c8-*
L000408:
	ori	#$700,sr
L00040c:
	bclr.b	#$5,$1c(a4)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000412:
	dc.w	$6600
	dc.w	L000d8a-*
L000416:
	tst.b	$2f(a6)
L00041a:
	beq.b	L000430
L00041c:
	move.l	$58(a6),d0
L000420:
	beq.b	L00042c
L000422:
	cmp.l	a4,d0
L000424:
	beq.b	L000430
L000426:
	movea.l	d0,a1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000428:
	dc.w	$6100
	dc.w	Q9_fpu_save_fe0-*
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00042c:
	dc.w	$6100
	dc.w	Q9_fpu_restore_1034-*
L000430:
	move.l	a3,-(sp)
L000432:
	movea.l	$3a4(a6),a3
L000436:
	pea	L000444(pc)
L00043a:
	move.l	$fc(a3),-(sp)
L00043e:
	movea.l	$4fc(a3),a3
L000442:
	rts
L000444:
	movea.l	(sp)+,a3
L000446:
	bclr.b	#$7,$1c(a4)
L00044c:
	movem.l	(sp)+,d0/d1/d2/d3/d4/d5/d6/d7/a0/a1/a2/a3/a4/a5/a6/sp
L000450:
	rte
* Spurious/Uninitialized-Interrupt-Handler (Vektoren 15, 24).
Q9_disp_452:
	move.l	a6,-(sp)
L000454:
	movec	vbr,a6
L000458:
	movea.l	(a6),a6
L00045a:
	addq.l	#$1,$84(a6)
L00045e:
	bcc.b	L000464
L000460:
	subq.l	#$1,$84(a6)
L000464:
	btst.b	#$6,$2e(a6)
L00046a:
	movea.l	(sp)+,a6
L00046c:
	beq.b	L000472
L00046e:
	addq.l	#$4,sp
L000470:
	rte
L000472:
	ori	#$700,sr
L000476:
	movem.l	a6/a5/a4/a3/a2/a1/a0/d7/d6/d5/d4/d3/d2/d1/d0,-(sp)
L00047a:
	movec	vbr,a6
L00047e:
	movea.l	(a6),a6
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000480:
	dc.w	$6000
	dc.w	L000804-*
L000484:
	dc.b	$51
L000485:
	dc.b	$fa
L000486:
	dc.b	$00
L000487:
	dc.b	$00
* TRAP #0 -- OS-9-Syscall-Dispatcher: liest Funktionsnummer aus dem Codestrom, Trampolin-Aufruf in eine der zwei Syscall-Tabellen.
Q9_disp_488:
	movem.l	a6/a5/a4/a3/a2/a1/a0/d7/d6/d5/d4/d3/d2/d1/d0,-(sp)
L00048c:
	movec	vbr,a6
L000490:
	movea.l	(a6),a6
L000492:
	movea.l	$4c(a6),a4
L000496:
	clr.b	$41(sp)
L00049a:
	movea.l	$42(sp),a5
L00049e:
	move.w	(a5),d7
L0004a0:
	addq.l	#$2,$42(sp)
L0004a4:
	cmpi.w	#$100,d7
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0004a8:
	dc.w	$6400
	dc.w	L0005bc-*
L0004ac:
	move.w	d7,$3c(sp)
L0004b0:
	move.l	$140(a4),d5
L0004b4:
	move.l	$144(a4),d6
L0004b8:
	movea.l	$3a4(a6),a3
L0004bc:
	bset.b	#$7,$1c(a4)
L0004c2:
	btst.b	#$5,$40(sp)
L0004c8:
	bne.b	L0004ec
L0004ca:
	move	usp,a3
L0004cc:
	move.l	a3,$c(a4)
L0004d0:
	move.l	sp,$8(a4)
L0004d4:
	move.b	d7,$21(a4)
L0004d8:
	movea.l	$3a8(a6),a3
L0004dc:
	cmpi.w	#$80,d7
L0004e0:
	bcc.b	L0004e8
L0004e2:
	addq.l	#$1,$2c4(a4)
L0004e6:
	bra.b	L0004f6
L0004e8:
	addq.l	#$1,$2c8(a4)
L0004ec:
	cmpi.w	#$80,d7
L0004f0:
	bcs.b	L0004f6
L0004f2:
	addq.l	#$1,$3ac(a4)
L0004f6:
	asl.w	#$2,d7
L0004f8:
	adda.w	d7,a3
L0004fa:
	movea.l	sp,a5
L0004fc:
	move.l	$3ac(a4),d7
L000500:
	movem.l	a6/a5/a4/d7/d6/d5,-(sp)
L000504:
	move.l	sp,$140(a4)
L000508:
	pea	L000558(pc)
L00050c:
	move.l	(sp),$144(a4)
L000510:
	move.l	(a3),-(sp)
L000512:
	movea.l	$400(a3),a3
L000516:
	rts
L000518:
	tst.l	$8bc(a6)
L00051c:
	bne.b	L000538
L00051e:
	tst.l	$3ac(a4)
L000522:
	bne.b	L000538
L000524:
	btst.b	#$5,$1c(a4)
L00052a:
	beq.b	L000538
L00052c:
	subq.l	#$8,sp
L00052e:
	moveq	#$1,d0
L000530:
	movea.l	sp,a5
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000532:
	dc.w	$6100
	dc.w	L003a06-*
L000536:
	addq.l	#$8,sp
L000538:
	move.w	$3b0(a4),d0
L00053c:
	sub.w	$3b2(a4),d0
L000540:
	cmpi.l	#$4a696d69,$0(a4,d0.w*1)
L000548:
	beq.b	L00054e
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00054a:
	dc.w	$6100
	dc.w	L0032c8-*
L00054e:
	movem.l	(sp)+,d0/d1/d2/d3/d4/d5/d6/d7/a0/a1/a2/a3/a4/a5/a6/sp
L000552:
	rte
L000554:
	trapf.w	#$0
L000558:
	movem.l	(sp)+,d5/d6/d7/a4/a5/a6
L00055c:
	bcc.b	L00056c
L00055e:
	ori.w	#$1,$40(sp)
L000564:
	clr.w	$4(sp)
L000568:
	move.w	d1,$6(sp)
L00056c:
	move.l	d5,$140(a4)
L000570:
	move.l	d6,$144(a4)
L000574:
	move.l	d7,$3ac(a4)
L000578:
	cmpi.w	#$80,$3c(sp)
L00057e:
	bcs.b	L000584
L000580:
	subq.l	#$1,$3ac(a4)
L000584:
	btst.b	#$5,$40(sp)
L00058a:
	bne.b	L000518
L00058c:
	tst.l	$2ac(a4)
L000590:
	beq.b	L000598
L000592:
	bset.b	#$5,$1c(a4)
L000598:
	movea.l	$c(a4),a0
L00059c:
	move	a0,usp
L00059e:
	btst.b	#$7,$40(sp)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0005a4:
	dc.w	$6700
	dc.w	L000350-*
L0005a8:
	tst.w	$26(a4)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0005ac:
	dc.w	$6600
	dc.w	L000350-*
L0005b0:
	tst.l	$2ac(a4)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0005b4:
	dc.w	$6700
	dc.w	L000350-*
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0005b8:
	dc.w	$6000
	dc.w	L000bb6-*
L0005bc:
	move.w	#$d0,$6(sp)
L0005c2:
	ori.w	#$1,$40(sp)
L0005c8:
	bra.b	L000584
L0005ca:
	dc.b	$51
L0005cb:
	dc.b	$fb
L0005cc:
	dc.b	$00
L0005cd:
	dc.b	$00
L0005ce:
	dc.b	$00
L0005cf:
	dc.b	$00
* TRAP #1-15 -- Dispatcher fuer prozesseigene, selbst installierte Trap-Handler.
Q9_disp_5d0:
	movem.l	a6/a1/a0/d1/d0,-(sp)
L0005d4:
	move.w	$16(sp),d1
L0005d8:
	movec	vbr,a6
L0005dc:
	movea.l	(a6),a6
L0005de:
	movea.l	$4c(a6),a6
L0005e2:
	move.l	$8(a6,d1.w*1),d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0005e6:
	dc.w	$6700
	dc.w	L00067e-*
L0005ea:
	move	usp,a0
L0005ec:
	move.l	a0,$c(a6)
L0005f0:
	movea.l	d0,a0
L0005f2:
	adda.w	#$44,a6
L0005f6:
	movea.l	$0(a6,d1.w*1),a6
L0005fa:
	btst.b	#$5,$14(a0)
L000600:
	adda.l	$30(a0),a0
L000604:
	beq.b	L000652
L000606:
	move.l	(sp),d0
L000608:
	move.l	a0,$0(sp)
L00060c:
	lea	L000634(pc),a0
L000610:
	move.l	a0,$c(sp)
L000614:
	movea.l	$1a(sp),a0
L000618:
	move.w	(a0)+,$14(sp)
L00061c:
	move.l	a0,$1a(sp)
L000620:
	movem.l	$4(sp),d1/a0
L000626:
	move.l	$14(sp),$8(sp)
L00062c:
	move.l	$10(sp),$4(sp)
L000632:
	rts
L000634:
	movec	vbr,a6
L000638:
	movea.l	(a6),a6
L00063a:
	movem.l	a5/a4/a3/a2/a1/a0/d7/d6/d5/d4/d3/d2/d1/d0,-(sp)
L00063e:
	move	sr,d0
L000640:
	move.b	d0,$41(sp)
L000644:
	movea.l	$4c(a6),a4
L000648:
	movea.l	$c(a4),a0
L00064c:
	move	a0,usp
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00064e:
	dc.w	$6000
	dc.w	L000350-*
L000652:
	movea.l	$1a(sp),a1
L000656:
	move.l	a0,$1a(sp)
L00065a:
	move.w	(a1)+,d0
L00065c:
	move	usp,a0
L00065e:
	move.l	a1,-(a0)
L000660:
	move.w	d1,-(a0)
L000662:
	move.w	d0,-(a0)
L000664:
	move.l	$10(sp),-(a0)
L000668:
	move	a0,usp
L00066a:
	movem.l	(sp)+,d0/d1/a0/a1
L00066e:
	addq.l	#$4,sp
L000670:
	btst.b	#$7,$4(sp)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000676:
	dc.w	$6600
	dc.w	Q9_disp_ba4-*
L00067a:
	addq.l	#$4,sp
L00067c:
	rte
L00067e:
	movea.l	$38(a6),a0
L000682:
	movea.l	$10(sp),a6
L000686:
	move.l	$34(a0),d0
L00068a:
	adda.l	d0,a0
L00068c:
	bne.b	L000652
L00068e:
	addq.l	#$2,$1a(sp)
L000692:
	movem.l	(sp)+,d0/d1/a0/a1/a6
L000696:
	move.l	#$85,d1
L00069c:
	ori.b	#$1,$5(sp)
L0006a2:
	bra.b	L000670
L0006a4:
	dc.b	$51
L0006a5:
	dc.b	$fa
L0006a6:
	dc.b	$00
L0006a7:
	dc.b	$00
* Periodischer Uhr-Tick-Handler: Systemzeit-Fortschreibung, Zeitscheiben-Ablauf-Erkennung des Schedulers.
Q9_clock_tick_6a8:
	addq.l	#$1,$54(a6)
L0006ac:
	subq.w	#$1,$774(a6)
L0006b0:
	bne.b	L0006ce
L0006b2:
	move.w	$28(a6),$774(a6)
L0006b8:
	subq.l	#$1,$34(a6)
L0006bc:
	bhi.b	L0006ce
L0006be:
	move.l	#$15180,$34(a6)
L0006c6:
	addq.l	#$1,$30(a6)
L0006ca:
	clr.l	$2a(a6)
L0006ce:
	move	sr,d0
L0006d0:
	ori	#$700,sr
L0006d4:
	subq.l	#$1,$77c(a6)
L0006d8:
	bhi.b	L0006e2
L0006da:
	bcs.b	L0006de
L0006dc:
	bsr.b	Q9_clock_hook_install_70c
L0006de:
	clr.l	$77c(a6)
L0006e2:
	move	d0,sr
L0006e4:
	movea.l	$4c(a6),a2
L0006e8:
	btst.b	#$7,$1c(a2)
L0006ee:
	bne.b	L0006f6
L0006f0:
	addq.l	#$1,$2b4(a2)
L0006f4:
	bra.b	L0006fa
L0006f6:
	addq.l	#$1,$2b8(a2)
L0006fa:
	subq.w	#$1,$778(a6)
L0006fe:
	bhi.b	L00070a
L000700:
	addq.w	#$1,$778(a6)
L000704:
	bset.b	#$5,$1c(a2)
L00070a:
	rts
* Selbstregistrierung als IRQ-Dispatcher-Tick-Hook (Scheduler-Tick).
Q9_clock_hook_install_70c:
	movem.l	a6/a1/a0/d7/d0,-(sp)
L000710:
	move	sr,d7
L000712:
	ori	#$700,sr
L000716:
	lea	$900(a6),a0
L00071a:
	bset.b	#$1,$c(a0)
L000720:
	bne.b	L000766
L000722:
	lea	L004550(pc),a1
L000726:
	move.l	a1,$4(a0)
L00072a:
	move.l	a6,$8(a0)
L00072e:
	move.l	#$0,$0(a0)
L000736:
	bclr.b	#$0,$c(a0)
L00073c:
	bra.b	L00074e
L00073e:
	dc.b	$48
L00073f:
	dc.b	$e7
L000740:
	dc.b	$81
L000741:
	dc.b	$c2
L000742:
	dc.b	$40
L000743:
	dc.b	$c7
L000744:
	dc.b	$00
L000745:
	dc.b	$7c
L000746:
	dc.b	$07
L000747:
	dc.b	$00
L000748:
	dc.b	$4e
L000749:
	dc.b	$7a
L00074a:
	dc.b	$e8
L00074b:
	dc.b	$01
L00074c:
	dc.b	$2c
L00074d:
	dc.b	$56
L00074e:
	move.l	$8c0(a6),d0
L000752:
	bne.b	L00075a
L000754:
	move.l	a0,$8c0(a6)
L000758:
	bra.b	L000766
L00075a:
	movea.l	d0,a1
L00075c:
	move.l	$0(a1),d0
L000760:
	bne.b	L00075a
L000762:
	move.l	a0,$0(a1)
L000766:
	move	d7,sr
L000768:
	movem.l	(sp)+,d0/d7/a0/a1/a6
L00076c:
	rts
L00076e:
	dc.b	$51
L00076f:
	dc.b	$fc
L000770:
	dc.b	$45,$72,$72,$6f,$72,$3a,$20,$73,$79,$73,$74,$65,$6d,$20,$73,$74
	dc.b	$61,$74,$65,$20,$65,$78,$63,$65,$70,$74,$69,$6f,$6e,$3b,$20,$76
	dc.b	$65,$63,$74,$6f,$72,$20,$24,$00
L000798:
	dc.b	$20,$20,$61,$74,$20,$61,$64,$64,$72,$20,$24,$00
L0007a4:
	dc.b	$2d,$2d,$3e,$20,$53,$79,$73,$74,$65,$6d,$20,$52,$65,$73,$65,$74
	dc.b	$20,$3c,$2d,$2d,$00
L0007b9:
	dc.b	$07
L0007ba:
	dc.b	$0d
L0007bb:
	dc.b	$0a
L0007bc:
	dc.b	$00
L0007bd:
	dc.b	$00
* Rettungsanker: bricht einen unterbrochenen internen Trampolin-Aufruf kontrolliert ab, statt in Panik zu enden.
Q9_trampolin_rescue_7be:
	ori	#$700,sr
L0007c2:
	cmpi.w	#$4afc,$0(a6)
L0007c8:
	bne.b	L000804
L0007ca:
	move	sr,d0
L0007cc:
	btst.l	#$c,d0
L0007d0:
	beq.b	L000804
L0007d2:
	movea.l	$4c(a6),a4
L0007d6:
	move.l	$140(a4),d0
L0007da:
	movea.l	$144(a4),a0
L0007de:
	beq.b	L000804
L0007e0:
	move.l	d7,d1
L0007e2:
	lsr.w	#$2,d1
L0007e4:
	addi.w	#$64,d1
L0007e8:
	ori	#$1,ccr
L0007ec:
	movea.l	d0,sp
L0007ee:
	andi	#-$701,sr
L0007f2:
	jmp	(a0)
L0007f4:
	movea.l	d0,a0
* Panik-/Diagnose-Reporter mit zwei Einstiegspunkten -- protokolliert und kehrt zurueck, haelt das System nicht an.
Q9_panic_report_7f6:
	move	sr,-(sp)
L0007f8:
	movem.l	sp/a6/a5/a4/a3/a2/a1/a0/d7/d6/d5/d4/d3/d2/d1/d0,-(sp)
L0007fc:
	ori	#$700,sr
L000800:
	bsr.b	L00084a
L000802:
	bra.b	L00081e
L000804:
	lea	L000770(pc),a0
L000808:
	bsr.b	Q9_console_puts_850
L00080a:
	move.l	$3c(sp),d0
L00080e:
	bsr.b	Q9_console_puthex_868
L000810:
	lea	L000798(pc),a0
L000814:
	bsr.b	Q9_console_puts_850
L000816:
	move.l	$42(sp),d0
L00081a:
	bsr.b	L000860
L00081c:
	bsr.b	L00084c
L00081e:
	lea	L0007a4(pc),a0
L000822:
	bsr.b	L00084a
L000824:
	move.l	$8ec(a6),$3e(sp)
L00082a:
	bsr.b	Q9_panic_delay_834
L00082c:
	movem.l	(sp)+,d0/d1/d2/d3/d4/d5/d6/d7/a0/a1/a2/a3/a4/a5/a6
L000830:
	addq.l	#$2,sp
L000832:
	rts
* Verzoegerungs-/Timeout-Schleife, pollt auf Konsolen-Bereitschaft.
Q9_panic_delay_834:
	move.l	#$320000,d0
L00083a:
	btst.b	d0,$1(sp)
L00083e:
	dbf	d0,L00083a
L000842:
	addq.w	#$1,d0
L000844:
	subq.l	#$1,d0
L000846:
	bcc.b	L00083a
L000848:
	rts
L00084a:
	bsr.b	Q9_console_puts_850
L00084c:
	lea	L0007b9(pc),a0
* Gibt einen nullterminierten ASCII-String auf der Systemkonsole aus.
Q9_console_puts_850:
	movea.l	$64(a6),a1
L000854:
	bra.b	L00085a
L000856:
	jsr	$8(a1)
L00085a:
	move.b	(a0)+,d0
L00085c:
	bne.b	L000856
L00085e:
	rts
L000860:
	move.w	d0,-(sp)
L000862:
	swap	d0
L000864:
	bsr.b	Q9_console_puthex_868
L000866:
	move.w	(sp)+,d0
* Gibt einen 32-Bit-Wert hexadezimal aus (rekursiv, ein Nibble pro Aufruf ueber ROR.L).
Q9_console_puthex_868:
	ror.l	#$8,d0
L00086a:
	bsr.b	L00086e
L00086c:
	rol.l	#$8,d0
L00086e:
	ror.l	#$4,d0
L000870:
	bsr.b	L000874
L000872:
	rol.l	#$4,d0
L000874:
	andi.b	#$f,d0
L000878:
	cmpi.b	#$9,d0
L00087c:
	bls.b	L000880
L00087e:
	addq.b	#$7,d0
L000880:
	addi.b	#$30,d0
L000884:
	jmp	$8(a1)
* Bus/Address-Error-Handler: sichert Fault-Frame-Zusatzfelder, faellt dann durch in Q9_disp_8d0.
Q9_disp_888:
	ori	#$700,sr
L00088c:
	movem.l	a6/a5/a4/a3/a2/a1/a0/d7/d6/d5/d4/d3/d2/d1/d0,-(sp)
L000890:
	movec	vbr,a6
L000894:
	movea.l	(a6),a6
L000896:
	moveq	#$0,d3
L000898:
	moveq	#$0,d5
L00089a:
	move.w	$4a(sp),d3
L00089e:
	move.l	$50(sp),d4
L0008a2:
	move.l	$4c(sp),d5
L0008a6:
	move.l	$3c(sp),d7
L0008aa:
	btst.b	#$5,$40(sp)
L0008b0:
	bne.b	L0008be
L0008b2:
	tst.b	$2f(a6)
L0008b6:
	beq.b	L0008be
L0008b8:
	cmpi.w	#$8,d7
L0008bc:
	beq.b	L0008ea
L0008be:
	bra.b	L0008de
L0008c0:
	dc.b	$00
L0008c1:
	dc.b	$00
L0008c2:
	dc.b	$04
L0008c3:
	dc.b	$04
L0008c4:
	dc.b	$08
L0008c5:
	dc.b	$00
L0008c6:
	dc.b	$00
L0008c7:
	dc.b	$34
L0008c8:
	dc.b	$32
L0008c9:
	dc.b	$0c
L0008ca:
	dc.b	$18
L0008cb:
	dc.b	$54
L0008cc:
	dc.b	$10
L0008cd:
	dc.b	$00
L0008ce:
	dc.b	$00
L0008cf:
	dc.b	$1a
* Sammel-Handler fuer Illegal Instr/Zero Div/CHK/TRAPV/Priv.Violation/Line-A-F/FPU/MMU: Software-Breakpoints, generisches Vektor-Handler-System, Signal-Zustellung.
Q9_disp_8d0:
	movem.l	a6/a5/a4/a3/a2/a1/a0/d7/d6/d5/d4/d3/d2/d1/d0,-(sp)
L0008d4:
	movec	vbr,a6
L0008d8:
	movea.l	(a6),a6
L0008da:
	move.l	$3c(sp),d7
L0008de:
	cmpi.w	#$c0,d7
L0008e2:
	bcs.b	L00091c
L0008e4:
	cmpi.w	#$d8,d7
L0008e8:
	bhi.b	L00091c
L0008ea:
	movea.l	$4c(a6),a1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0008ee:
	dc.w	$6100
	dc.w	Q9_fpu_save_fe0-*
L0008f2:
	cmpi.b	#$2,$2f(a6)
L0008f8:
	bne.b	L00091c
L0008fa:
	moveq	#$0,d0
L0008fc:
	tst.b	$74(a1)
L000900:
	beq.b	L00091c
L000902:
	move.b	$75(a1),d0
L000906:
	cmpi.b	#$38,d0
L00090a:
	beq.b	L000912
L00090c:
	clr.l	$74(a1)
L000910:
	bra.b	L00091c
* Vollformat-Adressierung erzwungen -- siehe FULL_EXT_OVERRIDE im Konverter
L000912:
	bset.b	#$3,($74,a1,d0.w*1)
L00091c:
	btst.b	#$5,$40(sp)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000922:
	dc.w	$6600
	dc.w	Q9_trampolin_rescue_7be-*
L000926:
	movea.l	$4c(a6),a4
L00092a:
	move.w	$46(sp),d0
L00092e:
	andi.w	#-$1000,d0
L000932:
	rol.w	#$4,d0
L000934:
	cmpi.b	#$2,d0
L000938:
	beq.b	L000940
L00093a:
	cmpi.b	#$9,d0
L00093e:
	bne.b	L000944
L000940:
	move.l	$48(sp),d4
L000944:
	move.b	L0008c0(pc,d0.w*1),d0
L00094a:
	beq.b	L00095a
L00094c:
	moveq	#$46,d2
L00094e:
	movea.l	sp,a0
L000950:
	lea	$0(sp,d0.w*1),a2
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000954:
	dc.w	$6100
	dc.w	L0030a0-*
L000958:
	movea.l	a2,sp
L00095a:
	clr.w	$46(sp)
L00095e:
	cmpi.w	#$10,d7
L000962:
	bne.b	L0009cc
L000964:
	move.l	$2ac(a4),d0
L000968:
	beq.b	L000994
L00096a:
	move.l	$2e8(a4),d1
L00096e:
	bpl.b	L000994
L000970:
	movea.l	d0,a0
L000972:
	movea.l	$8(a0),a5
L000976:
	movea.l	$20(a5),a1
L00097a:
	move.l	$42(sp),d0
L00097e:
	bra.b	L000982
L000980:
	cmp.l	(a1)+,d0
L000982:
	dbeq	d1,L000980
L000986:
	bne.b	L000994
L000988:
	move.l	#$1,$8(a5)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000990:
	dc.w	$6000
	dc.w	L000c74-*
L000994:
	movea.l	$42(sp),a0
L000998:
	move.w	(a0)+,d0
L00099a:
	move.w	d0,d1
L00099c:
	andi.w	#-$40,d1
L0009a0:
	cmpi.w	#$40c0,d1
L0009a4:
	bne.b	L0009d2
L0009a6:
	movea.l	sp,a5
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0009a8:
	dc.w	$6100
	dc.w	Q9_fpsp_handler_b04-*
L0009ac:
	bcs.b	L0009d2
L0009ae:
	btst.b	#$7,$40(sp)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0009b4:
	dc.w	$6700
	dc.w	L000350-*
L0009b8:
	tst.w	$26(a4)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0009bc:
	dc.w	$6600
	dc.w	L000350-*
L0009c0:
	tst.l	$2ac(a4)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0009c4:
	dc.w	$6700
	dc.w	L000350-*
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0009c8:
	dc.w	$6000
	dc.w	L000bb6-*
L0009cc:
	cmpi.w	#$20,d7
L0009d0:
	beq.b	L000994
L0009d2:
	cmpi.w	#$8,d7
L0009d6:
	bcs.b	L000a08
L0009d8:
	movea.l	$34(a4,d7.w*1),a1
L0009dc:
	movea.l	$5c(a4,d7.w*1),a5
L0009e0:
	cmpi.w	#$2c,d7
L0009e4:
	bls.b	L000a04
L0009e6:
	cmpi.w	#$c0,d7
L0009ea:
	bcs.b	L000a08
L0009ec:
	cmpi.w	#$d8,d7
L0009f0:
	bhi.b	L000a08
L0009f2:
	move.l	d7,d1
L0009f4:
	addi.w	#$278,d1
L0009f8:
	movea.l	$0(a4,d1.w*1),a1
L0009fc:
	addi.w	#$1c,d1
L000a00:
	movea.l	$0(a4,d1.w*1),a5
L000a04:
	move.l	a1,d1
L000a06:
	bne.b	L000a44
L000a08:
	move.l	d7,d1
L000a0a:
	lsr.w	#$2,d1
L000a0c:
	addi.w	#$64,d1
L000a10:
	tst.l	$2ac(a4)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000a14:
	dc.w	$6700
	dc.w	Q9_exc_no_handler_fc4-*
L000a18:
	movea.l	$2ac(a4),a0
L000a1c:
	movea.l	$8(a0),a5
L000a20:
	cmpi.w	#$c,d7
L000a24:
	bhi.b	L000a2c
L000a26:
	movem.l	d3/d4/d5,$c(a5)
L000a2c:
	move.w	d1,$6(a5)
L000a30:
	ori.b	#$1,$41(a5)
L000a36:
	move.l	d7,$8(a5)
L000a3a:
	bset.b	#$1,$1c(a4)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000a40:
	dc.w	$6000
	dc.w	L000c74-*
L000a44:
	addq.l	#$1,$3ac(a4)
L000a48:
	move.l	a5,d1
L000a4a:
	bne.b	L000a4e
L000a4c:
	move	usp,a5
L000a4e:
	lea	-$46(a5),a5
L000a52:
	movea.l	a5,a2
L000a54:
	moveq	#$46,d0
L000a56:
	moveq	#$2,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000a58:
	dc.w	$6100
	dc.w	Q9_range_check_wrap_134a-*
L000a5c:
	bcc.b	L000a6c
L000a5e:
	move.w	#$a6,d1
L000a62:
	subq.l	#$1,$3ac(a4)
L000a66:
	ori	#$1,ccr
L000a6a:
	bra.b	L000a10
L000a6c:
	movem.l	(sp)+,d0/d1/d2/d6
L000a70:
	movem.l	d0/d1/d2/d6,$0(a5)
L000a76:
	movem.l	(sp)+,d0/d1/d2/d6
L000a7a:
	movem.l	d0/d1/d2/d6,$10(a5)
L000a80:
	movem.l	(sp)+,d0/d1/d2/d6
L000a84:
	movem.l	d0/d1/d2/d6,$20(a5)
L000a8a:
	movem.l	(sp)+,d0/d1/d2/d6
L000a8e:
	movem.l	d0/d1/d2,$30(a5)
L000a94:
	move.w	$0(sp),$40(a5)
L000a9a:
	movea.l	$2(sp),a0
L000a9e:
	move.l	a0,$42(a5)
L000aa2:
	move.l	a1,$2(sp)
L000aa6:
	move	usp,a1
L000aa8:
	move.l	a1,$3c(a5)
L000aac:
	move	a5,usp
L000aae:
	movea.l	$32c(a4),a6
L000ab2:
	adda.l	#$8000,a6
L000ab8:
	subq.l	#$1,$3ac(a4)
L000abc:
	movem.l	a6/a4/a1/d0,-(sp)
L000ac0:
	move	sr,$0(sp)
L000ac4:
	movec	vbr,a6
L000ac8:
	movea.l	(a6),a6
L000aca:
	movea.l	$4c(a6),a4
L000ace:
	ori	#$700,sr
L000ad2:
	tst.b	$2f(a6)
L000ad6:
	beq.b	L000af0
L000ad8:
	move.l	$58(a6),d0
L000adc:
	beq.b	L000ae8
L000ade:
	cmp.l	a4,d0
L000ae0:
	beq.b	L000ae8
L000ae2:
	movea.l	d0,a1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000ae4:
	dc.w	$6100
	dc.w	Q9_fpu_save_fe0-*
L000ae8:
	frestore	L000050(pc)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000aec:
	dc.w	$6100
	dc.w	Q9_fpu_restore_1034-*
L000af0:
	move	$0(sp),sr
L000af4:
	movem.l	(sp)+,d0/a1/a4/a6
L000af8:
	btst.b	#$7,(sp)
L000afc:
	beq.b	L000b02
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000afe:
	dc.w	$6100
	dc.w	Q9_signal_pending_bc0-*
L000b02:
	rte
* FPSP-Handler (Floating-Point Software Package) fuer Vektor 48 (Branch/Set on Unordered), nutzt den EA-Decoder bei 0xb3a.
Q9_fpsp_handler_b04:
	move	usp,a1
L000b06:
	move.l	a1,$3c(a5)
L000b0a:
	move.w	d0,d1
L000b0c:
	andi.w	#$7,d0
L000b10:
	asl.w	#$2,d0
L000b12:
	andi.w	#$38,d1
L000b16:
	asr.w	#$2,d1
L000b18:
	move.w	L000b3a(pc,d1.w*1),d1
L000b1c:
	jsr	L000b3a(pc,d1.w*1)
L000b20:
	bcs.b	L000b38
L000b22:
	moveq	#$0,d0
L000b24:
	move.b	$41(a5),d0
L000b28:
	move.w	d0,(a1)
L000b2a:
	move.l	a0,$42(a5)
L000b2e:
	movea.l	$3c(a5),a1
L000b32:
	move	a1,usp
L000b34:
	move.l	a1,$c(a4)
L000b38:
	rts
L000b3a:
	dc.b	$00
L000b3b:
	dc.b	$10
L000b3c:
	dc.b	$00
L000b3d:
	dc.b	$64
L000b3e:
	dc.b	$00
L000b3f:
	dc.b	$1a
L000b40:
	dc.b	$00
L000b41:
	dc.b	$20
L000b42:
	dc.b	$00
L000b43:
	dc.b	$16
L000b44:
	dc.b	$00
L000b45:
	dc.b	$2a
L000b46:
	dc.b	$00
L000b47:
	dc.b	$32
L000b48:
	dc.b	$00
L000b49:
	dc.b	$54
L000b4a:
	lea	$2(a5,d0.w*1),a1
L000b4e:
	rts
L000b50:
	subq.l	#$2,$20(a5,d0.w*1)
L000b54:
	movea.l	$20(a5,d0.w*1),a1
L000b58:
	rts
L000b5a:
	movea.l	$20(a5,d0.w*1),a1
L000b5e:
	addq.l	#$2,$20(a5,d0.w*1)
L000b62:
	rts
L000b64:
	movea.l	$20(a5,d0.w*1),a1
L000b68:
	adda.w	(a0)+,a1
L000b6a:
	rts
L000b6c:
	movea.l	$20(a5,d0.w*1),a1
L000b70:
	moveq	#$0,d0
L000b72:
	move.b	(a0)+,d0
L000b74:
	move.b	(a0)+,d1
L000b76:
	ext.w	d1
L000b78:
	asr.w	#$2,d0
L000b7a:
	bchg.l	#$1,d0
L000b7e:
	bne.b	L000b86
L000b80:
	adda.w	$0(a5,d0.w*1),a1
L000b84:
	bra.b	L000b8a
L000b86:
	adda.l	$0(a5,d0.w*1),a1
L000b8a:
	adda.w	d1,a1
L000b8c:
	rts
L000b8e:
	cmpi.w	#$4,d0
L000b92:
	bhi.b	L000b9e
L000b94:
	beq.b	L000b9a
L000b96:
	movea.w	(a0)+,a1
L000b98:
	rts
L000b9a:
	movea.l	(a0)+,a1
L000b9c:
	rts
L000b9e:
	ori	#$1,ccr
L000ba2:
	rts
* Trace-Exception-Handler (Single-Step-Debugging), minimaler Epilog.
Q9_disp_ba4:
	btst.b	#$5,$4(sp)
L000baa:
	beq.b	Q9_signal_pending_bc0
L000bac:
	bclr.b	#$7,$4(sp)
L000bb2:
	addq.l	#$4,sp
L000bb4:
	rte
L000bb6:
	bclr.b	#$7,$1c(a4)
L000bbc:
	movem.l	(sp)+,d0/d1/d2/d3/d4/d5/d6/d7/a0/a1/a2/a3/a4/a5/a6
* Signal-/Breakpoint-Pending-Verwaltung mit Prioritaets-Aging der Deskriptorkette.
Q9_signal_pending_bc0:
	movem.l	a6/a5/a4,-(sp)
L000bc4:
	movec	vbr,a6
L000bc8:
	movea.l	(a6),a6
L000bca:
	movea.l	$4c(a6),a5
L000bce:
	move.w	$1c(a5),$0(sp)
L000bd4:
	tst.w	$26(a5)
L000bd8:
	bne.b	L000be2
L000bda:
	cmpi.b	#$61,$20(a5)
L000be0:
	beq.b	L000be8
L000be2:
	bset.b	#$4,$0(sp)
L000be8:
	addq.l	#$1,$2b0(a5)
L000bec:
	movea.l	$2ac(a5),a6
L000bf0:
	tst.w	$26(a6)
L000bf4:
	movea.l	$8(a6),a6
L000bf8:
	beq.b	L000c04
L000bfa:
	move.l	#$3,$8(a6)
L000c02:
	bra.b	L000c5c
L000c04:
	tst.w	$2ea(a5)
L000c08:
	beq.b	L000c32
L000c0a:
	movem.l	a6/d1/d0,-(sp)
L000c0e:
	move.w	$2ea(a5),d1
L000c12:
	move.l	$1e(sp),d0
L000c16:
	movea.l	$20(a6),a6
L000c1a:
	subq.w	#$1,d1
L000c1c:
	cmp.l	(a6)+,d0
L000c1e:
	dbcc	d1,L000c1c
L000c22:
	movem.l	(sp)+,d0/d1/a6
L000c26:
	bne.b	L000c32
L000c28:
	move.l	#$1,$8(a6)
L000c30:
	bra.b	L000c5c
L000c32:
	subq.l	#$1,$4(a6)
L000c36:
	bhi.b	L000c3e
L000c38:
	beq.b	L000c5c
L000c3a:
	addq.l	#$1,$4(a6)
L000c3e:
	andi.l	#$7fff0000,(sp)+
L000c44:
	bne.b	L000c4e
L000c46:
	movem.l	(sp)+,a5/a6
L000c4a:
	addq.l	#$4,sp
L000c4c:
	rte
L000c4e:
	movem.l	a4/a3/a2/a1/a0/d7/d6/d5/d4/d3/d2/d1/d0,-(sp)
L000c52:
	movec	vbr,a6
L000c56:
	movea.l	(a6),a6
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000c58:
	dc.w	$6000
	dc.w	L000350-*
L000c5c:
	addq.l	#$4,sp
L000c5e:
	movem.l	a4/a3/a2/a1/a0/d7/d6/d5/d4/d3/d2/d1/d0,-(sp)
L000c62:
	movec	vbr,a6
L000c66:
	movea.l	(a6),a6
L000c68:
	movea.l	$4c(a6),a4
L000c6c:
	movea.l	$2ac(a4),a0
L000c70:
	movea.l	$8(a0),a5
L000c74:
	move.l	$2e8(a4),d1
L000c78:
	bpl.b	L000ce4
L000c7a:
	lea	$2ec(a4),a1
L000c7e:
	movea.l	$20(a5),a2
L000c82:
	bra.b	L000cb8
L000c84:
	movea.l	(a2)+,a3
L000c86:
	move.w	(a1)+,d0
L000c88:
	cmpi.w	#$4afc,(a3)
L000c8c:
	bne.b	L000cb8
L000c8e:
	move.w	d0,(a3)
L000c90:
	move	sr,d0
L000c92:
	move.l	d0,-(sp)
L000c94:
	ori	#$700,sr
L000c98:
	movec	caar,d0
L000c9c:
	move.l	d0,-(sp)
L000c9e:
	movec	a3,caar
L000ca2:
	movec	cacr,d0
L000ca6:
	bset.l	#$2,d0
L000caa:
	movec	d0,cacr
L000cae:
	move.l	(sp)+,d0
L000cb0:
	movec	d0,caar
L000cb4:
	move.l	(sp)+,d0
L000cb6:
	move	d0,sr
L000cb8:
	dbf	d1,L000c84
L000cbc:
	move.b	$3e0(a6),d0
L000cc0:
	btst.l	#$0,d0
L000cc4:
	beq.b	L000ccc
L000cc6:
	btst.l	#$1,d0
L000cca:
	bne.b	L000ce4
L000ccc:
	moveq	#$44,d0
L000cce:
	move.l	a3,-(sp)
L000cd0:
	movea.l	$3a4(a6),a3
L000cd4:
	pea	L000ce2(pc)
L000cd8:
	move.l	$168(a3),-(sp)
L000cdc:
	movea.l	$568(a3),a3
L000ce0:
	rts
L000ce2:
	dc.b	$26
L000ce3:
	dc.b	$5f
L000ce4:
	clr.l	$2e8(a4)
L000ce8:
	tst.b	$2f(a6)
L000cec:
	beq.b	L000d32
L000cee:
	cmpa.l	$58(a6),a4
L000cf2:
	bne.b	L000cfa
L000cf4:
	movea.l	a4,a1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000cf6:
	dc.w	$6100
	dc.w	Q9_fpu_save_fe0-*
L000cfa:
	movea.l	$334(a4),a1
L000cfe:
	move.l	#$6c,d2
* Vollformat-Adressierung erzwungen -- siehe FULL_EXT_OVERRIDE im Konverter
L000d04:
	lea	([($2a8).w,a4],($6c).w),a2
L000d0c:
	move.b	$74(a1),$0(a2,d2.w*1)
L000d12:
	beq.b	L000d32
L000d14:
	lea	$8(a1),a0
L000d18:
	move.l	a3,-(sp)
L000d1a:
	movea.l	$3a4(a6),a3
L000d1e:
	pea	L000d2c(pc)
L000d22:
	move.l	$e0(a3),-(sp)
L000d26:
	movea.l	$4e0(a3),a3
L000d2a:
	rts
L000d2c:
	dc.b	$26
L000d2d:
	dc.b	$5f
L000d2e:
	dc.b	$20
L000d2f:
	dc.b	$6c
L000d30:
	dc.b	$02
L000d31:
	dc.b	$ac
L000d32:
	move.l	$2b0(a4),$0(a5)
L000d38:
	movea.l	$2a8(a4),a5
L000d3c:
	movem.l	$0(sp),d0/d1/d2/d3/d4/d5/d6/d7
L000d42:
	movem.l	d0/d1/d2/d3/d4/d5/d6/d7,$0(a5)
L000d48:
	movem.l	$20(sp),d0/d1/d2/d3/d4/d5/d6
L000d4e:
	movem.l	d0/d1/d2/d3/d4/d5/d6,$20(a5)
L000d54:
	move.w	$40(sp),$40(a5)
L000d5a:
	move.l	$42(sp),$42(a5)
L000d60:
	move	usp,a1
L000d62:
	move.l	a1,$c(a4)
L000d66:
	move.l	a1,$3c(a5)
L000d6a:
	ori	#$700,sr
L000d6e:
	bclr.b	#$7,$3ac(a4)
L000d74:
	move.b	#$64,$20(a4)
L000d7a:
	move.l	sp,$8(a4)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000d7e:
	dc.w	$6100
	dc.w	Q9_scheduler_183a-*
L000d82:
	subq.l	#$1,$3ac(a0)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000d86:
	dc.w	$6000
	dc.w	Q9_reschedule_trampolin_3140-*
L000d8a:
	andi	#-$701,sr
L000d8e:
	move.l	$2ac(a4),d0
L000d92:
	beq.b	L000dac
L000d94:
	movea.l	d0,a0
L000d96:
	tst.w	$26(a0)
L000d9a:
	beq.b	L000dac
L000d9c:
	movea.l	$8(a0),a5
L000da0:
	move.l	#$3,$8(a5)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000da8:
	dc.w	$6000
	dc.w	L000c74-*
L000dac:
	lea	$37c(a6),a0
L000db0:
	cmpa.l	$30(a0),a0
L000db4:
	bne.b	L000dc2
L000db6:
	move.w	$18(a4),d0
L000dba:
	cmp.w	$8a6(a6),d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000dbe:
	dc.w	$6400
	dc.w	L00035e-*
L000dc2:
	movea.l	a4,a0
L000dc4:
	ori	#$700,sr
L000dc8:
	bclr.b	#$7,$1c(a4)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000dce:
	dc.w	$6100
	dc.w	Q9_scheduler_183a-*
L000dd2:
	move.l	sp,$8(a4)
L000dd6:
	move	usp,a0
L000dd8:
	move.l	a0,$c(a4)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000ddc:
	dc.w	$6000
	dc.w	Q9_reschedule_trampolin_3140-*
L000de0:
	tst.b	$370(a4)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000de4:
	dc.w	$6600
	dc.w	L000370-*
L000de8:
	tst.l	$28(a4)
L000dec:
	bne.b	L000e08
L000dee:
	move.l	$2ac(a4),d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000df2:
	dc.w	$6700
	dc.w	L000fc8-*
L000df6:
	movea.l	d0,a0
L000df8:
	movea.l	$8(a0),a0
L000dfc:
	move.l	#$2,$8(a0)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000e04:
	dc.w	$6000
	dc.w	L000fc8-*
L000e08:
	move	sr,d3
L000e0a:
	ori	#$700,sr
L000e0e:
	movea.l	sp,a5
L000e10:
	moveq	#$48,d0
L000e12:
	addi.l	#$c,d0
L000e18:
	move.l	$334(a4),d1
L000e1c:
	beq.b	L000e3e
L000e1e:
	movea.l	d1,a1
L000e20:
	tst.b	$74(a1)
L000e24:
	bne.b	L000e3a
L000e26:
	cmpa.l	$58(a6),a4
L000e2a:
	bne.b	L000e36
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000e2c:
	dc.w	$6100
	dc.w	L000fe8-*
L000e30:
	tst.b	$74(a1)
L000e34:
	bne.b	L000e3a
L000e36:
	addq.l	#$4,d0
L000e38:
	bra.b	L000e3e
L000e3a:
	add.l	$0(a1),d0
L000e3e:
	addq.l	#$1,$3b4(a4)
L000e42:
	cmpi.l	#$1,$3b4(a4)
L000e4a:
	bhi.b	L000e58
L000e4c:
	suba.l	d0,sp
L000e4e:
	movea.l	sp,a2
L000e50:
	move	usp,a1
L000e52:
	move.l	a1,-(sp)
L000e54:
	move.l	a5,-(sp)
L000e56:
	bra.b	L000e9c
L000e58:
	move	usp,a2
L000e5a:
	movea.l	a2,a1
L000e5c:
	suba.l	d0,a2
L000e5e:
	move	a2,usp
L000e60:
	moveq	#$2,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000e62:
	dc.w	$6100
	dc.w	Q9_range_check_wrap_134a-*
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000e66:
	dc.w	$6500
	dc.w	L000fa2-*
L000e6a:
	movem.l	$0(sp),d0/d1/d2/d3/d4/d5/d6/d7
L000e70:
	movem.l	d0/d1/d2/d3/d4/d5/d6/d7,$0(a2)
L000e76:
	movem.l	$20(sp),d0/d1/d2/d3/d4/d5/d6
L000e7c:
	movem.l	d0/d1/d2/d3/d4/d5/d6,$20(a2)
L000e82:
	move.l	a1,$3c(a2)
L000e86:
	movem.l	$40(sp),d0/d1
L000e8c:
	movem.l	d0/d1,$40(a2)
L000e92:
	move.l	a2,$c(a4)
L000e96:
	lea	$48(a5),sp
L000e9a:
	movea.l	a2,a5
L000e9c:
	tst.b	$2f(a6)
L000ea0:
	beq.b	L000eda
L000ea2:
	move.l	$58(a6),d0
L000ea6:
	beq.b	L000ebc
L000ea8:
	cmp.l	a4,d0
L000eaa:
	bne.b	L000eb6
L000eac:
	lea	$54(a2),a1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000eb0:
	dc.w	$6100
	dc.w	L000fe8-*
L000eb4:
	bra.b	L000ed0
L000eb6:
	movea.l	d0,a1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000eb8:
	dc.w	$6100
	dc.w	Q9_fpu_save_fe0-*
L000ebc:
	lea	$54(a2),a1
L000ec0:
	movea.l	$334(a4),a0
L000ec4:
	clr.l	(a1)
L000ec6:
	tst.b	$74(a0)
L000eca:
	beq.b	L000ed2
L000ecc:
	move.l	a1,$4(a0)
L000ed0:
	move.l	a1,(a1)
L000ed2:
	frestore	L000050(pc)
L000ed6:
	move.l	a4,$58(a6)
L000eda:
	move	sr,d0
L000edc:
	ori	#$700,sr
L000ee0:
	movea.l	$37c(a4),a0
L000ee4:
	movea.l	$0(a0),a1
L000ee8:
	move.l	a1,$37c(a4)
L000eec:
	subq.l	#$1,$378(a4)
L000ef0:
	bne.b	L000ef6
L000ef2:
	clr.w	$26(a4)
L000ef6:
	addq.w	#$1,$372(a4)
L000efa:
	move	d0,sr
L000efc:
	btst.b	#$7,$40(a5)
L000f02:
	bne.b	L000f18
L000f04:
	move.l	$2ac(a4),d0
L000f08:
	beq.b	L000f18
L000f0a:
	move.l	$2e8(a4),d2
L000f0e:
	bpl.b	L000f18
L000f10:
	move.l	a0,-(sp)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000f12:
	dc.w	$6100
	dc.w	L000380-*
L000f16:
	movea.l	(sp)+,a0
L000f18:
	ori	#$700,sr
L000f1c:
	tst.b	$2f(a6)
L000f20:
	beq.b	L000f36
L000f22:
	move.l	$58(a6),d0
L000f26:
	beq.b	L000f32
L000f28:
	cmp.l	a4,d0
L000f2a:
	beq.b	L000f36
L000f2c:
	movea.l	d0,a1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000f2e:
	dc.w	$6100
	dc.w	Q9_fpu_save_fe0-*
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000f32:
	dc.w	$6100
	dc.w	Q9_fpu_restore_1034-*
L000f36:
	move.l	a3,-(sp)
L000f38:
	movea.l	$3a4(a6),a3
L000f3c:
	pea	L000f4a(pc)
L000f40:
	move.l	$fc(a3),-(sp)
L000f44:
	movea.l	$4fc(a3),a3
L000f48:
	rts
L000f4a:
	movea.l	(sp)+,a3
L000f4c:
	addq.b	#$1,$370(a4)
L000f50:
	lea	-$8(sp),sp
L000f54:
	move.w	#$0,$6(sp)
L000f5a:
	move.l	$28(a4),$2(sp)
L000f60:
	move.w	$40(a5),$0(sp)
L000f66:
	move.l	$378(a4),d0
L000f6a:
	moveq	#$0,d1
L000f6c:
	move.w	$a(a0),d1
L000f70:
	clr.w	$a(a0)
L000f74:
	tst.l	$38c(a4)
L000f78:
	bne.b	L000f7e
L000f7a:
	move.l	a0,$38c(a4)
L000f7e:
	moveq	#$0,d2
L000f80:
	movea.l	$2c(a4),a6
L000f84:
	bclr.b	#$7,$1c(a4)
L000f8a:
	btst.b	#$7,$40(a5)
L000f90:
	bne.b	L000f94
L000f92:
	rte
L000f94:
	tst.l	$2ac(a4)
L000f98:
	beq.b	L000f92
L000f9a:
	movem.l	sp/a6/a5/a4/a3/a2/a1/a0/d7/d6/d5/d4/d3/d2/d1/d0,-(sp)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000f9e:
	dc.w	$6000
	dc.w	L000bb6-*
L000fa2:
	move.w	#$a6,$26(a4)
L000fa8:
	move.l	$2ac(a4),d0
L000fac:
	beq.b	L000fc8
L000fae:
	movea.l	d0,a0
L000fb0:
	movea.l	$8(a0),a0
L000fb4:
	move.l	#$2,$8(a0)
L000fbc:
	ori.b	#$1,$41(a0)
L000fc2:
	bra.b	L000fc8
* Fallback ohne installierten Vektor-/Signal-Handler -- fuehrt zur Standard-Terminierungslogik.
Q9_exc_no_handler_fc4:
	move.w	d1,$26(a4)
L000fc8:
	movea.l	sp,a5
L000fca:
	andi	#-$701,sr
L000fce:
	move.w	$26(a4),d1
L000fd2:
	clr.w	$26(a4)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L000fd6:
	dc.w	$6000
	dc.w	Q9_exc_default_action_24d8-*
L000fda:
	dc.b	$51
L000fdb:
	dc.b	$fb
L000fdc:
	dc.b	$00
L000fdd:
	dc.b	$00
L000fde:
	dc.b	$00
L000fdf:
	dc.b	$00
* FPU-Kontext sichern (Lazy-Context-Switch, Save-Haelfte).
Q9_fpu_save_fe0:
	tst.l	$334(a1)
L000fe4:
	beq.b	L001002
L000fe6:
	bsr.b	Q9_fpu_migrate_1004
L000fe8:
	fsave	$74(a1)
L000fec:
	clr.l	$58(a6)
L000ff0:
	tst.b	$74(a1)
L000ff4:
	beq.b	L001002
L000ff6:
	fmovem.x	fp0/fp1/fp2/fp3/fp4/fp5/fp6/fp7,$8(a1)
L000ffc:
	fmovem.l	fpcr/fpsr/fpiar,$68(a1)
L001002:
	rts
* Migriert den Inhalt eines bereits belegten FPU-Save-Bereichs, bevor er ueberschrieben wird.
Q9_fpu_migrate_1004:
	movea.l	$334(a1),a1
L001008:
	tst.b	$2f(a6)
L00100c:
	beq.b	L001032
L00100e:
	tst.l	$4(a1)
L001012:
	beq.b	L001032
L001014:
	movem.l	a2/a1/d2,-(sp)
L001018:
	movea.l	$4(a1),a2
L00101c:
	move.l	$0(a1),d2
L001020:
	lsr.l	#$2,d2
L001022:
	subq.l	#$1,d2
L001024:
	move.l	(a1)+,(a2)+
L001026:
	dbf	d2,L001024
L00102a:
	movem.l	(sp)+,d2/a1/a2
L00102e:
	clr.l	$4(a1)
L001032:
	rts
* FPU-Kontext wiederherstellen (Restore-Haelfte, Gegenstueck zu Q9_fpu_save_fe0).
Q9_fpu_restore_1034:
	tst.l	$334(a4)
L001038:
	beq.b	L001058
L00103a:
	movea.l	$334(a4),a1
L00103e:
	tst.b	$74(a1)
L001042:
	beq.b	L001050
L001044:
	fmovem.l	$68(a1),fpcr/fpsr/fpiar
L00104a:
	fmovem.x	$8(a1),fp0/fp1/fp2/fp3/fp4/fp5/fp6/fp7
L001050:
	frestore	$74(a1)
L001054:
	move.l	a4,$58(a6)
L001058:
	rts
L00105a:
	dc.b	$51
L00105b:
	dc.b	$fb
L00105c:
	dc.b	$00
L00105d:
	dc.b	$00
L00105e:
	dc.b	$00
L00105f:
	dc.b	$00
L001060:
	moveq	#$0,d1
L001062:
	movem.l	a6/a5/a4/a3/a2/a1/a0/d7/d6/d5/d4/d3/d2/d1,-(sp)
L001066:
	movea.l	d0,a2
L001068:
	movea.l	$50(a6),a4
L00106c:
	move.l	$14(a4),-(sp)
L001070:
	move.l	$8(a2),$14(a4)
L001076:
	lea	$2c(a2),a5
L00107a:
	move.l	$140(a4),-(sp)
L00107e:
	move.l	$144(a4),-(sp)
L001082:
	move.l	sp,$140(a4)
L001086:
	pea	L00109a(pc)
L00108a:
	move.l	(sp),$144(a4)
L00108e:
	move.l	$6e(a2),-(sp)
L001092:
	movem.l	$0(a5),d0/d1/d2/d3/d4/d5/d6/d7/a0/a1/a2/a3
L001098:
	rts
L00109a:
	bcc.b	L0010a0
L00109c:
	move.w	d1,$e(sp)
L0010a0:
	movea.l	$40(sp),a6
L0010a4:
	movea.l	$50(a6),a4
L0010a8:
	move.l	(sp)+,$144(a4)
L0010ac:
	move.l	(sp)+,$140(a4)
L0010b0:
	move.l	(sp)+,$14(a4)
L0010b4:
	movem.l	(sp)+,d0/d2/d3/d4/d5/d6/d7/a0/a1/a2/a3/a4/a5/a6
L0010b8:
	rts
L0010ba:
	tst.l	d0
L0010bc:
	beq.b	L0010de
L0010be:
	movem.l	a2/d1,-(sp)
L0010c2:
	movea.l	d1,a2
L0010c4:
	lsr.l	#$2,d0
L0010c6:
	subq.l	#$1,d0
L0010c8:
	move.l	$c(sp),d1
L0010cc:
	move.l	d1,(a2)+
L0010ce:
	dbf	d0,L0010cc
L0010d2:
	addq.w	#$1,d0
L0010d4:
	subq.l	#$1,d0
L0010d6:
	bcc.b	L0010cc
L0010d8:
	movem.l	(sp)+,d1/a2
L0010dc:
	moveq	#$0,d0
L0010de:
	rts
L0010e0:
	dc.b	$02
L0010e1:
	dc.b	$7c
L0010e2:
	dc.b	$f8
L0010e3:
	dc.b	$ff
L0010e4:
	dc.b	$4e
L0010e5:
	dc.b	$75
* Interrupts bedingt maskieren (IPL 7), alte SR als Rueckgabewert.
Q9_irq_mask_10e6:
	tst.l	d0
L0010e8:
	move	sr,d0
L0010ea:
	beq.b	L0010f0
L0010ec:
	ori	#$700,sr
L0010f0:
	rts
* SR wiederherstellen -- Gegenstueck zu Q9_irq_mask_10e6.
Q9_irq_unmask_10f2:
	move	d1,sr
L0010f4:
	rts
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0010f6:
	dc.w	$6100
	dc.w	L00119c-*
L0010fa:
	movem.l	a3/a2/a1/a0/d7/d6/d5/d4/d3/d2/d1,-(sp)
L0010fe:
	movea.l	d0,a0
L001100:
	move.l	d1,d0
L001102:
	move.b	$33(sp),d5
L001106:
	moveq	#-$1,d1
L001108:
	tst.l	d0
L00110a:
	ble.b	L00117e
L00110c:
	move.l	sp,d6
L00110e:
	lea	L001178(pc),a1
L001112:
	cmpa.l	a1,a0
L001114:
	bcc.b	L001122
L001116:
	lea	L001136(pc),a1
L00111a:
	cmpa.l	a1,a0
L00111c:
	bcs.b	L001122
L00111e:
	lea	L001178(pc),a0
L001122:
	movea.l	$68(a6),a1
L001126:
	movea.l	$6(a1),a2
L00112a:
	lea	L001136(pc),a3
L00112e:
	move.l	a3,$6(a1)
L001132:
	movem.l	(a0),d2/d3
L001136:
	movea.l	d6,sp
L001138:
	lea	L001178(pc),a3
L00113c:
	move.l	a3,$6(a1)
L001140:
	tst.b	d5
L001142:
	bne.b	L00116a
L001144:
	move.l	#-$5a5a5a5b,d0
L00114a:
	move.l	d0,(a0)
L00114c:
	cmp.l	(a0),d0
L00114e:
	bne.b	L001166
L001150:
	move.l	#$5a5a5a5a,d4
L001156:
	move.l	d4,(a0)
L001158:
	cmp.l	(a0),d4
L00115a:
	bne.b	L001166
L00115c:
	move.l	d0,$4(a0)
L001160:
	cmp.l	(a0),d4
L001162:
	bne.b	L001166
L001164:
	moveq	#$1,d1
L001166:
	movem.l	d2/d3,(a0)
L00116a:
	moveq	#$1,d0
L00116c:
	cmp.l	d0,d1
L00116e:
	beq.b	L001178
L001170:
	move.l	(a0),d0
L001172:
	cmp.l	d2,d0
L001174:
	bne.b	L001178
L001176:
	moveq	#$2,d1
L001178:
	movea.l	d6,sp
L00117a:
	move.l	a2,$6(a1)
L00117e:
	move.l	d1,d0
L001180:
	movem.l	(sp)+,d1/d2/d3/d4/d5/d6/d7/a0/a1/a2/a3
* Rohbytes statt Instruktion (btst.b #0x6,(-0x113c,PC)) -- siehe FORCE_RAW_BYTES im Konverter
L001184:
	dc.b	$08,$3a,$00,$06,$ee,$c4
L00118a:
	bne.b	L00119a
L00118c:
	move.l	d0,-(sp)
L00118e:
	move.l	#$909,d0
L001194:
	movec	d0,cacr
L001198:
	move.l	(sp)+,d0
L00119a:
	rts
* Rohbytes statt Instruktion (btst.b #0x6,(-0x1154,PC)) -- siehe FORCE_RAW_BYTES im Konverter
L00119c:
	dc.b	$08,$3a,$00,$06,$ee,$ac
L0011a2:
	bne.b	L0011b2
L0011a4:
	move.l	d0,-(sp)
L0011a6:
	move.l	#$808,d0
L0011ac:
	movec	d0,cacr
L0011b0:
	move.l	(sp)+,d0
L0011b2:
	rts
L0011b4:
	movem.l	a4/a3/a2,-(sp)
L0011b8:
	clr.l	-(sp)
L0011ba:
	movea.l	$4c(a6),a4
L0011be:
	movea.l	$8(a4),a2
L0011c2:
	btst.b	#$5,$40(a2)
L0011c8:
	bne.b	L0011ea
L0011ca:
	movea.l	$14(sp),a2
L0011ce:
	move.l	a3,-(sp)
L0011d0:
	movea.l	$3a4(a6),a3
L0011d4:
	pea	L0011e2(pc)
L0011d8:
	move.l	$160(a3),-(sp)
L0011dc:
	movea.l	$560(a3),a3
L0011e0:
	rts
L0011e2:
	dc.b	$26
L0011e3:
	dc.b	$5f
L0011e4:
	dc.b	$64
L0011e5:
	dc.b	$04
L0011e6:
	dc.b	$3f
L0011e7:
	dc.b	$41
L0011e8:
	dc.b	$00
L0011e9:
	dc.b	$02
L0011ea:
	movem.l	(sp)+,d0/a2/a3/a4
L0011ee:
	rts
L0011f0:
	movem.l	a4/a3/a2/d2/d0,-(sp)
L0011f4:
	move.w	#$ec,d2
L0011f8:
	bra.b	L001202
L0011fa:
	movem.l	a4/a3/a2/d2/d0,-(sp)
L0011fe:
	move.w	#$e8,d2
L001202:
	clr.l	(sp)
L001204:
	movea.l	d0,a2
L001206:
	move.l	d1,d0
L001208:
	move.l	$18(sp),d1
L00120c:
	movea.l	$4c(a6),a4
L001210:
	movea.l	$3a4(a6),a3
L001214:
	pea	L001226(pc)
L001218:
	move.l	$0(a3,d2.w*1),-(sp)
L00121c:
	addi.w	#$400,d2
L001220:
	movea.l	$0(a3,d2.w*1),a3
L001224:
	rts
L001226:
	bcc.b	L00122c
L001228:
	move.w	d1,$2(sp)
L00122c:
	movem.l	(sp)+,d0/d2/a2/a3/a4
L001230:
	rts
L001232:
	movem.l	a5/a4/a0/d2/d1,-(sp)
L001236:
	movem.l	$3c(a6),a4/a5
L00123c:
	bra.b	L001242
L00123e:
	lea	$10(a4),a4
L001242:
	cmpa.l	a4,a5
L001244:
	beq.b	L001286
L001246:
	move.l	$0(a4),d1
L00124a:
	beq.b	L001286
L00124c:
	cmp.l	d0,d1
L00124e:
	bne.b	L00123e
L001250:
	movea.l	a4,a5
L001252:
	movea.l	d0,a0
L001254:
	move.l	$4(a0),d2
L001258:
	lea	$10(a4),a4
L00125c:
	move.l	$0(a4),d1
L001260:
	beq.b	L001270
L001262:
	movea.l	d1,a0
L001264:
	cmp.l	$4(a4),d0
L001268:
	bne.b	L001258
L00126a:
	add.l	$4(a0),d2
L00126e:
	bra.b	L001258
L001270:
	movea.l	$40(a6),a4
L001274:
	move.l	d2,$8(a5)
L001278:
	lea	$10(a5),a5
L00127c:
	cmpa.l	a5,a4
L00127e:
	beq.b	L001286
L001280:
	move.l	$0(a5),d1
L001284:
	bne.b	L001274
L001286:
	moveq	#$0,d0
L001288:
	movem.l	(sp)+,d1/d2/a0/a4/a5
L00128c:
	rts
L00128e:
	moveq	#$20,d0
L001290:
	add.l	a5,d0
L001292:
	move.l	d1,-(sp)
L001294:
	move.l	a5,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001296:
	dc.w	$6100
	dc.w	L0063f0-*
L00129a:
	addq.l	#$4,sp
L00129c:
	move.l	d0,d1
L00129e:
	beq.b	L0012a4
L0012a0:
	ori	#$1,ccr
L0012a4:
	rts
L0012a6:
	dc.b	$61
L0012a7:
	dc.b	$0c
L0012a8:
	dc.b	$65
L0012a9:
	dc.b	$08
L0012aa:
	dc.b	$2b
L0012ab:
	dc.b	$40
L0012ac:
	dc.b	$00
L0012ad:
	dc.b	$00
L0012ae:
	dc.b	$2b
L0012af:
	dc.b	$4a
L0012b0:
	dc.b	$00
L0012b1:
	dc.b	$28
L0012b2:
	dc.b	$4e
L0012b3:
	dc.b	$75
L0012b4:
	movem.l	a2/d1/d0,-(sp)
L0012b8:
	moveq	#$0,d1
L0012ba:
	move.l	sp,d0
L0012bc:
	pea	$8(sp)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0012c0:
	dc.w	$6100
	dc.w	Q9_arena_alloc_526c-*
L0012c4:
	addq.l	#$4,sp
L0012c6:
	tst.l	d0
L0012c8:
	beq.b	L0012d2
L0012ca:
	move.l	d0,$4(sp)
L0012ce:
	ori	#$1,ccr
L0012d2:
	movem.l	(sp)+,d0/d1/a2
L0012d6:
	rts
L0012d8:
	bsr.b	L0012b4
L0012da:
	bcs.b	L0012ee
L0012dc:
	move.l	d0,-(sp)
L0012de:
	exg	d1,a2
L0012e0:
	pea	$0.w
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0012e4:
	dc.w	$6100
	dc.w	L0010ba-*
L0012e8:
	addq.l	#$4,sp
L0012ea:
	exg	d1,a2
L0012ec:
	move.l	(sp)+,d0
L0012ee:
	rts
L0012f0:
	pea	L0012a8(pc)
L0012f4:
	movem.l	a2/d1/d0,-(sp)
L0012f8:
	bra.b	L0012ba
L0012fa:
	pea	L0012a8(pc)
L0012fe:
	movem.l	a2/d1/d0,-(sp)
L001302:
	moveq	#$0,d1
L001304:
	move.l	sp,d0
L001306:
	pea	$8(sp)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00130a:
	dc.w	$6100
	dc.w	L0057be-*
L00130e:
	addq.l	#$4,sp
L001310:
	bra.b	L0012c6
L001312:
	dc.b	$48
L001313:
	dc.b	$7a
L001314:
	dc.b	$ff
L001315:
	dc.b	$94
L001316:
	dc.b	$48
L001317:
	dc.b	$e7
L001318:
	dc.b	$c0
L001319:
	dc.b	$20
L00131a:
	dc.b	$60
L00131b:
	dc.b	$e8
* Gemeinsamer Deallokations-Tail-Wrapper (dispatcht auf Q9_mem_free_5a22 oder Bereichsvalidierung).
Q9_dealloc_tail_131c:
	movem.l	a2/d1/d0,-(sp)
L001320:
	move.l	a2,d1
L001322:
	move.l	#$1,-(sp)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001328:
	dc.w	$6100
	dc.w	Q9_mem_free_5a22-*
L00132c:
	addq.l	#$4,sp
L00132e:
	bra.b	L0012c6
L001330:
	movem.l	a2/d1/d0,-(sp)
L001334:
	move.l	a2,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001336:
	dc.w	$6100
	dc.w	L005cd2-*
L00133a:
	bra.b	L0012c6
L00133c:
	dc.b	$20
L00133d:
	dc.b	$0d
L00133e:
	dc.b	$72
L00133f:
	dc.b	$24
L001340:
	dc.b	$d2
L001341:
	dc.b	$8d
L001342:
	dc.b	$61
L001343:
	dc.b	$00
L001344:
	dc.b	$4d
L001345:
	dc.b	$be
L001346:
	dc.b	$60
L001347:
	dc.b	$00
L001348:
	dc.b	$ff
L001349:
	dc.b	$54
* Wrapper um Q9_owns_range_5d68 -- setzt Carry-Flag bei ungueltigem Adressbereich.
Q9_range_check_wrap_134a:
	move.l	d0,-(sp)
L00134c:
	move.l	a2,d1
L00134e:
	clr.l	-(sp)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001350:
	dc.w	$6100
	dc.w	Q9_owns_range_5d68-*
L001354:
	lea	$4(sp),sp
L001358:
	move.l	d0,d1
L00135a:
	movem.l	(sp)+,d0
L00135e:
	beq.b	L001364
L001360:
	ori	#$1,ccr
L001364:
	rts
L001366:
	pea	$0(a5)
L00136a:
	move.l	a0,-(sp)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00136c:
	dc.w	$6100
	dc.w	L006232-*
L001370:
	addq.l	#$8,sp
L001372:
	move.l	d0,d1
L001374:
	beq.b	L00137a
L001376:
	ori	#$1,ccr
L00137a:
	rts
L00137c:
	dc.b	$72
L00137d:
	dc.b	$00
L00137e:
	dc.b	$4e
L00137f:
	dc.b	$75
L001380:
	move.w	#$d0,d1
L001384:
	ori	#$1,ccr
L001388:
	rts
L00138a:
	dc.b	$51
L00138b:
	dc.b	$fb
L00138c:
	dc.b	$00
L00138d:
	dc.b	$00
L00138e:
	dc.b	$00
L00138f:
	dc.b	$00
* Genereller Kategorie-Dispatcher fuer kernel-interne Primitive (6 Kategorien, ueber Slot 8 der Syscall-Tabellen erreichbar).
Q9_category_dispatch_1390:
	movem.l	a3/a0/d2,-(sp)
L001394:
	lea	-$48(sp),sp
L001398:
	movea.l	sp,a0
L00139a:
	movea.l	$3a4(a6),a3
L00139e:
	move.l	$20(a3),$42(a0)
L0013a4:
	move.l	$420(a3),$2c(a0)
L0013aa:
	clr.w	$0(a0)
L0013ae:
	move.w	$0(a4),$2(a0)
L0013b4:
	move.l	d2,$4(a0)
L0013b8:
	moveq	#$0,d2
L0013ba:
	bsr.b	L0013d2
L0013bc:
	lea	$48(sp),sp
L0013c0:
	movem.l	(sp)+,d2/a0/a3
L0013c4:
	rts
L0013c6:
	dc.b	$00
L0013c7:
	dc.b	$5e
L0013c8:
	dc.b	$01
L0013c9:
	dc.b	$b8
L0013ca:
	dc.b	$01
L0013cb:
	dc.b	$ba
L0013cc:
	dc.b	$01
L0013cd:
	dc.b	$6e
L0013ce:
	dc.b	$01
L0013cf:
	dc.b	$7a
L0013d0:
	dc.b	$02
L0013d1:
	dc.b	$da
L0013d2:
	add.w	d1,d1
L0013d4:
	cmpi.w	#$c,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0013d8:
	dc.w	$6400
	dc.w	L001380-*
L0013dc:
	movem.l	a2/a1/a0/d4/d3/d2,-(sp)
L0013e0:
	cmpi.w	#$0,d1
L0013e4:
	bne.b	L0013ec
L0013e6:
	pea	L001402(pc)
L0013ea:
	bra.b	L0013f0
L0013ec:
	pea	L0013fc(pc)
L0013f0:
	lea	L0013c6(pc),a1
L0013f4:
	move.w	$0(a1,d1.w*1),d1
L0013f8:
	jmp	$0(a1,d1.w*1)
L0013fc:
	dc.b	$65
L0013fd:
	dc.b	$04
L0013fe:
	dc.b	$2b
L0013ff:
	dc.b	$40
L001400:
	dc.b	$00
L001401:
	dc.b	$00
L001402:
	movem.l	(sp)+,d2/d3/d4/a0/a1/a2
L001406:
	rts
L001408:
	movem.l	a4/a2/d1,-(sp)
L00140c:
	movea.l	$50(a6),a4
L001410:
	movea.l	d0,a2
L001412:
	move	sr,d1
L001414:
	ori	#$700,sr
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001418:
	dc.w	$6100
	dc.w	L0014ea-*
L00141c:
	movem.l	(sp)+,d1/a2/a4
L001420:
	moveq	#$0,d0
L001422:
	rts
* Kategorie-0-Handler: durchlaeuft die Liste angehaengter Module/Deskriptoren, prueft die 0xB0BD-Struktursignatur.
Q9_category0_handler_1424:
	movea.l	d0,a2
L001426:
	tst.l	d0
L001428:
	bne.b	L001478
L00142a:
	tst.l	$390(a4)
L00142e:
	lea	$37c(a4),a1
L001432:
	beq.b	L00146a
L001434:
	bra.b	L00145a
L001436:
	cmpi.w	#-$4f43,$0(a2)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00143c:
	dc.w	$6600
	dc.w	L001528-*
L001440:
	btst.b	#$1,$28(a2)
L001446:
	bne.b	L00146c
L001448:
	btst.b	#$2,$28(a2)
L00144e:
	bne.b	L001472
L001450:
	move.l	$14(a4),$8(a2)
L001456:
	bsr.b	Q9_module_unlink_14ae
L001458:
	bcs.b	L00146a
L00145a:
	move	sr,d1
L00145c:
	ori	#$700,sr
L001460:
	movea.l	$14(a1),a2
L001464:
	cmpa.l	a2,a1
L001466:
	bne.b	L001436
L001468:
	move	d1,sr
L00146a:
	rts
L00146c:
	bset.b	#$0,$28(a2)
L001472:
	movea.l	$c(a2),a2
L001476:
	bra.b	L001464
L001478:
	move	sr,d1
L00147a:
	ori	#$700,sr
L00147e:
	btst.l	#$4,d1
L001482:
	beq.b	Q9_module_unlink_14ae
L001484:
	btst.b	#$7,$2e(a6)
L00148a:
	beq.b	Q9_module_unlink_14ae
L00148c:
	lea	$37c(a4),a1
L001490:
	tst.l	$14(a1)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001494:
	dc.w	$6700
	dc.w	L001528-*
L001498:
	movea.l	a1,a0
L00149a:
	movea.l	$14(a0),a0
L00149e:
	cmpa.l	a0,a1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0014a0:
	dc.w	$6700
	dc.w	L001528-*
L0014a4:
	cmpa.l	a0,a2
L0014a6:
	bne.b	L00149a
L0014a8:
	move.l	$14(a4),$8(a2)
* Haengt einen Modul-Deskriptor aus zwei parallelen verketteten Listen gleichzeitig aus und gibt ihn frei.
Q9_module_unlink_14ae:
	cmpi.w	#-$4f43,$0(a2)
L0014b4:
	bne.b	L001528
L0014b6:
	btst.l	#$4,d1
L0014ba:
	beq.b	L0014cc
L0014bc:
	tst.w	$14(a4)
L0014c0:
	beq.b	L0014cc
L0014c2:
	move.l	$14(a4),d0
L0014c6:
	cmp.l	$8(a2),d0
L0014ca:
	bne.b	L001528
L0014cc:
	btst.b	#$1,$28(a2)
L0014d2:
	bne.b	L00151c
L0014d4:
	btst.b	#$2,$28(a2)
L0014da:
	bne.b	L001522
L0014dc:
	bset.b	#$2,$28(a2)
L0014e2:
	move	d1,sr
L0014e4:
	move	sr,d1
L0014e6:
	ori	#$700,sr
L0014ea:
	clr.w	$0(a2)
L0014ee:
	movem.l	a1/a0,-(sp)
L0014f2:
	movem.l	$c(a2),a0/a1
L0014f8:
	move.l	a0,$c(a1)
L0014fc:
	move.l	a1,$10(a0)
L001500:
	movem.l	$14(a2),a0/a1
L001506:
	move.l	a0,$14(a1)
L00150a:
	move.l	a1,$18(a0)
L00150e:
	movem.l	(sp)+,a0/a1
L001512:
	move.l	$4(a2),d0
L001516:
	move	d1,sr
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001518:
	dc.w	$6000
	dc.w	Q9_dealloc_tail_131c-*
L00151c:
	bset.b	#$0,$28(a2)
L001522:
	move	d1,sr
L001524:
	moveq	#$0,d1
L001526:
	rts
L001528:
	move	d1,sr
L00152a:
	move.w	#$e1,d1
L00152e:
	ori	#$1,ccr
L001532:
	rts
L001534:
	move.l	d3,d0
L001536:
	move.l	d4,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001538:
	dc.w	$6100
	dc.w	L002eea-*
L00153c:
	move.l	d0,d3
L00153e:
	move.l	d1,d4
L001540:
	tst.w	$2(a6)
L001544:
	beq.b	L00154e
L001546:
	tst.w	$28(a6)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00154a:
	dc.w	$6700
	dc.w	L003ce8-*
L00154e:
	cmp.l	$30(a6),d4
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001552:
	dc.w	$6500
	dc.w	L002e4a-*
L001556:
	bhi.b	L001568
L001558:
	move.l	#$15180,d0
L00155e:
	sub.l	$34(a6),d0
L001562:
	cmp.l	d0,d3
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001564:
	dc.w	$6500
	dc.w	L002e4a-*
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001568:
	dc.w	$6100
	dc.w	L00162c-*
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00156c:
	dc.w	$6500
	dc.w	L001618-*
L001570:
	movem.l	d3/d4,$20(a2)
L001576:
	lea	$774(a6),a0
L00157a:
	move.l	a2,d0
L00157c:
	bra.b	L0015c4
L00157e:
	moveq	#$0,d1
L001580:
	tst.w	$2(a6)
L001584:
	beq.b	L00158e
L001586:
	tst.w	$28(a6)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00158a:
	dc.w	$6700
	dc.w	L003ce8-*
L00158e:
	move.l	d3,d0
L001590:
	bpl.b	L00159a
L001592:
	bclr.l	#$1f,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001596:
	dc.w	$6100
	dc.w	L003b96-*
L00159a:
	move.l	d1,d4
L00159c:
	beq.b	L0015aa
L00159e:
	cmpi.l	#$1,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0015a4:
	dc.w	$6500
	dc.w	L002e4a-*
L0015a8:
	move.l	d0,d4
L0015aa:
	move.l	d0,d3
L0015ac:
	bsr.b	L00162c
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0015ae:
	dc.w	$6500
	dc.w	L00146a-*
L0015b2:
	add.l	$54(a6),d3
L0015b6:
	move.l	d3,$20(a2)
L0015ba:
	move.l	d4,$1c(a2)
L0015be:
	lea	$77c(a6),a0
L0015c2:
	move.l	a2,d0
L0015c4:
	movem.l	a2/a1/a0/d2/d1/d0,-(sp)
L0015c8:
	movem.l	$20(a2),d0/d1
L0015ce:
	movea.l	a0,a1
L0015d0:
	move	sr,d2
L0015d2:
	ori	#$700,sr
L0015d6:
	movea.l	$c(a0),a0
L0015da:
	cmpa.l	a1,a0
L0015dc:
	beq.b	L0015ec
L0015de:
	cmp.l	$24(a0),d1
L0015e2:
	bhi.b	L0015d6
L0015e4:
	bcs.b	L0015ec
L0015e6:
	cmp.l	$20(a0),d0
L0015ea:
	bpl.b	L0015d6
L0015ec:
	movea.l	$10(a0),a1
L0015f0:
	movem.l	a0/a1,$c(a2)
L0015f6:
	move.l	a2,$10(a0)
L0015fa:
	move.l	a2,$c(a1)
L0015fe:
	move.w	#-$4f43,$0(a2)
L001604:
	move	d2,sr
L001606:
	lea	L001624(pc),a0
L00160a:
	cmpa.l	$18(sp),a0
L00160e:
	beq.b	L001614
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001610:
	dc.w	$6100
	dc.w	Q9_clock_hook_install_70c-*
L001614:
	movem.l	(sp)+,d0/d1/d2/a0/a1/a2
L001618:
	rts
L00161a:
	movem.l	a2/a0,-(sp)
L00161e:
	movea.l	d0,a0
L001620:
	movea.l	d1,a2
L001622:
	bsr.b	L0015c4
L001624:
	movem.l	(sp)+,a0/a2
L001628:
	moveq	#$0,d0
L00162a:
	rts
L00162c:
	movem.l	a1/a0/d0,-(sp)
L001630:
	move.l	#$74,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001636:
	dc.w	$6100
	dc.w	L0012b4-*
L00163a:
	bcs.b	L00169a
L00163c:
	move.w	$0(a4),$2(a2)
L001642:
	move.l	d0,$4(a2)
L001646:
	move.l	$14(a4),$8(a2)
L00164c:
	moveq	#$0,d0
L00164e:
	move.l	d0,$1c(a2)
L001652:
	move.l	d0,$20(a2)
L001656:
	move.l	d0,$24(a2)
L00165a:
	move.l	d0,$28(a2)
L00165e:
	lea	$37c(a4),a0
L001662:
	move	sr,d0
L001664:
	ori	#$700,sr
L001668:
	tst.l	$390(a4)
L00166c:
	bne.b	L001676
L00166e:
	move.l	a0,$14(a0)
L001672:
	move.l	a0,$18(a0)
L001676:
	movea.l	$18(a0),a1
L00167a:
	movem.l	a0/a1,$14(a2)
L001680:
	move.l	a2,$14(a1)
L001684:
	move.l	a2,$18(a0)
L001688:
	move	d0,sr
L00168a:
	movea.l	$4(sp),a0
L00168e:
	lea	$2c(a2),a1
L001692:
	moveq	#$11,d0
L001694:
	move.l	(a0)+,(a1)+
L001696:
	dbf	d0,L001694
L00169a:
	movem.l	(sp)+,d0/a0/a1
L00169e:
	rts
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0016a0:
	dc.w	$6000
	dc.w	L001380-*
L0016a4:
	trapf.w	#$0
L0016a8:
	exg	a1,a2
L0016aa:
	bsr.b	L0016b6
L0016ac:
	bcs.b	L0016b4
L0016ae:
	exg	a1,a2
L0016b0:
	move.l	a2,$28(a5)
L0016b4:
	rts
L0016b6:
	movem.l	a2/a0/d2/d0,-(sp)
L0016ba:
	movea.l	$44(a6),a0
L0016be:
	move.w	#$e5,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0016c2:
	dc.w	$6100
	dc.w	L00171a-*
L0016c6:
	bcs.b	L001702
L0016c8:
	move.l	a1,$30(a1)
L0016cc:
	move.l	a1,$34(a1)
L0016d0:
	move.w	$8fe(a6),d0
L0016d4:
	move.w	d0,$3b2(a1)
L0016d8:
	movea.l	$44(a6),a0
L0016dc:
	move.w	$2(a0),d1
L0016e0:
	move.w	d1,$3b0(a1)
L0016e4:
	sub.w	d0,d1
L0016e6:
	move.l	#$4a696d69,$0(a1,d1.w*1)
L0016ee:
	move.l	#$15180,d0
L0016f4:
	sub.l	$34(a6),d0
L0016f8:
	move.l	d0,$2c0(a1)
L0016fc:
	move.l	$30(a6),$2bc(a1)
L001702:
	movem.l	(sp)+,d0/d2/a0/a2
L001706:
	rts
L001708:
	move.w	#$c8,d1
L00170c:
	bsr.b	L00171a
L00170e:
	bcs.b	L001718
L001710:
	move.l	d0,$0(a5)
L001714:
	move.l	a1,$24(a5)
L001718:
	rts
L00171a:
	movem.l	a3/a2/d2/d1,-(sp)
L00171e:
	subq.l	#$8,sp
L001720:
	moveq	#$0,d0
L001722:
	move.w	$2(a0),d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001726:
	dc.w	$6100
	dc.w	L0012d8-*
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00172a:
	dc.w	$6500
	dc.w	L0017f0-*
L00172e:
	move.l	a2,$4(sp)
L001732:
	move	sr,$0(sp)
L001736:
	ori	#$700,sr
L00173a:
	movea.l	a0,a1
L00173c:
	move.w	(a0),d0
L00173e:
	moveq	#$0,d1
L001740:
	move.w	d0,d1
L001742:
	lsl.l	#$2,d1
L001744:
	lea	$4(a0,d1.l*1),a2
L001748:
	move.w	(a2),d1
L00174a:
	beq.b	L00177c
L00174c:
	lsl.w	#$2,d1
L00174e:
	move.l	$0(a2,d1.w*1),d0
L001752:
	move.w	d0,(a2)
L001754:
	bne.b	L001758
L001756:
	clr.l	(a2)
L001758:
	clr.l	$0(a2,d1.w*1)
L00175c:
	lea	$0(a0,d1.w*1),a1
L001760:
	move.l	$4(sp),(a1)
L001764:
	move	$0(sp),sr
L001768:
	move.l	a1,d0
L00176a:
	sub.l	a0,d0
L00176c:
	lsr.l	#$2,d0
L00176e:
	movea.l	(a1),a0
L001770:
	move.w	d0,(a0)
L001772:
	exg	a0,a1
L001774:
	addq.l	#$8,sp
L001776:
	movem.l	(sp)+,d1/d2/a2/a3
L00177a:
	rts
L00177c:
	btst.b	#$0,$39(a6)
L001782:
	bne.b	L0017d4
L001784:
	lea	$44(a6),a3
L001788:
	cmpa.l	(a3),a0
L00178a:
	beq.b	L001794
L00178c:
	lea	$48(a6),a3
L001790:
	cmpa.l	(a3),a0
L001792:
	bne.b	L0017d4
L001794:
	addq.w	#$1,d0
L001796:
	asl.l	#$3,d0
L001798:
	move.l	$70(a6),d1
L00179c:
	subq.l	#$1,d1
L00179e:
	add.l	d1,d0
L0017a0:
	addq.l	#$1,d1
L0017a2:
	neg.l	d1
L0017a4:
	and.l	d1,d0
L0017a6:
	move.l	d0,d2
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0017a8:
	dc.w	$6100
	dc.w	L0043b0-*
L0017ac:
	bcs.b	L0017d4
L0017ae:
	lsr.w	#$1,d2
L0017b0:
	lea	$0(a0,d2.w*1),a2
L0017b4:
	move.l	a0,(a3)
L0017b6:
	lsr.l	#$1,d0
L0017b8:
	lea	$0(a0,d0.l*1),a3
L0017bc:
	lsr.l	#$2,d0
L0017be:
	subq.w	#$1,d0
L0017c0:
	move.w	d0,(a0)
L0017c2:
	move.l	a2,d1
L0017c4:
	sub.l	a0,d1
L0017c6:
	lsr.l	#$2,d1
L0017c8:
	addq.l	#$1,d1
L0017ca:
	exg	a0,a3
L0017cc:
	bsr.b	L0017fc
L0017ce:
	exg	a0,a3
L0017d0:
	movea.l	a2,a1
L0017d2:
	bra.b	L001760
L0017d4:
	move	$0(sp),sr
L0017d8:
	movea.l	$4(sp),a2
L0017dc:
	tst.w	d1
L0017de:
	beq.b	L0017e4
L0017e0:
	move.l	d1,$8(sp)
L0017e4:
	moveq	#$0,d0
L0017e6:
	move.w	$2(a0),d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0017ea:
	dc.w	$6100
	dc.w	Q9_dealloc_tail_131c-*
L0017ee:
	bra.b	L0017f4
L0017f0:
	move.l	d1,$8(sp)
L0017f4:
	ori	#$1,ccr
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0017f8:
	dc.w	$6000
	dc.w	L001774-*
L0017fc:
	move.l	a1,-(sp)
L0017fe:
	movea.l	a0,a1
L001800:
	move.w	d1,(a0)
L001802:
	move.l	d1,d2
L001804:
	lsl.l	#$2,d2
L001806:
	lea	$0(a0,d2.l*1),a0
L00180a:
	sub.l	d1,d0
L00180c:
	subq.l	#$1,d0
L00180e:
	addq.l	#$1,d1
L001810:
	move.l	d1,(a0)+
L001812:
	dbf	d0,L00180e
L001816:
	move.w	d1,$2(a1)
L00181a:
	movea.l	(sp)+,a1
L00181c:
	rts
L00181e:
	dc.b	$51
L00181f:
	dc.b	$fc
L001820:
	movem.l	a4/a0/d1,-(sp)
L001824:
	clr.l	-(sp)
L001826:
	movea.l	d0,a0
L001828:
	movea.l	$4c(a6),a4
L00182c:
	bsr.b	Q9_scheduler_183a
L00182e:
	bcc.b	L001834
L001830:
	move.w	d1,$2(sp)
L001834:
	movem.l	(sp)+,d0/d1/a0/a4
L001838:
	rts
* Scheduler: fuegt einen Prozess in die Ready-Queue ein, mit Prioritaets-Aging und Sortier-Schluessel-Berechnung.
Q9_scheduler_183a:
	cmpi.b	#$61,$20(a0)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001840:
	dc.w	$6700
	dc.w	L001920-*
L001844:
	move.b	#$61,$20(a0)
L00184a:
	movem.l	a3/a2/a1/d4/d3/d2/d1/d0,-(sp)
L00184e:
	moveq	#$0,d0
L001850:
	move	sr,d4
L001852:
	lea	$37c(a6),a3
L001856:
	subq.l	#$1,$3c4(a6)
L00185a:
	bpl.b	L00188c
L00185c:
	move.l	#$7fff0000,d1
L001862:
	move.l	d1,$3c4(a6)
L001866:
	movea.l	a3,a1
L001868:
	ori	#$700,sr
L00186c:
	bra.b	L001882
L00186e:
	move.l	$2e0(a1),d2
L001872:
	bmi.b	L001882
L001874:
	beq.b	L001882
L001876:
	add.l	d1,d2
L001878:
	bpl.b	L00187e
L00187a:
	move.l	d1,d2
L00187c:
	not.w	d2
L00187e:
	move.l	d2,$2e0(a1)
L001882:
	movea.l	$30(a1),a1
L001886:
	cmpa.l	a1,a3
L001888:
	bne.b	L00186e
L00188a:
	move	d4,sr
L00188c:
	move.w	$8aa(a6),d0
L001890:
	beq.b	L0018a0
L001892:
	cmp.w	$0(a0),d0
L001896:
	bne.b	L0018a0
L001898:
	move.w	$18(a0),d0
L00189c:
	moveq	#-$1,d2
L00189e:
	bra.b	L0018ce
L0018a0:
	move.w	$18(a0),d0
L0018a4:
	cmp.w	$8a6(a6),d0
L0018a8:
	bcc.b	L0018b6
L0018aa:
	btst.b	#$7,$1c(a0)
L0018b0:
	bne.b	L0018b6
L0018b2:
	moveq	#$0,d2
L0018b4:
	bra.b	L0018ce
L0018b6:
	move.w	$8a8(a6),d1
L0018ba:
	beq.b	L0018c0
L0018bc:
	cmp.w	d1,d0
L0018be:
	bcc.b	L0018c8
L0018c0:
	move.l	$3c4(a6),d2
L0018c4:
	add.l	d0,d2
L0018c6:
	bra.b	L0018ce
L0018c8:
	move.l	d0,d2
L0018ca:
	bset.l	#$1f,d2
L0018ce:
	ori	#$700,sr
L0018d2:
	movem.l	$30(a0),a1/a2
L0018d8:
	move.l	a1,$30(a2)
L0018dc:
	move.l	a2,$34(a1)
L0018e0:
	movea.l	a3,a1
L0018e2:
	move.l	d2,$2e0(a0)
L0018e6:
	beq.b	L001908
L0018e8:
	movea.l	$4c(a6),a2
L0018ec:
	cmp.w	$18(a2),d0
L0018f0:
	bls.b	L001900
L0018f2:
	bset.b	#$5,$1c(a2)
L0018f8:
	bra.b	L001900
L0018fa:
	cmp.l	$2e0(a1),d2
L0018fe:
	bhi.b	L001908
L001900:
	movea.l	$30(a1),a1
L001904:
	cmpa.l	a1,a3
L001906:
	bne.b	L0018fa
L001908:
	movea.l	$34(a1),a2
L00190c:
	movem.l	a1/a2,$30(a0)
L001912:
	move.l	a0,$30(a2)
L001916:
	move.l	a0,$34(a1)
L00191a:
	move	d4,sr
L00191c:
	movem.l	(sp)+,d0/d1/d2/d3/d4/a1/a2/a3
L001920:
	rts
L001922:
	trapf.l	#$0
L001928:
	addq.l	#$1,$3ac(a4)
L00192c:
	move.l	$2ac(a4),d0
L001930:
	beq.b	L001942
L001932:
	movea.l	d0,a0
L001934:
	movea.l	$8(a0),a0
L001938:
	move.l	#$5,$8(a0)
L001940:
	moveq	#$0,d0
L001942:
	moveq	#$0,d1
L001944:
	movem.l	d2/d1/d0,-(sp)
L001948:
	move.l	d2,d0
L00194a:
	beq.b	L001972
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00194c:
	dc.w	$6100
	dc.w	L0012b4-*
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001950:
	dc.w	$6500
	dc.w	L001a9c-*
L001954:
	movem.l	d0/a2,$0(sp)
L00195a:
	movea.l	a1,a0
L00195c:
	move.l	a3,-(sp)
L00195e:
	movea.l	$3a4(a6),a3
L001962:
	pea	L001970(pc)
L001966:
	move.l	$e0(a3),-(sp)
L00196a:
	movea.l	$4e0(a3),a3
L00196e:
	rts
L001970:
	dc.b	$26
L001971:
	dc.b	$5f
L001972:
	movea.l	$20(a5),a0
L001976:
	move.l	#$c8,d5
L00197c:
	tst.b	(a0)+
L00197e:
	dbeq	d5,L00197c
L001982:
	neg.w	d5
L001984:
	addi.w	#$cb,d5
L001988:
	bclr.l	#$0,d5
L00198c:
	movea.l	$20(a5),a0
L001990:
	suba.w	d5,sp
L001992:
	movea.l	sp,a1
L001994:
	move.l	a1,$20(a5)
L001998:
	move.w	d5,d1
L00199a:
	subq.w	#$2,d1
L00199c:
	move.b	(a0)+,(a1)+
L00199e:
	dbeq	d1,L00199c
L0019a2:
	clr.b	(a1)+
L0019a4:
	movea.l	a4,a0
L0019a6:
	cmpi.w	#$20,d3
L0019aa:
	bls.b	L0019b4
L0019ac:
	move.w	#$c9,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0019b0:
	dc.w	$6000
	dc.w	L001a9c-*
L0019b4:
	moveq	#$0,d1
L0019b6:
	move.w	d3,d1
* Rohbytes statt Instruktion (cmpi.b #-0x43,(0x22,A0)) -- siehe FORCE_RAW_BYTES im Konverter
L0019b8:
	dc.b	$0c,$28,$00,$bd,$00,$22
L0019be:
	bne.b	L0019c4
L0019c0:
	addq.l	#$1,$794(a6)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0019c4:
	dc.w	$6100
	dc.w	Q9_proc_slot_cleanup_25f8-*
L0019c8:
	moveq	#$0,d0
L0019ca:
	move.w	d0,$26(a4)
L0019ce:
	move.l	d0,$28(a4)
L0019d2:
	move.w	#$27,d1
L0019d6:
	lsr.w	#$2,d1
L0019d8:
	lea	$3c(a4),a0
L0019dc:
	move.l	d0,(a0)+
L0019de:
	dbf	d1,L0019dc
L0019e2:
	move.l	d0,$c(a4)
L0019e6:
	move.l	d0,$144(a4)
L0019ea:
	move.l	d0,$140(a4)
L0019ee:
	move.l	d0,$3a4(a4)
L0019f2:
	move.l	d0,$3a8(a4)
L0019f6:
	movea.l	a4,a0
L0019f8:
	suba.l	a2,a2
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0019fa:
	dc.w	$6100
	dc.w	L0029e6-*
L0019fe:
	adda.w	d5,sp
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001a00:
	dc.w	$6500
	dc.w	L001a9c-*
L001a04:
	movea.l	$4(sp),a0
L001a08:
	movea.l	$c(a4),a2
L001a0c:
	move.l	$8(sp),d2
L001a10:
	beq.b	L001a34
L001a12:
	move.l	a3,-(sp)
L001a14:
	movea.l	$3a4(a6),a3
L001a18:
	pea	L001a26(pc)
L001a1c:
	move.l	$e0(a3),-(sp)
L001a20:
	movea.l	$4e0(a3),a3
L001a24:
	rts
L001a26:
	dc.b	$26
L001a27:
	dc.b	$5f
L001a28:
	dc.b	$24
L001a29:
	dc.b	$6f
L001a2a:
	dc.b	$00
L001a2b:
	dc.b	$04
L001a2c:
	dc.b	$20
L001a2d:
	dc.b	$2f
L001a2e:
	dc.b	$00
L001a2f:
	dc.b	$00
L001a30:
	dc.b	$61
L001a31:
	dc.b	$00
L001a32:
	dc.b	$f8
L001a33:
	dc.b	$ea
L001a34:
	movea.l	$8(a4),sp
L001a38:
	movea.l	$3c(sp),a0
L001a3c:
	move	a0,usp
L001a3e:
	move.l	$334(a4),d0
L001a42:
	beq.b	L001a54
L001a44:
	movea.l	d0,a0
L001a46:
	clr.l	$74(a0)
L001a4a:
	clr.l	$4(a0)
L001a4e:
	bclr.b	#$0,$23(a4)
L001a54:
	movea.l	a4,a0
L001a56:
	subq.l	#$1,$3ac(a4)
L001a5a:
	moveq	#$5,d0
L001a5c:
	move.l	a3,-(sp)
L001a5e:
	movea.l	$3a4(a6),a3
L001a62:
	pea	L001a70(pc)
L001a66:
	move.l	$164(a3),-(sp)
L001a6a:
	movea.l	$564(a3),a3
L001a6e:
	rts
L001a70:
	movea.l	(sp)+,a3
L001a72:
	bcs.b	L001aae
L001a74:
	tst.l	$2ac(a4)
L001a78:
	bne.b	L001a8a
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001a7a:
	dc.w	$6100
	dc.w	Q9_scheduler_183a-*
L001a7e:
	ori	#$700,sr
L001a82:
	move.l	sp,$8(a4)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001a86:
	dc.w	$6000
	dc.w	Q9_reschedule_trampolin_3140-*
L001a8a:
	bset.b	#$7,$40(sp)
L001a90:
	movea.l	$2ac(a4),a0
L001a94:
	movea.l	$8(a0),a5
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001a98:
	dc.w	$6000
	dc.w	L000c74-*
L001a9c:
	move.l	$0(sp),d0
L001aa0:
	beq.b	L001aae
L001aa2:
	movea.l	$4(sp),a2
L001aa6:
	move.l	d1,(sp)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001aa8:
	dc.w	$6100
	dc.w	Q9_dealloc_tail_131c-*
L001aac:
	move.l	(sp),d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001aae:
	dc.w	$6000
	dc.w	Q9_exc_default_action_24d8-*
L001ab2:
	dc.b	$51
L001ab3:
	dc.b	$fb
L001ab4:
	dc.b	$00
L001ab5:
	dc.b	$00
L001ab6:
	dc.b	$00
L001ab7:
	dc.b	$00
L001ab8:
	movem.l	a2/a1/a0/d3/d2/d0,-(sp)
L001abc:
	move.l	d1,d3
L001abe:
	move.w	$3b0(a4),d2
L001ac2:
	sub.w	$3b2(a4),d2
L001ac6:
	lea	$4c(a4,d2.w*1),a2
L001aca:
	moveq	#$0,d2
L001acc:
	bra.b	L001b2e
L001ace:
	movem.l	a2/a1/a0/d3/d2/d0,-(sp)
L001ad2:
	moveq	#$0,d2
L001ad4:
	bra.b	L001b2e
L001ad6:
	move.b	(a0)+,d0
L001ad8:
	cmpi.b	#$2a,d0
L001adc:
	bne.b	L001b1e
L001ade:
	subq.w	#$1,d3
L001ae0:
	cmp.b	(a0)+,d0
L001ae2:
	beq.b	L001ade
L001ae4:
	subq.l	#$1,a0
L001ae6:
	addq.w	#$1,d3
L001ae8:
	tst.w	d3
L001aea:
	beq.b	L001b40
L001aec:
	tst.b	(a1)+
L001aee:
	beq.b	L001b38
L001af0:
	cmpa.l	a2,sp
L001af2:
	bcc.b	L001afa
L001af4:
	move.w	#$a6,d1
L001af8:
	bra.b	L001b3c
L001afa:
	bsr.b	L001ace
L001afc:
	bcc.b	L001b40
L001afe:
	cmpi.w	#$a6,d1
L001b02:
	beq.b	L001b3c
L001b04:
	addq.w	#$1,d3
L001b06:
	subq.l	#$1,a0
L001b08:
	bsr.b	L001ace
L001b0a:
	bcc.b	L001b40
L001b0c:
	cmpi.w	#$a6,d1
L001b10:
	beq.b	L001b3c
L001b12:
	subq.w	#$1,d3
L001b14:
	addq.l	#$1,a0
L001b16:
	subq.l	#$1,a1
L001b18:
	bsr.b	L001ace
L001b1a:
	bcc.b	L001b40
L001b1c:
	bra.b	L001b3c
L001b1e:
	move.b	(a1)+,d2
L001b20:
	beq.b	L001b38
L001b22:
	cmpi.b	#$3f,d0
L001b26:
	beq.b	L001b2e
L001b28:
	eor.b	d2,d0
L001b2a:
	andi.b	#-$21,d0
L001b2e:
	dbne	d3,L001ad6
L001b32:
	bne.b	L001b38
L001b34:
	tst.b	(a1)
L001b36:
	beq.b	L001b40
L001b38:
	move.w	#$a5,d1
L001b3c:
	ori	#$1,ccr
L001b40:
	movem.l	(sp)+,d0/d2/d3/a0/a1/a2
L001b44:
	rts
L001b46:
	trapf
L001b48:
	move.l	d1,d2
L001b4a:
	movea.l	a1,a2
L001b4c:
	btst.b	#$5,$40(a5)
L001b52:
	bne.b	L001b70
L001b54:
	move.l	d2,d0
L001b56:
	moveq	#$3,d1
L001b58:
	move.l	a3,-(sp)
L001b5a:
	movea.l	$3a4(a6),a3
L001b5e:
	pea	L001b6c(pc)
L001b62:
	move.l	$160(a3),-(sp)
L001b66:
	movea.l	$560(a3),a3
L001b6a:
	rts
L001b6c:
	dc.b	$26
L001b6d:
	dc.b	$5f
L001b6e:
	dc.b	$65
L001b6f:
	dc.b	$16
L001b70:
	move.l	a3,-(sp)
L001b72:
	movea.l	$3a4(a6),a3
L001b76:
	pea	L001b84(pc)
L001b7a:
	move.l	$e0(a3),-(sp)
L001b7e:
	movea.l	$4e0(a3),a3
L001b82:
	rts
L001b84:
	movea.l	(sp)+,a3
L001b86:
	rts
L001b88:
	move.l	d0,d3
L001b8a:
	move.l	a0,d0
L001b8c:
	bne.b	L001b98
L001b8e:
	andi.l	#$ffffff,d1
L001b94:
	bsr.b	L001bc4
L001b96:
	bra.b	L001b9a
L001b98:
	bsr.b	L001ba4
L001b9a:
	move.l	d1,$4(a5)
L001b9e:
	st	$4(a5)
L001ba2:
	rts
L001ba4:
	andi.l	#$ffffff,d1
L001baa:
	tst.l	d3
L001bac:
	beq.b	L001ba2
L001bae:
	move.w	a0,d0
L001bb0:
	btst.l	#$0,d0
L001bb4:
	beq.b	L001bba
L001bb6:
	bsr.b	L001bc0
L001bb8:
	subq.l	#$1,d3
L001bba:
	lsr.l	#$1,d3
L001bbc:
	bcc.b	L001c14
L001bbe:
	bsr.b	L001c14
L001bc0:
	moveq	#$0,d0
L001bc2:
	move.b	(a0)+,d0
L001bc4:
	swap	d0
L001bc6:
	eor.l	d1,d0
L001bc8:
	clr.w	d0
L001bca:
	lsl.l	#$8,d1
L001bcc:
	swap	d0
L001bce:
	move.w	d0,d2
L001bd0:
	lsl.l	#$1,d0
L001bd2:
	eor.l	d0,d1
L001bd4:
	lsl.l	#$5,d0
L001bd6:
	eor.l	d0,d1
L001bd8:
	andi.w	#$ff,d2
L001bdc:
	move.w	d2,d0
L001bde:
	lsr.b	#$3,d0
L001be0:
	btst.b	d2,L001bf4(pc,d0.w*1)
L001be4:
	beq.b	L001bec
L001be6:
	eori.l	#$800021,d1
L001bec:
	andi.l	#$ffffff,d1
L001bf2:
	rts
L001bf4:
	sub.w	$6996(a1),d3
L001bf8:
	bvs.b	L001b90
L001bfa:
	sub.w	$6996(a1),d3
L001bfe:
	sub.w	-$6997(a1),d3
L001c02:
	bvs.b	L001b9a
L001c04:
	bvs.b	L001b9c
L001c06:
	sub.w	-$6997(a1),d3
L001c0a:
	bvs.b	L001ba2
L001c0c:
	sub.w	$6996(a1),d3
L001c10:
	bvs.b	L001ba8
L001c12:
	dc.b	$96
L001c13:
	dc.b	$69
L001c14:
	movem.l	d5/d4,-(sp)
L001c18:
	move.l	#$800021,d5
L001c1e:
	lsr.l	#$1,d3
L001c20:
	bcc.b	L001c6c
L001c22:
	bsr.b	L001bc0
L001c24:
	bsr.b	L001bc0
L001c26:
	bra.b	L001c6c
L001c28:
	lsl.l	#$8,d1
L001c2a:
	move.l	(a0)+,d0
L001c2c:
	eor.l	d0,d1
L001c2e:
	move.l	d1,d2
L001c30:
	move.w	d1,d4
L001c32:
	clr.w	d2
L001c34:
	swap	d2
L001c36:
	moveq	#-$7d,d0
L001c38:
	and.w	d2,d0
L001c3a:
	eor.w	d0,d4
L001c3c:
	move.l	#$7fffff,d0
L001c42:
	and.l	d1,d0
L001c44:
	lsl.l	#$5,d0
L001c46:
	eor.l	d0,d1
L001c48:
	move.l	d1,d0
L001c4a:
	andi.w	#-$4,d2
L001c4e:
	lsl.l	#$3,d2
L001c50:
	eor.l	d2,d1
L001c52:
	moveq	#$17,d2
L001c54:
	lsr.l	d2,d0
L001c56:
	eor.l	d0,d1
L001c58:
	add.l	d1,d1
L001c5a:
	move.w	d4,d0
L001c5c:
	lsr.w	#$8,d0
L001c5e:
	eor.b	d4,d0
L001c60:
	move.w	d0,d4
L001c62:
	lsr.b	#$3,d4
L001c64:
	btst.b	d0,L001bf4(pc,d4.w*1)
L001c68:
	beq.b	L001c6c
L001c6a:
	eor.l	d5,d1
L001c6c:
	btst.b	#$3,$39(a6)
L001c72:
	beq.b	L001c9e
L001c74:
	tst.b	d3
L001c76:
	bne.b	L001c9e
L001c78:
	tst.l	d3
L001c7a:
	beq.b	L001ca8
L001c7c:
	btst.b	#$5,$1c(a4)
L001c82:
	beq.b	L001c9e
L001c84:
	movem.l	a5/a1/d1/d0,-(sp)
L001c88:
	lea	$37c(a6),a1
L001c8c:
	cmpa.l	$30(a1),a1
L001c90:
	beq.b	L001c9a
L001c92:
	movea.l	sp,a5
L001c94:
	moveq	#$1,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001c96:
	dc.w	$6100
	dc.w	L003a06-*
L001c9a:
	movem.l	(sp)+,d0/d1/a1/a5
L001c9e:
	dbf	d3,L001c28
L001ca2:
	addq.w	#$1,d3
L001ca4:
	subq.l	#$1,d3
L001ca6:
	bcc.b	L001c28
L001ca8:
	movem.l	(sp)+,d4/d5
L001cac:
	andi.l	#$ffffff,d1
L001cb2:
	rts
L001cb4:
	trapf.w	#$0
L001cb8:
	move.l	a2,$4c(a6)
L001cbc:
	bsr.b	L001cd0
L001cbe:
	subq.l	#$4,sp
L001cc0:
	move	sr,$0(sp)
L001cc4:
	move.l	a4,$4c(a6)
L001cc8:
	move	$0(sp),ccr
L001ccc:
	addq.l	#$4,sp
L001cce:
	rts
L001cd0:
	movem.l	d4/d3,-(sp)
L001cd4:
	bclr.l	#$f,d2
L001cd8:
	bne.b	L001ce4
L001cda:
	move.w	#$400,$2(sp)
L001ce0:
	clr.l	$4(sp)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001ce4:
	dc.w	$6100
	dc.w	L0032fa-*
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001ce8:
	dc.w	$6500
	dc.w	L001dfa-*
L001cec:
	moveq	#$0,d3
L001cee:
	move.w	d1,d3
L001cf0:
	movea.l	a0,a3
L001cf2:
	movea.l	a1,a0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001cf4:
	dc.w	$6100
	dc.w	L0032f0-*
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001cf8:
	dc.w	$6400
	dc.w	L001df6-*
L001cfc:
	move.l	a0,$20(a5)
L001d00:
	moveq	#$3b,d0
L001d02:
	add.w	d3,d0
L001d04:
	add.l	$0(a5),d0
L001d08:
	move.l	$4(sp),d1
L001d0c:
	move.l	a3,-(sp)
L001d0e:
	movea.l	$3a4(a6),a3
L001d12:
	pea	L001d20(pc)
L001d16:
	move.l	$170(a3),-(sp)
L001d1a:
	movea.l	$570(a3),a3
L001d1e:
	rts
L001d20:
	movea.l	(sp)+,a3
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001d22:
	dc.w	$6500
	dc.w	L001dfa-*
L001d26:
	movea.l	a2,a1
L001d28:
	move.l	d0,d2
L001d2a:
	lsr.l	#$2,d2
L001d2c:
	subq.l	#$1,d2
L001d2e:
	moveq	#$0,d1
L001d30:
	move.l	d1,(a2)+
L001d32:
	dbf	d2,L001d30
L001d36:
	addq.w	#$1,d2
L001d38:
	subq.l	#$1,d2
L001d3a:
	bcc.b	L001d30
L001d3c:
	move.w	#$4afc,$0(a1)
L001d42:
	move.w	#$1,$2(a1)
L001d48:
	move.l	d0,$4(a1)
L001d4c:
	sub.l	d3,d0
L001d4e:
	subq.l	#$5,d0
L001d50:
	bclr.l	#$0,d0
L001d54:
	move.l	d0,$c(a1)
L001d58:
	move.l	$14(a4),$8(a1)
L001d5e:
	move.w	$a(a5),d0
L001d62:
	bclr.l	#$f,d0
L001d66:
	move.w	d0,$10(a1)
L001d6a:
	move.w	$2(sp),$12(a1)
L001d70:
	move.w	$6(a5),$14(a1)
L001d76:
	move.w	#$1,$16(a1)
L001d7c:
	moveq	#$34,d1
L001d7e:
	move.l	d1,$30(a1)
L001d82:
	movea.l	a3,a0
L001d84:
	move.l	d3,d2
L001d86:
	movea.l	a1,a2
L001d88:
	adda.l	$c(a1),a2
L001d8c:
	move.l	a3,-(sp)
L001d8e:
	movea.l	$3a4(a6),a3
L001d92:
	pea	L001da0(pc)
L001d96:
	move.l	$e0(a3),-(sp)
L001d9a:
	movea.l	$4e0(a3),a3
L001d9e:
	rts
L001da0:
	movea.l	(sp)+,a3
L001da2:
	movea.l	a1,a0
L001da4:
	movea.l	a1,a2
L001da6:
	moveq	#$1,d2
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001da8:
	dc.w	$6100
	dc.w	L003072-*
L001dac:
	bcs.b	L001e00
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001dae:
	dc.w	$6100
	dc.w	L003660-*
L001db2:
	move.l	a0,d0
L001db4:
	move.l	$4(a0),d1
L001db8:
	moveq	#$1,d3
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001dba:
	dc.w	$6100
	dc.w	L00429a-*
L001dbe:
	bcs.b	L001e00
L001dc0:
	movea.l	$0(a2),a0
L001dc4:
	moveq	#$0,d0
L001dc6:
	moveq	#$0,d1
L001dc8:
	move.w	$12(a0),d0
L001dcc:
	move.w	$14(a0),d1
L001dd0:
	movem.l	d0/d1,$0(a5)
L001dd6:
	btst.b	#$5,$40(a5)
L001ddc:
	beq.b	L001de2
L001dde:
	movea.l	$50(a6),a4
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001de2:
	dc.w	$6100
	dc.w	L002fdc-*
L001de6:
	movea.l	$4c(a6),a4
L001dea:
	bcs.b	L001df2
L001dec:
	movem.l	a1/a2,$24(a5)
L001df2:
	addq.l	#$8,sp
L001df4:
	rts
L001df6:
	move.w	#$eb,d1
L001dfa:
	ori	#$1,ccr
L001dfe:
	bra.b	L001df2
L001e00:
	movea.l	a0,a2
L001e02:
	move.l	d1,-(sp)
L001e04:
	move.l	$4(a2),d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001e08:
	dc.w	$6100
	dc.w	Q9_dealloc_tail_131c-*
L001e0c:
	move.l	(sp)+,d1
L001e0e:
	ori	#$1,ccr
L001e12:
	bra.b	L001df2
L001e14:
	dc.b	$51
L001e15:
	dc.b	$fa
L001e16:
	dc.b	$00
L001e17:
	dc.b	$00
* Duenner Weiterreicher zu Q9_proc_id_free_3370.
Q9_proc_id_free_wrap_1e18:
	movem.l	a2/a1/a0/d0,-(sp)
L001e1c:
	movea.l	$44(a6),a0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001e20:
	dc.w	$6100
	dc.w	Q9_proc_id_free_3370-*
L001e24:
	movem.l	(sp)+,d0/a0/a1/a2
L001e28:
	rts
L001e2a:
	trapf.l	#$0
L001e30:
	andi	#-$2,ccr
L001e34:
	rts
L001e36:
	trapf
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001e38:
	dc.w	$6100
	dc.w	Q9_proc_id_lookup_2cee-*
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001e3c:
	dc.w	$6500
	dc.w	L001ff8-*
L001e40:
	addq.l	#$1,$3ac(a4)
L001e44:
	cmpa.l	$2ac(a1),a4
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001e48:
	dc.w	$6600
	dc.w	L001fe0-*
L001e4c:
	btst.b	#$1,$1c(a1)
L001e52:
	beq.b	L001e6c
L001e54:
	cmpi.b	#$6,$21(a1)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001e5a:
	dc.w	$6700
	dc.w	L001fe6-*
L001e5e:
	tst.l	$38(a1)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001e62:
	dc.w	$6700
	dc.w	L001fe6-*
L001e66:
	bclr.b	#$1,$1c(a1)
L001e6c:
	andi.b	#-$2,$41(a5)
L001e72:
	movea.l	$20(a5),a0
L001e76:
	tst.l	d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001e78:
	dc.w	$6a00
	dc.w	L001f46-*
L001e7c:
	addq.l	#$1,d1
L001e7e:
	bne.b	L001e86
L001e80:
	cmpi.w	#$10,d2
L001e84:
	bls.b	L001eae
L001e86:
	move.w	#$e1,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001e8a:
	dc.w	$6000
	dc.w	L001ff0-*
L001e8e:
	movea.l	(a0)+,a2
L001e90:
	moveq	#$1,d0
L001e92:
	moveq	#$1,d1
L001e94:
	move.l	a3,-(sp)
L001e96:
	movea.l	$3a4(a6),a3
L001e9a:
	pea	L001ea8(pc)
L001e9e:
	move.l	$160(a3),-(sp)
L001ea2:
	movea.l	$560(a3),a3
L001ea6:
	rts
L001ea8:
	dc.b	$26
L001ea9:
	dc.b	$5f
L001eaa:
	dc.b	$65
L001eab:
	dc.b	$00
L001eac:
	dc.b	$01
L001ead:
	dc.b	$44
L001eae:
	dbf	d2,L001e8e
L001eb2:
	move.w	$a(a5),d2
L001eb6:
	movea.l	$20(a5),a0
L001eba:
	lea	$2ec(a1),a3
L001ebe:
	bra.b	L001eee
L001ec0:
	movea.l	(a0)+,a2
L001ec2:
	move.w	(a2),(a3)
L001ec4:
	move.w	(a3)+,(a2)
L001ec6:
	move	sr,d0
L001ec8:
	move.l	d0,-(sp)
L001eca:
	ori	#$700,sr
L001ece:
	movec	caar,d0
L001ed2:
	move.l	d0,-(sp)
L001ed4:
	movec	a2,caar
L001ed8:
	movec	cacr,d0
L001edc:
	bset.l	#$2,d0
L001ee0:
	movec	d0,cacr
L001ee4:
	move.l	(sp)+,d0
L001ee6:
	movec	d0,caar
L001eea:
	move.l	(sp)+,d0
L001eec:
	move	d0,sr
L001eee:
	dbf	d2,L001ec0
L001ef2:
	move.b	$3e0(a6),d0
L001ef6:
	btst.l	#$0,d0
L001efa:
	beq.b	L001f02
L001efc:
	btst.l	#$1,d0
L001f00:
	bne.b	L001f1a
L001f02:
	moveq	#$44,d0
L001f04:
	move.l	a3,-(sp)
L001f06:
	movea.l	$3a4(a6),a3
L001f0a:
	pea	L001f18(pc)
L001f0e:
	move.l	$168(a3),-(sp)
L001f12:
	movea.l	$568(a3),a3
L001f16:
	rts
L001f18:
	dc.b	$26
L001f19:
	dc.b	$5f
L001f1a:
	movea.l	$8(a1),a2
L001f1e:
	bclr.b	#$7,$40(a2)
L001f24:
	bra.b	L001f54
L001f26:
	movea.l	(a0)+,a2
L001f28:
	moveq	#$5,d1
L001f2a:
	moveq	#$2,d0
L001f2c:
	move.l	a3,-(sp)
L001f2e:
	movea.l	$3a4(a6),a3
L001f32:
	pea	L001f40(pc)
L001f36:
	move.l	$160(a3),-(sp)
L001f3a:
	movea.l	$560(a3),a3
L001f3e:
	rts
L001f40:
	dc.b	$26
L001f41:
	dc.b	$5f
L001f42:
	dc.b	$65
L001f43:
	dc.b	$00
L001f44:
	dc.b	$00
L001f45:
	dc.b	$ac
L001f46:
	dbf	d2,L001f26
L001f4a:
	movea.l	$8(a1),a2
L001f4e:
	bset.b	#$7,$40(a2)
L001f54:
	movea.l	$2a8(a1),a0
L001f58:
	movem.l	$0(a0),d0/d1/d2/d3/d4/d5/d6/d7
L001f5e:
	movem.l	d0/d1/d2/d3/d4/d5/d6/d7,$0(a2)
L001f64:
	movem.l	$20(a0),d0/d1/d2/d3/d4/d5/d6/d7
L001f6a:
	movem.l	d0/d1/d2/d3/d4/d5/d6,$20(a2)
L001f70:
	move.l	d7,$c(a1)
L001f74:
	move.b	$41(a0),$41(a2)
L001f7a:
	move.l	$42(a0),$42(a2)
L001f80:
	move.l	$8(a5),$2e8(a1)
L001f86:
	move.b	$4(a5),$2e8(a1)
L001f8c:
	bmi.b	L001f94
L001f8e:
	bset.b	#$7,$3ac(a1)
L001f94:
	clr.l	$8(a5)
L001f98:
	tst.b	$2f(a6)
L001f9c:
	beq.b	L001fcc
L001f9e:
	lea	$6c(a0),a0
* Vollformat-Adressierung erzwungen -- siehe FULL_EXT_OVERRIDE im Konverter
L001fa2:
	lea	([($334).w,a1],($8).w),a2
L001faa:
	move.l	#$6c,d2
L001fb0:
	tst.b	$0(a0,d2.w*1)
L001fb4:
	beq.b	L001fcc
L001fb6:
	move.l	a3,-(sp)
L001fb8:
	movea.l	$3a4(a6),a3
L001fbc:
	pea	L001fca(pc)
L001fc0:
	move.l	$e0(a3),-(sp)
L001fc4:
	movea.l	$4e0(a3),a3
L001fc8:
	rts
L001fca:
	dc.b	$26
L001fcb:
	dc.b	$5f
L001fcc:
	movea.l	a1,a0
L001fce:
	move.b	#$64,$20(a4)
L001fd4:
	move.l	a5,$8(a4)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001fd8:
	dc.w	$6100
	dc.w	Q9_scheduler_183a-*
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L001fdc:
	dc.w	$6000
	dc.w	Q9_reschedule_trampolin_3140-*
L001fe0:
	move.w	#$e0,d1
L001fe4:
	bra.b	L001ff0
L001fe6:
	move.w	#$e4,d1
L001fea:
	move.l	$2b0(a1),$0(a5)
L001ff0:
	subq.l	#$1,$3ac(a4)
L001ff4:
	ori	#$1,ccr
L001ff8:
	rts
L001ffa:
	dc.b	$51
L001ffb:
	dc.b	$fb
L001ffc:
	dc.b	$00
L001ffd:
	dc.b	$00
L001ffe:
	dc.b	$00
L001fff:
	dc.b	$00
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002000:
	dc.w	$6100
	dc.w	Q9_proc_id_lookup_2cee-*
L002004:
	bcs.b	L002032
L002006:
	cmpa.l	$2ac(a1),a4
L00200a:
	bne.b	L00202a
L00200c:
	addq.l	#$1,$3ac(a1)
L002010:
	move.l	$c(a4),$c(a1)
L002016:
	movea.l	a1,a0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002018:
	dc.w	$6100
	dc.w	Q9_proc_die_prep_2590-*
L00201c:
	move.w	$0(a1),d0
L002020:
	movea.l	a4,a0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002022:
	dc.w	$6100
	dc.w	L00452c-*
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002026:
	dc.w	$6000
	dc.w	Q9_proc_id_free_wrap_1e18-*
L00202a:
	move.w	#$e0,d1
L00202e:
	ori	#$1,ccr
L002032:
	rts
L002034:
	trapf.w	#$0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002038:
	dc.w	$6100
	dc.w	L0016b6-*
L00203c:
	bcs.b	L0020b2
L00203e:
	move.l	a4,$2ac(a1)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002042:
	dc.w	$6100
	dc.w	L0028aa-*
L002046:
	bcs.b	L0020b2
L002048:
	movea.l	$38(a0),a2
L00204c:
	btst.b	#$5,$14(a2)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002052:
	dc.w	$6600
	dc.w	L00297a-*
L002056:
	tst.w	$8(a2)
L00205a:
	bne.b	L002064
L00205c:
	tst.w	$14(a4)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002060:
	dc.w	$6600
	dc.w	L00297a-*
L002064:
	movea.l	$28(a5),a2
L002068:
	moveq	#$46,d0
L00206a:
	moveq	#$3,d1
L00206c:
	move.l	a3,-(sp)
L00206e:
	movea.l	$3a4(a6),a3
L002072:
	pea	L002080(pc)
L002076:
	move.l	$160(a3),-(sp)
L00207a:
	movea.l	$560(a3),a3
L00207e:
	rts
L002080:
	movea.l	(sp)+,a3
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002082:
	dc.w	$6500
	dc.w	L00297e-*
L002086:
	move.l	a2,$2a8(a0)
L00208a:
	move.b	#$64,$20(a0)
L002090:
	movea.l	$8(a0),a0
L002094:
	bset.b	#$7,$40(a0)
L00209a:
	moveq	#$46,d2
L00209c:
	move.l	a3,-(sp)
L00209e:
	movea.l	$3a4(a6),a3
L0020a2:
	pea	L0020b0(pc)
L0020a6:
	move.l	$e0(a3),-(sp)
L0020aa:
	movea.l	$4e0(a3),a3
L0020ae:
	rts
L0020b0:
	dc.b	$26
L0020b1:
	dc.b	$5f
L0020b2:
	rts
L0020b4:
	trapf.w	#$0
* Rohbytes statt Instruktion (ori #0x8) -- siehe FORCE_RAW_BYTES im Konverter
L0020b8:
	dc.b	$00,$3c,$03,$08
L0020bc:
	btst.b	d0,$00000068.w
L0020c0:
	dc.b	$02
L0020c1:
	dc.b	$4c
L0020c2:
	dc.b	$02
L0020c3:
	dc.b	$1c
L0020c4:
	dc.b	$03
L0020c5:
	dc.b	$18
L0020c6:
	dc.b	$03
L0020c7:
	dc.b	$24
L0020c8:
	dc.b	$03
L0020c9:
	dc.b	$94
L0020ca:
	dc.b	$03
L0020cb:
	dc.b	$fe
L0020cc:
	dc.b	$04
L0020cd:
	dc.b	$10
L0020ce:
	dc.b	$04
L0020cf:
	dc.b	$18
L0020d0:
	dc.b	$43
L0020d1:
	dc.b	$fa
L0020d2:
	dc.b	$ff
L0020d3:
	dc.b	$e6
L0020d4:
	dc.b	$d2
L0020d5:
	dc.b	$41
L0020d6:
	dc.b	$55
L0020d7:
	dc.b	$c4
L0020d8:
	dc.b	$0c
L0020d9:
	dc.b	$41
L0020da:
	dc.b	$00
L0020db:
	dc.b	$18
L0020dc:
	dc.b	$64
L0020dd:
	dc.b	$00
L0020de:
	dc.b	$f2
L0020df:
	dc.b	$a2
L0020e0:
	dc.b	$32
L0020e1:
	dc.b	$31
L0020e2:
	dc.b	$10
L0020e3:
	dc.b	$00
L0020e4:
	dc.b	$40
L0020e5:
	dc.b	$c7
L0020e6:
	dc.b	$4e
L0020e7:
	dc.b	$b1
L0020e8:
	dc.b	$10
L0020e9:
	dc.b	$00
L0020ea:
	bcc.b	L0020f0
L0020ec:
	ori.w	#$1,d7
L0020f0:
	move	d7,sr
L0020f2:
	rts
L0020f4:
	addq.l	#$1,$3ac(a4)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0020f8:
	dc.w	$6100
	dc.w	L00217e-*
L0020fc:
	bcs.b	L002116
L0020fe:
	addq.w	#$1,$16(a2)
L002102:
	bcc.b	L002108
L002104:
	subq.w	#$1,$16(a2)
L002108:
	move.l	d0,$0(a5)
L00210c:
	move.l	a0,$20(a5)
L002110:
	subq.l	#$1,$3ac(a4)
L002114:
	rts
L002116:
	subq.l	#$1,$3ac(a4)
L00211a:
	ori	#$1,ccr
L00211e:
	rts
L002120:
	addq.l	#$1,$3ac(a4)
L002124:
	bsr.b	L00217e
L002126:
	bcs.b	L002116
L002128:
	cmpi.w	#$1,$16(a2)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00212e:
	dc.w	$6400
	dc.w	L0022c6-*
L002132:
	move.l	a0,$20(a5)
L002136:
	lea	-$18(a2),a3
L00213a:
	move.l	a3,d3
L00213c:
	movea.l	$30(a3),a3
L002140:
	bra.b	L002170
L002142:
	movea.l	$8(a3),a5
L002146:
	lea	L0020ea(pc),a0
L00214a:
	cmpa.l	$42(a5),a0
L00214e:
	bne.b	L002158
L002150:
	ori.w	#$1,$1e(a5)
L002156:
	bra.b	L00215e
L002158:
	ori.w	#$1,$40(a5)
L00215e:
	move.l	#$a7,$4(a5)
L002166:
	movea.l	a3,a0
L002168:
	movea.l	$30(a3),a3
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00216c:
	dc.w	$6100
	dc.w	Q9_scheduler_183a-*
L002170:
	cmp.l	a3,d3
L002172:
	bne.b	L002142
L002174:
	clr.w	$0(a2)
L002178:
	subq.l	#$1,$3ac(a4)
L00217c:
	rts
L00217e:
	movem.l	a1/d4/d3,-(sp)
L002182:
	sub.l	d3,d3
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002184:
	dc.w	$6100
	dc.w	L0032fa-*
L002188:
	bcs.b	L0021e4
L00218a:
	move.l	d1,d2
L00218c:
	ori	#$700,sr
L002190:
	movea.l	$3cc(a6),a2
L002194:
	move.l	$3d0(a6),d4
L002198:
	sub.l	a2,d4
L00219a:
	lsr.l	#$5,d4
L00219c:
	cmpi.w	#$b,d2
L0021a0:
	bls.b	L0021dc
L0021a2:
	move.w	#$eb,d1
L0021a6:
	bra.b	L0021e4
L0021a8:
	tst.w	$0(a2)
L0021ac:
	beq.b	L0021d2
L0021ae:
	lea	$2(a2),a1
L0021b2:
	move.l	d2,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0021b4:
	dc.w	$6100
	dc.w	L001ab8-*
L0021b8:
	bcs.b	L0021d8
L0021ba:
	adda.w	d2,a0
L0021bc:
	movem.l	(sp)+,d3/d4/a1
L0021c0:
	move.l	a2,d0
L0021c2:
	sub.l	$3cc(a6),d0
L0021c6:
	lsr.l	#$5,d0
L0021c8:
	swap	d0
L0021ca:
	move.w	$0(a2),d0
L0021ce:
	swap	d0
L0021d0:
	rts
L0021d2:
	tst.l	d3
L0021d4:
	bne.b	L0021d8
L0021d6:
	move.l	a2,d3
L0021d8:
	lea	$20(a2),a2
L0021dc:
	dbf	d4,L0021a8
L0021e0:
	move.w	#$a8,d1
L0021e4:
	movea.l	d3,a2
L0021e6:
	movem.l	(sp)+,d3/d4/a1
L0021ea:
	ori	#$1,ccr
L0021ee:
	rts
L0021f0:
	addq.l	#$1,$3ac(a4)
L0021f4:
	bsr.b	L00217e
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0021f6:
	dc.w	$6400
	dc.w	L0022c6-*
L0021fa:
	cmpi.w	#$a8,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0021fe:
	dc.w	$6600
	dc.w	L0022ca-*
L002202:
	move.l	a2,d0
L002204:
	bne.b	L00226e
L002206:
	btst.b	#$0,$39(a6)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00220c:
	dc.w	$6600
	dc.w	L0022c0-*
L002210:
	ori	#$700,sr
L002214:
	movea.l	$3cc(a6),a0
L002218:
	move.l	$3d0(a6),d0
L00221c:
	sub.l	a0,d0
L00221e:
	cmpi.l	#$100000,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002224:
	dc.w	$6200
	dc.w	L0022c0-*
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002228:
	dc.w	$6100
	dc.w	L0043b0-*
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00222c:
	dc.w	$6500
	dc.w	L0022ca-*
L002230:
	move.l	$3cc(a6),d1
L002234:
	cmp.l	$3d0(a6),d1
L002238:
	movem.l	a0/a1,$3cc(a6)
L00223e:
	beq.b	L00226e
L002240:
	sub.l	a0,d1
L002242:
	lea	-$18(a0,d1.l*1),a1
L002246:
	cmpa.l	$18(a0),a1
L00224a:
	bne.b	L002256
L00224c:
	sub.l	d1,$18(a0)
L002250:
	sub.l	d1,$1c(a0)
L002254:
	bra.b	L002266
L002256:
	movea.l	$1c(a0),a1
L00225a:
	sub.l	d1,$30(a1)
L00225e:
	movea.l	$18(a0),a1
L002262:
	sub.l	d1,$34(a1)
L002266:
	lea	$20(a0),a0
L00226a:
	cmpa.l	a2,a0
L00226c:
	bcs.b	L002242
L00226e:
	addq.w	#$1,$3d4(a6)
L002272:
	beq.b	L00226e
L002274:
	move.w	$3d4(a6),$0(a2)
L00227a:
	movea.l	$20(a5),a0
L00227e:
	lea	$2(a2),a1
L002282:
	subq.w	#$1,d2
L002284:
	move.b	(a0)+,(a1)+
L002286:
	dbf	d2,L002284
L00228a:
	clr.b	(a1)
L00228c:
	move.l	$0(a5),$e(a2)
L002292:
	move.w	$a(a5),$12(a2)
L002298:
	move.w	d3,$14(a2)
L00229c:
	lea	-$18(a2),a1
L0022a0:
	move.l	a1,$18(a2)
L0022a4:
	move.l	a1,$1c(a2)
L0022a8:
	move.w	#$1,$16(a2)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0022ae:
	dc.w	$6100
	dc.w	L0021c0-*
L0022b2:
	move.l	d0,$0(a5)
L0022b6:
	move.l	a0,$20(a5)
L0022ba:
	subq.l	#$1,$3ac(a4)
L0022be:
	rts
L0022c0:
	move.w	#$ed,d1
L0022c4:
	bra.b	L0022ca
L0022c6:
	move.w	#$a9,d1
L0022ca:
	subq.l	#$1,$3ac(a4)
L0022ce:
	ori	#$1,ccr
L0022d2:
	rts
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0022d4:
	dc.w	$6100
	dc.w	L002398-*
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0022d8:
	dc.w	$6500
	dc.w	L00238c-*
L0022dc:
	add.l	$e(a2),d2
L0022e0:
	bvc.b	L0022ec
L0022e2:
	moveq	#-$1,d2
L0022e4:
	roxr.l	#$1,d2
L0022e6:
	bpl.b	L0022ec
L0022e8:
	lsr.l	#$1,d2
L0022ea:
	not.l	d2
L0022ec:
	add.l	$e(a2),d3
L0022f0:
	bvc.b	L0022fc
L0022f2:
	moveq	#-$1,d3
L0022f4:
	roxr.l	#$1,d3
L0022f6:
	bpl.b	L0022fc
L0022f8:
	lsr.l	#$1,d3
L0022fa:
	not.l	d3
L0022fc:
	movem.l	d2/d3,$8(a5)
L002302:
	bra.b	L00230c
L002304:
	dc.b	$61
L002305:
	dc.b	$00
L002306:
	dc.b	$00
L002307:
	dc.b	$92
L002308:
	dc.b	$65
L002309:
	dc.b	$00
L00230a:
	dc.b	$00
L00230b:
	dc.b	$82
L00230c:
	cmp.l	d2,d3
L00230e:
	blt.b	L00238e
L002310:
	cmp.l	$e(a2),d2
L002314:
	bgt.b	L00231c
L002316:
	cmp.l	$e(a2),d3
L00231a:
	bge.b	L00236c
L00231c:
	move.l	$e(a2),$4(a5)
L002322:
	bclr.b	#$7,$371(a4)
L002328:
	beq.b	L002330
L00232a:
	tst.w	$26(a4)
L00232e:
	bne.b	L00238c
L002330:
	movea.l	$1c(a2),a0
L002334:
	move.l	a0,$34(a4)
L002338:
	move.l	$30(a0),$30(a4)
L00233e:
	move.l	a4,$1c(a2)
L002342:
	move.l	a4,$30(a0)
L002346:
	move.b	#$65,$20(a4)
L00234c:
	lea	L000558(pc),a0
L002350:
	cmpa.l	$4(sp),a0
L002354:
	bne.b	L002366
L002356:
	move.l	a5,$8(a4)
L00235a:
	move.l	d5,$140(a4)
L00235e:
	move.l	d6,$144(a4)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002362:
	dc.w	$6000
	dc.w	Q9_reschedule_trampolin_3140-*
L002366:
	move.w	d7,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002368:
	dc.w	$6000
	dc.w	L003b64-*
L00236c:
	move.w	$12(a2),d2
L002370:
	ext.l	d2
L002372:
	add.l	$e(a2),d2
L002376:
	bvc.b	L002382
L002378:
	moveq	#-$1,d2
L00237a:
	roxr.l	#$1,d2
L00237c:
	bpl.b	L002382
L00237e:
	lsr.l	#$1,d2
L002380:
	not.l	d2
L002382:
	move.l	$e(a2),$4(a5)
L002388:
	move.l	d2,$e(a2)
L00238c:
	rts
L00238e:
	move.w	#$aa,d1
L002392:
	ori	#$1,ccr
L002396:
	rts
L002398:
	ext.l	d0
L00239a:
	asl.l	#$5,d0
L00239c:
	ori	#$700,sr
L0023a0:
	movea.l	$3cc(a6),a2
L0023a4:
	move.l	$3d0(a6),d1
L0023a8:
	sub.l	a2,d1
L0023aa:
	cmp.l	d1,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0023ac:
	dc.w	$6400
	dc.w	L002432-*
L0023b0:
	adda.l	d0,a2
L0023b2:
	move.w	$0(a2),d0
L0023b6:
	beq.b	L002432
L0023b8:
	cmp.w	$0(a5),d0
L0023bc:
	bne.b	L002432
L0023be:
	rts
L0023c0:
	bsr.b	L002398
L0023c2:
	bcs.b	L0023ce
L0023c4:
	subq.w	#$1,$16(a2)
L0023c8:
	bcc.b	L0023ce
L0023ca:
	clr.w	$16(a2)
L0023ce:
	rts
L0023d0:
	dc.b	$61
L0023d1:
	dc.b	$c6
L0023d2:
	dc.b	$65
L0023d3:
	dc.b	$06
L0023d4:
	dc.b	$2b
L0023d5:
	dc.b	$6a
L0023d6:
	dc.b	$00
L0023d7:
	dc.b	$0e
L0023d8:
	dc.b	$00
L0023d9:
	dc.b	$04
L0023da:
	dc.b	$4e
L0023db:
	dc.b	$75
L0023dc:
	dc.b	$08
L0023dd:
	dc.b	$2d
L0023de:
	dc.b	$00
L0023df:
	dc.b	$05
L0023e0:
	dc.b	$00
L0023e1:
	dc.b	$40
L0023e2:
	dc.b	$66
L0023e3:
	dc.b	$24
L0023e4:
	dc.b	$2f
L0023e5:
	dc.b	$00
L0023e6:
	dc.b	$70
L0023e7:
	dc.b	$20
L0023e8:
	dc.b	$72
L0023e9:
	dc.b	$02
L0023ea:
	dc.b	$24
L0023eb:
	dc.b	$48
L0023ec:
	dc.b	$2f
L0023ed:
	dc.b	$0b
L0023ee:
	dc.b	$26
L0023ef:
	dc.b	$6e
L0023f0:
	dc.b	$03
L0023f1:
	dc.b	$a4
L0023f2:
	dc.b	$48
L0023f3:
	dc.b	$7a
L0023f4:
	dc.b	$00
L0023f5:
	dc.b	$0c
L0023f6:
	dc.b	$2f
L0023f7:
	dc.b	$2b
L0023f8:
	dc.b	$01
L0023f9:
	dc.b	$60
L0023fa:
	dc.b	$26
L0023fb:
	dc.b	$6b
L0023fc:
	dc.b	$05
L0023fd:
	dc.b	$60
L0023fe:
	dc.b	$4e
L0023ff:
	dc.b	$75
L002400:
	dc.b	$26
L002401:
	dc.b	$5f
L002402:
	dc.b	$4c
L002403:
	dc.b	$df
L002404:
	dc.b	$00
L002405:
	dc.b	$01
L002406:
	dc.b	$65
L002407:
	dc.b	$32
L002408:
	dc.b	$24
L002409:
	dc.b	$6e
L00240a:
	dc.b	$03
L00240b:
	dc.b	$cc
L00240c:
	dc.b	$26
L00240d:
	dc.b	$2e
L00240e:
	dc.b	$03
L00240f:
	dc.b	$d0
L002410:
	dc.b	$96
L002411:
	dc.b	$8a
L002412:
	dc.b	$ea
L002413:
	dc.b	$8b
L002414:
	dc.b	$96
L002415:
	dc.b	$40
L002416:
	dc.b	$63
L002417:
	dc.b	$1a
L002418:
	dc.b	$53
L002419:
	dc.b	$43
L00241a:
	dc.b	$72
L00241b:
	dc.b	$00
L00241c:
	dc.b	$32
L00241d:
	dc.b	$00
L00241e:
	dc.b	$eb
L00241f:
	dc.b	$81
L002420:
	dc.b	$d5
L002421:
	dc.b	$c1
L002422:
	dc.b	$74
L002423:
	dc.b	$20
L002424:
	dc.b	$4a
L002425:
	dc.b	$6a
L002426:
	dc.b	$00
L002427:
	dc.b	$00
L002428:
	dc.b	$66
L002429:
	dc.b	$12
L00242a:
	dc.b	$52
L00242b:
	dc.b	$40
L00242c:
	dc.b	$d5
L00242d:
	dc.b	$c2
L00242e:
	dc.b	$51
L00242f:
	dc.b	$cb
L002430:
	dc.b	$ff
L002431:
	dc.b	$f4
L002432:
	move.w	#$a7,d1
L002436:
	ori	#$1,ccr
L00243a:
	rts
L00243c:
	move.l	d0,$0(a5)
L002440:
	asr.l	#$2,d2
L002442:
	subq.l	#$1,d2
L002444:
	move.l	(a2)+,(a0)+
L002446:
	dbf	d2,L002444
L00244a:
	rts
L00244c:
	dc.b	$61
L00244d:
	dc.b	$00
L00244e:
	dc.b	$ff
L00244f:
	dc.b	$4a
L002450:
	dc.b	$65
L002451:
	dc.b	$62
L002452:
	dc.b	$34
L002453:
	dc.b	$2a
L002454:
	dc.b	$00
L002455:
	dc.b	$14
L002456:
	dc.b	$48
L002457:
	dc.b	$c2
L002458:
	dc.b	$d4
L002459:
	dc.b	$aa
L00245a:
	dc.b	$00
L00245b:
	dc.b	$0e
L00245c:
	dc.b	$68
L00245d:
	dc.b	$0a
L00245e:
	dc.b	$74
L00245f:
	dc.b	$ff
L002460:
	dc.b	$e2
L002461:
	dc.b	$92
L002462:
	dc.b	$6a
L002463:
	dc.b	$04
L002464:
	dc.b	$e2
L002465:
	dc.b	$8a
L002466:
	dc.b	$46
L002467:
	dc.b	$82
L002468:
	dc.b	$2b
L002469:
	dc.b	$6a
L00246a:
	dc.b	$00
L00246b:
	dc.b	$0e
L00246c:
	dc.b	$00
L00246d:
	dc.b	$04
L00246e:
	dc.b	$25
L00246f:
	dc.b	$42
L002470:
	dc.b	$00
L002471:
	dc.b	$0e
L002472:
	dc.b	$47
L002473:
	dc.b	$ea
L002474:
	dc.b	$ff
L002475:
	dc.b	$e8
L002476:
	dc.b	$26
L002477:
	dc.b	$0b
L002478:
	dc.b	$60
L002479:
	dc.b	$32
L00247a:
	dc.b	$2a
L00247b:
	dc.b	$6b
L00247c:
	dc.b	$00
L00247d:
	dc.b	$08
L00247e:
	dc.b	$41
L00247f:
	dc.b	$fa
L002480:
	dc.b	$fc
L002481:
	dc.b	$6a
L002482:
	dc.b	$b1
L002483:
	dc.b	$ed
L002484:
	dc.b	$00
L002485:
	dc.b	$42
L002486:
	dc.b	$66
L002487:
	dc.b	$04
L002488:
	dc.b	$2a
L002489:
	dc.b	$6d
L00248a:
	dc.b	$00
L00248b:
	dc.b	$34
L00248c:
	dc.b	$b4
L00248d:
	dc.b	$ad
L00248e:
	dc.b	$00
L00248f:
	dc.b	$08
L002490:
	dc.b	$6d
L002491:
	dc.b	$1a
L002492:
	dc.b	$b4
L002493:
	dc.b	$ad
L002494:
	dc.b	$00
L002495:
	dc.b	$0c
L002496:
	dc.b	$6e
L002497:
	dc.b	$14
L002498:
	dc.b	$20
L002499:
	dc.b	$4b
L00249a:
	dc.b	$26
L00249b:
	dc.b	$6b
L00249c:
	dc.b	$00
L00249d:
	dc.b	$30
L00249e:
	dc.b	$61
L00249f:
	dc.b	$00
L0024a0:
	dc.b	$fe
L0024a1:
	dc.b	$cc
L0024a2:
	dc.b	$61
L0024a3:
	dc.b	$00
L0024a4:
	dc.b	$f3
L0024a5:
	dc.b	$96
L0024a6:
	dc.b	$4a
L0024a7:
	dc.b	$04
L0024a8:
	dc.b	$66
L0024a9:
	dc.b	$06
L0024aa:
	dc.b	$4e
L0024ab:
	dc.b	$75
L0024ac:
	dc.b	$26
L0024ad:
	dc.b	$6b
L0024ae:
	dc.b	$00
L0024af:
	dc.b	$30
L0024b0:
	dc.b	$b6
L0024b1:
	dc.b	$8b
L0024b2:
	dc.b	$66
L0024b3:
	dc.b	$c6
L0024b4:
	dc.b	$4e
L0024b5:
	dc.b	$75
L0024b6:
	dc.b	$61
L0024b7:
	dc.b	$00
L0024b8:
	dc.b	$fe
L0024b9:
	dc.b	$e0
L0024ba:
	dc.b	$65
L0024bb:
	dc.b	$f8
L0024bc:
	dc.b	$2a
L0024bd:
	dc.b	$2a
L0024be:
	dc.b	$00
L0024bf:
	dc.b	$0e
L0024c0:
	dc.b	$61
L0024c1:
	dc.b	$a6
L0024c2:
	dc.b	$25
L0024c3:
	dc.b	$45
L0024c4:
	dc.b	$00
L0024c5:
	dc.b	$0e
L0024c6:
	dc.b	$4e
L0024c7:
	dc.b	$75
L0024c8:
	dc.b	$61
L0024c9:
	dc.b	$00
L0024ca:
	dc.b	$fe
L0024cb:
	dc.b	$ce
L0024cc:
	dc.b	$64
L0024cd:
	dc.b	$9a
L0024ce:
	dc.b	$4e
L0024cf:
	dc.b	$75
L0024d0:
	dc.b	$61
L0024d1:
	dc.b	$00
L0024d2:
	dc.b	$fe
L0024d3:
	dc.b	$c6
L0024d4:
	dc.b	$64
L0024d5:
	dc.b	$82
L0024d6:
	dc.b	$4e
L0024d7:
	dc.b	$75
* Standardaktion fuer unbehandelte Exceptions: Prozess-Terminierung inkl. Eltern-Benachrichtigung, oder Signal-Zustellung falls ein Handler existiert.
Q9_exc_default_action_24d8:
	addq.l	#$1,$3ac(a4)
L0024dc:
	bset.b	#$1,$1c(a4)
L0024e2:
	move.w	$8aa(a6),d0
L0024e6:
	cmp.w	$0(a4),d0
L0024ea:
	bne.b	L0024f0
L0024ec:
	clr.w	$8aa(a6)
L0024f0:
	tst.l	$2ac(a4)
L0024f4:
	bne.b	L002566
L0024f6:
	moveq	#$0,d0
L0024f8:
	move.w	$3b0(a4),d0
L0024fc:
	lea	-$48(a4,d0.l*1),sp
L002500:
	move.l	$334(a4),d0
L002504:
	beq.b	L00250c
L002506:
	movea.l	d0,a0
L002508:
	clr.l	$4(a0)
L00250c:
	movea.l	a4,a0
L00250e:
	move.w	d1,$26(a4)
L002512:
	bsr.b	Q9_proc_die_prep_2590
L002514:
	move.w	d1,$26(a4)
L002518:
	move.b	#$2d,$20(a4)
L00251e:
	move.w	$2(a4),d0
L002522:
	beq.b	L00254c
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002524:
	dc.w	$6100
	dc.w	Q9_proc_id_lookup_2cee-*
L002528:
	bcs.b	L00254c
L00252a:
	bset.b	#$3,$1c(a1)
L002530:
	cmpi.b	#$77,$20(a1)
L002536:
	bne.b	L002562
L002538:
	movea.l	$8(a1),a5
L00253c:
	movea.l	$34(a5),a5
L002540:
	movea.l	a1,a0
L002542:
	movea.l	a4,a1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002544:
	dc.w	$6100
	dc.w	Q9_parent_notify_4518-*
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002548:
	dc.w	$6100
	dc.w	Q9_scheduler_183a-*
L00254c:
	move.w	$0(a4),d0
L002550:
	movea.l	a4,a3
L002552:
	movea.l	$50(a6),a4
L002556:
	move.l	a4,$4c(a6)
L00255a:
	movea.l	$8(a4),sp
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00255e:
	dc.w	$6100
	dc.w	Q9_proc_id_free_wrap_1e18-*
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002562:
	dc.w	$6000
	dc.w	Q9_reschedule_trampolin_3140-*
L002566:
	movea.l	a5,sp
L002568:
	movea.l	$2ac(a4),a0
L00256c:
	movea.l	$8(a0),a5
L002570:
	cmpi.b	#$6,$21(a4)
L002576:
	bne.b	L002580
L002578:
	move.l	#$80,$8(a5)
L002580:
	bset.b	#$1,$1c(a4)
L002586:
	bne.b	L00258c
L002588:
	move.w	d1,$6(a5)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00258c:
	dc.w	$6000
	dc.w	L000c74-*
* Trampolin Tabellen-Slot 89 -- Vorbereitung kurz vor dem Sterben eines Prozesses.
Q9_proc_die_prep_2590:
	movem.l	a2/a1/a0/d7/d2/d1/d0,-(sp)
L002594:
	moveq	#$6,d0
L002596:
	move.l	a3,-(sp)
L002598:
	movea.l	$3a4(a6),a3
L00259c:
	pea	L0025aa(pc)
L0025a0:
	move.l	$164(a3),-(sp)
L0025a4:
	movea.l	$564(a3),a3
L0025a8:
	rts
L0025aa:
	movea.l	(sp)+,a3
L0025ac:
	lea	$2(a0),a1
L0025b0:
	bra.b	L0025e4
L0025b2:
	clr.w	$4(a1)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0025b6:
	dc.w	$6100
	dc.w	Q9_proc_id_lookup_2cee-*
L0025ba:
	bcs.b	L0025ea
L0025bc:
	clr.w	$2(a1)
L0025c0:
	btst.b	#$0,$1c(a1)
L0025c6:
	bne.b	L0025d2
L0025c8:
	tst.l	$2ac(a1)
L0025cc:
	beq.b	L0025e4
L0025ce:
	movea.l	a1,a0
L0025d0:
	bsr.b	Q9_proc_die_prep_2590
L0025d2:
	move.w	$0(a1),d0
L0025d6:
	move.w	$4(a1),d2
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0025da:
	dc.w	$6100
	dc.w	Q9_proc_id_free_wrap_1e18-*
L0025de:
	move.w	d2,d0
L0025e0:
	beq.b	L0025ea
L0025e2:
	bra.b	L0025b6
L0025e4:
	move.w	$4(a1),d0
L0025e8:
	bne.b	L0025b2
L0025ea:
	movea.l	$10(sp),a0
L0025ee:
	moveq	#$0,d1
L0025f0:
	bsr.b	Q9_proc_slot_cleanup_25f8
L0025f2:
	movem.l	(sp)+,d0/d1/d2/d7/a0/a1/a2
L0025f6:
	rts
* Prozessdeskriptor-Slot aufraeumen: Ressourcenlisten, offene Pfade (echter TRAP #0-Close), FPU-Ownership.
Q9_proc_slot_cleanup_25f8:
	movem.l	a3/a2/a1/a0/d1,-(sp)
L0025fc:
	exg	a0,a4
L0025fe:
	moveq	#$0,d0
L002600:
	moveq	#$0,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002602:
	dc.w	$6100
	dc.w	Q9_category_dispatch_1390-*
L002606:
	exg	a4,a0
L002608:
	lea	$c8(a0),a3
L00260c:
	moveq	#$e,d0
L00260e:
	move.l	a0,$4c(a6)
L002612:
	tst.l	-(a3)
L002614:
	dbne	d0,L002612
L002618:
	beq.b	L002622
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00261a:
	dc.w	$6100
	dc.w	L00403a-*
L00261e:
	dbf	d0,L002612
L002622:
	moveq	#$20,d2
L002624:
	sub.l	(sp)+,d2
L002626:
	lea	$1a8(a0),a1
L00262a:
	bra.b	L002636
L00262c:
	move.w	-(a1),d0
L00262e:
	beq.b	L002636
L002630:
	clr.w	(a1)
L002632:
	trap	#$0
L002634:
	dc.b	$00
L002635:
	dc.b	$8f
L002636:
	dbf	d2,L00262c
L00263a:
	bset.b	#$0,$1c(a0)
L002640:
	move.l	a0,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002642:
	dc.w	$6100
	dc.w	Q9_proc_resource_free_62da-*
L002646:
	move.l	a4,$4c(a6)
L00264a:
	tst.l	$37c(a0)
L00264e:
	beq.b	L002674
L002650:
	movea.l	$37c(a0),a1
L002654:
	lea	$380(a0),a2
L002658:
	move.l	a2,d2
L00265a:
	movea.l	a1,a2
L00265c:
	movea.l	$0(a2),a1
L002660:
	cmp.l	a2,d2
L002662:
	beq.b	L00266a
L002664:
	moveq	#$10,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002666:
	dc.w	$6100
	dc.w	Q9_dealloc_tail_131c-*
L00266a:
	cmpa.l	$37c(a0),a1
L00266e:
	bne.b	L00265a
L002670:
	clr.l	$37c(a0)
L002674:
	tst.b	$2f(a6)
L002678:
	beq.b	L00268c
L00267a:
	cmpa.l	$58(a6),a0
L00267e:
	bne.b	L00268c
L002680:
	frestore	L000050(pc)
L002684:
	clr.l	$334(a0)
L002688:
	clr.l	$58(a6)
L00268c:
	movea.l	$38(a0),a2
L002690:
	clr.l	$38(a0)
L002694:
	exg	a0,a4
L002696:
	move.l	a2,d0
L002698:
	beq.b	L00269e
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00269a:
	dc.w	$6100
	dc.w	Q9_trampolin_slot88_4078-*
L00269e:
	move.l	a3,-(sp)
L0026a0:
	movea.l	$3a4(a6),a3
L0026a4:
	pea	L0026b2(pc)
L0026a8:
	move.l	$100(a3),-(sp)
L0026ac:
	movea.l	$500(a3),a3
L0026b0:
	rts
L0026b2:
	movea.l	(sp)+,a3
* Rohbytes statt Instruktion (cmpi.b #-0x43,(0x22,A4)) -- siehe FORCE_RAW_BYTES im Konverter
L0026b4:
	dc.b	$0c,$2c,$00,$bd,$00,$22
L0026ba:
	bne.b	L0026de
L0026bc:
	subq.l	#$1,$794(a6)
L0026c0:
	bhi.b	L0026de
L0026c2:
	clr.l	$794(a6)
L0026c6:
	moveq	#$1,d0
L0026c8:
	move.l	a3,-(sp)
L0026ca:
	movea.l	$3a4(a6),a3
L0026ce:
	pea	L0026dc(pc)
L0026d2:
	move.l	$178(a3),-(sp)
L0026d6:
	movea.l	$578(a3),a3
L0026da:
	rts
L0026dc:
	dc.b	$26
L0026dd:
	dc.b	$5f
L0026de:
	exg	a0,a4
L0026e0:
	movem.l	(sp)+,a0/a1/a2/a3
L0026e4:
	rts
L0026e6:
	trapf
L0026e8:
	movea.l	a0,a1
L0026ea:
	cmp.w	(a1),d0
L0026ec:
	bhi.b	L002700
L0026ee:
	asl.w	#$2,d0
L0026f0:
	beq.b	L002700
L0026f2:
	adda.w	d0,a1
L0026f4:
	lsr.w	#$2,d0
L0026f6:
	tst.l	(a1)
L0026f8:
	beq.b	L002700
L0026fa:
	movea.l	(a1),a1
L0026fc:
	cmp.w	(a1),d0
L0026fe:
	beq.b	L00270a
L002700:
	move.w	#$c9,d1
L002704:
	ori	#$1,ccr
L002708:
	rts
L00270a:
	bcs.b	L002710
L00270c:
	move.l	a1,$24(a5)
L002710:
	rts
L002712:
	trapf.l	#$0
L002718:
	tst.b	d1
L00271a:
	bne.b	L00277c
L00271c:
	andi.w	#$ff,d0
L002720:
	asl.w	#$2,d0
L002722:
	cmpi.w	#$64,d0
L002726:
	bcs.b	L00277c
L002728:
	cmpi.w	#$80,d0
L00272c:
	bcs.b	L002734
L00272e:
	cmpi.w	#$100,d0
L002732:
	bcs.b	L00277c
L002734:
	tst.l	$20(a5)
L002738:
	beq.b	L002786
L00273a:
	movea.l	$8e4(a6),a3
L00273e:
	lea	$0(a3,d0.w*1),a3
L002742:
	tst.l	(a3)
L002744:
	bne.b	L002776
L002746:
	subq.l	#$4,sp
L002748:
	dc.b	$43
L002749:
	dc.b	$fa
L00274a:
	add.w	d4,$40d7(a6)
L00274e:
	ori	#$700,sr
L002752:
	move.l	a0,(a3)
L002754:
	move.l	a2,$400(a3)
L002758:
	asr.w	#$2,d0
L00275a:
	subq.w	#$2,d0
L00275c:
	movea.l	$68(a6),a3
L002760:
	mulu.w	#$a,d0
L002764:
	move.l	a1,$6(a3,d0.w*1)
L002768:
	moveq	#$44,d0
L00276a:
	trap	#$0
L00276c:
	ori.w	#$46d7,(a2)+
L002770:
	addq.l	#$4,sp
L002772:
	moveq	#$0,d1
L002774:
	rts
L002776:
	move.w	#$d4,d1
L00277a:
	bra.b	L002780
L00277c:
	move.w	#$e1,d1
L002780:
	ori	#$1,ccr
L002784:
	rts
L002786:
	movea.l	$8e4(a6),a1
L00278a:
	lea	$400(a1),a1
L00278e:
	cmpa.l	$0(a1,d0.w*1),a2
L002792:
	bne.b	L00277c
L002794:
	lea	$0(a1,d0.w*1),a1
L002798:
	subq.l	#$4,sp
L00279a:
	lea	Q9_disp_180(pc),a2
L00279e:
	move	sr,(sp)
L0027a0:
	moveq	#$0,d1
L0027a2:
	ori	#$700,sr
L0027a6:
	move.l	d1,(a1)
L0027a8:
	move.l	d1,-$400(a1)
L0027ac:
	asr.w	#$2,d0
L0027ae:
	subq.w	#$2,d0
L0027b0:
	movea.l	$68(a6),a3
L0027b4:
	mulu.w	#$a,d0
L0027b8:
	move.l	a2,$6(a3,d0.w*1)
L0027bc:
	bra.b	L002768
L0027be:
	trapf
L0027c0:
	bsr.b	L0027d4
L0027c2:
	bcs.b	L0027d2
L0027c4:
	movem.l	d0/d1,$0(a5)
L0027ca:
	move.l	a0,$20(a5)
L0027ce:
	move.l	a2,$28(a5)
L0027d2:
	rts
L0027d4:
	suba.l	a2,a2
L0027d6:
	movem.l	a2/a1/a0/d3/d2/d1/d0,-(sp)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0027da:
	dc.w	$6100
	dc.w	L0032fa-*
L0027de:
	bcs.b	L00283a
L0027e0:
	movea.l	$3c(a6),a2
L0027e4:
	move.l	d1,d3
L0027e6:
	tst.l	$0(a2)
L0027ea:
	bne.b	L0027f8
L0027ec:
	tst.l	$18(sp)
L0027f0:
	bne.b	L00282c
L0027f2:
	move.l	a2,$18(sp)
L0027f6:
	bra.b	L00282c
L0027f8:
	movea.l	$0(a2),a1
L0027fc:
	adda.l	$c(a1),a1
L002800:
	move.l	d3,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002802:
	dc.w	$6100
	dc.w	L001ab8-*
L002806:
	bcs.b	L00282c
L002808:
	movea.l	$0(a2),a1
L00280c:
	move.b	$12(a1),d0
L002810:
	move.b	$13(a1),d2
L002814:
	tst.b	$2(sp)
L002818:
	beq.b	L002820
L00281a:
	cmp.b	$2(sp),d0
L00281e:
	bne.b	L00282c
L002820:
	tst.b	$3(sp)
L002824:
	beq.b	L002844
L002826:
	cmp.b	$3(sp),d2
L00282a:
	beq.b	L002844
L00282c:
	lea	$10(a2),a2
L002830:
	cmpa.l	$40(a6),a2
L002834:
	bcs.b	L0027e6
L002836:
	move.w	#$dd,d1
L00283a:
	move.w	d1,$6(sp)
L00283e:
	ori	#$1,ccr
L002842:
	bra.b	L002862
L002844:
	clr.w	$0(sp)
L002848:
	move.w	$12(a1),$2(sp)
L00284e:
	clr.w	$4(sp)
L002852:
	move.w	$14(a1),$6(sp)
L002858:
	adda.w	d1,a0
L00285a:
	move.l	a0,$10(sp)
L00285e:
	move.l	a2,$18(sp)
L002862:
	movem.l	(sp)+,d0/d1/d2/d3/a0/a1/a2
L002866:
	rts
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002868:
	dc.w	$6100
	dc.w	L0016b6-*
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00286c:
	dc.w	$6500
	dc.w	L0029e4-*
L002870:
	bsr.b	L0028aa
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002872:
	dc.w	$6500
	dc.w	L0029e4-*
L002876:
	bclr.b	#$7,$1c(a0)
L00287c:
	movea.l	$38(a0),a2
L002880:
	btst.b	#$5,$14(a2)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002886:
	dc.w	$6700
	dc.w	Q9_scheduler_183a-*
L00288a:
	tst.w	$8(a2)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00288e:
	dc.w	$6600
	dc.w	L00297a-*
L002892:
	movea.l	$8(a0),a1
L002896:
	move.l	a6,$30(a1)
L00289a:
	bset.b	#$5,$40(a1)
L0028a0:
	bset.b	#$7,$1c(a0)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0028a6:
	dc.w	$6000
	dc.w	Q9_scheduler_183a-*
L0028aa:
	move.l	a1,-(sp)
L0028ac:
	move.l	$14(a4),$14(a1)
L0028b2:
	move.w	$18(a4),$18(a1)
L0028b8:
	lea	$148(a4),a0
L0028bc:
	lea	$148(a1),a1
L0028c0:
	moveq	#$1f,d1
L0028c2:
	lsr.w	#$2,d1
L0028c4:
	move.l	(a0)+,(a1)+
L0028c6:
	dbf	d1,L0028c4
L0028ca:
	move.w	$e(a5),d3
L0028ce:
	cmpi.w	#$20,d3
L0028d2:
	bls.b	L00293a
L0028d4:
	move.w	#$c9,d1
L0028d8:
	bra.b	L002950
L0028da:
	move.w	(a0)+,d0
L0028dc:
	beq.b	L002908
L0028de:
	movem.l	a1/d0,-(sp)
L0028e2:
	addq.l	#$1,$3ac(a4)
L0028e6:
	move.l	a3,-(sp)
L0028e8:
	movea.l	$3a4(a6),a3
L0028ec:
	pea	L0028fa(pc)
L0028f0:
	move.l	$208(a3),-(sp)
L0028f4:
	movea.l	$608(a3),a3
L0028f8:
	rts
L0028fa:
	dc.b	$26
L0028fb:
	dc.b	$5f
L0028fc:
	dc.b	$4c
L0028fd:
	dc.b	$df
L0028fe:
	dc.b	$02
L0028ff:
	dc.b	$01
L002900:
	dc.b	$64
L002901:
	dc.b	$02
L002902:
	dc.b	$42
L002903:
	dc.b	$40
L002904:
	dc.b	$53
L002905:
	dc.b	$ac
L002906:
	dc.b	$03
L002907:
	dc.b	$ac
L002908:
	move.w	d0,(a1)+
L00290a:
	tst.l	$3ac(a4)
L00290e:
	bne.b	L00293a
L002910:
	btst.b	#$5,$1c(a4)
L002916:
	beq.b	L00293a
L002918:
	movem.l	a5/d0,-(sp)
L00291c:
	movea.l	sp,a5
L00291e:
	moveq	#$1,d0
L002920:
	move.l	a3,-(sp)
L002922:
	movea.l	$3a4(a6),a3
L002926:
	pea	L002934(pc)
L00292a:
	move.l	$28(a3),-(sp)
L00292e:
	movea.l	$428(a3),a3
L002932:
	rts
L002934:
	dc.b	$26
L002935:
	dc.b	$5f
L002936:
	dc.b	$4c
L002937:
	dc.b	$df
L002938:
	dc.b	$20
L002939:
	dc.b	$01
L00293a:
	dbf	d3,L0028da
L00293e:
	movea.l	(sp),a0
L002940:
	move.w	$0(a4),$2(a0)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002946:
	dc.w	$6100
	dc.w	L0029e6-*
L00294a:
	bcc.b	L002982
L00294c:
	clr.w	$2(a0)
L002950:
	movea.l	(sp),a0
L002952:
	move.l	d1,(sp)
L002954:
	move.w	$2(a0),d0
L002958:
	beq.b	L002964
L00295a:
	move.w	$4(a0),$6(a4)
L002960:
	clr.w	$2(a0)
L002964:
	moveq	#$0,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002966:
	dc.w	$6100
	dc.w	Q9_proc_slot_cleanup_25f8-*
L00296a:
	move.w	$0(a0),d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00296e:
	dc.w	$6100
	dc.w	Q9_proc_id_free_wrap_1e18-*
L002972:
	move.l	(sp)+,d1
L002974:
	ori	#$1,ccr
L002978:
	rts
L00297a:
	move.w	#$a4,d1
L00297e:
	move.l	a0,-(sp)
L002980:
	bra.b	L002950
L002982:
	move.l	$8(a5),d2
L002986:
	movea.l	$c(a0),a1
L00298a:
	movea.l	$24(a5),a0
L00298e:
	movea.l	a1,a2
L002990:
	move.l	a3,-(sp)
L002992:
	movea.l	$3a4(a6),a3
L002996:
	pea	L0029a4(pc)
L00299a:
	move.l	$e0(a3),-(sp)
L00299e:
	movea.l	$4e0(a3),a3
L0029a2:
	rts
L0029a4:
	movea.l	(sp)+,a3
L0029a6:
	movea.l	(sp),a0
L0029a8:
	moveq	#$0,d0
L0029aa:
	move.w	$0(a0),d0
L0029ae:
	move.l	d0,$0(a5)
L0029b2:
	move.w	$6(a4),$4(a0)
L0029b8:
	move.w	d0,$6(a4)
L0029bc:
	moveq	#$3,d0
L0029be:
	move.l	a3,-(sp)
L0029c0:
	movea.l	$3a4(a6),a3
L0029c4:
	pea	L0029d2(pc)
L0029c8:
	move.l	$164(a3),-(sp)
L0029cc:
	movea.l	$564(a3),a3
L0029d0:
	rts
L0029d2:
	movea.l	(sp)+,a3
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0029d4:
	dc.w	$6500
	dc.w	L002950-*
L0029d8:
	addq.l	#$1,$794(a6)
L0029dc:
	move.b	#-$43,$22(a0)
L0029e2:
	addq.l	#$4,sp
L0029e4:
	rts
L0029e6:
	movem.l	a5/a4/a3/a2/a1/a0/d3/d2/d1/d0,-(sp)
L0029ea:
	movea.l	a0,a3
L0029ec:
	clr.b	$1c(a0)
L0029f0:
	clr.b	$370(a0)
L0029f4:
	move.w	$2(a5),d0
L0029f8:
	movea.l	$20(a5),a0
L0029fc:
	move.l	$c(a4),$c(a3)
L002a02:
	exg	a3,a4
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002a04:
	dc.w	$6100
	dc.w	L002fbe-*
L002a08:
	exg	a4,a3
L002a0a:
	bcc.b	L002a58
L002a0c:
	cmpi.w	#$d1,d1
L002a10:
	beq.b	L002a72
L002a12:
	moveq	#$0,d0
L002a14:
	move.l	$c(a4),$c(a3)
L002a1a:
	move.l	a3,$4c(a6)
L002a1e:
	movea.l	$20(a5),a0
L002a22:
	trap	#$0
* Rohbytes statt Instruktion (ori.b #-0x19,D1b) -- siehe FORCE_RAW_BYTES im Konverter
L002a24:
	dc.b	$00,$01,$48,$e7
L002a28:
	or.w	d0,d0
L002a2a:
	scs	d3
L002a2c:
	tst.b	$2f(a6)
L002a30:
	beq.b	L002a46
L002a32:
	move.l	$58(a6),d0
L002a36:
	beq.b	L002a42
L002a38:
	cmp.l	a4,d0
L002a3a:
	beq.b	L002a46
L002a3c:
	movea.l	d0,a1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002a3e:
	dc.w	$6100
	dc.w	Q9_fpu_save_fe0-*
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002a42:
	dc.w	$6100
	dc.w	Q9_fpu_restore_1034-*
L002a46:
	ror.l	#$1,d3
L002a48:
	movem.l	(sp)+,d0/a1
L002a4c:
	bcc.b	L002a54
L002a4e:
	move.l	a4,$4c(a6)
L002a52:
	bra.b	L002a72
L002a54:
	move.l	a4,$4c(a6)
L002a58:
	move.l	a1,d3
L002a5a:
	move.l	a0,$20(a5)
L002a5e:
	move.l	a2,$38(a3)
L002a62:
	cmpi.w	#$101,d0
L002a66:
	beq.b	L002a80
L002a68:
	cmpi.w	#$101,d0
L002a6c:
	beq.b	L002a80
L002a6e:
	move.w	#$ea,d1
L002a72:
	move.w	d1,$6(sp)
L002a76:
	movem.l	(sp)+,d0/d1/d2/d3/a0/a1/a2/a3/a4/a5
L002a7a:
	ori	#$1,ccr
L002a7e:
	rts
L002a80:
	move.l	$8(a2),$3a0(a3)
L002a86:
	moveq	#$4,d2
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002a88:
	dc.w	$6100
	dc.w	L003072-*
L002a8c:
	bcs.b	L002a72
L002a8e:
	move.l	$38(a2),d0
L002a92:
	add.l	$3c(a2),d0
L002a96:
	add.l	$8(a5),d0
L002a9a:
	add.l	$4(a5),d0
L002a9e:
	bne.b	L002aa2
L002aa0:
	moveq	#$1,d0
L002aa2:
	move.l	a3,$4c(a6)
L002aa6:
	trap	#$0
* Rohbytes statt Instruktion (ori.b #0x6,D7b) -- siehe FORCE_RAW_BYTES im Konverter
L002aa8:
	dc.b	$00,$07,$64,$06
L002aac:
	move.l	a4,$4c(a6)
L002ab0:
	bra.b	L002a72
L002ab2:
	move.l	a4,$4c(a6)
L002ab6:
	movea.l	$32c(a3),a2
L002aba:
	move.l	a1,d0
L002abc:
	move.l	a1,d1
L002abe:
	sub.l	$8(a5),d1
L002ac2:
	andi.b	#-$4,d1
L002ac6:
	move.l	d1,$c(a3)
L002aca:
	moveq	#$0,d2
L002acc:
	move.w	$3b0(a3),d2
L002ad0:
	movea.l	$38(a3),a1
L002ad4:
	btst.b	#$5,$14(a1)
L002ada:
	lea	-$48(a3,d2.l*1),a1
L002ade:
	beq.b	L002ae6
L002ae0:
	movea.l	d1,a1
L002ae2:
	lea	-$48(a1),a1
L002ae6:
	move.l	a1,$8(a3)
L002aea:
	clr.w	$46(a1)
L002aee:
	move.w	#$1000,$40(a1)
L002af4:
	move.l	d3,$42(a1)
L002af8:
	move.l	a2,$38(a1)
L002afc:
	addi.l	#$8000,$38(a1)
L002b04:
	move.l	d1,$34(a1)
L002b08:
	move.l	d1,$3c(a1)
L002b0c:
	move.l	$38(a3),$2c(a1)
L002b12:
	move.l	d0,$24(a1)
L002b16:
	move.l	$330(a3),$18(a1)
L002b1c:
	move.l	$8(a5),$14(a1)
L002b22:
	move.l	$c(a5),$c(a1)
L002b28:
	tst.w	$12(a5)
L002b2c:
	beq.b	L002b34
L002b2e:
	move.w	$12(a5),$18(a3)
L002b34:
	clr.w	$8(a1)
L002b38:
	move.w	$18(a3),$a(a1)
L002b3e:
	move.l	$14(a3),$4(a1)
L002b44:
	clr.w	$0(a1)
L002b48:
	move.w	$0(a3),$2(a1)
L002b4e:
	move.l	$79c(a6),d0
L002b52:
	beq.b	L002b60
L002b54:
	lea	$400(a3),a1
L002b58:
	move.l	a1,$334(a3)
L002b5c:
	move.l	d0,$0(a1)
L002b60:
	movea.l	$38(a3),a1
L002b64:
	bsr.b	L002b6c
L002b66:
	movem.l	(sp)+,d0/d1/d2/d3/a0/a1/a2/a3/a4/a5
L002b6a:
	rts
L002b6c:
	movem.l	a2/a1/a0/d3,-(sp)
L002b70:
	movea.l	a2,a0
L002b72:
	move.l	a2,d2
L002b74:
	add.l	$38(a1),d2
L002b78:
	adda.l	$40(a1),a1
L002b7c:
	move.l	(a1)+,d0
L002b7e:
	lsr.l	#$2,d0
L002b80:
	scs	d3
L002b82:
	moveq	#$0,d1
L002b84:
	bra.b	L002b88
L002b86:
	move.l	d1,(a2)+
L002b88:
	dbf	d0,L002b86
L002b8c:
	addq.w	#$1,d0
L002b8e:
	subq.l	#$1,d0
L002b90:
	bcc.b	L002b86
L002b92:
	tst.b	d3
L002b94:
	beq.b	L002b98
L002b96:
	move.w	d1,(a2)+
L002b98:
	move.l	(a1)+,d0
L002b9a:
	lsr.l	#$2,d0
L002b9c:
	bcc.b	L002ba4
L002b9e:
	move.w	(a1)+,(a2)+
L002ba0:
	bra.b	L002ba4
L002ba2:
	move.l	(a1)+,(a2)+
L002ba4:
	dbf	d0,L002ba2
L002ba8:
	addq.w	#$1,d0
L002baa:
	subq.l	#$1,d0
L002bac:
	bcc.b	L002ba2
L002bae:
	sub.l	a2,d2
L002bb0:
	bls.b	L002bda
L002bb2:
	move.l	a2,d3
L002bb4:
	btst.l	#$1,d3
L002bb8:
	beq.b	L002bc0
L002bba:
	move.w	d1,(a2)+
L002bbc:
	subq.l	#$2,d2
L002bbe:
	bls.b	L002bda
L002bc0:
	lsr.l	#$2,d2
L002bc2:
	scs	d3
L002bc4:
	moveq	#$0,d1
L002bc6:
	bra.b	L002bca
L002bc8:
	move.l	d1,(a2)+
L002bca:
	dbf	d2,L002bc8
L002bce:
	addq.w	#$1,d2
L002bd0:
	subq.l	#$1,d2
L002bd2:
	bcc.b	L002bc8
L002bd4:
	tst.b	d3
L002bd6:
	beq.b	L002bda
L002bd8:
	move.w	d1,(a2)+
L002bda:
	move.l	$8(sp),d1
L002bde:
	move.l	(a1)+,d2
L002be0:
	beq.b	L002be4
L002be2:
	bsr.b	L002c32
L002be4:
	move.l	a0,d1
L002be6:
	move.l	(a1)+,d2
L002be8:
	beq.b	L002bec
L002bea:
	bsr.b	L002c32
L002bec:
	move	sr,d1
L002bee:
	ori	#$700,sr
L002bf2:
	movec	cacr,d0
L002bf6:
	ori.w	#$808,d0
L002bfa:
	movec	d0,cacr
L002bfe:
	move	d1,sr
L002c00:
	move.b	$3e0(a6),d0
L002c04:
	btst.l	#$0,d0
L002c08:
	beq.b	L002c10
L002c0a:
	btst.l	#$1,d0
L002c0e:
	bne.b	L002c28
L002c10:
	moveq	#$44,d0
L002c12:
	move.l	a3,-(sp)
L002c14:
	movea.l	$3a4(a6),a3
L002c18:
	pea	L002c26(pc)
L002c1c:
	move.l	$168(a3),-(sp)
L002c20:
	movea.l	$568(a3),a3
L002c24:
	rts
L002c26:
	dc.b	$26
L002c27:
	dc.b	$5f
L002c28:
	andi	#-$2,ccr
L002c2c:
	movem.l	(sp)+,d3/a0/a1/a2
L002c30:
	rts
L002c32:
	move.w	d2,d0
L002c34:
	bra.b	L002c3c
L002c36:
	move.w	(a1)+,d2
L002c38:
	add.l	d1,$0(a0,d2.l*1)
L002c3c:
	dbf	d0,L002c36
L002c40:
	move.l	(a1)+,d2
L002c42:
	bne.b	L002c32
L002c44:
	rts
L002c46:
	trapf
L002c48:
	move.l	$40(a6),d2
L002c4c:
	sub.l	$3c(a6),d2
L002c50:
	cmp.l	d1,d2
L002c52:
	bcc.b	L002c5a
L002c54:
	move.l	d2,$4(a5)
L002c58:
	move.l	d2,d1
L002c5a:
	move.l	d1,d2
L002c5c:
	movea.l	a0,a2
L002c5e:
	movea.l	$3c(a6),a0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002c62:
	dc.w	$6000
	dc.w	L001b4c-*
L002c66:
	trapf
L002c68:
	movea.l	a0,a2
L002c6a:
	movea.l	$44(a6),a0
L002c6e:
	moveq	#$1,d2
L002c70:
	add.w	(a0),d2
L002c72:
	asl.l	#$2,d2
L002c74:
	cmp.l	d2,d1
L002c76:
	bls.b	L002c7e
L002c78:
	move.l	d2,$4(a5)
L002c7c:
	bra.b	L002c80
L002c7e:
	move.l	d1,d2
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002c80:
	dc.w	$6000
	dc.w	L001b4c-*
L002c84:
	trapf.w	#$0
L002c88:
	move.w	#$e0,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002c8c:
	dc.w	$6100
	dc.w	Q9_proc_id_lookup_2cee-*
L002c90:
	bcs.b	L002ce2
L002c92:
	move.w	$18(a1),d0
L002c96:
	move.b	$20(a1),d1
L002c9a:
	cmpi.b	#$61,d1
L002c9e:
	bne.b	L002cc4
L002ca0:
	move.l	$2e0(a1),d0
L002ca4:
	bmi.b	L002cc4
L002ca6:
	beq.b	L002cc4
L002ca8:
	sub.l	$3c4(a6),d0
L002cac:
	cmpi.l	#$10000,d0
L002cb2:
	bcs.b	L002cb6
L002cb4:
	moveq	#-$1,d0
L002cb6:
	move.w	$8a8(a6),d1
L002cba:
	beq.b	L002cc4
L002cbc:
	cmp.w	d1,d0
L002cbe:
	bcs.b	L002cc4
L002cc0:
	move.w	d1,d0
L002cc2:
	subq.w	#$1,d0
L002cc4:
	move.w	d0,$1a(a1)
L002cc8:
	moveq	#$0,d2
L002cca:
	move.w	$6(a5),d2
L002cce:
	cmp.w	$3b0(a1),d2
L002cd2:
	bls.b	L002cd8
L002cd4:
	move.w	$3b0(a1),d2
L002cd8:
	movea.l	a1,a0
L002cda:
	movea.l	$20(a5),a2
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002cde:
	dc.w	$6000
	dc.w	L001b4c-*
L002ce2:
	rts
L002ce4:
	dc.b	$51
L002ce5:
	dc.b	$fa
L002ce6:
	dc.b	$00
L002ce7:
	dc.b	$00
L002ce8:
	dc.b	$61
L002ce9:
	dc.b	$04
L002cea:
	dc.b	$60
L002ceb:
	dc.b	$00
L002cec:
	dc.b	$fa
L002ced:
	dc.b	$1e
* Prozess-ID-Lookup/-Validierung (Index+Generation-Schema gegen versehentliche Wiederverwendung).
Q9_proc_id_lookup_2cee:
	movea.l	$44(a6),a0
L002cf2:
	cmp.w	(a0),d0
L002cf4:
	bhi.b	L002d0a
L002cf6:
	asl.w	#$2,d0
L002cf8:
	beq.b	L002d0a
L002cfa:
	adda.w	d0,a0
L002cfc:
	lsr.w	#$2,d0
L002cfe:
	beq.b	L002d0a
L002d00:
	movea.l	(a0),a1
L002d02:
	tst.l	(a0)
L002d04:
	beq.b	L002d0a
L002d06:
	cmp.w	(a1),d0
L002d08:
	beq.b	L002d12
L002d0a:
	move.w	#$e0,d1
L002d0e:
	ori	#$1,ccr
L002d12:
	rts
L002d14:
	trapf.w	#$0
L002d18:
	movem.l	d4/d3/d2,-(sp)
L002d1c:
	divu.w	#$e10,d0
L002d20:
	swap	d0
L002d22:
	moveq	#$0,d3
L002d24:
	move.w	d0,d3
L002d26:
	divu.w	#$3c,d3
L002d2a:
	move.b	d3,d0
L002d2c:
	lsl.w	#$8,d0
L002d2e:
	swap	d3
L002d30:
	move.b	d3,d0
L002d32:
	move.l	d1,d4
L002d34:
	addi.l	#$5f5,d4
L002d3a:
	cmpi.l	#$231518,d1
L002d40:
	ble.b	L002d66
L002d42:
	asl.l	#$2,d1
L002d44:
	subi.l	#$71f73d,d1
L002d4a:
	divu.w	#-$41c5,d1
L002d4e:
	andi.l	#$ffff,d1
L002d54:
	divu.w	#$3,d1
L002d58:
	andi.l	#$ffff,d1
L002d5e:
	addq.l	#$1,d4
L002d60:
	add.l	d1,d4
L002d62:
	lsr.l	#$2,d1
L002d64:
	sub.l	d1,d4
L002d66:
	move.l	d4,d1
L002d68:
	lsl.l	#$4,d1
L002d6a:
	subi.l	#$7a2,d1
L002d70:
	divu.w	#$16d4,d1
L002d74:
	moveq	#$0,d3
L002d76:
	move.w	d1,d3
L002d78:
	mulu.w	#$16d,d1
L002d7c:
	sub.l	d1,d4
L002d7e:
	move.l	d3,d1
L002d80:
	lsr.l	#$2,d1
L002d82:
	sub.l	d1,d4
L002d84:
	move.l	#$7ce,d1
L002d8a:
	mulu.w	d4,d1
L002d8c:
	divu.w	#-$112d,d1
L002d90:
	move.w	d1,d2
L002d92:
	mulu.w	#-$112d,d1
L002d96:
	divu.w	#$7ce,d1
L002d9a:
	sub.w	d1,d4
L002d9c:
	cmpi.w	#$d,d2
L002da0:
	bls.b	L002da6
L002da2:
	subi.w	#$c,d2
L002da6:
	subq.w	#$1,d2
L002da8:
	cmpi.w	#$2,d2
L002dac:
	ble.b	L002db0
L002dae:
	subq.l	#$1,d3
L002db0:
	subi.l	#$126b,d3
L002db6:
	move.w	d3,d1
L002db8:
	swap	d1
L002dba:
	move.b	d2,d1
L002dbc:
	lsl.w	#$8,d1
L002dbe:
	move.b	d4,d1
L002dc0:
	movem.l	(sp)+,d2/d3/d4
L002dc4:
	movem.l	d0/d1,$0(a5)
L002dca:
	rts
L002dcc:
	trapf.w	#$0
L002dd0:
	move.l	a0,$28(a4)
L002dd4:
	move.l	$38(a5),$2c(a4)
L002dda:
	rts
L002ddc:
	trapf.w	#$0
L002de0:
	clr.w	$0(a5)
L002de4:
	move.w	$0(a4),$2(a5)
L002dea:
	move.l	$14(a4),$4(a5)
L002df0:
	clr.w	$8(a5)
L002df4:
	move.w	$18(a4),$a(a5)
L002dfa:
	rts
L002dfc:
	dc.b	$51
L002dfd:
	dc.b	$fa
L002dfe:
	dc.b	$00
L002dff:
	dc.b	$00
L002e00:
	dc.b	$02
L002e01:
	dc.b	$40
L002e02:
	dc.b	$00
L002e03:
	dc.b	$ff
L002e04:
	dc.b	$e5
L002e05:
	dc.b	$40
L002e06:
	dc.b	$0c
L002e07:
	dc.b	$40
L002e08:
	dc.b	$00
L002e09:
	dc.b	$64
L002e0a:
	dc.b	$65
L002e0b:
	dc.b	$3e
L002e0c:
	dc.b	$41
L002e0d:
	dc.b	$f6
L002e0e:
	dc.b	$00
L002e0f:
	dc.b	$00
L002e10:
	dc.b	$d0
L002e11:
	dc.b	$fc
L002e12:
	dc.b	$03
L002e13:
	dc.b	$84
L002e14:
	dc.b	$0c
L002e15:
	dc.b	$40
L002e16:
	dc.b	$00
L002e17:
	dc.b	$80
L002e18:
	dc.b	$65
L002e19:
	dc.b	$0e
L002e1a:
	dc.b	$0c
L002e1b:
	dc.b	$40
L002e1c:
	dc.b	$00
L002e1d:
	dc.b	$e4
L002e1e:
	dc.b	$65
L002e1f:
	dc.b	$2a
L002e20:
	dc.b	$41
L002e21:
	dc.b	$f6
L002e22:
	dc.b	$00
L002e23:
	dc.b	$00
L002e24:
	dc.b	$d0
L002e25:
	dc.b	$fc
L002e26:
	dc.b	$ff
L002e27:
	dc.b	$a4
L002e28:
	dc.b	$4a
L002e29:
	dc.b	$ad
L002e2a:
	dc.b	$00
L002e2b:
	dc.b	$20
L002e2c:
	dc.b	$67
L002e2d:
	dc.b	$00
L002e2e:
	dc.b	$00
L002e2f:
	dc.b	$84
L002e30:
	dc.b	$4a
L002e31:
	dc.b	$a8
L002e32:
	dc.b	$00
L002e33:
	dc.b	$00
L002e34:
	dc.b	$67
L002e35:
	dc.b	$1e
L002e36:
	dc.b	$4a
L002e37:
	dc.b	$01
L002e38:
	dc.b	$67
L002e39:
	dc.b	$0a
L002e3a:
	dc.b	$22
L002e3b:
	dc.b	$68
L002e3c:
	dc.b	$00
L002e3d:
	dc.b	$00
L002e3e:
	dc.b	$4a
L002e3f:
	dc.b	$29
L002e40:
	dc.b	$00
L002e41:
	dc.b	$10
L002e42:
	dc.b	$66
L002e43:
	dc.b	$10
L002e44:
	dc.b	$32
L002e45:
	dc.b	$3c
L002e46:
	dc.b	$00
L002e47:
	dc.b	$d4
L002e48:
	dc.b	$60
L002e49:
	dc.b	$04
L002e4a:
	move.w	#$e1,d1
L002e4e:
	ori	#$1,ccr
L002e52:
	rts
L002e54:
	tst.l	$3e4(a6)
L002e58:
	beq.b	L002eac
L002e5a:
	subq.l	#$4,sp
L002e5c:
	move	sr,$0(sp)
L002e60:
	ori	#$700,sr
L002e64:
	movea.l	$3e4(a6),a1
L002e68:
	move.l	$0(a1),$3e4(a6)
L002e6e:
	move	$0(sp),sr
L002e72:
	move.b	d1,$10(a1)
L002e76:
	move.l	$20(a5),$4(a1)
L002e7c:
	move.l	a2,$8(a1)
L002e80:
	move.l	$2c(a5),$c(a1)
L002e86:
	ori	#$700,sr
L002e8a:
	movea.l	a0,a3
L002e8c:
	move.l	$0(a0),d0
L002e90:
	beq.b	L002e9a
L002e92:
	movea.l	d0,a0
L002e94:
	cmp.b	$10(a0),d1
L002e98:
	bcc.b	L002e8a
L002e9a:
	move.l	$0(a3),$0(a1)
L002ea0:
	move.l	a1,$0(a3)
L002ea4:
	move	$0(sp),sr
L002ea8:
	addq.l	#$4,sp
L002eaa:
	rts
L002eac:
	move.w	#$ca,d1
L002eb0:
	bra.b	L002e4e
L002eb2:
	dc.b	$26
L002eb3:
	dc.b	$48
L002eb4:
	dc.b	$20
L002eb5:
	dc.b	$28
L002eb6:
	dc.b	$00
L002eb7:
	dc.b	$00
L002eb8:
	dc.b	$67
L002eb9:
	dc.b	$90
L002eba:
	dc.b	$20
L002ebb:
	dc.b	$40
L002ebc:
	dc.b	$b5
L002ebd:
	dc.b	$e8
L002ebe:
	dc.b	$00
L002ebf:
	dc.b	$08
L002ec0:
	dc.b	$66
L002ec1:
	dc.b	$f0
L002ec2:
	dc.b	$27
L002ec3:
	dc.b	$68
L002ec4:
	dc.b	$00
L002ec5:
	dc.b	$00
L002ec6:
	dc.b	$00
L002ec7:
	dc.b	$00
L002ec8:
	dc.b	$59
L002ec9:
	dc.b	$8f
L002eca:
	dc.b	$40
L002ecb:
	dc.b	$ef
L002ecc:
	dc.b	$00
L002ecd:
	dc.b	$00
L002ece:
	dc.b	$00
L002ecf:
	dc.b	$7c
L002ed0:
	dc.b	$07
L002ed1:
	dc.b	$00
L002ed2:
	dc.b	$21
L002ed3:
	dc.b	$6e
L002ed4:
	dc.b	$03
L002ed5:
	dc.b	$e4
L002ed6:
	dc.b	$00
L002ed7:
	dc.b	$00
L002ed8:
	dc.b	$2d
L002ed9:
	dc.b	$48
L002eda:
	dc.b	$03
L002edb:
	dc.b	$e4
L002edc:
	dc.b	$60
L002edd:
	dc.b	$c6
L002ede:
	dc.b	$51
L002edf:
	dc.b	$fc
L002ee0:
	dc.b	$61
L002ee1:
	dc.b	$08
L002ee2:
	dc.b	$48
L002ee3:
	dc.b	$ed
L002ee4:
	dc.b	$00
L002ee5:
	dc.b	$03
L002ee6:
	dc.b	$00
L002ee7:
	dc.b	$00
L002ee8:
	dc.b	$4e
L002ee9:
	dc.b	$75
L002eea:
	movem.l	d4/d3/d2/d1/d0,-(sp)
L002eee:
	moveq	#$0,d2
L002ef0:
	moveq	#$0,d3
L002ef2:
	move.b	d1,d3
L002ef4:
	asr.l	#$8,d1
L002ef6:
	move.b	d1,d2
L002ef8:
	asr.l	#$8,d1
L002efa:
	moveq	#$0,d0
L002efc:
	cmpi.b	#$3,d2
L002f00:
	bcc.b	L002f08
L002f02:
	subq.w	#$1,d1
L002f04:
	addi.b	#$c,d2
L002f08:
	cmpi.w	#$62e,d1
L002f0c:
	bgt.b	L002f1e
L002f0e:
	blt.b	L002f2e
L002f10:
	cmpi.b	#$a,d2
L002f14:
	bhi.b	L002f1e
L002f16:
	bcs.b	L002f2e
L002f18:
	cmpi.b	#$f,d3
L002f1c:
	bls.b	L002f2e
L002f1e:
	move.l	d1,d4
L002f20:
	divs.w	#$64,d4
L002f24:
	ext.l	d4
L002f26:
	moveq	#$2,d0
L002f28:
	sub.l	d4,d0
L002f2a:
	asr.l	#$2,d4
L002f2c:
	add.l	d4,d0
L002f2e:
	muls.w	#$5b5,d1
L002f32:
	asr.l	#$2,d1
L002f34:
	add.l	d0,d1
L002f36:
	addq.w	#$1,d2
L002f38:
	mulu.w	#$7fa9,d2
L002f3c:
	divu.w	#$42c,d2
L002f40:
	ext.l	d2
L002f42:
	add.l	d2,d1
L002f44:
	add.l	d3,d1
L002f46:
	addi.l	#$1a42a2,d1
L002f4c:
	move.l	d1,$4(sp)
L002f50:
	move.w	(sp),d0
L002f52:
	mulu.w	#$3c,d0
L002f56:
	moveq	#$0,d1
L002f58:
	move.b	$2(sp),d1
L002f5c:
	add.w	d1,d0
L002f5e:
	mulu.w	#$3c,d0
L002f62:
	move.b	$3(sp),d1
L002f66:
	add.l	d1,d0
L002f68:
	move.l	d0,(sp)
L002f6a:
	movem.l	(sp)+,d0/d1/d2/d3/d4
L002f6e:
	rts
L002f70:
	bsr.b	L002fa4
L002f72:
	bcs.b	L002f80
L002f74:
	movem.l	d0/d1,$0(a5)
L002f7a:
	movem.l	a0/a1/a2,$20(a5)
L002f80:
	rts
L002f82:
	movem.l	a4/a2/a1/a0,-(sp)
L002f86:
	clr.l	-(sp)
L002f88:
	movea.l	d1,a0
L002f8a:
	movea.l	$50(a6),a4
L002f8e:
	bsr.b	L002fa4
L002f90:
	bcc.b	L002f98
L002f92:
	move.w	d1,$2(sp)
L002f96:
	bra.b	L002f9e
L002f98:
	movea.l	$18(sp),a0
L002f9c:
	move.l	a2,(a0)
L002f9e:
	movem.l	(sp)+,d0/a0/a1/a2/a4
L002fa2:
	rts
L002fa4:
	addq.l	#$1,$3ac(a4)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002fa8:
	dc.w	$6100
	dc.w	L0027d4-*
L002fac:
	bcs.b	L002fca
L002fae:
	move.l	a4,-(sp)
L002fb0:
	movea.l	$50(a6),a4
L002fb4:
	bsr.b	L002fdc
L002fb6:
	movea.l	(sp)+,a4
L002fb8:
	bra.b	L002fca
L002fba:
	dc.b	$48
L002fbb:
	dc.b	$7a
L002fbc:
	dc.b	$ff
L002fbd:
	dc.b	$b6
L002fbe:
	addq.l	#$1,$3ac(a4)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L002fc2:
	dc.w	$6100
	dc.w	L0027d4-*
L002fc6:
	bcs.b	L002fca
L002fc8:
	bsr.b	L002fdc
L002fca:
	subq.l	#$4,sp
L002fcc:
	move	sr,$0(sp)
L002fd0:
	subq.l	#$1,$3ac(a4)
L002fd4:
	move	$0(sp),ccr
L002fd8:
	addq.l	#$4,sp
L002fda:
	rts
L002fdc:
	movem.l	d3/d2/d1/d0,-(sp)
L002fe0:
	btst.l	#$f,d1
L002fe4:
	bne.b	L002fec
L002fe6:
	tst.w	$c(a2)
L002fea:
	bne.b	L003064
L002fec:
	movea.l	$0(a2),a1
L002ff0:
	moveq	#$2e,d1
L002ff2:
	lsr.w	#$1,d1
L002ff4:
	moveq	#-$1,d2
L002ff6:
	moveq	#$0,d3
L002ff8:
	move.w	(a1)+,d0
L002ffa:
	add.w	d0,d3
L002ffc:
	ror.w	d3,d3
L002ffe:
	eor.w	d0,d2
L003000:
	dbf	d1,L002ff8
L003004:
	bne.b	L00305e
L003006:
	cmp.w	$e(a2),d3
L00300a:
	bne.b	L00305e
L00300c:
	movea.l	a2,a1
L00300e:
	movea.l	$0(a2),a2
L003012:
	moveq	#$5,d2
L003014:
	bsr.b	L003072
L003016:
	bcs.b	L003068
L003018:
	btst.l	#$1,d1
L00301c:
	beq.b	L003024
L00301e:
	move.b	#$7,d1
L003022:
	bra.b	L003028
L003024:
	move.b	#$5,d1
L003028:
	move.l	$4(a2),d0
L00302c:
	move.l	a3,-(sp)
L00302e:
	movea.l	$3a4(a6),a3
L003032:
	pea	L003040(pc)
L003036:
	move.l	$e8(a3),-(sp)
L00303a:
	movea.l	$4e8(a3),a3
L00303e:
	rts
L003040:
	dc.b	$26,$5f,$65,$24,$52,$69,$00
L003047:
	dc.b	$0c
L003048:
	dc.b	$64
L003049:
	dc.b	$04
L00304a:
	dc.b	$53
L00304b:
	dc.b	$69
L00304c:
	dc.b	$00
L00304d:
	dc.b	$0c
L00304e:
	dc.b	$22
L00304f:
	dc.b	$4a
L003050:
	dc.b	$d3
L003051:
	dc.b	$ea
L003052:
	dc.b	$00
L003053:
	dc.b	$30
L003054:
	dc.b	$02
L003055:
	dc.b	$3c
L003056:
	dc.b	$ff
L003057:
	dc.b	$fe
L003058:
	movem.l	(sp)+,d0/d1/d2/d3
L00305c:
	rts
L00305e:
	move.w	#$ec,d1
L003062:
	bra.b	L003068
L003064:
	move.w	#$d1,d1
L003068:
	move.w	d1,$6(sp)
L00306c:
	ori	#$1,ccr
L003070:
	bra.b	L003058
L003072:
	movem.l	d2/d0,-(sp)
L003076:
	move.l	$14(a4),d0
L00307a:
	move.w	$10(a2),d1
L00307e:
	cmp.l	$8(a2),d0
L003082:
	beq.b	L003094
L003084:
	swap	d0
L003086:
	tst.w	d0
L003088:
	beq.b	L003094
L00308a:
	lsr.w	#$4,d1
L00308c:
	cmp.w	$8(a2),d0
L003090:
	beq.b	L003094
L003092:
	lsr.w	#$4,d1
L003094:
	and.b	d1,d2
L003096:
	movem.l	(sp)+,d0/d2
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00309a:
	dc.w	$6700
	dc.w	L003d88-*
L00309e:
	rts
L0030a0:
	movem.l	a2/a0/d2/d0,-(sp)
L0030a4:
	cmpa.l	a0,a2
L0030a6:
	bhi.b	L0030f0
L0030a8:
	move.w	a0,d0
L0030aa:
	btst.l	#$0,d0
L0030ae:
	beq.b	L0030b6
L0030b0:
	subq.l	#$1,d2
L0030b2:
	bcs.b	L0030e8
L0030b4:
	move.b	(a0)+,(a2)+
L0030b6:
	move.w	a2,d0
L0030b8:
	btst.l	#$0,d0
L0030bc:
	bne.b	L0030de
L0030be:
	lsr.l	#$1,d2
L0030c0:
	scs	d0
L0030c2:
	lsr.l	#$1,d2
L0030c4:
	bcc.b	L0030cc
L0030c6:
	move.w	(a0)+,(a2)+
L0030c8:
	bra.b	L0030cc
L0030ca:
	move.l	(a0)+,(a2)+
L0030cc:
	dbf	d2,L0030ca
L0030d0:
	addq.w	#$1,d2
L0030d2:
	subq.l	#$1,d2
L0030d4:
	bcc.b	L0030ca
L0030d6:
	tst.b	d0
L0030d8:
	beq.b	L0030e8
L0030da:
	moveq	#$0,d2
L0030dc:
	move.b	(a0)+,(a2)+
L0030de:
	dbf	d2,L0030dc
L0030e2:
	addq.w	#$1,d2
L0030e4:
	subq.l	#$1,d2
L0030e6:
	bcc.b	L0030dc
L0030e8:
	moveq	#$0,d2
L0030ea:
	movem.l	(sp)+,d0/d2/a0/a2
L0030ee:
	rts
L0030f0:
	adda.l	d2,a0
L0030f2:
	adda.l	d2,a2
L0030f4:
	move.w	a0,d0
L0030f6:
	btst.l	#$0,d0
L0030fa:
	beq.b	L003102
L0030fc:
	subq.l	#$1,d2
L0030fe:
	bcs.b	L003134
L003100:
	move.b	-(a0),-(a2)
L003102:
	move.w	a2,d0
L003104:
	btst.l	#$0,d0
L003108:
	bne.b	L00312a
L00310a:
	lsr.l	#$1,d2
L00310c:
	scs	d0
L00310e:
	lsr.l	#$1,d2
L003110:
	bcc.b	L003118
L003112:
	move.w	-(a0),-(a2)
L003114:
	bra.b	L003118
L003116:
	move.l	-(a0),-(a2)
L003118:
	dbf	d2,L003116
L00311c:
	addq.w	#$1,d2
L00311e:
	subq.l	#$1,d2
L003120:
	bcc.b	L003116
L003122:
	tst.b	d0
L003124:
	beq.b	L003134
L003126:
	moveq	#$0,d2
L003128:
	move.b	-(a0),-(a2)
L00312a:
	dbf	d2,L003128
L00312e:
	addq.w	#$1,d2
L003130:
	subq.l	#$1,d2
L003132:
	bcc.b	L003128
L003134:
	moveq	#$0,d2
L003136:
	movem.l	(sp)+,d0/d2/a0/a2
L00313a:
	rts
L00313c:
	dc.b	$51
L00313d:
	dc.b	$fa
L00313e:
	dc.b	$00
L00313f:
	dc.b	$00
* Cache-Flush-Schleife (patcht/invalidiert selbstmodifizierten Code) + Trampolin-Sprung in Tabellen-Slot 90 (Kontextwechsel-Einstieg).
Q9_reschedule_trampolin_3140:
	addq.l	#$1,$3ac(a4)
L003144:
	move.l	$2ac(a4),d0
L003148:
	beq.b	L0031c0
L00314a:
	move.l	$2e8(a4),d1
L00314e:
	bpl.b	L0031c0
L003150:
	movea.l	d0,a0
L003152:
	movea.l	$8(a0),a0
L003156:
	movea.l	$20(a0),a0
L00315a:
	lea	$2ec(a4),a1
L00315e:
	bra.b	L003194
L003160:
	movea.l	(a0)+,a2
L003162:
	move.w	(a1)+,d0
L003164:
	cmpi.w	#$4afc,(a2)
L003168:
	bne.b	L003194
L00316a:
	move.w	d0,(a2)
L00316c:
	move	sr,d0
L00316e:
	move.l	d0,-(sp)
L003170:
	ori	#$700,sr
L003174:
	movec	caar,d0
L003178:
	move.l	d0,-(sp)
L00317a:
	movec	a2,caar
L00317e:
	movec	cacr,d0
L003182:
	bset.l	#$2,d0
L003186:
	movec	d0,cacr
L00318a:
	move.l	(sp)+,d0
L00318c:
	movec	d0,caar
L003190:
	move.l	(sp)+,d0
L003192:
	move	d0,sr
L003194:
	dbf	d1,L003160
L003198:
	move.b	$3e0(a6),d0
L00319c:
	btst.l	#$0,d0
L0031a0:
	beq.b	L0031a8
L0031a2:
	btst.l	#$1,d0
L0031a6:
	bne.b	L0031c0
L0031a8:
	moveq	#$44,d0
L0031aa:
	move.l	a3,-(sp)
L0031ac:
	movea.l	$3a4(a6),a3
L0031b0:
	pea	L0031be(pc)
L0031b4:
	move.l	$168(a3),-(sp)
L0031b8:
	movea.l	$568(a3),a3
L0031bc:
	rts
L0031be:
	dc.b	$26
L0031bf:
	dc.b	$5f
L0031c0:
	movea.l	$50(a6),a2
L0031c4:
	movea.l	$8(a2),sp
L0031c8:
	move.l	a2,$4c(a6)
L0031cc:
	subq.l	#$1,$3ac(a4)
L0031d0:
	lea	$37c(a6),a0
L0031d4:
	bra.b	L0031f0
L0031d6:
	btst.b	#$1,$2e(a6)
L0031dc:
	beq.b	L0031e4
L0031de:
	andi	#-$701,sr
L0031e2:
	rts
L0031e4:
	stop	#$3000
L0031e8:
	rts
L0031ea:
	movea.l	$940(a6),a4
L0031ee:
	jsr	(a4)
L0031f0:
	ori	#$700,sr
L0031f4:
	movea.l	$30(a0),a4
L0031f8:
	cmpa.l	a4,a0
L0031fa:
	beq.b	L0031ea
L0031fc:
	move.l	$2e0(a4),d0
L003200:
	beq.b	L0031ea
L003202:
	move.w	$8aa(a6),d0
L003206:
	beq.b	L003210
L003208:
	cmp.w	$0(a4),d0
L00320c:
	beq.b	L003234
L00320e:
	bra.b	L003228
L003210:
	move.w	$18(a4),d0
L003214:
	cmp.w	$8a6(a6),d0
L003218:
	bcc.b	L003234
L00321a:
	bset.b	#$5,$1c(a4)
L003220:
	btst.b	#$7,$1c(a4)
L003226:
	bne.b	L003234
L003228:
	andi	#-$701,sr
L00322c:
	movea.l	a4,a0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00322e:
	dc.w	$6100
	dc.w	L001844-*
L003232:
	bra.b	L0031d0
L003234:
	movem.l	$30(a4),a1/a2
L00323a:
	move.l	a1,$30(a2)
L00323e:
	move.l	a2,$34(a1)
L003242:
	move.l	a4,$30(a4)
L003246:
	move.l	a4,$34(a4)
L00324a:
	move.l	a4,$4c(a6)
L00324e:
	move.b	#$2a,$20(a4)
L003254:
	addq.l	#$1,$948(a6)
L003258:
	beq.b	L003254
L00325a:
	movea.l	$8(a4),sp
L00325e:
	movea.l	$c(a4),a1
L003262:
	move	a1,usp
L003264:
	move.w	$776(a6),$778(a6)
L00326a:
	btst.b	#$5,$40(sp)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L003270:
	dc.w	$6700
	dc.w	L00035e-*
L003274:
	move.w	$3b0(a4),d0
L003278:
	sub.w	$3b2(a4),d0
L00327c:
	cmpi.l	#$4a696d69,$0(a4,d0.w*1)
L003284:
	beq.b	L003288
L003286:
	bsr.b	L0032c8
L003288:
	ori	#$700,sr
L00328c:
	bclr.b	#$5,$1c(a4)
L003292:
	tst.b	$2f(a6)
L003296:
	beq.b	L0032ac
L003298:
	move.l	$58(a6),d0
L00329c:
	beq.b	L0032a8
L00329e:
	cmp.l	a4,d0
L0032a0:
	beq.b	L0032ac
L0032a2:
	movea.l	d0,a1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0032a4:
	dc.w	$6100
	dc.w	Q9_fpu_save_fe0-*
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0032a8:
	dc.w	$6100
	dc.w	Q9_fpu_restore_1034-*
L0032ac:
	move.l	a3,-(sp)
L0032ae:
	movea.l	$3a4(a6),a3
L0032b2:
	pea	L0032c0(pc)
L0032b6:
	move.l	$fc(a3),-(sp)
L0032ba:
	movea.l	$4fc(a3),a3
L0032be:
	rts
L0032c0:
	movea.l	(sp)+,a3
L0032c2:
	movem.l	(sp)+,d0/d1/d2/d3/d4/d5/d6/d7/a0/a1/a2/a3/a4/a5/a6/sp
L0032c6:
	rte
L0032c8:
	bset.b	#$2,$1c(a4)
L0032ce:
	beq.b	L0032d2
L0032d0:
	rts
L0032d2:
	move.w	#$a6,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0032d6:
	dc.w	$6000
	dc.w	Q9_exc_default_action_24d8-*
L0032da:
	trapf.l	#$0
L0032e0:
	bsr.b	L0032f0
L0032e2:
	movem.l	d0/d1,$0(a5)
L0032e8:
	movem.l	a0/a1,$20(a5)
L0032ee:
	rts
L0032f0:
	moveq	#$0,d0
L0032f2:
	move.b	(a0)+,d0
L0032f4:
	cmpi.b	#$2f,d0
L0032f8:
	bne.b	L0032fc
L0032fa:
	move.b	(a0)+,d0
L0032fc:
	lea	-$1(a0),a1
L003300:
	moveq	#$0,d1
L003302:
	bsr.b	L003326
L003304:
	bcs.b	L003318
L003306:
	addq.w	#$1,d1
L003308:
	move.b	(a0)+,d0
L00330a:
	bsr.b	L003326
L00330c:
	bcc.b	L003306
L00330e:
	move.b	-(a0),d0
L003310:
	bclr.l	#$1f,d1
L003314:
	bne.b	L003322
L003316:
	move.b	(a1),d0
L003318:
	movea.l	a1,a0
L00331a:
	move.w	#$eb,d1
L00331e:
	ori	#$1,ccr
L003322:
	exg	a1,a0
L003324:
	rts
L003326:
	cmpi.b	#$7a,d0
L00332a:
	bhi.b	L003368
L00332c:
	cmpi.b	#$61,d0
L003330:
	bcc.b	L003352
L003332:
	cmpi.b	#$41,d0
L003336:
	bcs.b	L003346
L003338:
	cmpi.b	#$5a,d0
L00333c:
	bls.b	L003352
L00333e:
	cmpi.b	#$5f,d0
L003342:
	beq.b	L003356
L003344:
	bra.b	L003368
L003346:
	cmpi.b	#$30,d0
L00334a:
	bcs.b	L00335c
L00334c:
	cmpi.b	#$39,d0
L003350:
	bhi.b	L003368
L003352:
	bset.l	#$1f,d1
L003356:
	andi	#-$2,ccr
L00335a:
	rts
L00335c:
	cmpi.b	#$2e,d0
L003360:
	beq.b	L003356
L003362:
	cmpi.b	#$24,d0
L003366:
	beq.b	L003356
L003368:
	ori	#$1,ccr
L00336c:
	rts
L00336e:
	dc.b	$51
L00336f:
	dc.b	$fc
* Gibt eine Prozess-ID in der ID-Tabelle frei (eigene Freiliste wiederverwendbarer IDs).
Q9_proc_id_free_3370:
	cmp.w	(a0),d0
L003372:
	bhi.b	L0033bc
L003374:
	tst.w	d0
L003376:
	beq.b	L0033bc
L003378:
	move.w	d0,d1
L00337a:
	lsl.w	#$2,d1
L00337c:
	tst.l	$0(a0,d1.w*1)
L003380:
	beq.b	L0033bc
L003382:
	move.w	(a0),d1
L003384:
	lsl.w	#$2,d1
L003386:
	lea	$4(a0,d1.w*1),a2
L00338a:
	move.w	$2(a2),d1
L00338e:
	beq.b	L003396
L003390:
	lsl.w	#$2,d1
L003392:
	move.w	d0,$2(a2,d1.w*1)
L003396:
	move.w	d0,$2(a2)
L00339a:
	tst.w	(a2)
L00339c:
	bne.b	L0033a0
L00339e:
	move.w	d0,(a2)
L0033a0:
	moveq	#$0,d1
L0033a2:
	move.w	$2(a0),d1
L0033a6:
	lsl.w	#$2,d0
L0033a8:
	adda.w	d0,a0
L0033aa:
	lsr.w	#$2,d0
L0033ac:
	movea.l	(a0),a2
L0033ae:
	cmp.w	(a2),d0
L0033b0:
	bne.b	L0033bc
L0033b2:
	clr.l	(a0)
L0033b4:
	moveq	#$0,d0
L0033b6:
	move.w	d1,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0033b8:
	dc.w	$6000
	dc.w	Q9_dealloc_tail_131c-*
L0033bc:
	move.w	#$e0,d1
L0033c0:
	ori	#$1,ccr
L0033c4:
	rts
L0033c6:
	trapf
L0033c8:
	move	sr,d3
L0033ca:
	ori	#$700,sr
L0033ce:
	subq.b	#$1,$370(a4)
L0033d2:
	bcc.b	L0033d8
L0033d4:
	clr.b	$370(a4)
L0033d8:
	bne.b	L0033f6
L0033da:
	tst.w	$26(a4)
L0033de:
	beq.b	L0033f6
L0033e0:
	movem.l	(sp)+,d4/d5/d6
L0033e4:
	move.l	d5,$140(a4)
L0033e8:
	move.l	d6,$144(a4)
L0033ec:
	lea	$48(a5),sp
L0033f0:
	movea.l	(sp),a5
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0033f2:
	dc.w	$6000
	dc.w	L000eda-*
L0033f6:
	movea.l	$37c(a4),a0
L0033fa:
	move.w	$372(a4),d1
L0033fe:
	cmpi.w	#$8,d1
L003402:
	bls.b	L003426
L003404:
	movea.l	$4(a0),a2
L003408:
	lea	$380(a4),a1
L00340c:
	cmpa.l	a2,a1
L00340e:
	beq.b	L003426
L003410:
	subq.w	#$1,$372(a4)
L003414:
	movea.l	$4(a2),a1
L003418:
	move.l	a0,$0(a1)
L00341c:
	move.l	a1,$4(a0)
L003420:
	moveq	#$10,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L003422:
	dc.w	$6100
	dc.w	Q9_dealloc_tail_131c-*
L003426:
	subq.l	#$1,$3b4(a4)
L00342a:
	beq.b	L00346c
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00342c:
	dc.w	$6500
	dc.w	L0034c0-*
L003430:
	move.l	d3,-(sp)
L003432:
	movea.l	$c(a4),a1
L003436:
	movem.l	$20(a1),d0/d1/d2/d3/d4/d5/d6/a0
L00343c:
	movem.l	d0/d1/d2/d3/d4/d5/d6,$20(a5)
L003442:
	movem.l	$0(a1),d0/d1/d2/d3/d4/d5/d6/d7
L003448:
	movem.l	d0/d1/d2/d3/d4/d5/d6/d7,$0(a5)
L00344e:
	move	a0,usp
L003450:
	move.l	a0,$c(a4)
L003454:
	move.w	$40(a1),$40(a5)
L00345a:
	move.l	$42(a1),$42(a5)
L003460:
	move.w	$46(a1),$46(a5)
L003466:
	move.l	a5,d1
L003468:
	move.l	(sp)+,d3
L00346a:
	bra.b	L003488
L00346c:
	movem.l	(sp)+,d4/d5/d6
L003470:
	move.l	d5,$140(a4)
L003474:
	move.l	d6,$144(a4)
L003478:
	lea	$48(a5),sp
L00347c:
	move.l	(sp)+,d1
L00347e:
	movea.l	(sp)+,a5
L003480:
	move	a5,usp
L003482:
	move.l	a5,$c(a4)
L003486:
	movea.l	sp,a1
L003488:
	ori	#$700,sr
L00348c:
	tst.b	$2f(a6)
L003490:
	beq.b	L0034b8
L003492:
	movea.l	$334(a4),a0
L003496:
	move.l	$4(a0),d0
L00349a:
	bne.b	L0034b0
L00349c:
	lea	$54(a1),a1
L0034a0:
	tst.l	(a1)
L0034a2:
	bne.b	L0034aa
L0034a4:
	frestore	L000050(pc)
L0034a8:
	bra.b	L0034b8
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0034aa:
	dc.w	$6100
	dc.w	L00103e-*
L0034ae:
	bra.b	L0034b8
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0034b0:
	dc.w	$6100
	dc.w	Q9_fpu_restore_1034-*
L0034b4:
	clr.l	$4(a1)
L0034b8:
	movea.l	d1,sp
L0034ba:
	move	d3,sr
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0034bc:
	dc.w	$6000
	dc.w	L000584-*
L0034c0:
	move.w	#$e9,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0034c4:
	dc.w	$6000
	dc.w	Q9_exc_no_handler_fc4-*
L0034c8:
	movem.l	a1/a0/d1,-(sp)
L0034cc:
	movea.l	$44(a6),a0
L0034d0:
	move.w	(a0),d0
L0034d2:
	cmp.w	$0(a4),d0
L0034d6:
	beq.b	L0034ee
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0034d8:
	dc.w	$6100
	dc.w	Q9_proc_id_lookup_2cee-*
L0034dc:
	bcs.b	L0034ee
L0034de:
	move.l	$14(a4),d1
L0034e2:
	cmp.l	$14(a1),d1
L0034e6:
	bne.b	L0034ee
L0034e8:
	movem.l	(sp),d1/a0
L0034ec:
	bsr.b	L003502
L0034ee:
	subq.w	#$1,d0
L0034f0:
	cmpi.w	#$2,d0
L0034f4:
	bcc.b	L0034d2
L0034f6:
	movem.l	(sp)+,d1/a0/a1
L0034fa:
	moveq	#$0,d0
L0034fc:
	rts
L0034fe:
	dc.b	$4a
L0034ff:
	dc.b	$40
L003500:
	dc.b	$67
L003501:
	dc.b	$c6
L003502:
	movem.l	a3/a2/a1/a0/d3/d2/d1/d0,-(sp)
L003506:
	move	sr,d3
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L003508:
	dc.w	$6100
	dc.w	Q9_proc_id_lookup_2cee-*
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00350c:
	dc.w	$6500
	dc.w	L003622-*
L003510:
	btst.b	#$0,$1c(a1)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L003516:
	dc.w	$6600
	dc.w	L00361e-*
L00351a:
	ori	#$700,sr
L00351e:
	tst.l	$37c(a1)
L003522:
	bne.b	L003546
L003524:
	lea	$380(a1),a0
L003528:
	move.l	a0,$37c(a1)
L00352c:
	move.l	a0,$0(a0)
L003530:
	move.l	a0,$4(a0)
L003534:
	move.l	a0,$c(a0)
L003538:
	clr.w	$a(a0)
L00353c:
	clr.l	$378(a1)
L003540:
	move.w	#$1,$372(a1)
L003546:
	cmpi.w	#$20,d1
L00354a:
	bcc.b	L00359e
L00354c:
	cmpi.w	#$1,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L003550:
	dc.w	$6700
	dc.w	L0035fa-*
L003554:
	cmpi.w	#$0,d1
L003558:
	bne.b	L003596
L00355a:
	move.w	#$e4,d1
L00355e:
	tst.w	$14(a4)
L003562:
	beq.b	L003570
L003564:
	move.l	$14(a4),d0
L003568:
	cmp.l	$14(a1),d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00356c:
	dc.w	$6600
	dc.w	L00361e-*
L003570:
	bset.b	#$1,$1c(a1)
L003576:
	movea.l	$37c(a1),a2
L00357a:
	tst.l	$2ac(a1)
L00357e:
	beq.b	L0035f2
L003580:
	movea.l	$2ac(a1),a0
L003584:
	movea.l	$8(a0),a0
L003588:
	clr.l	$4(a0)
L00358c:
	move.l	#$2,$8(a0)
L003594:
	bra.b	L0035f2
L003596:
	move.l	$374(a1),d0
L00359a:
	btst.l	d1,d0
L00359c:
	bne.b	L003616
L00359e:
	btst.b	#$1,$1c(a1)
L0035a4:
	bne.b	L00361e
L0035a6:
	tst.l	$38c(a1)
L0035aa:
	beq.b	L0035c8
L0035ac:
	movea.l	$38c(a1),a2
L0035b0:
	clr.l	$38c(a1)
L0035b4:
	movea.l	$0(a2),a0
L0035b8:
	cmpa.l	a0,a2
L0035ba:
	beq.b	L0035ea
L0035bc:
	tst.w	$a(a0)
L0035c0:
	bne.b	L0035ea
L0035c2:
	move.l	a0,$38c(a1)
L0035c6:
	bra.b	L0035ea
L0035c8:
	moveq	#$10,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0035ca:
	dc.w	$6100
	dc.w	L0012b4-*
L0035ce:
	bcs.b	L003622
L0035d0:
	addq.w	#$1,$372(a1)
L0035d4:
	movea.l	$37c(a1),a0
L0035d8:
	movea.l	$4(a0),a3
L0035dc:
	dc.b	$48
L0035dd:
	dc.b	$ea
L0035de:
	dc.b	$09
L0035df:
	dc.b	$00
L0035e0:
	dc.b	$00,$00,$27,$4a
L0035e4:
	dc.b	$00,$00,$21,$4a
* Rohbytes statt Instruktion (ori.b #0x69,D4b) -- siehe FORCE_RAW_BYTES im Konverter
L0035e8:
	dc.b	$00,$04,$53,$69
L0035ec:
	bchg.b	d1,-$57(a2,d5.w*2)
L0035f0:
	bchg.b	d1,$00003541.w
L0035f4:
	dc.b	$00
L0035f5:
	dc.b	$0a
L0035f6:
	dc.b	$33
L0035f7:
	dc.b	$41
L0035f8:
	dc.b	$00
L0035f9:
	dc.b	$26
L0035fa:
	dc.b	$b3
L0035fb:
	dc.b	$e9
L0035fc:
	dc.b	$00
L0035fd:
	dc.b	$30
L0035fe:
	dc.b	$67
L0035ff:
	dc.b	$08
L003600:
	dc.b	$0c
L003601:
	dc.b	$29
L003602:
	dc.b	$00
L003603:
	dc.b	$61
L003604:
	dc.b	$00
L003605:
	dc.b	$20
L003606:
	dc.b	$66
L003607:
	dc.b	$08
L003608:
	dc.b	$08
L003609:
	dc.b	$e9
L00360a:
	dc.b	$00
L00360b:
	dc.b	$07
L00360c:
	dc.b	$03
L00360d:
	dc.b	$71
L00360e:
	dc.b	$60
L00360f:
	dc.b	$06
L003610:
	dc.b	$20
L003611:
	dc.b	$49
L003612:
	dc.b	$61
L003613:
	dc.b	$00
L003614:
	dc.b	$e2
L003615:
	dc.b	$26
L003616:
	move	d3,sr
L003618:
	movem.l	(sp)+,d0/d1/d2/d3/a0/a1/a2/a3
L00361c:
	rts
L00361e:
	move.w	#$e0,d1
L003622:
	move.w	d1,$6(sp)
L003626:
	ori.w	#$1,d3
L00362a:
	bra.b	L003616
L00362c:
	trapf.w	#$0
L003630:
	move.l	a0,-(sp)
L003632:
	cmpi.w	#$4afc,(a0)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L003636:
	dc.w	$6600
	dc.w	L004478-*
L00363a:
	movem.l	d3/d2/d1/d0,-(sp)
L00363e:
	bsr.b	L003660
L003640:
	moveq	#-$1,d1
L003642:
	move.l	$4(a0),d3
L003646:
	subq.l	#$3,d3
L003648:
	clr.b	-$1(a0,d3.l*1)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00364c:
	dc.w	$6100
	dc.w	L001ba4-*
L003650:
	eori.l	#$ffffff,d1
L003656:
	move.l	d1,-$1(a0)
L00365a:
	movem.l	(sp)+,d0/d1/d2/d3/a0
L00365e:
	rts
L003660:
	move.l	a0,-(sp)
L003662:
	moveq	#$2d,d1
L003664:
	lsr.w	#$1,d1
L003666:
	move.w	#-$1,d2
L00366a:
	move.w	(a0)+,d0
L00366c:
	eor.w	d0,d2
L00366e:
	dbf	d1,L00366a
L003672:
	move.w	d2,(a0)
L003674:
	movea.l	(sp)+,a0
L003676:
	rts
L003678:
	dc.b	$26
L003679:
	dc.b	$6d
L00367a:
	dc.b	$00
L00367b:
	dc.b	$2c
L00367c:
	dc.b	$60
L00367d:
	dc.b	$3a
L00367e:
	andi.w	#-$7f01,d1
L003682:
	lsl.l	#$2,d1
L003684:
	move.w	(a1)+,d2
L003686:
	bne.b	L00368e
L003688:
	lea	L001380(pc),a0
L00368c:
	bra.b	L003692
L00368e:
	lea	$0(a1,d2.w*1),a0
L003692:
	movea.l	$3a4(a6),a2
L003696:
	move.l	a0,$0(a2,d1.w*1)
L00369a:
	lea	$400(a2),a2
L00369e:
	move.l	a3,$0(a2,d1.w*1)
L0036a2:
	btst.l	#$11,d1
L0036a6:
	bne.b	L0036b8
L0036a8:
	movea.l	$3a8(a6),a2
L0036ac:
	move.l	a0,$0(a2,d1.w*1)
L0036b0:
	lea	$400(a2),a2
L0036b4:
	move.l	a3,$0(a2,d1.w*1)
L0036b8:
	moveq	#$0,d1
L0036ba:
	move.w	(a1)+,d1
L0036bc:
	cmpi.w	#-$1,d1
L0036c0:
	bne.b	L00367e
L0036c2:
	andi	#-$2,ccr
L0036c6:
	rts
* Rohbytes statt Instruktion (ori.b #-0x54,D2b) -- siehe FORCE_RAW_BYTES im Konverter
L0036c8:
	dc.b	$00,$02,$09,$ac
L0036cc:
	or.b	d2,d0
L0036ce:
	bset.b	d4,-(a4)
* Rohbytes statt Instruktion (ori.b #-0x6c,D3b) -- siehe FORCE_RAW_BYTES im Konverter
L0036d0:
	dc.b	$00,$03,$f1,$94
* Rohbytes statt Instruktion (ori.b #-0x8,-(A5)) -- siehe FORCE_RAW_BYTES im Konverter
L0036d4:
	dc.b	$00,$25,$e5,$f8
* Rohbytes statt Instruktion (ori.b #0x54,-(A6)) -- siehe FORCE_RAW_BYTES im Konverter
L0036d8:
	dc.b	$00,$26,$ff,$54
* Rohbytes statt Instruktion (ori.b #-0x58,D4b) -- siehe FORCE_RAW_BYTES im Konverter
L0036dc:
	dc.b	$00,$04,$0d,$a8
L0036e0:
	or.b	d4,d0
* Rohbytes statt Instruktion (bclr.b D6,(0x5,PC)) -- siehe FORCE_RAW_BYTES im Konverter
L0036e2:
	dc.b	$0d,$ba,$00,$05
L0036e6:
	asr.w	#$1,d0
* Rohbytes statt Instruktion (ori.b #-0x14,D6b) -- siehe FORCE_RAW_BYTES im Konverter
L0036e8:
	dc.b	$00,$06,$ed,$ec
L0036ec:
	dc.b	$00
L0036ed:
	dc.b	$08
L0036ee:
	dc.b	$fe
L0036ef:
	dc.b	$0e
L0036f0:
	dc.b	$00
L0036f1:
	dc.b	$09
L0036f2:
	dc.b	$f6
L0036f3:
	dc.b	$dc
L0036f4:
	dc.b	$00
L0036f5:
	dc.b	$0a
L0036f6:
	dc.b	$02
L0036f7:
	dc.b	$f8
L0036f8:
	dc.b	$80
L0036f9:
	dc.b	$0a
L0036fa:
	dc.b	$03
L0036fb:
	dc.b	$0a
L0036fc:
	dc.b	$00
L0036fd:
	dc.b	$0d
L0036fe:
	dc.b	$04
L0036ff:
	dc.b	$c8
L003700:
	dc.b	$00
L003701:
	dc.b	$0c
L003702:
	dc.b	$f6
L003703:
	dc.b	$dc
L003704:
	dc.b	$00
L003705:
	dc.b	$0e
L003706:
	dc.b	$06
L003707:
	dc.b	$42
L003708:
	dc.b	$00
L003709:
	dc.b	$16
L00370a:
	dc.b	$04
L00370b:
	dc.b	$fc
L00370c:
	dc.b	$00
L00370d:
	dc.b	$15
L00370e:
	dc.b	$07
L00370f:
	dc.b	$a0
L003710:
	dc.b	$00
L003711:
	dc.b	$20
L003712:
	dc.b	$f7
L003713:
	dc.b	$cc
L003714:
	dc.b	$00
L003715:
	dc.b	$18
L003716:
	dc.b	$f5
L003717:
	dc.b	$70
L003718:
	dc.b	$00
L003719:
	dc.b	$19
L00371a:
	dc.b	$dc
L00371b:
	dc.b	$4a
L00371c:
	dc.b	$00
L00371d:
	dc.b	$1a
L00371e:
	dc.b	$f5
L00371f:
	dc.b	$28
L003720:
	dc.b	$00
L003721:
	dc.b	$1f
L003722:
	dc.b	$f5
L003723:
	dc.b	$44
L003724:
	dc.b	$00
L003725:
	dc.b	$1b
L003726:
	dc.b	$e4
L003727:
	dc.b	$20
L003728:
	dc.b	$00
L003729:
	dc.b	$1c
L00372a:
	dc.b	$06
L00372b:
	dc.b	$34
L00372c:
	dc.b	$00
L00372d:
	dc.b	$1d
L00372e:
	dc.b	$0a
L00372f:
	dc.b	$f8
L003730:
	dc.b	$80
L003731:
	dc.b	$1d
L003732:
	dc.b	$0a
L003733:
	dc.b	$f4
L003734:
	dc.b	$80
L003735:
	dc.b	$2f
L003736:
	dc.b	$ef
L003737:
	dc.b	$b0
L003738:
	dc.b	$80
L003739:
	dc.b	$30
L00373a:
	dc.b	$df
L00373b:
	dc.b	$cc
L00373c:
	dc.b	$80
L00373d:
	dc.b	$31
L00373e:
	dc.b	$fc
L00373f:
	dc.b	$30
L003740:
	dc.b	$80
L003741:
	dc.b	$37
L003742:
	dc.b	$f5
L003743:
	dc.b	$a4
L003744:
	dc.b	$80
L003745:
	dc.b	$4b
L003746:
	dc.b	$df
L003747:
	dc.b	$60
L003748:
	dc.b	$80
L003749:
	dc.b	$4c
L00374a:
	dc.b	$e6
L00374b:
	dc.b	$cc
L00374c:
	dc.b	$00
L00374d:
	dc.b	$00
L00374e:
	dc.b	$f8
L00374f:
	dc.b	$6a
L003750:
	dc.b	$80
L003751:
	dc.b	$00
L003752:
	dc.b	$f8
L003753:
	dc.b	$1c
L003754:
	dc.b	$00
L003755:
	dc.b	$10
L003756:
	dc.b	$fb
L003757:
	dc.b	$88
L003758:
	dc.b	$00
L003759:
	dc.b	$11
L00375a:
	dc.b	$e3
L00375b:
	dc.b	$5c
L00375c:
	dc.b	$00
L00375d:
	dc.b	$17
L00375e:
	dc.b	$e4
L00375f:
	dc.b	$28
L003760:
	dc.b	$00
L003761:
	dc.b	$28
L003762:
	dc.b	$db
L003763:
	dc.b	$96
L003764:
	dc.b	$80
L003765:
	dc.b	$28
L003766:
	dc.b	$db
L003767:
	dc.b	$3e
L003768:
	dc.b	$00
L003769:
	dc.b	$5c
L00376a:
	dc.b	$db
L00376b:
	dc.b	$a6
L00376c:
	dc.b	$80
L00376d:
	dc.b	$5c
L00376e:
	dc.b	$db
L00376f:
	dc.b	$80
L003770:
	dc.b	$00
L003771:
	dc.b	$29
L003772:
	dc.b	$db
L003773:
	dc.b	$bc
L003774:
	dc.b	$80
L003775:
	dc.b	$29
L003776:
	dc.b	$db
L003777:
	dc.b	$a4
L003778:
	dc.b	$80
L003779:
	dc.b	$2c
L00377a:
	dc.b	$e0
L00377b:
	dc.b	$be
L00377c:
	dc.b	$80
L00377d:
	dc.b	$2d
L00377e:
	dc.b	$f9
L00377f:
	dc.b	$c0
L003780:
	dc.b	$80
L003781:
	dc.b	$2e
L003782:
	dc.b	$0a
L003783:
	dc.b	$d4
L003784:
	dc.b	$80
L003785:
	dc.b	$32
L003786:
	dc.b	$fe
L003787:
	dc.b	$f0
L003788:
	dc.b	$80
L003789:
	dc.b	$38
L00378a:
	dc.b	$f9
L00378b:
	dc.b	$14
L00378c:
	dc.b	$80
L00378d:
	dc.b	$4e
L00378e:
	dc.b	$f0
L00378f:
	dc.b	$30
L003790:
	dc.b	$80
L003791:
	dc.b	$2a
L003792:
	dc.b	$f6
L003793:
	dc.b	$6c
L003794:
	dc.b	$00
L003795:
	dc.b	$1e
L003796:
	dc.b	$fc
L003797:
	dc.b	$30
L003798:
	dc.b	$00
L003799:
	dc.b	$21
L00379a:
	dc.b	$07
L00379b:
	dc.b	$ac
L00379c:
	dc.b	$80
L00379d:
	dc.b	$21
L00379e:
	dc.b	$db
L00379f:
	dc.b	$e0
L0037a0:
	dc.b	$00
L0037a1:
	dc.b	$27
L0037a2:
	dc.b	$00
L0037a3:
	dc.b	$9c
L0037a4:
	dc.b	$00
L0037a5:
	dc.b	$52
L0037a6:
	dc.b	$05
L0037a7:
	dc.b	$f0
L0037a8:
	dc.b	$80
L0037a9:
	dc.b	$52
L0037aa:
	dc.b	$05
L0037ab:
	dc.b	$f4
L0037ac:
	dc.b	$00
L0037ad:
	dc.b	$53
L0037ae:
	dc.b	$e9
L0037af:
	dc.b	$20
L0037b0:
	dc.b	$00
L0037b1:
	dc.b	$54
L0037b2:
	dc.b	$f5
L0037b3:
	dc.b	$64
L0037b4:
	dc.b	$00
L0037b5:
	dc.b	$57
L0037b6:
	dc.b	$01
L0037b7:
	dc.b	$a8
L0037b8:
	dc.b	$00
L0037b9:
	dc.b	$5a
L0037ba:
	dc.b	$ff
L0037bb:
	dc.b	$06
L0037bc:
	dc.b	$00
L0037bd:
	dc.b	$56
L0037be:
	dc.b	$db
L0037bf:
	dc.b	$d0
L0037c0:
	dc.b	$80
L0037c1:
	dc.b	$56
L0037c2:
	dc.b	$dc
L0037c3:
	dc.b	$0e
L0037c4:
	dc.b	$00
L0037c5:
	dc.b	$60
L0037c6:
	dc.b	$da
L0037c7:
	dc.b	$c6
L0037c8:
	dc.b	$80
L0037c9:
	dc.b	$61
L0037ca:
	dc.b	$ef
L0037cb:
	dc.b	$4c
L0037cc:
	dc.b	$00
L0037cd:
	dc.b	$62
L0037ce:
	dc.b	$10
L0037cf:
	dc.b	$58
L0037d0:
	dc.b	$00
L0037d1:
	dc.b	$55
L0037d2:
	dc.b	$06
L0037d3:
	dc.b	$2c
L0037d4:
	dc.b	$00
L0037d5:
	dc.b	$63
L0037d6:
	dc.b	$01
L0037d7:
	dc.b	$e4
L0037d8:
	dc.b	$80
L0037d9:
	dc.b	$63
L0037da:
	dc.b	$01
L0037db:
	dc.b	$cc
L0037dc:
	dc.b	$00
L0037dd:
	dc.b	$58
L0037de:
	dc.b	$db
L0037df:
	dc.b	$9c
L0037e0:
	dc.b	$00
L0037e1:
	dc.b	$3a
L0037e2:
	dc.b	$db
L0037e3:
	dc.b	$98
L0037e4:
	dc.b	$00
L0037e5:
	dc.b	$3b
L0037e6:
	dc.b	$db
L0037e7:
	dc.b	$94
L0037e8:
	dc.b	$00
L0037e9:
	dc.b	$40
L0037ea:
	dc.b	$e6
L0037eb:
	dc.b	$44
L0037ec:
	dc.b	$00
L0037ed:
	dc.b	$07
L0037ee:
	dc.b	$db
L0037ef:
	dc.b	$4c
L0037f0:
	dc.b	$00
L0037f1:
	dc.b	$22
L0037f2:
	dc.b	$e8
L0037f3:
	dc.b	$44
L0037f4:
	dc.b	$00
L0037f5:
	dc.b	$23
L0037f6:
	dc.b	$e6
L0037f7:
	dc.b	$40
L0037f8:
	dc.b	$00
L0037f9:
	dc.b	$24
L0037fa:
	dc.b	$e8
L0037fb:
	dc.b	$04
L0037fc:
	dc.b	$00
L0037fd:
	dc.b	$59
L0037fe:
	dc.b	$08
L0037ff:
	dc.b	$70
L003800:
	dc.b	$ff
L003801:
	dc.b	$ff
L003802:
	dc.b	$00,$02
L003804:
	dc.b	$d0,$86
L003806:
	dc.b	$00,$05
L003808:
	dc.b	$d0,$ce
* Rohbytes statt Instruktion (ori.b #-0x5e,D1b) -- siehe FORCE_RAW_BYTES im Konverter
L00380a:
	dc.b	$00,$01,$d3,$a2
* Rohbytes statt Instruktion (ori.b #-0x32,D5b) -- siehe FORCE_RAW_BYTES im Konverter
L00380e:
	dc.b	$00,$05,$d0,$ce
* Rohbytes statt Instruktion (ori.b #0x70,D1b) -- siehe FORCE_RAW_BYTES im Konverter
L003812:
	dc.b	$00,$01,$cc,$70
L003816:
	dc.b	$00
L003817:
	dc.b	$08
L003818:
	dc.b	$d0
L003819:
	dc.b	$ce
L00381a:
	dc.b	$00
L00381b:
	dc.b	$01
L00381c:
	dc.b	$cc
L00381d:
	dc.b	$50
L00381e:
	dc.b	$00
L00381f:
	dc.b	$07
L003820:
	dc.b	$c9
L003821:
	dc.b	$7e
L003822:
	dc.b	$00
L003823:
	dc.b	$01
L003824:
	dc.b	$cc
L003825:
	dc.b	$86
L003826:
	dc.b	$00
L003827:
	dc.b	$0f
L003828:
	dc.b	$cd
L003829:
	dc.b	$ce
L00382a:
	dc.b	$00
L00382b:
	dc.b	$07
L00382c:
	dc.b	$d0
L00382d:
	dc.b	$ce
L00382e:
	dc.b	$00
L00382f:
	dc.b	$02
L003830:
	dc.b	$d0
L003831:
	dc.b	$ce
L003832:
	dc.b	$00
L003833:
	dc.b	$07
L003834:
	dc.b	$d0
L003835:
	dc.b	$ce
L003836:
	dc.b	$00
L003837:
	dc.b	$c0
L003838:
	dc.b	$c9
L003839:
	dc.b	$7e
L00383a:
	dc.b	$00
L00383b:
	dc.b	$00
L00383c:
	dc.b	$51
L00383d:
	dc.b	$fa
L00383e:
	dc.b	$00
L00383f:
	dc.b	$00
L003840:
	dc.b	$4a
L003841:
	dc.b	$6c
L003842:
	dc.b	$00
L003843:
	dc.b	$14
L003844:
	dc.b	$67
L003845:
	dc.b	$06
L003846:
	dc.b	$4a
L003847:
	dc.b	$81
L003848:
	dc.b	$6a
L003849:
	dc.b	$00
L00384a:
	dc.b	$05
L00384b:
	dc.b	$3e
L00384c:
	dc.b	$76
L00384d:
	dc.b	$00
L00384e:
	dc.b	$4a
L00384f:
	dc.b	$81
L003850:
	dc.b	$6a
L003851:
	dc.b	$06
L003852:
	dc.b	$0c
L003853:
	dc.b	$40
L003854:
	dc.b	$10
L003855:
	dc.b	$00
L003856:
	dc.b	$60
L003857:
	dc.b	$0c
L003858:
	dc.b	$0c
L003859:
	dc.b	$40
L00385a:
	dc.b	$07
L00385b:
	dc.b	$a4
L00385c:
	dc.b	$65
L00385d:
	dc.b	$00
L00385e:
	dc.b	$05
L00385f:
	dc.b	$2a
L003860:
	dc.b	$0c
L003861:
	dc.b	$40
L003862:
	dc.b	$08
L003863:
	dc.b	$e4
L003864:
	dc.b	$64
L003865:
	dc.b	$00
L003866:
	dc.b	$05
L003867:
	dc.b	$22
L003868:
	dc.b	$0c
L003869:
	dc.b	$41
L00386a:
	dc.b	$00
L00386b:
	dc.b	$02
L00386c:
	dc.b	$62
L00386d:
	dc.b	$10
L00386e:
	dc.b	$67
L00386f:
	dc.b	$1c
L003870:
	dc.b	$16
L003871:
	dc.b	$36
L003872:
	dc.b	$00
L003873:
	dc.b	$00
L003874:
	dc.b	$4a
L003875:
	dc.b	$81
L003876:
	dc.b	$6b
L003877:
	dc.b	$34
L003878:
	dc.b	$1d
L003879:
	dc.b	$82
L00387a:
	dc.b	$00
L00387b:
	dc.b	$00
L00387c:
	dc.b	$60
L00387d:
	dc.b	$2e
L00387e:
	dc.b	$26
L00387f:
	dc.b	$36
L003880:
	dc.b	$00
L003881:
	dc.b	$00
L003882:
	dc.b	$4a
L003883:
	dc.b	$81
L003884:
	dc.b	$6b
L003885:
	dc.b	$26
L003886:
	dc.b	$2d
L003887:
	dc.b	$82
L003888:
	dc.b	$00
L003889:
	dc.b	$00
L00388a:
	dc.b	$60
L00388b:
	dc.b	$20
L00388c:
	dc.b	$36
L00388d:
	dc.b	$36
L00388e:
	dc.b	$00
L00388f:
	dc.b	$00
L003890:
	dc.b	$4a
L003891:
	dc.b	$81
L003892:
	dc.b	$6b
L003893:
	dc.b	$18
L003894:
	dc.b	$3d
L003895:
	dc.b	$82
L003896:
	dc.b	$00
L003897:
	dc.b	$00
L003898:
	dc.b	$b4
L003899:
	dc.b	$43
L00389a:
	dc.b	$67
L00389b:
	dc.b	$10
L00389c:
	dc.b	$47
L00389d:
	dc.b	$ee
L00389e:
	dc.b	$03
L00389f:
	dc.b	$7c
L0038a0:
	dc.b	$0c
L0038a1:
	dc.b	$40
L0038a2:
	dc.b	$08
L0038a3:
	dc.b	$a6
L0038a4:
	dc.b	$67
L0038a5:
	dc.b	$0c
L0038a6:
	dc.b	$0c
L0038a7:
	dc.b	$40
L0038a8:
	dc.b	$08
L0038a9:
	dc.b	$a8
L0038aa:
	dc.b	$67
L0038ab:
	dc.b	$2e
L0038ac:
	dc.b	$2b
L0038ad:
	dc.b	$43
L0038ae:
	dc.b	$00
L0038af:
	dc.b	$08
L0038b0:
	dc.b	$4e
L0038b1:
	dc.b	$75
L0038b2:
	dc.b	$b4
L0038b3:
	dc.b	$43
L0038b4:
	dc.b	$64
L0038b5:
	dc.b	$f6
L0038b6:
	dc.b	$20
L0038b7:
	dc.b	$2b
L0038b8:
	dc.b	$00
L0038b9:
	dc.b	$34
L0038ba:
	dc.b	$b7
L0038bb:
	dc.b	$c0
L0038bc:
	dc.b	$67
L0038bd:
	dc.b	$7e
L0038be:
	dc.b	$20
L0038bf:
	dc.b	$40
L0038c0:
	dc.b	$4a
L0038c1:
	dc.b	$a8
L0038c2:
	dc.b	$02
L0038c3:
	dc.b	$e0
L0038c4:
	dc.b	$66
L0038c5:
	dc.b	$76
L0038c6:
	dc.b	$20
L0038c7:
	dc.b	$28
L0038c8:
	dc.b	$00
L0038c9:
	dc.b	$34
L0038ca:
	dc.b	$b4
L0038cb:
	dc.b	$68
L0038cc:
	dc.b	$00
L0038cd:
	dc.b	$18
L0038ce:
	dc.b	$62
L0038cf:
	dc.b	$ea
L0038d0:
	dc.b	$21
L0038d1:
	dc.b	$7c
L0038d2:
	dc.b	$ff
L0038d3:
	dc.b	$ff
L0038d4:
	dc.b	$ff
L0038d5:
	dc.b	$ff
L0038d6:
	dc.b	$02
L0038d7:
	dc.b	$e0
L0038d8:
	dc.b	$60
L0038d9:
	dc.b	$e0
L0038da:
	dc.b	$20
L0038db:
	dc.b	$2b
L0038dc:
	dc.b	$00
L0038dd:
	dc.b	$30
L0038de:
	dc.b	$4a
L0038df:
	dc.b	$42
L0038e0:
	dc.b	$66
L0038e1:
	dc.b	$04
L0038e2:
	dc.b	$08
L0038e3:
	dc.b	$c2
L0038e4:
	dc.b	$00
L0038e5:
	dc.b	$10
L0038e6:
	dc.b	$22
L0038e7:
	dc.b	$03
L0038e8:
	dc.b	$66
L0038e9:
	dc.b	$04
L0038ea:
	dc.b	$08
L0038eb:
	dc.b	$c1
L0038ec:
	dc.b	$00
L0038ed:
	dc.b	$10
L0038ee:
	dc.b	$b4
L0038ef:
	dc.b	$81
L0038f0:
	dc.b	$65
L0038f1:
	dc.b	$24
L0038f2:
	dc.b	$72
L0038f3:
	dc.b	$00
L0038f4:
	dc.b	$b7
L0038f5:
	dc.b	$c0
L0038f6:
	dc.b	$67
L0038f7:
	dc.b	$44
L0038f8:
	dc.b	$20
L0038f9:
	dc.b	$40
L0038fa:
	dc.b	$4a
L0038fb:
	dc.b	$28
L0038fc:
	dc.b	$02
L0038fd:
	dc.b	$e0
L0038fe:
	dc.b	$6a
L0038ff:
	dc.b	$3c
L003900:
	dc.b	$20
L003901:
	dc.b	$28
L003902:
	dc.b	$00
L003903:
	dc.b	$30
L003904:
	dc.b	$32
L003905:
	dc.b	$28
L003906:
	dc.b	$00
L003907:
	dc.b	$18
L003908:
	dc.b	$b4
L003909:
	dc.b	$81
L00390a:
	dc.b	$63
L00390b:
	dc.b	$e8
L00390c:
	dc.b	$21
L00390d:
	dc.b	$7c
L00390e:
	dc.b	$ff
L00390f:
	dc.b	$ff
L003910:
	dc.b	$ff
L003911:
	dc.b	$ff
L003912:
	dc.b	$02
L003913:
	dc.b	$e0
L003914:
	dc.b	$60
L003915:
	dc.b	$de
L003916:
	dc.b	$72
L003917:
	dc.b	$00
L003918:
	dc.b	$b7
L003919:
	dc.b	$c0
L00391a:
	dc.b	$67
L00391b:
	dc.b	$20
L00391c:
	dc.b	$20
L00391d:
	dc.b	$40
L00391e:
	dc.b	$20
L00391f:
	dc.b	$28
L003920:
	dc.b	$00
L003921:
	dc.b	$30
L003922:
	dc.b	$4a
L003923:
	dc.b	$a8
L003924:
	dc.b	$02
L003925:
	dc.b	$e0
L003926:
	dc.b	$6b
L003927:
	dc.b	$f0
L003928:
	dc.b	$67
L003929:
	dc.b	$12
L00392a:
	dc.b	$32
L00392b:
	dc.b	$28
L00392c:
	dc.b	$00
L00392d:
	dc.b	$18
L00392e:
	dc.b	$b4
L00392f:
	dc.b	$81
L003930:
	dc.b	$62
L003931:
	dc.b	$e6
L003932:
	dc.b	$21
L003933:
	dc.b	$7c
L003934:
	dc.b	$ff
L003935:
	dc.b	$ff
L003936:
	dc.b	$ff
L003937:
	dc.b	$ff
L003938:
	dc.b	$02
L003939:
	dc.b	$e0
L00393a:
	dc.b	$60
L00393b:
	dc.b	$dc
L00393c:
	dc.b	$72
L00393d:
	dc.b	$ff
L00393e:
	dc.b	$20
L00393f:
	dc.b	$2b
L003940:
	dc.b	$00
L003941:
	dc.b	$30
L003942:
	dc.b	$60
L003943:
	dc.b	$10
L003944:
	dc.b	$20
L003945:
	dc.b	$40
L003946:
	dc.b	$20
L003947:
	dc.b	$28
L003948:
	dc.b	$00
L003949:
	dc.b	$30
L00394a:
	dc.b	$b2
L00394b:
	dc.b	$a8
L00394c:
	dc.b	$02
L00394d:
	dc.b	$e0
L00394e:
	dc.b	$66
L00394f:
	dc.b	$04
L003950:
	dc.b	$61
L003951:
	dc.b	$00
L003952:
	dc.b	$de
L003953:
	dc.b	$f2
L003954:
	dc.b	$b7
L003955:
	dc.b	$c0
L003956:
	dc.b	$66
L003957:
	dc.b	$ec
L003958:
	dc.b	$60
L003959:
	dc.b	$00
L00395a:
	dc.b	$ff
L00395b:
	dc.b	$52
L00395c:
	dc.b	$51
L00395d:
	dc.b	$fa
L00395e:
	dc.b	$00
L00395f:
	dc.b	$00
L003960:
	dc.b	$4a
L003961:
	dc.b	$80
L003962:
	dc.b	$66
L003963:
	dc.b	$36
L003964:
	dc.b	$4a
L003965:
	dc.b	$81
L003966:
	dc.b	$67
L003967:
	dc.b	$1c
L003968:
	dc.b	$6b
L003969:
	dc.b	$10
L00396a:
	dc.b	$53
L00396b:
	dc.b	$81
L00396c:
	dc.b	$62
L00396d:
	dc.b	$2c
L00396e:
	dc.b	$52
L00396f:
	dc.b	$2c
L003970:
	dc.b	$03
L003971:
	dc.b	$70
L003972:
	dc.b	$64
L003973:
	dc.b	$20
L003974:
	dc.b	$53
L003975:
	dc.b	$2c
L003976:
	dc.b	$03
L003977:
	dc.b	$70
L003978:
	dc.b	$60
L003979:
	dc.b	$1a
L00397a:
	dc.b	$52
L00397b:
	dc.b	$81
L00397c:
	dc.b	$66
L00397d:
	dc.b	$1c
L00397e:
	dc.b	$53
L00397f:
	dc.b	$2c
L003980:
	dc.b	$03
L003981:
	dc.b	$70
L003982:
	dc.b	$62
L003983:
	dc.b	$10
L003984:
	clr.b	$370(a4)
L003988:
	tst.l	$334(a4)
L00398c:
	beq.b	L003994
L00398e:
	movea.l	a4,a1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L003990:
	dc.w	$6100
	dc.w	Q9_fpu_migrate_1004-*
L003994:
	andi	#-$2,ccr
L003998:
	rts
L00399a:
	move.w	#$e1,d1
L00399e:
	ori	#$1,ccr
L0039a2:
	rts
L0039a4:
	trapf.w	#$0
L0039a8:
	move.w	#$ac,d1
L0039ac:
	ori	#$1,ccr
L0039b0:
	rts
L0039b2:
	dc.b	$32
L0039b3:
	dc.b	$3c
L0039b4:
	dc.b	$00
L0039b5:
	dc.b	$ab
L0039b6:
	dc.b	$00
L0039b7:
	dc.b	$3c
L0039b8:
	dc.b	$00
L0039b9:
	dc.b	$01
L0039ba:
	dc.b	$4e
L0039bb:
	dc.b	$75
L0039bc:
	dc.b	$4a
L0039bd:
	dc.b	$ac
L0039be:
	dc.b	$03
L0039bf:
	dc.b	$b4
L0039c0:
	dc.b	$67
L0039c1:
	dc.b	$2c
L0039c2:
	dc.b	$41
L0039c3:
	dc.b	$ed
L0039c4:
	dc.b	$ff
L0039c5:
	dc.b	$e4
L0039c6:
	dc.b	$bf
L0039c7:
	dc.b	$c8
L0039c8:
	dc.b	$66
L0039c9:
	dc.b	$e8
L0039ca:
	dc.b	$70
L0039cb:
	dc.b	$00
L0039cc:
	dc.b	$30
L0039cd:
	dc.b	$2c
L0039ce:
	dc.b	$03
L0039cf:
	dc.b	$b0
L0039d0:
	dc.b	$41
L0039d1:
	dc.b	$f4
L0039d2:
	dc.b	$08
L0039d3:
	dc.b	$9c
L0039d4:
	dc.b	$70
L0039d5:
	dc.b	$18
L0039d6:
	dc.b	$22
L0039d7:
	dc.b	$4f
L0039d8:
	dc.b	$20
L0039d9:
	dc.b	$d9
L0039da:
	dc.b	$51
L0039db:
	dc.b	$c8
L0039dc:
	dc.b	$ff
L0039dd:
	dc.b	$fc
L0039de:
	dc.b	$42
L0039df:
	dc.b	$ac
L0039e0:
	dc.b	$03
L0039e1:
	dc.b	$b4
L0039e2:
	dc.b	$4b
L0039e3:
	dc.b	$e8
L0039e4:
	dc.b	$ff
L0039e5:
	dc.b	$b8
L0039e6:
	dc.b	$29
L0039e7:
	dc.b	$4d
L0039e8:
	dc.b	$01
L0039e9:
	dc.b	$40
L0039ea:
	dc.b	$4f
L0039eb:
	dc.b	$ed
L0039ec:
	dc.b	$ff
L0039ed:
	dc.b	$e4
L0039ee:
	dc.b	$4e
L0039ef:
	dc.b	$75
L0039f0:
	dc.b	$61
L0039f1:
	dc.b	$00
L0039f2:
	dc.b	$ff
L0039f3:
	dc.b	$92
L0039f4:
	dc.b	$08
L0039f5:
	dc.b	$ac
L0039f6:
	dc.b	$00
L0039f7:
	dc.b	$07
L0039f8:
	dc.b	$03
L0039f9:
	dc.b	$71
L0039fa:
	dc.b	$4a
L0039fb:
	dc.b	$6c
L0039fc:
	dc.b	$00
L0039fd:
	dc.b	$26
L0039fe:
	dc.b	$67
L0039ff:
	dc.b	$06
L003a00:
	dc.b	$08
L003a01:
	dc.b	$ec
L003a02:
	dc.b	$00
L003a03:
	dc.b	$07
L003a04:
	dc.b	$03
L003a05:
	dc.b	$71
L003a06:
	cmpa.l	$50(a6),a4
L003a0a:
	bne.b	L003a14
L003a0c:
	tst.w	$2(a6)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L003a10:
	dc.w	$6600
	dc.w	L003ce8-*
L003a14:
	bclr.l	#$1f,d0
L003a18:
	beq.b	L003a1e
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L003a1a:
	dc.w	$6100
	dc.w	L003b96-*
L003a1e:
	move	sr,d1
L003a20:
	ori	#$700,sr
L003a24:
	bclr.b	#$7,$371(a4)
L003a2a:
	beq.b	L003a32
L003a2c:
	move	d1,sr
L003a2e:
	moveq	#$0,d1
L003a30:
	rts
L003a32:
	lea	L000558(pc),a3
L003a36:
	cmpa.l	(sp),a3
L003a38:
	bne.b	L003a92
L003a3a:
	cmpi.l	#$1,d0
L003a40:
	bhi.b	L003a92
L003a42:
	tst.l	$2ac(a4)
L003a46:
	bne.b	L003a92
L003a48:
	subq.l	#$1,d0
L003a4a:
	bcs.b	L003a64
L003a4c:
	move.l	d0,$0(a5)
L003a50:
	lea	$37c(a6),a3
L003a54:
	cmpa.l	$30(a3),a3
L003a58:
	beq.b	L003a2c
L003a5a:
	exg	a0,a4
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L003a5c:
	dc.w	$6100
	dc.w	Q9_scheduler_183a-*
L003a60:
	exg	a0,a4
L003a62:
	bra.b	L003a82
L003a64:
	move.b	#$73,$20(a4)
L003a6a:
	lea	$384(a6),a3
L003a6e:
	movea.l	$34(a3),a0
L003a72:
	move.l	a0,$34(a4)
L003a76:
	move.l	a3,$30(a4)
L003a7a:
	move.l	a4,$34(a3)
L003a7e:
	move.l	a4,$30(a0)
L003a82:
	move.l	a5,$8(a4)
L003a86:
	move.l	d5,$140(a4)
L003a8a:
	move.l	d6,$144(a4)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L003a8e:
	dc.w	$6000
	dc.w	Q9_reschedule_trampolin_3140-*
L003a92:
	movem.l	a2/a1/a0/d3/d2/d1,-(sp)
L003a96:
	lea	$384(a6),a2
L003a9a:
	subq.l	#$1,d0
L003a9c:
	bcc.b	L003aba
L003a9e:
	movea.l	$34(a2),a0
L003aa2:
	move.l	a0,$34(a4)
L003aa6:
	move.l	a2,$30(a4)
L003aaa:
	move.l	a4,$34(a2)
L003aae:
	move.l	a4,$30(a0)
L003ab2:
	move.b	#$73,$20(a4)
L003ab8:
	bra.b	L003ad6
L003aba:
	move.l	d0,$0(a5)
L003abe:
	bne.b	L003ae0
L003ac0:
	lea	$37c(a6),a3
L003ac4:
	cmpa.l	$30(a3),a3
L003ac8:
	bne.b	L003ad0
L003aca:
	move.l	(sp)+,d1
L003acc:
	move	d1,sr
L003ace:
	bra.b	L003b44
L003ad0:
	movea.l	a4,a0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L003ad2:
	dc.w	$6100
	dc.w	Q9_scheduler_183a-*
L003ad6:
	move.l	(sp)+,d1
L003ad8:
	pea	L003b50(pc)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L003adc:
	dc.w	$6000
	dc.w	L003b68-*
L003ae0:
	tst.w	$28(a6)
L003ae4:
	beq.b	L003b56
L003ae6:
	movea.l	a2,a0
L003ae8:
	move.l	$77c(a6),d1
L003aec:
	beq.b	L003af2
L003aee:
	cmp.l	d1,d0
L003af0:
	bcc.b	L003af6
L003af2:
	move.l	d0,$77c(a6)
L003af6:
	add.l	$54(a6),d0
L003afa:
	move.l	d0,$0(a5)
L003afe:
	movea.l	$30(a0),a0
L003b02:
	cmpa.l	a2,a0
L003b04:
	beq.b	L003b1a
L003b06:
	btst.b	#$6,$1c(a0)
L003b0c:
	beq.b	L003b1a
L003b0e:
	movea.l	$8(a0),a1
L003b12:
	move.l	$0(a1),d1
L003b16:
	cmp.l	d1,d0
L003b18:
	bpl.b	L003afe
L003b1a:
	movea.l	$34(a0),a1
L003b1e:
	move.l	a0,$30(a4)
L003b22:
	move.l	a1,$34(a4)
L003b26:
	move.l	a4,$30(a1)
L003b2a:
	move.l	a4,$34(a0)
L003b2e:
	bset.b	#$6,$1c(a4)
L003b34:
	move.l	(sp)+,d1
L003b36:
	move.b	#$73,$20(a4)
L003b3c:
	bsr.b	L003b68
L003b3e:
	sub.l	$54(a6),d0
L003b42:
	bpl.b	L003b46
L003b44:
	moveq	#$0,d0
L003b46:
	move.l	d0,$0(a5)
L003b4a:
	bclr.b	#$6,$1c(a4)
L003b50:
	movem.l	(sp)+,d2/d3/a0/a1/a2
L003b54:
	rts
L003b56:
	move.l	(sp)+,d1
L003b58:
	move	d1,sr
L003b5a:
	move.w	#$de,d1
L003b5e:
	ori	#$1,ccr
L003b62:
	bra.b	L003b50
L003b64:
	ori	#$700,sr
L003b68:
	move.l	$8(a4),d2
L003b6c:
	move.l	sp,d3
L003b6e:
	lea	-$48(sp),sp
L003b72:
	move.l	sp,$8(a4)
L003b76:
	movem.l	d0/d1/d2/d3/d4/d5/d6/d7/a0/a1/a2/a3/a4/a5/a6,(sp)
L003b7a:
	clr.w	$46(sp)
L003b7e:
	lea	L003b8e(pc),a0
L003b82:
	move.l	a0,$42(sp)
L003b86:
	move.w	d1,$40(sp)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L003b8a:
	dc.w	$6000
	dc.w	Q9_reschedule_trampolin_3140-*
L003b8e:
	move.l	d2,$8(a4)
L003b92:
	movea.l	d3,sp
L003b94:
	rts
L003b96:
	movem.l	d2/d1,-(sp)
L003b9a:
	move.l	d0,d1
L003b9c:
	beq.b	L003bc2
L003b9e:
	move.w	$28(a6),d2
L003ba2:
	beq.b	L003bc2
L003ba4:
	swap	d0
L003ba6:
	mulu.w	d2,d1
L003ba8:
	mulu.w	d2,d0
L003baa:
	lsr.l	#$8,d1
L003bac:
	cmpi.l	#$ffffff,d0
L003bb2:
	bhi.b	L003bc0
L003bb4:
	lsl.l	#$8,d0
L003bb6:
	add.l	d1,d0
L003bb8:
	bcs.b	L003bc0
L003bba:
	bne.b	L003bc2
L003bbc:
	moveq	#$1,d0
L003bbe:
	bra.b	L003bc2
L003bc0:
	moveq	#-$1,d0
L003bc2:
	movem.l	(sp)+,d1/d2
L003bc6:
	rts
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L003bc8:
	dc.w	$6100
	dc.w	Q9_proc_id_lookup_2cee-*
L003bcc:
	bcs.b	L003bfa
L003bce:
	tst.w	$14(a4)
L003bd2:
	beq.b	L003bde
L003bd4:
	move.l	$14(a4),d0
L003bd8:
	cmp.l	$14(a1),d0
L003bdc:
	bne.b	L003bfc
L003bde:
	move.w	$6(a5),$18(a1)
L003be4:
	move.w	$6(a5),$1a(a1)
L003bea:
	move.b	$20(a1),d0
L003bee:
	movea.l	a1,a0
L003bf0:
	cmpi.b	#$61,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L003bf4:
	dc.w	$6700
	dc.w	L001844-*
L003bf8:
	moveq	#$0,d1
L003bfa:
	rts
L003bfc:
	move.w	#$e0,d1
L003c00:
	ori	#$1,ccr
L003c04:
	rts
L003c06:
	trapf
L003c08:
	tst.w	$14(a4)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L003c0c:
	dc.w	$6600
	dc.w	L003d88-*
L003c10:
	addq.l	#$1,$3ac(a4)
L003c14:
	move.l	d1,$2a(a6)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L003c18:
	dc.w	$6100
	dc.w	L002eea-*
L003c1c:
	move.l	d1,$30(a6)
L003c20:
	neg.l	d0
L003c22:
	addi.l	#$15180,d0
L003c28:
	move.l	d0,$34(a6)
L003c2c:
	movea.l	$20(a6),a0
L003c30:
	move.w	$46(a0),d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L003c34:
	dc.w	$6700
	dc.w	L003ce4-*
L003c38:
	adda.w	d1,a0
L003c3a:
	move.w	#$c01,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L003c3e:
	dc.w	$6100
	dc.w	L002fa4-*
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L003c42:
	dc.w	$6500
	dc.w	L003cee-*
L003c46:
	tst.w	$8(a2)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L003c4a:
	dc.w	$6600
	dc.w	L003cdc-*
L003c4e:
	move.w	#$64,$774(a6)
L003c54:
	jsr	(a1)
L003c56:
	movem.l	a5/a1/a0/d2/d1/d0,-(sp)
L003c5a:
	scs	d2
L003c5c:
	move.w	$28(a6),$774(a6)
L003c62:
	subq.l	#$1,$3ac(a4)
L003c66:
	bne.b	L003c96
L003c68:
	tst.l	$8bc(a6)
L003c6c:
	bne.b	L003c96
* Rohbytes statt Instruktion (btst.b #-0x72,(0x1c,A4)) -- siehe FORCE_RAW_BYTES im Konverter
L003c6e:
	dc.b	$08,$2c,$0d,$8e,$00,$1c
L003c74:
	beq.b	L003c96
L003c76:
	moveq	#$1,d0
L003c78:
	movem.l	d1/d0,-(sp)
L003c7c:
	movea.l	sp,a5
L003c7e:
	move.l	a3,-(sp)
L003c80:
	movea.l	$3a4(a6),a3
L003c84:
	pea	L003c92(pc)
L003c88:
	move.l	$28(a3),-(sp)
L003c8c:
	movea.l	$428(a3),a3
L003c90:
	rts
L003c92:
	dc.b	$26
L003c93:
	dc.b	$5f
L003c94:
	dc.b	$50
L003c95:
	dc.b	$8f
L003c96:
	ror.b	#$1,d2
L003c98:
	bcs.b	L003cd6
L003c9a:
	move.l	#$15180,d2
L003ca0:
	sub.l	$34(a6),d2
L003ca4:
	movea.l	$44(a6),a0
L003ca8:
	move.w	(a0),d1
L003caa:
	addq.l	#$4,a0
L003cac:
	subq.w	#$2,d1
L003cae:
	move.l	(a0)+,d0
L003cb0:
	beq.b	L003cca
L003cb2:
	movea.l	d0,a1
L003cb4:
	tst.l	$2bc(a1)
L003cb8:
	beq.b	L003cc0
L003cba:
	tst.l	$2c0(a1)
L003cbe:
	bne.b	L003cca
L003cc0:
	move.l	d2,$2c0(a1)
L003cc4:
	move.l	$30(a6),$2bc(a1)
L003cca:
	dbf	d1,L003cae
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L003cce:
	dc.w	$6100
	dc.w	Q9_clock_hook_install_70c-*
L003cd2:
	andi	#-$2,ccr
L003cd6:
	movem.l	(sp)+,d0/d1/d2/a0/a1/a5
L003cda:
	rts
L003cdc:
	subq.l	#$1,$3ac(a4)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L003ce0:
	dc.w	$6000
	dc.w	L003d88-*
L003ce4:
	subq.l	#$1,$3ac(a4)
L003ce8:
	move.w	#$de,d1
L003cec:
	bra.b	L003cf2
L003cee:
	subq.l	#$1,$3ac(a4)
L003cf2:
	ori	#$1,ccr
L003cf6:
	rts
L003cf8:
	suba.l	a2,a2
L003cfa:
	move.w	(a1)+,d1
L003cfc:
	beq.b	L003d02
L003cfe:
	lea	$0(a1,d1.w*1),a2
L003d02:
	cmpi.w	#$8,d0
L003d06:
	bcs.b	L003d54
L003d08:
	move.b	d0,d1
L003d0a:
	andi.b	#$3,d1
L003d0e:
	bne.b	L003d54
L003d10:
	cmpi.w	#$2c,d0
L003d14:
	bls.b	L003d3c
L003d16:
	cmpi.w	#$c0,d0
L003d1a:
	bcs.b	L003d54
L003d1c:
	cmpi.w	#$d8,d0
L003d20:
	bhi.b	L003d54
L003d22:
	addi.w	#$278,d0
L003d26:
	move.l	a2,$0(a4,d0.w*1)
L003d2a:
	addi.w	#$1c,d0
L003d2e:
	beq.b	L003d36
L003d30:
	move.l	a0,$0(a4,d0.w*1)
L003d34:
	bra.b	L003d4a
L003d36:
	clr.l	$0(a4,d0.w*1)
L003d3a:
	bra.b	L003d4a
L003d3c:
	move.l	a0,$5c(a4,d0.w*1)
L003d40:
	move.l	a2,$34(a4,d0.w*1)
L003d44:
	bne.b	L003d4a
L003d46:
	move.l	a2,$5c(a4,d0.w*1)
L003d4a:
	move.w	(a1)+,d0
L003d4c:
	cmpi.w	#-$1,d0
L003d50:
	bne.b	L003cf8
L003d52:
	rts
L003d54:
	move.w	#$85,d1
L003d58:
	ori	#$1,ccr
L003d5c:
	rts
L003d5e:
	trapf
L003d60:
	tst.w	$14(a4)
L003d64:
	beq.b	L003d82
L003d66:
	move.l	$38(a4),d0
L003d6a:
	beq.b	L003d88
L003d6c:
	movea.l	d0,a0
L003d6e:
	move.l	$8(a0),d0
L003d72:
	cmp.l	$3a0(a4),d0
L003d76:
	bne.b	L003d88
L003d78:
	tst.w	$3a0(a4)
L003d7c:
	beq.b	L003d82
L003d7e:
	cmp.l	d0,d1
L003d80:
	bne.b	L003d88
L003d82:
	move.l	d1,$14(a4)
L003d86:
	rts
L003d88:
	move.w	#$a4,d1
L003d8c:
	ori	#$1,ccr
L003d90:
	rts
L003d92:
	trapf.l	#$0
L003d98:
	tst.l	$14(a4)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L003d9c:
	dc.w	$6600
	dc.w	L003d88-*
L003da0:
	moveq	#$66,d0
L003da2:
	bsr.b	L003dc6
L003da4:
	move	sr,d0
L003da6:
	ori	#$700,sr
L003daa:
	movem.l	a6/a5/a4/d0,-(sp)
L003dae:
	pea	L003dbe(pc)
L003db2:
	move.l	$8ec(a6),-(sp)
L003db6:
	movem.l	$0(a5),d0/d1/d2/d3/d4/d5/d6/d7/a0/a1/a2/a3/a4/a5/a6
L003dbc:
	rts
L003dbe:
	dc.b	$4c
L003dbf:
	dc.b	$df
L003dc0:
	dc.b	$70
L003dc1:
	dc.b	$01
L003dc2:
	dc.b	$46
L003dc3:
	dc.b	$c0
L003dc4:
	dc.b	$70
L003dc5:
	dc.b	$55
L003dc6:
	trap	#$0
L003dc8:
	ori.w	#$7200,(a2)+
L003dcc:
	rts
L003dce:
	trapf
L003dd0:
	movep.w	$443e(a3),d0
* Rohbytes statt Instruktion (cmpi.b #0x1e,(A0)) -- siehe FORCE_RAW_BYTES im Konverter
L003dd4:
	dc.b	$0c,$10,$01,$1e
L003dd8:
	move.b	(a2),$2003(a3)
* Rohbytes statt Instruktion (subi.b #0x18,(A5)) -- siehe FORCE_RAW_BYTES im Konverter
L003ddc:
	dc.b	$04,$15,$01,$18
L003de0:
	move.b	(a6)+,$4420(a3)
* Rohbytes statt Instruktion (andi.b #0x15,D0b) -- siehe FORCE_RAW_BYTES im Konverter
L003de4:
	dc.b	$02,$00,$16,$15
L003de8:
	chk.l	$d(a1,d1.w*8),d1
L003dec:
	btst.b	d6,(sp)
L003dee:
	dc.b	$44
L003def:
	dc.b	$4a
L003df0:
	dc.b	$43
L003df1:
	dc.b	$25
L003df2:
	dc.b	$05
L003df3:
	dc.b	$1e
L003df4:
	dc.b	$11
L003df5:
	dc.b	$17
L003df6:
	dc.b	$0a
L003df7:
	dc.b	$4c
L003df8:
	dc.b	$21
L003df9:
	dc.b	$00
L003dfa:
	dc.b	$0b
L003dfb:
	dc.b	$1b
L003dfc:
	dc.b	$0d
L003dfd:
	dc.b	$5c
L003dfe:
	dc.b	$64
L003dff:
	dc.b	$6c
L003e00:
	dc.b	$4c
L003e01:
	dc.b	$fa
L003e02:
	dc.b	$00
L003e03:
	dc.b	$03
L003e04:
	dc.b	$c2
L003e05:
	dc.b	$40
L003e06:
	dc.b	$24
L003e07:
	dc.b	$2e
L003e08:
	dc.b	$03
L003e09:
	dc.b	$c8
L003e0a:
	dc.b	$26
L003e0b:
	dc.b	$3a
L003e0c:
	dc.b	$c2
L003e0d:
	dc.b	$4c
L003e0e:
	dc.b	$78
L003e0f:
	dc.b	$00
L003e10:
	dc.b	$18
L003e11:
	dc.b	$2e
L003e12:
	dc.b	$00
L003e13:
	dc.b	$2f
L003e14:
	dc.b	$67
L003e15:
	dc.b	$24
L003e16:
	dc.b	$0c
L003e17:
	dc.b	$04
L003e18:
	dc.b	$00
L003e19:
	dc.b	$01
L003e1a:
	dc.b	$67
L003e1b:
	dc.b	$18
L003e1c:
	dc.b	$0c
L003e1d:
	dc.b	$04
L003e1e:
	dc.b	$00
L003e1f:
	dc.b	$02
L003e20:
	dc.b	$67
L003e21:
	dc.b	$12
L003e22:
	dc.b	$0c
L003e23:
	dc.b	$04
L003e24:
	dc.b	$00
L003e25:
	dc.b	$28
L003e26:
	dc.b	$67
L003e27:
	dc.b	$06
L003e28:
	dc.b	$0c
L003e29:
	dc.b	$04
L003e2a:
	dc.b	$00
L003e2b:
	dc.b	$3c
L003e2c:
	dc.b	$66
L003e2d:
	dc.b	$0c
L003e2e:
	dc.b	$04
L003e2f:
	dc.b	$84
L003e30:
	dc.b	$00
L003e31:
	dc.b	$00
L003e32:
	dc.b	$03
L003e33:
	dc.b	$70
L003e34:
	dc.b	$06
L003e35:
	dc.b	$84
L003e36:
	dc.b	$00
L003e37:
	dc.b	$01
L003e38:
	dc.b	$0d
L003e39:
	dc.b	$10
L003e3a:
	dc.b	$7a
L003e3b:
	dc.b	$00
L003e3c:
	dc.b	$2c
L003e3d:
	dc.b	$3c
L003e3e:
	dc.b	$01
L003e3f:
	dc.b	$03
L003e40:
	dc.b	$02
L003e41:
	dc.b	$00
L003e42:
	dc.b	$7e
L003e43:
	dc.b	$00
L003e44:
	dc.b	$48
L003e45:
	dc.b	$ed
L003e46:
	dc.b	$00
L003e47:
	dc.b	$ff
L003e48:
	dc.b	$00
L003e49:
	dc.b	$00
L003e4a:
	dc.b	$76
L003e4b:
	dc.b	$00
L003e4c:
	dc.b	$47
L003e4d:
	dc.b	$fa
L003e4e:
	dc.b	$c2
L003e4f:
	dc.b	$14
L003e50:
	dc.b	$61
L003e51:
	dc.b	$18
L003e52:
	dc.b	$65
L003e53:
	dc.b	$54
L003e54:
	dc.b	$20
L003e55:
	dc.b	$49
L003e56:
	dc.b	$47
L003e57:
	dc.b	$fa
L003e58:
	dc.b	$c2
L003e59:
	dc.b	$2c
L003e5a:
	dc.b	$61
L003e5b:
	dc.b	$0e
L003e5c:
	dc.b	$65
L003e5d:
	dc.b	$4a
L003e5e:
	dc.b	$20
L003e5f:
	dc.b	$4a
L003e60:
	dc.b	$47
L003e61:
	dc.b	$fa
L003e62:
	dc.b	$ff
L003e63:
	dc.b	$6e
L003e64:
	dc.b	$26
L003e65:
	dc.b	$3c
L003e66:
	dc.b	$72
L003e67:
	dc.b	$64
L003e68:
	dc.b	$6c
L003e69:
	dc.b	$63
L003e6a:
	dc.b	$22
L003e6b:
	dc.b	$08
L003e6c:
	dc.b	$67
L003e6d:
	dc.b	$3a
L003e6e:
	dc.b	$08
L003e6f:
	dc.b	$2d
L003e70:
	dc.b	$00
L003e71:
	dc.b	$05
L003e72:
	dc.b	$00
L003e73:
	dc.b	$40
L003e74:
	dc.b	$66
L003e75:
	dc.b	$20
L003e76:
	dc.b	$70
L003e77:
	dc.b	$50
L003e78:
	dc.b	$72
L003e79:
	dc.b	$03
L003e7a:
	dc.b	$c1
L003e7b:
	dc.b	$4a
L003e7c:
	dc.b	$2f
L003e7d:
	dc.b	$0b
L003e7e:
	dc.b	$26
L003e7f:
	dc.b	$6e
L003e80:
	dc.b	$03
L003e81:
	dc.b	$a4
L003e82:
	dc.b	$48
L003e83:
	dc.b	$7a
L003e84:
	dc.b	$00
L003e85:
	dc.b	$0c
L003e86:
	dc.b	$2f
L003e87:
	dc.b	$2b
L003e88:
	dc.b	$01
L003e89:
	dc.b	$60
L003e8a:
	dc.b	$26
L003e8b:
	dc.b	$6b
L003e8c:
	dc.b	$05
L003e8d:
	dc.b	$60
L003e8e:
	dc.b	$4e
L003e8f:
	dc.b	$75
L003e90:
	dc.b	$26
L003e91:
	dc.b	$5f
L003e92:
	dc.b	$c5
L003e93:
	dc.b	$48
L003e94:
	dc.b	$65
L003e95:
	dc.b	$12
L003e96:
	dc.b	$72
L003e97:
	dc.b	$4f
L003e98:
	dc.b	$10
L003e99:
	dc.b	$1b
L003e9a:
	dc.b	$b7
L003e9b:
	dc.b	$00
L003e9c:
	dc.b	$e1
L003e9d:
	dc.b	$9b
L003e9e:
	dc.b	$10
L003e9f:
	dc.b	$c0
L003ea0:
	dc.b	$57
L003ea1:
	dc.b	$c9
L003ea2:
	dc.b	$ff
L003ea3:
	dc.b	$f6
L003ea4:
	dc.b	$67
L003ea5:
	dc.b	$02
L003ea6:
	dc.b	$42
L003ea7:
	dc.b	$10
L003ea8:
	dc.b	$4e
L003ea9:
	dc.b	$75
L003eaa:
	dc.b	$51
L003eab:
	dc.b	$fb
L003eac:
	dc.b	$00
L003ead:
	dc.b	$00
L003eae:
	dc.b	$00
L003eaf:
	dc.b	$00
L003eb0:
	dc.b	$20
L003eb1:
	dc.b	$3c
L003eb2:
	dc.b	$00
L003eb3:
	dc.b	$01
L003eb4:
	dc.b	$51
L003eb5:
	dc.b	$80
L003eb6:
	dc.b	$36
L003eb7:
	dc.b	$2e
L003eb8:
	dc.b	$00
L003eb9:
	dc.b	$28
L003eba:
	dc.b	$59
L003ebb:
	dc.b	$8f
L003ebc:
	dc.b	$40
L003ebd:
	dc.b	$ef
L003ebe:
	dc.b	$00
L003ebf:
	dc.b	$00
L003ec0:
	dc.b	$00
L003ec1:
	dc.b	$7c
L003ec2:
	dc.b	$07
L003ec3:
	dc.b	$00
L003ec4:
	dc.b	$90
L003ec5:
	dc.b	$ae
L003ec6:
	dc.b	$00
L003ec7:
	dc.b	$34
L003ec8:
	dc.b	$22
L003ec9:
	dc.b	$2e
L003eca:
	dc.b	$00
L003ecb:
	dc.b	$2a
L003ecc:
	dc.b	$24
L003ecd:
	dc.b	$2e
L003ece:
	dc.b	$00
L003ecf:
	dc.b	$30
L003ed0:
	dc.b	$96
L003ed1:
	dc.b	$6e
L003ed2:
	dc.b	$07
L003ed3:
	dc.b	$74
L003ed4:
	dc.b	$46
L003ed5:
	dc.b	$ef
L003ed6:
	dc.b	$00
L003ed7:
	dc.b	$00
L003ed8:
	dc.b	$58
L003ed9:
	dc.b	$8f
L003eda:
	dc.b	$38
L003edb:
	dc.b	$2d
L003edc:
	dc.b	$00
L003edd:
	dc.b	$02
L003ede:
	dc.b	$08
L003edf:
	dc.b	$04
L003ee0:
	dc.b	$00
L003ee1:
	dc.b	$00
L003ee2:
	dc.b	$67
L003ee3:
	dc.b	$0a
L003ee4:
	dc.b	$2b
L003ee5:
	dc.b	$40
L003ee6:
	dc.b	$00
L003ee7:
	dc.b	$00
L003ee8:
	dc.b	$2b
L003ee9:
	dc.b	$42
L003eea:
	dc.b	$00
L003eeb:
	dc.b	$04
L003eec:
	dc.b	$60
L003eed:
	dc.b	$32
L003eee:
	dc.b	$2b
L003eef:
	dc.b	$41
L003ef0:
	dc.b	$00
L003ef1:
	dc.b	$04
L003ef2:
	dc.b	$66
L003ef3:
	dc.b	$12
L003ef4:
	dc.b	$22
L003ef5:
	dc.b	$02
L003ef6:
	dc.b	$61
L003ef7:
	dc.b	$00
L003ef8:
	dc.b	$ee
L003ef9:
	dc.b	$20
L003efa:
	dc.b	$b4
L003efb:
	dc.b	$ae
L003efc:
	dc.b	$00
L003efd:
	dc.b	$30
L003efe:
	dc.b	$66
L003eff:
	dc.b	$20
L003f00:
	dc.b	$2d
L003f01:
	dc.b	$41
L003f02:
	dc.b	$00
L003f03:
	dc.b	$2a
L003f04:
	dc.b	$60
L003f05:
	dc.b	$1a
L003f06:
	dc.b	$80
L003f07:
	dc.b	$fc
L003f08:
	dc.b	$0e
L003f09:
	dc.b	$10
L003f0a:
	dc.b	$3b
L003f0b:
	dc.b	$40
L003f0c:
	dc.b	$00
L003f0d:
	dc.b	$00
L003f0e:
	dc.b	$42
L003f0f:
	dc.b	$40
L003f10:
	dc.b	$48
L003f11:
	dc.b	$40
L003f12:
	dc.b	$80
L003f13:
	dc.b	$fc
L003f14:
	dc.b	$00
L003f15:
	dc.b	$3c
L003f16:
	dc.b	$1b
L003f17:
	dc.b	$40
L003f18:
	dc.b	$00
L003f19:
	dc.b	$02
L003f1a:
	dc.b	$48
L003f1b:
	dc.b	$40
L003f1c:
	dc.b	$1b
L003f1d:
	dc.b	$40
L003f1e:
	dc.b	$00
L003f1f:
	dc.b	$03
L003f20:
	dc.b	$54
L003f21:
	dc.b	$82
L003f22:
	dc.b	$84
L003f23:
	dc.b	$fc
L003f24:
	dc.b	$07
L003f25:
	dc.b	$00
L003f26:
	dc.b	$42
L003f27:
	dc.b	$42
L003f28:
	dc.b	$48
L003f29:
	dc.b	$42
L003f2a:
	dc.b	$84
L003f2b:
	dc.b	$fc
L003f2c:
	dc.b	$00
L003f2d:
	dc.b	$07
L003f2e:
	dc.b	$42
L003f2f:
	dc.b	$42
L003f30:
	dc.b	$48
L003f31:
	dc.b	$42
L003f32:
	dc.b	$2b
L003f33:
	dc.b	$42
L003f34:
	dc.b	$00
L003f35:
	dc.b	$08
L003f36:
	dc.b	$08
L003f37:
	dc.b	$04
L003f38:
	dc.b	$00
L003f39:
	dc.b	$01
L003f3a:
	dc.b	$67
L003f3b:
	dc.b	$0a
L003f3c:
	dc.b	$3b
L003f3d:
	dc.b	$6e
L003f3e:
	dc.b	$00
L003f3f:
	dc.b	$28
L003f40:
	dc.b	$00
L003f41:
	dc.b	$0c
L003f42:
	dc.b	$3b
L003f43:
	dc.b	$43
L003f44:
	dc.b	$00
L003f45:
	dc.b	$0e
L003f46:
	dc.b	$4e
L003f47:
	dc.b	$75
L003f48:
	dc.b	$32
L003f49:
	dc.b	$3c
L003f4a:
	dc.b	$00
L003f4b:
	dc.b	$e3
L003f4c:
	dc.b	$4a
L003f4d:
	dc.b	$40
L003f4e:
	dc.b	$67
L003f4f:
	dc.b	$00
L003f50:
	dc.b	$00
L003f51:
	dc.b	$b2
L003f52:
	dc.b	$0c
L003f53:
	dc.b	$40
L003f54:
	dc.b	$00
L003f55:
	dc.b	$0f
L003f56:
	dc.b	$62
L003f57:
	dc.b	$00
L003f58:
	dc.b	$00
L003f59:
	dc.b	$aa
L003f5a:
	dc.b	$e5
L003f5b:
	dc.b	$48
L003f5c:
	dc.b	$06
L003f5d:
	dc.b	$40
L003f5e:
	dc.b	$00
L003f5f:
	dc.b	$88
L003f60:
	dc.b	$47
L003f61:
	dc.b	$f4
L003f62:
	dc.b	$00
L003f63:
	dc.b	$00
L003f64:
	dc.b	$20
L003f65:
	dc.b	$08
L003f66:
	dc.b	$67
L003f67:
	dc.b	$00
L003f68:
	dc.b	$00
L003f69:
	dc.b	$d2
L003f6a:
	dc.b	$4a
L003f6b:
	dc.b	$10
L003f6c:
	dc.b	$67
L003f6d:
	dc.b	$00
L003f6e:
	dc.b	$00
L003f6f:
	dc.b	$cc
L003f70:
	dc.b	$4a
L003f71:
	dc.b	$93
L003f72:
	dc.b	$66
L003f73:
	dc.b	$00
L003f74:
	dc.b	$00
L003f75:
	dc.b	$8e
L003f76:
	dc.b	$30
L003f77:
	dc.b	$3c
L003f78:
	dc.b	$0b
L003f79:
	dc.b	$01
L003f7a:
	dc.b	$61
L003f7b:
	dc.b	$00
L003f7c:
	dc.b	$f0
L003f7d:
	dc.b	$42
L003f7e:
	dc.b	$64
L003f7f:
	dc.b	$08
L003f80:
	dc.b	$70
L003f81:
	dc.b	$00
L003f82:
	dc.b	$4e
L003f83:
	dc.b	$40
L003f84:
	dc.b	$00
L003f85:
	dc.b	$01
L003f86:
	dc.b	$65
L003f87:
	dc.b	$7a
L003f88:
	dc.b	$08
L003f89:
	dc.b	$2a
L003f8a:
	dc.b	$00
L003f8b:
	dc.b	$05
L003f8c:
	dc.b	$00
L003f8d:
	dc.b	$14
L003f8e:
	dc.b	$67
L003f8f:
	dc.b	$06
L003f90:
	dc.b	$4a
L003f91:
	dc.b	$6a
L003f92:
	dc.b	$00
L003f93:
	dc.b	$08
L003f94:
	dc.b	$66
L003f95:
	dc.b	$5e
L003f96:
	dc.b	$26
L003f97:
	dc.b	$8a
L003f98:
	dc.b	$48
L003f99:
	dc.b	$ed
L003f9a:
	dc.b	$07
L003f9b:
	dc.b	$00
L003f9c:
	dc.b	$00
L003f9d:
	dc.b	$20
L003f9e:
	dc.b	$22
L003f9f:
	dc.b	$4a
L003fa0:
	dc.b	$24
L003fa1:
	dc.b	$6c
L003fa2:
	dc.b	$03
L003fa3:
	dc.b	$2c
L003fa4:
	dc.b	$20
L003fa5:
	dc.b	$29
L003fa6:
	dc.b	$00
L003fa7:
	dc.b	$38
L003fa8:
	dc.b	$d0
L003fa9:
	dc.b	$a9
L003faa:
	dc.b	$00
L003fab:
	dc.b	$3c
L003fac:
	dc.b	$d0
L003fad:
	dc.b	$ad
L003fae:
	dc.b	$00
L003faf:
	dc.b	$04
L003fb0:
	dc.b	$67
L003fb1:
	dc.b	$0e
L003fb2:
	dc.b	$61
L003fb3:
	dc.b	$00
L003fb4:
	dc.b	$d3
L003fb5:
	dc.b	$4a
L003fb6:
	dc.b	$65
L003fb7:
	dc.b	$40
L003fb8:
	dc.b	$27
L003fb9:
	dc.b	$40
L003fba:
	dc.b	$00
L003fbb:
	dc.b	$78
L003fbc:
	dc.b	$61
L003fbd:
	dc.b	$00
L003fbe:
	dc.b	$eb
L003fbf:
	dc.b	$ae
L003fc0:
	dc.b	$d5
L003fc1:
	dc.b	$fc
L003fc2:
	dc.b	$00
L003fc3:
	dc.b	$00
L003fc4:
	dc.b	$80
L003fc5:
	dc.b	$00
L003fc6:
	dc.b	$27
L003fc7:
	dc.b	$4a
L003fc8:
	dc.b	$00
L003fc9:
	dc.b	$3c
L003fca:
	dc.b	$08
L003fcb:
	dc.b	$29
L003fcc:
	dc.b	$00
L003fcd:
	dc.b	$05
L003fce:
	dc.b	$00
L003fcf:
	dc.b	$14
L003fd0:
	dc.b	$d3
L003fd1:
	dc.b	$e9
L003fd2:
	dc.b	$00
L003fd3:
	dc.b	$48
L003fd4:
	dc.b	$66
L003fd5:
	dc.b	$32
L003fd6:
	dc.b	$20
L003fd7:
	dc.b	$6c
L003fd8:
	dc.b	$00
L003fd9:
	dc.b	$0c
L003fda:
	dc.b	$21
L003fdb:
	dc.b	$2d
L003fdc:
	dc.b	$00
L003fdd:
	dc.b	$42
L003fde:
	dc.b	$42
L003fdf:
	dc.b	$a0
L003fe0:
	dc.b	$21
L003fe1:
	dc.b	$2d
L003fe2:
	dc.b	$00
L003fe3:
	dc.b	$38
L003fe4:
	dc.b	$4e
L003fe5:
	dc.b	$60
L003fe6:
	dc.b	$29
L003fe7:
	dc.b	$48
L003fe8:
	dc.b	$00
L003fe9:
	dc.b	$0c
L003fea:
	dc.b	$2b
L003feb:
	dc.b	$4a
L003fec:
	dc.b	$00
L003fed:
	dc.b	$38
L003fee:
	dc.b	$2b
L003fef:
	dc.b	$49
L003ff0:
	dc.b	$00
L003ff1:
	dc.b	$42
L003ff2:
	dc.b	$4e
L003ff3:
	dc.b	$75
L003ff4:
	dc.b	$32
L003ff5:
	dc.b	$3c
L003ff6:
	dc.b	$00
L003ff7:
	dc.b	$a4
L003ff8:
	dc.b	$2f
L003ff9:
	dc.b	$01
L003ffa:
	dc.b	$24
L003ffb:
	dc.b	$49
L003ffc:
	dc.b	$61
L003ffd:
	dc.b	$00
L003ffe:
	dc.b	$00
L003fff:
	dc.b	$7a
L004000:
	dc.b	$22
L004001:
	dc.b	$1f
L004002:
	dc.b	$00
L004003:
	dc.b	$3c
L004004:
	dc.b	$00
L004005:
	dc.b	$01
L004006:
	dc.b	$4e
L004007:
	dc.b	$75
L004008:
	dc.b	$2f
L004009:
	dc.b	$0d
L00400a:
	dc.b	$48
L00400b:
	dc.b	$7a
L00400c:
	dc.b	$00
L00400d:
	dc.b	$1e
L00400e:
	dc.b	$48
L00400f:
	dc.b	$78
L004010:
	dc.b	$00
L004011:
	dc.b	$00
L004012:
	dc.b	$2f
L004013:
	dc.b	$2d
L004014:
	dc.b	$00
L004015:
	dc.b	$38
L004016:
	dc.b	$2f
L004017:
	dc.b	$09
L004018:
	dc.b	$22
L004019:
	dc.b	$4e
L00401a:
	dc.b	$2c
L00401b:
	dc.b	$4a
L00401c:
	dc.b	$4c
L00401d:
	dc.b	$ed
L00401e:
	dc.b	$01
L00401f:
	dc.b	$ff
L004020:
	dc.b	$00
L004021:
	dc.b	$00
L004022:
	dc.b	$4c
L004023:
	dc.b	$ed
L004024:
	dc.b	$3c
L004025:
	dc.b	$00
L004026:
	dc.b	$00
L004027:
	dc.b	$28
L004028:
	dc.b	$4e
L004029:
	dc.b	$75
L00402a:
	dc.b	$2c
L00402b:
	dc.b	$5f
L00402c:
	dc.b	$48
L00402d:
	dc.b	$ee
L00402e:
	dc.b	$3f
L00402f:
	dc.b	$ff
L004030:
	dc.b	$00
L004031:
	dc.b	$00
L004032:
	dc.b	$40
L004033:
	dc.b	$c0
L004034:
	dc.b	$1d
L004035:
	dc.b	$40
L004036:
	dc.b	$00
L004037:
	dc.b	$41
L004038:
	dc.b	$4e
L004039:
	dc.b	$75
L00403a:
	tst.l	(a3)
L00403c:
	beq.b	L004062
L00403e:
	movem.l	a2/a1/d2/d1/d0,-(sp)
L004042:
	move.l	$78(a3),d0
L004046:
	beq.b	L004056
L004048:
	movea.l	$3c(a3),a2
L00404c:
	suba.l	#$8000,a2
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004052:
	dc.w	$6100
	dc.w	L001330-*
L004056:
	movea.l	(a3),a2
L004058:
	clr.l	(a3)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00405a:
	dc.w	$6100
	dc.w	Q9_trampolin_slot88_4078-*
L00405e:
	movem.l	(sp)+,d0/d1/d2/a1/a2
L004062:
	clr.l	$3c(a3)
L004066:
	clr.l	$78(a3)
L00406a:
	rts
L00406c:
	dc.b	$51
L00406d:
	dc.b	$fa
L00406e:
	dc.b	$00
L00406f:
	dc.b	$00
L004070:
	dc.b	$02
L004071:
	dc.b	$3c
L004072:
	dc.b	$ff
L004073:
	dc.b	$fe
L004074:
	dc.b	$4e
L004075:
	dc.b	$75
L004076:
	dc.b	$51
L004077:
	dc.b	$fc
* Internes Trampolin, Tabellen-Slot 88.
Q9_trampolin_slot88_4078:
	moveq	#$30,d0
L00407a:
	moveq	#$1,d1
L00407c:
	move.l	a3,-(sp)
L00407e:
	movea.l	$3a4(a6),a3
L004082:
	pea	L004090(pc)
L004086:
	move.l	$160(a3),-(sp)
L00408a:
	movea.l	$560(a3),a3
L00408e:
	rts
L004090:
	movea.l	(sp)+,a3
L004092:
	bcs.b	L0040b2
L004094:
	move.l	$4(a2),d0
L004098:
	bsr.b	L0040b4
L00409a:
	bcs.b	L0040b2
L00409c:
	move.l	a3,-(sp)
L00409e:
	movea.l	$3a4(a6),a3
L0040a2:
	pea	L0040b0(pc)
L0040a6:
	move.l	$ec(a3),-(sp)
L0040aa:
	movea.l	$4ec(a3),a3
L0040ae:
	rts
L0040b0:
	dc.b	$26
L0040b1:
	dc.b	$5f
L0040b2:
	rts
L0040b4:
	movem.l	a3/a2/a0,-(sp)
L0040b8:
	move.l	a2,d1
L0040ba:
	beq.b	L0040d4
L0040bc:
	movea.l	a2,a0
L0040be:
	movea.l	$3c(a6),a2
L0040c2:
	movea.l	$40(a6),a3
L0040c6:
	cmpa.l	$0(a2),a0
L0040ca:
	beq.b	L0040de
L0040cc:
	lea	$10(a2),a2
L0040d0:
	cmpa.l	a3,a2
L0040d2:
	bcs.b	L0040c6
L0040d4:
	move.w	#$dd,d1
L0040d8:
	ori	#$1,ccr
L0040dc:
	bra.b	L0040e4
L0040de:
	movea.l	$8(sp),a3
L0040e2:
	bsr.b	L00410e
L0040e4:
	movem.l	(sp)+,a0/a2/a3
L0040e8:
	rts
L0040ea:
	movem.l	a2/a0/d1,-(sp)
L0040ee:
	movea.l	d0,a2
L0040f0:
	moveq	#$0,d0
L0040f2:
	bsr.b	L0040fc
L0040f4:
	movem.l	(sp)+,d1/a0/a2
L0040f8:
	scs	d0
L0040fa:
	rts
L0040fc:
	movea.l	$0(a2),a0
L004100:
	bsr.b	L00413c
L004102:
	tst.l	$0(a2)
L004106:
	beq.b	L00410c
L004108:
	ori	#$1,ccr
L00410c:
	rts
L00410e:
	move.l	d2,-(sp)
L004110:
	moveq	#$5,d2
L004112:
	exg	a0,a2
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004114:
	dc.w	$6100
	dc.w	L003072-*
L004118:
	exg	a2,a0
L00411a:
	movem.l	(sp)+,d2
L00411e:
	bcs.b	L00410c
L004120:
	subq.w	#$1,$c(a2)
L004124:
	bhi.b	L00410c
L004126:
	bcs.b	L004138
L004128:
	btst.b	#$6,$14(a0)
L00412e:
	beq.b	L004138
L004130:
	btst.b	#$2,$2e(a6)
L004136:
	beq.b	L00410c
L004138:
	clr.w	$c(a2)
L00413c:
	movem.l	a3/a2/a1/a0/d2/d0,-(sp)
L004140:
	cmpi.b	#$d,$12(a0)
L004146:
	bcs.b	L00416e
L004148:
	move.l	a2,-(sp)
L00414a:
	move.l	a3,-(sp)
L00414c:
	movea.l	$3a4(a6),a3
L004150:
	pea	L00415e(pc)
L004154:
	move.l	$cc(a3),-(sp)
L004158:
	movea.l	$4cc(a3),a3
L00415c:
	rts
L00415e:
	dc.b	$26
L00415f:
	dc.b	$5f
L004160:
	dc.b	$24
L004161:
	dc.b	$5f
L004162:
	dc.b	$64
L004163:
	dc.b	$0a
L004164:
	move.w	#$1,$c(a2)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00416a:
	dc.w	$6000
	dc.w	L00421c-*
L00416e:
	move.l	$40(a6),d2
L004172:
	movea.l	$4(a2),a1
L004176:
	movea.l	$3c(a6),a0
L00417a:
	cmpa.l	$4(a0),a1
L00417e:
	bne.b	L004188
L004180:
	tst.w	$c(a0)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004184:
	dc.w	$6600
	dc.w	L00421c-*
L004188:
	lea	$10(a0),a0
L00418c:
	cmpa.l	d2,a0
L00418e:
	bcs.b	L00417a
L004190:
	movea.l	$8(sp),a0
L004194:
	cmpi.b	#$1,$12(a0)
L00419a:
	beq.b	L0041ac
L00419c:
	cmpi.b	#$b,$12(a0)
L0041a2:
	beq.b	L0041ac
L0041a4:
	cmpi.b	#$b,$12(a0)
L0041aa:
	bne.b	L0041e4
L0041ac:
	movem.l	a2/a1,-(sp)
L0041b0:
	movea.l	$44(a6),a1
L0041b4:
	move.w	(a1),d0
L0041b6:
	subq.w	#$2,d0
L0041b8:
	lea	$4(a1),a1
L0041bc:
	move.l	(a1)+,d1
L0041be:
	beq.b	L0041d8
L0041c0:
	movea.l	d1,a2
L0041c2:
	cmpa.l	$38(a2),a0
L0041c6:
	beq.b	L0041de
L0041c8:
	lea	$c8(a2),a2
L0041cc:
	move.w	#$e,d1
L0041d0:
	cmpa.l	-(a2),a0
L0041d2:
	dbeq	d1,L0041d0
L0041d6:
	beq.b	L0041de
L0041d8:
	dbf	d0,L0041bc
L0041dc:
	moveq	#$1,d0
L0041de:
	movem.l	(sp)+,a1/a2
L0041e2:
	beq.b	L004164
L0041e4:
	movea.l	$3c(a6),a0
L0041e8:
	move.l	$8(a2),d0
L0041ec:
	move.l	$4(a2),d1
L0041f0:
	cmpa.l	$4(a0),a1
L0041f4:
	bne.b	L00420e
L0041f6:
	movea.l	$0(a0),a3
L0041fa:
	clr.w	(a3)
L0041fc:
	clr.l	$0(a0)
L004200:
	clr.l	$4(a0)
L004204:
	cmp.l	$8(a0),d0
L004208:
	bls.b	L00420e
L00420a:
	move.l	$8(a0),d0
L00420e:
	lea	$10(a0),a0
L004212:
	cmpa.l	d2,a0
L004214:
	bcs.b	L0041f0
L004216:
	movea.l	d1,a2
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004218:
	dc.w	$6100
	dc.w	Q9_dealloc_tail_131c-*
L00421c:
	movem.l	(sp)+,d0/d2/a0/a1/a2/a3
L004220:
	rts
L004222:
	trapf.l	#$0
L004228:
	addq.l	#$1,$3ac(a4)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00422c:
	dc.w	$6100
	dc.w	L0027d4-*
L004230:
	bcs.b	L00424c
L004232:
	movea.l	a0,a1
L004234:
	movea.l	$0(a2),a0
L004238:
	move.l	$4(a0),d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00423c:
	dc.w	$6100
	dc.w	L00410e-*
L004240:
	bcs.b	L00424c
L004242:
	move.l	a1,$20(a5)
L004246:
	subq.l	#$1,$3ac(a4)
L00424a:
	rts
L00424c:
	subq.l	#$1,$3ac(a4)
L004250:
	ori	#$1,ccr
L004254:
	rts
L004256:
	trapf
L004258:
	movem.l	d4/d3,-(sp)
L00425c:
	moveq	#$0,d3
L00425e:
	bsr.b	L00429a
L004260:
	move	sr,$4(sp)
L004264:
	bcc.b	L00426c
L004266:
	cmpi.w	#$e7,d1
L00426a:
	bne.b	L004270
L00426c:
	move.l	a2,$28(a5)
L004270:
	move.l	(sp)+,d3
L004272:
	move	$0(sp),ccr
L004276:
	addq.l	#$4,sp
L004278:
	rts
L00427a:
	movem.l	a4/a2/a0/d3,-(sp)
L00427e:
	clr.l	-(sp)
L004280:
	movea.l	d0,a0
L004282:
	move.l	$18(sp),d0
L004286:
	movea.l	$50(a6),a4
L00428a:
	moveq	#$0,d3
L00428c:
	bsr.b	L00429a
L00428e:
	bcc.b	L004294
L004290:
	move.w	d1,$2(sp)
L004294:
	movem.l	(sp)+,d0/d3/a0/a2/a4
L004298:
	rts
L00429a:
	movem.l	a1/a0/d3/d2/d1/d0,-(sp)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00429e:
	dc.w	$6100
	dc.w	L004410-*
L0042a2:
	bcs.b	L0042ec
L0042a4:
	move.w	$12(a0),d0
L0042a8:
	move.b	$15(a0),d2
L0042ac:
	adda.l	$c(a0),a0
L0042b0:
	addq.l	#$1,$3ac(a4)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0042b4:
	dc.w	$6100
	dc.w	L0027d4-*
L0042b8:
	bcs.b	L004304
L0042ba:
	cmp.b	d2,d1
L0042bc:
	bcs.b	L004330
L0042be:
	bhi.b	L0042e4
L0042c0:
	btst.b	#$2,$2e(a6)
L0042c6:
	bne.b	L0042e4
L0042c8:
	tst.w	$c(a2)
L0042cc:
	bne.b	L0042e4
L0042ce:
	move.l	(sp),d0
L0042d0:
	cmp.l	$4(a2),d0
L0042d4:
	beq.b	L0042e4
L0042d6:
	cmpi.w	#$4afc,$0(a6)
L0042dc:
	bne.b	L0042e4
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0042de:
	dc.w	$6100
	dc.w	L0040fc-*
L0042e2:
	bcc.b	L004330
L0042e4:
	subq.l	#$1,$3ac(a4)
L0042e8:
	move.w	#$e7,d1
L0042ec:
	move.w	d1,$6(sp)
L0042f0:
	movem.l	(sp)+,d0/d1/d2/d3/a0/a1
L0042f4:
	ori	#$1,ccr
L0042f8:
	rts
L0042fa:
	subq.l	#$1,$3ac(a4)
L0042fe:
	move.w	#$ce,d1
L004302:
	bra.b	L0042ec
L004304:
	cmpa.l	#$0,a2
L00430a:
	bne.b	L004330
L00430c:
	btst.b	#$0,$39(a6)
L004312:
	bne.b	L0042fa
L004314:
	move.l	$40(a6),d0
L004318:
	movea.l	$3c(a6),a0
L00431c:
	sub.l	a0,d0
L00431e:
	move.l	d0,d1
L004320:
	lsr.l	#$2,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004322:
	dc.w	$6100
	dc.w	L0043b0-*
L004326:
	bcs.b	L0042fa
L004328:
	move.l	a0,$3c(a6)
L00432c:
	move.l	a1,$40(a6)
L004330:
	clr.w	$c(a2)
L004334:
	move.l	(sp),$4(a2)
L004338:
	move.l	$4(sp),$8(a2)
L00433e:
	movea.l	$10(sp),a0
L004342:
	move.l	a0,$0(a2)
L004346:
	moveq	#$2e,d1
L004348:
	lsr.w	#$1,d1
L00434a:
	moveq	#$0,d0
L00434c:
	add.w	(a0)+,d0
L00434e:
	ror.w	d0,d0
L004350:
	dbf	d1,L00434c
L004354:
	move.w	d0,$e(a2)
L004358:
	subq.l	#$1,$3ac(a4)
L00435c:
	move.b	$3e0(a6),d1
L004360:
	btst.l	#$0,d1
L004364:
	beq.b	L00438e
L004366:
	btst.l	#$1,d1
L00436a:
	beq.b	L00438e
L00436c:
	btst.l	#$2,d1
L004370:
	beq.b	L004378
L004372:
	btst.l	#$3,d1
L004376:
	bne.b	L0043a6
L004378:
	move	sr,d1
L00437a:
	ori	#$700,sr
L00437e:
	movec	cacr,d0
L004382:
	ori.w	#$808,d0
L004386:
	movec	d0,cacr
L00438a:
	move	d1,sr
L00438c:
	bra.b	L0043a6
L00438e:
	moveq	#$44,d0
L004390:
	move.l	a3,-(sp)
L004392:
	movea.l	$3a4(a6),a3
L004396:
	pea	L0043a4(pc)
L00439a:
	move.l	$168(a3),-(sp)
L00439e:
	movea.l	$568(a3),a3
L0043a2:
	rts
L0043a4:
	dc.b	$26
L0043a5:
	dc.b	$5f
L0043a6:
	andi	#-$2,ccr
L0043aa:
	movem.l	(sp)+,d0/d1/d2/d3/a0/a1
L0043ae:
	rts
L0043b0:
	movem.l	d3/d2,-(sp)
L0043b4:
	move.l	d0,d2
L0043b6:
	bne.b	L0043be
L0043b8:
	move.l	#$80,d0
L0043be:
	lsl.l	#$1,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0043c0:
	dc.w	$6100
	dc.w	L0012b4-*
L0043c4:
	bcs.b	L00440a
L0043c6:
	move.l	a3,-(sp)
L0043c8:
	movea.l	$3a4(a6),a3
L0043cc:
	pea	L0043da(pc)
L0043d0:
	move.l	$e0(a3),-(sp)
L0043d4:
	movea.l	$4e0(a3),a3
L0043d8:
	rts
L0043da:
	movea.l	(sp)+,a3
L0043dc:
	lea	$0(a2,d2.l*1),a1
L0043e0:
	move.l	d0,d1
L0043e2:
	sub.l	d2,d1
L0043e4:
	lsr.l	#$4,d1
L0043e6:
	subq.l	#$1,d1
L0043e8:
	moveq	#$0,d3
L0043ea:
	move.l	d3,(a1)+
L0043ec:
	move.l	d3,(a1)+
L0043ee:
	move.l	d3,(a1)+
L0043f0:
	move.l	d3,(a1)+
L0043f2:
	dbf	d1,L0043ea
L0043f6:
	addq.w	#$1,d1
L0043f8:
	subq.l	#$1,d1
L0043fa:
	bcc.b	L0043ea
L0043fc:
	exg	a0,a2
L0043fe:
	exg	d0,d2
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004400:
	dc.w	$6100
	dc.w	Q9_dealloc_tail_131c-*
L004404:
	lea	$0(a0,d0.l*1),a2
L004408:
	move.l	d2,d0
L00440a:
	movem.l	(sp)+,d2/d3
L00440e:
	rts
L004410:
	move.l	a0,-(sp)
L004412:
	cmpi.w	#$4afc,(a0)
L004416:
	bne.b	L004478
L004418:
	cmpi.w	#$1,$2(a0)
L00441e:
	bne.b	L004436
L004420:
	moveq	#$2e,d1
L004422:
	lsr.w	#$1,d1
L004424:
	moveq	#-$1,d2
L004426:
	move.w	(a0)+,d0
L004428:
	eor.w	d0,d2
L00442a:
	dbf	d1,L004426
L00442e:
	beq.b	L00443c
L004430:
	move.w	#$ec,d1
L004434:
	bra.b	L00447c
L004436:
	move.w	#$ac,d1
L00443a:
	bra.b	L00447c
L00443c:
	cmpi.w	#$6f6b,$0(a6)
L004442:
	beq.b	L004462
L004444:
	movea.l	(sp),a0
L004446:
	tst.l	d3
L004448:
	bne.b	L004462
L00444a:
	moveq	#-$1,d1
L00444c:
	move.l	$4(a0),d3
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004450:
	dc.w	$6100
	dc.w	L001ba4-*
L004454:
	cmpi.l	#$800fe3,d1
L00445a:
	beq.b	L004462
L00445c:
	move.w	#$e8,d1
L004460:
	bra.b	L00447c
L004462:
	movea.l	(sp),a0
L004464:
	adda.l	$c(a0),a0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004468:
	dc.w	$6100
	dc.w	L0032fa-*
L00446c:
	bcs.b	L00447c
L00446e:
	tst.b	d0
L004470:
	beq.b	L004480
L004472:
	move.w	#$eb,d1
L004476:
	bra.b	L00447c
L004478:
	move.w	#$cd,d1
L00447c:
	ori	#$1,ccr
L004480:
	movea.l	(sp)+,a0
L004482:
	rts
L004484:
	trapf.w	#$0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004488:
	dc.w	$6100
	dc.w	L003984-*
L00448c:
	bclr.b	#$7,$371(a4)
L004492:
	tst.w	$26(a4)
L004496:
	beq.b	L00449e
L004498:
	bset.b	#$7,$371(a4)
L00449e:
	move.w	$6(a4),d0
L0044a2:
	beq.b	L00450e
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0044a4:
	dc.w	$6100
	dc.w	Q9_proc_id_lookup_2cee-*
L0044a8:
	bcs.b	L00450e
L0044aa:
	btst.b	#$0,$1c(a1)
L0044b0:
	beq.b	L0044c0
L0044b2:
	bclr.b	#$3,$1c(a4)
L0044b8:
	movea.l	a4,a0
L0044ba:
	bsr.b	Q9_parent_notify_4518
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0044bc:
	dc.w	$6000
	dc.w	Q9_proc_id_free_wrap_1e18-*
L0044c0:
	move.w	$4(a1),d0
L0044c4:
	bne.b	L0044a4
L0044c6:
	clr.l	$0(a5)
L0044ca:
	clr.l	$4(a5)
L0044ce:
	move	sr,d1
L0044d0:
	ori	#$700,sr
L0044d4:
	bclr.b	#$7,$371(a4)
L0044da:
	bne.b	L00450a
L0044dc:
	bclr.b	#$3,$1c(a4)
L0044e2:
	bne.b	L004506
L0044e4:
	lea	$38c(a6),a2
L0044e8:
	movea.l	$34(a2),a0
L0044ec:
	move.l	a0,$34(a4)
L0044f0:
	move.l	a2,$30(a4)
L0044f4:
	move.l	a4,$34(a2)
L0044f8:
	move.l	a4,$30(a0)
L0044fc:
	move.b	#$77,$20(a4)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004502:
	dc.w	$6000
	dc.w	L003b64-*
L004506:
	move	d1,sr
L004508:
	bra.b	L00449e
L00450a:
	move	d1,sr
L00450c:
	rts
L00450e:
	move.w	#$e2,d1
L004512:
	ori	#$1,ccr
L004516:
	rts
* Eltern-Benachrichtigung beim Kindprozess-Tod, weckt einen wartenden Elternprozess (SIGCHLD/wait-artig).
Q9_parent_notify_4518:
	clr.w	$0(a5)
L00451c:
	move.w	$0(a1),$2(a5)
L004522:
	clr.w	$4(a5)
L004526:
	move.w	$26(a1),$6(a5)
L00452c:
	movea.l	a1,a2
L00452e:
	lea	$2(a0),a1
L004532:
	move.l	a0,-(sp)
L004534:
	bra.b	L00453c
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004536:
	dc.w	$6100
	dc.w	Q9_proc_id_lookup_2cee-*
L00453a:
	bcs.b	L00454c
L00453c:
	move.w	$4(a1),d0
L004540:
	cmp.w	$0(a2),d0
L004544:
	bne.b	L004536
L004546:
	move.w	$4(a2),$4(a1)
L00454c:
	movea.l	(sp)+,a0
L00454e:
	rts
L004550:
	movem.l	a1/a0/d7/d6/d5/d4/d3/d2/d1,-(sp)
L004554:
	moveq	#$0,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004556:
	dc.w	$6100
	dc.w	Q9_irq_mask_10e6-*
L00455a:
	move.l	d0,d5
L00455c:
	moveq	#$0,d0
L00455e:
	move.l	d5,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004560:
	dc.w	$6100
	dc.w	Q9_irq_unmask_10f2-*
L004564:
	move.l	$77c(a6),d3
L004568:
	moveq	#$0,d6
L00456a:
	moveq	#$0,d4
L00456c:
	moveq	#$1,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00456e:
	dc.w	$6100
	dc.w	Q9_irq_mask_10e6-*
L004572:
	movea.l	$3b4(a6),a0
L004576:
	lea	$384(a6),a1
L00457a:
	cmpa.l	a1,a0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00457c:
	dc.w	$6700
	dc.w	L004602-*
L004580:
	move.w	#$4000,d0
L004584:
	and.w	$1c(a0),d0
L004588:
	moveq	#$0,d1
L00458a:
	move.w	d0,d1
L00458c:
	tst.l	d1
L00458e:
	beq.b	L004602
L004590:
	move.l	$54(a6),d1
L004594:
	movea.l	$8(a0),a1
L004598:
	move.l	(a1),d0
L00459a:
	cmp.l	d0,d1
L00459c:
	bcs.b	L0045ae
L00459e:
	move.l	d1,d7
L0045a0:
	sub.l	d0,d7
L0045a2:
	cmpi.l	#$fffffff,d7
L0045a8:
	bls.b	L0045bc
L0045aa:
	neg.l	d7
L0045ac:
	bra.b	L0045c0
L0045ae:
	sub.l	d1,d0
L0045b0:
	cmpi.l	#$fffffff,d0
L0045b6:
	bcc.b	L0045bc
L0045b8:
	move.l	d0,d7
L0045ba:
	bra.b	L0045c0
L0045bc:
	moveq	#$0,d0
L0045be:
	move.l	d0,d7
L0045c0:
	tst.l	d7
L0045c2:
	bne.b	L0045e2
L0045c4:
	andi.w	#-$4001,$1c(a0)
L0045ca:
	moveq	#$0,d0
L0045cc:
	move.l	d5,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0045ce:
	dc.w	$6100
	dc.w	Q9_irq_unmask_10f2-*
L0045d2:
	move.l	a0,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0045d4:
	dc.w	$6100
	dc.w	L001820-*
L0045d8:
	moveq	#$1,d0
L0045da:
	move.l	d0,d6
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0045dc:
	dc.w	$6100
	dc.w	Q9_irq_mask_10e6-*
L0045e0:
	bra.b	L004602
L0045e2:
	moveq	#$1,d0
L0045e4:
	move.l	d0,d4
L0045e6:
	move.l	$77c(a6),d0
L0045ea:
	bne.b	L0045fa
L0045ec:
	tst.l	d3
L0045ee:
	beq.b	L0045f4
L0045f0:
	moveq	#$1,d1
L0045f2:
	move.l	d1,d6
L0045f4:
	move.l	d7,$77c(a6)
L0045f8:
	bra.b	L004602
L0045fa:
	cmp.l	d7,d0
L0045fc:
	bls.b	L004602
L0045fe:
	move.l	d7,$77c(a6)
L004602:
	movea.l	$788(a6),a0
L004606:
	lea	$77c(a6),a1
L00460a:
	cmpa.l	a1,a0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00460c:
	dc.w	$6700
	dc.w	L004708-*
L004610:
	move.l	$54(a6),d1
L004614:
	move.l	$20(a0),d0
L004618:
	cmp.l	d0,d1
L00461a:
	bcs.b	L00462c
L00461c:
	move.l	d1,d7
L00461e:
	sub.l	d0,d7
L004620:
	cmpi.l	#$fffffff,d7
L004626:
	bls.b	L00463a
L004628:
	neg.l	d7
L00462a:
	bra.b	L00463e
L00462c:
	sub.l	d1,d0
L00462e:
	cmpi.l	#$fffffff,d0
L004634:
	bcc.b	L00463a
L004636:
	move.l	d0,d7
L004638:
	bra.b	L00463e
L00463a:
	moveq	#$0,d0
L00463c:
	move.l	d0,d7
L00463e:
	tst.l	d7
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004640:
	dc.w	$6600
	dc.w	L0046e8-*
L004644:
	move.l	$c(a0),([$10,a0],$c)
L004652:
	move.l	$10(a0),([$c,a0],$10)
L004660:
	move.l	a0,$10(a0)
L004664:
	move.l	a0,$c(a0)
L004668:
	moveq	#$5,d0
L00466a:
	and.b	$28(a0),d0
L00466e:
	moveq	#$0,d1
L004670:
	move.b	d0,d1
L004672:
	tst.l	d1
L004674:
	bne.b	L00468a
L004676:
	bset.b	#$1,$28(a0)
L00467c:
	moveq	#$0,d0
L00467e:
	move.l	d5,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004680:
	dc.w	$6100
	dc.w	Q9_irq_unmask_10f2-*
L004684:
	move.l	a0,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004686:
	dc.w	$6100
	dc.w	L001060-*
L00468a:
	moveq	#$4,d0
L00468c:
	and.b	$28(a0),d0
L004690:
	moveq	#$0,d1
L004692:
	move.b	d0,d1
L004694:
	tst.l	d1
L004696:
	beq.b	L0046a0
* Rohbytes statt Instruktion (andi.b #-0x3,(0x28,A0)) -- siehe FORCE_RAW_BYTES im Konverter
L004698:
	dc.b	$02,$28,$00,$fd,$00,$28
L00469e:
	bra.b	L0046de
L0046a0:
	moveq	#$0,d0
L0046a2:
	move.l	d5,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0046a4:
	dc.w	$6100
	dc.w	Q9_irq_unmask_10f2-*
L0046a8:
	tst.l	$1c(a0)
L0046ac:
	beq.b	L0046bc
L0046ae:
	moveq	#$1,d0
L0046b0:
	and.b	$28(a0),d0
L0046b4:
	moveq	#$0,d1
L0046b6:
	move.b	d0,d1
L0046b8:
	tst.l	d1
L0046ba:
	beq.b	L0046c4
L0046bc:
	move.l	a0,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0046be:
	dc.w	$6100
	dc.w	L001408-*
L0046c2:
	bra.b	L0046de
L0046c4:
	move.l	$1c(a0),d0
L0046c8:
	add.l	d0,$20(a0)
* Rohbytes statt Instruktion (andi.b #-0x3,(0x28,A0)) -- siehe FORCE_RAW_BYTES im Konverter
L0046cc:
	dc.b	$02,$28,$00,$fd,$00,$28
L0046d2:
	lea	$77c(a6),a1
L0046d6:
	move.l	a1,d0
L0046d8:
	move.l	a0,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0046da:
	dc.w	$6100
	dc.w	L00161a-*
L0046de:
	moveq	#$1,d0
L0046e0:
	move.l	d0,d6
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0046e2:
	dc.w	$6100
	dc.w	Q9_irq_mask_10e6-*
L0046e6:
	bra.b	L004708
L0046e8:
	moveq	#$1,d0
L0046ea:
	move.l	d0,d4
L0046ec:
	move.l	$77c(a6),d0
L0046f0:
	bne.b	L004700
L0046f2:
	tst.l	d3
L0046f4:
	beq.b	L0046fa
L0046f6:
	moveq	#$1,d1
L0046f8:
	move.l	d1,d6
L0046fa:
	move.l	d7,$77c(a6)
L0046fe:
	bra.b	L004708
L004700:
	cmp.l	d7,d0
L004702:
	bls.b	L004708
L004704:
	move.l	d7,$77c(a6)
L004708:
	movea.l	$780(a6),a0
L00470c:
	lea	$774(a6),a1
L004710:
	cmpa.l	a1,a0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004712:
	dc.w	$6700
	dc.w	L0047dc-*
L004716:
	move.l	$24(a0),d7
L00471a:
	sub.l	$30(a6),d7
L00471e:
	tst.l	d7
L004720:
	blt.b	L00473c
L004722:
	tst.l	d7
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004724:
	dc.w	$6600
	dc.w	L0047a8-*
L004728:
	move.l	#$15180,d0
L00472e:
	sub.l	$34(a6),d0
L004732:
	move.l	$20(a0),d1
L004736:
	sub.l	d0,d1
L004738:
	move.l	d1,d2
L00473a:
	bgt.b	L0047a8
L00473c:
	move.l	$c(a0),([$10,a0],$c)
L00474a:
	move.l	$10(a0),([$c,a0],$10)
L004758:
	move.l	a0,$10(a0)
L00475c:
	move.l	a0,$c(a0)
L004760:
	moveq	#$5,d0
L004762:
	and.b	$28(a0),d0
L004766:
	moveq	#$0,d1
L004768:
	move.b	d0,d1
L00476a:
	tst.l	d1
L00476c:
	bne.b	L004782
L00476e:
	bset.b	#$1,$28(a0)
L004774:
	moveq	#$0,d0
L004776:
	move.l	d5,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004778:
	dc.w	$6100
	dc.w	Q9_irq_unmask_10f2-*
L00477c:
	move.l	a0,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00477e:
	dc.w	$6100
	dc.w	L001060-*
L004782:
	moveq	#$4,d0
L004784:
	and.b	$28(a0),d0
L004788:
	moveq	#$0,d1
L00478a:
	move.b	d0,d1
L00478c:
	tst.l	d1
L00478e:
	beq.b	L004798
* Rohbytes statt Instruktion (andi.b #-0x3,(0x28,A0)) -- siehe FORCE_RAW_BYTES im Konverter
L004790:
	dc.b	$02,$28,$00,$fd,$00,$28
L004796:
	bra.b	L00479e
L004798:
	move.l	a0,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00479a:
	dc.w	$6100
	dc.w	L001408-*
L00479e:
	moveq	#$1,d0
L0047a0:
	move.l	d0,d6
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0047a2:
	dc.w	$6100
	dc.w	Q9_irq_mask_10e6-*
L0047a6:
	bra.b	L0047dc
L0047a8:
	moveq	#$1,d0
L0047aa:
	move.l	d0,d4
L0047ac:
	tst.l	d7
L0047ae:
	ble.b	L0047b4
L0047b0:
	move.l	$34(a6),d2
L0047b4:
	move.l	d2,d1
L0047b6:
	moveq	#$0,d0
L0047b8:
	move.w	$28(a6),d0
L0047bc:
	mulu.l	d0,d1
L0047c0:
	move.l	$77c(a6),d0
L0047c4:
	bne.b	L0047d4
L0047c6:
	tst.l	d3
L0047c8:
	beq.b	L0047ce
L0047ca:
	moveq	#$1,d7
L0047cc:
	move.l	d7,d6
L0047ce:
	move.l	d1,$77c(a6)
L0047d2:
	bra.b	L0047dc
L0047d4:
	cmp.l	d1,d0
L0047d6:
	bls.b	L0047dc
L0047d8:
	move.l	d1,$77c(a6)
L0047dc:
	tst.l	d6
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0047de:
	dc.w	$6600
	dc.w	L00455c-*
L0047e2:
	tst.l	$77c(a6)
L0047e6:
	bne.b	L0047ee
L0047e8:
	tst.l	d4
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0047ea:
	dc.w	$6600
	dc.w	L00455c-*
L0047ee:
	moveq	#$0,d0
L0047f0:
	movem.l	(sp)+,d1/d2/d3/d4/d5/d6/d7/a0/a1
L0047f4:
	rts
L0047f6:
	trapf
L0047f8:
	move.l	d7,-(sp)
L0047fa:
	move.l	d0,d7
L0047fc:
	cmp.l	d1,d7
L0047fe:
	bcs.b	L004810
L004800:
	move.l	d7,d0
L004802:
	sub.l	d1,d0
L004804:
	cmpi.l	#$fffffff,d0
L00480a:
	bls.b	L00481e
L00480c:
	neg.l	d0
L00480e:
	bra.b	L004820
L004810:
	sub.l	d7,d1
L004812:
	cmpi.l	#$fffffff,d1
L004818:
	bcc.b	L00481e
L00481a:
	move.l	d1,d0
L00481c:
	bra.b	L004820
L00481e:
	moveq	#$0,d0
L004820:
	move.l	(sp)+,d7
L004822:
	rts
L004824:
	trapf.w	#$0
L004828:
	movem.l	a2/a1/a0/d0,-(sp)
L00482c:
	move	sr,$0(sp)
L004830:
	movea.l	d0,a1
L004832:
	move.l	d1,d2
L004834:
	btst.b	#$5,$40(a5)
L00483a:
	bne.b	L00485c
L00483c:
	moveq	#$24,d0
L00483e:
	moveq	#$3,d1
L004840:
	movea.l	a1,a2
L004842:
	move.l	a3,-(sp)
L004844:
	movea.l	$3a4(a6),a3
L004848:
	pea	L004856(pc)
L00484c:
	move.l	$160(a3),-(sp)
L004850:
	movea.l	$560(a3),a3
L004854:
	rts
L004856:
	dc.b	$26
L004857:
	dc.b	$5f
L004858:
	dc.b	$65
L004859:
	dc.b	$00
L00485a:
	dc.b	$00
L00485b:
	dc.b	$c4
L00485c:
	ori	#$700,sr
L004860:
	subq.l	#$1,d2
L004862:
	beq.b	L0048a2
L004864:
	subq.l	#$1,d2
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004866:
	dc.w	$6600
	dc.w	L004924-*
L00486a:
	tst.l	$10(a1)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00486e:
	dc.w	$6700
	dc.w	L004918-*
L004872:
	lea	-$28(a1),a0
L004876:
	cmpa.l	$30(a0),a0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00487a:
	dc.w	$6700
	dc.w	L004918-*
L00487e:
	movea.l	$30(a0),a0
L004882:
	move.w	$0(a0),$16(a1)
L004888:
	subq.l	#$1,$10(a1)
L00488c:
	movea.l	$8(a0),a2
L004890:
	move.w	#$0,$6(a2)
L004896:
	move.b	#$0,$41(a2)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00489c:
	dc.w	$6100
	dc.w	Q9_scheduler_183a-*
L0048a0:
	bra.b	L004918
L0048a2:
	tst.b	$0(a1)
L0048a6:
	beq.b	L004918
L0048a8:
	lea	-$28(a1),a0
L0048ac:
	tst.l	$30(a0)
L0048b0:
	bne.b	L0048ba
L0048b2:
	move.l	a0,$30(a0)
L0048b6:
	move.l	a0,$34(a0)
L0048ba:
	movea.l	$34(a0),a2
L0048be:
	move.l	a4,$30(a2)
L0048c2:
	move.l	a2,$34(a4)
L0048c6:
	move.l	a0,$30(a4)
L0048ca:
	move.l	a4,$34(a0)
L0048ce:
	addq.l	#$1,$10(a1)
L0048d2:
	move.b	#$70,$20(a4)
L0048d8:
	lea	L000558(pc),a2
L0048dc:
	cmpa.l	$10(sp),a2
L0048e0:
	bne.b	L004904
L0048e2:
	tst.l	$2ac(a4)
L0048e6:
	bne.b	L004904
L0048e8:
	move.w	#$b1,$6(a5)
L0048ee:
	move.b	#$1,$41(a5)
L0048f4:
	move.l	a5,$8(a4)
L0048f8:
	move.l	d5,$140(a4)
L0048fc:
	move.l	d6,$144(a4)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004900:
	dc.w	$6000
	dc.w	Q9_reschedule_trampolin_3140-*
L004904:
	move.w	$0(sp),d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004908:
	dc.w	$6100
	dc.w	L003b64-*
L00490c:
	move.w	$16(a1),d0
L004910:
	beq.b	L00492a
L004912:
	cmp.w	$0(a4),d0
L004916:
	bne.b	L00492a
L004918:
	move	$0(sp),sr
L00491c:
	moveq	#$0,d1
L00491e:
	movem.l	(sp)+,d0/a0/a1/a2
L004922:
	rts
L004924:
	move.w	#$e1,d1
L004928:
	bra.b	L00492e
L00492a:
	move.w	#$b1,d1
L00492e:
	move	$0(sp),sr
L004932:
	ori	#$1,ccr
L004936:
	bra.b	L00491e
L004938:
	dc.b	$63,$61,$6e,$27,$74,$20,$61,$6c,$6c,$6f,$63,$61,$74,$65,$20,$73
	dc.b	$79,$73,$74,$65,$6d,$20,$74,$61,$62,$6c,$65,$73,$00
L004955:
	dc.b	$42,$61,$64,$20,$6d,$65,$6d,$6f,$72,$79,$20,$6c,$69,$73,$74,$20
	dc.b	$69,$6e,$20,$27,$69,$6e,$69,$74,$27,$20,$6d,$6f,$64,$75,$6c,$65
	dc.b	$2e,$00
L004977:
	dc.b	$00
* Initialisiert feste System-Global-Konstanten (Speicher-Alignment=16, Groessenkonstante=256) -- kein Allocator.
Q9_const_init_4978:
	moveq	#$10,d0
L00497a:
	move.l	d0,$70(a6)
L00497e:
	move.l	#$100,$7c(a6)
L004986:
	move.b	#$1,$8f5(a6)
L00498c:
	rts
L00498e:
	movem.l	a2/a1/a0/d7/d6,-(sp)
L004992:
	movea.l	d0,a0
L004994:
	move.l	d1,d6
L004996:
	move.l	$70(a6),d7
L00499a:
	move.l	d7,d0
L00499c:
	add.l	d6,d0
L00499e:
	subq.l	#$1,d0
L0049a0:
	neg.l	d7
L0049a2:
	and.l	d0,d7
L0049a4:
	move.l	d7,d6
L0049a6:
	movea.l	a0,a1
L0049a8:
	movea.l	(a1),a0
L0049aa:
	bra.b	L0049b0
L0049ac:
	movea.l	a0,a1
L0049ae:
	movea.l	(a0),a0
L0049b0:
	tst.l	a0
L0049b2:
	beq.b	L0049ba
L0049b4:
	cmp.l	$4(a0),d6
L0049b8:
	bhi.b	L0049ac
L0049ba:
	tst.l	a0
L0049bc:
	bne.b	L0049c8
L0049be:
	lea	L004938(pc),a2
L0049c2:
	move.l	a2,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0049c4:
	dc.w	$6100
	dc.w	L0007f4-*
L0049c8:
	cmp.l	$4(a0),d6
L0049cc:
	bne.b	L0049d2
L0049ce:
	move.l	(a0),(a1)
L0049d0:
	bra.b	L0049e8
L0049d2:
	move.l	d6,d0
L0049d4:
	add.l	(a1),d0
L0049d6:
	move.l	d0,(a1)
L0049d8:
	movea.l	(a1),a2
L0049da:
	move.l	(a0),(a2)
L0049dc:
	movea.l	(a1),a2
L0049de:
	move.l	$4(a0),d0
L0049e2:
	sub.l	d6,d0
L0049e4:
	move.l	d0,$4(a2)
L0049e8:
	clr.l	-(sp)
L0049ea:
	move.l	d6,d0
L0049ec:
	move.l	a0,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0049ee:
	dc.w	$6100
	dc.w	L0010ba-*
L0049f2:
	move.l	a0,d0
L0049f4:
	addq.l	#$4,sp
L0049f6:
	movem.l	(sp)+,d6/d7/a0/a1/a2
L0049fa:
	rts
L0049fc:
	link.w	a5,#-$4
L004a00:
	movem.l	a4/a3/a2/a1/a0/d7/d6/d5/d4/d3/d2,-(sp)
L004a04:
	movea.l	d0,a2
L004a06:
	movea.l	$8(a5),a4
L004a0a:
	move.l	a2,d6
L004a0c:
	clr.l	-$4(a5)
L004a10:
	move.l	d1,d3
L004a12:
	moveq	#$0,d4
L004a14:
	move.l	a2,d7
L004a16:
	add.l	d1,d7
L004a18:
	cmp.l	$3dc(a6),d7
L004a1c:
	bls.b	L004a22
L004a1e:
	move.l	d7,$3dc(a6)
L004a22:
	move.l	$7c(a6),d0
L004a26:
	move.l	a2,d1
L004a28:
	move.l	d0,d2
L004a2a:
	add.l	d1,d2
L004a2c:
	subq.l	#$1,d2
L004a2e:
	neg.l	d0
L004a30:
	and.l	d2,d0
L004a32:
	movea.l	d0,a3
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004a34:
	dc.w	$6000
	dc.w	L004af2-*
L004a38:
	moveq	#$0,d0
L004a3a:
	move.w	(a2)+,d0
L004a3c:
	cmpi.l	#$4afc,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004a42:
	dc.w	$6600
	dc.w	L004ad4-*
L004a46:
	subq.l	#$2,a2
L004a48:
	move.l	$4(a2),d0
L004a4c:
	tst.l	a4
L004a4e:
	beq.b	L004a66
L004a50:
	tst.l	-$4(a5)
L004a54:
	beq.b	L004a62
L004a56:
	move.l	$70(a6),d1
L004a5a:
	subq.l	#$1,d1
L004a5c:
	move.l	a2,d2
L004a5e:
	and.l	d2,d1
L004a60:
	bne.b	L004a64
L004a62:
	move.l	a2,d6
L004a64:
	move.l	d0,d3
L004a66:
	movea.l	a2,a1
L004a68:
	adda.l	d0,a1
L004a6a:
	cmpa.l	d7,a1
L004a6c:
	bhi.b	L004ad0
L004a6e:
	move.l	d6,-(sp)
L004a70:
	move.l	a2,d0
L004a72:
	move.l	d3,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004a74:
	dc.w	$6100
	dc.w	L00427a-*
L004a78:
	tst.l	d0
L004a7a:
	addq.l	#$4,sp
L004a7c:
	beq.b	L004a86
L004a7e:
	cmpi.l	#$e7,d0
L004a84:
	bne.b	L004ad0
L004a86:
	tst.l	-$4(a5)
L004a8a:
	bne.b	L004aa4
L004a8c:
	tst.l	d0
L004a8e:
	bne.b	L004aa4
L004a90:
	pea	-$4(a5)
L004a94:
	moveq	#$0,d0
L004a96:
	move.l	a2,d1
L004a98:
	add.l	$c(a2),d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004a9c:
	dc.w	$6100
	dc.w	L002f82-*
L004aa0:
	addq.l	#$4,sp
L004aa2:
	bra.b	L004aa8
L004aa4:
	moveq	#$1,d0
L004aa6:
	move.l	d0,d4
L004aa8:
	tst.l	a4
L004aaa:
	beq.b	L004aca
L004aac:
	cmpa.l	a3,a2
L004aae:
	bls.b	L004aca
L004ab0:
	tst.l	(a4)
L004ab2:
	bne.b	L004aba
L004ab4:
	movea.l	a4,a0
L004ab6:
	move.l	a3,(a0)
L004ab8:
	bra.b	L004abe
L004aba:
	movea.l	d5,a0
L004abc:
	move.l	a3,(a0)
L004abe:
	clr.l	(a3)
L004ac0:
	move.l	a2,d0
L004ac2:
	sub.l	a3,d0
L004ac4:
	move.l	d0,$4(a3)
L004ac8:
	move.l	a3,d5
L004aca:
	movea.l	a1,a2
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004acc:
	dc.w	$6000
	dc.w	L004a22-*
L004ad0:
	addq.l	#$2,a2
L004ad2:
	bra.b	L004af2
L004ad4:
	tst.l	a4
L004ad6:
	beq.b	L004af2
L004ad8:
	tst.l	-$4(a5)
L004adc:
	beq.b	L004af2
L004ade:
	tst.l	d4
L004ae0:
	beq.b	L004aee
L004ae2:
	move.l	-$4(a5),d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004ae6:
	dc.w	$6100
	dc.w	L001232-*
L004aea:
	moveq	#$0,d0
L004aec:
	move.l	d0,d4
L004aee:
	clr.l	-$4(a5)
L004af2:
	cmpa.l	d7,a2
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004af4:
	dc.w	$6500
	dc.w	L004a38-*
L004af8:
	tst.l	a4
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004afa:
	dc.w	$6700
	dc.w	L00507c-*
L004afe:
	cmpa.l	a3,a2
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004b00:
	dc.w	$6300
	dc.w	L00507c-*
L004b04:
	tst.l	(a4)
L004b06:
	bne.b	L004b0e
L004b08:
	movea.l	a4,a0
L004b0a:
	move.l	a3,(a0)
L004b0c:
	bra.b	L004b12
L004b0e:
	movea.l	d5,a0
L004b10:
	move.l	a3,(a0)
L004b12:
	clr.l	(a3)
L004b14:
	suba.l	a3,a2
L004b16:
	move.l	a2,d0
L004b18:
	move.l	d0,$4(a3)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004b1c:
	dc.w	$6000
	dc.w	L00507c-*
L004b20:
	link.w	a5,#-$4
L004b24:
	movem.l	a4/a3/a2/a1/a0/d7/d6/d5/d4,-(sp)
L004b28:
	move.l	d0,-$4(a5)
L004b2c:
	move.l	d1,d7
L004b2e:
	move.l	$8(a5),d5
L004b32:
	movea.l	$20(a6),a1
L004b36:
	movea.l	a1,a0
L004b38:
	moveq	#$0,d6
L004b3a:
	move.w	$6a(a0),d6
L004b3e:
	tst.l	d6
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004b40:
	dc.w	$6600
	dc.w	L004bf6-*
L004b44:
	lea	-$4(a5),a0
L004b48:
	move.l	a0,d0
L004b4a:
	moveq	#$54,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004b4c:
	dc.w	$6100
	dc.w	L00498e-*
L004b50:
	movea.l	d0,a2
L004b52:
	move.w	#$1,$28(a2)
L004b58:
	move.w	#$1,$24(a2)
L004b5e:
	moveq	#-$1,d0
L004b60:
	move.l	d0,$4(a2)
L004b64:
	move.l	a2,d0
L004b66:
	lea	$3fc(a6),a0
L004b6a:
	move.l	a0,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004b6c:
	dc.w	$6100
	dc.w	Q9_sorted_list_insert_5c7c-*
L004b70:
	lea	$2a(a2),a0
L004b74:
	movea.l	a2,a3
L004b76:
	moveq	#$4,d0
L004b78:
	move.l	(a3)+,(a0)+
L004b7a:
	move.l	(a3)+,(a0)+
L004b7c:
	dbf	d0,L004b78
L004b80:
	move.w	(a3)+,(a0)+
L004b82:
	moveq	#$2a,d1
L004b84:
	move.l	a2,d0
L004b86:
	add.l	d1,d0
L004b88:
	movea.l	$50(a6),a0
L004b8c:
	lea	$390(a0),a0
L004b90:
	move.l	a0,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004b92:
	dc.w	$6100
	dc.w	Q9_sorted_list_insert_5c7c-*
L004b96:
	bra.b	L004be0
L004b98:
	movea.l	-$4(a5),a0
L004b9c:
	movea.l	(a0),a0
L004b9e:
	move.l	-$4(a5),d0
L004ba2:
	cmp.l	$3dc(a6),d0
L004ba6:
	bls.b	L004bb8
L004ba8:
	movea.l	-$4(a5),a3
L004bac:
	move.l	-$4(a5),d0
L004bb0:
	add.l	$4(a3),d0
L004bb4:
	move.l	d0,$3dc(a6)
L004bb8:
	btst.b	#$4,$93f(a6)
L004bbe:
	bne.b	L004bc4
L004bc0:
	moveq	#$1,d0
L004bc2:
	bra.b	L004bc8
L004bc4:
	moveq	#$0,d1
L004bc6:
	move.l	d1,d0
L004bc8:
	move.l	d0,-(sp)
L004bca:
	movea.l	-$4(a5),a3
L004bce:
	move.l	$4(a3),d0
L004bd2:
	move.l	-$4(a5),d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004bd6:
	dc.w	$6100
	dc.w	Q9_mem_free_5a22-*
L004bda:
	move.l	a0,-$4(a5)
L004bde:
	addq.l	#$4,sp
L004be0:
	tst.l	-$4(a5)
L004be4:
	bne.b	L004b98
L004be6:
	move.l	$3dc(a6),$4(a2)
L004bec:
	move.l	$3dc(a6),$2e(a2)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004bf2:
	dc.w	$6000
	dc.w	L004e88-*
L004bf6:
	adda.l	d6,a1
L004bf8:
	movea.l	a1,a0
L004bfa:
	moveq	#$0,d6
L004bfc:
	bra.b	L004c14
L004bfe:
	moveq	#$10,d0
L004c00:
	and.w	$4(a0),d0
L004c04:
	moveq	#$0,d1
L004c06:
	move.w	d0,d1
L004c08:
	tst.l	d1
L004c0a:
	bne.b	L004c10
L004c0c:
	moveq	#$54,d0
L004c0e:
	add.l	d0,d6
L004c10:
	moveq	#$20,d0
L004c12:
	adda.l	d0,a0
L004c14:
	moveq	#$0,d0
L004c16:
	move.w	(a0),d0
L004c18:
	tst.l	d0
L004c1a:
	bne.b	L004bfe
L004c1c:
	moveq	#$0,d0
L004c1e:
	move.w	$2(a0),d0
L004c22:
	tst.l	d0
L004c24:
	bne.b	L004bfe
L004c26:
	lea	-$4(a5),a0
L004c2a:
	move.l	a0,d0
L004c2c:
	move.l	d6,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004c2e:
	dc.w	$6100
	dc.w	L00498e-*
L004c32:
	movea.l	d0,a1
L004c34:
	movea.l	d7,a0
L004c36:
	move.l	a1,$18(a0)
L004c3a:
	move.l	d6,$1c(a0)
L004c3e:
	clr.l	$20(a0)
L004c42:
	moveq	#$4,d4
L004c44:
	clr.l	$6c(a6)
L004c48:
	movea.l	$20(a6),a2
L004c4c:
	moveq	#$0,d0
L004c4e:
	move.w	$6a(a2),d0
L004c52:
	adda.l	d0,a2
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004c54:
	dc.w	$6000
	dc.w	L004e72-*
L004c58:
	moveq	#$10,d0
L004c5a:
	and.w	$4(a2),d0
L004c5e:
	moveq	#$0,d1
L004c60:
	move.w	d0,d1
L004c62:
	tst.l	d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004c64:
	dc.w	$6700
	dc.w	L004d90-*
L004c68:
	movea.l	$8(a2),a0
L004c6c:
	move.l	$c(a2),d0
L004c70:
	sub.l	a0,d0
L004c72:
	cmp.l	$7c(a6),d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004c76:
	dc.w	$6300
	dc.w	L004e6e-*
L004c7a:
	btst.b	#$1,$93f(a6)
L004c80:
	bne.b	L004c98
L004c82:
	clr.l	-(sp)
L004c84:
	move.l	a0,d0
L004c86:
	move.l	$70(a6),d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004c8a:
	dc.w	$6100
	dc.w	L0010f6-*
L004c8e:
	moveq	#$1,d1
L004c90:
	cmp.l	d1,d0
L004c92:
	addq.l	#$4,sp
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004c94:
	dc.w	$6600
	dc.w	L004e6e-*
L004c98:
	movea.l	$8(a2),a3
L004c9c:
	move.w	(a2),$26(a3)
L004ca0:
	move.w	$2(a2),$28(a3)
L004ca6:
	move.w	$4(a2),$24(a3)
L004cac:
	move.l	$8(a2),(a3)
L004cb0:
	move.l	$c(a2),$4(a3)
L004cb6:
	tst.l	$14(a2)
L004cba:
	beq.b	L004cc2
L004cbc:
	movea.l	$14(a2),a0
L004cc0:
	bra.b	L004cc6
L004cc2:
	movea.l	$8(a2),a0
L004cc6:
	move.l	a0,$1c(a3)
L004cca:
	movea.l	d7,a0
L004ccc:
	lea	$0(a0,d4.l*8),a0
L004cd0:
	move.l	a3,(a0)
L004cd2:
	moveq	#$54,d0
L004cd4:
	move.l	d0,$4(a0)
L004cd8:
	addq.l	#$1,d4
L004cda:
	movea.l	d7,a0
L004cdc:
	clr.l	$0(a0,d4.l*8)
L004ce0:
	btst.b	#$1,$93f(a6)
L004ce6:
	beq.b	L004d44
L004ce8:
	move.l	d5,-(sp)
L004cea:
	move.l	a3,d0
L004cec:
	move.l	d7,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004cee:
	dc.w	$6100
	dc.w	L004e90-*
L004cf2:
	movea.l	(a3),a0
L004cf4:
	move.l	$4(a3),d6
L004cf8:
	sub.l	a0,d6
L004cfa:
	moveq	#$2,d0
L004cfc:
	and.w	$4(a2),d0
L004d00:
	moveq	#$0,d1
L004d02:
	move.w	d0,d1
L004d04:
	tst.l	d1
L004d06:
	addq.l	#$4,sp
L004d08:
	beq.b	L004d22
L004d0a:
	btst.b	#$6,$93f(a6)
L004d10:
	bne.b	L004d22
L004d12:
	pea	$4c6f7665.l
L004d18:
	move.l	d6,d0
L004d1a:
	move.l	a0,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004d1c:
	dc.w	$6100
	dc.w	L0010ba-*
L004d20:
	addq.l	#$4,sp
L004d22:
	btst.b	#$4,$93f(a6)
L004d28:
	bne.b	L004d2e
L004d2a:
	moveq	#$1,d0
L004d2c:
	bra.b	L004d32
L004d2e:
	moveq	#$0,d1
L004d30:
	move.l	d1,d0
L004d32:
	move.l	d0,-(sp)
L004d34:
	move.l	d6,d0
L004d36:
	move.l	(a3),d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004d38:
	dc.w	$6100
	dc.w	Q9_mem_free_5a22-*
L004d3c:
	add.l	d6,$6c(a6)
L004d40:
	addq.l	#$4,sp
L004d42:
	bra.b	L004d66
L004d44:
	move.l	d5,-(sp)
L004d46:
	move.l	d7,-(sp)
L004d48:
	move.l	a3,d0
L004d4a:
	moveq	#$0,d1
L004d4c:
	move.w	$6(a2),d1
L004d50:
	lsl.l	#$4,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004d52:
	dc.w	$6100
	dc.w	L004ef6-*
L004d56:
	tst.l	d0
L004d58:
	addq.l	#$8,sp
L004d5a:
	beq.b	L004d66
L004d5c:
	lea	L004955(pc),a0
L004d60:
	move.l	a0,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004d62:
	dc.w	$6100
	dc.w	L0007f4-*
L004d66:
	moveq	#$2a,d0
L004d68:
	adda.l	d0,a3
L004d6a:
	lea	-$2a(a3),a4
L004d6e:
	movea.l	a3,a0
L004d70:
	moveq	#$4,d0
L004d72:
	move.l	(a4)+,(a0)+
L004d74:
	move.l	(a4)+,(a0)+
L004d76:
	dbf	d0,L004d72
L004d7a:
	move.w	(a4)+,(a0)+
L004d7c:
	move.l	a3,d0
L004d7e:
	movea.l	$50(a6),a0
L004d82:
	lea	$390(a0),a0
L004d86:
	move.l	a0,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004d88:
	dc.w	$6100
	dc.w	Q9_sorted_list_insert_5c7c-*
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004d8c:
	dc.w	$6000
	dc.w	L004e6e-*
L004d90:
	move.w	(a2),$26(a1)
L004d94:
	move.w	$2(a2),$28(a1)
L004d9a:
	move.w	$4(a2),$24(a1)
L004da0:
	move.l	$8(a2),(a1)
L004da4:
	move.l	$c(a2),$4(a1)
L004daa:
	tst.l	$14(a2)
L004dae:
	beq.b	L004db6
L004db0:
	movea.l	$14(a2),a0
L004db4:
	bra.b	L004dba
L004db6:
	movea.l	$8(a2),a0
L004dba:
	move.l	a0,$1c(a1)
L004dbe:
	btst.b	#$1,$93f(a6)
L004dc4:
	beq.b	L004e22
L004dc6:
	move.l	d5,-(sp)
L004dc8:
	move.l	a1,d0
L004dca:
	move.l	d7,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004dcc:
	dc.w	$6100
	dc.w	L004e90-*
L004dd0:
	movea.l	(a1),a0
L004dd2:
	move.l	$4(a1),d6
L004dd6:
	sub.l	a0,d6
L004dd8:
	moveq	#$2,d0
L004dda:
	and.w	$4(a2),d0
L004dde:
	moveq	#$0,d1
L004de0:
	move.w	d0,d1
L004de2:
	tst.l	d1
L004de4:
	addq.l	#$4,sp
L004de6:
	beq.b	L004e00
L004de8:
	btst.b	#$6,$93f(a6)
L004dee:
	bne.b	L004e00
L004df0:
	pea	$4c6f7665.l
L004df6:
	move.l	d6,d0
L004df8:
	move.l	a0,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004dfa:
	dc.w	$6100
	dc.w	L0010ba-*
L004dfe:
	addq.l	#$4,sp
L004e00:
	btst.b	#$4,$93f(a6)
L004e06:
	bne.b	L004e0c
L004e08:
	moveq	#$1,d0
L004e0a:
	bra.b	L004e10
L004e0c:
	moveq	#$0,d1
L004e0e:
	move.l	d1,d0
L004e10:
	move.l	d0,-(sp)
L004e12:
	move.l	d6,d0
L004e14:
	move.l	(a1),d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004e16:
	dc.w	$6100
	dc.w	Q9_mem_free_5a22-*
L004e1a:
	add.l	d6,$6c(a6)
L004e1e:
	addq.l	#$4,sp
L004e20:
	bra.b	L004e44
L004e22:
	move.l	d5,-(sp)
L004e24:
	move.l	d7,-(sp)
L004e26:
	move.l	a1,d0
L004e28:
	moveq	#$0,d1
L004e2a:
	move.w	$6(a2),d1
L004e2e:
	lsl.l	#$4,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004e30:
	dc.w	$6100
	dc.w	L004ef6-*
L004e34:
	tst.l	d0
L004e36:
	addq.l	#$8,sp
L004e38:
	beq.b	L004e44
L004e3a:
	lea	L004955(pc),a0
L004e3e:
	move.l	a0,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004e40:
	dc.w	$6100
	dc.w	L0007f4-*
L004e44:
	moveq	#$2a,d0
L004e46:
	adda.l	d0,a1
L004e48:
	lea	-$2a(a1),a3
L004e4c:
	movea.l	a1,a0
L004e4e:
	moveq	#$4,d0
L004e50:
	move.l	(a3)+,(a0)+
L004e52:
	move.l	(a3)+,(a0)+
L004e54:
	dbf	d0,L004e50
L004e58:
	move.w	(a3)+,(a0)+
L004e5a:
	move.l	a1,d0
L004e5c:
	movea.l	$50(a6),a0
L004e60:
	lea	$390(a0),a0
L004e64:
	move.l	a0,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004e66:
	dc.w	$6100
	dc.w	Q9_sorted_list_insert_5c7c-*
L004e6a:
	moveq	#$2a,d0
L004e6c:
	adda.l	d0,a1
L004e6e:
	moveq	#$20,d0
L004e70:
	adda.l	d0,a2
L004e72:
	moveq	#$0,d0
L004e74:
	move.w	(a2),d0
L004e76:
	tst.l	d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004e78:
	dc.w	$6600
	dc.w	L004c58-*
L004e7c:
	moveq	#$0,d0
L004e7e:
	move.w	$2(a2),d0
L004e82:
	tst.l	d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004e84:
	dc.w	$6600
	dc.w	L004c58-*
L004e88:
	movem.l	(sp)+,d4/d5/d6/d7/a0/a1/a2/a3/a4
L004e8c:
	unlk	a5
L004e8e:
	rts
L004e90:
	link.w	a5,#$0
L004e94:
	movem.l	a4/a3/a2/a1/a0,-(sp)
L004e98:
	movea.l	d0,a4
L004e9a:
	movea.l	d1,a3
L004e9c:
	movea.l	(a4),a1
L004e9e:
	movea.l	$4(a4),a2
L004ea2:
	bra.b	L004ec8
L004ea4:
	cmpa.l	(a3),a1
L004ea6:
	bhi.b	L004ec6
L004ea8:
	cmpa.l	(a3),a2
L004eaa:
	bls.b	L004ec6
L004eac:
	move.l	(a3),d0
L004eae:
	add.l	$4(a3),d0
L004eb2:
	movea.l	d0,a0
L004eb4:
	cmpa.l	a2,a0
L004eb6:
	bcc.b	L004ebe
L004eb8:
	move.l	a0,(a4)
L004eba:
	movea.l	a0,a1
L004ebc:
	bra.b	L004ec6
L004ebe:
	move.l	$4(a4),(a4)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004ec2:
	dc.w	$6000
	dc.w	L0050ec-*
L004ec6:
	addq.l	#$8,a3
L004ec8:
	tst.l	(a3)
L004eca:
	bne.b	L004ea4
L004ecc:
	move.l	(a4),d0
L004ece:
	cmp.l	$4(a4),d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004ed2:
	dc.w	$6400
	dc.w	L0050ec-*
L004ed6:
	move.l	$7c(a6),d0
L004eda:
	move.l	d0,d1
L004edc:
	add.l	(a4),d1
L004ede:
	subq.l	#$1,d1
L004ee0:
	neg.l	d0
L004ee2:
	and.l	d1,d0
L004ee4:
	move.l	d0,(a4)
L004ee6:
	move.l	a4,d0
L004ee8:
	lea	$3fc(a6),a0
L004eec:
	move.l	a0,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004eee:
	dc.w	$6100
	dc.w	Q9_sorted_list_insert_5c7c-*
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004ef2:
	dc.w	$6000
	dc.w	L0050ec-*
L004ef6:
	link.w	a5,#$0
L004efa:
	movem.l	a4/a3/a2/a1/a0/d7/d6/d5/d4/d3/d2,-(sp)
L004efe:
	movea.l	d0,a0
L004f00:
	move.l	d1,d3
L004f02:
	movea.l	$8(a5),a1
L004f06:
	movea.l	$c(a5),a4
L004f0a:
	move.l	(a0),d0
L004f0c:
	cmp.l	$4(a0),d0
L004f10:
	bcc.b	L004f24
L004f12:
	move.l	$7c(a6),d0
L004f16:
	subq.l	#$1,d0
L004f18:
	move.l	d0,d1
L004f1a:
	and.l	(a0),d1
L004f1c:
	bne.b	L004f24
L004f1e:
	and.l	$4(a0),d0
L004f22:
	beq.b	L004f2e
L004f24:
	move.l	#$d2,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004f2a:
	dc.w	$6000
	dc.w	L00507e-*
L004f2e:
	movea.l	$404(a6),a2
L004f32:
	bra.b	L004f52
L004f34:
	move.l	(a0),d0
L004f36:
	cmp.l	$4(a2),d0
L004f3a:
	bcc.b	L004f4e
L004f3c:
	move.l	$4(a0),d0
L004f40:
	cmp.l	(a2),d0
L004f42:
	bls.b	L004f4e
L004f44:
	move.l	#$d2,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004f4a:
	dc.w	$6000
	dc.w	L00507e-*
L004f4e:
	movea.l	$8(a2),a2
L004f52:
	lea	$3fc(a6),a3
L004f56:
	cmpa.l	a3,a2
L004f58:
	bne.b	L004f34
L004f5a:
	move.l	a0,d0
L004f5c:
	lea	$3fc(a6),a2
L004f60:
	move.l	a2,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004f62:
	dc.w	$6100
	dc.w	Q9_sorted_list_insert_5c7c-*
L004f66:
	moveq	#$4,d0
L004f68:
	and.w	$24(a0),d0
L004f6c:
	moveq	#$0,d1
L004f6e:
	move.w	d0,d1
L004f70:
	tst.l	d1
L004f72:
	beq.b	L004f78
L004f74:
	moveq	#$1,d2
L004f76:
	bra.b	L004f7c
L004f78:
	moveq	#$0,d0
L004f7a:
	move.l	d0,d2
L004f7c:
	tst.l	d3
L004f7e:
	bne.b	L004f86
L004f80:
	lea	$2000.w,a2
L004f84:
	move.l	a2,d3
L004f86:
	move.l	$7c(a6),d0
L004f8a:
	cmp.l	d3,d0
L004f8c:
	bls.b	L004f90
L004f8e:
	move.l	d0,d3
L004f90:
	movea.l	(a0),a2
L004f92:
	move.l	a4,-(sp)
L004f94:
	move.l	a1,-(sp)
L004f96:
	move.l	a2,d0
L004f98:
	move.l	$4(a0),d1
L004f9c:
	sub.l	a2,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004f9e:
	dc.w	$6100
	dc.w	L005086-*
L004fa2:
	move.l	d0,d6
L004fa4:
	lea	$0(a2,d3.l*1),a3
L004fa8:
	subq.l	#$1,a3
L004faa:
	move.l	d3,d0
L004fac:
	neg.l	d0
L004fae:
	move.l	a3,d1
L004fb0:
	and.l	d1,d0
L004fb2:
	movea.l	d0,a3
L004fb4:
	move.l	a3,d4
L004fb6:
	sub.l	a2,d4
L004fb8:
	move.l	d2,-(sp)
L004fba:
	move.l	a2,d0
L004fbc:
	move.l	d4,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004fbe:
	dc.w	$6100
	dc.w	L0010f6-*
L004fc2:
	move.l	d0,d7
L004fc4:
	cmpi.b	#-$1,d7
L004fc8:
	adda.w	#$c,sp
L004fcc:
	beq.b	L004ff4
L004fce:
	cmpi.b	#$1,d7
L004fd2:
	bne.b	L004fd8
L004fd4:
	add.l	d4,$6c(a6)
L004fd8:
	moveq	#$0,d0
L004fda:
	move.b	d6,d0
L004fdc:
	tst.l	d0
L004fde:
	beq.b	L004ff4
L004fe0:
	move.l	a4,-(sp)
L004fe2:
	move.l	a1,-(sp)
L004fe4:
	move.l	a2,d0
L004fe6:
	move.l	d4,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L004fe8:
	dc.w	$6100
	dc.w	L005086-*
L004fec:
	tst.l	d0
L004fee:
	addq.l	#$8,sp
L004ff0:
	beq.b	L004ff4
L004ff2:
	moveq	#-$1,d7
L004ff4:
	move.l	$4(a0),d4
L004ff8:
	sub.l	a3,d4
L004ffa:
	bra.b	L005060
L004ffc:
	move.l	d2,-(sp)
L004ffe:
	move.l	a3,d0
L005000:
	move.l	d4,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005002:
	dc.w	$6100
	dc.w	L0010f6-*
L005006:
	move.b	d0,d5
L005008:
	cmpi.b	#-$1,d5
L00500c:
	addq.l	#$4,sp
L00500e:
	beq.b	L00503e
L005010:
	cmpi.b	#$1,d5
L005014:
	bne.b	L005024
L005016:
	cmp.l	d3,d4
L005018:
	bge.b	L00501e
L00501a:
	move.l	d4,d0
L00501c:
	bra.b	L005020
L00501e:
	move.l	d3,d0
L005020:
	add.l	d0,$6c(a6)
L005024:
	moveq	#$0,d0
L005026:
	move.b	d6,d0
L005028:
	tst.l	d0
L00502a:
	beq.b	L00503e
L00502c:
	move.l	a4,-(sp)
L00502e:
	move.l	a1,-(sp)
L005030:
	move.l	a3,d0
L005032:
	move.l	d3,d1
L005034:
	bsr.b	L005086
L005036:
	tst.l	d0
L005038:
	addq.l	#$8,sp
L00503a:
	beq.b	L00503e
L00503c:
	moveq	#-$1,d5
L00503e:
	cmp.b	d5,d7
L005040:
	beq.b	L00505c
L005042:
	move.l	d2,-(sp)
L005044:
	move.b	d7,d0
L005046:
	extb.l	d0
L005048:
	move.l	d0,-(sp)
L00504a:
	move.l	a3,-(sp)
L00504c:
	move.l	a0,d0
L00504e:
	move.l	a2,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005050:
	dc.w	$6100
	dc.w	L0050f4-*
L005054:
	move.b	d5,d7
L005056:
	movea.l	a3,a2
L005058:
	adda.w	#$c,sp
L00505c:
	adda.l	d3,a3
L00505e:
	sub.l	d3,d4
L005060:
	tst.l	d4
L005062:
	bge.b	L004ffc
L005064:
	suba.l	d3,a3
L005066:
	cmp.b	d5,d7
L005068:
	bne.b	L00507c
L00506a:
	move.l	d2,-(sp)
L00506c:
	extb.l	d7
L00506e:
	move.l	d7,-(sp)
L005070:
	move.l	a3,-(sp)
L005072:
	move.l	a0,d0
L005074:
	move.l	a2,d1
L005076:
	bsr.b	L0050f4
L005078:
	adda.w	#$c,sp
L00507c:
	moveq	#$0,d0
L00507e:
	movem.l	(sp)+,d2/d3/d4/d5/d6/d7/a0/a1/a2/a3/a4
L005082:
	unlk	a5
L005084:
	rts
L005086:
	link.w	a5,#$0
L00508a:
	movem.l	a4/a3/a2/a1/a0,-(sp)
L00508e:
	movea.l	d0,a1
L005090:
	movea.l	$8(a5),a2
L005094:
	movea.l	$c(a5),a4
L005098:
	movea.l	a1,a0
L00509a:
	adda.l	d1,a0
L00509c:
	bra.b	L0050bc
L00509e:
	movea.l	(a2),a3
L0050a0:
	cmpa.l	a3,a1
L0050a2:
	bcs.b	L0050b2
L0050a4:
	move.l	a3,d0
L0050a6:
	add.l	$4(a2),d0
L0050aa:
	cmpa.l	d0,a1
L0050ac:
	bcc.b	L0050ba
L0050ae:
	moveq	#-$1,d0
L0050b0:
	bra.b	L0050ec
L0050b2:
	cmpa.l	a3,a0
L0050b4:
	bls.b	L0050ba
L0050b6:
	moveq	#-$1,d0
L0050b8:
	bra.b	L0050ec
L0050ba:
	addq.l	#$8,a2
L0050bc:
	tst.l	(a2)
L0050be:
	bne.b	L00509e
L0050c0:
	movea.l	a4,a2
L0050c2:
	movea.l	a1,a0
L0050c4:
	adda.l	d1,a0
L0050c6:
	bra.b	L0050e6
L0050c8:
	movea.l	(a2),a3
L0050ca:
	cmpa.l	a3,a1
L0050cc:
	bcs.b	L0050dc
L0050ce:
	move.l	a3,d0
L0050d0:
	add.l	$4(a2),d0
L0050d4:
	cmpa.l	d0,a1
L0050d6:
	bcc.b	L0050e4
L0050d8:
	moveq	#-$1,d0
L0050da:
	bra.b	L0050ec
L0050dc:
	cmpa.l	a3,a0
L0050de:
	bls.b	L0050e4
L0050e0:
	moveq	#-$1,d0
L0050e2:
	bra.b	L0050ec
L0050e4:
	addq.l	#$8,a2
L0050e6:
	tst.l	(a2)
L0050e8:
	bne.b	L0050c8
L0050ea:
	moveq	#$0,d0
L0050ec:
	movem.l	(sp)+,a0/a1/a2/a3/a4
L0050f0:
	unlk	a5
L0050f2:
	rts
L0050f4:
	link.w	a5,#-$4
L0050f8:
	movem.l	a4/a3/a2/a1/a0/d7/d6,-(sp)
L0050fc:
	movea.l	d0,a0
L0050fe:
	movea.l	d1,a1
L005100:
	movea.l	$8(a5),a2
L005104:
	move.b	$f(a5),d7
L005108:
	move.l	$10(a5),d6
L00510c:
	cmpi.b	#$1,d7
L005110:
	bne.b	L005190
L005112:
	moveq	#$8,d0
L005114:
	and.w	$24(a0),d0
L005118:
	moveq	#$0,d1
L00511a:
	move.w	d0,d1
L00511c:
	tst.l	d1
L00511e:
	beq.b	L005166
L005120:
	clr.l	-$4(a5)
L005124:
	pea	-$4(a5)
L005128:
	move.l	a1,d0
L00512a:
	move.l	a2,d1
L00512c:
	sub.l	a1,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00512e:
	dc.w	$6100
	dc.w	L0049fc-*
L005132:
	addq.l	#$4,sp
L005134:
	bra.b	L00515c
L005136:
	movea.l	-$4(a5),a4
L00513a:
	move.l	(a4),-$4(a5)
L00513e:
	btst.b	#$4,$93f(a6)
L005144:
	bne.b	L00514a
L005146:
	moveq	#$1,d0
L005148:
	bra.b	L00514e
L00514a:
	moveq	#$0,d1
L00514c:
	move.l	d1,d0
L00514e:
	move.l	d0,-(sp)
L005150:
	move.l	$4(a3),d0
L005154:
	move.l	a3,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005156:
	dc.w	$6100
	dc.w	Q9_mem_free_5a22-*
L00515a:
	addq.l	#$4,sp
L00515c:
	movea.l	-$4(a5),a3
L005160:
	tst.l	a3
L005162:
	bne.b	L005136
L005164:
	bra.b	L005184
L005166:
	btst.b	#$4,$93f(a6)
L00516c:
	bne.b	L005172
L00516e:
	moveq	#$1,d0
L005170:
	bra.b	L005176
L005172:
	moveq	#$0,d1
L005174:
	move.l	d1,d0
L005176:
	move.l	d0,-(sp)
L005178:
	move.l	a2,d0
L00517a:
	sub.l	a1,d0
L00517c:
	move.l	a1,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00517e:
	dc.w	$6100
	dc.w	Q9_mem_free_5a22-*
L005182:
	addq.l	#$4,sp
L005184:
	cmpa.l	$3dc(a6),a2
L005188:
	bls.b	L0051ae
L00518a:
	move.l	a2,$3dc(a6)
L00518e:
	bra.b	L0051ae
L005190:
	cmpi.b	#$2,d7
L005194:
	bne.b	L0051ae
L005196:
	clr.l	-(sp)
L005198:
	move.l	a1,d0
L00519a:
	suba.l	a1,a2
L00519c:
	move.l	a2,d7
L00519e:
	move.l	d7,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0051a0:
	dc.w	$6100
	dc.w	L0049fc-*
L0051a4:
	tst.l	d6
L0051a6:
	addq.l	#$4,sp
L0051a8:
	beq.b	L0051ae
L0051aa:
	add.l	d7,$20(a0)
L0051ae:
	movem.l	(sp)+,d6/d7/a0/a1/a2/a3/a4
L0051b2:
	unlk	a5
L0051b4:
	rts
L0051b6:
	movem.l	a1/a0/d7,-(sp)
L0051ba:
	movea.l	d0,a0
L0051bc:
	movea.l	d1,a1
L0051be:
	move.l	a1,-(sp)
L0051c0:
	moveq	#$0,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0051c2:
	dc.w	$6100
	dc.w	Q9_arena_alloc_526c-*
L0051c6:
	move.l	d0,d7
L0051c8:
	addq.l	#$4,sp
L0051ca:
	bne.b	L0051da
L0051cc:
	clr.l	-(sp)
L0051ce:
	move.l	(a0),d0
L0051d0:
	move.l	(a1),d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0051d2:
	dc.w	$6100
	dc.w	L0010ba-*
L0051d6:
	addq.l	#$4,sp
L0051d8:
	bra.b	L0051dc
L0051da:
	move.l	d7,d0
L0051dc:
	movem.l	(sp)+,d7/a0/a1
L0051e0:
	rts
L0051e2:
	dc.b	$51
L0051e3:
	dc.b	$fb
L0051e4:
	dc.b	$00
L0051e5:
	dc.b	$00
L0051e6:
	dc.b	$00
L0051e7:
	dc.b	$00
L0051e8:
	link.w	a5,#$0
L0051ec:
	movem.l	a2/a1/a0/d7/d6/d5,-(sp)
L0051f0:
	move.l	d0,d7
L0051f2:
	move.l	d1,d6
L0051f4:
	movea.l	$8(a5),a0
L0051f8:
	move.l	$c(a5),d1
L0051fc:
	move.l	d1,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0051fe:
	dc.w	$6100
	dc.w	Q9_irq_mask_10e6-*
L005202:
	move.l	d0,d1
L005204:
	tst.l	$8(a0)
L005208:
	bne.b	L005212
L00520a:
	move.l	a0,$c(a0)
L00520e:
	move.l	a0,$8(a0)
L005212:
	movea.l	$8(a0),a1
L005216:
	bra.b	L00525a
L005218:
	cmp.w	$26(a1),d6
L00521c:
	beq.b	L005230
L00521e:
	moveq	#$0,d0
L005220:
	move.w	d6,d0
L005222:
	tst.l	d0
L005224:
	bne.b	L005256
L005226:
	moveq	#$0,d0
L005228:
	move.w	$28(a1),d0
L00522c:
	tst.l	d0
L00522e:
	beq.b	L005256
L005230:
	cmp.l	$20(a1),d7
L005234:
	bhi.b	L005256
L005236:
	movea.l	$10(a1),a2
L00523a:
	tst.l	a2
L00523c:
	beq.b	L005256
L00523e:
	bra.b	L00524c
L005240:
	cmp.l	$8(a2),d7
L005244:
	bcc.b	L00524a
L005246:
	move.l	$8(a2),d7
L00524a:
	movea.l	(a2),a2
L00524c:
	moveq	#$10,d0
L00524e:
	move.l	a1,d5
L005250:
	add.l	d0,d5
L005252:
	cmpa.l	d5,a2
L005254:
	bne.b	L005240
L005256:
	movea.l	$8(a1),a1
L00525a:
	cmpa.l	a0,a1
L00525c:
	bne.b	L005218
L00525e:
	move.l	d7,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005260:
	dc.w	$6100
	dc.w	Q9_irq_unmask_10f2-*
L005264:
	movem.l	(sp)+,d5/d6/d7/a0/a1/a2
L005268:
	unlk	a5
L00526a:
	rts
* Arena-Deskriptor-Allocator mit Fallback auf den zweiten Speicherpool.
Q9_arena_alloc_526c:
	link.w	a5,#-$4
L005270:
	movem.l	a4/a3/a2/a1/a0/d7/d6/d5/d4/d3,-(sp)
L005274:
	move.l	d0,d5
L005276:
	move.l	d1,d7
L005278:
	move.l	$8(a5),d6
L00527c:
	movea.l	d5,a0
L00527e:
	moveq	#-$1,d0
L005280:
	sub.l	$7c(a6),d0
L005284:
	cmp.l	(a0),d0
L005286:
	bcc.b	L0052b2
L005288:
	movea.l	d5,a0
L00528a:
	moveq	#-$1,d0
L00528c:
	cmp.l	(a0),d0
L00528e:
	bne.b	L0052a8
L005290:
	moveq	#$1,d0
L005292:
	move.l	d0,-(sp)
L005294:
	pea	$3fc(a6)
L005298:
	moveq	#$0,d1
L00529a:
	move.w	d7,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00529c:
	dc.w	$6100
	dc.w	L0051e8-*
L0052a0:
	movea.l	d5,a0
L0052a2:
	move.l	d0,(a0)
L0052a4:
	addq.l	#$8,sp
L0052a6:
	bra.b	L0052b2
L0052a8:
	move.l	#$ed,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0052ae:
	dc.w	$6000
	dc.w	L005438-*
L0052b2:
	move.l	$70(a6),d0
L0052b6:
	movea.l	d5,a0
L0052b8:
	move.l	d0,d1
L0052ba:
	add.l	(a0),d1
L0052bc:
	subq.l	#$1,d1
L0052be:
	neg.l	d0
L0052c0:
	and.l	d1,d0
L0052c2:
	move.l	d0,(a0)
L0052c4:
	bne.b	L0052d0
L0052c6:
	lea	$e1.w,a0
L0052ca:
	move.l	a0,d4
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0052cc:
	dc.w	$6000
	dc.w	L005406-*
L0052d0:
	moveq	#$1,d0
L0052d2:
	move.l	d0,-(sp)
L0052d4:
	movea.l	$50(a6),a0
L0052d8:
	pea	$390(a0)
L0052dc:
	move.l	d6,-(sp)
L0052de:
	movea.l	d5,a0
L0052e0:
	move.l	(a0),d0
L0052e2:
	moveq	#$0,d1
L0052e4:
	move.w	d7,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0052e6:
	dc.w	$6100
	dc.w	Q9_mem_alloc_5440-*
L0052ea:
	move.l	d0,d4
L0052ec:
	cmpi.l	#$ed,d4
L0052f2:
	adda.w	#$c,sp
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0052f6:
	dc.w	$6600
	dc.w	L005406-*
L0052fa:
	move.l	$7c(a6),d0
L0052fe:
	movea.l	d5,a0
L005300:
	move.l	d0,d1
L005302:
	add.l	(a0),d1
L005304:
	subq.l	#$1,d1
L005306:
	neg.l	d0
L005308:
	and.l	d1,d0
L00530a:
	move.l	d0,-$4(a5)
L00530e:
	moveq	#$1,d0
L005310:
	move.l	d0,-(sp)
L005312:
	pea	$3fc(a6)
L005316:
	move.l	d6,-(sp)
L005318:
	move.l	-$4(a5),d0
L00531c:
	moveq	#$0,d1
L00531e:
	move.w	d7,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005320:
	dc.w	$6100
	dc.w	Q9_mem_alloc_5440-*
L005324:
	move.l	d0,d4
L005326:
	cmpi.l	#$ed,d4
L00532c:
	adda.w	#$c,sp
L005330:
	bne.b	L00533a
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005332:
	dc.w	$6100
	dc.w	L005558-*
L005336:
	tst.l	d0
L005338:
	beq.b	L00530e
L00533a:
	tst.l	d4
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00533c:
	dc.w	$6600
	dc.w	L005406-*
L005340:
	movea.l	d6,a0
L005342:
	movea.l	(a0),a2
L005344:
	move.l	-$4(a5),d7
L005348:
	movea.l	$50(a6),a0
L00534c:
	lea	$390(a0),a4
L005350:
	movea.l	$8(a4),a3
L005354:
	tst.l	a3
L005356:
	beq.b	L0053c6
L005358:
	moveq	#$1,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00535a:
	dc.w	$6100
	dc.w	Q9_irq_mask_10e6-*
L00535e:
	move.l	d0,d3
L005360:
	bra.b	L005366
L005362:
	movea.l	$8(a3),a3
L005366:
	cmpa.l	a4,a3
L005368:
	beq.b	L005374
L00536a:
	cmpa.l	(a3),a2
L00536c:
	bcs.b	L005362
L00536e:
	cmpa.l	$4(a3),a2
L005372:
	bcc.b	L005362
L005374:
	cmpa.l	a4,a3
L005376:
	beq.b	L0053be
L005378:
	adda.l	d7,a2
L00537a:
	moveq	#$10,d0
L00537c:
	movea.l	a3,a1
L00537e:
	adda.l	d0,a1
L005380:
	movea.l	$4(a1),a0
L005384:
	tst.l	a0
L005386:
	beq.b	L0053be
L005388:
	bra.b	L00538e
L00538a:
	movea.l	$4(a0),a0
L00538e:
	cmpa.l	a2,a0
L005390:
	bls.b	L005396
L005392:
	cmpa.l	a1,a0
L005394:
	bne.b	L00538a
L005396:
	cmpa.l	a2,a0
L005398:
	bne.b	L0053be
L00539a:
	movea.l	(a0),a1
L00539c:
	move.l	$4(a0),$4(a1)
L0053a2:
	movea.l	$4(a0),a1
L0053a6:
	move.l	(a0),(a1)
L0053a8:
	add.l	$8(a0),d7
L0053ac:
	moveq	#-$1,d0
L0053ae:
	move.l	d0,-(sp)
L0053b0:
	move.l	a4,-(sp)
L0053b2:
	move.l	$8(a0),d0
L0053b6:
	move.l	a3,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0053b8:
	dc.w	$6100
	dc.w	Q9_freelist_bysize_5712-*
L0053bc:
	addq.l	#$8,sp
L0053be:
	moveq	#$0,d0
L0053c0:
	move.l	d3,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0053c2:
	dc.w	$6100
	dc.w	Q9_irq_unmask_10f2-*
L0053c6:
	movea.l	d5,a0
L0053c8:
	move.l	d7,-$4(a5)
L0053cc:
	cmp.l	(a0),d7
L0053ce:
	bls.b	L005406
L0053d0:
	movea.l	d5,a0
L0053d2:
	move.l	(a0),d0
L0053d4:
	sub.l	d0,-$4(a5)
L0053d8:
	movea.l	d6,a0
L0053da:
	move.l	-$4(a5),d0
L0053de:
	add.l	(a0),d0
L0053e0:
	movea.l	d0,a1
L0053e2:
	moveq	#$1,d0
L0053e4:
	move.l	d0,-(sp)
L0053e6:
	move.l	$70(a6),-(sp)
L0053ea:
	movea.l	$50(a6),a0
L0053ee:
	pea	$390(a0)
L0053f2:
	move.l	d6,d0
L0053f4:
	lea	-$4(a5),a0
L0053f8:
	move.l	a0,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0053fa:
	dc.w	$6100
	dc.w	Q9_pool_lookup_55a4-*
L0053fe:
	movea.l	d6,a0
L005400:
	move.l	a1,(a0)
L005402:
	adda.w	#$c,sp
L005406:
	tst.l	d4
L005408:
	beq.b	L005414
L00540a:
	movea.l	d5,a0
L00540c:
	clr.l	(a0)
L00540e:
	movea.l	d6,a0
L005410:
	clr.l	(a0)
L005412:
	bra.b	L005436
L005414:
	moveq	#$10,d0
L005416:
	and.b	$2e(a6),d0
L00541a:
	moveq	#$0,d1
L00541c:
	move.b	d0,d1
L00541e:
	tst.l	d1
L005420:
	beq.b	L005436
L005422:
	pea	$a110ca7d.l
L005428:
	movea.l	d5,a0
L00542a:
	move.l	(a0),d0
L00542c:
	movea.l	d6,a0
L00542e:
	move.l	(a0),d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005430:
	dc.w	$6100
	dc.w	L0010ba-*
L005434:
	addq.l	#$4,sp
L005436:
	move.l	d4,d0
L005438:
	movem.l	(sp)+,d3/d4/d5/d6/d7/a0/a1/a2/a3/a4
L00543c:
	unlk	a5
L00543e:
	rts
* Speicher-Allokations-Primitive: First-Fit ueber Arena-Ketten, Split-von-hinten bei Restflaeche.
Q9_mem_alloc_5440:
	link.w	a5,#$0
L005444:
	movem.l	a4/a3/a2/a1/a0/d7/d6/d5/d4/d3/d2,-(sp)
L005448:
	move.l	d0,d6
L00544a:
	move.l	d1,d3
L00544c:
	movea.l	$8(a5),a0
L005450:
	move.l	$c(a5),d5
L005454:
	move.l	$10(a5),d2
L005458:
	clr.l	(a0)
L00545a:
	move.l	d2,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00545c:
	dc.w	$6100
	dc.w	Q9_irq_mask_10e6-*
L005460:
	move.l	d0,d2
L005462:
	tst.l	d6
L005464:
	bne.b	L00546e
L005466:
	lea	$e1.w,a1
L00546a:
	move.l	a1,d4
L00546c:
	bra.b	L005482
L00546e:
	movea.l	d5,a1
L005470:
	tst.l	$8(a1)
L005474:
	bne.b	L00547c
L005476:
	lea	$ed.w,a1
L00547a:
	bra.b	L005480
L00547c:
	moveq	#$0,d0
L00547e:
	movea.l	d0,a1
L005480:
	move.l	a1,d4
L005482:
	movea.l	d5,a4
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005484:
	dc.w	$6000
	dc.w	L005546-*
L005488:
	movea.l	$8(a4),a4
L00548c:
	cmpa.l	d5,a4
L00548e:
	bne.b	L00549a
L005490:
	lea	$ed.w,a1
L005494:
	move.l	a1,d4
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005496:
	dc.w	$6000
	dc.w	L005546-*
L00549a:
	cmp.w	$26(a4),d3
L00549e:
	beq.b	L0054b6
L0054a0:
	moveq	#$0,d0
L0054a2:
	move.w	d3,d0
L0054a4:
	tst.l	d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0054a6:
	dc.w	$6600
	dc.w	L005546-*
L0054aa:
	moveq	#$0,d0
L0054ac:
	move.w	$28(a4),d0
L0054b0:
	tst.l	d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0054b2:
	dc.w	$6700
	dc.w	L005546-*
L0054b6:
	cmp.l	$20(a4),d6
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0054ba:
	dc.w	$6200
	dc.w	L005546-*
L0054be:
	moveq	#$4,d0
L0054c0:
	and.w	$24(a4),d0
L0054c4:
	moveq	#$0,d1
L0054c6:
	move.w	d0,d1
L0054c8:
	tst.l	d1
L0054ca:
	bne.b	L005546
L0054cc:
	moveq	#$10,d0
L0054ce:
	movea.l	a4,a1
L0054d0:
	adda.l	d0,a1
L0054d2:
	movea.l	$4(a1),a2
L0054d6:
	bra.b	L005538
L0054d8:
	cmpa.l	(a2),a1
L0054da:
	bne.b	L0054f0
L0054dc:
	moveq	#$10,d0
L0054de:
	move.l	a4,d1
L0054e0:
	add.l	d0,d1
L0054e2:
	cmpa.l	d1,a1
L0054e4:
	beq.b	L0054f8
L0054e6:
	move.l	a2,d0
L0054e8:
	add.l	$8(a2),d0
L0054ec:
	cmp.l	a1,d0
L0054ee:
	bcs.b	L0054f8
L0054f0:
	lea	$ab.w,a3
L0054f4:
	move.l	a3,d4
L0054f6:
	bra.b	L005546
L0054f8:
	move.l	$8(a2),d7
L0054fc:
	sub.l	d6,d7
L0054fe:
	tst.l	d7
L005500:
	blt.b	L005532
L005502:
	moveq	#-$1,d0
L005504:
	move.l	d0,-(sp)
L005506:
	move.l	d5,-(sp)
L005508:
	move.l	d6,d0
L00550a:
	move.l	a4,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00550c:
	dc.w	$6100
	dc.w	Q9_freelist_bysize_5712-*
L005510:
	tst.l	d7
L005512:
	addq.l	#$8,sp
L005514:
	bne.b	L005528
L005516:
	movea.l	$4(a2),a3
L00551a:
	move.l	(a2),(a3)
L00551c:
	movea.l	(a2),a3
L00551e:
	move.l	$4(a2),$4(a3)
L005524:
	move.l	a2,(a0)
L005526:
	bra.b	L005532
L005528:
	move.l	d7,$8(a2)
L00552c:
	move.l	a2,d0
L00552e:
	add.l	d7,d0
L005530:
	move.l	d0,(a0)
L005532:
	movea.l	a2,a1
L005534:
	movea.l	$4(a2),a2
L005538:
	moveq	#$10,d0
L00553a:
	move.l	a4,d1
L00553c:
	add.l	d0,d1
L00553e:
	cmpa.l	d1,a2
L005540:
	beq.b	L005546
L005542:
	tst.l	(a0)
L005544:
	beq.b	L0054d8
L005546:
	tst.l	(a0)
L005548:
	bne.b	L005550
L00554a:
	tst.l	d4
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00554c:
	dc.w	$6700
	dc.w	L005488-*
L005550:
	move.l	d4,d0
L005552:
	move.l	d2,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005554:
	dc.w	$6000
	dc.w	L005706-*
L005558:
	movem.l	a1/a0/d1,-(sp)
L00555c:
	moveq	#$4,d0
L00555e:
	and.b	$2e(a6),d0
L005562:
	moveq	#$0,d1
L005564:
	move.b	d0,d1
L005566:
	tst.l	d1
L005568:
	bne.b	L005598
L00556a:
	movea.l	$3c(a6),a0
L00556e:
	movea.l	$40(a6),a1
L005572:
	bra.b	L005594
L005574:
	tst.l	(a0)
L005576:
	beq.b	L005590
L005578:
	moveq	#$0,d0
L00557a:
	move.w	$c(a0),d0
L00557e:
	tst.l	d0
L005580:
	bne.b	L005590
L005582:
	move.l	a0,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005584:
	dc.w	$6100
	dc.w	L0040ea-*
L005588:
	tst.l	d0
L00558a:
	bne.b	L005590
L00558c:
	moveq	#$0,d0
L00558e:
	bra.b	L00559e
L005590:
	moveq	#$10,d0
L005592:
	adda.l	d0,a0
L005594:
	cmpa.l	a1,a0
L005596:
	bcs.b	L005574
L005598:
	move.l	#$ed,d0
L00559e:
	movem.l	(sp)+,d1/a0/a1
L0055a2:
	rts
* Pool-Lookup: findet den zustaendigen Speicherpool-Deskriptor fuer eine Adresse/Groesse.
Q9_pool_lookup_55a4:
	link.w	a5,#-$4
L0055a8:
	movem.l	a4/a3/a2/a1/a0/d7/d6/d5/d4/d3/d2,-(sp)
L0055ac:
	move.l	d0,d2
L0055ae:
	move.l	d1,d3
L0055b0:
	move.l	$8(a5),d5
L0055b4:
	move.l	$c(a5),d1
L0055b8:
	move.l	$10(a5),d4
L0055bc:
	movea.l	d3,a0
L0055be:
	move.l	(a0),d6
L0055c0:
	bne.b	L0055c8
L0055c2:
	moveq	#$0,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0055c4:
	dc.w	$6000
	dc.w	L00570a-*
L0055c8:
	movea.l	d2,a0
L0055ca:
	move.l	(a0),d7
L0055cc:
	move.l	d7,d0
L0055ce:
	or.l	d6,d0
L0055d0:
	subq.l	#$1,d1
L0055d2:
	and.l	d0,d1
L0055d4:
	beq.b	L0055e0
L0055d6:
	move.l	#$db,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0055dc:
	dc.w	$6000
	dc.w	L00570a-*
L0055e0:
	tst.l	d7
L0055e2:
	bne.b	L0055ee
L0055e4:
	move.l	#$db,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0055ea:
	dc.w	$6000
	dc.w	L00570a-*
L0055ee:
	move.l	d4,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0055f0:
	dc.w	$6100
	dc.w	Q9_irq_mask_10e6-*
L0055f4:
	move.l	d0,d4
L0055f6:
	pea	-$4(a5)
L0055fa:
	move.l	d5,-(sp)
L0055fc:
	move.l	d6,d0
L0055fe:
	move.l	d7,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005600:
	dc.w	$6100
	dc.w	Q9_arena_lookup_5bac-*
L005604:
	tst.l	d0
L005606:
	addq.l	#$8,sp
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005608:
	dc.w	$6600
	dc.w	L005704-*
L00560c:
	moveq	#$10,d0
L00560e:
	add.l	-$4(a5),d0
L005612:
	movea.l	d0,a0
L005614:
	tst.l	(a0)
L005616:
	bne.b	L005624
L005618:
	movea.l	a0,a1
L00561a:
	movea.l	a1,a2
L00561c:
	move.l	a2,$4(a0)
L005620:
	move.l	a2,(a0)
L005622:
	bra.b	L005644
L005624:
	movea.l	a0,a2
L005626:
	bra.b	L00563a
L005628:
	cmpa.l	$4(a1),a2
L00562c:
	beq.b	L005638
L00562e:
	lea	$ab.w,a3
L005632:
	move.l	a3,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005634:
	dc.w	$6000
	dc.w	L005704-*
L005638:
	movea.l	a1,a2
L00563a:
	movea.l	(a2),a1
L00563c:
	cmpa.l	d7,a1
L00563e:
	bcc.b	L005644
L005640:
	cmpa.l	a0,a1
L005642:
	bne.b	L005628
L005644:
	cmpa.l	a0,a2
L005646:
	beq.b	L005650
L005648:
	movea.l	a2,a3
L00564a:
	adda.l	$8(a2),a3
L00564e:
	bra.b	L005654
L005650:
	moveq	#$0,d0
L005652:
	movea.l	d0,a3
L005654:
	cmpa.l	a1,a3
L005656:
	bls.b	L005666
L005658:
	cmpa.l	a0,a1
L00565a:
	beq.b	L005666
L00565c:
	lea	$ab.w,a4
L005660:
	move.l	a4,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005662:
	dc.w	$6000
	dc.w	L005704-*
L005666:
	cmpa.l	d7,a3
L005668:
	bhi.b	L005676
L00566a:
	move.l	d7,d0
L00566c:
	add.l	d6,d0
L00566e:
	cmp.l	a1,d0
L005670:
	bls.b	L005680
L005672:
	cmpa.l	a0,a1
L005674:
	beq.b	L005680
L005676:
	lea	$d2.w,a0
L00567a:
	move.l	a0,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00567c:
	dc.w	$6000
	dc.w	L005704-*
L005680:
	moveq	#$1,d0
L005682:
	move.l	d0,-(sp)
L005684:
	move.l	d5,-(sp)
L005686:
	move.l	d6,d0
L005688:
	move.l	-$4(a5),d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00568c:
	dc.w	$6100
	dc.w	Q9_freelist_bysize_5712-*
L005690:
	moveq	#$10,d0
L005692:
	and.b	$2e(a6),d0
L005696:
	moveq	#$0,d1
L005698:
	move.b	d0,d1
L00569a:
	tst.l	d1
L00569c:
	addq.l	#$8,sp
L00569e:
	beq.b	L0056b0
L0056a0:
	pea	$66726565.l
L0056a6:
	move.l	d6,d0
L0056a8:
	move.l	d7,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0056aa:
	dc.w	$6100
	dc.w	L0010ba-*
L0056ae:
	addq.l	#$4,sp
L0056b0:
	cmpa.l	d7,a3
L0056b2:
	bne.b	L0056ce
L0056b4:
	movea.l	d2,a0
L0056b6:
	move.l	a2,(a0)
L0056b8:
	move.l	a2,d7
L0056ba:
	movea.l	d7,a0
L0056bc:
	add.l	d6,$8(a0)
L0056c0:
	movea.l	d7,a3
L0056c2:
	movea.l	d3,a0
L0056c4:
	move.l	$8(a3),d0
L0056c8:
	move.l	d0,(a0)
L0056ca:
	move.l	d0,d6
L0056cc:
	bra.b	L0056e0
L0056ce:
	movea.l	d7,a0
L0056d0:
	move.l	d6,$8(a0)
L0056d4:
	move.l	a2,$4(a0)
L0056d8:
	move.l	a1,(a0)
L0056da:
	move.l	d7,(a2)
L0056dc:
	move.l	d7,$4(a1)
L0056e0:
	move.l	d7,d0
L0056e2:
	add.l	d0,d6
L0056e4:
	move.l	a1,d0
L0056e6:
	cmp.l	d0,d6
L0056e8:
	bne.b	L005702
L0056ea:
	movea.l	d3,a0
L0056ec:
	move.l	(a0),d0
L0056ee:
	add.l	$8(a1),d0
L0056f2:
	move.l	d0,(a0)
L0056f4:
	movea.l	d7,a0
L0056f6:
	move.l	d0,$8(a0)
L0056fa:
	move.l	(a1),(a0)
L0056fc:
	movea.l	(a0),a1
L0056fe:
	move.l	d7,$4(a1)
L005702:
	moveq	#$0,d0
L005704:
	move.l	d4,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005706:
	dc.w	$6100
	dc.w	Q9_irq_unmask_10f2-*
L00570a:
	movem.l	(sp)+,d2/d3/d4/d5/d6/d7/a0/a1/a2/a3/a4
L00570e:
	unlk	a5
L005710:
	rts
* Groessen-/klassensortierte Freiliste auf Arena-Ebene (Einfuegen und Schrumpfen bestehender Eintraege).
Q9_freelist_bysize_5712:
	link.w	a5,#$0
L005716:
	movem.l	a3/a2/a1/a0,-(sp)
L00571a:
	movea.l	d1,a0
L00571c:
	movea.l	$8(a5),a1
L005720:
	move.l	$c(a5),d1
L005724:
	ble.b	L00576e
L005726:
	add.l	d0,$20(a0)
L00572a:
	bra.b	L005750
L00572c:
	move.l	$8(a0),$8(a2)
L005732:
	movea.l	$8(a0),a3
L005736:
	move.l	a2,$c(a3)
L00573a:
	move.l	a2,$8(a0)
L00573e:
	move.l	$c(a2),$c(a0)
L005744:
	move.l	a0,$c(a2)
L005748:
	movea.l	$c(a0),a3
L00574c:
	move.l	a0,$8(a3)
L005750:
	movea.l	$c(a0),a2
L005754:
	cmpa.l	a1,a2
L005756:
	beq.b	L0057b4
L005758:
	move.w	$28(a0),d1
L00575c:
	cmp.w	$28(a2),d1
L005760:
	bne.b	L0057b4
L005762:
	move.l	$20(a0),d1
L005766:
	cmp.l	$20(a2),d1
L00576a:
	bhi.b	L00572c
L00576c:
	bra.b	L0057b4
L00576e:
	sub.l	d0,$20(a0)
L005772:
	bra.b	L005798
L005774:
	movea.l	$c(a0),a3
L005778:
	move.l	a2,$8(a3)
L00577c:
	move.l	$c(a0),$c(a2)
L005782:
	move.l	$8(a2),$8(a0)
L005788:
	movea.l	$8(a0),a3
L00578c:
	move.l	a0,$c(a3)
L005790:
	move.l	a0,$8(a2)
L005794:
	move.l	a2,$c(a0)
L005798:
	movea.l	$8(a0),a2
L00579c:
	cmpa.l	a1,a2
L00579e:
	beq.b	L0057b4
L0057a0:
	move.w	$28(a0),d0
L0057a4:
	cmp.w	$28(a2),d0
L0057a8:
	bne.b	L0057b4
L0057aa:
	move.l	$20(a0),d0
L0057ae:
	cmp.l	$20(a2),d0
L0057b2:
	bcs.b	L005774
L0057b4:
	moveq	#$0,d0
L0057b6:
	movem.l	(sp)+,a0/a1/a2/a3
L0057ba:
	unlk	a5
L0057bc:
	rts
L0057be:
	link.w	a5,#-$4
L0057c2:
	movem.l	a2/a1/a0/d7/d6/d5/d4,-(sp)
L0057c6:
	movea.l	d0,a1
L0057c8:
	move.l	d1,d5
L0057ca:
	movea.l	$8(a5),a2
L0057ce:
	move.l	(a1),d6
L0057d0:
	moveq	#-$1,d0
L0057d2:
	cmp.l	d0,d6
L0057d4:
	bne.b	L005804
L0057d6:
	clr.l	-(sp)
L0057d8:
	movea.l	$4c(a6),a0
L0057dc:
	pea	$390(a0)
L0057e0:
	moveq	#$1,d0
L0057e2:
	moveq	#$0,d1
L0057e4:
	move.w	d5,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0057e6:
	dc.w	$6100
	dc.w	L0051e8-*
L0057ea:
	move.l	d0,d6
L0057ec:
	moveq	#$1,d0
L0057ee:
	move.l	d0,-(sp)
L0057f0:
	pea	$3fc(a6)
L0057f4:
	move.l	d6,d0
L0057f6:
	moveq	#$0,d1
L0057f8:
	move.w	d5,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0057fa:
	dc.w	$6100
	dc.w	L0051e8-*
L0057fe:
	move.l	d0,d6
L005800:
	adda.w	#$10,sp
L005804:
	move.l	$70(a6),d0
L005808:
	move.l	d0,d1
L00580a:
	add.l	d6,d1
L00580c:
	subq.l	#$1,d1
L00580e:
	neg.l	d0
L005810:
	and.l	d1,d0
L005812:
	move.l	d0,d6
L005814:
	bne.b	L005824
L005816:
	movea.l	a2,a0
L005818:
	clr.l	(a0)
L00581a:
	move.l	#$e1,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005820:
	dc.w	$6000
	dc.w	L005908-*
L005824:
	clr.l	-(sp)
L005826:
	movea.l	$4c(a6),a0
L00582a:
	pea	$390(a0)
L00582e:
	move.l	a2,-(sp)
L005830:
	move.l	d6,d0
L005832:
	moveq	#$0,d1
L005834:
	move.w	d5,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005836:
	dc.w	$6100
	dc.w	Q9_mem_alloc_5440-*
L00583a:
	move.l	d0,d7
L00583c:
	adda.w	#$c,sp
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005840:
	dc.w	$6700
	dc.w	L0058dc-*
L005844:
	cmpi.l	#$ed,d7
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00584a:
	dc.w	$6600
	dc.w	L005900-*
L00584e:
	move.l	$7c(a6),d0
L005852:
	move.l	d0,d1
L005854:
	add.l	d6,d1
L005856:
	subq.l	#$1,d1
L005858:
	neg.l	d0
L00585a:
	and.l	d1,d0
L00585c:
	move.l	d0,-$4(a5)
L005860:
	moveq	#$1,d0
L005862:
	move.l	d0,-(sp)
L005864:
	pea	$3fc(a6)
L005868:
	move.l	a2,-(sp)
L00586a:
	move.l	-$4(a5),d0
L00586e:
	moveq	#$0,d1
L005870:
	move.w	d5,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005872:
	dc.w	$6100
	dc.w	Q9_mem_alloc_5440-*
L005876:
	move.l	d0,d7
L005878:
	cmpi.l	#$ed,d7
L00587e:
	adda.w	#$c,sp
L005882:
	bne.b	L00588c
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005884:
	dc.w	$6100
	dc.w	L005558-*
L005888:
	tst.l	d0
L00588a:
	beq.b	L005860
L00588c:
	tst.l	d7
L00588e:
	bne.b	L005900
L005890:
	move.l	(a2),d0
L005892:
	move.l	-$4(a5),d1
L005896:
	bsr.b	L005910
L005898:
	move.l	d0,d7
L00589a:
	bne.b	L005900
L00589c:
	move.l	$70(a6),d4
L0058a0:
	cmp.l	$7c(a6),d4
L0058a4:
	beq.b	L0058dc
L0058a6:
	clr.l	-(sp)
L0058a8:
	move.l	d4,-(sp)
L0058aa:
	movea.l	$4c(a6),a0
L0058ae:
	pea	$390(a0)
L0058b2:
	move.l	a2,d0
L0058b4:
	lea	-$4(a5),a0
L0058b8:
	move.l	a0,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0058ba:
	dc.w	$6100
	dc.w	Q9_pool_lookup_55a4-*
L0058be:
	clr.l	-(sp)
L0058c0:
	movea.l	$4c(a6),a0
L0058c4:
	pea	$390(a0)
L0058c8:
	move.l	a2,-(sp)
L0058ca:
	move.l	d6,d0
L0058cc:
	moveq	#$0,d1
L0058ce:
	move.w	d5,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0058d0:
	dc.w	$6100
	dc.w	Q9_mem_alloc_5440-*
L0058d4:
	move.l	d0,d7
L0058d6:
	adda.w	#$18,sp
L0058da:
	bne.b	L005900
L0058dc:
	move.l	d6,(a1)
L0058de:
	moveq	#$10,d0
L0058e0:
	and.b	$2e(a6),d0
L0058e4:
	moveq	#$0,d1
L0058e6:
	move.b	d0,d1
L0058e8:
	tst.l	d1
L0058ea:
	beq.b	L0058fc
L0058ec:
	pea	$a110ca7d.l
L0058f2:
	move.l	d6,d0
L0058f4:
	move.l	(a2),d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0058f6:
	dc.w	$6100
	dc.w	L0010ba-*
L0058fa:
	addq.l	#$4,sp
L0058fc:
	moveq	#$0,d0
L0058fe:
	bra.b	L005908
L005900:
	movea.l	a2,a0
L005902:
	clr.l	(a0)
L005904:
	clr.l	(a1)
L005906:
	move.l	d7,d0
L005908:
	movem.l	(sp)+,d4/d5/d6/d7/a0/a1/a2
L00590c:
	unlk	a5
L00590e:
	rts
L005910:
	link.w	a5,#-$c
L005914:
	movem.l	a1/a0/d7,-(sp)
L005918:
	movea.l	d0,a1
L00591a:
	move.l	d1,d7
L00591c:
	pea	$100.w
L005920:
	moveq	#$2,d0
L005922:
	move.l	d0,-(sp)
L005924:
	movea.l	$4c(a6),a0
L005928:
	lea	$2d8(a0),a0
L00592c:
	move.l	a0,d0
L00592e:
	lea	-$c(a5),a0
L005932:
	move.l	a0,d1
L005934:
	bsr.b	L0059a0
L005936:
	tst.l	d0
L005938:
	addq.l	#$8,sp
L00593a:
	beq.b	L00596c
L00593c:
	move.l	a1,-$8(a5)
L005940:
	move.l	d7,-$4(a5)
L005944:
	moveq	#$1,d0
L005946:
	move.l	d0,-(sp)
L005948:
	move.l	$7c(a6),-(sp)
L00594c:
	pea	$3fc(a6)
L005950:
	lea	-$8(a5),a0
L005954:
	move.l	a0,d0
L005956:
	lea	-$4(a5),a0
L00595a:
	move.l	a0,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00595c:
	dc.w	$6100
	dc.w	Q9_pool_lookup_55a4-*
L005960:
	move.l	#$cf,d0
L005966:
	adda.w	#$c,sp
L00596a:
	bra.b	L005998
L00596c:
	movea.l	-$c(a5),a0
L005970:
	move.l	a1,d0
L005972:
	move.l	d0,(a0)
L005974:
	movea.l	-$c(a5),a0
L005978:
	move.l	d7,$4(a0)
L00597c:
	moveq	#$3,d0
L00597e:
	move.l	d0,-(sp)
L005980:
	move.l	a1,d0
L005982:
	move.l	d7,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005984:
	dc.w	$6100
	dc.w	L0011fa-*
L005988:
	tst.l	d0
L00598a:
	addq.l	#$4,sp
L00598c:
	beq.b	L005996
L00598e:
	cmpi.l	#$d0,d0
L005994:
	bne.b	L005998
L005996:
	moveq	#$0,d0
L005998:
	movem.l	(sp)+,d7/a0/a1
L00599c:
	unlk	a5
L00599e:
	rts
L0059a0:
	link.w	a5,#-$4
L0059a4:
	movem.l	a4/a3/a2/a1/a0/d7/d6/d5,-(sp)
L0059a8:
	move.l	d0,d7
L0059aa:
	movea.l	d1,a3
L0059ac:
	move.l	$8(a5),d6
L0059b0:
	move.l	$c(a5),d5
L0059b4:
	movea.l	d7,a0
L0059b6:
	movea.l	(a0),a4
L0059b8:
	move.l	d6,d0
L0059ba:
	lsl.l	#$2,d0
L0059bc:
	movea.l	d0,a2
L0059be:
	lsl.w	#$2,d6
L0059c0:
	move.l	(a4),d0
L0059c2:
	subq.w	#$8,d0
L0059c4:
	moveq	#$0,d1
L0059c6:
	move.w	d0,d1
L0059c8:
	divu.w	d6,d1
L0059ca:
	movea.l	a3,a0
L0059cc:
	move.l	a4,d0
L0059ce:
	addq.l	#$8,d0
L0059d0:
	move.l	d0,(a0)
L0059d2:
	bra.b	L0059e8
L0059d4:
	movea.l	a3,a0
L0059d6:
	movea.l	(a0),a1
L0059d8:
	tst.l	(a1)
L0059da:
	bne.b	L0059e2
L0059dc:
	moveq	#$0,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0059de:
	dc.w	$6000
	dc.w	L0062d2-*
L0059e2:
	movea.l	a3,a0
L0059e4:
	move.l	a2,d0
L0059e6:
	add.l	d0,(a0)
L0059e8:
	subq.w	#$1,d1
L0059ea:
	bge.b	L0059d4
L0059ec:
	tst.l	$4(a4)
L0059f0:
	bne.b	L005a1c
L0059f2:
	move.l	d5,-(sp)
L0059f4:
	lea	-$4(a5),a0
L0059f8:
	move.l	a0,d0
L0059fa:
	move.l	a3,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0059fc:
	dc.w	$6100
	dc.w	L00646c-*
L005a00:
	tst.l	d0
L005a02:
	addq.l	#$4,sp
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005a04:
	dc.w	$6600
	dc.w	L0062d2-*
L005a08:
	movea.l	-$4(a5),a0
L005a0c:
	movea.l	d7,a1
L005a0e:
	move.l	(a1),$4(a0)
L005a12:
	move.l	-$4(a5),(a1)
L005a16:
	moveq	#$0,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005a18:
	dc.w	$6000
	dc.w	L0062d2-*
L005a1c:
	movea.l	$4(a4),a4
L005a20:
	bra.b	L0059c0
* Zentrale Speicherfreigabe-Primitive: zwei Pools nacheinander versucht, Boundary-Tag-Coalescing.
Q9_mem_free_5a22:
	link.w	a5,#-$c
L005a26:
	movem.l	a2/a1/a0/d7/d6/d5,-(sp)
L005a2a:
	move.l	d0,-$c(a5)
L005a2e:
	move.l	d1,-$8(a5)
L005a32:
	move.l	$8(a5),d6
L005a36:
	move.l	d6,-(sp)
L005a38:
	move.l	$7c(a6),-(sp)
L005a3c:
	pea	$3fc(a6)
L005a40:
	lea	-$8(a5),a0
L005a44:
	move.l	a0,d0
L005a46:
	lea	-$c(a5),a0
L005a4a:
	move.l	a0,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005a4c:
	dc.w	$6100
	dc.w	Q9_pool_lookup_55a4-*
L005a50:
	cmpi.l	#$db,d0
L005a56:
	adda.w	#$c,sp
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005a5a:
	dc.w	$6600
	dc.w	L005ba4-*
L005a5e:
	move.l	$70(a6),d0
L005a62:
	move.l	d0,d1
L005a64:
	add.l	-$c(a5),d1
L005a68:
	subq.l	#$1,d1
L005a6a:
	neg.l	d0
L005a6c:
	and.l	d1,d0
L005a6e:
	move.l	d0,-$c(a5)
L005a72:
	move.l	d6,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005a74:
	dc.w	$6100
	dc.w	Q9_irq_mask_10e6-*
L005a78:
	move.l	d0,d7
L005a7a:
	move.l	d6,-(sp)
L005a7c:
	move.l	$70(a6),-(sp)
L005a80:
	movea.l	$50(a6),a0
L005a84:
	pea	$390(a0)
L005a88:
	lea	-$8(a5),a0
L005a8c:
	move.l	a0,d0
L005a8e:
	lea	-$c(a5),a0
L005a92:
	move.l	a0,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005a94:
	dc.w	$6100
	dc.w	Q9_pool_lookup_55a4-*
L005a98:
	tst.l	d0
L005a9a:
	adda.w	#$c,sp
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005a9e:
	dc.w	$6600
	dc.w	L005b9e-*
L005aa2:
	move.l	$7c(a6),d5
L005aa6:
	cmp.l	-$c(a5),d5
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005aaa:
	dc.w	$6200
	dc.w	L005b9c-*
L005aae:
	move.l	d5,d0
L005ab0:
	subq.l	#$1,d0
L005ab2:
	and.l	-$8(a5),d0
L005ab6:
	tst.l	d0
L005ab8:
	bne.b	L005ad4
L005aba:
	movea.l	-$8(a5),a0
L005abe:
	movea.l	$4(a0),a0
L005ac2:
	movea.l	-$8(a5),a1
L005ac6:
	move.l	(a1),(a0)
L005ac8:
	movea.l	-$8(a5),a1
L005acc:
	movea.l	(a1),a2
L005ace:
	move.l	a0,$4(a2)
L005ad2:
	bra.b	L005b00
L005ad4:
	sub.l	d0,d5
L005ad6:
	sub.l	d5,-$c(a5)
L005ada:
	move.l	-$c(a5),d0
L005ade:
	cmp.l	$7c(a6),d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005ae2:
	dc.w	$6500
	dc.w	L005b9c-*
L005ae6:
	movea.l	-$8(a5),a0
L005aea:
	move.l	d5,$8(a0)
L005aee:
	add.l	-$8(a5),d5
L005af2:
	move.l	d5,-$8(a5)
L005af6:
	movea.l	-$8(a5),a1
L005afa:
	move.l	-$c(a5),$8(a1)
L005b00:
	move.l	$7c(a6),d0
L005b04:
	subq.l	#$1,d0
L005b06:
	and.l	-$c(a5),d0
L005b0a:
	tst.l	d0
L005b0c:
	beq.b	L005b2e
L005b0e:
	sub.l	d0,-$c(a5)
L005b12:
	move.l	-$8(a5),d1
L005b16:
	add.l	-$c(a5),d1
L005b1a:
	movea.l	d1,a1
L005b1c:
	move.l	d0,$8(a1)
L005b20:
	move.l	a0,$4(a1)
L005b24:
	move.l	(a0),(a1)
L005b26:
	move.l	a1,(a0)
L005b28:
	movea.l	(a1),a0
L005b2a:
	move.l	a1,$4(a0)
L005b2e:
	moveq	#$0,d0
L005b30:
	move.l	d7,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005b32:
	dc.w	$6100
	dc.w	Q9_irq_unmask_10f2-*
L005b36:
	pea	-$4(a5)
L005b3a:
	movea.l	$50(a6),a0
L005b3e:
	pea	$390(a0)
L005b42:
	move.l	-$c(a5),d0
L005b46:
	move.l	-$8(a5),d1
L005b4a:
	bsr.b	Q9_arena_lookup_5bac
L005b4c:
	tst.l	d0
L005b4e:
	addq.l	#$8,sp
L005b50:
	bne.b	L005b7c
L005b52:
	move.l	d6,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005b54:
	dc.w	$6100
	dc.w	Q9_irq_mask_10e6-*
L005b58:
	move.l	d0,d7
L005b5a:
	moveq	#-$1,d0
L005b5c:
	move.l	d0,-(sp)
L005b5e:
	movea.l	$50(a6),a0
L005b62:
	pea	$390(a0)
L005b66:
	move.l	-$c(a5),d0
L005b6a:
	move.l	-$4(a5),d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005b6e:
	dc.w	$6100
	dc.w	Q9_freelist_bysize_5712-*
L005b72:
	moveq	#$0,d0
L005b74:
	move.l	d7,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005b76:
	dc.w	$6100
	dc.w	Q9_irq_unmask_10f2-*
L005b7a:
	addq.l	#$8,sp
L005b7c:
	move.l	d6,-(sp)
L005b7e:
	move.l	$7c(a6),-(sp)
L005b82:
	pea	$3fc(a6)
L005b86:
	lea	-$8(a5),a0
L005b8a:
	move.l	a0,d0
L005b8c:
	lea	-$c(a5),a0
L005b90:
	move.l	a0,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005b92:
	dc.w	$6100
	dc.w	Q9_pool_lookup_55a4-*
L005b96:
	adda.w	#$c,sp
L005b9a:
	bra.b	L005ba4
L005b9c:
	moveq	#$0,d0
L005b9e:
	move.l	d7,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005ba0:
	dc.w	$6100
	dc.w	Q9_irq_unmask_10f2-*
L005ba4:
	movem.l	(sp)+,d5/d6/d7/a0/a1/a2
L005ba8:
	unlk	a5
L005baa:
	rts
* Arena-Lookup-oder-Erzeugen fuer eine freizugebende Adresse (legt bei Bedarf einen neuen Arena-Deskriptor an).
Q9_arena_lookup_5bac:
	link.w	a5,#-$8
L005bb0:
	movem.l	a4/a3/a2/a1/a0/d7/d6,-(sp)
L005bb4:
	move.l	d0,d7
L005bb6:
	movea.l	d1,a1
L005bb8:
	movea.l	$8(a5),a2
L005bbc:
	move.l	$c(a5),d6
L005bc0:
	tst.l	$8(a2)
L005bc4:
	bne.b	L005bce
L005bc6:
	move.l	a2,$c(a2)
L005bca:
	move.l	a2,$8(a2)
L005bce:
	movea.l	$8(a2),a3
L005bd2:
	bra.b	L005bd8
L005bd4:
	movea.l	$8(a3),a3
L005bd8:
	cmpa.l	a2,a3
L005bda:
	beq.b	L005be6
L005bdc:
	cmpa.l	(a3),a1
L005bde:
	bcs.b	L005bd4
L005be0:
	cmpa.l	$4(a3),a1
L005be4:
	bcc.b	L005bd4
L005be6:
	cmpa.l	a2,a3
L005be8:
	bne.b	L005c62
L005bea:
	movea.l	$404(a6),a3
L005bee:
	bra.b	L005c06
L005bf0:
	movea.l	$8(a3),a3
L005bf4:
	lea	$3fc(a6),a0
L005bf8:
	cmpa.l	a0,a3
L005bfa:
	bne.b	L005c06
L005bfc:
	move.l	#$d2,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005c02:
	dc.w	$6000
	dc.w	L0060fa-*
L005c06:
	cmpa.l	(a3),a1
L005c08:
	bcs.b	L005bf0
L005c0a:
	cmpa.l	$4(a3),a1
L005c0e:
	bcc.b	L005bf0
L005c10:
	move.l	a1,d0
L005c12:
	add.l	d7,d0
L005c14:
	cmp.l	$4(a3),d0
L005c18:
	bls.b	L005c24
L005c1a:
	move.l	#$d2,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005c20:
	dc.w	$6000
	dc.w	L0060fa-*
L005c24:
	moveq	#$2a,d0
L005c26:
	move.l	d0,-$8(a5)
L005c2a:
	pea	-$4(a5)
L005c2e:
	lea	-$8(a5),a0
L005c32:
	move.l	a0,d0
L005c34:
	moveq	#$0,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005c36:
	dc.w	$6100
	dc.w	Q9_arena_alloc_526c-*
L005c3a:
	tst.l	d0
L005c3c:
	addq.l	#$4,sp
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005c3e:
	dc.w	$6600
	dc.w	L0060fa-*
L005c42:
	movea.l	-$4(a5),a0
L005c46:
	movea.l	a3,a4
L005c48:
	moveq	#$4,d0
L005c4a:
	move.l	(a4)+,(a0)+
L005c4c:
	move.l	(a4)+,(a0)+
L005c4e:
	dbf	d0,L005c4a
L005c52:
	move.w	(a4)+,(a0)+
L005c54:
	move.l	-$4(a5),d0
L005c58:
	move.l	a2,d1
L005c5a:
	bsr.b	Q9_sorted_list_insert_5c7c
L005c5c:
	movea.l	-$4(a5),a3
L005c60:
	bra.b	L005c74
L005c62:
	adda.l	d7,a1
L005c64:
	cmpa.l	$4(a3),a1
L005c68:
	bls.b	L005c74
L005c6a:
	move.l	#$d2,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005c70:
	dc.w	$6000
	dc.w	L0060fa-*
L005c74:
	movea.l	d6,a0
L005c76:
	move.l	a3,(a0)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005c78:
	dc.w	$6000
	dc.w	L0060f8-*
* Genereller, nach Klassen-/Typ-Tag sortierter Doppelverkettungs-Insert (auch fuer Arenen genutzt).
Q9_sorted_list_insert_5c7c:
	movem.l	a2/a1/a0,-(sp)
L005c80:
	movea.l	d0,a0
L005c82:
	movea.l	d1,a1
L005c84:
	tst.l	$8(a1)
L005c88:
	bne.b	L005c92
L005c8a:
	move.l	a1,$c(a1)
L005c8e:
	move.l	a1,$8(a1)
L005c92:
	clr.l	$14(a0)
L005c96:
	clr.l	$10(a0)
L005c9a:
	clr.l	$20(a0)
L005c9e:
	movea.l	$8(a1),a2
L005ca2:
	bra.b	L005ca8
L005ca4:
	movea.l	$8(a2),a2
L005ca8:
	cmpa.l	a1,a2
L005caa:
	beq.b	L005cb6
L005cac:
	move.w	$28(a2),d0
L005cb0:
	cmp.w	$28(a0),d0
L005cb4:
	bcc.b	L005ca4
L005cb6:
	move.l	a2,$8(a0)
L005cba:
	move.l	$c(a2),$c(a0)
L005cc0:
	movea.l	$c(a2),a1
L005cc4:
	move.l	a0,$8(a1)
L005cc8:
	move.l	a0,$c(a2)
L005ccc:
	movem.l	(sp)+,a0/a1/a2
L005cd0:
	rts
L005cd2:
	link.w	a5,#-$8
L005cd6:
	movem.l	a2/a1/a0/d7/d6,-(sp)
L005cda:
	move.l	d0,d7
L005cdc:
	movea.l	d1,a0
L005cde:
	move.l	$70(a6),d0
L005ce2:
	move.l	d0,d1
L005ce4:
	add.l	d7,d1
L005ce6:
	subq.l	#$1,d1
L005ce8:
	neg.l	d0
L005cea:
	and.l	d1,d0
L005cec:
	move.l	d0,d7
L005cee:
	pea	-$8(a5)
L005cf2:
	move.l	a0,d1
L005cf4:
	bsr.b	Q9_owns_range_5d68
L005cf6:
	move.l	d0,d6
L005cf8:
	cmpi.l	#$d2,d6
L005cfe:
	addq.l	#$4,sp
L005d00:
	bne.b	L005d4a
L005d02:
	move.l	d7,-$4(a5)
L005d06:
	movea.l	a0,a1
L005d08:
	bra.b	L005d32
L005d0a:
	move.l	a0,d0
L005d0c:
	add.l	-$4(a5),d0
L005d10:
	movea.l	d0,a0
L005d12:
	sub.l	-$4(a5),d7
L005d16:
	move.l	-$8(a5),-(sp)
L005d1a:
	move.l	-$4(a5),d0
L005d1e:
	move.l	a1,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005d20:
	dc.w	$6100
	dc.w	L005dee-*
L005d24:
	move.l	d0,d6
L005d26:
	addq.l	#$4,sp
L005d28:
	bne.b	L005d5e
L005d2a:
	movea.l	a0,a1
L005d2c:
	move.l	d7,-$4(a5)
L005d30:
	beq.b	L005d5e
L005d32:
	pea	-$8(a5)
L005d36:
	lea	-$4(a5),a2
L005d3a:
	move.l	a2,d0
L005d3c:
	move.l	a1,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005d3e:
	dc.w	$6100
	dc.w	L00608c-*
L005d42:
	move.l	d0,d6
L005d44:
	addq.l	#$4,sp
L005d46:
	beq.b	L005d0a
L005d48:
	bra.b	L005d5e
L005d4a:
	tst.l	d6
L005d4c:
	bne.b	L005d5e
L005d4e:
	move.l	-$8(a5),-(sp)
L005d52:
	move.l	d7,d0
L005d54:
	move.l	a0,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005d56:
	dc.w	$6100
	dc.w	L005dee-*
L005d5a:
	addq.l	#$4,sp
L005d5c:
	bra.b	L005d60
L005d5e:
	move.l	d6,d0
L005d60:
	movem.l	(sp)+,d6/d7/a0/a1/a2
L005d64:
	unlk	a5
L005d66:
	rts
* Prueft, ob ein Adressbereich zu den vom aktuellen Prozess gehaltenen Speicherbloecken gehoert.
Q9_owns_range_5d68:
	link.w	a5,#$0
L005d6c:
	movem.l	a4/a3/a2/a1/a0/d7/d6/d5,-(sp)
L005d70:
	move.l	$8(a5),d6
L005d74:
	beq.b	L005d86
L005d76:
	move.l	$70(a6),d7
L005d7a:
	move.l	d7,d5
L005d7c:
	add.l	d0,d5
L005d7e:
	subq.l	#$1,d5
L005d80:
	neg.l	d7
L005d82:
	and.l	d5,d7
L005d84:
	move.l	d7,d0
L005d86:
	movea.l	d1,a3
L005d88:
	adda.l	d0,a3
L005d8a:
	cmpa.l	d1,a3
L005d8c:
	bcc.b	L005d98
L005d8e:
	move.l	#$d2,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005d94:
	dc.w	$6000
	dc.w	L0062d2-*
L005d98:
	movea.l	$4c(a6),a0
L005d9c:
	movea.l	$2d8(a0),a0
L005da0:
	movea.l	a0,a1
L005da2:
	addq.l	#$8,a1
L005da4:
	lea	$100(a0),a2
L005da8:
	tst.l	(a1)
L005daa:
	beq.b	L005dba
L005dac:
	cmp.l	(a1),d1
L005dae:
	bcs.b	L005dba
L005db0:
	move.l	(a1),d0
L005db2:
	add.l	$4(a1),d0
L005db6:
	cmpa.l	d0,a3
L005db8:
	bls.b	L005de0
L005dba:
	addq.l	#$8,a1
L005dbc:
	cmpa.l	a2,a1
L005dbe:
	bne.b	L005da8
L005dc0:
	movea.l	$4(a0),a0
L005dc4:
	tst.l	a0
L005dc6:
	bne.b	L005dd2
L005dc8:
	move.l	#$d2,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005dce:
	dc.w	$6000
	dc.w	L0062d2-*
L005dd2:
	move.l	a0,d0
L005dd4:
	addq.l	#$8,d0
L005dd6:
	movea.l	d0,a1
L005dd8:
	lea	$100(a0),a4
L005ddc:
	movea.l	a4,a2
L005dde:
	bra.b	L005da8
L005de0:
	tst.l	d6
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005de2:
	dc.w	$6700
	dc.w	L0062d0-*
L005de6:
	movea.l	d6,a0
L005de8:
	move.l	a1,(a0)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005dea:
	dc.w	$6000
	dc.w	L0062d0-*
L005dee:
	link.w	a5,#-$c
L005df2:
	movem.l	a4/a3/a2/a1/a0/d7,-(sp)
L005df6:
	move.l	d0,-$c(a5)
L005dfa:
	move.l	d1,-$8(a5)
L005dfe:
	movea.l	$8(a5),a4
L005e02:
	clr.l	-(sp)
L005e04:
	move.l	$70(a6),-(sp)
L005e08:
	movea.l	$4c(a6),a0
L005e0c:
	pea	$390(a0)
L005e10:
	lea	-$8(a5),a0
L005e14:
	move.l	a0,d0
L005e16:
	lea	-$c(a5),a0
L005e1a:
	move.l	a0,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005e1c:
	dc.w	$6100
	dc.w	Q9_pool_lookup_55a4-*
L005e20:
	move.l	d0,d7
L005e22:
	adda.w	#$c,sp
L005e26:
	beq.b	L005e80
L005e28:
	cmpi.l	#$ed,d7
L005e2e:
	bne.b	L005e7a
L005e30:
	move.l	$7c(a6),d0
L005e34:
	move.l	d0,d1
L005e36:
	subq.l	#$1,d1
L005e38:
	and.l	-$c(a5),d1
L005e3c:
	bne.b	L005e7a
L005e3e:
	moveq	#$1,d1
L005e40:
	move.l	d1,-(sp)
L005e42:
	move.l	d0,-(sp)
L005e44:
	pea	$3fc(a6)
L005e48:
	lea	-$8(a5),a0
L005e4c:
	move.l	a0,d0
L005e4e:
	lea	-$c(a5),a0
L005e52:
	move.l	a0,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005e54:
	dc.w	$6100
	dc.w	Q9_pool_lookup_55a4-*
L005e58:
	move.l	d0,d7
L005e5a:
	adda.w	#$c,sp
L005e5e:
	bne.b	L005e7a
L005e60:
	moveq	#$3,d0
L005e62:
	move.l	d0,-(sp)
L005e64:
	move.l	-$8(a5),d0
L005e68:
	move.l	-$c(a5),d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005e6c:
	dc.w	$6100
	dc.w	L0011f0-*
L005e70:
	movea.l	a4,a0
L005e72:
	clr.l	(a0)
L005e74:
	clr.l	$4(a4)
L005e78:
	addq.l	#$4,sp
L005e7a:
	move.l	d7,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005e7c:
	dc.w	$6000
	dc.w	L00622a-*
L005e80:
	moveq	#$0,d7
L005e82:
	move.l	d7,d1
L005e84:
	movea.l	-$8(a5),a3
L005e88:
	move.l	-$8(a5),d0
L005e8c:
	cmp.l	(a4),d0
L005e8e:
	bcc.b	L005ea0
L005e90:
	move.l	(a4),d0
L005e92:
	sub.l	-$8(a5),d0
L005e96:
	move.l	d0,d1
L005e98:
	move.l	(a4),-$8(a5)
L005e9c:
	sub.l	d0,-$c(a5)
L005ea0:
	move.l	(a4),d0
L005ea2:
	add.l	$4(a4),d0
L005ea6:
	movea.l	d0,a1
L005ea8:
	move.l	-$8(a5),d0
L005eac:
	add.l	-$c(a5),d0
L005eb0:
	movea.l	d0,a2
L005eb2:
	cmpa.l	a1,a2
L005eb4:
	bls.b	L005ec0
L005eb6:
	suba.l	a1,a2
L005eb8:
	move.l	a2,d0
L005eba:
	move.l	d0,d7
L005ebc:
	sub.l	d7,-$c(a5)
L005ec0:
	move.l	-$c(a5),d0
L005ec4:
	cmp.l	$7c(a6),d0
L005ec8:
	bcc.b	L005ed0
L005eca:
	moveq	#$0,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005ecc:
	dc.w	$6000
	dc.w	L00622a-*
L005ed0:
	tst.l	d7
L005ed2:
	beq.b	L005ef4
L005ed4:
	move.l	-$8(a5),d0
L005ed8:
	add.l	-$c(a5),d0
L005edc:
	movea.l	d0,a1
L005ede:
	move.l	(a3),(a1)
L005ee0:
	move.l	a3,$4(a1)
L005ee4:
	move.l	d7,$8(a1)
L005ee8:
	movea.l	(a1),a2
L005eea:
	move.l	a1,$4(a2)
L005eee:
	move.l	a1,(a3)
L005ef0:
	sub.l	d7,$8(a3)
L005ef4:
	tst.l	d1
L005ef6:
	beq.b	L005f24
L005ef8:
	movea.l	-$8(a5),a0
L005efc:
	move.l	(a3),(a0)
L005efe:
	movea.l	-$8(a5),a0
L005f02:
	move.l	a3,$4(a0)
L005f06:
	movea.l	-$8(a5),a0
L005f0a:
	move.l	-$c(a5),$8(a0)
L005f10:
	movea.l	-$8(a5),a0
L005f14:
	movea.l	(a0),a1
L005f16:
	move.l	-$8(a5),$4(a1)
L005f1c:
	move.l	-$8(a5),(a3)
L005f20:
	move.l	d1,$8(a3)
L005f24:
	move.l	-$8(a5),d0
L005f28:
	cmp.l	(a4),d0
L005f2a:
	bne.b	L005fa0
L005f2c:
	move.l	$7c(a6),d0
L005f30:
	subq.l	#$1,d0
L005f32:
	move.l	d0,d1
L005f34:
	and.l	-$c(a5),d1
L005f38:
	bne.b	L005f5a
L005f3a:
	movea.l	-$8(a5),a0
L005f3e:
	movea.l	$4(a0),a1
L005f42:
	movea.l	-$8(a5),a0
L005f46:
	move.l	(a0),(a1)
L005f48:
	movea.l	-$8(a5),a0
L005f4c:
	movea.l	(a0),a1
L005f4e:
	movea.l	-$8(a5),a0
L005f52:
	move.l	$4(a0),$4(a1)
L005f58:
	bra.b	L005f96
L005f5a:
	not.l	d0
L005f5c:
	and.l	d0,-$c(a5)
L005f60:
	move.l	-$8(a5),d0
L005f64:
	add.l	-$c(a5),d0
L005f68:
	movea.l	d0,a0
L005f6a:
	movea.l	-$8(a5),a1
L005f6e:
	move.l	(a1),(a0)
L005f70:
	movea.l	-$8(a5),a1
L005f74:
	move.l	$4(a1),$4(a0)
L005f7a:
	movea.l	-$8(a5),a1
L005f7e:
	move.l	$8(a1),d0
L005f82:
	sub.l	-$c(a5),d0
L005f86:
	move.l	d0,$8(a0)
L005f8a:
	movea.l	$4(a0),a1
L005f8e:
	movea.l	(a0),a2
L005f90:
	move.l	a0,$4(a2)
L005f94:
	move.l	a0,(a1)
L005f96:
	movea.l	a4,a0
L005f98:
	move.l	-$c(a5),d0
L005f9c:
	add.l	d0,(a0)
L005f9e:
	bra.b	L006008
L005fa0:
	move.l	(a4),d0
L005fa2:
	add.l	$4(a4),d0
L005fa6:
	move.l	-$8(a5),d1
L005faa:
	add.l	-$c(a5),d1
L005fae:
	cmp.l	d1,d0
L005fb0:
	beq.b	L005fb8
L005fb2:
	moveq	#$0,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L005fb4:
	dc.w	$6000
	dc.w	L00622a-*
L005fb8:
	move.l	$7c(a6),d0
L005fbc:
	subq.l	#$1,d0
L005fbe:
	move.l	d0,d1
L005fc0:
	and.l	-$c(a5),d1
L005fc4:
	bne.b	L005fe6
L005fc6:
	movea.l	-$8(a5),a0
L005fca:
	movea.l	$4(a0),a1
L005fce:
	movea.l	-$8(a5),a0
L005fd2:
	move.l	(a0),(a1)
L005fd4:
	movea.l	-$8(a5),a0
L005fd8:
	movea.l	(a0),a1
L005fda:
	movea.l	-$8(a5),a0
L005fde:
	move.l	$4(a0),$4(a1)
L005fe4:
	bra.b	L006008
L005fe6:
	not.l	d0
L005fe8:
	and.l	d0,-$c(a5)
L005fec:
	movea.l	-$8(a5),a0
L005ff0:
	move.l	-$c(a5),d0
L005ff4:
	sub.l	d0,$8(a0)
L005ff8:
	movea.l	-$8(a5),a0
L005ffc:
	move.l	-$8(a5),d0
L006000:
	add.l	$8(a0),d0
L006004:
	move.l	d0,-$8(a5)
L006008:
	move.l	$4(a4),d0
L00600c:
	sub.l	-$c(a5),d0
L006010:
	move.l	d0,$4(a4)
L006014:
	bne.b	L00601e
L006016:
	movea.l	a4,a0
L006018:
	clr.l	(a0)
L00601a:
	clr.l	$4(a4)
L00601e:
	moveq	#$3,d0
L006020:
	move.l	d0,-(sp)
L006022:
	move.l	-$8(a5),d0
L006026:
	move.l	-$c(a5),d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00602a:
	dc.w	$6100
	dc.w	L0011f0-*
L00602e:
	pea	-$4(a5)
L006032:
	movea.l	$4c(a6),a0
L006036:
	pea	$390(a0)
L00603a:
	move.l	-$c(a5),d0
L00603e:
	move.l	-$8(a5),d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006042:
	dc.w	$6100
	dc.w	Q9_arena_lookup_5bac-*
L006046:
	tst.l	d0
L006048:
	adda.w	#$c,sp
L00604c:
	bne.b	L006068
L00604e:
	moveq	#-$1,d0
L006050:
	move.l	d0,-(sp)
L006052:
	movea.l	$4c(a6),a0
L006056:
	pea	$390(a0)
L00605a:
	move.l	-$c(a5),d0
L00605e:
	move.l	-$4(a5),d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006062:
	dc.w	$6100
	dc.w	Q9_freelist_bysize_5712-*
L006066:
	addq.l	#$8,sp
L006068:
	moveq	#$1,d0
L00606a:
	move.l	d0,-(sp)
L00606c:
	move.l	$7c(a6),-(sp)
L006070:
	pea	$3fc(a6)
L006074:
	lea	-$8(a5),a0
L006078:
	move.l	a0,d0
L00607a:
	lea	-$c(a5),a0
L00607e:
	move.l	a0,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006080:
	dc.w	$6100
	dc.w	Q9_pool_lookup_55a4-*
L006084:
	adda.w	#$c,sp
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006088:
	dc.w	$6000
	dc.w	L00622a-*
L00608c:
	link.w	a5,#$0
L006090:
	movem.l	a4/a3/a2/a1/a0/d7/d6,-(sp)
L006094:
	move.l	d0,d6
L006096:
	movea.l	d1,a3
L006098:
	move.l	$8(a5),d7
L00609c:
	movea.l	$4c(a6),a0
L0060a0:
	movea.l	$2d8(a0),a0
L0060a4:
	movea.l	a0,a1
L0060a6:
	addq.l	#$8,a1
L0060a8:
	lea	$100(a0),a2
L0060ac:
	tst.l	(a1)
L0060ae:
	beq.b	L0060be
L0060b0:
	cmpa.l	(a1),a3
L0060b2:
	bcs.b	L0060be
L0060b4:
	move.l	(a1),d1
L0060b6:
	add.l	$4(a1),d1
L0060ba:
	cmpa.l	d1,a3
L0060bc:
	bcs.b	L0060e2
L0060be:
	addq.l	#$8,a1
L0060c0:
	cmpa.l	a2,a1
L0060c2:
	bne.b	L0060ac
L0060c4:
	movea.l	$4(a0),a0
L0060c8:
	tst.l	a0
L0060ca:
	bne.b	L0060d4
L0060cc:
	move.l	#$d2,d0
L0060d2:
	bra.b	L0060fa
L0060d4:
	move.l	a0,d0
L0060d6:
	addq.l	#$8,d0
L0060d8:
	movea.l	d0,a1
L0060da:
	lea	$100(a0),a4
L0060de:
	movea.l	a4,a2
L0060e0:
	bra.b	L0060ac
L0060e2:
	movea.l	d6,a0
L0060e4:
	move.l	a3,d0
L0060e6:
	add.l	(a0),d0
L0060e8:
	cmp.l	d1,d0
L0060ea:
	bls.b	L0060f4
L0060ec:
	movea.l	d6,a0
L0060ee:
	move.l	a3,d0
L0060f0:
	sub.l	d0,d1
L0060f2:
	move.l	d1,(a0)
L0060f4:
	movea.l	d7,a0
L0060f6:
	move.l	a1,(a0)
L0060f8:
	moveq	#$0,d0
L0060fa:
	movem.l	(sp)+,d6/d7/a0/a1/a2/a3/a4
L0060fe:
	unlk	a5
L006100:
	rts
L006102:
	link.w	a5,#-$4
L006106:
	movem.l	a4/a3/a2/a1/a0/d7,-(sp)
L00610a:
	movea.l	d0,a0
L00610c:
	movea.l	d1,a1
L00610e:
	movea.l	$4c(a6),a4
L006112:
	move.l	#$100,-$4(a5)
L00611a:
	tst.l	$2d8(a4)
L00611e:
	bne.b	L00613e
L006120:
	lea	-$4(a5),a2
L006124:
	move.l	a2,d0
L006126:
	lea	$2d8(a4),a2
L00612a:
	move.l	a2,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00612c:
	dc.w	$6100
	dc.w	L0051b6-*
L006130:
	tst.l	d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006132:
	dc.w	$6600
	dc.w	L00622a-*
L006136:
	movea.l	$2d8(a4),a2
L00613a:
	move.l	-$4(a5),(a2)
L00613e:
	move.l	$70(a6),d7
L006142:
	move.l	d7,d0
L006144:
	add.l	(a0),d0
L006146:
	subq.l	#$1,d0
L006148:
	neg.l	d7
L00614a:
	and.l	d0,d7
L00614c:
	tst.l	d7
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00614e:
	dc.w	$6700
	dc.w	L00621c-*
L006152:
	tst.l	$330(a4)
L006156:
	bne.b	L0061d4
L006158:
	move.l	$7c(a6),d0
L00615c:
	move.l	d0,d1
L00615e:
	add.l	d7,d1
L006160:
	subq.l	#$1,d1
L006162:
	neg.l	d0
L006164:
	and.l	d1,d0
L006166:
	move.l	d0,$330(a4)
L00616a:
	moveq	#$1,d0
L00616c:
	move.l	d0,-(sp)
L00616e:
	pea	$3fc(a6)
L006172:
	pea	$32c(a4)
L006176:
	move.l	$330(a4),d0
L00617a:
	moveq	#$0,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00617c:
	dc.w	$6100
	dc.w	Q9_mem_alloc_5440-*
L006180:
	move.l	d0,d1
L006182:
	cmpi.l	#$ed,d1
L006188:
	adda.w	#$c,sp
L00618c:
	bne.b	L006196
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00618e:
	dc.w	$6100
	dc.w	L005558-*
L006192:
	tst.l	d0
L006194:
	beq.b	L00616a
L006196:
	tst.l	d1
L006198:
	beq.b	L0061a0
L00619a:
	move.l	d1,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00619c:
	dc.w	$6000
	dc.w	L00622a-*
L0061a0:
	move.l	$32c(a4),d0
L0061a4:
	move.l	$330(a4),d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0061a8:
	dc.w	$6100
	dc.w	L005910-*
L0061ac:
	tst.l	d0
L0061ae:
	bne.b	L00622a
L0061b0:
	moveq	#$10,d0
L0061b2:
	and.b	$2e(a6),d0
L0061b6:
	moveq	#$0,d1
L0061b8:
	move.b	d0,d1
L0061ba:
	tst.l	d1
L0061bc:
	beq.b	L0061e2
L0061be:
	pea	$a110ca7d.l
L0061c4:
	move.l	$330(a4),d0
L0061c8:
	move.l	$32c(a4),d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0061cc:
	dc.w	$6100
	dc.w	L0010ba-*
L0061d0:
	addq.l	#$4,sp
L0061d2:
	bra.b	L0061e2
L0061d4:
	cmp.l	$330(a4),d7
L0061d8:
	bls.b	L0061e2
L0061da:
	move.l	#$ed,d0
L0061e0:
	bra.b	L00622a
L0061e2:
	cmp.l	$330(a4),d7
L0061e6:
	bcc.b	L00621c
L0061e8:
	movea.l	$32c(a4),a2
L0061ec:
	movea.l	a2,a3
L0061ee:
	adda.l	d7,a3
L0061f0:
	cmpa.l	$c(a4),a3
L0061f4:
	bcc.b	L006208
L0061f6:
	adda.l	$330(a4),a2
L0061fa:
	cmpa.l	$c(a4),a2
L0061fe:
	bcs.b	L006208
L006200:
	move.l	#$df,d0
L006206:
	bra.b	L00622a
L006208:
	move.l	$330(a4),d0
L00620c:
	sub.l	d7,d0
L00620e:
	move.l	a3,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006210:
	dc.w	$6100
	dc.w	L005cd2-*
L006214:
	tst.l	d0
L006216:
	bne.b	L00622a
L006218:
	move.l	d7,$330(a4)
L00621c:
	move.l	$330(a4),(a0)
L006220:
	move.l	$32c(a4),d0
L006224:
	add.l	(a0),d0
L006226:
	move.l	d0,(a1)
L006228:
	moveq	#$0,d0
L00622a:
	movem.l	(sp)+,d7/a0/a1/a2/a3/a4
L00622e:
	unlk	a5
L006230:
	rts
L006232:
	link.w	a5,#$0
L006236:
	movem.l	a4/a3/a2/a1/a0/d7/d6/d5,-(sp)
L00623a:
	movea.l	d0,a3
L00623c:
	move.l	d1,d7
L00623e:
	movea.l	$8(a5),a0
L006242:
	movea.l	$c(a5),a4
L006246:
	moveq	#$0,d6
L006248:
	moveq	#$0,d5
L00624a:
	tst.l	d7
L00624c:
	beq.b	L006264
L00624e:
	move.l	a0,-(sp)
L006250:
	move.l	d7,d0
L006252:
	moveq	#$3,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006254:
	dc.w	$6100
	dc.w	L0011b4-*
L006258:
	tst.l	d0
L00625a:
	addq.l	#$4,sp
L00625c:
	bne.b	L0062d2
L00625e:
	move.l	d7,d0
L006260:
	lsr.l	#$3,d0
L006262:
	move.l	d0,d7
L006264:
	movea.l	$404(a6),a1
L006268:
	bra.b	L0062ac
L00626a:
	moveq	#$1,d0
L00626c:
	and.w	$24(a1),d0
L006270:
	moveq	#$0,d1
L006272:
	move.w	d0,d1
L006274:
	tst.l	d1
L006276:
	beq.b	L0062a8
L006278:
	movea.l	$10(a1),a2
L00627c:
	tst.l	a2
L00627e:
	beq.b	L0062a8
L006280:
	bra.b	L00629e
L006282:
	cmpa.l	a3,a2
L006284:
	bcs.b	L006296
L006286:
	tst.l	d7
L006288:
	beq.b	L006296
L00628a:
	move.l	a2,(a0)
L00628c:
	move.l	$8(a2),$4(a0)
L006292:
	addq.l	#$8,a0
L006294:
	subq.l	#$1,d7
L006296:
	addq.l	#$1,d6
L006298:
	add.l	$8(a2),d5
L00629c:
	movea.l	(a2),a2
L00629e:
	moveq	#$10,d0
L0062a0:
	move.l	a1,d1
L0062a2:
	add.l	d0,d1
L0062a4:
	cmpa.l	d1,a2
L0062a6:
	bne.b	L006282
L0062a8:
	movea.l	$8(a1),a1
L0062ac:
	lea	$3fc(a6),a2
L0062b0:
	cmpa.l	a2,a1
L0062b2:
	bne.b	L00626a
L0062b4:
	tst.l	d7
L0062b6:
	beq.b	L0062be
L0062b8:
	clr.l	(a0)
L0062ba:
	clr.l	$4(a0)
L0062be:
	move.l	$7c(a6),(a4)
L0062c2:
	move.l	d6,$4(a4)
L0062c6:
	move.l	$6c(a6),$8(a4)
L0062cc:
	move.l	d5,$c(a4)
L0062d0:
	moveq	#$0,d0
L0062d2:
	movem.l	(sp)+,d5/d6/d7/a0/a1/a2/a3/a4
L0062d6:
	unlk	a5
L0062d8:
	rts
* Gibt die pro Prozess gehaltenen Speicherblock- und Fixgroessen-Ressourcenlisten beim Exit frei.
Q9_proc_resource_free_62da:
	movem.l	a4/a3/a2/a1/a0/d1,-(sp)
L0062de:
	movea.l	d0,a4
L0062e0:
	movea.l	$2d8(a4),a1
L0062e4:
	movea.l	a1,a2
L0062e6:
	addq.l	#$8,a2
L0062e8:
	lea	$100(a1),a3
L0062ec:
	bra.b	L006330
L0062ee:
	tst.l	(a2)
L0062f0:
	beq.b	L00630a
L0062f2:
	moveq	#$1,d0
L0062f4:
	move.l	d0,-(sp)
L0062f6:
	move.l	$4(a2),d0
L0062fa:
	move.l	(a2),d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0062fc:
	dc.w	$6100
	dc.w	Q9_mem_free_5a22-*
L006300:
	clr.l	$4(a2)
L006304:
	movea.l	a2,a0
L006306:
	clr.l	(a0)
L006308:
	addq.l	#$4,sp
L00630a:
	addq.l	#$8,a2
L00630c:
	cmpa.l	a3,a2
L00630e:
	bne.b	L006330
L006310:
	movea.l	$4(a1),a0
L006314:
	moveq	#$1,d0
L006316:
	move.l	d0,-(sp)
L006318:
	move.l	(a1),d0
L00631a:
	move.l	a1,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00631c:
	dc.w	$6100
	dc.w	Q9_mem_free_5a22-*
L006320:
	movea.l	a0,a1
L006322:
	move.l	a1,d0
L006324:
	addq.l	#$8,d0
L006326:
	movea.l	d0,a2
L006328:
	lea	$100(a1),a0
L00632c:
	movea.l	a0,a3
L00632e:
	addq.l	#$4,sp
L006330:
	tst.l	a1
L006332:
	bne.b	L0062ee
L006334:
	clr.l	$2d8(a4)
L006338:
	clr.l	$32c(a4)
L00633c:
	clr.l	$330(a4)
L006340:
	lea	$390(a4),a0
L006344:
	movea.l	$8(a0),a1
L006348:
	tst.l	a1
L00634a:
	beq.b	L006366
L00634c:
	bra.b	L006362
L00634e:
	movea.l	$8(a1),a2
L006352:
	moveq	#$1,d0
L006354:
	move.l	d0,-(sp)
L006356:
	moveq	#$2a,d0
L006358:
	move.l	a1,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00635a:
	dc.w	$6100
	dc.w	Q9_mem_free_5a22-*
L00635e:
	movea.l	a2,a1
L006360:
	addq.l	#$4,sp
L006362:
	cmpa.l	a0,a1
L006364:
	bne.b	L00634e
L006366:
	clr.l	$39c(a4)
L00636a:
	clr.l	$398(a4)
L00636e:
	moveq	#$0,d0
L006370:
	movem.l	(sp)+,d1/a0/a1/a2/a3/a4
L006374:
	rts
L006376:
	movem.l	a4/a3/a2/a1/a0/d7/d6/d1/d0,-(sp)
L00637a:
	move.l	$7c(a6),d7
L00637e:
	subq.l	#$1,d7
L006380:
	lea	$3fc(a6),a4
L006384:
	tst.l	$8(a4)
L006388:
	beq.b	L0063ea
L00638a:
	movea.l	a4,a0
L00638c:
	bra.b	L0063e2
L00638e:
	movea.l	$10(a0),a1
L006392:
	tst.l	a1
L006394:
	beq.b	L0063e2
L006396:
	bra.b	L0063d6
L006398:
	movea.l	(a2),a1
L00639a:
	move.l	$8(a2),d0
L00639e:
	move.l	a2,d1
L0063a0:
	move.l	d0,d6
L0063a2:
	or.l	d1,d6
L0063a4:
	and.l	d7,d6
L0063a6:
	beq.b	L0063d6
L0063a8:
	moveq	#-$1,d1
L0063aa:
	move.l	d1,-(sp)
L0063ac:
	move.l	a4,-(sp)
L0063ae:
	move.l	a0,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0063b0:
	dc.w	$6100
	dc.w	Q9_freelist_bysize_5712-*
L0063b4:
	movea.l	$4(a2),a3
L0063b8:
	move.l	a1,(a3)
L0063ba:
	move.l	$4(a2),$4(a1)
L0063c0:
	moveq	#$1,d0
L0063c2:
	move.l	d0,-(sp)
L0063c4:
	move.l	$8(a2),d0
L0063c8:
	move.l	a2,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0063ca:
	dc.w	$6100
	dc.w	Q9_mem_free_5a22-*
L0063ce:
	movea.l	a4,a0
L0063d0:
	adda.w	#$c,sp
L0063d4:
	bra.b	L0063e2
L0063d6:
	moveq	#$10,d0
L0063d8:
	move.l	a0,d1
L0063da:
	add.l	d0,d1
L0063dc:
	movea.l	a1,a2
L0063de:
	cmpa.l	d1,a2
L0063e0:
	bne.b	L006398
L0063e2:
	movea.l	$8(a0),a0
L0063e6:
	cmpa.l	a4,a0
L0063e8:
	bne.b	L00638e
L0063ea:
	movem.l	(sp)+,d0/d1/d6/d7/a0/a1/a2/a3/a4
L0063ee:
	rts
L0063f0:
	link.w	a5,#$0
L0063f4:
	movem.l	a4/a3/a2/a1/a0,-(sp)
L0063f8:
	movea.l	d0,a2
L0063fa:
	movea.l	d1,a0
L0063fc:
	move.b	$b(a5),d1
L006400:
	lea	$3fc(a6),a3
L006404:
	bra.b	L006452
L006406:
	movea.l	a2,a1
L006408:
	movea.l	(a1),a4
L00640a:
	moveq	#$0,d0
L00640c:
	move.b	d1,d0
L00640e:
	tst.l	d0
L006410:
	beq.b	L00641c
L006412:
	move.l	a4,d0
L006414:
	sub.l	$1c(a3),d0
L006418:
	add.l	(a3),d0
L00641a:
	movea.l	d0,a4
L00641c:
	cmpa.l	(a3),a4
L00641e:
	bcs.b	L006452
L006420:
	cmpa.l	$4(a3),a4
L006424:
	bcc.b	L006452
L006426:
	move.l	a4,d0
L006428:
	add.l	(a0),d0
L00642a:
	cmp.l	$4(a3),d0
L00642e:
	bls.b	L006438
L006430:
	move.l	$4(a3),d0
L006434:
	sub.l	a4,d0
L006436:
	move.l	d0,(a0)
L006438:
	moveq	#$0,d0
L00643a:
	move.b	d1,d0
L00643c:
	tst.l	d0
L00643e:
	bne.b	L00644a
L006440:
	move.l	a4,d0
L006442:
	sub.l	(a3),d0
L006444:
	add.l	$1c(a3),d0
L006448:
	movea.l	d0,a4
L00644a:
	movea.l	a2,a1
L00644c:
	move.l	a4,(a1)
L00644e:
	moveq	#$0,d0
L006450:
	bra.b	L006464
L006452:
	movea.l	$8(a3),a3
L006456:
	lea	$3fc(a6),a1
L00645a:
	cmpa.l	a1,a3
L00645c:
	bne.b	L006406
L00645e:
	move.l	#$db,d0
L006464:
	movem.l	(sp)+,a0/a1/a2/a3/a4
L006468:
	unlk	a5
L00646a:
	rts
L00646c:
	link.w	a5,#$0
L006470:
	movem.l	a2/a1/a0,-(sp)
L006474:
	movea.l	d0,a2
L006476:
	movea.l	d1,a1
L006478:
	move.l	a2,-(sp)
L00647a:
	lea	$8(a5),a0
L00647e:
	move.l	a0,d0
L006480:
	moveq	#$0,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006482:
	dc.w	$6100
	dc.w	Q9_arena_alloc_526c-*
L006486:
	tst.l	d0
L006488:
	addq.l	#$4,sp
L00648a:
	bne.b	L0064a8
L00648c:
	clr.l	-(sp)
L00648e:
	move.l	$8(a5),d0
L006492:
	move.l	(a2),d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006494:
	dc.w	$6100
	dc.w	L0010ba-*
L006498:
	movea.l	(a2),a0
L00649a:
	move.l	$8(a5),(a0)
L00649e:
	moveq	#$8,d0
L0064a0:
	add.l	(a2),d0
L0064a2:
	move.l	d0,(a1)
L0064a4:
	moveq	#$0,d0
L0064a6:
	addq.l	#$4,sp
L0064a8:
	movem.l	(sp)+,a0/a1/a2
L0064ac:
	unlk	a5
L0064ae:
	rts
L0064b0:
	dc.b	$00,$00,$01,$00
L0064b4:
	dc.b	$00,$40
L0064b6:
	dc.b	$05,$dc
L0064b8:
	dc.b	$00,$40
L0064ba:
	dc.b	$00,$20
L0064bc:
	dc.b	$00,$20
L0064be:
	dc.b	$01,$00
L0064c0:
	dc.b	$69
L0064c1:
	dc.b	$6e
L0064c2:
	dc.b	$69
L0064c3:
	dc.b	$74
L0064c4:
	dc.b	$00
L0064c5:
	dc.b	$6b,$65,$72,$6e,$65,$6c,$3a,$20,$63,$61,$6e,$27,$74,$20,$66,$69
	dc.b	$6e,$64,$20,$49,$6e,$69,$74,$20,$6d,$6f,$64,$75,$6c,$65,$00
L0064e4:
	dc.b	$4d,$50,$55,$20,$69,$6e,$63,$6f,$6d,$70,$61,$74,$69,$62,$6c,$65
	dc.b	$20,$77,$69,$74,$68,$20,$4f,$53,$2d,$39,$20,$6b,$65,$72,$6e,$65
	dc.b	$6c,$00
L006506:
	dc.b	$6b,$65,$72,$6e,$65,$6c,$3a,$20,$62,$61,$64,$20,$70,$73,$75,$65
	dc.b	$64,$6f,$2d,$76,$65,$63,$74,$6f,$72,$20,$74,$61,$62,$6c,$65,$00
L006526:
	dc.b	$6b,$65,$72,$6e,$65,$6c,$3a,$20,$70,$72,$6f,$63,$20,$64,$65,$73
	dc.b	$63,$20,$28,$74,$6f,$74,$61,$6c,$29,$20,$73,$69,$7a,$65,$20,$6f
	dc.b	$76,$65,$72,$66,$6c,$6f,$77,$00
L00654e:
	dc.b	$6b,$65,$72,$6e,$65,$6c,$3a,$20,$57,$61,$72,$6e,$69,$6e,$67,$20
	dc.b	$2d,$20,$70,$72,$6f,$63,$20,$64,$65,$73,$63,$20,$73,$74,$61,$63
	dc.b	$6b,$20,$73,$69,$7a,$65,$20,$5b,$49,$6e,$69,$74,$5d,$20,$74,$6f
	dc.b	$6f,$20,$73,$6d,$61,$6c,$6c,$2c,$20,$69,$67,$6e,$6f,$72,$65,$64
	dc.b	$00
L00658f:
	dc.b	$6b,$65,$72,$6e,$65,$6c,$3a,$20,$57,$61,$72,$6e,$69,$6e,$67,$20
	dc.b	$2d,$20,$6d,$6f,$64,$75,$6c,$65,$20,$63,$6f,$75,$6e,$74,$20,$5b
	dc.b	$49,$6e,$69,$74,$5d,$20,$74,$6f,$6f,$20,$73,$6d,$61,$6c,$6c,$2c
	dc.b	$20,$69,$67,$6e,$6f,$72,$65,$64,$00
L0065c8:
	dc.b	$6b,$65,$72,$6e,$65,$6c,$3a,$20,$57,$61,$72,$6e,$69,$6e,$67,$20
	dc.b	$2d,$20,$70,$72,$6f,$63,$65,$73,$73,$20,$63,$6f,$75,$6e,$74,$20
	dc.b	$5b,$49,$6e,$69,$74,$5d,$20,$74,$6f,$6f,$20,$73,$6d,$61,$6c,$6c
	dc.b	$2c,$20,$69,$67,$6e,$6f,$72,$65,$64,$00
L006602:
	dc.b	$6b,$65,$72,$6e,$65,$6c,$3a,$20,$57,$61,$72,$6e,$69,$6e,$67,$20
	dc.b	$2d,$20,$65,$76,$65,$6e,$74,$20,$63,$6f,$75,$6e,$74,$20,$5b,$49
	dc.b	$6e,$69,$74,$5d,$20,$74,$6f,$6f,$20,$73,$6d,$61,$6c,$6c,$2c,$20
	dc.b	$69,$67,$6e,$6f,$72,$65,$64,$00
L00663a:
	dc.b	$57,$41,$52,$4e,$49,$4e,$47,$20,$2d,$20,$6b,$65,$72,$6e,$65,$6c
	dc.b	$20,$68,$61,$73,$20,$62,$61,$64,$20,$43,$52,$43,$00
L006657:
	dc.b	$6b,$65,$72,$6e,$65,$6c,$3a,$20,$63,$61,$6e,$27,$74,$20,$61,$6c
	dc.b	$6c,$6f,$63,$61,$74,$65,$20,$70,$72,$6f,$63,$65,$73,$73,$20,$62
	dc.b	$6c,$6f,$63,$6b,$20,$74,$61,$62,$6c,$65,$00
L006682:
	dc.b	$6b,$65,$72,$6e,$65,$6c,$3a,$20,$63,$61,$6e,$27,$74,$20,$61,$6c
	dc.b	$6c,$6f,$63,$61,$74,$65,$20,$49,$52,$51,$20,$73,$74,$61,$63,$6b
	dc.b	$00
L0066a3:
	dc.b	$6b,$65,$72,$6e,$65,$6c,$3a,$20,$63,$61,$6e,$27,$74,$20,$66,$6f
	dc.b	$72,$6b,$20,$69,$6e,$69,$74,$69,$61,$6c,$20,$70,$72,$6f,$63,$65
	dc.b	$73,$73,$00
L0066c6:
	dc.b	$6b,$65,$72,$6e,$65,$6c,$3a,$20,$4f,$53,$2d,$39,$20,$65,$78,$74
	dc.b	$65,$6e,$73,$69,$6f,$6e,$20,$6d,$6f,$64,$75,$6c,$65,$20,$61,$62
	dc.b	$6f,$72,$74,$65,$64,$00
L0066ec:
	dc.b	$6b,$65,$72,$6e,$65,$6c,$3a,$20,$49,$52,$51,$20,$70,$6f,$6c,$6c
	dc.b	$69,$6e,$67,$20,$65,$6e,$74,$72,$69,$65,$73,$20,$5b,$49,$6e,$69
	dc.b	$74,$5d,$20,$74,$6f,$6f,$20,$73,$6d,$61,$6c,$6c,$2c,$20,$69,$67
	dc.b	$6e,$6f,$72,$65,$64,$00
L006722:
	dc.b	$6b,$65,$72,$6e,$65,$6c,$3a,$20,$49,$52,$51,$20,$73,$74,$61,$63
	dc.b	$6b,$20,$73,$69,$7a,$65,$20,$5b,$49,$6e,$69,$74,$5d,$20,$74,$6f
	dc.b	$6f,$20,$73,$6d,$61,$6c,$6c,$2c,$20,$69,$67,$6e,$6f,$72,$65,$64
	dc.b	$00
L006753:
	dc.b	$6b,$65,$72,$6e,$65,$6c,$3a,$20,$63,$61,$6e,$27,$74,$20,$61,$6c
	dc.b	$6c,$6f,$63,$61,$74,$65,$20,$49,$52,$51,$20,$70,$6f,$6c,$6c,$69
	dc.b	$6e,$67,$20,$74,$61,$62,$6c,$65,$00
L00677c:
	dc.b	$6b,$65,$72,$6e,$65,$6c,$3a,$20,$63,$61,$6e,$27,$74,$20,$61,$6c
	dc.b	$6c,$6f,$63,$61,$74,$65,$20,$45,$76,$65,$6e,$74,$20,$74,$61,$62
	dc.b	$6c,$65,$00
L00679f:
	dc.b	$00
* Zentraler Kernel-Bootstrap: alloziert und befuellt die Exception-Sprungtabelle sowie die Syscall-Tabellen im RAM.
Q9_kernel_init_67a0:
	btst.l	#$4,d3
L0067a4:
	bne.b	L0067aa
L0067a6:
	ori	#$700,sr
L0067aa:
	movea.l	sp,a0
L0067ac:
	subq.l	#$4,sp
L0067ae:
	movea.l	sp,a2
L0067b0:
	move.l	(a0)+,(a2)+
L0067b2:
	cmpa.l	a0,a6
L0067b4:
	bhi.b	L0067b0
L0067b6:
	clr.l	(a2)
L0067b8:
	move.l	sp,d6
L0067ba:
	lea	-$80(sp),sp
L0067be:
	move.l	sp,d7
L0067c0:
	addi.l	#$109a0,d1
L0067c6:
	movem.l	a6/a5/a4/a3/a2/a1/a0/d7/d6/d5/d4/d3/d2/d1/d0,-(sp)
L0067ca:
	move.l	#$80,d0
L0067d0:
	move.l	d7,d1
L0067d2:
	pea	$0.w
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0067d6:
	dc.w	$6100
	dc.w	L0010ba-*
L0067da:
	move.l	#$7a4,d0
L0067e0:
	move.l	a6,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0067e2:
	dc.w	$6100
	dc.w	L0010ba-*
L0067e6:
	move.l	#$75c,d0
L0067ec:
	move.l	a6,d1
L0067ee:
	addi.l	#$8a4,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0067f4:
	dc.w	$6100
	dc.w	L0010ba-*
L0067f8:
	addq.l	#$4,sp
L0067fa:
	move.l	#$1010101,$8ac(a6)
L006802:
	move.l	$0(sp),$6c(a6)
L006808:
	move.l	$0(sp),$798(a6)
L00680e:
	move.b	d2,$8f4(a6)
L006812:
	move.l	a1,$64(a6)
L006816:
	lea	$10(a1),a1
L00681a:
	move.l	a1,$8ec(a6)
L00681e:
	move.l	d3,$93c(a6)
L006822:
	move.b	#$14,$932(a6)
L006828:
	move.b	#$2c,$933(a6)
L00682e:
	move.b	#$1,$930(a6)
L006834:
	move.l	a6,$60(a6)
L006838:
	move.l	L0064b0(pc),$8b8(a6)
L00683e:
	lea	Q9_clock_tick_6a8(pc),a0
L006842:
	move.l	a0,$24(a6)
L006846:
	lea	L00073e(pc),a0
L00684a:
	move.l	a0,$8c4(a6)
L00684e:
	lea	L0031d6(pc),a0
L006852:
	move.l	a0,$940(a6)
L006856:
	lea	$37c(a6),a0
L00685a:
	move.l	a0,$30(a0)
L00685e:
	move.l	a0,$34(a0)
L006862:
	lea	$38c(a6),a0
L006866:
	move.l	a0,$30(a0)
L00686a:
	move.l	a0,$34(a0)
L00686e:
	lea	$384(a6),a0
L006872:
	move.l	a0,$30(a0)
L006876:
	move.l	a0,$34(a0)
L00687a:
	lea	$3fc(a6),a0
L00687e:
	move.l	a0,$8(a0)
L006882:
	move.l	a0,$c(a0)
L006886:
	lea	$77c(a6),a0
L00688a:
	move.l	a0,$c(a0)
L00688e:
	move.l	a0,$10(a0)
L006892:
	lea	$774(a6),a0
L006896:
	move.l	a0,$c(a0)
L00689a:
	move.l	a0,$10(a0)
L00689e:
	move.l	$4(sp),d0
L0068a2:
	move.l	d0,$3c8(a6)
L0068a6:
	cmp.l	L000058(pc),d0
L0068aa:
	beq.b	L0068b4
L0068ac:
	lea	L0064e4(pc),a0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0068b0:
	dc.w	$6100
	dc.w	Q9_panic_report_7f6-*
L0068b4:
	movea.l	d7,a0
L0068b6:
	move.l	#$400,$4(a0)
L0068be:
	movec	vbr,d1
L0068c2:
	move.l	d1,$0(a0)
L0068c6:
	bne.b	L0068d0
L0068c8:
	addq.l	#$1,$0(a0)
L0068cc:
	subq.l	#$1,$4(a0)
L0068d0:
	movea.l	d6,a2
L0068d2:
	move.l	$0(a2),$8(a0)
L0068d8:
	move.l	$4(a2),$c(a0)
L0068de:
	addq.l	#$8,d6
L0068e0:
	move.l	$1c(sp),d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L0068e4:
	dc.w	$6100
	dc.w	Q9_const_init_4978-*
L0068e8:
	movea.l	$34(sp),a0
L0068ec:
	move.l	a0,$68(a6)
L0068f0:
	lea	L003802(pc),a1
L0068f4:
	movea.l	a1,a2
L0068f6:
	moveq	#$2,d2
L0068f8:
	movem.w	(a1)+,d0/d1
L0068fc:
	tst.w	d0
L0068fe:
	beq.b	L006914
L006900:
	lea	$0(a2,d1.w*1),a3
L006904:
	move.l	a3,$6(a0)
L006908:
	addq.l	#$1,d2
L00690a:
	lea	$a(a0),a0
L00690e:
	subq.w	#$1,d0
L006910:
	bne.b	L006904
L006912:
	bra.b	L0068f8
L006914:
	cmpi.w	#$100,d2
L006918:
	beq.b	L006922
L00691a:
	lea	L006506(pc),a0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00691e:
	dc.w	$6100
	dc.w	Q9_panic_report_7f6-*
L006922:
	moveq	#$0,d0
L006924:
	move.l	d0,d1
L006926:
	move.l	sp,d2
L006928:
	lea	L006974(pc),a0
L00692c:
	movea.l	$68(a6),a1
L006930:
	movea.l	$60(a1),a2
L006934:
	move.l	a0,$60(a1)
L006938:
	frestore	L000050(pc)
L00693c:
	fmove	fp0,fp0
L006940:
	fsave	-(sp)
L006942:
	moveq	#$1,d0
L006944:
	move.l	#$12c,d1
L00694a:
	cmpi.w	#$1f18,(sp)
L00694e:
	beq.b	L006974
L006950:
	cmpi.w	#$3f18,(sp)
L006954:
	beq.b	L006974
L006956:
	cmpi.w	#$3f18,(sp)
L00695a:
	beq.b	L006974
L00695c:
	moveq	#$2,d0
L00695e:
	move.l	#$14c,d1
L006964:
	cmpi.w	#$1f38,(sp)
L006968:
	beq.b	L006974
L00696a:
	cmpi.w	#$1f38,(sp)
L00696e:
	beq.b	L006974
L006970:
	moveq	#$0,d0
L006972:
	move.l	d0,d1
L006974:
	movea.l	d2,sp
L006976:
	move.l	a2,$60(a1)
L00697a:
	move.b	d0,$2f(a6)
L00697e:
	move.l	d1,$79c(a6)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006982:
	dc.w	$6100
	dc.w	L001184-*
L006986:
	suba.l	a5,a5
L006988:
	moveq	#-$1,d4
L00698a:
	moveq	#-$1,d3
L00698c:
	movea.l	d6,a3
L00698e:
	move.l	(a3)+,d0
L006990:
	beq.b	L0069fa
L006992:
	movea.l	d0,a0
L006994:
	move.l	(a3)+,d0
L006996:
	lea	$0(a0,d0.l*1),a4
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L00699a:
	dc.w	$6100
	dc.w	L004410-*
L00699e:
	bcc.b	L0069a4
L0069a0:
	addq.l	#$2,a0
L0069a2:
	bra.b	L0069f2
L0069a4:
	move.l	$4(a0),d0
L0069a8:
	lea	$0(a0,d0.l*1),a1
L0069ac:
	cmpa.l	a1,a4
L0069ae:
	bcs.b	L0069a0
L0069b0:
	move.l	a0,-(sp)
L0069b2:
	adda.l	$c(a0),a0
L0069b6:
	lea	L0064c0(pc),a1
L0069ba:
	moveq	#$5,d1
L0069bc:
	moveq	#$0,d2
L0069be:
	bra.b	L0069ca
L0069c0:
	move.b	(a0)+,d0
L0069c2:
	move.b	(a1)+,d2
L0069c4:
	eor.b	d2,d0
L0069c6:
	andi.b	#-$21,d0
L0069ca:
	dbne	d1,L0069c0
L0069ce:
	beq.b	L0069d4
L0069d0:
	ori	#$1,ccr
L0069d4:
	movea.l	(sp)+,a0
L0069d6:
	bcs.b	L0069ee
L0069d8:
	move.l	$93c(a6),d0
L0069dc:
	btst.l	#$3,d0
L0069e0:
	bne.b	L0069f8
L0069e2:
	cmp.b	$15(a0),d4
L0069e6:
	bge.b	L0069ee
L0069e8:
	move.b	$15(a0),d4
L0069ec:
	movea.l	a0,a5
L0069ee:
	adda.l	$4(a0),a0
L0069f2:
	cmpa.l	a0,a4
L0069f4:
	bhi.b	L00699a
L0069f6:
	bra.b	L00698e
L0069f8:
	movea.l	a0,a5
L0069fa:
	move.l	a5,d0
L0069fc:
	bne.b	L006a06
L0069fe:
	lea	L0064c5(pc),a0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006a02:
	dc.w	$6100
	dc.w	Q9_panic_report_7f6-*
L006a06:
	move.l	a5,$20(a6)
L006a0a:
	move.w	$5e(a5),$8a6(a6)
L006a10:
	move.w	$60(a5),$8a8(a6)
L006a16:
	move.w	$48(a5),$776(a6)
L006a1c:
	move.w	$48(a5),$778(a6)
L006a22:
	move.b	$68(a5),$2e(a6)
L006a28:
	move.b	$69(a5),$3e0(a6)
L006a2e:
	move.w	$7a(a5),$38(a6)
L006a34:
	btst.b	#$3,$39(a6)
L006a3a:
	beq.b	L006a42
L006a3c:
	bset.b	#$7,$8bc(a6)
L006a42:
	move.w	$7e(a5),d0
L006a46:
	cmp.w	L0064b6(pc),d0
L006a4a:
	bcc.b	L006a58
L006a4c:
	lea	L00654e(pc),a0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006a50:
	dc.w	$6100
	dc.w	L00084a-*
L006a54:
	move.w	L0064b6(pc),d0
L006a58:
	move.w	d0,$8fe(a6)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006a5c:
	dc.w	$6100
	dc.w	L006eee-*
L006a60:
	btst.b	#$3,$3e0(a6)
L006a66:
	beq.b	L006a74
L006a68:
	btst.b	#$1,$3e0(a6)
L006a6e:
	beq.b	L006a74
L006a70:
	st	$3e1(a6)
L006a74:
	move.w	$62(a5),d2
L006a78:
	cmp.w	L0064b4(pc),d2
L006a7c:
	bcc.b	L006a8a
L006a7e:
	lea	L00658f(pc),a0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006a82:
	dc.w	$6100
	dc.w	L00084a-*
L006a86:
	move.w	L0064b4(pc),d2
L006a8a:
	mulu.w	#$10,d2
L006a8e:
	move.l	d2,d3
L006a90:
	moveq	#$0,d0
L006a92:
	move.w	$3e2(a6),d0
L006a96:
	add.l	d0,d3
L006a98:
	addi.l	#$1800,d3
L006a9e:
	lea	$30(sp),a1
L006aa2:
	move.l	a1,d0
L006aa4:
	move.l	d3,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006aa6:
	dc.w	$6100
	dc.w	L00498e-*
L006aaa:
	movea.l	d0,a1
L006aac:
	movea.l	d7,a0
L006aae:
	move.l	a1,$10(a0)
L006ab2:
	move.l	d3,$14(a0)
L006ab6:
	movea.l	a1,a4
L006ab8:
	move.l	a4,$4c(a6)
L006abc:
	move.l	a4,$50(a6)
L006ac0:
	adda.w	$3e2(a6),a1
L006ac4:
	move.l	a1,$3a4(a6)
L006ac8:
	lea	$800(a1),a1
L006acc:
	move.l	a1,$3a8(a6)
L006ad0:
	lea	$800(a1),a1
L006ad4:
	move.l	a1,$3c(a6)
L006ad8:
	adda.l	d2,a1
L006ada:
	move.l	a1,$40(a6)
L006ade:
	move.l	a1,$8e4(a6)
L006ae2:
	move.w	$5c(a5),$18(a4)
L006ae8:
	move.w	$5c(a5),$1a(a4)
L006aee:
	move.w	#$1,$0(a4)
L006af4:
	lea	L000000(pc),a1
L006af8:
	move.l	a1,$38(a4)
L006afc:
	move.l	a4,$34(a4)
L006b00:
	move.l	a4,$30(a4)
L006b04:
	move.b	#$2a,$20(a4)
L006b0a:
	move.w	$8fe(a6),$3b2(a4)
L006b10:
	moveq	#$0,d0
L006b12:
	move.w	$3e2(a6),d0
L006b16:
	move.w	d0,$3b0(a4)
L006b1a:
	lea	$0(a4,d0.l*1),a1
L006b1e:
	move.l	a1,$8(a4)
L006b22:
	sub.w	$3b2(a4),d0
L006b26:
	move.l	#$4a696d69,$0(a4,d0.w*1)
L006b2e:
	bset.b	#$7,$3ac(a4)
L006b34:
	move.l	#$400,d0
L006b3a:
	move.l	$3a4(a6),d1
L006b3e:
	pea	L001380(pc)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006b42:
	dc.w	$6100
	dc.w	L0010ba-*
L006b46:
	move.l	#$400,d0
L006b4c:
	move.l	$3a8(a6),d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006b50:
	dc.w	$6100
	dc.w	L0010ba-*
L006b54:
	addq.l	#$4,sp
L006b56:
	lea	L0036c8(pc),a1
L006b5a:
	movea.l	a6,a3
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006b5c:
	dc.w	$6100
	dc.w	L0036b8-*
L006b60:
	move.l	$30(sp),d0
L006b64:
	move.l	d7,d1
L006b66:
	move.l	d6,-(sp)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006b68:
	dc.w	$6100
	dc.w	L004b20-*
L006b6c:
	addq.l	#$4,sp
L006b6e:
	move.l	$c(sp),d0
L006b72:
	btst.l	#$0,d0
L006b76:
	beq.b	L006b7e
L006b78:
	move.w	#$6f6b,$0(a6)
L006b7e:
	movea.l	d6,a0
L006b80:
	pea	$0.w
L006b84:
	move.l	(a0)+,d0
L006b86:
	beq.b	L006b90
L006b88:
	move.l	(a0)+,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006b8a:
	dc.w	$6100
	dc.w	L0049fc-*
L006b8e:
	bra.b	L006b84
L006b90:
	addq.l	#$4,sp
L006b92:
	clr.w	$0(a6)
L006b96:
	move.l	a4,-(sp)
L006b98:
	lea	L006f30(pc),a0
L006b9c:
	move.w	#$c01,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006ba0:
	dc.w	$6100
	dc.w	L002fa4-*
L006ba4:
	movea.l	(sp)+,a4
L006ba6:
	bcc.b	L006bb0
L006ba8:
	lea	L00663a(pc),a0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006bac:
	dc.w	$6100
	dc.w	L00084a-*
L006bb0:
	tst.b	$8f4(a6)
L006bb4:
	beq.b	L006bca
L006bb6:
	move.l	$93c(a6),d0
L006bba:
	btst.l	#$5,d0
L006bbe:
	bne.b	L006bca
L006bc0:
	pea	L006bca(pc)
L006bc4:
	move.l	$8ec(a6),-(sp)
L006bc8:
	rts
L006bca:
	moveq	#$0,d0
L006bcc:
	move.w	$38(a5),d0
L006bd0:
	cmp.w	L0064b8(pc),d0
L006bd4:
	bcc.b	L006be2
L006bd6:
	lea	L0065c8(pc),a0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006bda:
	dc.w	$6100
	dc.w	L00084a-*
L006bde:
	move.w	L0064b8(pc),d0
L006be2:
	addq.l	#$1,d0
L006be4:
	asl.l	#$3,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006be6:
	dc.w	$6100
	dc.w	L0012d8-*
L006bea:
	bcc.b	L006bf4
L006bec:
	lea	L006657(pc),a0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006bf0:
	dc.w	$6100
	dc.w	Q9_panic_report_7f6-*
L006bf4:
	move.l	a2,$44(a6)
L006bf8:
	move.l	d0,d2
L006bfa:
	lsr.l	#$3,d0
L006bfc:
	subq.l	#$1,d0
L006bfe:
	move.w	d0,(a2)
L006c00:
	move.w	$3e2(a6),$2(a2)
L006c06:
	move.l	a4,$4(a2)
L006c0a:
	lsr.l	#$1,d2
L006c0c:
	lea	$0(a2,d2.l*1),a2
L006c10:
	move.w	d0,$2(a2)
L006c14:
	moveq	#$2,d2
L006c16:
	move.w	d2,(a2)
L006c18:
	lea	$8(a2),a2
L006c1c:
	subq.l	#$2,d0
L006c1e:
	addq.l	#$1,d2
L006c20:
	move.l	d2,(a2)+
L006c22:
	dbf	d0,L006c1e
L006c26:
	clr.l	-$4(a2)
L006c2a:
	move.w	$34(a5),d0
L006c2e:
	cmp.w	L0064ba(pc),d0
L006c32:
	bcc.b	L006c40
L006c34:
	lea	L0066ec(pc),a0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006c38:
	dc.w	$6100
	dc.w	L00084a-*
L006c3c:
	move.w	L0064ba(pc),d0
L006c40:
	mulu.w	#$18,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006c44:
	dc.w	$6100
	dc.w	L0012d8-*
L006c48:
	bcc.b	L006c52
L006c4a:
	lea	L006753(pc),a0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006c4e:
	dc.w	$6100
	dc.w	Q9_panic_report_7f6-*
L006c52:
	move.l	a2,$3e4(a6)
L006c56:
	divu.w	#$18,d0
L006c5a:
	subq.l	#$1,d0
L006c5c:
	lea	$18(a2),a1
L006c60:
	move.l	a1,$0(a2)
L006c64:
	lea	$18(a2),a2
L006c68:
	dbf	d0,L006c5c
L006c6c:
	clr.l	-$18(a2)
L006c70:
	moveq	#$0,d0
L006c72:
	move.w	$6c(a5),d0
L006c76:
	beq.b	L006c9e
L006c78:
	cmp.w	L0064be(pc),d0
L006c7c:
	bcc.b	L006c88
L006c7e:
	lea	L006722(pc),a0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006c82:
	dc.w	$6100
	dc.w	L00084a-*
L006c86:
	bra.b	L006c9e
L006c88:
	asl.l	#$2,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006c8a:
	dc.w	$6100
	dc.w	L0012d8-*
L006c8e:
	bcc.b	L006c98
L006c90:
	lea	L006682(pc),a0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006c94:
	dc.w	$6100
	dc.w	Q9_panic_report_7f6-*
L006c98:
	add.l	a2,d0
L006c9a:
	move.l	d0,$60(a6)
L006c9e:
	move.w	$66(a5),d0
L006ca2:
	cmp.w	L0064bc(pc),d0
L006ca6:
	bcc.b	L006cb4
L006ca8:
	lea	L006602(pc),a0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006cac:
	dc.w	$6100
	dc.w	L00084a-*
L006cb0:
	move.w	L0064bc(pc),d0
L006cb4:
	mulu.w	#$20,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006cb8:
	dc.w	$6100
	dc.w	L0012d8-*
L006cbc:
	bcc.b	L006cc6
L006cbe:
	lea	L00677c(pc),a0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006cc2:
	dc.w	$6100
	dc.w	Q9_panic_report_7f6-*
L006cc6:
	move.l	a2,$3cc(a6)
L006cca:
	adda.l	d0,a2
L006ccc:
	move.l	a2,$3d0(a6)
L006cd0:
	tst.l	$764(a6)
L006cd4:
	bne.b	L006cda
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006cd6:
	dc.w	$6100
	dc.w	L00119c-*
L006cda:
	movea.l	$8(a4),a0
L006cde:
	movec	a0,msp
L006ce2:
	move	sr,d0
L006ce4:
	bset.l	#$c,d0
L006ce8:
	movea.l	$60(a6),a0
L006cec:
	movec	a0,isp
L006cf0:
	move	d0,sr
L006cf2:
	move.l	$7c(a6),d7
L006cf6:
	move.l	$79c(a6),d6
L006cfa:
	move.l	$70(a6),d5
L006cfe:
	movem.l	d7/d6/d5,-(sp)
L006d02:
	andi	#-$701,sr
L006d06:
	move.w	$78(a5),d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006d0a:
	dc.w	$6100
	dc.w	L006e3e-*
L006d0e:
	move.w	$76(a5),d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006d12:
	dc.w	$6100
	dc.w	L006e3e-*
L006d16:
	move.w	#$4afc,$0(a6)
L006d1c:
	move.w	$44(a5),d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006d20:
	dc.w	$6100
	dc.w	L006e3e-*
L006d24:
	movem.l	(sp)+,d5/d6/d7
L006d28:
	cmp.l	$79c(a6),d6
L006d2c:
	bne.b	L006d34
L006d2e:
	cmp.l	$70(a6),d5
L006d32:
	beq.b	L006d40
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006d34:
	dc.w	$6100
	dc.w	L006eee-*
L006d38:
	movea.l	$44(a6),a0
L006d3c:
	move.w	d0,$2(a0)
L006d40:
	cmp.l	$7c(a6),d7
L006d44:
	beq.b	L006d4a
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006d46:
	dc.w	$6100
	dc.w	L006376-*
L006d4a:
	moveq	#$0,d0
L006d4c:
	move.w	$3e(a5),d0
L006d50:
	bne.b	L006d62
L006d52:
	move.w	#$eb,d1
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006d56:
	dc.w	$6100
	dc.w	L006eb6-*
L006d5a:
	lea	L0066a3(pc),a0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006d5e:
	dc.w	$6100
	dc.w	Q9_panic_report_7f6-*
L006d62:
	lea	$0(a5,d0.l*1),a0
L006d66:
	movem.l	a4/a0,-(sp)
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006d6a:
	dc.w	$6100
	dc.w	L0032fa-*
L006d6e:
	bcs.b	L006d7e
L006d70:
	tst.b	d0
L006d72:
	bne.b	L006d7e
L006d74:
	move.w	#$101,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006d78:
	dc.w	$6100
	dc.w	L002fa4-*
L006d7c:
	bcc.b	L006d9c
L006d7e:
	moveq	#$0,d0
L006d80:
	movea.l	(sp),a0
L006d82:
	trap	#$0
* Rohbytes statt Instruktion (ori.b #-0x32,D1b) -- siehe FORCE_RAW_BYTES im Konverter
L006d84:
	dc.b	$00,$01,$65,$ce
L006d88:
	movea.l	(sp),a1
L006d8a:
	move.b	-(a0),d0
L006d8c:
	cmpa.l	a0,a1
L006d8e:
	beq.b	L006d9c
L006d90:
	cmpi.b	#$2f,d0
L006d94:
	bne.b	L006d8a
L006d96:
	lea	$1(a0),a0
L006d9a:
	move.l	a0,(sp)
L006d9c:
	movem.l	$3c(a6),a0/a1
L006da2:
	cmpa.l	$0(a0),a2
L006da6:
	beq.b	L006db2
L006da8:
	lea	$10(a0),a0
L006dac:
	cmpa.l	a1,a0
L006dae:
	bcs.b	L006da2
L006db0:
	bra.b	L006db6
L006db2:
	subq.w	#$1,$c(a0)
L006db6:
	movem.l	(sp)+,a0/a4
L006dba:
	moveq	#$0,d2
L006dbc:
	moveq	#$0,d0
L006dbe:
	move.w	$3c(a5),d0
L006dc2:
	beq.b	L006dce
L006dc4:
	lea	$0(a5,d0.l*1),a1
L006dc8:
	addq.l	#$1,d2
L006dca:
	tst.b	(a1)+
L006dcc:
	bne.b	L006dc8
L006dce:
	lea	$0(a5,d0.l*1),a1
L006dd2:
	move.w	#$101,d0
L006dd6:
	moveq	#$0,d1
L006dd8:
	moveq	#$3,d3
L006dda:
	moveq	#$0,d4
L006ddc:
	trap	#$0
* Rohbytes statt Instruktion (ori.b #0x0,D3b) -- siehe FORCE_RAW_BYTES im Konverter
L006dde:
	dc.b	$00,$03,$65,$00
L006de2:
	dc.b	$ff
L006de3:
	dc.b	$74
L006de4:
	dc.b	$61
L006de5:
	dc.b	$00
L006de6:
	dc.b	$bf
L006de7:
	dc.b	$08
L006de8:
	dc.b	$64
L006de9:
	dc.b	$06
L006dea:
	dc.b	$61
L006deb:
	dc.b	$00
L006dec:
	dc.b	$00
L006ded:
	dc.b	$ca
L006dee:
	dc.b	$60
L006def:
	dc.b	$04
L006df0:
	dc.b	$42
L006df1:
	dc.b	$69
L006df2:
	dc.b	$00
L006df3:
	dc.b	$02
L006df4:
	dc.b	$70
L006df5:
	dc.b	$ff
L006df6:
	dc.b	$39
L006df7:
	dc.b	$40
L006df8:
	dc.b	$00
L006df9:
	dc.b	$18
L006dfa:
	dc.b	$39
L006dfb:
	dc.b	$40
L006dfc:
	dc.b	$00
L006dfd:
	dc.b	$1a
L006dfe:
	dc.b	$08
L006dff:
	dc.b	$2e
L006e00:
	dc.b	$00
L006e01:
	dc.b	$05
L006e02:
	dc.b	$00
L006e03:
	dc.b	$2e
L006e04:
	dc.b	$66
L006e05:
	dc.b	$0c
L006e06:
	dc.b	$70
L006e07:
	dc.b	$00
L006e08:
	dc.b	$22
L006e09:
	dc.b	$3c
L006e0a:
	dc.b	$07
L006e0b:
	dc.b	$6c
L006e0c:
	dc.b	$00
L006e0d:
	dc.b	$00
L006e0e:
	dc.b	$4e
L006e0f:
	dc.b	$40
L006e10:
	dc.b	$00
L006e11:
	dc.b	$16
L006e12:
	dc.b	$3d
L006e13:
	dc.b	$7c
L006e14:
	dc.b	$00
L006e15:
	dc.b	$01
L006e16:
	dc.b	$00
L006e17:
	dc.b	$02
L006e18:
	dc.b	$30
L006e19:
	dc.b	$2c
L006e1a:
	dc.b	$01
L006e1b:
	dc.b	$68
L006e1c:
	dc.b	$67
L006e1d:
	dc.b	$1c
L006e1e:
	dc.b	$4e
L006e1f:
	dc.b	$40
L006e20:
	dc.b	$00
L006e21:
	dc.b	$8f
L006e22:
	dc.b	$30
L006e23:
	dc.b	$2c
L006e24:
	dc.b	$01
L006e25:
	dc.b	$6a
L006e26:
	dc.b	$4e
L006e27:
	dc.b	$40
L006e28:
	dc.b	$00
L006e29:
	dc.b	$8f
L006e2a:
	dc.b	$30
L006e2b:
	dc.b	$2c
L006e2c:
	dc.b	$01
L006e2d:
	dc.b	$6c
L006e2e:
	dc.b	$4e
L006e2f:
	dc.b	$40
L006e30:
	dc.b	$00
L006e31:
	dc.b	$8f
L006e32:
	dc.b	$42
L006e33:
	dc.b	$ac
L006e34:
	dc.b	$01
L006e35:
	dc.b	$68
L006e36:
	dc.b	$42
L006e37:
	dc.b	$6c
L006e38:
	dc.b	$01
L006e39:
	dc.b	$6c
L006e3a:
	dc.b	$60
L006e3b:
	dc.b	$00
L006e3c:
	dc.b	$c3
L006e3d:
	dc.b	$04
L006e3e:
	andi.l	#$ffff,d0
L006e44:
	beq.b	L006eb4
L006e46:
	lea	$0(a5,d0.l*1),a1
L006e4a:
	move.b	(a1)+,d0
L006e4c:
	beq.b	L006eb4
L006e4e:
	cmpi.b	#$20,d0
L006e52:
	beq.b	L006e4a
L006e54:
	cmpi.b	#$2c,d0
L006e58:
	beq.b	L006e4a
L006e5a:
	cmpi.b	#$d,d0
L006e5e:
	beq.b	L006e4a
L006e60:
	lea	-$1(a1),a0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006e64:
	dc.w	$6100
	dc.w	L0032f0-*
L006e68:
	bcs.b	L006e4a
L006e6a:
	movem.l	a4/a1,-(sp)
L006e6e:
	move.w	#$c01,d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006e72:
	dc.w	$6100
	dc.w	L002fa4-*
L006e76:
	movem.l	(sp)+,a1/a4
L006e7a:
	bcs.b	L006e4a
L006e7c:
	tst.w	$8(a2)
L006e80:
	bne.b	L006eae
L006e82:
	move.l	$38(a2),d0
L006e86:
	movea.l	d0,a3
L006e88:
	beq.b	L006e94
L006e8a:
	movea.l	a2,a3
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006e8c:
	dc.w	$6100
	dc.w	L0012d8-*
L006e90:
	bcs.b	L006ea4
L006e92:
	exg	a2,a3
L006e94:
	movem.l	a6/a5/a4/a3/a1/d7/d6/d5/d4/d3/d2,-(sp)
L006e98:
	adda.l	$30(a2),a2
L006e9c:
	jsr	(a2)
L006e9e:
	movem.l	(sp)+,d2/d3/d4/d5/d6/d7/a1/a3/a4/a5/a6
L006ea2:
	bcc.b	L006e4a
L006ea4:
	bsr.b	L006eb6
L006ea6:
	lea	L0066c6(pc),a0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006eaa:
	dc.w	$6100
	dc.w	Q9_panic_report_7f6-*
L006eae:
	move.w	#$a4,d1
L006eb2:
	bra.b	L006ea4
L006eb4:
	rts
L006eb6:
	movem.l	a1/a0/d2/d1/d0,-(sp)
L006eba:
	move	sr,d2
L006ebc:
	ori	#$700,sr
L006ec0:
	lea	L006f30(pc),a0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006ec4:
	dc.w	$6100
	dc.w	Q9_console_puts_850-*
L006ec8:
	lea	L006ee4(pc),a0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006ecc:
	dc.w	$6100
	dc.w	Q9_console_puts_850-*
L006ed0:
	move.w	$6(sp),d0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006ed4:
	dc.w	$6100
	dc.w	Q9_console_puthex_868-*
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006ed8:
	dc.w	$6100
	dc.w	L00084c-*
L006edc:
	move	d2,sr
L006ede:
	movem.l	(sp)+,d0/d1/d2/a0/a1
L006ee2:
	rts
L006ee4:
	dc.b	$3a,$20,$45,$72,$72,$6f,$72,$20,$24,$00
L006eee:
	move.l	#$400,d2
L006ef4:
	add.l	$79c(a6),d2
L006ef8:
	moveq	#$0,d0
L006efa:
	move.w	$8fe(a6),d0
L006efe:
	add.l	d2,d0
L006f00:
	move.l	$70(a6),d1
L006f04:
	add.l	d1,d0
L006f06:
	subq.l	#$1,d0
L006f08:
	neg.l	d1
L006f0a:
	and.l	d1,d0
L006f0c:
	neg.l	d1
L006f0e:
	move.l	d0,d1
L006f10:
	swap	d1
L006f12:
	tst.w	d1
L006f14:
	bne.b	L006f24
L006f16:
	swap	d1
L006f18:
	sub.l	d2,d1
L006f1a:
	move.w	d1,$8fe(a6)
L006f1e:
	move.w	d0,$3e2(a6)
L006f22:
	rts
L006f24:
	lea	L006526(pc),a0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006f28:
	dc.w	$6100
	dc.w	Q9_panic_report_7f6-*
L006f2c:
	trapf.w	#$0
* Rohbytes statt Instruktion (bmi.b 0x00006f97) -- siehe FORCE_RAW_BYTES im Konverter
L006f30:
	dc.b	$6b,$65
L006f32:
	dc.b	$72
L006f33:
	dc.b	$6e
L006f34:
	dc.b	$65
L006f35:
	dc.b	$6c
L006f36:
	dc.b	$00
L006f37:
	dc.b	$00
L006f38:
	dc.b	$00
L006f39:
	dc.b	$a8
L006f3a:
	dc.b	$db
L006f3b:
	dc.b	$7f
	endsect
