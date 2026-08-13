/* Ghidra-Pseudo-C-Dekompilierung des x86-Boot-Bootstraps, mit den in
 * dieser Runde vergebenen Q9X_-Namen. Kein handgeschriebener/kompilier-
 * barer C-Code -- reine Ghidra-Ausgabe (undefined4/uVar/unaff_* etc.
 * sind Ghidra-Artefakte), aber deutlich lesbarer als reines Assembler.
 *
 * WICHTIGER HINWEIS zu Q9X_kernel_globals_init: Ghidra dekompiliert die
 * letzten beiden Aufrufe (FUN_0021e5a0/FUN_0021f680) als normale
 * Funktionsaufrufe mit "return" danach -- das ist IRREFUEHREND. Die
 * Rohdisassemblierung von FUN_0021e5a0 zeigt:
 *
 *     XCHG EAX,ESP      ; tauscht EAX und ESP
 *     MOV EAX,[EAX]     ; liest von der (jetzt alten) Stackadresse
 *     JMP EAX
 *
 * Das ist ein manueller STACK-SWITCH: EAX wird vorher mit der Adresse
 * eines neu vorbereiteten Stack-Rahmens geladen; nach dem XCHG läuft die
 * CPU auf diesem neuen Stack weiter, und der JMP springt zu einer
 * Adresse, die zuvor an dessen "Spitze" abgelegt wurde -- exakt das
 * x86-Aequivalent zum manuell gebauten Rueckspruengkontext, den der
 * 68K-Kernel per BRA/JMP-Ueberschreibung auf dem eigenen Stack aufbaut
 * (siehe README.md, Abschnitt "pcVar2"). "pcVar2" in
 * Q9X_kernel_init ist letztlich das Ergebnis dieser Kette, kein simpler
 * Rueckgabewert -- Ghidra kann das nicht als Sprung modellieren, deshalb
 * hier zusaetzlich die rohe Disassemblierung von FUN_0021e5a0 abgedruckt.
 *
 * HINWEIS ZUR KOMPILIERBARKEIT: Dies ist reine Ghidra-Pseudo-C-Ausgabe,
 * keine handgeschriebene, tatsaechlich kompilierbare Quelle. Die
 * folgenden Typedefs bilden nur Ghidras eigene Pseudo-Typen
 * (undefined1/2/4/8, code, uint, ushort, byte) auf Standard-C ab, damit
 * die Datei wenigstens SYNTAKTISCH parsebar ist. Alle referenzierten
 * Ghidra-Adress-/Label-Symbole (DAT_, LAB_) und alle nicht umbenannten
 * FUN_-Hilfsfunktionen werden unten pauschal vorwaertsdeklariert, damit
 * kein Editor/Compiler an unbekannten Bezeichnern haengen bleibt --
 * echte Verlinkung waere ohne die komplette Ghidra-Symboltabelle
 * ohnehin nicht moeglich. Ziel ist Lesbarkeit, nicht Uebersetzbarkeit.
 */

typedef unsigned char undefined;
typedef unsigned char undefined1;
typedef unsigned char byte;
typedef unsigned char bool;
typedef unsigned short undefined2;
typedef unsigned short ushort;
typedef unsigned int undefined4;
typedef unsigned int uint;
typedef unsigned long long undefined8;
typedef int (*code)();   /* alte K&R-Form: unspezifizierte Parameterzahl,
                           * da dieselbe "code"-Zeiger-Variable je nach
                           * Aufrufstelle mit unterschiedlicher Argumentzahl
                           * benutzt wird (Ghidra kennt keinen einheitlichen
                           * Funktionszeiger-Typ fuer diese Aufrufe). */
#define true 1
#define false 0
#define CONCAT44(hi, lo) (((unsigned long long)(hi) << 32) | (unsigned int)(lo))
#define CONCAT22(hi, lo) (((unsigned int)(hi) << 16) | (unsigned short)(lo))

/* Vorwaertsdeklarationen aller referenzierten Ghidra-Symbole, die NICHT
 * weiter unten in dieser Datei definiert werden -- alte K&R-Schreibweise
 * "int f();" (leere Klammern = unspezifizierte Parameter), rein damit
 * kein Compiler/Linter an unbekannten Bezeichnern haengen bleibt. Q9X_-
 * Funktionen, die unten eine echte Definition haben (Q9X_kernel_init,
 * Q9X_module_check_reloc, Q9X_kernel_globals_init, FUN_0021f680), werden
 * hier bewusst NICHT vorab deklariert (sonst Typkonflikt). */
