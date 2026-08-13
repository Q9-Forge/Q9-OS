* Wortwoertlicher Auszug aus src/kernel/kernel.r: kompletter Boot-
* Bootstrap des 68K-Kernels, von Q9_kernel_init_67a0 (0x67a0, Ziel von
* M$Exec, siehe Thema 00) bis zum finalen Sprung in
* Q9_reschedule_trampolin_3140 (0x6e3a-0x6e3d) -- die Uebergabe der
* Kontrolle an den Scheduler/ersten Prozess. Praefix aller Funktionen:
* Q9_ (bereits etablierte Namen aus fruaheren Runden, hier unveraendert).
*
* Rohbyte-Bloecke (dc.b) sind Stellen, an denen der r68-Assembler eine
* kuerzere Instruktionsform gewaehlt haette als das Original (siehe
* BRANCH_WORD_RISK/FORCE_RAW_BYTES-Kommentare im Originalquelltext) --
* absichtlich als Rohbytes belassen, um Byte-Exaktheit zu garantieren.

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
	dc.w	Q9_module_name_match_32fa-*
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
*----------------------------------------------------------------------
* Q9_boot_finalize_6de4  (0x006de4)
* Letzter Abschnitt des Kernel-Bootstraps (Fortsetzung von Q9_kernel_init_67a0): Prozess-ID-Validierung via Q9_proc_id_lookup_2cee,
* invalidiert zwei Deskriptorfelder, bedingter TRAP-#0-Modulaufruf, setzt System-Global-Flag bei Offset 0x2, raeumt Tabellen-Slot-90-Bereich
* auf (0x168/0x16a/0x16c, je per TRAP #0), und endet mit Sprung in Q9_reschedule_trampolin_3140 -- der Kernel startet damit den Scheduler.
* Details/Kontext: docs/REVERSE_ENGINEERING.md
*----------------------------------------------------------------------
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
Q9_boot_finalize_6de4:
	dc.w	$6100
	dc.w	Q9_proc_id_lookup_2cee-*
L006de8:
	bcc.b	L006df0
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006dea:
	dc.w	$6100
	dc.w	L006eb6-*
L006dee:
	bra.b	L006df4
L006df0:
	clr.w	$2(a1)
L006df4:
	moveq	#-$1,d0
L006df6:
	move.w	d0,$18(a4)
L006dfa:
	move.w	d0,$1a(a4)
L006dfe:
	btst.b	#$5,$2e(a6)
L006e04:
	bne.b	L006e12
L006e06:
	moveq	#$0,d0
L006e08:
	move.l	#$76c0000,d1
L006e0e:
	trap	#$0
* Rohbytes statt Instruktion (ori.b #0x7c,(A6)) -- siehe FORCE_RAW_BYTES im Konverter
L006e10:
	dc.b	$00,$16,$3d,$7c
L006e14:
	ori.b	#$2,d1
L006e18:
	move.w	$168(a4),d0
L006e1c:
	beq.b	L006e3a
L006e1e:
	trap	#$0
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
* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK
L006e3a:
	dc.w	$6000
	dc.w	Q9_reschedule_trampolin_3140-*
