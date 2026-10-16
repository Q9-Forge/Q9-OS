/*
 * rbf_03.c - rbf part 3
 *
 * Original address range: 0x002724 - 0x003944 of the V4.9 kernel module.
 * Functions 0x2724 to 0x380a (support routines)
 * Generated from the Ghidra export; see tools/gen_src.py. Hand edits belong in hand/*.inc.
 */
#include "kernel.h"

/* ----------------------------------------------------------------------
 * fn_002724   @0x002724, 294 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_001796, fn_002180, fn_002bdc, fn_003f8c, fn_0058f4
 */
 /* WARNING: Unknown calling convention */
uint fn_002724(a0, a1, a2, a3)
    uint a0;
    uint a1;
    uint a2;
    uint a3;
{
  int *piVar1;
  uint uVar2;
  int *piVar3;
  uint in_stack_00000004;
  uint local_18;
  uint local_14;
  uint local_10;
  uint local_c;
  
  uVar2 = in_stack_00000004 + a0;
  local_18 = in_stack_00000004 + 0xff >> 8;
  local_14 = in_stack_00000004 & 0xff;
  if (local_14 == 0) {
    local_14 = 0x100;
  }
  local_10 = 0;
  if (0xff < in_stack_00000004) {
    in_stack_00000004 = 0x100;
  }
  do {
    if (in_stack_00000004 <= local_10) {
      return local_10;
    }
    piVar3 = *(int **)(a1 + (local_10 + a0 & 0xff) * 8);
    if (local_14 == local_10) {
      local_18 = local_18 - 1;
    }
    local_c = 0;
    piVar1 = (int *)piVar3[1];
    for (; piVar3 != piVar1; piVar3 = (int *)*piVar3) {
      if ((a0 <= (uint)piVar3[4]) && ((uint)piVar3[4] < uVar2)) {
        piVar3[4] = 0;
        *(int *)(piVar3[3] + 8) = piVar3[2];
        *(int *)(piVar3[2] + 0xc) = piVar3[3];
        *(int *)piVar3[1] = *piVar3;
        *(int *)(*piVar3 + 4) = piVar3[1];
        piVar3[2] = *(int *)(a1 + 0x81c);
        *(int **)(a1 + 0x81c) = piVar3;
        local_c = local_c + 1;
        if (local_18 <= local_c) break;
      }
    }
    if (a2 == 1) {
      *(int *)(a1 + 0x82c) = *(int *)(a1 + 0x82c) + local_c;
    }
    else {
      *(int *)(a1 + 0x840) = *(int *)(a1 + 0x840) + local_c;
    }
    local_10 = local_10 + 1;
  } while( true );
}

/* ----------------------------------------------------------------------
 * fn_00284a   @0x00284a, 32 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0054b8, fn_005cfc, fn_006f10, fn_0081fa
 * calls: fn_00286a, fn_002888
 */
 /* WARNING: Unknown calling convention */
uint fn_00284a(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  uint uVar2;
  uint a0_00;
  uint in_stack_00000004;
  
  uVar1 = fn_00286a(in_stack_00000004);
  uVar2 = a0_00;
  if (uVar1 != 0) {
    uVar2 = fn_002888(a0_00);
  }
  return uVar2;
}

/* ----------------------------------------------------------------------
 * fn_00286a   @0x00286a, 30 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_00284a, fn_00352c, fn_008878
 */
 /* WARNING: Unknown calling convention */
uint fn_00286a(a0)
    uint a0;
{
  return (uint)(*(char *)(*(int *)(*(int *)(a0 + 4) + 0x20) + 200) == 'B');
}

/* ----------------------------------------------------------------------
 * fn_002888   @0x002888, 62 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_00284a, fn_008878
 */
 /* WARNING: Unknown calling convention */
uint fn_002888(a0)
    uint a0;
{
  undefined1 local_c;
  undefined1 uStack_b;
  undefined1 uStack_a;
  undefined1 uStack_9;
  undefined4 local_8;
  
  uStack_9 = (undefined1)(a0 >> 0x18);
  uStack_a = (undefined1)(a0 >> 0x10);
  uStack_b = (undefined1)(a0 >> 8);
  local_c = (undefined1)a0;
  local_8 = CONCAT13(local_c,CONCAT12(uStack_b,CONCAT11(uStack_a,uStack_9)));
  return local_8;
}

/* ----------------------------------------------------------------------
 * fn_0028c6   @0x0028c6, 174 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0035d6
 */
 /* WARNING: Unknown calling convention */
uint fn_0028c6(a0, a1)
    uint a0;
    uint a1;
{
  byte bVar1;
  ushort uVar2;
  undefined4 uVar3;
  int in_stack_00000004;
  undefined4 local_14;
  undefined1 local_10;
  undefined1 uStack_f;
  undefined1 uStack_e;
  undefined1 uStack_d;
  byte *local_c;
  
  for (local_c = (byte *)a0; *local_c != 0; local_c = local_c + 2) {
    bVar1 = *local_c;
    if (bVar1 == 0x20) {
      uVar3 = *(undefined4 *)((uint)local_c[1] + in_stack_00000004);
      uStack_d = (undefined1)((uint)uVar3 >> 0x18);
      uStack_e = (undefined1)((uint)uVar3 >> 0x10);
      uStack_f = (undefined1)((uint)uVar3 >> 8);
      local_10 = (undefined1)uVar3;
      local_14 = CONCAT13(local_10,CONCAT12(uStack_f,CONCAT11(uStack_e,uStack_d)));
      *(undefined4 *)((uint)local_c[1] + in_stack_00000004) = local_14;
    }
    else if ((bVar1 < 0x21) && (bVar1 == 0x10)) {
      uVar2 = *(ushort *)((uint)local_c[1] + in_stack_00000004);
      *(ushort *)((uint)local_c[1] + in_stack_00000004) = (uVar2 >> 8) + uVar2 * 0x100;
    }
  }
  return CONCAT22((short)((uint)local_c >> 0x10),(ushort)*local_c);
}