int FUN_0021e5a0(); int FUN_0021f0a4(); int FUN_0021f106(); int FUN_0021f1e0();
int FUN_0021f802(); int FUN_0021fe70(); int FUN_0021ff2c();
int FUN_00220ed2(); int FUN_00221218(); int FUN_00221642(); int FUN_00221fa8();
int FUN_0022264a(); int FUN_0022458c(); int FUN_00224ea8(); int FUN_00227a2e();
int FUN_0022e9ba(); int FUN_0022ea38();
int Q9X_device_module_init_loop(); int Q9X_dispatch_table_build();
int Q9X_query_memsize();
int Q9X_trivial_ret_stub();
extern int DAT_0021e400, DAT_0021e430, DAT_0021ea16;
extern int LAB_0022151c, LAB_00221522, LAB_0022e9f0;

/* Diese drei WERDEN unten definiert -- hier nur mit passendem
 * Rueckgabetyp, aber unspezifizierter Parameterliste vorwaerts-
 * deklariert (K&R-Stil), weil Ghidra an verschiedenen Aufrufstellen
 * leicht unterschiedliche, nicht ganz konsistente Parametertypen
 * herleitet -- ein bekanntes Dekompilierungs-Artefakt, keine
 * Inkonsistenz in dieser Abschrift: */
undefined8 Q9X_module_check_reloc();
/* Ghidra dekompiliert Q9X_kernel_globals_init eigentlich als "void" --
 * hier bewusst zu "undefined4" (generischer 32-Bit-Wert) geaendert,
 * weil die Aufrufstelle in Q9X_kernel_init das Ergebnis als Wert
 * (pcVar2) weiterverwendet. Genau diese Diskrepanz IST der Kernbefund
 * dieses Themas (s. Hinweis oben) -- keine Abschreibkorrektur, sondern
 * die einzige Stelle, an der "authentisches Ghidra-void" und "tatsaechlich
 * wird ein Wert weitergegeben" syntaktisch nicht beides gleichzeitig
 * gehen; fuer Lesbarkeit hier zugunsten des zweiten aufgeloest. */
undefined4 Q9X_kernel_globals_init();
undefined8 FUN_0021f680();

/* === DECOMPILE Q9X_kernel_init @ 0021e4c0 === */
/* WARNING: Unable to track spacebase fully for stack */

void Q9X_kernel_init(void)

{
  int in_EAX;
  int iVar1;
  code *pcVar2;
  undefined4 extraout_ECX;
  undefined1 *puVar3;
  int iVar4;

  if ((*(uint *)(*(int *)(*(int *)(in_EAX + 0x90) + 0x2c) + 0x10) & 0x10) == 0) {
    Q9X_trivial_ret_stub();
  }
  iVar4 = DAT_0021e430 + 4;
  iVar1 = Q9X_query_memsize();
  iVar4 = iVar1 + iVar4;
  *(int *)(iVar4 + -4) = iVar4;
  Q9X_module_check_reloc(iVar4);
  *(int *)(iVar4 + 0xe8) = iVar1;
  *(int *)(iVar4 + -8) = iVar4 + -4;
  *(undefined4 *)(iVar4 + -0xc) = 0x21e51e;     /* Ruecksprungadresse #1 einprogrammiert */
  pcVar2 = (code *)Q9X_kernel_globals_init();   /* siehe Hinweis oben -- kein normaler Rueckgabewert */
  *pcVar2 = (code)((char)*pcVar2 + (char)pcVar2); /* Ghidra-Artefakt, keine echte Operation */
  iVar1 = *(int *)(iVar4 + -4);
  if (*(int *)(iVar4 + -4) == 0) {
    iVar1 = iVar4;
  }
  *(undefined4 *)(iVar4 + -0xc) = extraout_ECX;
  *(int *)(iVar4 + -0x10) = iVar4;
  *(int *)(iVar4 + -0x14) = iVar1;
  puVar3 = (undefined1 *)(iVar4 + -0x18);
  *(undefined4 *)(iVar4 + -0x18) = 0x21e536;    /* Ruecksprungadresse #2 einprogrammiert */
  iVar1 = (*pcVar2)();                           /* Aufruf durch den Funktionszeiger */
  if (iVar1 != 0) {
    *(int *)(puVar3 + 0x10) = iVar1;
    *(undefined4 *)(puVar3 + 0xc) = 0x21e54b;
    *(undefined4 *)(puVar3 + 0xc) = 0x21e557;
    FUN_0021f106();
  }
  return;
}


