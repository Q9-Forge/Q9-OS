/*
 * rbf_02.c - rbf part 2
 *
 * Original address range: 0x001332 - 0x002724 of the V4.9 kernel module.
 * Functions 0x1332 to 0x2180 (support routines)
 * Generated from the Ghidra export; see tools/gen_src.py. Hand edits belong in hand/*.inc.
 */
#include "kernel.h"

/* ----------------------------------------------------------------------
 * fn_001332   @0x001332, 117 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_000d72, fn_0029e4, fn_005b0e, rbf_detach
 */
 /* WARNING: Unknown calling convention */
uint fn_001332(a0)
    uint a0;
{
  code *pcVar1;
  int extraout_ECX;
  int iVar2;
  int extraout_EDX;
  int iVar3;
  
  iVar2 = *(int *)(a0 + 0x84);
  *(undefined4 *)(a0 + 0x88) = 0;
  *(undefined4 *)(a0 + 0x84) = 0;
  iVar3 = *(int *)(iVar2 + 0xc);
  while (iVar3 != iVar2) {
    pcVar1 = (code *)swi(0xff);
    (*pcVar1)();
    iVar2 = extraout_ECX;
    iVar3 = extraout_EDX;
  }
  pcVar1 = (code *)swi(0xff);
  (*pcVar1)();
  return 0;
}

/* ----------------------------------------------------------------------
 * fn_0013a8   @0x0013a8, 192 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_000d72
 * calls: os_f_srqmem
 */
 /* WARNING: Unknown calling convention */
