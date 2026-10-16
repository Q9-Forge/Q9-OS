/*
 * rbf_07.c - rbf part 7
 *
 * Original address range: 0x0073d0 - 0x008660 of the V4.9 kernel module.
 * Functions 0x73d0 to 0x851a (support routines)
 * Generated from the Ghidra export; see tools/gen_src.py. Hand edits belong in hand/*.inc.
 */
#include "kernel.h"

/* ----------------------------------------------------------------------
 * fn_0073d0   @0x0073d0, 275 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_004fd4, fn_0054b8, fn_005b7c, fn_006f10, fn_007518
 * calls: fn_0034ee, fn_00351a, fn_0076fa, fn_00789c, fn_0079d2, lock_release
 */
 /* WARNING: Unknown calling convention */
uint fn_0073d0(a0, a1, a2)
    uint a0;
    uint a1;
    uint a2;
{
  ushort uVar1;
  int iVar2;
  uint uVar3;
  uint extraout_ECX;
  int extraout_ECX_00;
  uint extraout_ECX_01;
  uint extraout_ECX_02;
  uint extraout_EDX;
  uint a0_00;
  uint a0_01;
  uint a0_02;
  int extraout_EDX_00;
  uint a0_03;
  uint a0_04;
  int in_stack_00000004;
  
  if (*(short *)(*(int *)(a1 + 4) + 0x56) == 0x4143) {
    return 0xf2;
  }
  iVar2 = *(int *)(a1 + 0xc0);
  if (in_stack_00000004 != iVar2) {
    if (((*(byte *)(a1 + 100) & 1) != 0) && (*(int *)(a1 + 0xc0) != 0)) {
      fn_0034ee(a1);
      a1 = extraout_EDX;
    }
    uVar3 = fn_0079d2(a1);
    if (uVar3 != 0) {
      return uVar3;
    }
    *(int *)(a0_00 + 0xc0) = in_stack_00000004;
    uVar3 = fn_0076fa(a0_00);
    a1 = a0_01;
    if (uVar3 != 0) {
      *(int *)(a0_01 + 0xc0) = iVar2;
      fn_0076fa(a0_01);
      return extraout_ECX;
    }
  }
  fn_00789c(*(int *)(a1 + 0x70) + 0x28);
  uVar1 = *(ushort *)(extraout_ECX_00 + 0x12);
  *(short *)(extraout_ECX_00 + 0x12) = *(short *)(extraout_ECX_00 + 0x12) + (short)a0;
  if ((int)a0 < 0) {
    if ((uVar1 < *(ushort *)(extraout_ECX_00 + 0x12)) || (*(short *)(extraout_ECX_00 + 0x12) == 0))
    {
      *(undefined2 *)(extraout_ECX_00 + 0x12) = 1;
    }
  }
  else if (*(ushort *)(extraout_ECX_00 + 0x12) < uVar1) {
    *(undefined2 *)(extraout_ECX_00 + 0x12) = 0xffff;
  }
  fn_00351a(a0_02);
  lock_release(*(int *)(extraout_EDX_00 + 0x70) + 0x28);
  uVar3 = extraout_ECX_01;
  if (in_stack_00000004 != iVar2) {
    fn_0079d2(a0_03);
    *(int *)(a0_04 + 0xc0) = iVar2;
    fn_0076fa(a0_04);
    uVar3 = extraout_ECX_02;
  }
  return uVar3;
}

/* ----------------------------------------------------------------------
 * rbf_makdir   @0x0074e4, 51 bytes
 * RBF entry point #9: make a directory [file manager entry table]
 * called from: data@00009144
 * calls: fn_007518
 */
 /* WARNING: Unknown calling convention */
 /* RBF entry point #9: make a directory [file manager entry table] */