/* === DECOMPILE Q9X_module_check_reloc @ 0022246c === */
undefined8 Q9X_module_check_reloc(undefined2 *param_1)

{
  short *in_EAX;
  int iVar1;
  undefined4 uVar2;
  int extraout_ECX;
  undefined2 *puVar3;
  int extraout_ECX_00;
  undefined4 in_EDX;
  uint uVar4;
  uint extraout_EDX;
  uint extraout_EDX_00;
  uint uVar5;
  int *piVar6;
  int unaff_EBX;
  uint *puVar7;
  uint *puVar8;

  if (*in_EAX == 0x4afc) {                       /* Modul-Sync pruefen */
    iVar1 = FUN_00227a2e();                       /* Header-Checksumme */
    if (iVar1 == 0) {
      puVar7 = (uint *)((int)in_EAX + *(int *)(in_EAX + 0x1a));
      uVar4 = *puVar7;
      if ((uVar4 >> 1 & 1) != 0) {
        *param_1 = 0;
        uVar4 = uVar4 - 2;
      }
      FUN_0022ea38(0,uVar4);
      puVar8 = puVar7 + 2;
      puVar3 = (undefined2 *)(extraout_ECX + (extraout_EDX & 0xfffffffc));
      uVar4 = puVar7[1];
      if ((uVar4 >> 1 & 1) != 0) {
        *puVar3 = (short)*puVar8;
        puVar3 = puVar3 + 1;
        puVar8 = (uint *)((int)puVar7 + 10);
        uVar4 = uVar4 - 2;
      }
      FUN_00221642(puVar3,uVar4);
      puVar3 = (undefined2 *)(extraout_ECX_00 + (extraout_EDX_00 >> 2) * 4);
      puVar8 = puVar8 + (extraout_EDX_00 >> 2);
      uVar4 = (int)param_1 + (*(int *)(in_EAX + 0x16) - (int)puVar3);
      if (uVar4 != 0) {
        if ((uVar4 >> 1 & 1) != 0) {
          *puVar3 = 0;
          uVar4 = uVar4 - 2;
        }
        FUN_0022ea38(0,uVar4);
      }
      while( true ) {                             /* Relozierungstabelle 1 durchlaufen */
        uVar4 = *puVar8;
        puVar7 = puVar8 + 1;
        uVar5 = (uint)*(ushort *)((int)puVar8 + 2);
        puVar8 = puVar7;
        if (uVar5 == 0) break;
        for (; uVar5 != 0; uVar5 = uVar5 - 1) {
          piVar6 = (int *)((uint)(ushort)uVar4 * 0x10000 + (uint)(ushort)*puVar8 + (int)param_1);
          *piVar6 = *piVar6 + (int)in_EAX;         /* Basisadresse aufaddieren */
          puVar8 = (uint *)((int)puVar8 + 2);
        }
      }
      while( true ) {                              /* Relozierungstabelle 2 durchlaufen */
        uVar4 = *puVar7;
        uVar5 = (uint)*(ushort *)((int)puVar7 + 2);
        puVar7 = (uint *)((int)puVar7 + 2);
        if (uVar5 == 0) break;
        for (; puVar7 = (uint *)((int)puVar7 + 2), uVar5 != 0; uVar5 = uVar5 - 1) {
          piVar6 = (int *)((uint)(ushort)uVar4 * 0x10000 + (uint)*(ushort *)puVar7 + (int)param_1);
          *piVar6 = *piVar6 + (int)param_1;
        }
      }
      if (*(code **)(unaff_EBX + 0xa50) != (code *)0x0) {
        (**(code **)(unaff_EBX + 0xa50))(param_1,*(undefined4 *)(in_EAX + 0x16)); /* Post-Load-Hook */
      }
      uVar2 = 0;                                    /* Erfolg */
    }
    else {
      uVar2 = 0xec;                                 /* Checksummenfehler */
    }
  }
  else {
    uVar2 = 0xcd;                                   /* kein gueltiges Modul */
  }
  return CONCAT44(in_EDX,uVar2);
}


