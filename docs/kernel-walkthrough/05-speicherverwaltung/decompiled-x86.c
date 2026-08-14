/* Ghidra-Pseudo-C-Dekompilierung der x86-Speicherverwaltung, mit den in
 * dieser Runde vergebenen Q9X_-Namen. Kein handgeschriebener/kompilier-
 * barer C-Code -- reine Ghidra-Ausgabe.
 *
 * WICHTIGSTER BEFUND: alle fuenf Fehlercode-Konstanten, die schon fuer
 * den 68K-Allokator (docs/REVERSE_ENGINEERING.md, Funde zu 0x5440/0x5a22/
 * 0x5bac) dokumentiert sind, tauchen hier UNVERAENDERT wieder auf:
 *   0xDB = "gehoert nicht zu diesem Pool"
 *   0xD2 = "Adresse gehoert zu keiner bekannten Region"
 *   0xAB = "keine Arena mit ausreichend freiem Speicher"
 *   0xED = "Pool/Arena-Liste voll bzw. leer"
 *   0xE1 = "Groesse 0"
 * Dieselbe "Template-Kopie in einen neu erzeugten Arena-Deskriptor"-
 * Technik wie beim 68K (dort 42 Byte per Kopierschleife, hier 0x40=64
 * Byte per 16x REP-artiger dword-Kopie) ist ebenfalls wortwoertlich
 * wiederzuerkennen (s. u., "for (iVar4 = 0x10; ...)").
 */

typedef unsigned char undefined;
typedef unsigned char undefined1;
typedef unsigned char bool;
typedef unsigned short undefined2;
typedef unsigned short ushort;
typedef unsigned int undefined4;
typedef unsigned int uint;
typedef unsigned long long undefined8;
typedef unsigned long long ulonglong;
#define CONCAT44(hi, lo) (((unsigned long long)(hi) << 32) | (unsigned int)(lo))

int FUN_002225ee(); int FUN_002228ac(); int FUN_00222a34();
int FUN_0022ea38(); int FUN_00222e9e();
/* Vorwaertsdeklaration, nur weil Q9X_arena_reserve_space unten VOR
 * seiner eigenen Definition aufgerufen wird -- Parametertypen an die
 * spaetere Definition angeglichen, damit clang -std=c99 nicht wegen
 * widerspruechlicher Deklarationen abbricht (reines Formalproblem,
 * kein inhaltlicher Fund). */
undefined8 Q9X_arena_reserve_space(uint *param_1, undefined2 param_2);

/* === Q9X_arena_lookup_or_create @ 0x222b90 (vormals FUN_00222b90) ===
 * Sucht die Arena, die einen Adressbereich abdeckt -- oder erzeugt eine
 * neue (per Template-Kopie aus einer Kandidatenregion), falls keine
 * passt. Zweiter Teil (ab LAB_00222d2a): Freilisten-Insert mit
 * Boundary-Tag-Coalescing -- strukturell identisch zum 68K-Fund bei
 * Q9_mem_free_5a22 (Nachbarblock-Verschmelzung statt neuem Eintrag).
 */
undefined8 Q9X_arena_lookup_or_create(uint *param_1,uint *param_2,int param_3)