uint rbf_makdir(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  int extraout_ECX;
  uint in_EDX;
  PathDesc *pd;
  void *local_8;
  
  local_8 = pd->pd_plbuf;
  uVar1 = fn_007518((uint)&local_8,*(uint *)(a0 + 0x12),*(uint *)(a0 + 0x16),(uint)pd,in_EDX);
  *(uint *)(extraout_ECX + 4) = uVar1;
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_007518   @0x007518, 473 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: rbf_makdir
 * calls: fn_0015c4, fn_0017f6, fn_004464, fn_006194, fn_0066d8, fn_00673a, fn_0067ea, fn_006e96
 */
 /* WARNING: Unknown calling convention */
uint fn_007518(a0, a1, a2, a3, a4)
    uint a0;
    uint a1;
    uint a2;
    uint a3;
    uint a4;
{
  uint *puVar1;
  uint uVar2;
  uint uVar3;
  undefined1 *a0_00;
  int extraout_ECX;
  int extraout_EDX;
  uint unaff_EDI;
  ushort in_stack_00000004;
  
  if (*(short *)(*(int *)(a3 + 4) + 0x56) == 0x4143) {
    uVar3 = 0xf2;
  }
  else {
    if ((in_stack_00000004 & 0x2000) == 0) {
      fn_0066d8(a3);
      a2 = 0;
      if (*(int *)(a3 + 0x128) != 0) {
        a2 = (*(int *)(a3 + 0x128) + -1) * *(int *)(a3 + 0x13c);
      }
    }
    if (a2 < 0x80) {
      a2 = 0x80;
    }
    uVar3 = a3;
    uVar2 = fn_006194(a0,a1 & 0xffff7fff,a2,0,a3,unaff_EDI);
    if (uVar2 == 0) {
      uVar3 = fn_008660(a3,CONCAT22((short)(uVar3 >> 0x10),*(undefined2 *)(a3 + 0xd0)),unaff_EDI)
      ;
      if (uVar3 == 0) {
        uVar3 = fn_004464(0x80,unaff_EDI);
        if (uVar3 == 0) {
          uVar3 = fn_0015c4(*(uint *)(a3 + 0x84),unaff_EDI);
          if (uVar3 == 0) {
            uVar3 = *(uint *)(a3 + 0x13c);
            fn_0090f8(0);
            a0_00[1] = 0x2e;
            *a0_00 = 0x2e;
            *(undefined4 *)(a0_00 + 0x3c) = *(undefined4 *)(a3 + 0xc4);
            uVar2 = a3;
            fn_008878((uint)a0_00,uVar3);
            *(undefined1 *)(extraout_ECX + 0x40) = 0x2e;
            *(undefined4 *)(extraout_ECX + 0x7c) = *(undefined4 *)(a3 + 0xc0);
            fn_008878((uint)(extraout_ECX + 0x40),uVar2);
            puVar1 = (uint *)(*(int *)(a3 + 0x6c) + 8);
            *puVar1 = *puVar1 | 1;
            uVar3 = fn_0017f6(a3);
            if (uVar3 == 0) {
              *(byte *)(extraout_EDX + 9) = *(byte *)(extraout_EDX + 9) | 0x80;
              fn_006e96(a3);
              uVar3 = fn_0073d0(1,a3,unaff_EDI);
              if (uVar3 == 0) {
                fn_0073d0(1,a3,unaff_EDI);
              }
            }
            uVar3 = fn_00673a(a3,unaff_EDI);
          }
          else {
            uVar3 = fn_00673a(a3,unaff_EDI);
          }
        }
        else {
          fn_0067ea(a3,*(uint *)(a3 + 0x88),*(uint *)(a3 + 0x80),unaff_EDI);
          uVar3 = fn_00673a(a3,uVar3);
        }
      }
      else {
        uVar3 = fn_00673a(a3,unaff_EDI);
      }
    }
    else {
      uVar3 = fn_00673a(a3,unaff_EDI);
    }
  }
  return uVar3;
}

/* ----------------------------------------------------------------------
 * rbf_dup   @0x0076f2, 7 bytes
 * RBF entry point #7: duplicate a path (shared use) [file manager entry table]
 * called from: data@0000913c
 */
 /* WARNING: Unknown calling convention */
 /* RBF entry point #7: duplicate a path (shared use) [file manager entry table] */
uint rbf_dup(a0)
    uint a0;
{
  return 0;
}

/* ----------------------------------------------------------------------
 * fn_0076fa   @0x0076fa, 418 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_00517a, fn_005b7c, fn_005e06, fn_006194, fn_0067ea, fn_006f10, fn_0073d0, fn_0081fa
 * calls: fn_003024, fn_003068, fn_00789c, fn_0079b6, lock_create, os_f_srtmem
 */
 /* WARNING: Unknown calling convention */
uint fn_0076fa(a0)
    uint a0;
{
  int iVar1;
  uint uVar2;
  uint uVar3;
  uint extraout_EDX;
  uint unaff_EDI;
  int iVar4;
  
  *(undefined4 *)(a0 + 0x78) = 0;
  *(undefined4 *)(a0 + 0x88) = 0;
  uVar2 = *(uint *)(*(int *)(a0 + 4) + 0x20);
  *(uint *)(a0 + 0x98) = a0;
  fn_00789c(uVar2 + 0x60);
  iVar1 = uVar2 - 0x24;
  while (iVar4 = iVar1, iVar1 = *(int *)(iVar4 + 0x94), iVar1 != 0) {
    if ((extraout_EDX <= *(uint *)(iVar1 + 0xc0)) &&
       ((extraout_EDX != *(uint *)(iVar1 + 0xc0) || (*(int *)(iVar1 + 0x8c) == *(int *)(a0 + 0x8c)))
       )) break;
  }
  if ((iVar1 == 0) || (extraout_EDX != *(uint *)(iVar1 + 0xc0))) {
    if (((*(int *)(a0 + 0x70) == 0) && (*(int *)(a0 + 0xc0) != 0)) &&
       (uVar3 = fn_003068(a0,unaff_EDI), uVar3 != 0)) {
      uVar2 = fn_0079b6(uVar2,unaff_EDI);
    }
    else {
      uVar3 = lock_create(a0 + 0x90);
      if (uVar3 == 0) {
        uVar3 = lock_create(a0 + 0xb0);
        if (uVar3 == 0) {
          *(undefined4 *)(a0 + 0x94) = *(undefined4 *)(iVar4 + 0x94);
          *(uint *)(iVar4 + 0x94) = a0;
          uVar2 = fn_0079b6(uVar2,unaff_EDI);
        }
        else {
          os_f_srtmem(*(uint *)(a0 + 0x90));
          uVar2 = fn_0079b6(uVar2,unaff_EDI);
        }
      }
      else {
        uVar2 = fn_0079b6(uVar2,unaff_EDI);
      }
    }
  }
  else if ((*(byte *)(a0 + 0x65) & 2) == 0) {
    *(uint *)(a0 + 0x94) = a0;
    *(undefined4 *)(a0 + 0x98) = *(undefined4 *)(iVar1 + 0x98);
    *(uint *)(iVar1 + 0x98) = a0;
    *(undefined4 *)(a0 + 0x90) = *(undefined4 *)(iVar1 + 0x90);
    *(undefined4 *)(a0 + 0xb0) = *(undefined4 *)(iVar1 + 0xb0);
    *(undefined4 *)(a0 + 0x70) = *(undefined4 *)(iVar1 + 0x70);
    *(undefined4 *)(a0 + 0x7c) = *(undefined4 *)(iVar1 + 0x7c);
    fn_003024(a0);
    uVar2 = fn_0079b6(uVar2,unaff_EDI);
  }
  else {
    uVar2 = fn_0079b6(uVar2,unaff_EDI);
  }
  return uVar2;
}

/* ----------------------------------------------------------------------
 * fn_00789c   @0x00789c, 282 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0000ea, fn_00057e, fn_0005ee, fn_000770, fn_000a76, fn_0015c4, fn_0017f6, fn_001a00
 * calls: get_current_process
 */
 /* WARNING: Unknown calling convention */
uint fn_00789c(a0)
    uint a0;
{
  int iVar1;
  code *pcVar2;
  uint *puVar3;
  uint uVar4;
  int extraout_ECX;
  uint *puVar5;
  uint *puVar6;
  int iVar7;
  byte bVar8;
  byte bVar9;
  byte bVar10;
  byte in_AF;
  byte bVar11;
  byte bVar12;
  byte in_TF;
  byte in_IF;
  byte bVar13;
  byte bVar14;
  byte bVar15;
  byte bVar16;
  byte in_NT;
  byte bVar17;
  byte in_AC;
  byte bVar18;
  byte in_VIF;
  byte bVar19;
  byte in_VIP;
  byte bVar20;
  byte in_ID;
  byte bVar21;
  uint uStack_3c;
  undefined4 local_14;
  uint local_c;
  
  bVar9 = false;
  puVar5 = (uint *)GHIDRA_STACK_PTR(0xffffffc8);
  do {
    uVar4 = *(uint *)a0;
    bVar8 = a0 < uVar4;
    bVar15 = SBORROW4(a0,uVar4);
    uVar4 = a0 - uVar4;
    bVar12 = (int)uVar4 < 0;
    bVar11 = uVar4 == 0;
    bVar10 = (POPCOUNT(uVar4 & 0xff) & 1U) == 0;
    if ((bool)bVar11) {
      puVar5[-1] = 0x78ce;
      uVar4 = get_current_process();
      puVar5[-1] = get_eflags();
      local_14 = puVar5[-1];
      puVar3 = puVar5;
      if (*(int *)(a0 + 4) == 0) {
        *(undefined4 *)(a0 + 4) = *(undefined4 *)(uVar4 + 4);
      }
      else if (*(int *)(uVar4 + 4) != *(int *)(a0 + 4)) {
        for (iVar7 = *(int *)(a0 + 8);
            (iVar7 != a0 - 0x4c && (*(uint *)(uVar4 + 0x20) <= *(uint *)(iVar7 + 0x20)));
            iVar7 = *(int *)(iVar7 + 0x54)) {
        }
        iVar1 = *(int *)(iVar7 + 0x58);
        *(int *)(uVar4 + 0x58) = iVar1;
        *(uint *)(iVar1 + 0x54) = uVar4;
        *(int *)(uVar4 + 0x54) = iVar7;
        *(uint *)(iVar7 + 0x58) = uVar4;
        if ((*(uint *)(uVar4 + 0x24) >> 3 & 1) == 0) {
          *(ushort *)(uVar4 + 0x84) = *(ushort *)(uVar4 + 0x84) & 0xff7f;
        }
        puVar5[-1] = local_14;
        uVar4 = puVar5[-1];
        bVar17 = (uVar4 & 0x4000) != 0;
        bVar16 = (uVar4 & 0x800) != 0;
        bVar14 = (uVar4 & 0x400) != 0;
        bVar13 = (uVar4 & 0x200) != 0;
        bVar15 = (uVar4 & 0x100) != 0;
        bVar12 = (uVar4 & 0x80) != 0;
        bVar11 = (uVar4 & 0x40) != 0;
        bVar10 = (uVar4 & 0x10) != 0;
        bVar8 = (uVar4 & 4) != 0;
        bVar9 = (uVar4 & 1) != 0;
        bVar21 = (uVar4 & 0x200000) != 0;
        bVar18 = (uVar4 & 0x40000) != 0;
        bVar20 = 0;
        bVar19 = 0;
        pcVar2 = (code *)swi(0xff);
        (*pcVar2)();
        puVar6 = puVar5 + 1;
        *puVar5 = (uint)(bVar17 & 1) * 0x4000 | (uint)(bVar16 & 1) * 0x800 |
                  (uint)(bVar14 & 1) * 0x400 | (uint)(bVar13 & 1) * 0x200 |
                  (uint)(bVar15 & 1) * 0x100 | (uint)(bVar12 & 1) * 0x80 | (uint)(bVar11 & 1) * 0x40
                  | (uint)(bVar10 & 1) * 0x10 | (uint)(bVar8 & 1) * 4 | (uint)(bVar9 & 1) |
                  (uint)(bVar21 & 1) * 0x200000 | (uint)(bVar20 & 1) * 0x100000 |
                  (uint)(bVar19 & 1) * 0x80000 | (uint)(bVar18 & 1) * 0x40000;
        local_14 = *puVar5;
        puVar3 = puVar5 + 1;
        if (*(int *)(extraout_ECX + 4) != *(int *)(a0 + 4)) {
          *(undefined4 *)(*(int *)(extraout_ECX + 0x58) + 0x54) =
               *(undefined4 *)(extraout_ECX + 0x54);
          *(undefined4 *)(*(int *)(extraout_ECX + 0x54) + 0x58) =
               *(undefined4 *)(extraout_ECX + 0x58);
          *(undefined4 *)(extraout_ECX + 0x58) = 0;
          *(undefined4 *)(extraout_ECX + 0x54) = 0;
          *puVar5 = local_14;
          uVar4 = *puVar5;
          in_NT = (uVar4 & 0x4000) != 0;
          bVar9 = (uVar4 & 0x400) != 0;
          in_IF = (uVar4 & 0x200) != 0;
          in_TF = (uVar4 & 0x100) != 0;
          in_AF = (uVar4 & 0x10) != 0;
          in_ID = (uVar4 & 0x200000) != 0;
          in_AC = (uVar4 & 0x40000) != 0;
          in_VIP = 0;
          in_VIF = 0;
          local_c = 0xe9;
          goto LAB_000079a2;
        }
      }
      puVar6 = puVar3;
      *(undefined4 *)((int)puVar6 + -4) = local_14;
      uVar4 = *(uint *)((int)puVar6 + -4);
      in_NT = (uVar4 & 0x4000) != 0;
      bVar9 = (uVar4 & 0x400) != 0;
      in_IF = (uVar4 & 0x200) != 0;
      in_TF = (uVar4 & 0x100) != 0;
      in_AF = (uVar4 & 0x10) != 0;
      in_ID = (uVar4 & 0x200000) != 0;
      in_AC = (uVar4 & 0x40000) != 0;
      in_VIP = 0;
      in_VIF = 0;
      local_c = 0;
    }
    else {
      local_c = 0xc6;
      puVar6 = puVar5;
    }
LAB_000079a2:
    puVar5 = puVar6;
    if (local_c != 0xe9) {
      return local_c;
    }
  } while( true );
}

/* ----------------------------------------------------------------------
 * fn_0079b6   @0x0079b6, 28 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_005cfc, fn_0076fa, fn_0079d2, fn_008660
 * calls: lock_release
 */
 /* WARNING: Unknown calling convention */
uint fn_0079b6(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  uint extraout_ECX;
  
  uVar1 = lock_release(a0 + 0x60);
  if ((uVar1 == 0) || (extraout_ECX != 0)) {
    uVar1 = extraout_ECX;
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_0079d2   @0x0079d2, 396 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_00517a, fn_005b7c, fn_005e06, fn_006194, fn_00673a, fn_0067ea, fn_0073d0, fn_0081fa
 * calls: fn_001bc8, fn_00344a, fn_00789c, fn_0079b6, fn_007b6a, os_f_srtmem, lock_release
 */
 /* WARNING: Unknown calling convention */
uint fn_0079d2(a0)
    uint a0;
{
  uint uVar1;
  uint uVar2;
  int extraout_ECX;
  int iVar3;
  undefined4 extraout_EDX;
  int extraout_EDX_00;
  int extraout_EDX_01;
  int extraout_EDX_02;
  int extraout_EDX_03;
  int extraout_EDX_04;
  uint a0_00;
  uint a0_01;
  int extraout_EDX_05;
  int iVar4;
  int iVar5;
  uint unaff_EDI;
  
  uVar1 = *(uint *)(*(int *)(a0 + 4) + 0x20);
  fn_00789c(uVar1 + 0x60);
  iVar5 = uVar1 - 0x24;
  fn_007b6a(CONCAT31((int3)((uint)extraout_EDX >> 8),7),unaff_EDI);
  if (*(int *)(extraout_EDX_00 + 0x90) == 0) {
    uVar1 = fn_0079b6(uVar1,unaff_EDI);
  }
  else {
    fn_00789c(*(uint *)(extraout_EDX_00 + 0x90));
    for (iVar3 = extraout_ECX; extraout_EDX_01 != *(int *)(iVar3 + 0x98);
        iVar3 = *(int *)(iVar3 + 0x98)) {
      if (*(int *)(iVar3 + 0x98) == 0) goto LAB_00007a43;
    }
    if (iVar3 != extraout_EDX_01) {
      *(undefined4 *)(extraout_EDX_01 + 0x70) = 0;
    }
    *(undefined4 *)(iVar3 + 0x98) = *(undefined4 *)(extraout_EDX_01 + 0x98);
    do {
      if (extraout_EDX_01 == *(int *)(iVar5 + 0x94)) {
        iVar3 = *(int *)(extraout_EDX_01 + 0x94);
        if (extraout_EDX_01 == *(int *)(extraout_EDX_01 + 0x98)) {
          uVar2 = lock_release(*(uint *)(extraout_EDX_01 + 0x90));
          if (((uVar2 != 0) || (uVar2 = os_f_srtmem(*(uint *)(extraout_EDX_03 + 0x90)), uVar2 != 0))
             || (uVar2 = os_f_srtmem(*(uint *)(extraout_EDX_04 + 0xb0)), uVar2 != 0)) {
            uVar1 = fn_0079b6(uVar1,unaff_EDI);
            return uVar1;
          }
          *(undefined4 *)(a0_00 + 0x90) = 0;
          uVar2 = fn_001bc8(a0_00);
          if ((uVar2 != 0) || (uVar2 = fn_00344a(a0_01), iVar4 = extraout_EDX_05, uVar2 != 0)) {
            uVar1 = fn_0079b6(uVar1,unaff_EDI);
            return uVar1;
          }
        }
        else {
          iVar3 = *(int *)(extraout_EDX_01 + 0x98);
          *(undefined4 *)(iVar3 + 0x94) = *(undefined4 *)(extraout_EDX_01 + 0x94);
          lock_release(*(uint *)(extraout_EDX_01 + 0x90));
          iVar4 = extraout_EDX_02;
        }
        *(int *)(iVar5 + 0x94) = iVar3;
        *(int *)(iVar4 + 0x98) = iVar4;
        uVar1 = fn_0079b6(uVar1,unaff_EDI);
        return uVar1;
      }
      iVar5 = *(int *)(iVar5 + 0x94);
    } while (iVar5 != 0);
LAB_00007a43:
    *(int *)(extraout_EDX_01 + 0x98) = extraout_EDX_01;
    lock_release(*(uint *)(extraout_EDX_01 + 0x90));
    uVar1 = fn_0079b6(uVar1,unaff_EDI);
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_007b6a   @0x007b6a, 569 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_003a26, rbf_write, fn_004fd4, fn_006194, fn_0079d2, fn_007efa, fn_00800a
 * calls: get_current_process
 */
 /* WARNING: Unknown calling convention */
uint fn_007b6a(a0, a1)
    uint a0;
    uint a1;
{
  uint *puVar1;
  int iVar2;
  code *pcVar3;
  undefined4 uVar4;
  byte bVar5;
  uint *puVar6;
  uint uVar7;
  uint *puVar8;
  uint *puVar9;
  byte bVar10;
  byte bVar11;
  byte in_AF;
  byte bVar12;
  byte bVar13;
  byte in_TF;
  byte in_IF;
  byte bVar14;
  byte bVar15;
  byte bVar16;
  byte bVar17;
  byte in_NT;
  byte bVar18;
  byte in_AC;
  byte bVar19;
  byte in_VIF;
  byte bVar20;
  byte in_VIP;
  byte bVar21;
  byte in_ID;
  byte bVar22;
  uint uVar23;
  int in_stack_00000004;
  uint uStack_74;
  int local_20;
  undefined4 local_14;
  int local_c;
  
  bVar11 = false;
  puVar8 = (uint *)GHIDRA_STACK_PTR(0xffffff90);
  bVar5 = (byte)a0 & *(byte *)(in_stack_00000004 + 0x65);
  if ((bVar5 == 0) ||
     (*(byte *)(in_stack_00000004 + 0x65) = *(byte *)(in_stack_00000004 + 0x65) ^ bVar5,
     (*(byte *)(in_stack_00000004 + 0x65) & 1) == 0)) {
    local_c = in_stack_00000004;
    puVar1 = *(uint **)(in_stack_00000004 + 0xb0);
    if (puVar1 != (uint *)0x0) {
      do {
        puVar6 = (uint *)*puVar1;
        bVar5 = puVar1 < puVar6;
        bVar16 = SBORROW4((int)puVar1,(int)puVar6);
        uVar7 = (int)puVar1 - (int)puVar6;
        bVar13 = (int)uVar7 < 0;
        bVar12 = uVar7 == 0;
        bVar10 = (POPCOUNT(uVar7 & 0xff) & 1U) == 0;
        if ((bool)bVar12) {
          puVar8[-1] = 0x7beb;
          uVar7 = get_current_process();
          puVar8[-1] = get_eflags();
          local_14 = puVar8[-1];
          puVar6 = puVar8;
          if (puVar1[1] == 0) {
            puVar1[1] = *(uint *)(uVar7 + 4);
          }
          else if (*(uint *)(uVar7 + 4) != puVar1[1]) {
            for (puVar6 = (uint *)puVar1[2];
                (puVar6 != puVar1 + -0x13 && (*(uint *)(uVar7 + 0x20) <= puVar6[8]));
                puVar6 = (uint *)puVar6[0x15]) {
            }
            uVar23 = puVar6[0x16];
            *(uint *)(uVar7 + 0x58) = uVar23;
            *(uint *)(uVar23 + 0x54) = uVar7;
            *(uint **)(uVar7 + 0x54) = puVar6;
            puVar6[0x16] = uVar7;
            if ((*(uint *)(uVar7 + 0x24) >> 3 & 1) == 0) {
              *(ushort *)(uVar7 + 0x84) = *(ushort *)(uVar7 + 0x84) & 0xff7f;
            }
            puVar8[-1] = local_14;
            uVar23 = puVar8[-1];
            bVar18 = (uVar23 & 0x4000) != 0;
            bVar17 = (uVar23 & 0x800) != 0;
            bVar15 = (uVar23 & 0x400) != 0;
            bVar14 = (uVar23 & 0x200) != 0;
            bVar16 = (uVar23 & 0x100) != 0;
            bVar13 = (uVar23 & 0x80) != 0;
            bVar12 = (uVar23 & 0x40) != 0;
            bVar10 = (uVar23 & 0x10) != 0;
            bVar11 = (uVar23 & 4) != 0;
            bVar5 = (uVar23 & 1) != 0;
            bVar22 = (uVar23 & 0x200000) != 0;
            bVar19 = (uVar23 & 0x40000) != 0;
            bVar21 = 0;
            bVar20 = 0;
            pcVar3 = (code *)swi(0xff);
            (*pcVar3)();
            puVar9 = puVar8 + 1;
            *puVar8 = (uint)(bVar18 & 1) * 0x4000 | (uint)(bVar17 & 1) * 0x800 |
                      (uint)(bVar15 & 1) * 0x400 | (uint)(bVar14 & 1) * 0x200 |
                      (uint)(bVar16 & 1) * 0x100 | (uint)(bVar13 & 1) * 0x80 |
                      (uint)(bVar12 & 1) * 0x40 | (uint)(bVar10 & 1) * 0x10 | (uint)(bVar11 & 1) * 4
                      | (uint)(bVar5 & 1) | (uint)(bVar22 & 1) * 0x200000 |
                      (uint)(bVar21 & 1) * 0x100000 | (uint)(bVar20 & 1) * 0x80000 |
                      (uint)(bVar19 & 1) * 0x40000;
            local_14 = *puVar8;
            puVar6 = puVar8 + 1;
            if (*(uint *)(uVar7 + 4) != puVar1[1]) {
              *(undefined4 *)(*(int *)(uVar7 + 0x58) + 0x54) = *(undefined4 *)(uVar7 + 0x54);
              *(undefined4 *)(*(int *)(uVar7 + 0x54) + 0x58) = *(undefined4 *)(uVar7 + 0x58);
              *(undefined4 *)(uVar7 + 0x58) = 0;
              *(undefined4 *)(uVar7 + 0x54) = 0;
              *puVar8 = local_14;
              uVar7 = *puVar8;
              in_NT = (uVar7 & 0x4000) != 0;
              bVar11 = (uVar7 & 0x400) != 0;
              in_IF = (uVar7 & 0x200) != 0;
              in_TF = (uVar7 & 0x100) != 0;
              in_AF = (uVar7 & 0x10) != 0;
              in_ID = (uVar7 & 0x200000) != 0;
              in_AC = (uVar7 & 0x40000) != 0;
              in_VIP = 0;
              in_VIF = 0;
              local_20 = 0xe9;
              goto LAB_00007cbf;
            }
          }
          puVar9 = puVar6;
          *(undefined4 *)((int)puVar9 + -4) = local_14;
          uVar7 = *(uint *)((int)puVar9 + -4);
          in_NT = (uVar7 & 0x4000) != 0;
          bVar11 = (uVar7 & 0x400) != 0;
          in_IF = (uVar7 & 0x200) != 0;
          in_TF = (uVar7 & 0x100) != 0;
          in_AF = (uVar7 & 0x10) != 0;
          in_ID = (uVar7 & 0x200000) != 0;
          in_AC = (uVar7 & 0x40000) != 0;
          in_VIP = 0;
          in_VIF = 0;
          local_20 = 0;
        }
        else {
          local_20 = 0xc6;
          puVar9 = puVar8;
        }
LAB_00007cbf:
        puVar8 = puVar9;
      } while (local_20 == 0xe9);
      while (iVar2 = *(int *)(local_c + 0xb4), iVar2 != in_stack_00000004) {
        *(int *)(local_c + 0xb4) = local_c;
        *(byte *)(iVar2 + 0x66) = *(byte *)(iVar2 + 0x66) & 0x7f;
        pcVar3 = (code *)swi(0xff);
        (*pcVar3)();
        puVar9 = (uint *)((int)puVar9 + 4);
        local_c = iVar2;
      }
      *(int *)(local_c + 0xb4) = local_c;
      puVar8 = (uint *)*puVar1;
      if (puVar1 == puVar8) {
        *(uint *)((int)puVar9 + -4) = get_eflags();
        uVar4 = *(undefined4 *)((int)puVar9 + -4);
        if (puVar1 + -0x13 == (uint *)puVar1[2]) {
          puVar1[1] = 0;
          uVar7 = 0;
        }
        else {
          uVar7 = puVar1[2];
          puVar1[1] = *(uint *)(uVar7 + 4);
          *(undefined4 *)(*(int *)(uVar7 + 0x58) + 0x54) = *(undefined4 *)(uVar7 + 0x54);
          *(undefined4 *)(*(int *)(uVar7 + 0x54) + 0x58) = *(undefined4 *)(uVar7 + 0x58);
          *(undefined4 *)(uVar7 + 0x58) = 0;
          *(undefined4 *)(uVar7 + 0x54) = 0;
        }
        *(undefined4 *)((int)puVar9 + -4) = uVar4;
        if (uVar7 != 0) {
          pcVar3 = (code *)swi(0xff);
          (*pcVar3)();
        }
      }
    }
  }
  return 0;
}

/* ----------------------------------------------------------------------
 * fn_007da4   @0x007da4, 341 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_000a76, fn_003a26, rbf_write, fn_004fb6, fn_008776
 * calls: fn_001af2, fn_00789c, fn_007efa, fn_007f74, lock_release, fn_008d26
 */
 /* WARNING: Unknown calling convention */
uint fn_007da4(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  uint extraout_ECX;
  uint a0_00;
  int extraout_EDX;
  int extraout_EDX_00;
  int extraout_EDX_01;
  int extraout_EDX_02;
  int extraout_EDX_03;
  uint unaff_EDI;
  int in_stack_00000004;
  int local_c;
  int local_8;
  
  while( true ) {
    *(undefined1 *)(in_stack_00000004 + 0x66) = *(undefined1 *)(in_stack_00000004 + 0x65);
    uVar1 = fn_007efa(a0,(uint)&local_c,unaff_EDI);
    if (uVar1 != 0xfd) {
      return uVar1;
    }
    uVar1 = fn_008d26(*(uint *)(local_c + 0xa0));
    if ((uVar1 != 0) && (uVar1 != 0xe0)) break;
    fn_001af2(a0_00);
    if (*(int *)(extraout_EDX + 0xb0) == 0) {
      return 0xfc;
    }
    if (*(char *)(local_c + 0x65) == '\0') {
      fn_008d26(0);
      in_stack_00000004 = extraout_EDX_00;
    }
    else {
      *(undefined4 *)(extraout_EDX + 0xb4) = *(undefined4 *)(local_c + 0xb4);
      *(int *)(local_c + 0xb4) = extraout_EDX;
      local_8 = *(int *)(extraout_EDX + 0xb8);
      fn_007f74((uint)&local_8,unaff_EDI);
      local_c = extraout_EDX_01;
      fn_00789c(*(uint *)(extraout_EDX_01 + 0xb0));
      for (; extraout_EDX_02 != *(int *)(local_c + 0xb4); local_c = *(int *)(local_c + 0xb4)) {
      }
      *(undefined4 *)(local_c + 0xb4) = *(undefined4 *)(extraout_EDX_02 + 0xb4);
      *(int *)(extraout_EDX_02 + 0xb4) = extraout_EDX_02;
      lock_release(*(uint *)(extraout_EDX_02 + 0xb0));
      fn_008d26(0);
      if (extraout_ECX != 0) {
        return extraout_ECX;
      }
      if ((local_8 == 0) && (*(int *)(extraout_EDX_03 + 0xb8) != 0)) {
        if (*(int *)(extraout_EDX_03 + 0xb8) == 0) {
          uVar1 = 0;
        }
        else {
          uVar1 = 0xfc;
        }
        return uVar1;
      }
      *(undefined1 *)(extraout_EDX_03 + 0x65) = *(undefined1 *)(extraout_EDX_03 + 0x66);
      in_stack_00000004 = extraout_EDX_03;
      if (*(char *)(extraout_EDX_03 + 0x69) != '\0') {
        *(undefined4 *)(extraout_EDX_03 + 0x78) = *(undefined4 *)(extraout_EDX_03 + 0x7c);
      }
    }
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_007efa   @0x007efa, 121 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_007da4
 * calls: fn_007b6a, fn_00800a
 */
 /* WARNING: Unknown calling convention */
uint fn_007efa(a0, a1, a2)
    uint a0;
    uint a1;
    uint a2;
{
  undefined3 uVar2;
  uint uVar1;
  uint unaff_EDI;
  int in_stack_00000004;
  
  if ((*(int *)(in_stack_00000004 + 0xc0) == 0) && ((*(byte *)(in_stack_00000004 + 0x6a) & 1) != 0))
  {
    a0 = a0 >> (*(byte *)(in_stack_00000004 + 0x67) & 0x1f);
  }
  uVar2 = (undefined3)((uint)in_stack_00000004 >> 8);
  if (a0 == 0) {
    uVar1 = fn_007b6a(CONCAT31(uVar2,7),unaff_EDI);
  }
  else {
    uVar1 = fn_00800a(a0,a1,unaff_EDI);
    if (uVar1 == 0) {
      fn_007b6a(1,unaff_EDI);
      *(byte *)(in_stack_00000004 + 0x65) = *(byte *)(in_stack_00000004 + 0x65) | 1;
      uVar1 = 0;
    }
    else {
      fn_007b6a(CONCAT31(uVar2,7),unaff_EDI);
      uVar1 = 0xfd;
    }
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_007f74   @0x007f74, 149 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_007da4
 */
 /* WARNING: Unknown calling convention */
uint fn_007f74(a0, a1)
    uint a0;
    uint a1;
{
  int iVar1;
  int iVar2;
  code *pcVar3;
  int extraout_ECX;
  int *extraout_EDX;
  uint uVar4;
  int in_stack_00000004;
  uint local_c;
  
  iVar1 = *(int *)a0;
  *(byte *)(in_stack_00000004 + 0x66) = *(byte *)(in_stack_00000004 + 0x66) | 0x80;
  do {
    iVar2 = *(int *)a0;
    pcVar3 = (code *)swi(0xff);
    (*pcVar3)();
    uVar4 = *(uint *)(extraout_ECX + 4);
    *extraout_EDX = iVar2;
    if (uVar4 != 0) {
LAB_00007ffa:
      *(byte *)(in_stack_00000004 + 0x66) = *(byte *)(in_stack_00000004 + 0x66) & 0x7f;
      return uVar4;
    }
    if ((1 < local_c) && (local_c < 5)) {
      uVar4 = 0xb1;
      goto LAB_00007ffa;
    }
    if (((*(byte *)(in_stack_00000004 + 0x66) & 0x80) == 0) ||
       ((a0 = (uint)extraout_EDX, iVar1 != 0 && (*extraout_EDX == 0)))) goto LAB_00007ffa;
  } while( true );
}

/* ----------------------------------------------------------------------
 * fn_00800a   @0x00800a, 495 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_007efa
 * calls: fn_00789c, fn_007b6a, lock_release
 */
 /* WARNING: Unknown calling convention */
uint fn_00800a(a0, a1, a2)
    uint a0;
    uint a1;
    uint a2;
{
  ushort uVar1;
  uint uVar2;
  uint uVar3;
  byte bVar4;
  int extraout_EDX;
  int iVar5;
  uint unaff_EDI;
  int in_stack_00000004;
  uint local_18;
  uint local_c;
  uint local_8;
  
  local_c = 0;
  uVar2 = a0 + *(uint *)(in_stack_00000004 + 0x78);
  if (uVar2 < *(uint *)(in_stack_00000004 + 0x78)) {
    uVar2 = 0;
  }
  *(uint *)(in_stack_00000004 + 0xac) = uVar2 - 1;
  if (uVar2 - 1 < *(uint *)(in_stack_00000004 + 0x7c)) {
    uVar1 = (ushort)(*(byte *)(in_stack_00000004 + 0x65) & 4);
    if (uVar1 != 0) {
      fn_007b6a(CONCAT31((int3)(CONCAT22((short)((uint)in_stack_00000004 >> 0x10),uVar1) >> 8),4)
                   ,unaff_EDI);
    }
  }
  else {
    *(byte *)(in_stack_00000004 + 0x65) = *(byte *)(in_stack_00000004 + 0x65) | 4;
  }
  *(undefined4 *)(in_stack_00000004 + 0xa8) = *(undefined4 *)(in_stack_00000004 + 0x78);
  *(undefined4 *)(in_stack_00000004 + 0xa0) = *(undefined4 *)(in_stack_00000004 + 0x1c);
  fn_00789c(*(uint *)(in_stack_00000004 + 0x90));
  iVar5 = in_stack_00000004;
  do {
    do {
      while( true ) {
        do {
          iVar5 = *(int *)(iVar5 + 0x98);
          if (iVar5 == in_stack_00000004) goto LAB_000081e2;
        } while ((*(int *)(iVar5 + 0xa0) == *(int *)(in_stack_00000004 + 0xa0)) ||
                (*(char *)(iVar5 + 0x65) == '\0'));
        uVar2 = CONCAT31((int3)((uint)in_stack_00000004 >> 8),
                         *(byte *)(in_stack_00000004 + 0x65) | *(byte *)(iVar5 + 0x65)) & 0xffffff02
        ;
        if ((char)uVar2 != '\0') {
          *(int *)a1 = iVar5;
          local_c = 0xfc;
          goto LAB_000081e2;
        }
        if ((*(byte *)(in_stack_00000004 + 0x65) & *(byte *)(iVar5 + 0x65) & 4) == 0) break;
        if ((*(char *)(iVar5 + 0x65) != '\x04') ||
           (uVar1 = (ushort)(*(byte *)(iVar5 + 0x69) & *(byte *)(iVar5 + 0x69)), uVar1 == 0)) {
          *(int *)a1 = iVar5;
          local_c = 0xfc;
          goto LAB_000081e2;
        }
        fn_007b6a(CONCAT31((int3)(CONCAT22((short)(uVar2 >> 0x10),uVar1) >> 8),4),unaff_EDI);
        iVar5 = extraout_EDX;
      }
    } while ((*(byte *)(iVar5 + 0x65) & 1) == 0);
    local_18 = *(uint *)(in_stack_00000004 + 0xa8);
    local_8 = *(uint *)(in_stack_00000004 + 0xac);
    uVar2 = *(uint *)(iVar5 + 0xa8);
    uVar3 = *(uint *)(iVar5 + 0xac);
    if ((*(int *)(in_stack_00000004 + 0xc0) == 0) &&
       (bVar4 = *(byte *)(in_stack_00000004 + 0x6a) & 1, bVar4 != (*(byte *)(iVar5 + 0x6a) & 1))) {
      if (bVar4 == 0) {
        local_18 = local_18 >> (*(byte *)(in_stack_00000004 + 0x67) & 0x1f);
        local_8 = local_8 >> (*(byte *)(in_stack_00000004 + 0x67) & 0x1f);
      }
      else {
        uVar2 = uVar2 >> (*(byte *)(in_stack_00000004 + 0x67) & 0x1f);
        uVar3 = uVar3 >> (*(byte *)(in_stack_00000004 + 0x67) & 0x1f);
      }
    }
  } while ((local_8 < uVar2) || (uVar3 < local_18));
  *(int *)a1 = iVar5;
  local_c = 0xfc;
LAB_000081e2:
  lock_release(*(uint *)(in_stack_00000004 + 0x90));
  return local_c;
}

/* ----------------------------------------------------------------------
 * fn_0081fa   @0x0081fa, 799 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_00517a, fn_006194, fn_006b50, fn_006f10
 * calls: fn_0017f6, fn_00284a, fn_0029e4, fn_006054, fn_006a22, fn_0076fa, fn_0079d2, fn_00851a
 */
 /* WARNING: Unknown calling convention */
uint fn_0081fa(a0, a1, a2)
    uint a0;
    uint a1;
    uint a2;
{
  int iVar1;
  uint uVar2;
  uint uVar3;
  uint extraout_ECX;
  int extraout_ECX_00;
  undefined4 extraout_ECX_01;
  uint extraout_EDX;
  uint unaff_EDI;
  int in_stack_00000004;
  undefined2 uVar4;
  char *local_24;
  uint local_20;
  char *local_1c;
  int local_18;
  int *local_14;
  int local_10;
  int local_c;
  int local_8;
  
  local_18 = in_stack_00000004;
  local_14 = (int *)a1;
  local_8 = 0;
  local_10 = 0;
  local_c = 0;
  local_24 = *(char **)a1;
  *(undefined4 *)(a0 + 0x60) = 0;
  *(char *)(in_stack_00000004 + 2) = *local_24;
  if ((*local_24 != '/') && (*local_24 != '@')) {
    local_10 = *(int *)(a0 + 0x10);
    *(undefined4 *)(a0 + 0x13c) = *(undefined4 *)(local_10 + 0x13c);
    *(undefined1 *)(a0 + 0x67) = *(undefined1 *)(local_10 + 0x67);
    *(undefined1 *)(in_stack_00000004 + 2) = 0x2f;
  }
  if (1 << (*(byte *)(a0 + 0x67) & 0x1f) != *(int *)(a0 + 0x13c)) {
    *(undefined1 *)(a0 + 0x67) = 8;
    for (uVar2 = *(uint *)(a0 + 0x13c) >> 8; uVar2 != 1; uVar2 = uVar2 >> 1) {
      *(char *)(a0 + 0x67) = *(char *)(a0 + 0x67) + '\x01';
    }
  }
  *(uint *)(a0 + 0xb4) = a0;
  *(byte *)(a0 + 0x6a) = *(byte *)(a0 + 0x6a) | 2;
  *(undefined4 *)(a0 + 0xc0) = 0;
  *(undefined4 *)(a0 + 0x8c) = 0;
  if (local_10 == 0) {
    if (*local_24 != '@') {
      uVar2 = fn_00851a((uint)&local_24,(uint)&local_20,unaff_EDI);
      if (uVar2 != 0) {
        return uVar2;
      }
      local_24 = *(char **)(local_18 + 4);
      *local_14 = (int)local_24;
    }
  }
  else {
    *(undefined4 *)(a0 + 0xc0) = *(undefined4 *)(local_10 + 0xc0);
  }
  if (*(char *)(local_18 + 2) == '@') {
    local_24 = local_24 + 1;
  }
  else {
    if ((local_c == 0) && (uVar2 = fn_0029e4(a0), uVar2 != 0)) {
      *local_14 = (int)local_24;
      return uVar2;
    }
    iVar1 = *(int *)(*(int *)(a0 + 4) + 0x20);
    *(undefined4 *)(a0 + 0x8c) = *(undefined4 *)(iVar1 + 4);
    if (*(int *)(a0 + 0xc0) == 0) {
      *(undefined4 *)(a0 + 0xc0) = *(undefined4 *)(iVar1 + 0x28);
    }
  }
  while( true ) {
    uVar4 = 0;
    uVar2 = fn_0076fa(a0);
    if ((uVar2 != 0) || (*(char *)(local_18 + 2) != '/')) goto LAB_000084e0;
    uVar2 = fn_008660(a0,CONCAT22(uVar4,*(undefined2 *)(a0 + 0x14)) & 0xfffffffe | 0x8004,
                         unaff_EDI);
    if (uVar2 != 0) break;
    uVar2 = extraout_ECX;
    fn_006a22(a0,unaff_EDI);
    uVar2 = fn_00851a((uint)&local_24,(uint)&local_20,uVar2);
    if (uVar2 != 0) goto LAB_000084e0;
    uVar2 = a0 + 0xd4;
    *(undefined1 *)(uVar2 + local_20) = 0;
    fn_006054((uint)local_24,local_20,unaff_EDI);
    uVar2 = fn_008776(a0,uVar2);
    if (uVar2 != 0) goto LAB_000084e0;
    do {
      if (*local_1c == '\0') {
        if ((*(char *)(local_18 + 2) != '/') && (local_8 == 0)) {
          local_8 = *(int *)(a0 + 0x78);
          fn_0087de(a0);
        }
      }
      else {
        uVar3 = fn_0087f6(a0 + 0xd4,local_20,unaff_EDI);
        uVar2 = extraout_EDX;
        if (uVar3 == 0) break;
      }
      uVar2 = fn_008840(a0,unaff_EDI);
    } while (uVar2 == 0);
    if (uVar2 != 0) goto LAB_000084e0;
    fn_0087de(a0);
    fn_00284a(*(uint *)(extraout_ECX_00 + 0x3c),unaff_EDI);
    uVar2 = fn_0017f6(a0);
    if (uVar2 != 0) {
      return uVar2;
    }
    fn_0079d2(a0);
    *(undefined4 *)(a0 + 0xc0) = extraout_ECX_01;
    local_24 = *(char **)(local_18 + 4);
    *local_14 = (int)local_24;
  }
  if ((*(byte *)(extraout_ECX + 9) & 0x80) == 0) {
    uVar2 = fn_008758(a0,unaff_EDI);
    return uVar2;
  }
LAB_000084e0:
  if (((uVar2 == 0xd3) && (*(char *)(local_18 + 2) != '/')) && (local_8 == 0)) {
    fn_0087de(a0);
  }
  uVar2 = fn_008758(a0,unaff_EDI);
  return uVar2;
}

/* ----------------------------------------------------------------------
 * fn_00851a   @0x00851a, 325 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0054b8, fn_0081fa
 * calls: os_f_prsnam
 */
 /* WARNING: Unknown calling convention */
uint fn_00851a(a0, a1, a2)
    uint a0;
    uint a1;
    uint a2;
{
  byte bVar1;
  char *pcVar2;
  uint uVar3;
  char *pcVar4;
  int *extraout_EDX;
  uint unaff_EDI;
  ushort *in_stack_00000004;
  
  if ((**(char **)a0 == '/') && (pcVar2 = *(char **)a0 + 1, *pcVar2 == '/')) {
    *(char **)a0 = pcVar2;
  }
  uVar3 = os_f_prsnam(*(uint *)a0,(uint)(in_stack_00000004 + 1),(uint)(in_stack_00000004 + 2),
                      unaff_EDI);
  pcVar2 = *(char **)(in_stack_00000004 + 2);
  *extraout_EDX = (int)pcVar2;
  *(int *)(in_stack_00000004 + 2) = *extraout_EDX + *(int *)a1;
  if (uVar3 == 0) {
    if (0x2b < *(uint *)a1) {
      return 0xd7;
    }
    bVar1 = (byte)in_stack_00000004[1];
    if (bVar1 != 0x20) {
      if (bVar1 < 0x21) {
        if ((bVar1 != 0xd) && ((0xd < bVar1 || (bVar1 != 0)))) {
          return 0xd7;
        }
      }
      else if (bVar1 != 0x40) {
        if (0x40 < bVar1) {
          return 0xd7;
        }
        if (bVar1 != 0x2f) {
          return 0xd7;
        }
        if ((((*in_stack_00000004 & 0x8000) != 0) && (**(char **)(in_stack_00000004 + 2) == '/')) &&
           (*(char *)(*(int *)(in_stack_00000004 + 2) + 1) == '\0')) {
          *(undefined1 *)(in_stack_00000004 + 1) = 0;
          *(int *)(in_stack_00000004 + 2) = *(int *)(in_stack_00000004 + 2) + 1;
        }
      }
    }
    return 0;
  }
  *(undefined4 *)a1 = 0;
  do {
    pcVar4 = pcVar2;
    if ((char)in_stack_00000004[1] != '.') goto LAB_000085ac;
    *(int *)a1 = *(int *)a1 + 1;
    *(char *)(in_stack_00000004 + 1) = pcVar4[1];
    pcVar2 = pcVar4 + 1;
  } while (*(uint *)a1 < 3);
  *(undefined1 *)(in_stack_00000004 + 1) = 0x2f;
  pcVar4 = pcVar4 + -1;
  *(int *)a1 = *(int *)a1 + -1;
LAB_000085ac:
  if (*(int *)a1 != 0) {
    *(char **)(in_stack_00000004 + 2) = pcVar4;
    if ((((*in_stack_00000004 & 0x8000) != 0) && (*pcVar4 == '/')) && (pcVar4[1] == '\0')) {
      *(undefined1 *)(in_stack_00000004 + 1) = 0;
      *(int *)(in_stack_00000004 + 2) = *(int *)(in_stack_00000004 + 2) + 1;
    }
    return 0;
  }
  return 0xd7;
}
