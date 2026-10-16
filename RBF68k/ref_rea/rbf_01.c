/*
 * rbf_01.c - rbf part 1
 *
 * Original address range: 0x000000 - 0x001332 of the V4.9 kernel module.
 * Functions 0xac to 0x11b0 (support routines)
 * Generated from the Ghidra export; see tools/gen_src.py. Hand edits belong in hand/*.inc.
 */
#include "kernel.h"

/* ----------------------------------------------------------------------
 * fn_0000ac   @0x0000ac, 5 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 */
/* jump thunk: forwards to fn_0000b8 */
uint fn_0000ac(a0)
    uint a0;
{
  return fn_0000b8(a0);
}

/* ----------------------------------------------------------------------
 * fn_0000b1   @0x0000b1, 6 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 */
 /* WARNING: Unknown calling convention */
uint fn_0000b1(a0)
    uint a0;
{
  char cVar1;
  uint uVar2;
  char *in_ECX;
  uint uVar3;
  char in_DH;
  uint in_stack_00000004;
  
  cVar1 = (char)a0;
  *(char *)a0 = *(char *)a0 + cVar1;
  *(char *)a0 = *(char *)a0 + cVar1;
  *(char *)a0 = *(char *)a0 + cVar1;
  *in_ECX = *in_ECX + in_DH;
  *(char *)a0 = *(char *)a0 + cVar1;
  *(char *)a0 = *(char *)a0 + cVar1;
  uVar2 = 0;
  uVar3 = in_stack_00000004 >> 2;
  do {
    uVar2 = uVar2 ^ *(uint *)a0;
    a0 = a0 + 4;
    uVar3 = uVar3 - 1;
  } while (uVar3 != 0);
  return uVar2;
}

/* ----------------------------------------------------------------------
 * fn_0000b8   @0x0000b8, 3 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0000ac
 */
 /* WARNING: Unknown calling convention */
uint fn_0000b8(a0)
    uint a0;
{
  return 0;
}

/* ----------------------------------------------------------------------
 * fn_0000bc   @0x0000bc, 4 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 */
 /* WARNING: Unknown calling convention */