{
  uint uVar1;
  uint uVar2;
  undefined4 *in_EAX;
  uint *puVar3;
  int iVar4;
  undefined4 in_EDX;
  undefined4 *puVar5;
  int unaff_EBX;
  uint *puVar6;
  bool bVar7;
  undefined8 uVar8;
  uint in_stack_ffffffb0;
  uint *local_3c;
  undefined4 local_38;
  uint *local_34;
  uint *local_30;
  int local_2c;
  uint *local_28;
  uint *local_24;
  uint local_20;
  undefined4 *local_1c;
  uint *local_18;
  uint local_14;
  uint *local_10;
  uint *local_c;
  uint *local_8;

  local_18 = param_1;
  local_28 = param_2;
  local_24 = (uint *)0x0;
  local_20 = *param_1;
  if (*param_1 == 0) {
    local_2c = 0;
    goto LAB_00222e97;
  }
  local_30 = (uint *)*in_EAX;
  if ((((uint)local_30 | local_20) & param_3 - 1U) != 0) {
    local_2c = 0xdb;                    /* "gehoert nicht zu diesem Pool" -- wie 68K */
    goto LAB_00222e97;
  }
  if (local_30 == (uint *)0x0) {
    local_2c = 0xdb;
    goto LAB_00222e97;
  }
  local_34 = param_2;
  if (param_2[2] == 0) {
    param_2[3] = (uint)param_2;
    param_2[2] = (uint)param_2;
  }
  for (puVar3 = (uint *)param_2[2];
      (puVar3 != param_2 && ((local_30 < (uint *)*puVar3 || ((uint *)puVar3[1] <= local_30))));
      puVar3 = (uint *)puVar3[2]) {
  }
  local_1c = in_EAX;
  local_14 = local_20;
  local_10 = local_30;
  if (puVar3 == param_2) {
    puVar5 = *(undefined4 **)(unaff_EBX + 0xc4);
LAB_00222c65:
    if ((local_30 < (uint *)*puVar5) || ((uint *)puVar5[1] <= local_30)) goto LAB_00222c4c;
    if ((uint)puVar5[1] < (int)local_30 + local_20) {
      local_2c = 0xd2;                  /* "Adresse gehoert zu keiner bekannten Region" -- wie 68K */
    }
    else {
      local_38 = 0x40;                  /* 64-Byte-Arena-Deskriptor (68K: 42 Byte) */
      uVar8 = Q9X_arena_reserve_space(&local_3c,in_stack_ffffffb0 & 0xffff0000);
      local_2c = (int)uVar8;
      if (local_2c == 0) {
        puVar3 = (uint *)((ulonglong)uVar8 >> 0x20);
        puVar6 = local_3c;
        for (iVar4 = 0x10; iVar4 != 0; iVar4 = iVar4 + -1) {
          *puVar6 = *puVar3;             /* Template-Kopie -- identische Technik wie 68K 0x5bac */
          puVar3 = puVar3 + 1;
          puVar6 = puVar6 + 1;
        }
        if (local_34[2] == 0) {
          local_34[3] = (uint)local_34;
          local_34[2] = (uint)local_34;
        }
        local_3c[5] = (uint)(local_3c + 4);
        local_3c[4] = (uint)(local_3c + 4);
        local_3c[7] = 0;
        for (puVar3 = (uint *)local_34[2];
            (puVar3 != local_34 && ((ushort)local_3c[10] <= (ushort)puVar3[10]));
            puVar3 = (uint *)puVar3[2]) {
        }
        local_3c[2] = (uint)puVar3;
        local_3c[3] = puVar3[3];
        *(uint **)(puVar3[3] + 8) = local_3c;
        puVar3[3] = (uint)local_3c;
        puVar3 = local_3c;
        goto LAB_00222d20;
      }
    }
    goto LAB_00222d2a;
  }
  if (puVar3[1] < (int)local_30 + local_20) {
    local_2c = 0xd2;
  }
  else {
LAB_00222d20:
    local_2c = 0;
    local_24 = puVar3;
  }
  goto LAB_00222d2a;
LAB_00222c4c:
  puVar5 = (undefined4 *)puVar5[2];
  if (puVar5 == (undefined4 *)(unaff_EBX + 0xbc)) goto code_r0x00222c59;
  goto LAB_00222c65;
code_r0x00222c59:
  local_2c = 0xd2;
LAB_00222d2a:
  if (local_2c == 0) {
    puVar3 = local_24 + 4;
    local_c = puVar3;
    do {
      puVar6 = (uint *)*local_c;
      if ((local_10 <= puVar6) || (puVar6 == puVar3)) {
        if (puVar3 == local_c) {
          local_8 = (uint *)0x0;
        }
        else {
          local_8 = (uint *)((int)local_c + local_c[2]);
        }
        if ((puVar6 < local_8) && (puVar6 != puVar3)) {
          local_2c = 0xab;              /* "keine Arena mit ausreichend freiem Speicher" -- wie 68K */
        }
        else if ((local_10 < local_8) ||
                ((puVar6 < (uint *)((int)local_10 + local_14) && (puVar6 != puVar3)))) {
          local_2c = 0xd2;
        }
        else {
          local_24[7] = local_24[7] + local_14;
          while (((puVar3 = (uint *)local_24[3], puVar3 != local_28 &&
                  ((short)local_24[10] == (short)puVar3[10])) && (puVar3[7] < local_24[7]))) {
            puVar3[2] = local_24[2];
            *(uint **)(local_24[2] + 0xc) = puVar3;
            local_24[2] = (uint)puVar3;
            local_24[3] = puVar3[3];
            puVar3[3] = (uint)local_24;
            *(uint **)(local_24[3] + 8) = local_24;
          }
          if ((*(ushort *)(unaff_EBX + 0x28) & 2) != 0) {
            FUN_0022ea38(0x46,local_14);
          }
          if (local_8 == local_10) {
            *local_1c = (undefined4)(unsigned long)local_c;  /* Cast ergaenzt, s. Datei-Kopfkommentar */
            local_10 = local_c;
            local_c[2] = local_c[2] + local_14;
            local_14 = local_c[2];
            *local_18 = local_14;
          }
          else {
            local_10[2] = local_14;
            local_10[1] = (uint)local_c;
            *local_10 = (uint)puVar6;
            *local_c = (uint)local_10;
            puVar6[1] = (uint)local_10;
          }
          if ((uint *)(local_14 + (int)local_10) == puVar6) {
            uVar1 = puVar6[2];
            uVar2 = *local_18;
            *local_18 = uVar1 + uVar2;
            local_10[2] = uVar1 + uVar2;
            *local_10 = *puVar6;
            *(uint **)(*local_10 + 4) = local_10;
          }
          local_2c = 0;
        }
        goto LAB_00222e97;
      }
      bVar7 = local_c == (uint *)puVar6[1];
      local_c = puVar6;
    } while (bVar7);
    local_2c = 0xab;
  }
LAB_00222e97:
  return CONCAT44(in_EDX,local_2c);
}