/* ----------------------------------------------------------------------
 * fn_002988   @0x002988, 91 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0017f6, fn_002c74, fn_003a26, rbf_write
 * calls: fn_002eca, fn_00789c, lock_release
 */
 /* WARNING: Unknown calling convention */
uint fn_002988(a0)
    uint a0;
{
  uint uVar1;
  int extraout_ECX;
  uint extraout_EDX;
  uint unaff_ESI;
  undefined1 local_8 [4];
  
  if ((*(uint *)(a0 + 0x78) - *(uint *)(a0 + 0x80) < *(uint *)(a0 + 0x88)) &&
     (*(uint *)(a0 + 0x80) <= *(uint *)(a0 + 0x78))) {
    uVar1 = 0;
  }
  else {
    uVar1 = fn_00789c(*(int *)(a0 + 0x70) + 0x28);
    if (uVar1 == 0) {
      fn_002eca((uint)local_8,unaff_ESI);
      lock_release(*(int *)(extraout_ECX + 0x70) + 0x28);
      uVar1 = extraout_EDX;
    }
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_0029e4   @0x0029e4, 236 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_004bf8, fn_0081fa
 * calls: fn_001332, fn_001a00, fn_001b52, fn_002ad0, fn_00789c, lock_release
 */
 /* WARNING: Unknown calling convention */
uint fn_0029e4(a0)
    uint a0;
{
  int *piVar1;
  undefined4 uVar2;
  uint uVar3;
  uint extraout_ECX;
  int extraout_EDX;
  uint a0_00;
  uint extraout_EDX_00;
  uint unaff_EDI;
  uint local_c;
  uint local_8;
  
  local_8 = a0;
  uVar3 = fn_001a00((uint)&local_c,unaff_EDI);
  if (uVar3 == 0) {
    piVar1 = *(int **)(local_c + 0x28);
    uVar2 = *(undefined4 *)(local_8 + 0x60);
    *(int **)(local_8 + 0x60) = piVar1;
    *(undefined4 *)(local_8 + 0x8c) = *(undefined4 *)(*(int *)(*(int *)(local_8 + 4) + 0x20) + 4);
    uVar3 = fn_002ad0(1,local_8,unaff_EDI);
    *(undefined4 *)(local_8 + 0x60) = uVar2;
    if ((uVar3 == 0) && (*piVar1 == -0x524f4f53)) {
      if (*(int *)(extraout_EDX + 0x8c) != piVar1[1]) {
        *(int *)(extraout_EDX + 0x8c) = piVar1[1];
        fn_00789c(extraout_EDX + 0x74);
        uVar3 = a0_00;
        if (*(int *)(a0_00 + 0x84) != 0) {
          fn_001332(a0_00);
          uVar3 = extraout_EDX_00;
        }
        lock_release(uVar3 + 0x74);
      }
      lock_release(local_c + 0x10);
      uVar3 = fn_001b52(local_c,unaff_EDI);
    }
    else {
      lock_release(local_c + 0x10);
      fn_001b52(local_c,unaff_EDI);
      uVar3 = extraout_ECX;
    }
  }
  return uVar3;
}

/* ----------------------------------------------------------------------
 * fn_002ad0   @0x002ad0, 142 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_000d72, fn_0029e4, fn_003374, fn_00486a
 * calls: fn_001d1a, fn_002cd2, fn_00789c, lock_release
 */
 /* WARNING: Unknown calling convention */
uint fn_002ad0(a0, a1, a2)
    uint a0;
    uint a1;
    uint a2;
{
  uint uVar1;
  uint a0_00;
  uint extraout_ECX;
  int extraout_EDX;
  uint unaff_EDI;
  
  if ((*(int *)(a1 + 0x8c) == 0) ||
     (*(int *)(a1 + 0x8c) == *(int *)(*(int *)(*(int *)(a1 + 4) + 0x20) + 4))) {
    uVar1 = 0;
  }
  else {
    uVar1 = 0xfb;
  }
  if (uVar1 == 0) {
    uVar1 = fn_001d1a(a0,a1,unaff_EDI);
    if (uVar1 == 0) {
      uVar1 = fn_00789c(*(uint *)(*(int *)(*(int *)(a1 + 4) + 0x18) + 0x1c));
      if (uVar1 == 0) {
        fn_002cd2(a0_00,1,a1,0,unaff_EDI);
        lock_release(*(uint *)(extraout_EDX + 0x1c));
        uVar1 = extraout_ECX;
      }
      if (uVar1 == 0) {
        uVar1 = 0;
      }
    }
    else {
      uVar1 = 0;
    }
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_002b5e   @0x002b5e, 37 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_001740, fn_001796, fn_003f8c
 */
 /* WARNING: Unknown calling convention */
uint fn_002b5e(a0)
    uint a0;
{
  uint uVar1;
  
  if ((*(int *)(a0 + 0x8c) == 0) ||
     (*(int *)(a0 + 0x8c) == *(int *)(*(int *)(*(int *)(a0 + 4) + 0x20) + 4))) {
    uVar1 = 0;
  }
  else {
    uVar1 = 0xfb;
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_002b84   @0x002b84, 88 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_000770, fn_001740, fn_001796, rbf_getstat, rbf_setstat, fn_0066d8, fn_006ae0
 * calls: fn_002cd2, fn_00789c, lock_release
 */
 /* WARNING: Unknown calling convention */
uint fn_002b84(a0, a1, a2, a3, a4)
    uint a0;
    uint a1;
    uint a2;
    uint a3;
    uint a4;
{
  int iVar1;
  uint uVar2;
  uint extraout_ECX;
  uint a2_00;
  uint unaff_EDI;
  
  iVar1 = *(int *)(*(int *)(a2 + 4) + 0x18);
  uVar2 = fn_00789c(*(uint *)(iVar1 + 0x1c));
  if (uVar2 == 0) {
    fn_002cd2(a0,a1,a2_00,a3,unaff_EDI);
    lock_release(*(uint *)(iVar1 + 0x1c));
    uVar2 = extraout_ECX;
  }
  return uVar2;
}

/* ----------------------------------------------------------------------
 * fn_002bdc   @0x002bdc, 151 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_001468, fn_00352c, fn_006194
 * calls: fn_002724, fn_002cd2, fn_00789c, lock_release
 */
 /* WARNING: Unknown calling convention */
uint fn_002bdc(a0, a1, a2)
    uint a0;
    uint a1;
    uint a2;
{
  uint uVar1;
  uint extraout_ECX;
  uint extraout_ECX_00;
  int extraout_EDX;
  uint unaff_EDI;
  uint in_stack_00000004;
  
  if ((*(int *)(a1 + 0x8c) == 0) ||
     (*(int *)(a1 + 0x8c) == *(int *)(*(int *)(*(int *)(a1 + 4) + 0x20) + 4))) {
    uVar1 = 0;
  }
  else {
    uVar1 = 0xfb;
  }
  if (uVar1 == 0) {
    uVar1 = *(uint *)(*(int *)(*(int *)(a1 + 4) + 0x20) + 0xac);
    if (uVar1 != 0) {
      fn_002724(in_stack_00000004,uVar1,2,unaff_EDI);
      a0 = extraout_ECX;
    }
    uVar1 = fn_00789c(*(uint *)(*(int *)(*(int *)(a1 + 4) + 0x18) + 0x1c));
    if (uVar1 == 0) {
      fn_002cd2(a0,2,a1,0,unaff_EDI);
      lock_release(*(uint *)(extraout_EDX + 0x1c));
      uVar1 = extraout_ECX_00;
    }
    if (uVar1 == 0) {
      uVar1 = 0;
    }
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_002c74   @0x002c74, 93 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_003a26, rbf_write, fn_0054b8, fn_005cfc, fn_008776
 * calls: fn_001592, fn_001740, fn_002988
 */
 /* WARNING: Unknown calling convention */
uint fn_002c74(a0)
    uint a0;
{
  uint uVar1;
  int extraout_EDX;
  uint unaff_ESI;
  
  uVar1 = fn_002988(a0);
  if (uVar1 == 0) {
    if ((*(int *)(extraout_EDX + 0x6c) == 0) ||
       (((uint)(*(int *)(extraout_EDX + 0x78) - *(int *)(extraout_EDX + 0x80)) >>
        (*(byte *)(extraout_EDX + 0x67) & 0x1f)) + *(int *)(extraout_EDX + 0x84) !=
        **(int **)(extraout_EDX + 0x6c))) {
      uVar1 = fn_001592(unaff_ESI);
    }
    else {
      if (((*(uint *)(*(int *)(extraout_EDX + 0x6c) + 8) >> 1 & 1) == 0) &&
         (uVar1 = fn_001740(*(uint *)(extraout_EDX + 0x6c),unaff_ESI), uVar1 != 0)) {
        return uVar1;
      }
      uVar1 = 0;
    }
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_002cd2   @0x002cd2, 487 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_002ad0, fn_002b84, fn_002bdc, fn_003f8c
 * calls: fn_002180
 */
 /* WARNING: Unknown calling convention */
uint fn_002cd2(a0, a1, a2, a3, a4)
    uint a0;
    uint a1;
    uint a2;
    uint a3;
    uint a4;
{
  byte bVar1;
  ushort uVar2;
  int iVar3;
  int iVar4;
  int *piVar5;
  undefined4 uVar6;
  code *pcVar7;
  uint unaff_EDI;
  undefined8 uVar8;
  uint in_stack_00000004;
  undefined1 local_30;
  undefined1 uStack_2f;
  undefined1 uStack_2e;
  undefined1 uStack_2d;
  undefined4 local_2c;
  byte *local_28;
  uint a3_00;
  
  iVar3 = *(int *)(a2 + 4);
  iVar4 = *(int *)(iVar3 + 0x18);
  switch(a1) {
  case 1:
    pcVar7 = *(code **)(iVar4 + 8);
    break;
  case 2:
    if ((*(uint *)(a2 + 0x140) >> 6 & 1) != 0) {
      return 0xf2;
    }
    pcVar7 = *(code **)(iVar4 + 0xc);
    break;
  case 3:
    pcVar7 = *(code **)(iVar4 + 0x10);
    break;
  case 4:
    pcVar7 = *(code **)(iVar4 + 0x14);
    break;
  default:
    pcVar7 = (code *)0x0;
  }
  *(undefined4 *)(iVar4 + 0x20) = *(undefined4 *)(a2 + 0x1c);
  if ((a1 == 1) || (a1 == 2)) {
    uVar8 = (*pcVar7)();
  }
  else {
    uVar8 = (*pcVar7)(a2,iVar3);
  }
  a3_00 = (uint)uVar8;
  *(undefined4 *)(iVar4 + 0x20) = 0;
  fn_002180(a0,in_stack_00000004,a2,a3_00,unaff_EDI);
  if ((((a1 == 1) && (in_stack_00000004 == 0)) && (a3_00 == 0)) &&
     ((piVar5 = *(int **)(iVar3 + 0x20), *piVar5 == -0x524f4f53 &&
      (*(char *)(piVar5 + 0x32) = (char)piVar5[0x16], (char)piVar5[0x16] == 'B')))) {
    for (local_28 = (byte *)((int)((ulonglong)uVar8 >> 0x20) + 0x6c); *local_28 != 0;
        local_28 = local_28 + 2) {
      bVar1 = *local_28;
      if (bVar1 == 0x20) {
        uVar6 = *(undefined4 *)((uint)local_28[1] + (int)piVar5);
        uStack_2d = (undefined1)((uint)uVar6 >> 0x18);
        uStack_2e = (undefined1)((uint)uVar6 >> 0x10);
        uStack_2f = (undefined1)((uint)uVar6 >> 8);
        local_30 = (undefined1)uVar6;
        local_2c = CONCAT13(local_30,CONCAT12(uStack_2f,CONCAT11(uStack_2e,uStack_2d)));
        *(undefined4 *)((uint)local_28[1] + (int)piVar5) = local_2c;
      }
      else if ((bVar1 < 0x21) && (bVar1 == 0x10)) {
        uVar2 = *(ushort *)((uint)local_28[1] + (int)piVar5);
        *(ushort *)((uint)local_28[1] + (int)piVar5) = (uVar2 >> 8) + uVar2 * 0x100;
      }
    }
    *piVar5 = -0x4f525250;
  }
  return a3_00;
}

/* ----------------------------------------------------------------------
 * fn_002eca   @0x002eca, 346 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0000ea, fn_000a76, fn_002988
 */
 /* WARNING: Unknown calling convention */
uint fn_002eca(a0, a1)
    uint a0;
    uint a1;
{
  int iVar1;
  uint uVar2;
  int in_stack_00000004;
  undefined4 *local_14;
  undefined4 *local_10;
  
  local_14 = *(undefined4 **)(in_stack_00000004 + 0x74);
  local_10 = *(undefined4 **)(in_stack_00000004 + 0x9c);
  if (local_10 == (undefined4 *)0x0) {
    local_14 = *(undefined4 **)(in_stack_00000004 + 0x70);
    local_10 = local_14 + 0x1a;
    *(undefined4 **)(in_stack_00000004 + 0x74) = local_14;
    *(undefined4 *)(in_stack_00000004 + 0x80) = 0;
  }
  while( true ) {
    for (; *(uint *)(in_stack_00000004 + 0x78) < (uint)local_14[2];
        local_14 = (undefined4 *)*local_14) {
    }
    if (*(uint *)(in_stack_00000004 + 0x78) < (uint)(local_14[2] + local_14[3])) break;
    local_14 = (undefined4 *)local_14[1];
    if (local_14 == *(undefined4 **)(in_stack_00000004 + 0x70)) {
      return 0xd5;
    }
  }
  uVar2 = *(uint *)(in_stack_00000004 + 0x80);
  if (local_14 != *(undefined4 **)(in_stack_00000004 + 0x74)) {
    uVar2 = local_14[2];
    local_10 = local_14 + 0x1a;
  }
  iVar1 = local_10[1] << (*(byte *)(in_stack_00000004 + 0x67) & 0x1f);
  *(int *)(in_stack_00000004 + 0x88) = iVar1;
  *(undefined4 *)(in_stack_00000004 + 0x84) = *local_10;
  while( true ) {
    for (; *(uint *)(in_stack_00000004 + 0x78) < uVar2; uVar2 = uVar2 - iVar1) {
      if (local_10 == local_14 + 0x1a) {
        return 0x2df2;
      }
      iVar1 = local_10[-1] << (*(byte *)(in_stack_00000004 + 0x67) & 0x1f);
      local_10 = local_10 + -2;
    }
    if (*(uint *)(in_stack_00000004 + 0x78) < uVar2 + iVar1) break;
    uVar2 = uVar2 + iVar1;
    if (local_10 + 2 == (undefined4 *)((int)local_14 + *(int *)(in_stack_00000004 + 0x13c) + 0x30))
    {
      return 0x2df2;
    }
    iVar1 = local_10[3] << (*(byte *)(in_stack_00000004 + 0x67) & 0x1f);
    local_10 = local_10 + 2;
  }
  *(int *)(in_stack_00000004 + 0x88) = iVar1;
  *(uint *)(in_stack_00000004 + 0x80) = uVar2;
  *(undefined4 *)(in_stack_00000004 + 0x84) = *local_10;
  *(undefined4 **)(in_stack_00000004 + 0x9c) = local_10;
  *(undefined4 **)a0 = local_10;
  *(undefined4 **)(in_stack_00000004 + 0x74) = local_14;
  return 0;
}

/* ----------------------------------------------------------------------
 * fn_003024   @0x003024, 67 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_003068, fn_0076fa
 */
 /* WARNING: Unknown calling convention */
uint fn_003024(a0)
    uint a0;
{
  *(undefined4 *)(a0 + 0x74) = *(undefined4 *)(a0 + 0x70);
  *(int *)(a0 + 0x9c) = *(int *)(a0 + 0x70) + 0x68;
  *(int *)(a0 + 0x88) = *(int *)(*(int *)(a0 + 0x9c) + 4) << (*(byte *)(a0 + 0x67) & 0x1f);
  *(undefined4 *)(a0 + 0x84) = **(undefined4 **)(a0 + 0x9c);
  *(undefined4 *)(a0 + 0x80) = 0;
  return a0;
}

/* ----------------------------------------------------------------------
 * fn_003068   @0x003068, 555 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_006194, fn_0076fa
 * calls: fn_003024, fn_003294, fn_003374, fn_0033f6, fn_00344a, fn_0037d2, fn_006054, fn_00789c
 */
 /* WARNING: Unknown calling convention */
uint fn_003068(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  uint uVar2;
  uint a0_00;
  uint extraout_ECX;
  uint unaff_EDI;
  uint *local_24;
  uint local_20;
  uint local_1c;
  uint local_18;
  uint local_14;
  uint local_10;
  uint local_c;
  uint local_8;
  
  local_18 = a0;
  uVar1 = fn_003294((uint)&local_20,unaff_EDI);
  if (uVar1 == 0) {
    local_10 = local_20;
    fn_00789c(local_20 + 0x28);
    *(uint *)(local_18 + 0x70) = local_10;
    *(uint *)local_10 = local_10;
    *(uint *)(local_10 + 4) = local_10;
    *(undefined4 *)(local_10 + 0x10) = *(undefined4 *)(local_18 + 0xc0);
    *(undefined4 *)(local_10 + 0x14) = 0;
    if (a0_00 == 0) {
      uVar1 = fn_003374(local_10,unaff_EDI);
      if (uVar1 != 0) {
        lock_release(local_10 + 0x28);
        return extraout_ECX;
      }
    }
    else {
      fn_006054(a0_00,*(uint *)(local_18 + 0x13c),unaff_EDI);
    }
    fn_0033f6(local_10,unaff_EDI);
    *(undefined4 *)(local_18 + 0x7c) = *(undefined4 *)(local_10 + 0x4c);
    local_c = 0;
    if ((*(byte *)(local_10 + 0x42) & 1) == 0) {
      while (uVar1 = *(uint *)(local_10 + *(int *)(local_18 + 0x13c) + 0x34), local_8 = uVar1,
            uVar1 != *(uint *)(local_18 + 0xc0)) {
        local_14 = 0xf1;
        if ((uVar1 == 0) || (local_c == uVar1)) {
LAB_0000321f:
          lock_release(*(int *)(local_18 + 0x70) + 0x28);
          fn_00344a(local_18);
          return local_14;
        }
        local_1c = local_10;
        uVar2 = fn_003294((uint)&local_24,unaff_EDI);
        if (uVar2 == 0) {
          local_24[4] = uVar1;
          uVar2 = fn_003374((uint)local_24,unaff_EDI);
          if (uVar2 == 0) {
            local_24[1] = *(uint *)(local_1c + 4);
            *(uint **)(local_1c + 4) = local_24;
            *(uint **)local_24[1] = local_24;
            *local_24 = local_1c;
            fn_0033f6((uint)local_24,unaff_EDI);
            uVar2 = 0;
          }
          else {
            fn_0037d2((uint)local_24,unaff_EDI);
          }
        }
        local_14 = uVar2;
        if (uVar2 != 0) goto LAB_0000321f;
        local_10 = *(uint *)(local_10 + 4);
        local_c = local_8;
        local_14 = 0;
      }
      fn_003024(local_18);
    }
    else {
      *(undefined4 *)(local_18 + 0x74) = 0;
      *(undefined4 *)(local_18 + 0x9c) = 0;
    }
    lock_release(*(int *)(local_18 + 0x70) + 0x28);
    uVar1 = 0;
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_003294   @0x003294, 223 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_003068, fn_00380a
 * calls: fn_00789c, lock_release, os_f_srqmem
 */
 /* WARNING: Unknown calling convention */
uint fn_003294(a0, a1)
    uint a0;
    uint a1;
{
  int extraout_ECX;
  int iVar1;
  int iVar2;
  uint unaff_EDI;
  int in_stack_00000004;
  int local_10;
  uint local_c;
  int local_8;
  
  local_c = 0;
  local_8 = *(int *)(*(int *)(in_stack_00000004 + 4) + 0x20);
  fn_00789c(local_8 + 0xcc);
  iVar1 = 0;
  for (iVar2 = *(int *)(local_8 + 0xdc);
      (iVar2 != 0 && (*(int *)(iVar2 + 0xc) != *(int *)(extraout_ECX + 0x13c)));
      iVar2 = *(int *)(iVar2 + 4)) {
    iVar1 = iVar2;
  }
  if (iVar2 == 0) {
    local_10 = *(int *)(extraout_ECX + 0x13c) + 0x38;
    local_c = os_f_srqmem((uint)&local_10,0,unaff_EDI);
    if (local_c == 0) {
      iVar2 = *(int *)a0;
    }
  }
  else {
    *(int *)a0 = iVar2;
    if (iVar1 == 0) {
      *(undefined4 *)(local_8 + 0xdc) = *(undefined4 *)(iVar2 + 4);
    }
    else {
      *(undefined4 *)(iVar1 + 4) = *(undefined4 *)(iVar2 + 4);
    }
  }
  if (iVar2 != 0) {
    *(int *)(iVar2 + 0x18) = iVar2 + 0x18;
    *(undefined4 *)(iVar2 + 0x1c) = 0;
    *(int *)(iVar2 + 0x24) = iVar2 + -0x34;
    *(int *)(iVar2 + 0x20) = iVar2 + -0x34;
    *(int *)(iVar2 + 0x28) = iVar2 + 0x28;
    *(undefined4 *)(iVar2 + 0x2c) = 0;
    *(int *)(iVar2 + 0x34) = iVar2 + -0x24;
    *(int *)(iVar2 + 0x30) = iVar2 + -0x24;
  }
  lock_release(local_8 + 0xcc);
  return local_c;
}

/* ----------------------------------------------------------------------
 * fn_003374   @0x003374, 130 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_003068
 * calls: fn_0000c0, fn_002ad0, fn_0035d6
 */
 /* WARNING: Unknown calling convention */
uint fn_003374(a0, a1)
    uint a0;
    uint a1;
{
  undefined4 uVar1;
  uint uVar2;
  uint uVar3;
  int *a0_00;
  uint a0_01;
  int extraout_EDX;
  uint unaff_ESI;
  uint in_stack_00000004;
  uint uVar4;
  
  uVar1 = *(undefined4 *)(in_stack_00000004 + 0x60);
  *(uint *)(in_stack_00000004 + 0x60) = a0 + 0x38;
  uVar2 = fn_002ad0(1,in_stack_00000004,unaff_ESI);
  *(undefined4 *)(extraout_EDX + 0x60) = uVar1;
  if (uVar2 == 0) {
    if ((*a0_00 == -0x24f4f03) || (*a0_00 == -0x4f020250)) {
      uVar2 = a0_00[1];
      a0_00[1] = -1;
      uVar4 = *(uint *)(extraout_EDX + 0x13c);
      uVar3 = fn_0000c0((uint)a0_00);
      *(uint *)(a0_01 + 4) = uVar2;
      if (uVar2 == uVar3) {
        fn_0035d6(a0_01,uVar4 & 0xffffff00,unaff_ESI);
        uVar2 = 0;
      }
      else {
        uVar2 = 0xf3;
      }
    }
    else {
      uVar2 = 0xf3;
    }
  }
  return uVar2;
}

/* ----------------------------------------------------------------------
 * fn_0033f6   @0x0033f6, 83 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0000ea, fn_000a76, fn_003068
 */
 /* WARNING: Unknown calling convention */
uint fn_0033f6(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  int iVar2;
  int iVar3;
  int in_stack_00000004;
  
  iVar3 = 0;
  for (iVar2 = a0 + 0x68;
      (iVar2 != a0 + *(int *)(in_stack_00000004 + 0x13c) + 0x30 && (*(int *)(iVar2 + 4) != 0));
      iVar2 = iVar2 + 8) {
    iVar3 = iVar3 + *(int *)(iVar2 + 4);
  }
  *(int *)(a0 + 0xc) = iVar3 << (*(byte *)(in_stack_00000004 + 0x67) & 0x1f);
  uVar1 = *(uint *)a0;
  if ((uVar1 == a0) || (a0 == *(uint *)(in_stack_00000004 + 0x70))) {
    iVar2 = 0;
  }
  else {
    iVar2 = *(int *)(uVar1 + 8) + *(int *)(uVar1 + 0xc);
  }
  *(int *)(a0 + 8) = iVar2;
  return a0;
}

/* ----------------------------------------------------------------------
 * fn_00344a   @0x00344a, 164 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_003068, fn_0079d2
 * calls: fn_00352c, fn_0037d2, fn_00789c, os_f_srtmem, lock_release
 */
 /* WARNING: Unknown calling convention */
uint fn_00344a(a0)
    uint a0;
{
  uint a0_00;
  uint uVar1;
  uint extraout_ECX;
  uint extraout_EDX;
  uint unaff_EDI;
  
  uVar1 = *(uint *)(a0 + 0x70);
  if (uVar1 == 0) {
    uVar1 = 0;
  }
  else {
    a0_00 = uVar1 + 0x28;
    fn_00789c(a0_00);
    if ((*(byte *)(a0 + 100) & 1) != 0) {
      *(byte *)(a0 + 100) = *(byte *)(a0 + 100) & 0xfe;
      fn_00352c(*(uint *)(a0 + 0x70),unaff_EDI);
    }
    do {
      uVar1 = fn_0037d2(uVar1,unaff_EDI);
      if (uVar1 != 0) {
        *(undefined4 *)(a0 + 0x74) = 0;
        *(undefined4 *)(a0 + 0x70) = 0;
        *(undefined4 *)(a0 + 0x9c) = 0;
        os_f_srtmem(a0_00);
        lock_release(*(int *)(a0 + 0x70) + 0x28);
        return extraout_ECX;
      }
      uVar1 = extraout_EDX;
    } while (extraout_EDX != *(uint *)(a0 + 0x70));
    *(undefined4 *)(a0 + 0x74) = 0;
    *(undefined4 *)(a0 + 0x70) = 0;
    *(undefined4 *)(a0 + 0x9c) = 0;
    uVar1 = lock_release(a0_00);
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_0034ee   @0x0034ee, 44 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_004fd4, fn_006194, fn_006f10, fn_0073d0
 * calls: fn_00351a, fn_00789c, lock_release
 */
 /* WARNING: Unknown calling convention */
uint fn_0034ee(a0)
    uint a0;
{
  uint a0_00;
  int extraout_ECX;
  uint extraout_EDX;
  
  fn_00789c(*(int *)(a0 + 0x70) + 0x28);
  fn_00351a(a0_00);
  lock_release(*(int *)(extraout_ECX + 0x70) + 0x28);
  return extraout_EDX;
}

/* ----------------------------------------------------------------------
 * fn_00351a   @0x00351a, 17 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0000ea, fn_000a76, fn_0034ee, fn_00380a, fn_004e64, fn_0054b8, fn_006194, fn_006894
 * calls: fn_00352c
 */
 /* WARNING: Unknown calling convention */
uint fn_00351a(a0)
    uint a0;
{
  uint uVar1;
  uint unaff_retaddr;
  
  *(byte *)(a0 + 100) = *(byte *)(a0 + 100) & 0xfe;
  uVar1 = fn_00352c(*(uint *)(a0 + 0x70),unaff_retaddr);
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_00352c   @0x00352c, 155 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0000ea, fn_000a76, fn_00344a, fn_00351a, fn_00380a
 * calls: fn_0000c0, fn_00286a, fn_002bdc, fn_0035d6
 */
 /* WARNING: Unknown calling convention */
uint fn_00352c(a0, a1)
    uint a0;
    uint a1;
{
  undefined4 uVar1;
  undefined2 uVar2;
  uint uVar3;
  uint uVar4;
  uint a0_00;
  int extraout_ECX;
  uint a1_00;
  int extraout_ECX_00;
  int extraout_EDX;
  int extraout_EDX_00;
  uint unaff_EDI;
  int in_stack_00000004;
  uint uVar5;
  undefined4 in_stack_ffffffe8;
  
  if (a0 == 0) {
    uVar3 = 0xd3;
  }
  else {
    uVar1 = *(undefined4 *)(in_stack_00000004 + 0x60);
    *(uint *)(in_stack_00000004 + 0x60) = a0 + 0x38;
    fn_0035d6(*(uint *)(in_stack_00000004 + 0x60),
                 CONCAT31((int3)((uint)in_stack_ffffffe8 >> 8),1),unaff_EDI);
    uVar3 = fn_00286a(a0_00);
    if (uVar3 == 0) {
      uVar2 = 1;
    }
    else {
      uVar2 = 0x100;
    }
    *(undefined2 *)(*(int *)(extraout_ECX + 0x60) + 0x2c) = uVar2;
    *(undefined4 *)(*(int *)(extraout_ECX + 0x60) + 4) = 0xffffffff;
    uVar3 = *(uint *)(extraout_ECX + 0x13c);
    uVar4 = fn_0000c0(*(uint *)(extraout_ECX + 0x60));
    *(uint *)(*(int *)(a1_00 + 0x60) + 4) = uVar4;
    uVar4 = *(uint *)(extraout_EDX + 0x10);
    uVar5 = 0x35a7;
    uVar3 = fn_002bdc(1,a1_00,uVar3);
    fn_0035d6(extraout_EDX_00 + 0x38,uVar5 & 0xffffff00,uVar4);
    *(undefined4 *)(extraout_ECX_00 + 0x60) = uVar1;
  }
  return uVar3;
}

/* ----------------------------------------------------------------------
 * fn_0035d6   @0x0035d6, 507 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_000d72, fn_003374, fn_00352c, fn_00486a, fn_006194
 * calls: fn_0028c6
 */
 /* WARNING: Unknown calling convention */
uint fn_0035d6(a0, a1, a2)
    uint a0;
    uint a1;
    uint a2;
{
  int iVar1;
  undefined4 uVar2;
  uint uVar3;
  uint uVar4;
  undefined4 *extraout_ECX;
  undefined4 *extraout_ECX_00;
  uint extraout_EDX;
  Statics *g;
  ushort uVar6;
  uint unaff_EDI;
  uint uVar7;
  int in_stack_00000004;
  undefined4 local_20;
  undefined1 local_1c;
  undefined1 uStack_1b;
  undefined1 uStack_1a;
  undefined1 uStack_19;
  undefined4 local_18;
  undefined1 local_14;
  undefined1 uStack_13;
  undefined1 uStack_12;
  undefined1 uStack_11;
  uint local_c;
  undefined4 *local_8;
  undefined2 uVar5;
  
  local_20 = (undefined4 *)CONCAT31((int3)((uint)in_stack_00000004 >> 8),(byte)a1);
  iVar1 = *(int *)(*(int *)(in_stack_00000004 + 4) + 0x20);
  uVar6 = *(ushort *)(iVar1 + 0x18) & 1;
  uVar4 = (uint)uVar6;
  if ((*(char *)(iVar1 + 200) == 'B') || (uVar6 == 0)) {
    uVar5 = (undefined2)((uint)in_stack_00000004 >> 0x10);
    uVar3 = CONCAT22(uVar5,(ushort)(byte)a1);
    uVar7 = 0;
    if ((byte)a1 == 0) {
      if (*(int *)a0 == -0x4f020250) {
        return uVar3;
      }
      if (*(char *)(iVar1 + 200) == 'B') {
        uVar4 = fn_0028c6((uint)&g->f_04c,unaff_EDI);
        uVar5 = (undefined2)(uVar4 >> 0x10);
        uVar7 = extraout_ECX[2];
        a0 = (uint)extraout_ECX;
        uVar4 = extraout_EDX;
      }
      if (uVar4 == 0) {
        if (*(int *)(a0 + 0x18) != 0) {
          *(int *)(a0 + 0x18) = *(int *)(a0 + 0x18) + 0x12cea600;
        }
        if (*(int *)(a0 + 0x1c) != 0) {
          *(int *)(a0 + 0x1c) = *(int *)(a0 + 0x1c) + 0x12cea600;
        }
        if (*(int *)(a0 + 0x20) != 0) {
          *(int *)(a0 + 0x20) = *(int *)(a0 + 0x20) + 0x12cea600;
        }
        if (*(int *)(a0 + 0x24) != 0) {
          *(int *)(a0 + 0x24) = *(int *)(a0 + 0x24) + 0x12cea600;
        }
        if (*(int *)(a0 + 0x28) != 0) {
          *(int *)(a0 + 0x28) = *(int *)(a0 + 0x28) + 0x12cea600;
        }
      }
      *(undefined4 *)a0 = 0xb0fdfdb0;
    }
    else {
      if (*(int *)a0 == -0x24f4f03) {
        return uVar3;
      }
      if (uVar6 == 0) {
        if (*(int *)(a0 + 0x18) != 0) {
          *(int *)(a0 + 0x18) = *(int *)(a0 + 0x18) + -0x12cea600;
        }
        if (*(int *)(a0 + 0x1c) != 0) {
          *(int *)(a0 + 0x1c) = *(int *)(a0 + 0x1c) + -0x12cea600;
        }
        if (*(int *)(a0 + 0x20) != 0) {
          *(int *)(a0 + 0x20) = *(int *)(a0 + 0x20) + -0x12cea600;
        }
        if (*(int *)(a0 + 0x24) != 0) {
          *(int *)(a0 + 0x24) = *(int *)(a0 + 0x24) + -0x12cea600;
        }
        if (*(int *)(a0 + 0x28) != 0) {
          *(int *)(a0 + 0x28) = *(int *)(a0 + 0x28) + -0x12cea600;
        }
      }
      if (*(char *)(iVar1 + 200) == 'B') {
        uVar7 = *(uint *)(a0 + 8);
        uVar4 = fn_0028c6((uint)&g->f_04c,unaff_EDI);
        uVar5 = (undefined2)(uVar4 >> 0x10);
        a0 = (uint)extraout_ECX_00;
      }
      *(undefined4 *)a0 = 0xfdb0b0fd;
    }
    local_20 = (undefined4 *)CONCAT22(uVar5,(ushort)*(byte *)(iVar1 + 200));
    if ((*(byte *)(iVar1 + 200) == 0x42) && ((uVar7 >> 0x10 & 1) == 0)) {
      local_20 = (undefined4 *)(a0 + 0x30);
      local_8 = local_20;
      for (local_c = *(int *)(in_stack_00000004 + 0x13c) - 0x30U >> 3; local_c != 0;
          local_c = local_c - 1) {
        uVar2 = *local_8;
        uStack_11 = (undefined1)((uint)uVar2 >> 0x18);
        uStack_12 = (undefined1)((uint)uVar2 >> 0x10);
        uStack_13 = (undefined1)((uint)uVar2 >> 8);
        local_14 = (undefined1)uVar2;
        local_18 = CONCAT13(local_14,CONCAT12(uStack_13,CONCAT11(uStack_12,uStack_11)));
        *local_8 = local_18;
        uVar2 = local_8[1];
        uStack_19 = (undefined1)((uint)uVar2 >> 0x18);
        uStack_1a = (undefined1)((uint)uVar2 >> 0x10);
        uStack_1b = (undefined1)((uint)uVar2 >> 8);
        local_1c = (undefined1)uVar2;
        local_20 = (undefined4 *)
                   CONCAT13(local_1c,CONCAT12(uStack_1b,CONCAT11(uStack_1a,uStack_19)));
        local_8[1] = local_20;
        local_8 = local_8 + 2;
      }
    }
  }
  return (uint)local_20;
}

/* ----------------------------------------------------------------------
 * fn_0037d2   @0x0037d2, 55 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_003068, fn_00344a, fn_003944
 */
 /* WARNING: Unknown calling convention */
uint fn_0037d2(a0, a1)
    uint a0;
    uint a1;
{
  int iVar1;
  int in_stack_00000004;
  
  iVar1 = *(int *)(*(int *)(in_stack_00000004 + 4) + 0x20);
  *(undefined4 *)(a0 + 0xc) = *(undefined4 *)(in_stack_00000004 + 0x13c);
  *(undefined4 *)(a0 + 4) = *(undefined4 *)(iVar1 + 0xdc);
  *(uint *)(iVar1 + 0xdc) = a0;
  return 0;
}

/* ----------------------------------------------------------------------
 * fn_00380a   @0x00380a, 313 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0000ea
 * calls: fn_0005ee, fn_003294, fn_00351a, fn_00352c, fn_0090f8
 */
 /* WARNING: Unknown calling convention */
uint fn_00380a(a0)
    uint a0;
{
  int iVar1;
  uint uVar2;
  uint a2;
  uint a0_00;
  uint extraout_EDX;
  uint a0_01;
  uint unaff_EDI;
  undefined4 local_10;
  uint *local_c;
  uint local_8;
  
  local_10 = 1;
  uVar2 = fn_003294((uint)&local_c,unaff_EDI);
  if ((uVar2 == 0) &&
     (uVar2 = fn_0005ee((uint)&local_10,*(int *)(a2 + 0x84) + 1,a2,unaff_EDI), uVar2 == 0)) {
    fn_0090f8(0);
    a0_01 = **(uint **)(a0_00 + 0x70);
    iVar1 = *(int *)(a0_00 + 0x13c);
    *(undefined4 *)((int)local_c + iVar1 + 0x30) = *(undefined4 *)(a0_01 + 0x10);
    *(undefined4 *)((int)local_c + iVar1 + 0x34) = *(undefined4 *)(a0_00 + 0xc0);
    *(uint *)(a0_01 + *(int *)(a0_00 + 0x13c) + 0x34) = local_8;
    *(uint *)(*(int *)(a0_00 + 0x70) + *(int *)(a0_00 + 0x13c) + 0x30) = local_8;
    *local_c = a0_01;
    local_c[1] = *(uint *)(a0_00 + 0x70);
    *(uint **)(a0_01 + 4) = local_c;
    **(undefined4 **)(a0_00 + 0x70) = local_c;
    local_c[0xe] = 0xfdb0b0fd;
    local_c[2] = *(int *)(a0_01 + 8) + *(int *)(a0_01 + 0xc);
    local_c[3] = 0;
    local_c[4] = local_8;
    if (((a0_01 == *(uint *)(a0_00 + 0x70)) ||
        (uVar2 = fn_00351a(a0_00), a0_01 = extraout_EDX, uVar2 == 0)) &&
       ((uVar2 = fn_00352c(a0_01,unaff_EDI), uVar2 == 0 &&
        (uVar2 = fn_00352c((uint)local_c,unaff_EDI), uVar2 == 0)))) {
      uVar2 = 0;
    }
  }
  return uVar2;
}