uint fn_0000bc(a0)
    uint a0;
{
  uint uVar1;
  uint uVar2;
  uint in_stack_00000004;
  
  *(char *)a0 = *(char *)a0 + (char)a0;
  *(char *)a0 = *(char *)a0 + (char)a0;
  uVar1 = 0;
  uVar2 = in_stack_00000004 >> 2;
  do {
    uVar1 = uVar1 ^ *(uint *)a0;
    a0 = a0 + 4;
    uVar2 = uVar2 - 1;
  } while (uVar2 != 0);
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_0000c0   @0x0000c0, 23 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_001c5a, fn_001d1a, fn_002180, fn_003374, fn_00352c, fn_004a76, fn_006194
 */
 /* WARNING: Unknown calling convention */
uint fn_0000c0(a0)
    uint a0;
{
  uint uVar1;
  uint uVar2;
  uint in_stack_00000004;
  
  uVar1 = 0;
  uVar2 = in_stack_00000004 >> 2;
  do {
    uVar1 = uVar1 ^ *(uint *)a0;
    a0 = a0 + 4;
    uVar2 = uVar2 - 1;
  } while (uVar2 != 0);
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_0000d8   @0x0000d8, 17 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 */
 /* WARNING: Removing unreachable block (ram,0x00000251) */
 /* WARNING: Unknown calling convention */
uint fn_0000d8(a0)
    uint a0;
{
  byte *pbVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  undefined1 uVar5;
  undefined4 uVar6;
  uint uVar7;
  int in_ECX;
  int iVar8;
  int extraout_ECX;
  int extraout_ECX_00;
  byte bVar9;
  char *in_EDX;
  int *piVar10;
  int *extraout_EDX;
  undefined4 extraout_EDX_00;
  Statics *g;
  int unaff_EBP;
  undefined4 *puVar11;
  int *piVar12;
  uint uVar13;
  int *piVar14;
  undefined3 unaff_retaddr;
  undefined4 in_stack_fffffffc;
  
  *(char *)a0 = *(char *)a0 + (char)a0;
  *in_EDX = *in_EDX + (char)((uint)g >> 8);
  uVar13 = CONCAT31(unaff_retaddr,(char)((uint)in_stack_fffffffc >> 0x18)) - 1;
  bVar9 = (byte)(in_EDX + 1);
  *(char *)(unaff_EBP + -0x77) = *(char *)(unaff_EBP + -0x77) + bVar9;
  uVar6 = in(0x83);
  uVar5 = in((short)(in_EDX + 1));
  *(byte *)(in_ECX + 0x52) = *(byte *)(in_ECX + 0x52) ^ bVar9;
  *(uint *)(unaff_EBP + -0x14) = CONCAT31((int3)((uint)uVar6 >> 8),uVar5);
  *(undefined4 *)(unaff_EBP + -0x10) = 0;
  *(undefined4 *)(unaff_EBP + -0x24) = 0;
  if (*(int *)(*(int *)(unaff_EBP + -0x14) + 0xc0) == 0) {
    return 0xf8;
  }
  fn_00789c(*(int *)(*(int *)(unaff_EBP + -0x14) + 0x70) + 0x28);
  iVar2 = *(int *)(*(int *)(unaff_EBP + -0x14) + 0x70);
  iVar8 = iVar2 + 0x38;
  iVar3 = *(int *)(iVar2 + 0x4c);
  if ((*(byte *)(iVar2 + 0x42) & 1) != 0) {
    uVar7 = *(int *)(*(int *)(unaff_EBP + -0x14) + 0x13c) - 0x30;
    *(uint *)(unaff_EBP + -0x10) = uVar7;
    if (*(uint *)(*(int *)(unaff_EBP + -0x14) + 0x7c) <= uVar7) {
      *(undefined4 *)(iVar2 + 0x4c) = *(undefined4 *)(*(int *)(unaff_EBP + -0x14) + 0x7c);
      pbVar1 = (byte *)(*(int *)(unaff_EBP + -0x14) + 100);
      *pbVar1 = *pbVar1 | 1;
      uVar13 = fn_00351a(*(uint *)(unaff_EBP + -0x14));
      *(uint *)(unaff_EBP + -0xc) = uVar13;
joined_r0x00000557:
      if (uVar13 == 0) {
LAB_0000055d:
        fn_00057e(*(uint *)(unaff_EBP + -0x14));
        lock_release(*(int *)(*(int *)(unaff_EBP + -0x14) + 0x70) + 0x28);
        return 0;
      }
      goto LAB_000004a5;
    }
    if (iVar3 != 0) {
      *(undefined4 *)(unaff_EBP + -0x30) = *(undefined4 *)(unaff_EBP + -0x10);
      uVar7 = os_f_srqmem(unaff_EBP - 0x30,0,uVar13);
      *(uint *)(unaff_EBP + -0xc) = uVar7;
      if (uVar7 != 0) goto LAB_000004a5;
      fn_006054(extraout_ECX + 0x30,*(uint *)(unaff_EBP + -0x10),uVar13);
      iVar8 = extraout_ECX_00;
    }
    puVar11 = (undefined4 *)(iVar8 + 0x30);
    *(undefined4 **)(*(int *)(unaff_EBP + -0x14) + 0x9c) = puVar11;
    uVar7 = *(uint *)(unaff_EBP + -0x10) >> 3;
    while (uVar7 = uVar7 - 1, 0 < (int)uVar7) {
      *puVar11 = 0;
      puVar11[1] = 0;
      puVar11 = puVar11 + 2;
    }
    puVar11[1] = *(undefined4 *)(*(int *)(unaff_EBP + -0x14) + 0xc0);
    *puVar11 = puVar11[1];
    *(undefined4 *)(*(int *)(unaff_EBP + -0x14) + 0x74) =
         *(undefined4 *)(*(int *)(unaff_EBP + -0x14) + 0x70);
    iVar2 = *(int *)(*(int *)(unaff_EBP + -0x14) + 0x70);
    *(undefined4 *)(*(int *)(*(int *)(unaff_EBP + -0x14) + 0x70) + 8) = 0;
    *(undefined4 *)(iVar2 + 0xc) = 0;
    *(byte *)(iVar8 + 10) = *(byte *)(iVar8 + 10) & 0xfe;
  }
  do {
    iVar2 = **(int **)(*(int *)(unaff_EBP + -0x14) + 0x70);
    uVar7 = *(int *)(iVar2 + 8) + *(int *)(iVar2 + 0xc);
    if (*(uint *)(*(int *)(unaff_EBP + -0x14) + 0x7c) <= uVar7) {
      uVar7 = *(uint *)(*(int *)(unaff_EBP + -0x14) + 0x78);
      uVar4 = *(uint *)(*(int *)(unaff_EBP + -0x14) + 0x80);
      if ((*(uint *)(*(int *)(unaff_EBP + -0x14) + 0x88) <= uVar7 - uVar4) || (uVar7 < uVar4)) {
        uVar7 = fn_002eca(unaff_EBP - 0x20,uVar13);
        *(uint *)(unaff_EBP + -0xc) = uVar7;
        if (uVar7 != 0) break;
      }
      if (*(int *)(unaff_EBP + -0x24) == 0) goto LAB_0000055d;
      lock_release(*(int *)(*(int *)(unaff_EBP + -0x14) + 0x70) + 0x28);
      uVar7 = fn_0017f6(*(uint *)(unaff_EBP + -0x14));
      *(uint *)(unaff_EBP + -0xc) = uVar7;
      if (uVar7 == 0) {
        uVar7 = fn_0015c4(*(uint *)(*(int *)(unaff_EBP + -0x14) + 0x84),uVar13);
        *(uint *)(unaff_EBP + -0xc) = uVar7;
        if (uVar7 == 0) {
          fn_00789c(*(int *)(*(int *)(unaff_EBP + -0x14) + 0x70) + 0x28);
          *(undefined4 *)(*(int *)(*(int *)(unaff_EBP + -0x14) + 0x6c) + 8) = 3;
          uVar7 = *(uint *)(*(int *)(*(int *)(unaff_EBP + -0x14) + 0x6c) + 0x28);
          fn_006054(*(uint *)(unaff_EBP + -0x24),*(uint *)(unaff_EBP + -0x10),uVar13);
          uVar13 = os_f_srtmem_8db8(*(uint *)(unaff_EBP + -0x10),uVar7);
          *(uint *)(unaff_EBP + -0xc) = uVar13;
          goto joined_r0x00000557;
        }
      }
      os_f_srtmem_8db8(*(uint *)(unaff_EBP + -0x10),uVar13);
      break;
    }
    uVar7 = (*(int *)(*(int *)(unaff_EBP + -0x14) + 0x7c) - uVar7) +
            *(int *)(*(int *)(unaff_EBP + -0x14) + 0x13c) + -1 >>
            (*(byte *)(*(int *)(unaff_EBP + -0x14) + 0x67) & 0x1f);
    if (uVar7 < *(uint *)(*(int *)(unaff_EBP + -0x14) + 0x128)) {
      uVar7 = *(uint *)(*(int *)(unaff_EBP + -0x14) + 0x128);
    }
    *(uint *)(unaff_EBP + -0x28) = uVar7;
    *(int *)(unaff_EBP + -0x1c) = *(int *)(unaff_EBP + -0x14);
    *(undefined4 *)(unaff_EBP + -8) = **(undefined4 **)(*(int *)(unaff_EBP + -0x14) + 0x70);
    if (*(int *)(unaff_EBP + -8) == *(int *)(*(int *)(unaff_EBP + -0x1c) + 0x74)) {
      piVar14 = *(int **)(*(int *)(unaff_EBP + -0x1c) + 0x9c);
      *(undefined4 *)(unaff_EBP + -4) = *(undefined4 *)(*(int *)(unaff_EBP + -0x1c) + 0x80);
    }
    else {
      piVar14 = (int *)(*(int *)(unaff_EBP + -8) + 0x68);
      *(undefined4 *)(unaff_EBP + -4) = *(undefined4 *)(*(int *)(unaff_EBP + -8) + 8);
    }
    piVar12 = (int *)(*(int *)(unaff_EBP + -8) + *(int *)(*(int *)(unaff_EBP + -0x1c) + 0x13c) +
                     0x30);
    piVar10 = (int *)0x0;
    for (; (piVar14[1] != 0 && (piVar14 != piVar12)); piVar14 = piVar14 + 2) {
      *(int *)(unaff_EBP + -4) =
           *(int *)(unaff_EBP + -4) +
           (piVar14[1] << (*(byte *)(*(int *)(unaff_EBP + -0x1c) + 0x67) & 0x1f));
      piVar10 = piVar14;
    }
    if (piVar10 == (int *)0x0) {
      uVar7 = 0;
    }
    else {
      uVar7 = *piVar10 + piVar10[1];
    }
    uVar7 = fn_0005ee(unaff_EBP - 0x28,uVar7,*(uint *)(unaff_EBP + -0x1c),uVar13);
    if (uVar7 == 0) {
      if ((extraout_EDX == (int *)0x0) ||
         (extraout_EDX[1] + *extraout_EDX != *(int *)(unaff_EBP + -0x2c))) {
        if (piVar14 == piVar12) {
          uVar7 = fn_00380a(*(uint *)(unaff_EBP + -0x1c));
          if (uVar7 != 0) {
            *(undefined4 *)(*(int *)(unaff_EBP + -0x1c) + 0x84) = *(undefined4 *)(unaff_EBP + -0x2c)
            ;
            *(int *)(*(int *)(unaff_EBP + -0x1c) + 0x88) =
                 *(int *)(unaff_EBP + -0x28) <<
                 (*(byte *)(*(int *)(unaff_EBP + -0x1c) + 0x67) & 0x1f);
            *(undefined4 *)(*(int *)(unaff_EBP + -0x1c) + 0x9c) = 0;
            fn_000770(*(uint *)(unaff_EBP + -0x1c));
            *(undefined4 *)(unaff_EBP + -0x18) = extraout_EDX_00;
            goto LAB_0000045b;
          }
          *(undefined4 *)(unaff_EBP + -8) = **(undefined4 **)(*(int *)(unaff_EBP + -0x1c) + 0x70);
          piVar14 = (int *)(*(int *)(unaff_EBP + -8) + 0x68);
          *(undefined4 *)(unaff_EBP + -4) = *(undefined4 *)(*(int *)(unaff_EBP + -8) + 8);
        }
        piVar14[1] = *(int *)(unaff_EBP + -0x28);
        *piVar14 = *(int *)(unaff_EBP + -0x2c);
      }
      else {
        *(int *)(unaff_EBP + -4) =
             *(int *)(unaff_EBP + -4) -
             (extraout_EDX[1] << (*(byte *)(*(int *)(unaff_EBP + -0x1c) + 0x67) & 0x1f));
        extraout_EDX[1] = extraout_EDX[1] + *(int *)(unaff_EBP + -0x28);
        piVar14 = extraout_EDX;
      }
      *(int *)(*(int *)(unaff_EBP + -0x1c) + 0x88) =
           piVar14[1] << (*(byte *)(*(int *)(unaff_EBP + -0x1c) + 0x67) & 0x1f);
      *(int *)(*(int *)(unaff_EBP + -0x1c) + 0x84) = *piVar14;
      *(undefined4 *)(*(int *)(unaff_EBP + -0x1c) + 0x80) = *(undefined4 *)(unaff_EBP + -4);
      *(int **)(*(int *)(unaff_EBP + -0x1c) + 0x9c) = piVar14;
      *(undefined4 *)(*(int *)(unaff_EBP + -0x1c) + 0x74) = *(undefined4 *)(unaff_EBP + -8);
      fn_0033f6(*(uint *)(unaff_EBP + -8),uVar13);
      *(int *)(*(int *)(*(int *)(unaff_EBP + -0x1c) + 0x70) + 0x4c) =
           *(int *)(unaff_EBP + -4) + *(int *)(*(int *)(unaff_EBP + -0x1c) + 0x88);
      pbVar1 = (byte *)(*(int *)(unaff_EBP + -0x1c) + 100);
      *pbVar1 = *pbVar1 | 1;
      uVar7 = fn_00351a(*(uint *)(unaff_EBP + -0x1c));
      if (uVar7 == 0) {
        if ((*(int *)(unaff_EBP + -8) == *(int *)(*(int *)(unaff_EBP + -0x1c) + 0x70)) ||
           (uVar7 = fn_00352c(*(uint *)(*(int *)(unaff_EBP + -0x1c) + 0x74),uVar13), uVar7 == 0))
        {
          *(undefined4 *)(unaff_EBP + -0x18) = 0;
        }
        else {
          *(uint *)(unaff_EBP + -0x18) = uVar7;
        }
      }
      else {
        *(uint *)(unaff_EBP + -0x18) = uVar7;
      }
    }
    else {
      *(uint *)(unaff_EBP + -0x18) = uVar7;
    }
LAB_0000045b:
    *(undefined4 *)(unaff_EBP + -0xc) = *(undefined4 *)(unaff_EBP + -0x18);
  } while (*(int *)(unaff_EBP + -0xc) == 0);
LAB_000004a5:
  lock_release(*(int *)(*(int *)(unaff_EBP + -0x14) + 0x70) + 0x28);
  return *(uint *)(unaff_EBP + -0xc);
}

/* ----------------------------------------------------------------------
 * fn_0000ea   @0x0000ea, 1171 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_004464
 * calls: fn_00057e, fn_0005ee, fn_000770, fn_0015c4, fn_0017f6, fn_002eca, fn_0033f6, fn_00351a
 */
 /* WARNING: Removing unreachable block (ram,0x00000251) */
 /* WARNING: Unknown calling convention */
uint fn_0000ea(a0)
    uint a0;
{
  int iVar1;
  uint uVar2;
  int iVar3;
  int extraout_ECX;
  int extraout_ECX_00;
  int *piVar4;
  int *extraout_EDX;
  uint extraout_EDX_00;
  undefined4 *puVar5;
  int *piVar6;
  uint unaff_EDI;
  int *piVar7;
  uint local_34;
  int local_30;
  uint local_2c;
  uint local_28;
  undefined1 local_24 [4];
  uint local_20;
  uint local_1c;
  uint local_18;
  uint local_14;
  uint local_10;
  uint local_c;
  int local_8;
  
  local_14 = 0;
  local_28 = 0;
  if (*(int *)(a0 + 0xc0) == 0) {
    return 0xf8;
  }
  local_18 = a0;
  fn_00789c(*(int *)(a0 + 0x70) + 0x28);
  iVar1 = *(int *)(local_18 + 0x70);
  iVar3 = iVar1 + 0x38;
  if ((*(byte *)(iVar1 + 0x42) & 1) != 0) {
    local_14 = *(int *)(local_18 + 0x13c) - 0x30;
    if (*(uint *)(local_18 + 0x7c) <= local_14) {
      *(undefined4 *)(iVar1 + 0x4c) = *(undefined4 *)(local_18 + 0x7c);
      *(byte *)(local_18 + 100) = *(byte *)(local_18 + 100) | 1;
      local_10 = fn_00351a(local_18);
joined_r0x00000557:
      if (local_10 == 0) {
LAB_0000055d:
        fn_00057e(local_18);
        lock_release(*(int *)(local_18 + 0x70) + 0x28);
        return 0;
      }
      goto LAB_000004a5;
    }
    if (*(int *)(iVar1 + 0x4c) != 0) {
      local_34 = local_14;
      local_10 = os_f_srqmem((uint)&local_34,0,unaff_EDI);
      if (local_10 != 0) goto LAB_000004a5;
      fn_006054(extraout_ECX + 0x30,local_14,unaff_EDI);
      iVar3 = extraout_ECX_00;
    }
    puVar5 = (undefined4 *)(iVar3 + 0x30);
    *(undefined4 **)(local_18 + 0x9c) = puVar5;
    uVar2 = local_14 >> 3;
    while (uVar2 = uVar2 - 1, 0 < (int)uVar2) {
      *puVar5 = 0;
      puVar5[1] = 0;
      puVar5 = puVar5 + 2;
    }
    puVar5[1] = *(undefined4 *)(local_18 + 0xc0);
    *puVar5 = puVar5[1];
    *(undefined4 *)(local_18 + 0x74) = *(undefined4 *)(local_18 + 0x70);
    iVar1 = *(int *)(local_18 + 0x70);
    *(undefined4 *)(*(int *)(local_18 + 0x70) + 8) = 0;
    *(undefined4 *)(iVar1 + 0xc) = 0;
    *(byte *)(iVar3 + 10) = *(byte *)(iVar3 + 10) & 0xfe;
  }
  do {
    uVar2 = *(int *)(**(int **)(local_18 + 0x70) + 8) + *(int *)(**(int **)(local_18 + 0x70) + 0xc);
    if (*(uint *)(local_18 + 0x7c) <= uVar2) {
      if (((*(uint *)(local_18 + 0x78) - *(uint *)(local_18 + 0x80) < *(uint *)(local_18 + 0x88)) &&
          (*(uint *)(local_18 + 0x80) <= *(uint *)(local_18 + 0x78))) ||
         (local_10 = fn_002eca((uint)local_24,unaff_EDI), local_10 == 0)) {
        if (local_28 == 0) goto LAB_0000055d;
        lock_release(*(int *)(local_18 + 0x70) + 0x28);
        local_10 = fn_0017f6(local_18);
        if ((local_10 == 0) &&
           (local_10 = fn_0015c4(*(uint *)(local_18 + 0x84),unaff_EDI), local_10 == 0)) {
          fn_00789c(*(int *)(local_18 + 0x70) + 0x28);
          *(undefined4 *)(*(int *)(local_18 + 0x6c) + 8) = 3;
          uVar2 = *(uint *)(*(int *)(local_18 + 0x6c) + 0x28);
          fn_006054(local_28,local_14,unaff_EDI);
          local_10 = os_f_srtmem_8db8(local_14,uVar2);
          goto joined_r0x00000557;
        }
        os_f_srtmem_8db8(local_14,unaff_EDI);
      }
      break;
    }
    local_2c = (*(int *)(local_18 + 0x7c) - uVar2) + *(int *)(local_18 + 0x13c) + -1 >>
               (*(byte *)(local_18 + 0x67) & 0x1f);
    if (local_2c < *(uint *)(local_18 + 0x128)) {
      local_2c = *(uint *)(local_18 + 0x128);
    }
    local_20 = local_18;
    local_c = **(uint **)(local_18 + 0x70);
    if (local_c == *(uint *)(local_18 + 0x74)) {
      piVar7 = *(int **)(local_18 + 0x9c);
      local_8 = *(int *)(local_18 + 0x80);
    }
    else {
      piVar7 = (int *)(local_c + 0x68);
      local_8 = *(int *)(local_c + 8);
    }
    piVar6 = (int *)(local_c + *(int *)(local_18 + 0x13c) + 0x30);
    piVar4 = (int *)0x0;
    for (; (piVar7[1] != 0 && (piVar7 != piVar6)); piVar7 = piVar7 + 2) {
      local_8 = local_8 + (piVar7[1] << (*(byte *)(local_18 + 0x67) & 0x1f));
      piVar4 = piVar7;
    }
    if (piVar4 == (int *)0x0) {
      uVar2 = 0;
    }
    else {
      uVar2 = *piVar4 + piVar4[1];
    }
    uVar2 = fn_0005ee((uint)&local_2c,uVar2,local_18,unaff_EDI);
    if (uVar2 == 0) {
      if ((extraout_EDX == (int *)0x0) || (extraout_EDX[1] + *extraout_EDX != local_30)) {
        if (piVar7 == piVar6) {
          uVar2 = fn_00380a(local_20);
          if (uVar2 != 0) {
            *(int *)(local_20 + 0x84) = local_30;
            *(uint *)(local_20 + 0x88) = local_2c << (*(byte *)(local_20 + 0x67) & 0x1f);
            *(undefined4 *)(local_20 + 0x9c) = 0;
            fn_000770(local_20);
            uVar2 = extraout_EDX_00;
            goto LAB_0000045b;
          }
          local_c = **(uint **)(local_20 + 0x70);
          piVar7 = (int *)(local_c + 0x68);
          local_8 = *(int *)(local_c + 8);
        }
        piVar7[1] = local_2c;
        *piVar7 = local_30;
      }
      else {
        local_8 = local_8 - (extraout_EDX[1] << (*(byte *)(local_20 + 0x67) & 0x1f));
        extraout_EDX[1] = extraout_EDX[1] + local_2c;
        piVar7 = extraout_EDX;
      }
      *(int *)(local_20 + 0x88) = piVar7[1] << (*(byte *)(local_20 + 0x67) & 0x1f);
      *(int *)(local_20 + 0x84) = *piVar7;
      *(int *)(local_20 + 0x80) = local_8;
      *(int **)(local_20 + 0x9c) = piVar7;
      *(uint *)(local_20 + 0x74) = local_c;
      fn_0033f6(local_c,unaff_EDI);
      *(int *)(*(int *)(local_20 + 0x70) + 0x4c) = local_8 + *(int *)(local_20 + 0x88);
      *(byte *)(local_20 + 100) = *(byte *)(local_20 + 100) | 1;
      uVar2 = fn_00351a(local_20);
      if ((uVar2 == 0) &&
         ((local_c == *(uint *)(local_20 + 0x70) ||
          (uVar2 = fn_00352c(*(uint *)(local_20 + 0x74),unaff_EDI), uVar2 == 0)))) {
        local_1c = 0;
        uVar2 = local_1c;
      }
    }
LAB_0000045b:
    local_1c = uVar2;
    local_10 = local_1c;
  } while (local_1c == 0);
LAB_000004a5:
  lock_release(*(int *)(local_18 + 0x70) + 0x28);
  return local_10;
}

/* ----------------------------------------------------------------------
 * fn_00057e   @0x00057e, 84 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0000ea, fn_000a76
 * calls: fn_00789c, lock_release
 */
 /* WARNING: Unknown calling convention */
uint fn_00057e(a0)
    uint a0;
{
  uint extraout_ECX;
  int extraout_EDX;
  int iVar1;
  
  *(undefined4 *)(*(int *)(a0 + 0x70) + 0x4c) = *(undefined4 *)(a0 + 0x7c);
  fn_00789c(*(uint *)(a0 + 0x90));
  iVar1 = extraout_EDX;
  while (iVar1 = *(int *)(iVar1 + 0x98), iVar1 != extraout_EDX) {
    *(undefined4 *)(iVar1 + 0x7c) = *(undefined4 *)(extraout_EDX + 0x7c);
  }
  lock_release(*(uint *)(extraout_EDX + 0x90));
  return extraout_ECX;
}

/* ----------------------------------------------------------------------
 * fn_0005d2   @0x0005d2, 28 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * calls: os_f_srqmem
 */
 /* WARNING: Unknown calling convention */
uint fn_0005d2(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  
  uVar1 = os_f_srqmem((uint)GHIDRA_STACK_PTR(0xfffffff8),0,a0);
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_0005ee   @0x0005ee, 386 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0000ea, fn_00380a, fn_006194
 * calls: fn_000d72, fn_0011b0, fn_00789c, lock_release, os_f_srtmem_8db8
 */
 /* WARNING: Unknown calling convention */
uint fn_0005ee(a0, a1, a2, a3)
    uint a0;
    uint a1;
    uint a2;
    uint a3;
{
  uint a3_00;
  int *piVar1;
  uint uVar2;
  int iVar3;
  int *piVar4;
  int extraout_EDX;
  int extraout_EDX_00;
  int iVar5;
  int *piVar6;
  int *piVar7;
  uint unaff_EDI;
  int *in_stack_00000004;
  uint local_c;
  
  a3_00 = *(uint *)(*(int *)(a2 + 4) + 0x20);
  fn_00789c(a3_00 + 0x74);
  iVar5 = extraout_EDX;
  if ((*(int *)(a3_00 + 0x84) != 0) ||
     (local_c = fn_000d72(a2,unaff_EDI), iVar5 = extraout_EDX_00, local_c == 0)) {
    piVar1 = *(int **)(a3_00 + 0x84);
    uVar2 = *(uint *)a0;
    piVar4 = piVar1;
    piVar6 = piVar1;
    do {
      piVar7 = piVar4;
      if (((iVar5 == *piVar4) && (*(char *)(a2 + 0x68) != '\0')) || (uVar2 == piVar4[1])) break;
      if (uVar2 < (uint)piVar4[1]) {
        if ((uint)piVar6[1] < uVar2) {
          piVar6 = piVar4;
        }
      }
      else if ((uint)piVar6[1] < (uint)piVar4[1]) {
        piVar6 = piVar4;
      }
      piVar4 = (int *)piVar4[3];
      piVar7 = piVar6;
    } while (piVar4 != piVar1);
    if (piVar7[1] == 0) {
      lock_release(a3_00 + 0x74);
      return 0xf8;
    }
    *in_stack_00000004 = *piVar7;
    if ((uint)piVar7[1] < uVar2) {
      *(int *)a0 = piVar7[1];
    }
    iVar5 = piVar7[1];
    iVar3 = *(int *)a0;
    piVar7[1] = iVar5 - iVar3;
    if (iVar5 - iVar3 != 0) {
      *piVar7 = *piVar7 + *(int *)a0;
    }
    *(int *)(a3_00 + 0x88) = piVar7[3];
    local_c = fn_0011b0(*(uint *)a0,(uint)piVar7,a2,a3_00,unaff_EDI);
    if (local_c == 0) {
      if ((piVar7[1] == 0) && (piVar7 != (int *)piVar7[3])) {
        *(int *)(piVar7[3] + 8) = piVar7[2];
        *(int *)(piVar7[2] + 0xc) = piVar7[3];
        if (piVar7 == piVar1) {
          *(int *)(a3_00 + 0x84) = piVar7[3];
        }
        os_f_srtmem_8db8(0x10,unaff_EDI);
      }
      lock_release(a3_00 + 0x74);
      return 0;
    }
  }
  lock_release(a3_00 + 0x74);
  return local_c;
}

/* ----------------------------------------------------------------------
 * fn_000770   @0x000770, 773 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0000ea, fn_000a76, fn_003944, fn_0067ea, fn_006f10
 * calls: fn_000d72, fn_0011b0, fn_002b84, fn_00789c, lock_release, os_f_srtmem_8db8, os_f_srqmem
 */
 /* WARNING: Unknown calling convention */
uint fn_000770(a0)
    uint a0;
{
  uint uVar1;
  uint uVar2;
  uint *extraout_EDX;
  uint *extraout_EDX_00;
  uint *extraout_EDX_01;
  uint *extraout_EDX_02;
  uint *puVar3;
  uint unaff_EDI;
  undefined4 local_40;
  undefined1 local_3c [12];
  undefined4 local_30;
  undefined2 local_2c;
  undefined4 *local_2a;
  uint *local_24;
  undefined4 local_20;
  uint local_1c;
  undefined4 *local_18;
  uint local_14;
  uint local_10;
  uint *local_c;
  uint *local_8;
  
  if (*(int *)(a0 + 0x88) == 0) {
    local_10 = 0;
  }
  else {
    local_14 = *(uint *)(*(int *)(a0 + 4) + 0x20);
    local_18 = (undefined4 *)a0;
    fn_00789c(local_14 + 0x74);
    if ((*(int *)(local_14 + 0x84) != 0) ||
       (local_10 = fn_000d72((uint)local_18,unaff_EDI), local_10 == 0)) {
      local_40 = 0x10;
      local_10 = os_f_srqmem((uint)&local_40,0,unaff_EDI);
      if (local_10 == 0) {
        local_8 = local_24;
        local_24[1] = (local_18[0x22] + local_18[0x4f]) - 1 >>
                      (*(byte *)((int)local_18 + 0x67) & 0x1f);
        *local_24 = local_18[0x21];
        puVar3 = *(uint **)(local_14 + 0x84);
        local_24[3] = 0;
        uVar2 = 0;
        local_c = puVar3;
        if ((*(byte *)(local_14 + 0xc9) & 1) == 0) {
          local_20 = local_18[0x21];
          local_1c = (local_18[0x22] + local_18[0x4f]) - 1 >>
                     (*(byte *)((int)local_18 + 0x67) & 0x1f);
          local_2a = &local_20;
          local_30 = *local_18;
          local_2c = 0x81;
          uVar2 = fn_002b84(0,4,(uint)local_18,(uint)local_3c,unaff_EDI);
          puVar3 = extraout_EDX;
          if (uVar2 != 0) {
            if (uVar2 != 0xd0) {
              return uVar2;
            }
            *(byte *)(local_14 + 0xc9) = *(byte *)(local_14 + 0xc9) | 1;
          }
        }
        do {
          local_10 = uVar2;
          if (*puVar3 == *local_8) {
            if (local_8[1] < puVar3[1]) {
              uVar2 = puVar3[1];
            }
            else {
              uVar2 = local_8[1];
            }
            puVar3[1] = uVar2;
            os_f_srtmem_8db8(0x10,unaff_EDI);
            puVar3 = extraout_EDX_00;
            local_8 = extraout_EDX_00;
            break;
          }
          if (*local_8 < *puVar3) {
            if (local_8[1] + *local_8 == *puVar3) {
              *puVar3 = *puVar3 - local_8[1];
              puVar3[1] = puVar3[1] + local_8[1];
              os_f_srtmem_8db8(0x10,unaff_EDI);
              puVar3 = (uint *)extraout_EDX_01[3];
              local_8 = extraout_EDX_01;
            }
            else {
              local_8[3] = (uint)puVar3;
              local_8[2] = puVar3[2];
              *(uint **)(local_8[2] + 0xc) = local_8;
              puVar3[2] = (uint)local_8;
            }
            break;
          }
          if (*puVar3 + puVar3[1] == *local_8) {
            uVar2 = local_8[1];
            uVar1 = puVar3[1];
            puVar3[1] = uVar2 + uVar1;
            if (uVar2 + uVar1 + *puVar3 == *(int *)puVar3[3]) {
              puVar3[1] = puVar3[1] + ((int *)puVar3[3])[1];
              uVar2 = *(uint *)(puVar3[3] + 0xc);
              puVar3[3] = uVar2;
              *(uint **)(uVar2 + 8) = puVar3;
              os_f_srtmem_8db8(0x10,unaff_EDI);
            }
            os_f_srtmem_8db8(0x10,unaff_EDI);
            puVar3 = extraout_EDX_02;
            local_8 = extraout_EDX_02;
            break;
          }
          puVar3 = (uint *)puVar3[3];
          uVar2 = local_10;
        } while (puVar3 != local_c);
        if ((puVar3 == local_c) && (local_8[3] == 0)) {
          local_8[3] = (uint)puVar3;
          local_8[2] = puVar3[2];
          puVar3[2] = (uint)local_8;
          *(uint **)(local_8[2] + 0xc) = local_8;
        }
        if (*local_8 < **(uint **)(local_14 + 0x84)) {
          *(uint **)(local_14 + 0x84) = local_8;
        }
        local_10 = fn_0011b0((uint)local_18[0x22] >> (*(byte *)((int)local_18 + 0x67) & 0x1f),
                                (uint)local_8,(uint)local_18,local_14,unaff_EDI);
        if (local_10 == 0) {
          local_18[0x22] = 0;
          lock_release(local_14 + 0x74);
          return 0;
        }
      }
    }
    lock_release(local_14 + 0x74);
  }
  return local_10;
}

/* ----------------------------------------------------------------------
 * fn_000a76   @0x000a76, 763 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_004464, fn_004dca, fn_006194, fn_006de4, fn_006f10
 * calls: fn_00057e, fn_000770, fn_0017f6, fn_002eca, fn_0033f6, fn_00351a, fn_00352c, fn_003944
 */
 /* WARNING: Unknown calling convention */
uint fn_000a76(a0)
    uint a0;
{
  byte bVar1;
  int iVar2;
  uint uVar3;
  uint extraout_EDX;
  uint *puVar4;
  uint *extraout_EDX_00;
  uint *extraout_EDX_01;
  uint *extraout_EDX_02;
  uint uVar5;
  uint *a0_00;
  uint unaff_EDI;
  uint *local_14;
  int local_10;
  uint local_c;
  uint *local_8;
  
  local_10 = 0;
  if (*(int *)(a0 + 0xc0) == 0) {
    return 0xe1;
  }
  fn_0017f6(a0);
  if ((*(ushort *)(a0 + 0x14) & 0x8000) != 0) {
    return 0;
  }
  *(undefined4 *)(a0 + 0x78) = *(undefined4 *)(a0 + 0x7c);
  local_c = fn_007da4(0xffffffff,unaff_EDI);
  if (local_c != 0) {
    return local_c;
  }
  *(undefined4 *)(a0 + 0x7c) = *(undefined4 *)(a0 + 0x78);
  fn_00789c(*(int *)(a0 + 0x70) + 0x28);
  uVar3 = fn_00057e(a0);
  if (uVar3 != 1) {
    iVar2 = *(int *)(a0 + 0x70);
    if ((*(byte *)(iVar2 + 0x42) & 1) != 0) {
      *(undefined4 *)(iVar2 + 0x4c) = *(undefined4 *)(a0 + 0x7c);
      if ((*(byte *)(a0 + 100) & 0x80) == 0) {
        local_c = fn_00351a(a0);
      }
LAB_00000c32:
      lock_release(*(int *)(a0 + 0x70) + 0x28);
      return local_c;
    }
    *(undefined4 *)(iVar2 + 0x4c) = *(undefined4 *)(a0 + 0x7c);
    local_c = fn_002eca((uint)&local_14,unaff_EDI);
    if (local_c == 0) {
      bVar1 = *(byte *)(a0 + 0x67);
      uVar5 = (*(uint *)(a0 + 0x88) + *(int *)(a0 + 0x80)) - *(int *)(a0 + 0x78) >> (bVar1 & 0x1f);
      uVar3 = (*(uint *)(a0 + 0x88) >> (bVar1 & 0x1f)) - uVar5;
      *(uint *)(a0 + 0x88) = uVar5 << (bVar1 & 0x1f);
      *(int *)(a0 + 0x84) = *(int *)(a0 + 0x84) + uVar3;
      if ((uVar5 == 0) || (local_c = fn_000770(a0), uVar3 = extraout_EDX, local_c == 0)) {
        local_14[1] = uVar3;
      }
      else if (local_c != 0xdb) goto LAB_00000c32;
      a0_00 = *(uint **)(a0 + 0x74);
      uVar3 = local_c;
      if (a0_00 == *(uint **)(a0 + 0x70)) {
        iVar2 = *(int *)(a0 + 0x13c);
        *(undefined4 *)((int)a0_00 + iVar2 + 0x30) = *(undefined4 *)(a0 + 0xc0);
        *(undefined4 *)((int)a0_00 + iVar2 + 0x34) = *(undefined4 *)(a0 + 0xc0);
      }
      else {
        local_8 = a0_00;
        if ((a0_00 + 0x1a == local_14) && (local_14[1] == 0)) {
          local_10 = 1;
          local_8 = (uint *)*a0_00;
        }
        *(uint *)(*(int *)(a0 + 0x70) + *(int *)(a0 + 0x13c) + 0x30) = local_8[4];
        *(undefined4 *)((int)local_8 + *(int *)(a0 + 0x13c) + 0x34) = *(undefined4 *)(a0 + 0xc0);
        if (((*(byte *)(a0 + 100) & 0x80) == 0) &&
           ((local_c = fn_00351a(a0), local_c != 0 ||
            (((uVar3 = 0, a0_00 != local_8 && (local_8 != *(uint **)(a0 + 0x70))) &&
             (local_c = fn_00352c((uint)local_8,unaff_EDI), uVar3 = local_c, local_c != 0))))))
        goto LAB_00000c32;
      }
      local_c = uVar3;
      local_14 = local_14 + 2;
      do {
        puVar4 = (uint *)((int)a0_00 + *(int *)(a0 + 0x13c) + 0x30);
        for (; (puVar4 != local_14 && (local_14[1] != 0)); local_14 = local_14 + 2) {
          *(uint *)(a0 + 0x88) = local_14[1] << (*(byte *)(a0 + 0x67) & 0x1f);
          *(uint *)(a0 + 0x84) = *local_14;
          local_c = fn_000770(a0);
          if (local_c != 0) goto LAB_00000c32;
          local_14[1] = 0;
          local_c = 0;
          puVar4 = extraout_EDX_00;
        }
        if (local_10 == 0) {
          fn_0033f6((uint)a0_00,unaff_EDI);
          puVar4 = extraout_EDX_01;
          if (((*(byte *)(a0 + 100) & 0x80) == 0) &&
             (local_c = fn_00352c((uint)a0_00,unaff_EDI), puVar4 = extraout_EDX_02, local_c != 0)
             ) goto LAB_00000c32;
          if (a0_00 == *(uint **)(a0 + 0x70)) {
            *(undefined1 *)(a0 + 100) = 0;
          }
          if (puVar4 != local_14) break;
          local_10 = 1;
          a0_00 = (uint *)a0_00[1];
        }
        else {
          a0_00 = (uint *)a0_00[1];
          local_c = fn_003944(*a0_00,unaff_EDI);
          if (local_c != 0) goto LAB_00000c32;
        }
        local_14 = a0_00 + 0x1a;
      } while (a0_00 != *(uint **)(a0 + 0x70));
    }
    else if (local_c != 0xd5) goto LAB_00000c32;
  }
  lock_release(*(int *)(a0 + 0x70) + 0x28);
  return 0;
}

/* ----------------------------------------------------------------------
 * fn_000d72   @0x000d72, 1085 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0005ee, fn_000770, fn_004bf8
 * calls: fn_001332, fn_0013a8, fn_002ad0, fn_0035d6, os_f_srtmem_8db8, os_f_srqmem
 */
 /* WARNING: Unknown calling convention */
uint fn_000d72(a0, a1)
    uint a0;
    uint a1;
{
  int iVar1;
  uint uVar2;
  uint uVar3;
  uint extraout_ECX;
  undefined4 extraout_ECX_00;
  uint extraout_ECX_01;
  int extraout_EDX;
  undefined4 extraout_EDX_00;
  uint unaff_EDI;
  uint in_stack_00000004;
  undefined4 *local_58;
  undefined4 local_54;
  uint *local_50;
  undefined4 local_4c;
  undefined4 local_48;
  undefined4 local_44;
  uint local_40;
  uint local_3c;
  uint local_38;
  uint local_34;
  uint local_30;
  uint local_2c;
  uint local_28;
  uint local_24;
  undefined4 local_20;
  uint local_1c;
  uint local_18;
  uint *local_14;
  uint *local_10;
  uint local_c;
  uint local_8;
  
  local_30 = in_stack_00000004;
  local_2c = ((*(int *)(in_stack_00000004 + 8) + 7U >> 3) + *(int *)(a0 + 0x13c)) - 1 >>
             (*(byte *)(a0 + 0x67) & 0x1f);
  if (0x40 < local_2c) {
    local_2c = 0x40;
  }
  local_40 = local_2c << (*(byte *)(a0 + 0x67) & 0x1f);
  local_34 = a0;
  local_28 = local_40;
  local_24 = os_f_srqmem((uint)&local_40,0,unaff_EDI);
  if (local_24 == 0) {
    local_20 = *(undefined4 *)(local_34 + 0x60);
    *(uint *)(local_34 + 0x60) = local_3c;
    uVar3 = local_34;
    local_24 = fn_002ad0(1,local_34,unaff_EDI);
    if (local_24 == 0) {
      fn_0035d6(local_3c,uVar3 & 0xffffff00,unaff_EDI);
      iVar1 = *(int *)(local_34 + 0x60);
      *(undefined4 *)(local_30 + 0x90) = *(undefined4 *)(iVar1 + 0x30);
      *(undefined4 *)(local_30 + 0x94) = *(undefined4 *)(iVar1 + 0x34);
      local_38 = 0;
      local_1c = 0;
      for (local_18 = 0; local_18 < *(uint *)(local_30 + 0x94); local_18 = local_18 + local_2c) {
        uVar3 = *(int *)(local_30 + 0x94) - local_18;
        if (uVar3 < local_2c) {
          local_2c = uVar3;
        }
        local_24 = fn_002ad0(local_2c,local_34,unaff_EDI);
        if (local_24 != 0) {
LAB_000010ed:
          fn_001332(local_30);
          goto LAB_00000e0e;
        }
        local_14 = *(uint **)(local_34 + 0x60);
        local_10 = (uint *)((local_2c << (*(byte *)(local_34 + 0x67) & 0x1f)) +
                           *(int *)(local_34 + 0x60));
        do {
          uVar2 = local_38;
          local_24 = 0;
          uVar3 = *local_14;
          local_c = uVar3;
          if (*local_14 == 0) {
            local_1c = local_1c + 0x20;
          }
          else if (uVar3 == 0xffffffff) {
            if (local_1c != 0) {
              if (local_38 < *(uint *)(local_30 + 8)) {
                local_4c = 0x10;
                local_24 = os_f_srqmem((uint)&local_4c,0,unaff_EDI);
                if (local_24 == 0) {
                  *local_50 = uVar2;
                  local_50[1] = extraout_ECX;
                  if (*(int *)(extraout_EDX + 0x84) == 0) {
                    local_50[2] = (uint)local_50;
                    local_50[3] = (uint)local_50;
                    *(uint **)(extraout_EDX + 0x88) = local_50;
                    *(uint **)(extraout_EDX + 0x84) = local_50;
                  }
                  else {
                    local_50[3] = *(uint *)(extraout_EDX + 0x84);
                    local_50[2] = *(uint *)(*(int *)(extraout_EDX + 0x84) + 8);
                    *(uint **)(*(int *)(*(int *)(extraout_EDX + 0x84) + 8) + 0xc) = local_50;
                    *(uint **)(*(int *)(extraout_EDX + 0x84) + 8) = local_50;
                  }
                  local_24 = 0;
                }
              }
              else {
                local_24 = 0;
              }
              if (local_24 != 0) goto LAB_000010ed;
              local_38 = local_38 + local_1c;
              local_1c = 0;
            }
            local_38 = local_38 + 0x20;
          }
          else {
            (*(uint8 *)((uint8 *)&local_44 + 3)) = (undefined1)(uVar3 >> 0x18);
            (*(uint8 *)((uint8 *)&local_44 + 2)) = (undefined1)(uVar3 >> 0x10);
            (*(uint8 *)((uint8 *)&local_44 + 1)) = (undefined1)(uVar3 >> 8);
            (*(uint8 *)((uint8 *)&local_44 + 0)) = (undefined1)uVar3;
            local_48 = CONCAT13((undefined1)local_44,
                                CONCAT12((*(uint8 *)((uint8 *)&local_44 + 1)),CONCAT11((*(uint8 *)((uint8 *)&local_44 + 2)),(*(uint8 *)((uint8 *)&local_44 + 3)))));
            local_c = local_48;
            local_8 = 0x80000000;
            local_44 = uVar3;
            do {
              uVar3 = local_30;
              local_24 = 0;
              if ((local_8 & local_c) == 0) {
                local_1c = local_1c + 1;
              }
              else {
                if (local_1c != 0) {
                  if (local_38 < *(uint *)(local_30 + 8)) {
                    local_54 = 0x10;
                    local_24 = os_f_srqmem((uint)&local_54,0,unaff_EDI);
                    if (local_24 == 0) {
                      *local_58 = extraout_EDX_00;
                      local_58[1] = extraout_ECX_00;
                      if (*(int *)(uVar3 + 0x84) == 0) {
                        local_58[2] = local_58;
                        local_58[3] = local_58;
                        *(undefined4 **)(uVar3 + 0x88) = local_58;
                        *(undefined4 **)(uVar3 + 0x84) = local_58;
                      }
                      else {
                        local_58[3] = *(undefined4 *)(uVar3 + 0x84);
                        local_58[2] = *(undefined4 *)(*(int *)(uVar3 + 0x84) + 8);
                        *(undefined4 **)(*(int *)(*(int *)(uVar3 + 0x84) + 8) + 0xc) = local_58;
                        *(undefined4 **)(*(int *)(uVar3 + 0x84) + 8) = local_58;
                      }
                      local_24 = 0;
                    }
                  }
                  else {
                    local_24 = 0;
                  }
                  if (local_24 != 0) goto LAB_000010ed;
                  local_38 = local_38 + local_1c;
                  local_1c = 0;
                }
                local_38 = local_38 + 1;
              }
              local_8 = local_8 >> 1;
            } while (local_8 != 0);
          }
          local_24 = 0;
          local_14 = local_14 + 1;
        } while (local_14 < local_10);
      }
      os_f_srtmem_8db8(local_28,unaff_EDI);
      *(undefined4 *)(local_34 + 0x60) = local_20;
      if (((local_1c == 0) || (uVar3 = fn_0013a8(local_30,local_1c,unaff_EDI), uVar3 == 0)) &&
         ((*(int *)(local_30 + 0x84) != 0 ||
          (uVar3 = fn_0013a8(local_30,0,unaff_EDI), uVar3 == 0)))) {
        local_24 = 0;
      }
      else {
        fn_001332(local_30);
        local_24 = extraout_ECX_01;
      }
    }
    else {
LAB_00000e0e:
      os_f_srtmem_8db8(local_28,unaff_EDI);
      *(undefined4 *)(local_34 + 0x60) = local_20;
    }
  }
  return local_24;
}

/* ----------------------------------------------------------------------
 * fn_0011b0   @0x0011b0, 386 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0005ee, fn_000770
 * calls: fn_001468, fn_0014e6, fn_001a00, fn_001b52, lock_release, os_f_srtmem_8db8, os_f_srqmem, fn_0090f8
 */
 /* WARNING: Unknown calling convention */
uint fn_0011b0(a0, a1, a2, a3, a4)
    uint a0;
    uint a1;
    uint a2;
    uint a3;
    uint a4;
{
  byte bVar1;
  uint uVar2;
  uint *puVar3;
  uint unaff_EDI;
  uint in_stack_00000004;
  int local_28;
  uint local_24;
  uint local_20;
  int local_1c;
  uint local_18;
  uint local_14;
  uint local_10;
  uint local_c;
  uint local_8;
  
  local_18 = a2;
  local_14 = a3;
  local_8 = (*(int *)(a2 + 0x13c) + -1) * 8 | 7;
  local_c = ~local_8 & in_stack_00000004;
  local_8 = (in_stack_00000004 + a0) - 1 | local_8;
  bVar1 = *(byte *)(a2 + 0x67);
  local_10 = (uint)bVar1;
  local_1c = (local_8 - local_c >> (bVar1 + 3 & 0x1f)) + 1;
  if (local_1c == 1) {
    uVar2 = fn_001a00((uint)&local_24,unaff_EDI);
    if (uVar2 != 0) {
      return uVar2;
    }
    local_20 = *(uint *)(local_24 + 0x28);
  }
  else {
    local_28 = local_1c << (bVar1 & 0x1f);
    uVar2 = os_f_srqmem((uint)&local_28,0,unaff_EDI);
    if (uVar2 != 0) {
      return uVar2;
    }
  }
  fn_0090f8(0xffffffff);
  puVar3 = *(uint **)(local_14 + 0x84);
  do {
    uVar2 = *puVar3;
    if (local_c < uVar2 + puVar3[1]) {
      if (local_8 < uVar2) break;
      if (uVar2 < local_c) {
        uVar2 = local_c;
      }
      fn_0014e6(uVar2 - local_c,local_20,unaff_EDI);
    }
    puVar3 = (uint *)puVar3[3];
  } while (puVar3 != *(uint **)(local_14 + 0x84));
  uVar2 = fn_001468(local_c >> ((char)local_10 + 3U & 0x1f),local_20,local_18,unaff_EDI);
  if (uVar2 == 0) {
    if (local_1c == 1) {
      lock_release(local_24 + 0x10);
      uVar2 = fn_001b52(local_24,unaff_EDI);
    }
    else {
      uVar2 = os_f_srtmem_8db8(local_1c << ((byte)local_10 & 0x1f),unaff_EDI);
    }
  }
  return uVar2;
}