/* === DECOMPILE Q9X_kernel_globals_init @ 0021eb26 === */
/* Original-Ghidra-Signatur war "void Q9X_kernel_globals_init(undefined4 param_1)" --
 * Rueckgabetyp hier zu undefined4 geaendert, s. Hinweis bei der Vorwaertsdeklaration oben. */
undefined4 Q9X_kernel_globals_init(undefined4 param_1)

{
  int in_EAX;
  int iVar1;
  int iVar2;
  undefined4 *puVar3;
  int iVar4;
  undefined4 *puVar5;
  int extraout_ECX;
  int *piVar6;
  int iVar7;
  uint *puVar8;
  undefined2 *unaff_EBX;
  bool bVar9;
  undefined4 in_stack_ffffffcc;
  undefined2 uVar10;
  uint local_18;

  uVar10 = (undefined2)((uint)in_stack_ffffffcc >> 0x10);
  *(undefined4 *)(unaff_EBX + 0x1a) = *(undefined4 *)(in_EAX + 0x28);  /* Boot-Parameter-Felder in Kernel-Globals kopieren */
  *(int *)(unaff_EBX + 0x36) = in_EAX;                                  /* Rueckzeiger auf die Boot-Parameter-Struktur */
  *(undefined4 *)(unaff_EBX + 0x10) = *(undefined4 *)(in_EAX + 0x30);
  *(undefined4 *)(unaff_EBX + 0x12) = *(undefined4 *)(in_EAX + 0x6c);
  *(undefined4 *)(unaff_EBX + 0x502) = *(undefined4 *)(in_EAX + 0x14);
  *(undefined4 *)(unaff_EBX + 0x76) = param_1;
  *(undefined4 *)(unaff_EBX + 0x1e) = 0x10;                             /* Alignment-Konstante 16, wie beim 68K */
  *(undefined4 *)(unaff_EBX + 0x1c) = 0x100;
  *(undefined2 **)(unaff_EBX + 0x548) = unaff_EBX + 0x58c;
  *(undefined1 **)(unaff_EBX + 0x500) = &LAB_0022e9f0;
  *(code **)(unaff_EBX + 0x546) = FUN_00220ed2;
  /* Vier leere zirkulaere Doppel-Listen initialisieren (Kopf/Schwanz = sich selbst): */
  *(undefined2 **)(unaff_EBX + 0x48) = unaff_EBX + 0x20;
  *(undefined2 **)(unaff_EBX + 0x46) = unaff_EBX + 0x20;
  *(undefined2 **)(unaff_EBX + 0x50) = unaff_EBX + 0x28;
  *(undefined2 **)(unaff_EBX + 0x4e) = unaff_EBX + 0x28;
  *(undefined2 **)(unaff_EBX + 0x4c) = unaff_EBX + 0x24;
  *(undefined2 **)(unaff_EBX + 0x4a) = unaff_EBX + 0x24;
  *(undefined2 **)(unaff_EBX + 100) = unaff_EBX + 0x5e;
  *(undefined2 **)(unaff_EBX + 0x62) = unaff_EBX + 0x5e;
  iVar1 = FUN_0021f0a4();                                                /* Modul-Scanner (findet weitere Module im Speicher) */
  *(int *)(unaff_EBX + 0x34) = iVar1;
  if (iVar1 == 0) {
    uVar10 = 0x21;
    FUN_0021f106(0,0);                                                   /* Panic/Fehlerpfad */
  }
  iVar4 = *(int *)(unaff_EBX + 0x34) + *(int *)(*(int *)(unaff_EBX + 0x34) + 0x24);
  unaff_EBX[0x15] = *(undefined2 *)(iVar4 + 0x48);
  unaff_EBX[0x16] = *(undefined2 *)(iVar4 + 0x4a);
  unaff_EBX[0x27] = *(undefined2 *)(iVar4 + 0x44);
  unaff_EBX[0x26] = *(undefined2 *)(iVar4 + 0x44);
  unaff_EBX[0x14] = (ushort)*(byte *)(iVar4 + 0x56);
  unaff_EBX[0x18] = *(undefined2 *)(iVar4 + 0x4c);
  unaff_EBX[0x17] = *(undefined2 *)(iVar4 + 0x58);
  FUN_0021fe70();
  iVar2 = Q9X_query_memsize(in_EAX);                                     /* Speichergroesse erneut abfragen */
  *(int *)(unaff_EBX + 0x40) = iVar2;
  *(int *)(unaff_EBX + 0x42) = iVar2;
  *(int *)(unaff_EBX + 0x47c) = iVar2 + 0x464a;                          /* fester Bereich (0x464A Byte) */
  iVar1 = iVar2 + 0x464a + (uint)(ushort)unaff_EBX[0x18] * 0x10;
  *(int *)(unaff_EBX + 0x47e) = iVar1;                                   /* erstes 16-Byte-Array (Prozess-Deskriptoren) */
  iVar1 = iVar1 + (uint)(ushort)unaff_EBX[0x18] * 0x10;
  *(int *)(unaff_EBX + 0x3a) = iVar1;                                    /* zweites 16-Byte-Array (Pfad-Deskriptoren) */
  *(undefined2 *)(iVar1 + 8) = 0x777;                                    /* Freiliste-Sentinel */
  iVar1 = *(int *)(unaff_EBX + 0x3a);
  iVar7 = *(int *)(unaff_EBX + 0x3a);
  *(int *)(iVar1 + 0x30) = iVar1;
  *(int *)(iVar7 + 0x34) = iVar1;
  **(undefined4 **)(unaff_EBX + 0x3a) = (undefined4)(unsigned long)&DAT_0021ea16;  /* Cast ergaenzt, s. Datei-Kopfkommentar */
  *(int *)(iVar2 + 0x14) = iVar2 + 0x416e;
  *(undefined4 *)(iVar2 + 4) = 1;
  *(undefined **)(*(int *)(iVar2 + 0x14) + 0xc) = &DAT_0021e400;
  *(int *)(iVar2 + 0x50) = iVar2;
  *(int *)(iVar2 + 0x4c) = iVar2;
  *(undefined4 *)(*(int *)(iVar2 + 0x14) + 0x158) = 0x80;
  *(undefined4 *)(*(int *)(iVar2 + 0x14) + 4) = 1;
  *(undefined4 *)(*(int *)(iVar2 + 0x14) + 0x1f8) = 0x80;
  *(undefined4 *)(*(int *)(iVar2 + 0x14) + 0x45c) = 0x80;
  iVar7 = *(int *)(iVar2 + 0x14);
  iVar1 = *(int *)(iVar2 + 0x14) + 0x42e;
  *(int *)(*(int *)(iVar2 + 0x14) + 0x454) = iVar1;
  *(int *)(iVar7 + 0x450) = iVar1;
  iVar1 = *(int *)(iVar2 + 0x14);
  iVar7 = *(int *)(iVar2 + 0x14);
  *(undefined4 *)(iVar1 + 0x1c) = *(undefined4 *)(unaff_EBX + 0x3a);
  *(undefined4 *)(iVar7 + 0x18) = *(undefined4 *)(iVar1 + 0x1c);
  puVar3 = *(undefined4 **)(unaff_EBX + 0x47e);
  puVar5 = *(undefined4 **)(unaff_EBX + 0x47c);
  for (local_18 = 0; local_18 < (ushort)unaff_EBX[0x18]; local_18 = local_18 + 1) {  /* beide Deskriptor-Tabellen als frei markieren */
    *puVar5 = (undefined4)(unsigned long)&LAB_0022151c;  /* Cast ergaenzt, s. Datei-Kopfkommentar */
    *puVar3 = (undefined4)(unsigned long)&LAB_0022151c;  /* Cast ergaenzt, s. Datei-Kopfkommentar */
    *(undefined2 *)((int)puVar5 + 0xe) = 0xffff;
    *(undefined2 *)((int)puVar3 + 0xe) = 0xffff;
    puVar3 = puVar3 + 4;
    puVar5 = puVar5 + 4;
  }
  *(undefined4 *)(unaff_EBX + 0x78) = 0x808;
  *(undefined4 *)(unaff_EBX + 0x506) = 0;
  *(undefined1 **)(unaff_EBX + 0x50a) = &LAB_0022151c;
  *(undefined1 **)(unaff_EBX + 0x514) = &LAB_00221522;
  *(undefined1 **)(unaff_EBX + 0x516) = &LAB_00221522;
  *(undefined1 **)(unaff_EBX + 0x51c) = &LAB_00221522;
  *(undefined1 **)(unaff_EBX + 0x518) = &LAB_0022151c;
  *(undefined1 **)(unaff_EBX + 0x51a) = &LAB_0022151c;
  *(undefined1 **)(unaff_EBX + 0x528) = &LAB_0022151c;
  *(code **)(unaff_EBX + 0x54c) = FUN_0022e9ba;
  Q9X_dispatch_table_build(CONCAT22(uVar10,1),unaff_EBX + 0x962);        /* Exception-Dispatch-Tabelle, Teil 1 */
  Q9X_dispatch_table_build(2,unaff_EBX + 0xb70);                          /* Exception-Dispatch-Tabelle, Teil 2 */
  FUN_0021f1e0(in_EAX);
  piVar6 = *(int **)(in_EAX + 0x24);
  while (*piVar6 != 0) {                                                  /* Geraete-/Modul-Init-Schleife */
    Q9X_device_module_init_loop(piVar6[1],0);
    piVar6 = (int *)(extraout_ECX + 8);
  }
  FUN_0021ff2c();
  (**(code **)(in_EAX + 0x18))();                                        /* Funktionszeiger aus Boot-Parametern aufrufen */
  *unaff_EBX = 0x4afc;
  **(int **)(unaff_EBX + 0x36) = **(int **)(unaff_EBX + 0x36) + 1;
  if (*(int *)(*(int *)(unaff_EBX + 0x36) + 0x2c) != 0) {
    (**(code **)(in_EAX + 0x14))();
  }
  iVar1 = FUN_0022264a(unaff_EBX + 0x3e);
  bVar9 = iVar1 != 0;
  if (!bVar9) {
    iVar1 = FUN_0022264a(unaff_EBX + 0x38);
    bVar9 = iVar1 != 0;
    if (!bVar9) goto LAB_0021f026;
  }
  FUN_0021f106(bVar9,0);
LAB_0021f026:
  **(uint **)(unaff_EBX + 0x38) = (uint)*(ushort *)(iVar4 + 0x40);
  iVar1 = *(ushort *)(iVar4 + 0x40) - 2;
  iVar7 = 2;
  puVar8 = (uint *)(*(int *)(unaff_EBX + 0x38) + 8);
  while (iVar1 != 0) {                                                   /* getaggte Freiliste aufbauen: (index*2)|1 */
    *puVar8 = iVar7 * 2 | 1;
    iVar1 = iVar1 + -1;
    iVar7 = iVar7 + 1;
    puVar8 = puVar8 + 1;
  }
  *puVar8 = 1;
  **(uint **)(unaff_EBX + 0x3e) = (uint)*(ushort *)(iVar4 + 0x3c);
  *(int *)(*(int *)(unaff_EBX + 0x3e) + 4) = iVar2;
  FUN_0021e5a0();     /* <-- Stack-Switch-Trampolin, siehe Hinweis oben und README.md */
  return FUN_0021f680();     /* Ghidra haengt diesen Aufruf noch an -- tatsaechlich nicht mehr erreicht;
                                * "return" hier ergaenzt (Original war "FUN_0021f680(); return;" ohne
                                * Wert, s. Hinweis zum geaenderten Rueckgabetyp oben) */
}


