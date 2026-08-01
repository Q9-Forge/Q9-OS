* Q9-OS eigener Nachbau des OS-9/68K-Kernels (dker030s), 68030,
* Development/Standard-Allocator. Byte-exakt per Disassemblierung
* rekonstruiert, siehe docs/REVERSE_ENGINEERING.md und docs/REBUILD.md.
* Eigenstaendige Quelle, keine Uebernahme von Microware-Originaltext.

	nam	kernel
	psect	kernel,3073,40960,375,0,_start,0

_start:
	bra.w	q9_boot_init

* Bislang unbekanntes Wortpaar direkt nach dem Sprung, vor dem
* eingebetteten ID-String -- Bedeutung noch nicht geklaert, Bytes
* exakt aus dem Original uebernommen (siehe REVERSE_ENGINEERING.md).
	dc.w	$0001
	dc.w	$09be

* Eingebetteter Identifikations-String (reine Fakteninformation aus
* dem Original-Modul: Zielsystem/Version/Copyright-Datum -- kein
* geschuetzter Kernel-Code, sondern eine Datenkonstante).
	dc.b	"68030"
	dc.b	0
	dc.b	" OS-9/68K Kernel (Dev-Std) V3.2.0"
	dc.b	0
	dc.b	"Copyright (c) 1999 by Microware Systems Corp."
	dc.b	0


q9_boot_init:
	rts	* PLATZHALTER, noch nicht nachgebaut

	endsect