uint fn_0013a8(a0, a1, a2)
    uint a0;
    uint a1;
    uint a2;
{
  uint uVar1;
  undefined4 extraout_ECX;
  int extraout_EDX;
  uint unaff_ESI;
  uint in_stack_00000004;
  undefined4 local_c;
  undefined4 *local_8;
  
  if (in_stack_00000004 < *(uint *)(a0 + 8)) {
    if (*(uint *)(a0 + 8) < a1 + in_stack_00000004) {
      a1 = *(int *)(a0 + 8) - in_stack_00000004;
    }
    local_c = 0x10;
    uVar1 = os_f_srqmem((uint)&local_c,0,unaff_ESI);
    if (uVar1 == 0) {
      *local_8 = extraout_ECX;
      local_8[1] = a1;
      if (*(int *)(extraout_EDX + 0x84) == 0) {
        local_8[2] = local_8;
        local_8[3] = local_8;
        *(undefined4 **)(extraout_EDX + 0x88) = local_8;
        *(undefined4 **)(extraout_EDX + 0x84) = local_8;
      }
      else {
        local_8[3] = *(undefined4 *)(extraout_EDX + 0x84);
        local_8[2] = *(undefined4 *)(*(int *)(extraout_EDX + 0x84) + 8);
        *(undefined4 **)(*(int *)(*(int *)(extraout_EDX + 0x84) + 8) + 0xc) = local_8;
        *(undefined4 **)(*(int *)(extraout_EDX + 0x84) + 8) = local_8;
      }
      uVar1 = 0;
    }
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_001468   @0x001468, 125 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0011b0
 * calls: fn_002bdc
 */
 /* WARNING: Unknown calling convention */
uint fn_001468(a0, a1, a2, a3)
    uint a0;
    uint a1;
    uint a2;
    uint a3;
{
  undefined4 uVar1;
  uint uVar2;
  int extraout_EDX;
  uint uVar3;
  uint unaff_EDI;
  uint in_stack_00000004;
  
  uVar1 = *(undefined4 *)(a2 + 0x60);
  *(uint *)(a2 + 0x60) = a1;
  uVar3 = in_stack_00000004;
  while( true ) {
    uVar2 = uVar3;
    if (0xff < uVar3) {
      uVar2 = 0xff;
    }
    uVar2 = fn_002bdc(uVar2,a2,unaff_EDI);
    if (uVar2 != 0) break;
    uVar3 = uVar3 - extraout_EDX;
    if (uVar3 == 0) {
      *(undefined4 *)(a2 + 0x60) = uVar1;
      return 0;
    }
    *(int *)(a2 + 0x60) = *(int *)(a2 + 0x60) + (extraout_EDX << (*(byte *)(a2 + 0x67) & 0x1f));
  }
  *(undefined4 *)(a2 + 0x60) = uVar1;
  return uVar2;
}

/* ----------------------------------------------------------------------
 * fn_0014e6   @0x0014e6, 172 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0011b0
 * calls: fn_0090f8
 */
 /* WARNING: Unknown calling convention */
uint fn_0014e6(a0, a1, a2)
    uint a0;
    uint a1;
    uint a2;
{
  int iVar1;
  short sVar2;
  uint uVar3;
  int extraout_ECX;
  uint uVar4;
  int extraout_EDX;
  byte *pbVar5;
  byte *pbVar6;
  byte *pbVar7;
  uint in_stack_00000004;
  
  uVar3 = 8 - (a0 & 7);
  pbVar5 = (byte *)(a1 + (a0 >> 3));
  if (in_stack_00000004 < uVar3) {
    uVar4 = 0;
    iVar1 = -(1 << ((byte)uVar3 - (char)in_stack_00000004 & 0x1f));
  }
  else {
    uVar4 = in_stack_00000004 - uVar3;
    iVar1 = -1;
  }
  pbVar7 = (byte *)((1 << ((byte)uVar3 & 0x1f)) + iVar1 ^ 0xffffffff);
  pbVar6 = pbVar5 + 1;
  *pbVar5 = *pbVar5 & (byte)pbVar7;
  if (uVar4 != 0) {
    while (8 < uVar4) {
      if ((((uint)pbVar6 & 3) == 0) && (0x20 < uVar4)) {
        pbVar7 = (byte *)fn_0090f8(0);
        uVar4 = extraout_EDX + extraout_ECX * -8;
        pbVar6 = pbVar6 + extraout_ECX;
      }
      else {
        uVar4 = uVar4 - 8;
        *pbVar6 = 0;
        pbVar7 = pbVar6;
        pbVar6 = pbVar6 + 1;
      }
    }
    if (uVar4 != 0) {
      sVar2 = 0xff >> ((byte)uVar4 & 0x1f);
      pbVar7 = (byte *)CONCAT22((short)((uint)pbVar7 >> 0x10),sVar2);
      *pbVar6 = (byte)sVar2;
    }
  }
  return (uint)pbVar7;
}

/* ----------------------------------------------------------------------
 * fn_001592   @0x001592, 50 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_002c74
 * calls: fn_0015c4, fn_001740
 */
 /* WARNING: Unknown calling convention */
uint fn_001592(a0)
    uint a0;
{
  uint in_EAX;
  uint uVar1;
  uint in_ECX;
  int extraout_ECX;
  
  uVar1 = fn_0015c4(in_EAX,in_ECX);
  if (uVar1 == 0) {
    if (((*(uint *)(*(uint *)(extraout_ECX + 0x6c) + 8) >> 1 & 1) == 0) &&
       (uVar1 = fn_001740(*(uint *)(extraout_ECX + 0x6c),in_ECX), uVar1 != 0)) {
      return uVar1;
    }
    uVar1 = 0;
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_0015c4   @0x0015c4, 379 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0000ea, fn_001592, fn_0017f6, fn_003f8c, fn_004518, fn_007518
 * calls: fn_001796, fn_0017f6, fn_001a00, fn_00789c, lock_release, os_f_srqmem
 */
 /* WARNING: Unknown calling convention */
uint fn_0015c4(a0, a1)
    uint a0;
    uint a1;
{
  int iVar1;
  uint uVar2;
  uint *puVar3;
  int extraout_ECX;
  uint *extraout_ECX_00;
  uint extraout_ECX_01;
  int extraout_EDX;
  uint unaff_EDI;
  uint in_stack_00000004;
  int local_14;
  uint *local_10;
  uint local_c;
  uint local_8;
  
  local_8 = in_stack_00000004;
  local_c = a0;
  if (*(int *)(in_stack_00000004 + 0xc0) == 0) {
    if (*(int *)(in_stack_00000004 + 0x6c) == 0) {
      local_14 = *(int *)(in_stack_00000004 + 0x13c) + 0x2c;
      uVar2 = os_f_srqmem((uint)&local_14,0,unaff_EDI);
      if (uVar2 != 0) {
        return uVar2;
      }
      local_10[9] = 0;
      local_10[2] = 0;
      local_10[10] = (uint)(local_10 + 0xb);
    }
    else {
      local_10 = *(uint **)(in_stack_00000004 + 0x6c);
      if ((a0 != *local_10) && ((local_10[2] & 1) != 0)) {
        uVar2 = fn_001796((uint)local_10,unaff_EDI);
        if (uVar2 != 0) {
          return uVar2;
        }
        local_10[2] = 0;
      }
    }
    local_10[1] = 1;
    *local_10 = local_c;
  }
  else {
    if ((*(int *)(in_stack_00000004 + 0x6c) != 0) &&
       (uVar2 = fn_0017f6(in_stack_00000004), uVar2 != 0)) {
      return uVar2;
    }
    iVar1 = *(int *)(local_8 + 0x70);
    uVar2 = fn_00789c(iVar1 + 0x18);
    if (uVar2 != 0) {
      return uVar2;
    }
    for (puVar3 = *(uint **)(iVar1 + 0x14); (puVar3 != (uint *)0x0 && (*puVar3 < local_c));
        puVar3 = (uint *)puVar3[9]) {
    }
    if ((puVar3 == (uint *)0x0) || (local_c != *puVar3)) {
      uVar2 = fn_001a00((uint)&local_10,unaff_EDI);
      if (uVar2 != 0) {
        return uVar2;
      }
      if (extraout_EDX == 0) {
        *(uint **)(*(int *)(local_8 + 0x70) + 0x14) = local_10;
      }
      else {
        *(uint **)(extraout_EDX + 0x24) = local_10;
      }
      local_10[9] = extraout_ECX_01;
      *local_10 = local_c;
      local_10[1] = 1;
      local_10[2] = 0;
      uVar2 = lock_release(iVar1 + 0x18);
    }
    else {
      puVar3[1] = puVar3[1] + 1;
      uVar2 = lock_release(iVar1 + 0x18);
      if (uVar2 != 0) {
        return uVar2;
      }
      uVar2 = fn_00789c(extraout_ECX + 0x10);
      local_10 = extraout_ECX_00;
    }
    if (uVar2 != 0) {
      return uVar2;
    }
  }
  *(uint **)(local_8 + 0x6c) = local_10;
  return 0;
}

/* ----------------------------------------------------------------------
 * fn_001740   @0x001740, 86 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_001592, fn_002c74, fn_003f8c
 * calls: fn_001d1a, fn_002b5e, fn_002b84
 */
 /* WARNING: Unknown calling convention */
uint fn_001740(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  uint a1_00;
  uint a2;
  uint in_EDX;
  int extraout_EDX;
  int extraout_EDX_00;
  int extraout_EDX_01;
  int iVar2;
  uint in_stack_00000004;
  
  uVar1 = fn_002b5e(in_stack_00000004);
  if (uVar1 == 0) {
    *(undefined4 *)(a1_00 + 0x60) = *(undefined4 *)(extraout_EDX + 0x28);
    uVar1 = fn_001d1a(1,a1_00,in_EDX);
    iVar2 = extraout_EDX_00;
    if ((uVar1 == 0) && (uVar1 = fn_002b84(1,1,a2,0,in_EDX), iVar2 = extraout_EDX_01, uVar1 != 0)
       ) {
      return uVar1;
    }
    *(uint *)(iVar2 + 8) = *(uint *)(iVar2 + 8) | 2;
    uVar1 = 0;
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_001796   @0x001796, 96 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0015c4, fn_0017f6, fn_001bc8
 * calls: fn_002724, fn_002b5e, fn_002b84
 */
 /* WARNING: Unknown calling convention */
uint fn_001796(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  uint extraout_ECX;
  uint extraout_ECX_00;
  uint a2;
  uint in_EDX;
  uint *extraout_EDX;
  uint *extraout_EDX_00;
  uint *puVar2;
  int extraout_EDX_01;
  uint in_stack_00000004;
  
  uVar1 = fn_002b5e(in_stack_00000004);
  if (uVar1 == 0) {
    uVar1 = *(uint *)(*(int *)(*(int *)(extraout_ECX + 4) + 0x20) + 0xac);
    a2 = extraout_ECX;
    puVar2 = extraout_EDX;
    if (uVar1 != 0) {
      fn_002724(*extraout_EDX,uVar1,2,in_EDX);
      a2 = extraout_ECX_00;
      puVar2 = extraout_EDX_00;
    }
    *(uint *)(a2 + 0x60) = puVar2[10];
    uVar1 = fn_002b84(1,2,a2,0,in_EDX);
    if (uVar1 == 0) {
      *(uint *)(extraout_EDX_01 + 8) = *(uint *)(extraout_EDX_01 + 8) & 0xfffffffe;
      uVar1 = 0;
    }
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_0017f6   @0x0017f6, 521 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0000ea, fn_000a76, fn_0015c4, fn_003a26, fn_003f8c, rbf_write, fn_004518, fn_00517a
 * calls: fn_0015c4, fn_001796, fn_001b52, fn_002988, fn_00789c, lock_release, os_f_srtmem_8db8
 */
 /* WARNING: Unknown calling convention */
uint fn_0017f6(a0)
    uint a0;
{
  int *piVar1;
  uint uVar2;
  uint *puVar3;
  uint uVar4;
  uint a0_00;
  uint extraout_ECX;
  uint a0_01;
  uint a0_02;
  uint extraout_EDX;
  uint uVar5;
  uint uVar6;
  uint unaff_EDI;
  
  if ((*(int *)(a0 + 0x70) == 0) && (*(int *)(a0 + 0xc0) != 0)) {
    return 0;
  }
  a0_00 = *(int *)(a0 + 0x70) + 0x18;
  if (*(int *)(a0 + 0xc0) == 0) {
    if (*(int *)(a0 + 0x6c) == 0) {
      return 0;
    }
  }
  else {
    uVar2 = fn_00789c(a0_00);
    if (uVar2 != 0) {
      return uVar2;
    }
    if (*(int *)(a0 + 0x6c) == 0) {
      uVar2 = fn_002988(a0);
      if (uVar2 == 0) {
        uVar2 = ((uint)(*(int *)(a0 + 0x78) - *(int *)(a0 + 0x80)) >> (*(byte *)(a0 + 0x67) & 0x1f))
                + *(int *)(a0 + 0x84);
        for (puVar3 = *(uint **)(*(int *)(a0 + 0x70) + 0x14);
            (puVar3 != (uint *)0x0 && (*puVar3 < uVar2)); puVar3 = (uint *)puVar3[9]) {
        }
        if ((puVar3 != (uint *)0x0) && (uVar2 == *puVar3)) {
          lock_release(a0_00);
          uVar2 = fn_0015c4(a0_02,unaff_EDI);
          if (uVar2 != 0) {
            return uVar2;
          }
          fn_00789c(a0_00);
          goto LAB_000018f7;
        }
      }
      lock_release(a0_00);
      return 0;
    }
  }
LAB_000018f7:
  uVar2 = 0;
  piVar1 = (int *)(*(int *)(a0 + 0x6c) + 4);
  *piVar1 = *piVar1 + -1;
  if (*piVar1 == 0) {
    if (*(int *)(a0 + 0xc0) != 0) {
      lock_release(*(int *)(a0 + 0x6c) + 0x10);
    }
    a0_01 = *(uint *)(a0 + 0x6c);
    uVar5 = a0;
    if (((*(uint *)(a0_01 + 8) & 1) == 0) ||
       (uVar2 = fn_001796(a0_01,unaff_EDI), a0_01 = extraout_ECX, uVar5 = extraout_EDX,
       uVar2 == 0)) {
      if (*(int *)(uVar5 + 0xc0) == 0) {
        uVar2 = os_f_srtmem_8db8(*(int *)(uVar5 + 0x13c) + 0x2c,unaff_EDI);
      }
      else {
        uVar2 = *(uint *)(*(int *)(uVar5 + 0x70) + 0x14);
        uVar6 = uVar2;
        while (uVar4 = uVar2, uVar4 != a0_01) {
          uVar6 = uVar4;
          uVar2 = *(uint *)(uVar4 + 0x24);
        }
        if (uVar6 == a0_01) {
          *(undefined4 *)(*(int *)(uVar5 + 0x70) + 0x14) = *(undefined4 *)(a0_01 + 0x24);
        }
        else {
          *(undefined4 *)(uVar6 + 0x24) = *(undefined4 *)(a0_01 + 0x24);
        }
        uVar2 = fn_001b52(a0_01,unaff_EDI);
      }
    }
  }
  else {
    if ((*(uint *)(*(uint *)(a0 + 0x6c) + 8) & 3) == 3) {
      uVar2 = fn_001796(*(uint *)(a0 + 0x6c),unaff_EDI);
    }
    lock_release(*(int *)(a0 + 0x6c) + 0x10);
  }
  *(undefined4 *)(a0 + 0x6c) = 0;
  if (*(int *)(a0 + 0xc0) != 0) {
    lock_release(a0_00);
  }
  return uVar2;
}

/* ----------------------------------------------------------------------
 * fn_001a00   @0x001a00, 241 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0011b0, fn_0015c4, fn_0029e4, fn_006194
 * calls: fn_00789c, lock_release, os_f_srqmem
 */
 /* WARNING: Unknown calling convention */
uint fn_001a00(a0, a1)
    uint a0;
    uint a1;
{
  int iVar1;
  int iVar2;
  int iVar3;
  int extraout_ECX;
  int extraout_EDX;
  int extraout_EDX_00;
  int iVar4;
  uint unaff_EDI;
  int in_stack_00000004;
  int local_10;
  int *local_c;
  uint local_8;
  
  local_8 = 0;
  iVar1 = *(int *)(*(int *)(in_stack_00000004 + 4) + 0x20);
  local_c = (int *)a0;
  fn_00789c(iVar1 + 0xe0);
  iVar2 = *(int *)(iVar1 + 0xf0);
  iVar4 = 0;
  while ((iVar3 = iVar2, iVar3 != 0 && (*(int *)(iVar3 + 0xc) != *(int *)(extraout_EDX + 0x13c)))) {
    iVar4 = iVar3;
    iVar2 = *(int *)(iVar3 + 0x24);
  }
  if (iVar3 == 0) {
    local_10 = *(int *)(extraout_EDX + 0x13c) + 0x2c;
    local_8 = os_f_srqmem((uint)&local_10,0,unaff_EDI);
    if (local_8 == 0) {
      iVar2 = *local_c;
      *(undefined4 *)(iVar2 + 0xc) = *(undefined4 *)(extraout_EDX_00 + 0x13c);
      *(int *)(iVar2 + 0x28) = iVar2 + 0x2c;
      *(int *)(iVar2 + 0x10) = iVar2 + 0x10;
      *(undefined4 *)(iVar2 + 0x14) = 0;
      *(int *)(iVar2 + 0x1c) = iVar2 + -0x3c;
      *(int *)(iVar2 + 0x18) = iVar2 + -0x3c;
      fn_00789c(iVar2 + 0x10);
    }
  }
  else {
    *(int *)(iVar3 + 0x10) = iVar3 + 0x10;
    *(undefined4 *)(iVar3 + 0x14) = 0;
    *(int *)(iVar3 + 0x1c) = iVar3 + -0x3c;
    *(int *)(iVar3 + 0x18) = iVar3 + -0x3c;
    fn_00789c(iVar3 + 0x10);
    if (iVar4 == 0) {
      *(undefined4 *)(iVar1 + 0xf0) = *(undefined4 *)(extraout_ECX + 0x24);
    }
    else {
      *(undefined4 *)(iVar4 + 0x24) = *(undefined4 *)(extraout_ECX + 0x24);
    }
    *local_c = extraout_ECX;
  }
  lock_release(iVar1 + 0xe0);
  return local_8;
}

/* ----------------------------------------------------------------------
 * fn_001af2   @0x001af2, 96 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_003a26, rbf_write, fn_006194, fn_007da4
 * calls: fn_00789c, lock_release
 */
 /* WARNING: Unknown calling convention */
uint fn_001af2(a0)
    uint a0;
{
  int *piVar1;
  uint uVar2;
  int extraout_ECX;
  int extraout_ECX_00;
  int iVar3;
  uint extraout_EDX;
  uint extraout_EDX_00;
  uint a0_00;
  
  if ((*(int *)(a0 + 0x6c) == 0) || (*(int *)(a0 + 0xc0) == 0)) {
    uVar2 = 0;
  }
  else {
    uVar2 = fn_00789c(*(int *)(a0 + 0x70) + 0x18);
    if (uVar2 == 0) {
      iVar3 = *(int *)(extraout_ECX + 0x6c);
      piVar1 = (int *)(iVar3 + 4);
      *piVar1 = *piVar1 + -1;
      if (*(int *)(extraout_ECX + 0xc0) == 0) {
        uVar2 = 0;
        iVar3 = extraout_ECX;
        a0_00 = extraout_EDX;
      }
      else {
        uVar2 = lock_release(iVar3 + 0x10);
        iVar3 = extraout_ECX_00;
        a0_00 = extraout_EDX_00;
      }
      if (uVar2 == 0) {
        *(undefined4 *)(iVar3 + 0x6c) = 0;
      }
      lock_release(a0_00);
    }
  }
  return uVar2;
}

/* ----------------------------------------------------------------------
 * fn_001b52   @0x001b52, 46 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0011b0, fn_0017f6, fn_001bc8, fn_0029e4, fn_006194
 */
 /* WARNING: Unknown calling convention */
uint fn_001b52(a0, a1)
    uint a0;
    uint a1;
{
  int iVar1;
  int in_stack_00000004;
  
  iVar1 = *(int *)(*(int *)(in_stack_00000004 + 4) + 0x20);
  *(undefined4 *)(a0 + 0x24) = *(undefined4 *)(iVar1 + 0xf0);
  *(uint *)(iVar1 + 0xf0) = a0;
  return 0;
}

/* ----------------------------------------------------------------------
 * fn_001b80   @0x001b80, 71 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_003f8c, fn_004518
 */
 /* WARNING: Unknown calling convention */
uint fn_001b80(a0, a1, a2)
    uint a0;
    uint a1;
    uint a2;
{
  uint *puVar1;
  int *in_stack_00000004;
  
  if (*(int *)(a1 + 0xc0) != 0) {
    for (puVar1 = *(uint **)(*(int *)(a1 + 0x70) + 0x14); puVar1 != (uint *)0x0;
        puVar1 = (uint *)puVar1[9]) {
      if ((a0 <= *puVar1) && (*puVar1 <= a0 + *in_stack_00000004)) {
        *in_stack_00000004 = *puVar1 - a0;
        return 0;
      }
    }
  }
  return 0;
}

/* ----------------------------------------------------------------------
 * fn_001bc8   @0x001bc8, 146 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0079d2
 * calls: fn_001796, fn_001b52, lock_release, os_f_srtmem_8db8
 */
 /* WARNING: Unknown calling convention */
uint fn_001bc8(a0)
    uint a0;
{
  uint uVar1;
  uint extraout_ECX;
  uint a0_00;
  uint extraout_ECX_00;
  uint in_EDX;
  int extraout_EDX;
  uint extraout_EDX_00;
  uint extraout_EDX_01;
  
  if (*(int *)(a0 + 0xc0) == 0) {
    if (*(int *)(a0 + 0x6c) == 0) {
      uVar1 = 0;
    }
    else {
      uVar1 = os_f_srtmem_8db8(*(int *)(a0 + 0x13c) + 0x2c,in_EDX);
      *(undefined4 *)(extraout_EDX + 0x6c) = 0;
    }
  }
  else {
    if (*(int *)(a0 + 0x6c) != 0) {
      lock_release(*(int *)(a0 + 0x6c) + 0x10);
      a0 = extraout_EDX_00;
    }
    a0_00 = *(uint *)(*(int *)(a0 + 0x70) + 0x14);
    do {
      if (a0_00 == 0) {
        *(undefined4 *)(a0 + 0x6c) = 0;
        return 0;
      }
      if ((((*(uint *)(a0_00 + 8) & 1) != 0) &&
          (uVar1 = fn_001796(a0_00,in_EDX), a0_00 = extraout_ECX, uVar1 != 0)) && (uVar1 != 0xf2)
         ) {
        return uVar1;
      }
      uVar1 = fn_001b52(a0_00,in_EDX);
      a0_00 = extraout_ECX_00;
      a0 = extraout_EDX_01;
    } while (uVar1 == 0);
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_001c5a   @0x001c5a, 191 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_001d1a
 * calls: fn_0000c0
 */
 /* WARNING: Unknown calling convention */
uint fn_001c5a(a0, a1, a2, a3)
    uint a0;
    uint a1;
    uint a2;
    uint a3;
{
  undefined4 *puVar1;
  uint uVar2;
  int *extraout_ECX;
  undefined4 *puVar3;
  int in_stack_00000004;
  
  puVar3 = *(undefined4 **)(in_stack_00000004 + (a0 & 0xff) * 8);
  puVar1 = (undefined4 *)puVar3[1];
  while( true ) {
    if (puVar3 == puVar1) {
      return 0;
    }
    *(int *)a1 = *(int *)a1 + 1;
    if (a0 == puVar3[4]) break;
    puVar3 = (undefined4 *)*puVar3;
  }
  uVar2 = fn_0000c0(puVar3[5]);
  if (uVar2 != extraout_ECX[6]) {
    *(int *)(in_stack_00000004 + 0x85c) = *(int *)(in_stack_00000004 + 0x85c) + 1;
    extraout_ECX[4] = 0;
    *(int *)(extraout_ECX[3] + 8) = extraout_ECX[2];
    *(int *)(extraout_ECX[2] + 0xc) = extraout_ECX[3];
    *(int *)extraout_ECX[1] = *extraout_ECX;
    *(int *)(*extraout_ECX + 4) = extraout_ECX[1];
    extraout_ECX[2] = *(int *)(in_stack_00000004 + 0x81c);
    *(int **)(in_stack_00000004 + 0x81c) = extraout_ECX;
    return 0;
  }
  return (uint)extraout_ECX;
}

/* ----------------------------------------------------------------------
 * fn_001d1a   @0x001d1a, 703 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_001740, fn_002ad0, fn_003f8c
 * calls: fn_0000c0, fn_001c5a, fn_001fda, fn_00789c, lock_release
 */
 /* WARNING: Unknown calling convention */
uint fn_001d1a(a0, a1, a2)
    uint a0;
    uint a1;
    uint a2;
{
  int *piVar1;
  uint uVar2;
  int *extraout_ECX;
  undefined4 *puVar3;
  short sVar4;
  uint unaff_EDI;
  uint in_stack_00000004;
  int local_38;
  uint local_34;
  uint local_30;
  uint local_2c;
  int *local_28;
  uint local_24;
  uint local_20;
  uint local_1c;
  uint local_18;
  int local_14;
  uint local_10;
  int *local_c;
  uint local_8;
  
  local_20 = in_stack_00000004;
  local_1c = a1;
  local_18 = 0;
  local_38 = 0;
  local_14 = *(int *)(*(int *)(a1 + 4) + 0x20);
  local_24 = a0;
  uVar2 = fn_00789c(local_14 + 0xb0);
  if (uVar2 != 0) {
    return 0;
  }
  local_10 = *(uint *)(local_14 + 0xac);
  if (local_10 == 0) goto LAB_00001fc4;
  if (*(int *)(local_1c + 0x13c) != *(int *)(local_10 + 0x80c)) {
    return 0;
  }
  if (local_20 == 0) {
    *(undefined4 *)(local_10 + 0x804) = *(undefined4 *)(local_14 + 4);
  }
  else if (local_24 < 3) {
    local_c = (int *)0x0;
    local_8 = fn_001c5a(local_20,(uint)&local_38,local_1c,unaff_EDI);
    if (local_8 == 0) {
LAB_00001f40:
      *(int *)(local_10 + 0x864) = *(int *)(local_10 + 0x864) + local_24;
      *(int *)(local_10 + 0x86c) = *(int *)(local_10 + 0x86c) + local_38;
    }
    else {
      if (1 < local_24) {
        local_30 = local_20 + 1;
        local_34 = local_10;
        puVar3 = *(undefined4 **)(local_10 + (local_30 & 0xff) * 8);
        piVar1 = puVar3 + 1;
        for (; puVar3 != (undefined4 *)*piVar1; puVar3 = (undefined4 *)*puVar3) {
          local_38 = local_38 + 1;
          if (local_30 == puVar3[4]) {
            uVar2 = fn_0000c0(puVar3[5]);
            local_28 = extraout_ECX;
            if (uVar2 == extraout_ECX[6]) goto LAB_00001e8c;
            *(int *)(local_34 + 0x85c) = *(int *)(local_34 + 0x85c) + 1;
            local_2c = local_34;
            extraout_ECX[4] = 0;
            *(int *)(extraout_ECX[3] + 8) = extraout_ECX[2];
            *(int *)(extraout_ECX[2] + 0xc) = extraout_ECX[3];
            *(int *)extraout_ECX[1] = *extraout_ECX;
            *(int *)(*extraout_ECX + 4) = extraout_ECX[1];
            extraout_ECX[2] = *(int *)(local_34 + 0x81c);
            *(int **)(local_34 + 0x81c) = extraout_ECX;
            break;
          }
        }
        local_28 = (int *)0x0;
LAB_00001e8c:
        local_c = local_28;
        if (local_28 == (int *)0x0) goto LAB_00001f40;
      }
      if ((*(byte *)(local_1c + 0x6a) & 2) == 0) {
        sVar4 = 0;
      }
      else {
        sVar4 = 2;
      }
      fn_001fda(*(uint *)(local_1c + 0x60),local_10,(int)sVar4,local_20,
                   *(uint *)(local_1c + 0x13c),unaff_EDI);
      if (local_24 == 2) {
        fn_001fda(*(uint *)(local_1c + 0x13c) + *(int *)(local_1c + 0x60),local_10,(int)sVar4,
                     local_20 + 1,*(uint *)(local_1c + 0x13c),unaff_EDI);
      }
      *(int *)(local_10 + 0x868) = *(int *)(local_10 + 0x868) + local_38;
      if (sVar4 == 0) {
        *(int *)(local_10 + 0x828) = *(int *)(local_10 + 0x828) + local_24;
      }
      else {
        *(int *)(local_10 + 0x850) = *(int *)(local_10 + 0x850) + local_24;
      }
      local_18 = 1;
    }
  }
  if ((*(byte *)(local_1c + 0x6a) & 2) == 0) {
    *(int *)(local_10 + 0x820) = *(int *)(local_10 + 0x820) + 1;
    *(int *)(local_10 + 0x824) = *(int *)(local_10 + 0x824) + local_24;
  }
  else {
    *(int *)(local_10 + 0x848) = *(int *)(local_10 + 0x848) + 1;
    *(int *)(local_10 + 0x84c) = *(int *)(local_10 + 0x84c) + local_24;
  }
  if (2 < local_24) {
    *(int *)(local_10 + 0x830) = *(int *)(local_10 + 0x830) + 1;
  }
  if (local_20 == 0) {
    *(int *)(local_10 + 0x834) = *(int *)(local_10 + 0x834) + 1;
  }
LAB_00001fc4:
  lock_release(local_14 + 0xb0);
  return local_18;
}

/* ----------------------------------------------------------------------
 * fn_001fda   @0x001fda, 422 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_001d1a
 * calls: fn_00909f, fn_0090b3, fn_0090c8, fn_0090dc
 */
 /* WARNING: Unknown calling convention */
uint fn_001fda(a0, a1, a2, a3, a4, a5)
    uint a0;
    uint a1;
    uint a2;
    uint a3;
    uint a4;
    uint a5;
{
  int *piVar1;
  undefined2 *a0_00;
  undefined2 *a0_01;
  int extraout_ECX;
  int extraout_ECX_00;
  int iVar2;
  undefined1 *puVar3;
  uint uVar4;
  int *in_stack_00000004;
  undefined2 *local_14;
  uint local_c;
  
  if (in_stack_00000004 != *(int **)(a1 + 0x814)) {
    *(int *)(in_stack_00000004[3] + 8) = in_stack_00000004[2];
    *(int *)(in_stack_00000004[2] + 0xc) = in_stack_00000004[3];
    iVar2 = *(int *)(*(int *)(a1 + 0x814) + 0xc);
    in_stack_00000004[2] = *(int *)(iVar2 + 8);
    *(int **)(*(int *)(iVar2 + 8) + 0xc) = in_stack_00000004;
    *(int **)(iVar2 + 8) = in_stack_00000004;
    in_stack_00000004[3] = iVar2;
  }
  piVar1 = (int *)(a1 + (a3 & 0xff) * 8);
  if (in_stack_00000004 != (int *)*piVar1) {
    *(int *)in_stack_00000004[1] = *in_stack_00000004;
    *(int *)(*in_stack_00000004 + 4) = in_stack_00000004[1];
    piVar1 = *(int **)(*piVar1 + 4);
    *in_stack_00000004 = *piVar1;
    *(int **)(*piVar1 + 4) = in_stack_00000004;
    *piVar1 = (int)in_stack_00000004;
    in_stack_00000004[1] = (int)piVar1;
  }
  (*(uint16 *)((uint8 *)&local_c + 0)) = (short)a2;
  *(ushort *)(in_stack_00000004 + 7) =
       (short)in_stack_00000004[7] + (ushort)*(byte *)(a1 + 0x801) + (short)local_c;
  if ((short)in_stack_00000004[7] < 0) {
    *(undefined2 *)(in_stack_00000004 + 7) = 0x7fff;
  }
  a0_00 = (undefined2 *)in_stack_00000004[5];
  local_c = a4;
  if ((a4 != 0) && (a0_00 != (undefined2 *)a0)) {
    if ((a0_00 < a0) && (a0 < (undefined1 *)((int)a0_00 + a4))) {
      a0 = fn_0090dc((int)a0_00 + a4);
    }
    else if (((((uint)a0_00 ^ a0) & 1) == 0) && (0xf < a4)) {
      a0_01 = a0_00;
      local_14 = (undefined2 *)a0;
      if (((uint)a0_00 & 1) != 0) {
        a0_01 = (undefined2 *)((int)a0_00 + 1);
        local_14 = (undefined2 *)(a0 + 1);
        *(undefined1 *)a0 = *(undefined1 *)a0_00;
        local_c = a4 - 1;
      }
      if ((((uint)a0_00 ^ a0) >> 1 & 1) == 0) {
        if (((uint)a0_01 >> 1 & 1) != 0) {
          *local_14 = *a0_01;
          local_14 = local_14 + 1;
          a0_01 = a0_01 + 1;
          local_c = local_c - 2;
        }
        fn_0090c8((uint)a0_01);
        a0 = local_c & 3;
        iVar2 = extraout_ECX_00;
        uVar4 = a0;
      }
      else {
        a0 = fn_0090b3((uint)a0_01);
        iVar2 = extraout_ECX;
        uVar4 = local_c & 1;
      }
      if (uVar4 != 0) {
        a0 = local_c - uVar4;
        puVar3 = (undefined1 *)(iVar2 + a0);
        local_14 = (undefined2 *)((int)local_14 + a0);
        do {
          a0 = CONCAT31((int3)(a0 >> 8),*puVar3);
          *(undefined1 *)local_14 = *puVar3;
          uVar4 = uVar4 - 1;
          puVar3 = puVar3 + 1;
          local_14 = (undefined2 *)((int)local_14 + 1);
        } while (uVar4 != 0);
      }
    }
    else {
      a0 = fn_00909f((uint)a0_00);
    }
  }
  return a0;
}

/* ----------------------------------------------------------------------
 * fn_002180   @0x002180, 1443 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_002cd2
 * calls: fn_0000c0, fn_002724, fn_00789c, fn_00909f, fn_0090b3, fn_0090c8, fn_0090dc
 */
 /* WARNING: Unknown calling convention */
uint fn_002180(a0, a1, a2, a3, a4)
    uint a0;
    uint a1;
    uint a2;
    uint a3;
    uint a4;
{
  int iVar1;
  int *piVar2;
  undefined2 *puVar3;
  code *pcVar4;
  short sVar5;
  uint uVar6;
  uint uVar7;
  int *extraout_ECX;
  undefined4 *puVar8;
  undefined2 *a0_00;
  undefined2 *a0_01;
  int extraout_ECX_00;
  int extraout_ECX_01;
  undefined1 *puVar9;
  int iVar10;
  int *piVar11;
  uint unaff_EDI;
  uint in_stack_00000004;
  undefined2 *local_40;
  uint local_3c;
  int *local_38;
  uint local_30;
  uint local_28;
  int local_14;
  int *local_8;
  
  local_28 = a1;
  iVar1 = *(int *)(*(int *)(a2 + 4) + 0x20);
  uVar6 = fn_00789c(iVar1 + 0xb0);
  if (uVar6 == 0) {
    uVar6 = *(uint *)(iVar1 + 0xac);
    if (((uVar6 != 0) && (*(int *)(a2 + 0x13c) == *(int *)(uVar6 + 0x80c))) &&
       ((in_stack_00000004 == 1 || (in_stack_00000004 == 2)))) {
      if (*(int *)(uVar6 + 0x804) == *(int *)(iVar1 + 4)) {
        if (in_stack_00000004 == 2) {
          if ((*(byte *)(a2 + 0x6a) & 2) == 0) {
            *(int *)(uVar6 + 0x838) = *(int *)(uVar6 + 0x838) + 1;
            *(int *)(uVar6 + 0x83c) = *(int *)(uVar6 + 0x83c) + a0;
          }
          else {
            *(int *)(uVar6 + 0x854) = *(int *)(uVar6 + 0x854) + 1;
            *(int *)(uVar6 + 0x858) = *(int *)(uVar6 + 0x858) + a0;
          }
          if (2 < a0) {
            *(int *)(uVar6 + 0x844) = *(int *)(uVar6 + 0x844) + 1;
          }
        }
        if ((a1 != 0) || (1 < a0)) {
          if ((a3 == 0) && (a0 < 3)) {
            if ((*(byte *)(a2 + 0x6a) & 2) == 0) {
              sVar5 = 0;
            }
            else {
              sVar5 = 2;
            }
            local_14 = 0;
            local_30 = a0;
            while (local_30 != 0) {
              puVar8 = *(undefined4 **)(uVar6 + (local_28 & 0xff) * 8);
              piVar2 = puVar8 + 1;
              for (; puVar8 != (undefined4 *)*piVar2; puVar8 = (undefined4 *)*puVar8) {
                if (local_28 == puVar8[4]) {
                  uVar7 = fn_0000c0(puVar8[5]);
                  local_38 = extraout_ECX;
                  if (uVar7 == extraout_ECX[6]) goto LAB_0000237b;
                  *(int *)(uVar6 + 0x85c) = *(int *)(uVar6 + 0x85c) + 1;
                  extraout_ECX[4] = 0;
                  *(int *)(extraout_ECX[3] + 8) = extraout_ECX[2];
                  *(int *)(extraout_ECX[2] + 0xc) = extraout_ECX[3];
                  *(int *)extraout_ECX[1] = *extraout_ECX;
                  *(int *)(*extraout_ECX + 4) = extraout_ECX[1];
                  extraout_ECX[2] = *(int *)(uVar6 + 0x81c);
                  *(int **)(uVar6 + 0x81c) = extraout_ECX;
                  break;
                }
              }
              local_38 = (int *)0x0;
LAB_0000237b:
              local_8 = local_38;
              if (local_38 == (int *)0x0) {
                if (local_28 != 0) {
                  local_8 = *(int **)(uVar6 + 0x81c);
                  if (local_8 == (int *)0x0) {
                    local_8 = *(int **)(uVar6 + 0x818);
                    piVar2 = (int *)local_8[2];
                    piVar11 = local_8;
                    for (; (local_8 != piVar2 &&
                           (*(short *)(local_8 + 7) = (short)local_8[7] + -1, 0 < (short)local_8[7])
                           ); local_8 = (int *)local_8[3]) {
                      if ((short)local_8[7] < (short)piVar11[7]) {
                        piVar11 = local_8;
                      }
                    }
                    if (local_8 == piVar2) {
                      local_8 = piVar11;
                    }
                    *(int *)(local_8[3] + 8) = local_8[2];
                    *(int *)(local_8[2] + 0xc) = local_8[3];
                    *(int *)local_8[1] = *local_8;
                    *(int *)(*local_8 + 4) = local_8[1];
                  }
                  else {
                    *(int *)(uVar6 + 0x81c) = local_8[2];
                  }
                  piVar2 = *(int **)(*(int *)(uVar6 + (local_28 & 0xff) * 8) + 4);
                  *local_8 = *piVar2;
                  *(int **)(*piVar2 + 4) = local_8;
                  *piVar2 = (int)local_8;
                  local_8[1] = (int)piVar2;
                  iVar10 = *(int *)(*(int *)(uVar6 + 0x814) + 0xc);
                  local_8[2] = *(int *)(iVar10 + 8);
                  *(int **)(*(int *)(iVar10 + 8) + 0xc) = local_8;
                  *(int **)(iVar10 + 8) = local_8;
                  local_8[3] = iVar10;
                  *(ushort *)(local_8 + 7) = (ushort)*(byte *)(uVar6 + 0x800) + sVar5;
                  local_8[4] = local_28;
                  if (sVar5 == 0) {
                    *(ushort *)((int)local_8 + 0x1e) = *(ushort *)((int)local_8 + 0x1e) & 0xfffe;
                  }
                  else {
                    *(byte *)((int)local_8 + 0x1e) = *(byte *)((int)local_8 + 0x1e) | 1;
                  }
                }
              }
              else {
                if (local_38 != *(int **)(uVar6 + 0x814)) {
                  *(int *)(local_38[3] + 8) = local_38[2];
                  *(int *)(local_38[2] + 0xc) = local_38[3];
                  iVar10 = *(int *)(*(int *)(uVar6 + 0x814) + 0xc);
                  local_38[2] = *(int *)(iVar10 + 8);
                  *(int **)(*(int *)(iVar10 + 8) + 0xc) = local_38;
                  *(int **)(iVar10 + 8) = local_38;
                  local_38[3] = iVar10;
                }
                *(ushort *)(local_38 + 7) =
                     (short)local_38[7] + (ushort)*(byte *)(uVar6 + 0x801) + sVar5;
                if ((short)local_38[7] < 0) {
                  *(undefined2 *)(local_38 + 7) = 0x7fff;
                }
              }
              if (local_8 != (int *)0x0) {
                a0_00 = (undefined2 *)(local_14 + *(int *)(a2 + 0x60));
                puVar3 = (undefined2 *)local_8[5];
                local_3c = *(uint *)(a2 + 0x13c);
                if ((local_3c != 0) && (a0_00 != puVar3)) {
                  if ((a0_00 < puVar3) && (puVar3 < (undefined2 *)((int)a0_00 + local_3c))) {
                    fn_0090dc((int)a0_00 + local_3c);
                  }
                  else if (((((uint)a0_00 ^ (uint)puVar3) & 1) == 0) && (0xf < local_3c)) {
                    a0_01 = a0_00;
                    local_40 = puVar3;
                    if (((uint)a0_00 & 1) != 0) {
                      a0_01 = (undefined2 *)((int)a0_00 + 1);
                      local_40 = (undefined2 *)((int)puVar3 + 1);
                      *(undefined1 *)puVar3 = *(undefined1 *)a0_00;
                      local_3c = local_3c - 1;
                    }
                    if ((((uint)a0_00 ^ (uint)puVar3) >> 1 & 1) == 0) {
                      if (((uint)a0_01 >> 1 & 1) != 0) {
                        *local_40 = *a0_01;
                        local_40 = local_40 + 1;
                        a0_01 = a0_01 + 1;
                        local_3c = local_3c - 2;
                      }
                      fn_0090c8((uint)a0_01);
                      uVar7 = local_3c & 3;
                      iVar10 = extraout_ECX_01;
                    }
                    else {
                      fn_0090b3((uint)a0_01);
                      uVar7 = local_3c & 1;
                      iVar10 = extraout_ECX_00;
                    }
                    if (uVar7 != 0) {
                      puVar9 = (undefined1 *)(iVar10 + (local_3c - uVar7));
                      local_40 = (undefined2 *)((int)local_40 + (local_3c - uVar7));
                      do {
                        *(undefined1 *)local_40 = *puVar9;
                        uVar7 = uVar7 - 1;
                        puVar9 = puVar9 + 1;
                        local_40 = (undefined2 *)((int)local_40 + 1);
                      } while (uVar7 != 0);
                    }
                  }
                  else {
                    fn_00909f((uint)a0_00);
                  }
                }
                local_8[6] = -1;
                uVar7 = fn_0000c0(local_8[5]);
                local_8[6] = uVar7;
              }
              local_28 = local_28 + 1;
              local_14 = local_14 + *(int *)(a2 + 0x13c);
              local_30 = local_30 - 1;
            }
          }
          else {
            fn_002724(a1,uVar6,in_stack_00000004,unaff_EDI);
          }
        }
        if (a3 != 0) {
          *(int *)(uVar6 + 0x860) = *(int *)(uVar6 + 0x860) + 1;
        }
      }
      else {
        fn_002724(1,uVar6,in_stack_00000004,unaff_EDI);
      }
    }
    uVar6 = a3;
    if ((int *)(iVar1 + 0xb0) == *(int **)(iVar1 + 0xb0)) {
      if (iVar1 + 100 == *(int *)(iVar1 + 0xb8)) {
        *(undefined4 *)(iVar1 + 0xb4) = 0;
        iVar10 = 0;
      }
      else {
        iVar10 = *(int *)(iVar1 + 0xb8);
        *(undefined4 *)(iVar1 + 0xb4) = *(undefined4 *)(iVar10 + 4);
        *(undefined4 *)(*(int *)(iVar10 + 0x58) + 0x54) = *(undefined4 *)(iVar10 + 0x54);
        *(undefined4 *)(*(int *)(iVar10 + 0x54) + 0x58) = *(undefined4 *)(iVar10 + 0x58);
        *(undefined4 *)(iVar10 + 0x58) = 0;
        *(undefined4 *)(iVar10 + 0x54) = 0;
      }
      if (iVar10 != 0) {
        pcVar4 = (code *)swi(0xff);
        (*pcVar4)();
      }
    }
  }
  else if (a3 != 0) {
    uVar6 = a3;
  }
  return uVar6;
}