/* === DISASM FUN_0021e5a0 (der Stack-Switch-Trampolin) ===
 * 0021e5a0: XCHG EAX,ESP
 * 0021e5a1: MOV EAX,dword ptr [EAX]
 * 0021e5a3: JMP EAX
 * === */


/* === DECOMPILE FUN_0021f680 (letzte "echte" Vorbereitung vor dem Sprung) === */
undefined8 FUN_0021f680(void)

{
  undefined4 in_EAX;
  int iVar1;
  int extraout_ECX;
  int extraout_ECX_00;
  int extraout_ECX_01;
  undefined4 in_EDX;
  char *pcVar2;
  int iVar3;
  int unaff_EBX;
  char *pcVar4;
  undefined4 in_stack_ffffffc4;
  undefined2 uVar5;
  undefined4 in_stack_ffffffc8;
  ushort uVar6;
  undefined1 local_1c [4];
  int local_18;
  undefined2 local_14;
  undefined2 local_12;
  undefined4 local_8;

  /* Modul-/Pfad-Namen aus der Boot-Parameter-Struktur zusammenbauen (mehrere
   * String-Laengen-Ermittlungsschleifen) und an FUN_00221fa8 uebergeben --
   * plausibel: initialen Pfad/Kommandozeile fuer den ersten Prozess aufbauen. */
  iVar3 = *(int *)(unaff_EBX + 0x68);
  iVar1 = iVar3 + *(int *)(iVar3 + 0x24);
  if (*(int *)(iVar1 + 0x5a) != 0) {
    pcVar2 = (char *)(iVar3 + *(int *)(iVar1 + 0x5a));
    do {
      if (*pcVar2 == '\0') break;
      pcVar2 = (char *)FUN_0021f802();
    } while (pcVar2 != (char *)0x0);
  }
  FUN_0022e9ba();
  iVar3 = extraout_ECX;
  if (*(int *)(extraout_ECX + 0x30) != 0) {
    FUN_0021f802();
    iVar3 = extraout_ECX_00;
  }
  uVar5 = (undefined2)((uint)in_stack_ffffffc4 >> 0x10);
  uVar6 = (ushort)((uint)in_stack_ffffffc8 >> 0x10);
  if (*(int *)(iVar3 + 0x24) != 0) {
    pcVar2 = (char *)(*(int *)(unaff_EBX + 0x68) + *(int *)(iVar3 + 0x24));
    do {
      uVar5 = (undefined2)((uint)in_stack_ffffffc4 >> 0x10);
      uVar6 = (ushort)((uint)in_stack_ffffffc8 >> 0x10);
      if (*pcVar2 == '\0') break;
      pcVar2 = (char *)FUN_0021f802();
      uVar5 = (undefined2)((uint)in_stack_ffffffc4 >> 0x10);
      uVar6 = (ushort)((uint)in_stack_ffffffc8 >> 0x10);
    } while (pcVar2 != (char *)0x0);
  }
  FUN_0022458c();
  pcVar2 = (char *)0x0;
  iVar3 = 0;
  if (*(int *)(extraout_ECX_01 + 0x18) != 0) {
    pcVar2 = (char *)(*(int *)(unaff_EBX + 0x68) + *(int *)(extraout_ECX_01 + 0x18));
    for (pcVar4 = pcVar2; *pcVar4 != '\0'; pcVar4 = pcVar4 + 1) {
      iVar3 = iVar3 + 1;
    }
    iVar3 = iVar3 + 1;
  }
  iVar3 = FUN_00221fa8(3,local_1c,*(int *)(unaff_EBX + 0x68) + *(int *)(extraout_ECX_01 + 0x14),
                       pcVar2,0,iVar3,CONCAT22(uVar5,0x101),(uint)uVar6 << 0x10,0);
  if (iVar3 != 0) {
    FUN_0021f106(iVar3,0);                                                /* Panic/Fehlerpfad */
  }
  FUN_00224ea8(&local_18);
  *(undefined4 *)(local_18 + 8) = 0;
  iVar3 = *(int *)(unaff_EBX + 0x84);
  *(undefined2 *)(*(int *)(unaff_EBX + 0x84) + 0x1e) = 0xffff;
  *(undefined2 *)(iVar3 + 0x1c) = 0xffff;
  *(int *)(unaff_EBX + 0xb0) = unaff_EBX + 0x9c;                          /* weitere zirkulaere Liste initialisiert */
  *(int *)(unaff_EBX + 0xac) = unaff_EBX + 0x9c;
  *(int *)(unaff_EBX + 0xa8) = unaff_EBX + 0x94;
  *(int *)(unaff_EBX + 0xa4) = unaff_EBX + 0x94;
  if ((*(ushort *)(unaff_EBX + 0x28) & 4) == 0) {
    local_14 = 0x32;
    local_12 = 1;
    local_8 = 0;
    FUN_00220ed2();
  }
  FUN_0021e5a0();       /* <-- HIER springt die Kontrolle tatsaechlich weg, s.o. */
  FUN_00221218();       /* von Ghidra faelschlich als "danach ausgefuehrt" gezeigt */
  return CONCAT44(in_EDX,in_EAX);
}