/* === Q9X_arena_reserve_space @ 0x222680 (vormals FUN_00222680) ===
 * Rundet die angeforderte Groesse (neg/and-Muster, identisch zum 68K-
 * Rundungsidiom), ruft eine tiefere Primitive (FUN_002228ac, nicht mehr
 * disassembliert -- vermutliches Aequivalent zu 68Ks First-Fit-
 * Allokator 0x5440) und faellt bei Erschoepfung auf eine erneute
 * Arena-Erzeugung ueber Q9X_arena_lookup_or_create zurueck.
 */
undefined8 Q9X_arena_reserve_space(uint *param_1,undefined2 param_2)

{
  uint *in_EAX;
  uint uVar1;
  int iVar2;
  uint *puVar3;
  uint *puVar4;
  uint extraout_ECX;
  undefined4 in_EDX;
  int unaff_EBX;
  uint *puVar5;
  uint local_18;
  uint local_14;
  uint *local_10;
  uint *local_c;
  int local_8;

  local_c = param_1;
  local_10 = in_EAX;
  if (-*(int *)(unaff_EBX + 0x38) - 1U < *in_EAX) {
    if (*in_EAX != 0xffffffff) {
      local_8 = 0xed;                   /* wie 68K: Pool/Liste voll */
      goto LAB_0022288c;
    }
    uVar1 = FUN_002225ee(param_2,unaff_EBX + 0xbc,1);
    *local_10 = uVar1;
  }
  uVar1 = -*(int *)(unaff_EBX + 0x3c) & (*(int *)(unaff_EBX + 0x3c) + *local_10) - 1;
  *local_10 = uVar1;                    /* Groessen-Rundung, identisches neg/and-Idiom wie 68K */
  if (uVar1 == 0) {
    local_8 = 0xe1;                     /* wie 68K: Groesse 0 */
  }
  else {
    local_8 = FUN_002228ac(param_2,local_c,*(int *)(*(int *)(unaff_EBX + 0x84) + 0x14) + 0x3c,1);
    if (local_8 == 0xed) {
      local_18 = -*(int *)(unaff_EBX + 0x38) & (*(int *)(unaff_EBX + 0x38) + *local_10) - 1;
      local_8 = 0xed;
      do {
        local_8 = FUN_002228ac(param_2,local_c,unaff_EBX + 0xbc,1);
        if (local_8 != 0xed) break;
        iVar2 = FUN_00222a34();
      } while (iVar2 == 0);
      if (local_8 == 0) {
        uVar1 = *local_c;
        local_14 = local_18;
        iVar2 = *(int *)(*(int *)(unaff_EBX + 0x84) + 0x14);
        puVar5 = (uint *)(iVar2 + 0x3c);
        puVar4 = *(uint **)(iVar2 + 0x44);
        if (puVar4 != (uint *)0x0) {
          for (; (puVar4 != puVar5 && ((uVar1 < *puVar4 || (puVar4[1] <= uVar1))));
              puVar4 = (uint *)puVar4[2]) {
          }
          if (puVar4 != puVar5) {
            puVar3 = (uint *)puVar4[5];
            if (puVar3 != (uint *)0x0) {
              for (; ((uint *)(uVar1 + local_18) < puVar3 && (puVar3 != puVar4 + 4));
                  puVar3 = (uint *)puVar3[1]) {
              }
              if (puVar3 == (uint *)(uVar1 + local_18)) {
                *(uint *)(*puVar3 + 4) = puVar3[1];
                *(uint *)puVar3[1] = *puVar3;
                local_14 = local_18 + puVar3[2];
                FUN_00222e9e(puVar4,puVar5,0xffffffff);
              }
            }
          }
        }
        local_18 = local_14;
        if (*local_10 < local_14) {
          local_18 = local_14 - *local_10;
          /* Ghidra generiert hier einen 4. Aufrufparameter ("1"), obwohl
           * Q9X_arena_lookup_or_create nur 3 Parameter definiert hat --
           * ein bekanntes Kalling-Konventions-Artefakt (vgl. Thema 00/01),
           * hier zwecks Syntaxpruefbarkeit entfernt, inhaltlich unveraendert. */
          Q9X_arena_lookup_or_create
                    (&local_18,(uint *)(*(int *)(*(int *)(unaff_EBX + 0x84) + 0x14) + 0x3c),
                     *(undefined4 *)(unaff_EBX + 0x3c));
          *local_c = extraout_ECX;
        }
      }
    }
  }
  if (local_8 == 0) {
    if ((*(ushort *)(unaff_EBX + 0x28) & 2) != 0) {
      FUN_0022ea38(0x41,*local_10);
    }
  }
  else {
    *local_10 = 0;
    *local_c = 0;
  }
LAB_0022288c:
  return CONCAT44(in_EDX,local_8);
}
