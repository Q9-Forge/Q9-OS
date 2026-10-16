/*
 * rbf_06.c - rbf part 6
 *
 * Original address range: 0x005dee - 0x0073d0 of the V4.9 kernel module.
 * Functions 0x5dee to 0x6f10 (support routines)
 * Generated from the Ghidra export; see tools/gen_src.py. Hand edits belong in hand/*.inc.
 */
#include "kernel.h"

/* ----------------------------------------------------------------------
 * fn_005dee   @0x005dee, 24 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0054b8
 * calls: os_i_close
 */
 /* WARNING: Unknown calling convention */
uint fn_005dee(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  uint extraout_ECX;
  
  uVar1 = os_i_close(*(uint *)(a0 + 0x12));
  if (extraout_ECX != 0) {
    uVar1 = extraout_ECX;
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_005e06   @0x005e06, 80 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0054b8, fn_006f10
 * calls: fn_0017f6, fn_006a22, fn_0076fa, fn_0079d2
 */
 /* WARNING: Unknown calling convention */
uint fn_005e06(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  uint extraout_ECX;
  uint a0_00;
  uint a0_01;
  uint in_EDX;
  undefined4 extraout_EDX;
  
  if ((*(int *)(a0 + 0x6c) != 0) && (uVar1 = fn_0017f6(a0), a0 = extraout_ECX, uVar1 != 0)) {
    return uVar1;
  }
  uVar1 = fn_0079d2(a0);
  if (uVar1 == 0) {
    *(undefined4 *)(a0_00 + 0xc0) = extraout_EDX;
    uVar1 = fn_0076fa(a0_00);
    if (uVar1 == 0) {
      fn_006a22(a0_01,in_EDX);
      uVar1 = 0;
    }
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_005e56   @0x005e56, 58 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0058f4, rbf_detach
 * calls: os_f_srtmem_8db8
 */
 /* WARNING: Unknown calling convention */
uint fn_005e56(a0)
    uint a0;
{
  uint uVar1;
  uint extraout_ECX;
  uint in_EDX;
  int extraout_EDX;
  int iVar2;
  
  iVar2 = *(int *)(a0 + 0x81c);
  while (iVar2 != 0) {
    os_f_srtmem_8db8(*(int *)(a0 + 0x80c) + 0x20,in_EDX);
    a0 = extraout_ECX;
    iVar2 = extraout_EDX;
  }
  uVar1 = os_f_srtmem_8db8(0x870,in_EDX);
  return uVar1;
}

/* ----------------------------------------------------------------------
 * rbf_attach   @0x005e90, 272 bytes
 * RBF entry point #1: attach a device to the file manager (device init) [file manager entry table]
 * called from: data@00009124
 */
 /* WARNING: Unknown calling convention */
 /* RBF entry point #1: attach a device to the file manager (device init) [file manager entry table] */
uint rbf_attach(a0, a1)
    uint a0;
    uint a1;
{
  void *pvVar1;
  uint uVar2;
  undefined8 uVar3;
  DevEntry *dev;
  
  pvVar1 = dev->v_lu_stat;
  *(short *)((int)pvVar1 + 0xc0) = *(short *)((int)pvVar1 + 0xc0) + 1;
  if (*(short *)((int)pvVar1 + 0xc0) == 1) {
    *(undefined4 *)((int)pvVar1 + 0xc4) = 0;
    *(int *)((int)pvVar1 + 0x60) = (int)pvVar1 + 0x60;
    *(undefined4 *)((int)pvVar1 + 100) = 0;
    *(int *)((int)pvVar1 + 0x6c) = (int)pvVar1 + 0x14;
    *(int *)((int)pvVar1 + 0x68) = (int)pvVar1 + 0x14;
    *(int *)((int)pvVar1 + 0x74) = (int)pvVar1 + 0x74;
    *(undefined4 *)((int)pvVar1 + 0x78) = 0;
    *(int *)((int)pvVar1 + 0x80) = (int)pvVar1 + 0x28;
    *(int *)((int)pvVar1 + 0x7c) = (int)pvVar1 + 0x28;
    *(int *)((int)pvVar1 + 0xe0) = (int)pvVar1 + 0xe0;
    *(undefined4 *)((int)pvVar1 + 0xe4) = 0;
    *(int *)((int)pvVar1 + 0xec) = (int)pvVar1 + 0x94;
    *(int *)((int)pvVar1 + 0xe8) = (int)pvVar1 + 0x94;
    *(int *)((int)pvVar1 + 0xcc) = (int)pvVar1 + 0xcc;
    *(undefined4 *)((int)pvVar1 + 0xd0) = 0;
    *(int *)((int)pvVar1 + 0xd8) = (int)pvVar1 + 0x80;
    *(int *)((int)pvVar1 + 0xd4) = (int)pvVar1 + 0x80;
    *(undefined4 *)((int)pvVar1 + 0xac) = 0;
    *(undefined4 *)((int)pvVar1 + 0xf0) = 0;
    *(undefined4 *)((int)pvVar1 + 0xdc) = 0;
    *(int *)((int)pvVar1 + 0xb0) = (int)pvVar1 + 0xb0;
    *(undefined4 *)((int)pvVar1 + 0xb4) = 0;
    *(int *)((int)pvVar1 + 0xbc) = (int)pvVar1 + 100;
    *(int *)((int)pvVar1 + 0xb8) = (int)pvVar1 + 100;
    uVar3 = (**(code **)((int)dev->v_dr_stat + 4))();
    uVar2 = (uint)uVar3;
    *(uint *)((int)((ulonglong)uVar3 >> 0x20) + 4) = uVar2;
  }
  else {
    uVar2 = 0;
  }
  return uVar2;
}

/* ----------------------------------------------------------------------
 * rbf_detach   @0x005fa0, 180 bytes
 * RBF entry point #6: detach the device [file manager entry table]
 * called from: data@00009138
 * calls: fn_001332, fn_005e56, os_f_srtmem_8db8
 */
 /* WARNING: Unknown calling convention */
 /* RBF entry point #6: detach the device [file manager entry table] */
uint rbf_detach(a0, a1)
    uint a0;
    uint a1;
{
  int iVar1;
  uint uVar2;
  void *extraout_ECX;
  void *extraout_ECX_00;
  void *extraout_ECX_01;
  void *a0_00;
  int iVar3;
  uint unaff_EDI;
  DevEntry *dev;
  
  a0_00 = dev->v_lu_stat;
  *(short *)((int)a0_00 + 0xc0U) = *(short *)((int)a0_00 + 0xc0U) + -1;
  if (*(short *)((int)a0_00 + 0xc0) == 0) {
    if (*(int *)((int)a0_00 + 0xac) != 0) {
      fn_005e56(*(uint *)((int)a0_00 + 0xac));
      a0_00 = extraout_ECX;
    }
    iVar3 = *(int *)((int)a0_00 + 0xf0);
    while (iVar3 != 0) {
      iVar1 = *(int *)(iVar3 + 0x24);
      os_f_srtmem_8db8(*(int *)(iVar3 + 0xc) + 0x2c,unaff_EDI);
      a0_00 = extraout_ECX_00;
      iVar3 = iVar1;
    }
    iVar3 = *(int *)((int)a0_00 + 0xdc);
    while (iVar3 != 0) {
      iVar1 = *(int *)(iVar3 + 4);
      os_f_srtmem_8db8(*(int *)(iVar3 + 0xc) + 0x38,unaff_EDI);
      a0_00 = extraout_ECX_01;
      iVar3 = iVar1;
    }
    if (*(int *)((int)a0_00 + 0x84) != 0) {
      fn_001332((uint)a0_00);
    }
    uVar2 = (**(code **)((int)dev->v_dr_stat + 0x18))();
    *(uint *)(a0 + 4) = uVar2;
  }
  else {
    uVar2 = 0;
  }
  return uVar2;
}

/* ----------------------------------------------------------------------
 * fn_006054   @0x006054, 265 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0000ea, fn_003068, fn_003a26, fn_003f8c, rbf_write, fn_004694, fn_0046e4, fn_00473c
 * calls: fn_00909f, fn_0090b3, fn_0090c8, fn_0090dc
 */
 /* WARNING: Unknown calling convention */
uint fn_006054(a0, a1, a2)
    uint a0;
    uint a1;
    uint a2;
{
  undefined2 *a0_00;
  int extraout_ECX;
  int extraout_ECX_00;
  int iVar1;
  undefined1 *puVar2;
  uint uVar3;
  undefined2 *in_stack_00000004;
  uint local_c;
  undefined2 *local_8;
  
  local_c = a1;
  local_8 = in_stack_00000004;
  if ((a1 != 0) && ((undefined2 *)a0 != in_stack_00000004)) {
    if ((a0 < in_stack_00000004) && (in_stack_00000004 < (undefined2 *)(a0 + a1))) {
      a0 = fn_0090dc(a0 + a1);
    }
    else if ((((a0 ^ (uint)in_stack_00000004) & 1) == 0) && (0xf < a1)) {
      a0_00 = (undefined2 *)a0;
      if ((a0 & 1) != 0) {
        a0_00 = (undefined2 *)(a0 + 1);
        local_8 = (undefined2 *)((int)in_stack_00000004 + 1);
        *(undefined1 *)in_stack_00000004 = *(undefined1 *)a0;
        local_c = a1 - 1;
      }
      if (((a0 ^ (uint)in_stack_00000004) >> 1 & 1) == 0) {
        if (((uint)a0_00 >> 1 & 1) != 0) {
          *local_8 = *a0_00;
          local_8 = local_8 + 1;
          a0_00 = a0_00 + 1;
          local_c = local_c - 2;
        }
        fn_0090c8((uint)a0_00);
        a0 = local_c & 3;
        iVar1 = extraout_ECX_00;
        uVar3 = a0;
      }
      else {
        a0 = fn_0090b3((uint)a0_00);
        iVar1 = extraout_ECX;
        uVar3 = local_c & 1;
      }
      if (uVar3 != 0) {
        a0 = local_c - uVar3;
        puVar2 = (undefined1 *)(iVar1 + a0);
        local_8 = (undefined2 *)((int)local_8 + a0);
        do {
          a0 = CONCAT31((int3)(a0 >> 8),*puVar2);
          *(undefined1 *)local_8 = *puVar2;
          uVar3 = uVar3 - 1;
          puVar2 = puVar2 + 1;
          local_8 = (undefined2 *)((int)local_8 + 1);
        } while (uVar3 != 0);
      }
    }
    else {
      a0 = fn_00909f(a0);
    }
  }
  return a0;
}

/* ----------------------------------------------------------------------
 * rbf_create   @0x00615e, 53 bytes
 * RBF entry point #4: create and open a file [file manager entry table]
 * called from: data@00009130
 * calls: fn_006194
 */
 /* WARNING: Unknown calling convention */
 /* RBF entry point #4: create and open a file [file manager entry table] */
uint rbf_create(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  int extraout_ECX;
  uint in_EDX;
  PathDesc *pd;
  void *local_8;
  
  local_8 = pd->pd_plbuf;
  uVar1 = fn_006194((uint)&local_8,*(uint *)(a0 + 0x16),*(uint *)(a0 + 0x1a),1,(uint)pd,in_EDX);
  *(uint *)(extraout_ECX + 4) = uVar1;
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_006194   @0x006194, 1347 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: rbf_create, fn_007518
 * calls: fn_0000c0, fn_0005ee, fn_000a76, fn_0017f6, fn_001a00, fn_001af2, fn_001b52, fn_002bdc
 */
 /* WARNING: Unknown calling convention */
uint fn_006194(a0, a1, a2, a3, a4, a5)
    uint a0;
    uint a1;
    uint a2;
    uint a3;
    uint a4;
    uint a5;
{
  uint uVar1;
  uint uVar2;
  int iVar3;
  int extraout_ECX;
  int extraout_ECX_00;
  uint a0_00;
  uint a0_01;
  uint a2_00;
  uint a0_02;
  uint a0_03;
  uint a0_04;
  uint a0_05;
  uint a0_06;
  uint extraout_EDX;
  uint a0_07;
  uint a0_08;
  uint extraout_EDX_00;
  uint extraout_EDX_01;
  uint a0_09;
  int extraout_EDX_02;
  int extraout_EDX_03;
  uint a1_00;
  uint extraout_EDX_04;
  uint a0_10;
  uint a0_11;
  uint a0_12;
  uint extraout_EDX_05;
  uint a0_13;
  uint a0_14;
  uint extraout_EDX_06;
  uint a1_01;
  uint a0_15;
  uint unaff_EDI;
  ushort in_stack_00000004;
  uint *a1_02;
  uint a1_03;
  uint local_40;
  char local_3a;
  uint local_34;
  uint local_30;
  uint local_2c;
  undefined1 local_28 [4];
  uint local_24;
  uint local_20;
  uint local_1c;
  uint *local_18;
  uint local_14;
  uint local_10;
  uint local_c;
  uint local_8;
  
  local_14 = a1;
  local_1c = a2;
  local_10 = a3;
  local_c = 0;
  local_18 = (uint *)a0;
  fn_0090f8(0);
  fn_0066d8(a0_00);
  if (*(short *)(*(int *)(a0_01 + 4) + 0x56) == 0x4143) {
    return 0xf2;
  }
  if ((in_stack_00000004 & 0x78) != 0) {
    return 0xcb;
  }
  if ((local_14 & 0xffffb888) != 0) {
    return 0xe1;
  }
  local_40 = *local_18;
  a1_02 = &local_40;
  uVar1 = fn_0081fa(a0_01,(uint)a1_02,unaff_EDI);
  if (uVar1 == 0) {
    local_8 = *(int *)(a2_00 + 0x70) + 0x38;
    if (((in_stack_00000004 & 0x400) != 0) || (*(int *)(a2_00 + 0xc0) == 0)) {
      uVar1 = fn_00673a(a2_00,unaff_EDI);
      return uVar1;
    }
    if ((in_stack_00000004 & 0x80) != 0) {
      uVar1 = fn_00673a(a2_00,unaff_EDI);
      return uVar1;
    }
    uVar1 = fn_008660(a2_00,CONCAT22((short)((uint)a1_02 >> 0x10),0x4002),unaff_EDI);
    if (uVar1 != 0) {
      uVar1 = fn_00673a(a0_02,unaff_EDI);
      return uVar1;
    }
    if ((in_stack_00000004 & 0x2000) != 0) {
      local_c = local_1c;
    }
    if (*(uint *)(a0_02 + 0x7c) < local_c) {
      uVar2 = fn_004464(local_c,unaff_EDI);
      uVar1 = a0_03;
      if (uVar2 != 0) {
        uVar1 = fn_00673a(a0_03,unaff_EDI);
        return uVar1;
      }
    }
    else {
      uVar1 = a0_02;
      if (local_c < *(uint *)(a0_02 + 0x7c)) {
        *(uint *)(a0_02 + 0x7c) = local_c;
        uVar2 = fn_000a76(a0_02);
        uVar1 = a0_04;
        if (uVar2 != 0) {
          uVar1 = fn_00673a(a0_04,unaff_EDI);
          return uVar1;
        }
      }
    }
    if ((((in_stack_00000004 & 0x200) != 0) && ((*(byte *)(*(int *)(uVar1 + 0x70) + 0x42) & 1) != 0)
        ) && (*(uint *)(uVar1 + 0x88) < local_1c)) {
      uVar1 = fn_00673a(uVar1,unaff_EDI);
      return uVar1;
    }
    fn_00676e(uVar1,unaff_EDI);
    uVar1 = fn_0034ee(a0_05);
    if (uVar1 != 0) {
      uVar1 = fn_00673a(a0_06,unaff_EDI);
      return uVar1;
    }
    fn_007b6a(2,unaff_EDI);
    uVar1 = extraout_EDX;
  }
  else {
    if ((uVar1 != 0xd8) || (local_3a == '/')) {
      uVar1 = fn_00673a(a2_00,unaff_EDI);
      return uVar1;
    }
    if ((in_stack_00000004 & 0x80) != 0) {
      uVar1 = fn_00673a(a2_00,unaff_EDI);
      return uVar1;
    }
    *local_18 = local_40;
    if ((in_stack_00000004 & 0x2000) != 0) {
      local_c = local_1c;
    }
    if ((local_c < *(int *)(a2_00 + 0x13c) - 0x30U) && (local_10 != 0)) {
      local_c = 1;
    }
    else {
      local_10 = 0;
      local_c = (local_c + *(int *)(a2_00 + 0x13c) + -1 >> (*(byte *)(a2_00 + 0x67) & 0x1f)) + 1;
    }
    local_34 = local_c;
    uVar1 = fn_0005ee((uint)&local_34,0,a2_00,unaff_EDI);
    if (uVar1 != 0) {
      uVar1 = fn_00673a(a0_07,unaff_EDI);
      return uVar1;
    }
    if ((((in_stack_00000004 & 0x200) != 0) && (local_10 == 0)) && (local_34 < local_c)) {
      uVar1 = fn_0067ea(a0_07,local_34,local_30,unaff_EDI);
      return uVar1;
    }
    uVar2 = os_f_prsnam(local_40,(uint)local_28,(uint)&local_24,unaff_EDI);
    uVar1 = a0_08;
    if (((uVar2 != 0) ||
        (uVar2 = fn_006894(a0_08,local_24,local_2c,unaff_EDI), uVar1 = extraout_EDX_00,
        uVar2 != 0)) ||
       (uVar2 = fn_001a00((uint)&local_20,unaff_EDI), uVar1 = extraout_EDX_01, uVar2 != 0)) {
      uVar1 = fn_0067ea(uVar1,local_34,local_30,unaff_EDI);
      return uVar1;
    }
    uVar1 = *(uint *)(local_20 + 0x28);
    *(uint *)(extraout_EDX_01 + 0x60) = uVar1;
    uVar2 = *(uint *)(extraout_EDX_01 + 0x13c);
    local_8 = uVar1;
    fn_0090f8(0);
    fn_00676e(a0_09,uVar2);
    *(undefined2 *)(local_8 + 0x12) = 1;
    if (local_10 == 0) {
      if (1 < local_c) {
        *(uint *)(local_8 + 0x34) = local_34 - 1;
        *(uint *)(local_8 + 0x30) = local_30 + 1;
      }
      iVar3 = local_8 + *(int *)(extraout_EDX_02 + 0x13c);
      *(uint *)(iVar3 + -4) = local_30;
      *(uint *)(iVar3 + -8) = local_30;
    }
    else {
      *(byte *)(local_8 + 10) = *(byte *)(local_8 + 10) | 1;
    }
    a1_03 = CONCAT31((int3)(uVar1 >> 8),1);
    fn_0035d6(local_8,a1_03,unaff_EDI);
    *(undefined4 *)(local_8 + 4) = 0xffffffff;
    uVar1 = *(uint *)(extraout_EDX_03 + 0x13c);
    uVar2 = fn_0000c0(local_8);
    *(uint *)(local_8 + 4) = uVar2;
    uVar1 = fn_002bdc(1,a1_00,uVar1);
    if (uVar1 != 0) {
      lock_release(local_20 + 0x10);
      uVar1 = extraout_EDX_04;
      fn_001b52(local_20,unaff_EDI);
      uVar1 = fn_0067ea(a0_10,local_34,local_30,uVar1);
      return uVar1;
    }
    fn_0035d6(local_8,a1_03 & 0xffffff00,unaff_EDI);
    fn_0017f6(a0_11);
    uVar1 = a0_12;
    if ((*(byte *)(a0_12 + 100) & 1) != 0) {
      fn_00351a(a0_12);
      uVar1 = extraout_EDX_05;
    }
    fn_0079d2(uVar1);
    *(uint *)(a0_13 + 0xc0) = local_30;
    if (extraout_ECX == 0) {
      fn_003068(a0_13,unaff_EDI);
    }
    lock_release(local_20 + 0x10);
    fn_001b52(local_20,unaff_EDI);
    if (extraout_ECX_00 != 0) {
      uVar1 = fn_0067ea(a0_14,local_34,local_30,unaff_EDI);
      return uVar1;
    }
    fn_0076fa(a0_14);
    local_8 = *(int *)(extraout_EDX_06 + 0x70) + 0x38;
    uVar1 = extraout_EDX_06;
  }
  *(undefined1 *)(uVar1 + 0x65) = 4;
  uVar2 = local_8;
  uVar1 = fn_006a22(uVar1,unaff_EDI);
  uVar1 = fn_006ae0(CONCAT22((short)(uVar1 >> 0x10),0x2a),a1_01,uVar2);
  if (uVar1 == 0) {
    *(undefined1 *)(a0_15 + 0x68) = 1;
    uVar1 = fn_001af2(a0_15);
  }
  else {
    uVar1 = fn_0067ea(a0_15,local_34,local_30,unaff_EDI);
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_0066d8   @0x0066d8, 97 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_006194, fn_006b50, fn_006f10, fn_007518
 * calls: fn_002b84
 */
 /* WARNING: Unknown calling convention */
uint fn_0066d8(a0)
    uint a0;
{
  int iVar1;
  uint uVar2;
  int iVar3;
  int extraout_EDX;
  undefined4 *puVar4;
  uint unaff_EDI;
  undefined4 *puVar5;
  undefined1 local_24 [12];
  undefined4 local_18;
  undefined2 local_14;
  undefined1 *local_12;
  undefined1 local_c [4];
  uint local_8;
  
  iVar1 = *(int *)(*(int *)(a0 + 4) + 0x1c);
  puVar4 = (undefined4 *)(iVar1 + *(int *)(iVar1 + 0x24) + 0x18);
  puVar5 = (undefined4 *)(a0 + 0x110);
  for (iVar3 = 0x16; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar5 = *puVar4;
    puVar4 = puVar4 + 1;
    puVar5 = puVar5 + 1;
  }
  local_18 = *(undefined4 *)a0;
  local_12 = local_c;
  local_14 = 0x2d;
  uVar2 = fn_002b84(0,3,a0,(uint)local_24,unaff_EDI);
  if (uVar2 == 0) {
    *(uint *)(extraout_EDX + 0x13c) = local_8;
    uVar2 = local_8;
  }
  return uVar2;
}

/* ----------------------------------------------------------------------
 * fn_00673a   @0x00673a, 52 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_006194, fn_0067ea, fn_006b50, fn_006de4, fn_006f10, fn_007518
 * calls: fn_006ae0, fn_0079d2
 */
 /* WARNING: Unknown calling convention */
uint fn_00673a(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  uint extraout_ECX;
  uint in_EDX;
  uint extraout_EDX;
  
  if (*(char *)(a0 + 0x68) != '\0') {
    fn_006ae0(CONCAT22((short)(a0 >> 0x10),0x2b),a0,in_EDX);
    a0 = extraout_EDX;
  }
  uVar1 = fn_0079d2(a0);
  if (extraout_ECX != 0) {
    uVar1 = extraout_ECX;
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_00676e   @0x00676e, 123 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_006194
 * calls: os_f_time
 */
 /* WARNING: Unknown calling convention */
uint fn_00676e(a0, a1)
    uint a0;
    uint a1;
{
  int iVar1;
  uint in_EAX;
  uint uVar2;
  int extraout_ECX;
  undefined4 uVar3;
  undefined4 *in_stack_00000004;
  
  iVar1 = *(int *)(*(int *)(a0 + 4) + 0x20);
  if ((*(char *)(iVar1 + 200) == 'B') || ((*(ushort *)(iVar1 + 0x18) & 1) == 0)) {
    uVar3 = 0xb0fdfdb0;
  }
  else {
    uVar3 = 0xfdb0b0fd;
  }
  *in_stack_00000004 = uVar3;
  in_stack_00000004[2] = in_EAX & 0xffff7fff;
  *(undefined2 *)(in_stack_00000004 + 3) = 0;
  *(undefined2 *)((int)in_stack_00000004 + 0xe) = *(undefined2 *)(a0 + 8);
  *(undefined2 *)(in_stack_00000004 + 4) = *(undefined2 *)(a0 + 10);
  uVar2 = os_f_time();
  *(uint *)(extraout_ECX + 0x20) = uVar2;
  *(uint *)(extraout_ECX + 0x24) = uVar2;
  *(uint *)(extraout_ECX + 0x1c) = uVar2;
  *(uint *)(extraout_ECX + 0x18) = uVar2;
  return uVar2;
}

/* ----------------------------------------------------------------------
 * fn_0067ea   @0x0067ea, 169 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_006194, fn_007518
 * calls: fn_000770, fn_0017f6, fn_006054, fn_00673a, fn_0076fa, fn_0079d2, fn_008776
 */
 /* WARNING: Unknown calling convention */
uint fn_0067ea(a0, a1, a2, a3)
    uint a0;
    uint a1;
    uint a2;
    uint a3;
{
  uint *puVar1;
  uint uVar2;
  uint uVar3;
  uint a0_00;
  uint a0_01;
  uint a0_02;
  uint extraout_EDX;
  uint a0_03;
  uint extraout_EDX_00;
  uint unaff_EDI;
  uint *local_8;
  
  *(uint *)(a0 + 0x88) = a1 << (*(byte *)(a0 + 0x67) & 0x1f);
  *(uint *)(a0 + 0x84) = a2;
  fn_000770(a0);
  uVar2 = fn_0079d2(a0_00);
  uVar3 = a0_01;
  if (uVar2 == 0) {
    *(undefined4 *)(a0_01 + 0xc0) = *(undefined4 *)(a0_01 + 0xc4);
    uVar2 = fn_0076fa(a0_01);
    uVar3 = a0_02;
    if (uVar2 == 0) {
      *(undefined4 *)(a0_02 + 0x78) = *(undefined4 *)(a0_02 + 200);
      uVar2 = fn_008776(a0_02,unaff_EDI);
      uVar3 = extraout_EDX;
      if (uVar2 == 0) {
        fn_006054((uint)local_8,0x2a,unaff_EDI);
        *local_8 = 0;
        puVar1 = (uint *)(*(int *)(a0_03 + 0x6c) + 8);
        *puVar1 = *puVar1 | 1;
        fn_0017f6(a0_03);
        uVar3 = extraout_EDX_00;
      }
    }
  }
  uVar3 = fn_00673a(uVar3,unaff_EDI);
  return uVar3;
}

/* ----------------------------------------------------------------------
 * fn_006894   @0x006894, 398 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0054b8, fn_006194
 * calls: fn_0017f6, fn_00351a, fn_004464, fn_006054, os_f_time, fn_00789c, fn_008776, fn_008840
 */
 /* WARNING: Unknown calling convention */
uint fn_006894(a0, a1, a2, a3)
    uint a0;
    uint a1;
    uint a2;
    uint a3;
{
  uint *puVar1;
  uint uVar2;
  int extraout_ECX;
  undefined4 extraout_EDX;
  uint unaff_EDI;
  uint uVar3;
  undefined4 in_stack_00000004;
  char *a1_00;
  char *local_14;
  
  uVar3 = 0;
  if (*(int *)(a0 + 0x6c) != 0) {
    fn_0017f6(a0);
  }
  *(undefined4 *)(a0 + 0x78) = *(undefined4 *)(a0 + 200);
  uVar2 = fn_008776(a0,unaff_EDI);
  while( true ) {
    if ((uVar2 == 0) && (*local_14 == '\0')) goto LAB_0000699b;
    if (uVar2 != 0) break;
    uVar2 = fn_008840(a0,unaff_EDI);
  }
  if (uVar2 == 0xd3) {
    uVar3 = *(uint *)(a0 + 0x78) + 0x40;
    if (uVar3 < *(uint *)(a0 + 0x78)) {
      uVar2 = 0xf8;
    }
    else {
      uVar2 = fn_004464(uVar3,unaff_EDI);
      if (uVar2 == 0) {
        *(undefined4 *)(*(int *)(a0 + 0x70) + 0x4c) = extraout_EDX;
        *(byte *)(a0 + 100) = *(byte *)(a0 + 100) | 1;
        uVar2 = fn_00351a(a0);
        if ((uVar2 == 0) && (uVar2 = fn_008776(a0,unaff_EDI), uVar2 == 0)) {
          *(byte *)(a0 + 100) = *(byte *)(a0 + 100) | 1;
          *(undefined4 *)(*(int *)(a0 + 0x70) + 0x4c) = *(undefined4 *)(a0 + 0x7c);
          uVar2 = os_f_time();
          *(uint *)(*(int *)(a0 + 0x70) + 0x5c) = uVar2;
LAB_0000699b:
          if (uVar3 == 0) {
            fn_00789c(*(int *)(a0 + 0x70) + 0x28);
            uVar3 = os_f_time();
            *(uint *)(extraout_ECX + 0x24) = uVar3;
            *(uint *)(extraout_ECX + 0x20) = uVar3;
            *(uint *)(extraout_ECX + 0x1c) = uVar3;
            *(byte *)(a0 + 100) = *(byte *)(a0 + 100) | 1;
            lock_release(*(int *)(a0 + 0x70) + 0x28);
          }
          uVar3 = 0x40;
          fn_0090f8(0);
          a1_00 = local_14;
          fn_006054(a1,a2,uVar3);
          local_14[a2] = '\0';
          *(undefined4 *)(local_14 + 0x3c) = in_stack_00000004;
          fn_008878((uint)local_14,(uint)a1_00);
          puVar1 = (uint *)(*(int *)(a0 + 0x6c) + 8);
          *puVar1 = *puVar1 | 1;
          *(byte *)(a0 + 100) = *(byte *)(a0 + 100) | 1;
          uVar2 = 0;
        }
      }
    }
  }
  return uVar2;
}

/* ----------------------------------------------------------------------
 * fn_006a22   @0x006a22, 189 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_00517a, fn_005e06, fn_006194, fn_006b50, fn_006f10, fn_0081fa
 * calls: fn_00789c, lock_release
 */
 /* WARNING: Unknown calling convention */
uint fn_006a22(a0, a1)
    uint a0;
    uint a1;
{
  int iVar1;
  uint uVar2;
  int extraout_EDX;
  int in_stack_00000004;
  
  *(undefined4 *)(a0 + 0x80) = 0;
  *(undefined4 *)(a0 + 0x78) = 0;
  *(uint *)(a0 + 0xd0) = *(uint *)(in_stack_00000004 + 8) & 0xfff;
  if ((*(byte *)(in_stack_00000004 + 10) & 1) == 0) {
    *(undefined4 *)(a0 + 0x84) = **(undefined4 **)(a0 + 0x9c);
    *(undefined4 *)(a0 + 0x88) = (*(undefined4 **)(a0 + 0x9c))[1] << (*(byte *)(a0 + 0x67) & 0x1f);
  }
  else {
    *(undefined4 *)(a0 + 0x84) = 0;
    *(undefined4 *)(a0 + 0x88) = 0;
  }
  if (a0 == *(uint *)(a0 + 0x98)) {
    uVar2 = *(uint *)(a0 + 0x70);
    *(undefined4 *)(a0 + 0x7c) = *(undefined4 *)(uVar2 + 0x4c);
  }
  else {
    fn_00789c(*(uint *)(a0 + 0x90));
    for (iVar1 = *(int *)(extraout_EDX + 0x98);
        (*(char *)(iVar1 + 0x68) == '\0' && (iVar1 != extraout_EDX)); iVar1 = *(int *)(iVar1 + 0x98)
        ) {
    }
    if (iVar1 == extraout_EDX) {
      *(undefined4 *)(extraout_EDX + 0x7c) = *(undefined4 *)(*(int *)(extraout_EDX + 0x70) + 0x4c);
    }
    else {
      *(undefined4 *)(extraout_EDX + 0x7c) = *(undefined4 *)(iVar1 + 0x7c);
    }
    uVar2 = lock_release(*(uint *)(extraout_EDX + 0x90));
  }
  return uVar2;
}

/* ----------------------------------------------------------------------
 * fn_006ae0   @0x006ae0, 65 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_006194, fn_00673a, fn_006b50
 * calls: fn_002b84
 */
 /* WARNING: Unknown calling convention */
uint fn_006ae0(a0, a1, a2)
    uint a0;
    uint a1;
    uint a2;
{
  uint uVar1;
  uint unaff_EDI;
  undefined1 local_1c [12];
  undefined4 local_10;
  undefined2 local_c;
  
  local_10 = *(undefined4 *)a1;
  local_c = (undefined2)a0;
  uVar1 = fn_002b84(0,4,a1,(uint)local_1c,unaff_EDI);
  if (uVar1 == 0xd0) {
    uVar1 = 0;
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * rbf_open   @0x006b22, 45 bytes
 * RBF entry point #10: open a path [file manager entry table]
 * called from: data@00009148
 * calls: fn_006b50
 */
 /* WARNING: Unknown calling convention */
 /* RBF entry point #10: open a path [file manager entry table] */
uint rbf_open(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  int extraout_ECX;
  uint in_EDX;
  PathDesc *pd;
  void *local_8;
  
  local_8 = pd->pd_plbuf;
  uVar1 = fn_006b50((uint)&local_8,(uint)pd,in_EDX);
  *(uint *)(extraout_ECX + 4) = uVar1;
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_006b50   @0x006b50, 544 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: rbf_open, rbf_chgdir
 * calls: os_f_id, fn_0066d8, fn_00673a, fn_006a22, fn_006ae0, os_f_time, fn_00789c, fn_0081fa
 */
 /* WARNING: Unknown calling convention */
uint fn_006b50(a0, a1, a2)
    uint a0;
    uint a1;
    uint a2;
{
  uint uVar1;
  undefined2 uVar2;
  undefined4 *extraout_ECX;
  undefined4 *extraout_ECX_00;
  int extraout_ECX_01;
  int extraout_ECX_02;
  uint unaff_EDI;
  uint uVar3;
  ushort in_stack_00000004;
  undefined4 *a1_00;
  undefined4 local_14;
  ushort local_10;
  ushort local_8;
  
  local_8 = in_stack_00000004;
  fn_0090f8(0);
  fn_0066d8(a1);
  if ((local_8 & 0x2678) == 0) {
    local_14 = *extraout_ECX;
    local_10 = local_8;
    a1_00 = &local_14;
    uVar1 = fn_0081fa(a1,(uint)a1_00,unaff_EDI);
    if (uVar1 == 0) {
      *extraout_ECX_00 = local_14;
      if (*(int *)(a1 + 0xc0) == 0) {
        if ((local_8 & 0x9000) != 0) {
          uVar1 = fn_00673a(a1,unaff_EDI);
          return uVar1;
        }
        uVar1 = os_f_id();
        if (uVar1 >> 0x10 == 0) {
          uVar1 = *(uint *)(extraout_ECX_01 + 8);
        }
        else {
          if ((local_8 & 2) != 0) {
            uVar1 = fn_00673a(a1,unaff_EDI);
            return uVar1;
          }
          uVar1 = 1;
        }
        if ((local_8 & 0x80) == 0) {
          uVar3 = -1 << (0x20 - *(byte *)(a1 + 0x67) & 0x1f);
          if ((uVar1 & uVar3) != 0) {
            uVar1 = ~uVar3;
          }
          uVar1 = uVar1 << (*(byte *)(a1 + 0x67) & 0x1f);
        }
        else {
          *(byte *)(a1 + 0x6a) = *(byte *)(a1 + 0x6a) | 1;
        }
        uVar2 = 0;
        *(undefined4 *)(a1 + 0x78) = 0;
        *(undefined4 *)(a1 + 0x84) = 0;
        *(undefined4 *)(a1 + 0x80) = 0;
        *(uint *)(a1 + 0x7c) = uVar1;
        *(uint *)(a1 + 0x88) = uVar1;
      }
      else {
        if ((local_8 & 0x80) != 0) {
          uVar1 = fn_00673a(a1,unaff_EDI);
          return uVar1;
        }
        uVar1 = fn_008660(a1,CONCAT22((short)((uint)a1_00 >> 0x10),local_8),unaff_EDI);
        if (uVar1 != 0) {
          uVar1 = fn_00673a(a1,unaff_EDI);
          return uVar1;
        }
        fn_006a22(a1,unaff_EDI);
        fn_00789c(*(int *)(a1 + 0x70) + 0x28);
        uVar1 = os_f_time();
        *(uint *)(extraout_ECX_02 + 0x1c) = uVar1;
        if (((local_8 & 2) != 0) &&
           (*(undefined4 *)(extraout_ECX_02 + 0x20) = *(undefined4 *)(extraout_ECX_02 + 0x1c),
           (local_8 & 0x1000) != 0)) {
          *(undefined4 *)(a1 + 0x78) = *(undefined4 *)(a1 + 0x7c);
        }
        uVar1 = lock_release(*(int *)(a1 + 0x70) + 0x28);
        uVar2 = (undefined2)(uVar1 >> 0x10);
      }
      uVar1 = fn_006ae0(CONCAT22(uVar2,0x2a),a1,unaff_EDI);
      if (uVar1 == 0) {
        *(undefined1 *)(a1 + 0x68) = 1;
        uVar1 = 0;
      }
      else {
        uVar1 = fn_00673a(a1,unaff_EDI);
      }
    }
    else {
      uVar1 = fn_00673a(a1,unaff_EDI);
    }
  }
  else {
    uVar1 = 0xcb;
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * rbf_chgdir   @0x006d70, 51 bytes
 * RBF entry point #2: change the current data/execution directory [file manager entry table]
 * called from: data@00009128
 * calls: fn_006b50
 */
 /* WARNING: Unknown calling convention */
 /* RBF entry point #2: change the current data/execution directory [file manager entry table] */
uint rbf_chgdir(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  int extraout_ECX;
  uint in_EDX;
  PathDesc *pd;
  void *local_8;
  
  local_8 = pd->pd_plbuf;
  uVar1 = fn_006b50((uint)&local_8,(uint)pd,in_EDX);
  *(uint *)(extraout_ECX + 4) = uVar1;
  return uVar1;
}

/* ----------------------------------------------------------------------
 * os_f_time   @0x006da4, 43 bytes
 * kernel call wrapper: builds the parameter block for F_TIME (code 0x23, system state) and enters the kernel
 * called from: fn_003a26, fn_004464, fn_004dca, fn_0054b8, fn_00676e, fn_006894, fn_006b50, fn_006e96
 */
 /* WARNING: Unknown calling convention */
 /* kernel call wrapper: builds the parameter block for F_TIME (code 0x23, system state) and enters the kernel */
uint os_f_time()
{
  code *pcVar1;
  int extraout_ECX;
  uint local_c;
  
  pcVar1 = (code *)swi(0xff);
  (*pcVar1)(0x23);
  if (*(int *)(extraout_ECX + 4) != 0) {
    local_c = 0;
  }
  return local_c;
}

/* ----------------------------------------------------------------------
 * rbf_close   @0x006dd0, 20 bytes
 * RBF entry point #3: close a path [file manager entry table]
 * called from: data@0000912c
 * calls: fn_006de4
 */
 /* WARNING: Unknown calling convention */
 /* RBF entry point #3: close a path [file manager entry table] */
uint rbf_close(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  int extraout_ECX;
  PathDesc *pd;
  
  uVar1 = fn_006de4((uint)pd);
  *(uint *)(extraout_ECX + 4) = uVar1;
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_006de4   @0x006de4, 178 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: rbf_close
 * calls: fn_000a76, fn_0017f6, fn_0039b0, fn_00673a, fn_006e96
 */
 /* WARNING: Unknown calling convention */
uint fn_006de4(a0)
    uint a0;
{
  uint *puVar1;
  uint uVar2;
  uint a0_00;
  uint a0_01;
  uint extraout_ECX;
  uint extraout_ECX_00;
  uint in_EDX;
  undefined4 local_8;
  
  if (*(short *)(*(int *)(a0 + 4) + 0x56) == 0x4143) {
    puVar1 = (uint *)(*(int *)(a0 + 0x6c) + 8);
    *puVar1 = *puVar1 & 0xfffffffe;
    *(byte *)(a0 + 100) = *(byte *)(a0 + 100) & 0xfe;
    uVar2 = fn_00673a(a0,in_EDX);
  }
  else {
    uVar2 = fn_0017f6(a0);
    if (uVar2 == 0) {
      if (*(short *)(a0_00 + 0x16) == 0) {
        uVar2 = a0_00;
        if (((*(ushort *)(a0_00 + 0x14) & 2) != 0) && (*(int *)(a0_00 + 0xc0) != 0)) {
          local_8 = 1;
          uVar2 = fn_0039b0((uint)&local_8,in_EDX);
          if (uVar2 == 0) {
            fn_006e96(a0_01);
            uVar2 = extraout_ECX;
          }
          else {
            fn_000a76(a0_01);
            uVar2 = extraout_ECX_00;
          }
        }
        uVar2 = fn_00673a(uVar2,in_EDX);
      }
      else {
        uVar2 = 0;
      }
    }
    else {
      uVar2 = fn_00673a(a0_00,in_EDX);
    }
  }
  return uVar2;
}

/* ----------------------------------------------------------------------
 * fn_006e96   @0x006e96, 76 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_006de4, fn_007518
 * calls: fn_00351a, os_f_time, fn_00789c, lock_release
 */
 /* WARNING: Unknown calling convention */
uint fn_006e96(a0)
    uint a0;
{
  uint uVar1;
  int extraout_ECX;
  uint a0_00;
  int extraout_ECX_00;
  uint extraout_EDX;
  
  if (*(int *)(a0 + 0x70) == 0) {
    uVar1 = 0xd3;
  }
  else {
    fn_00789c(*(int *)(a0 + 0x70) + 0x28);
    *(undefined4 *)(*(int *)(extraout_ECX + 0x70) + 0x4c) = *(undefined4 *)(extraout_ECX + 0x7c);
    uVar1 = os_f_time();
    *(uint *)(*(int *)(a0_00 + 0x70) + 0x5c) = uVar1;
    fn_00351a(a0_00);
    lock_release(*(int *)(extraout_ECX_00 + 0x70) + 0x28);
    uVar1 = extraout_EDX;
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * rbf_delete   @0x006ee2, 45 bytes
 * RBF entry point #5: delete a file [file manager entry table]
 * called from: data@00009134
 * calls: fn_006f10
 */
 /* WARNING: Unknown calling convention */
 /* RBF entry point #5: delete a file [file manager entry table] */
uint rbf_delete(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  int extraout_ECX;
  uint in_EDX;
  PathDesc *pd;
  void *local_8;
  
  local_8 = pd->pd_plbuf;
  uVar1 = fn_006f10((uint)&local_8,(uint)pd,in_EDX);
  *(uint *)(extraout_ECX + 4) = uVar1;
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_006f10   @0x006f10, 1216 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: rbf_delete
 * calls: fn_000770, fn_000a76, fn_0017f6, fn_00284a, fn_0034ee, fn_005cfc, fn_005e06, fn_006054
 */
 /* WARNING: Unknown calling convention */
uint fn_006f10(a0, a1, a2)
    uint a0;
    uint a1;
    uint a2;
{
  uint *puVar1;
  short *psVar2;
  int iVar3;
  uint uVar4;
  int extraout_ECX;
  int extraout_ECX_00;
  uint extraout_ECX_01;
  uint extraout_ECX_02;
  uint extraout_ECX_03;
  undefined1 *extraout_EDX;
  uint extraout_EDX_00;
  undefined1 *extraout_EDX_01;
  uint unaff_EDI;
  undefined2 in_stack_00000004;
  uint uVar5;
  char *a1_00;
  undefined2 uVar6;
  undefined4 local_1c;
  undefined2 local_18;
  char *local_10;
  uint local_c;
  undefined2 local_8;
  
  local_8 = in_stack_00000004;
  if (*(short *)(*(int *)(a1 + 4) + 0x56) == 0x4143) {
    return 0xf2;
  }
  uVar6 = (undefined2)(a1 + 0x60 >> 0x10);
  uVar5 = 0x108;
  fn_0090f8(0);
  fn_0066d8(a1);
  local_1c = *(undefined4 *)a0;
  local_18 = local_8;
  uVar5 = fn_0081fa(a1,(uint)&local_1c,uVar5);
  if (uVar5 != 0) {
    if (uVar5 == 0xf3) {
      fn_00673a(a1,unaff_EDI);
      *(undefined4 *)(a1 + 0xc0) = *(undefined4 *)(a1 + 0xc4);
      uVar5 = fn_0076fa(a1);
      if (uVar5 != 0) {
        return uVar5;
      }
      uVar5 = *(int *)(a1 + 0x70) + 0x38;
      fn_006a22(a1,unaff_EDI);
      *(undefined4 *)(a1 + 0x78) = *(undefined4 *)(a1 + 200);
      uVar5 = fn_008776(a1,uVar5);
      if (uVar5 != 0) {
        uVar5 = fn_00673a(a1,unaff_EDI);
        return uVar5;
      }
      fn_006054((uint)local_10,0x2a,unaff_EDI);
      *extraout_EDX = 0;
      puVar1 = (uint *)(*(int *)(a1 + 0x6c) + 8);
      *puVar1 = *puVar1 | 1;
      fn_0017f6(a1);
    }
    uVar5 = fn_00673a(a1,unaff_EDI);
    return uVar5;
  }
  *(undefined4 *)a0 = local_1c;
  if (*(int *)(a1 + 0xc0) == 0) {
    uVar5 = fn_00673a(a1,unaff_EDI);
    return uVar5;
  }
  uVar5 = fn_008660(a1,CONCAT22(uVar6,local_8) | 0x4002,unaff_EDI);
  if (uVar5 != 0) {
    uVar5 = fn_00673a(a1,unaff_EDI);
    return uVar5;
  }
  if ((*(byte *)(extraout_ECX + 9) & 0x80) != 0) {
    if (*(int *)(a1 + 0xc0) == *(int *)(*(int *)(*(int *)(a1 + 4) + 0x20) + 0x28)) {
      uVar5 = fn_00673a(a1,unaff_EDI);
      return uVar5;
    }
    fn_006a22(a1,unaff_EDI);
    if (*(ushort *)(extraout_ECX_00 + 0x12) < 3) {
      uVar5 = fn_005cfc(a1,unaff_EDI);
      if ((uVar5 != 0) || (uVar5 = fn_0073d0(0xffffffff,a1,unaff_EDI), uVar5 != 0)) {
        uVar5 = fn_00673a(a1,unaff_EDI);
        return uVar5;
      }
      *(ushort *)(a1 + 0x14) = *(ushort *)(a1 + 0x14) & 0x7fff;
      psVar2 = (short *)(*(int *)(a1 + 0x70) + 0x4a);
      *psVar2 = *psVar2 + -1;
      *(byte *)(a1 + 100) = *(byte *)(a1 + 100) | 1;
    }
    else {
      uVar5 = fn_008776(a1,unaff_EDI);
      if (uVar5 == 0) {
        do {
          if (((*local_10 == '.') && (local_10[1] == '.')) && (local_10[2] == '\0')) {
            local_c = fn_00284a(*(uint *)(local_10 + 0x3c),unaff_EDI);
            uVar5 = extraout_ECX_01;
            break;
          }
          uVar5 = fn_008840(a1,unaff_EDI);
        } while (uVar5 == 0);
        if (uVar5 == 0xd3) {
          uVar5 = 0xf1;
        }
      }
      if (uVar5 != 0) {
        if (*(int *)(a1 + 0x6c) != 0) {
          fn_0017f6(a1);
        }
        uVar5 = fn_00673a(a1,unaff_EDI);
        return uVar5;
      }
      if (local_c == *(uint *)(a1 + 0xc4)) {
        uVar5 = fn_005e06(a1,unaff_EDI);
        if (uVar5 != 0) {
          uVar5 = fn_00673a(a1,unaff_EDI);
          return uVar5;
        }
        *(undefined4 *)(a1 + 0x78) = 0;
        uVar5 = fn_008776(a1,unaff_EDI);
        if (uVar5 == 0) {
          do {
            if (((*local_10 != '\0') && (*(int *)(a1 + 0x78) != *(int *)(a1 + 200))) &&
               (uVar4 = fn_00284a(*(uint *)(local_10 + 0x3c),unaff_EDI), uVar5 = extraout_ECX_02,
               extraout_EDX_00 == uVar4)) break;
            uVar5 = fn_008840(a1,unaff_EDI);
          } while (uVar5 == 0);
          if (uVar5 == 0xd3) {
            uVar5 = 0xee;
          }
        }
        if ((uVar5 != 0) && (*(int *)(a1 + 0x6c) != 0)) {
          fn_0017f6(a1);
          uVar5 = extraout_ECX_03;
        }
        if ((uVar5 != 0) || (uVar5 = fn_005e06(a1,unaff_EDI), uVar5 != 0)) {
          uVar5 = fn_00673a(a1,unaff_EDI);
          return uVar5;
        }
      }
    }
  }
  uVar5 = fn_005e06(a1,unaff_EDI);
  if (uVar5 != 0) {
    uVar5 = fn_00673a(a1,unaff_EDI);
    return uVar5;
  }
  *(undefined4 *)(a1 + 0x78) = *(undefined4 *)(a1 + 200);
  uVar5 = fn_008776(a1,unaff_EDI);
  if (uVar5 != 0) {
    uVar5 = fn_00673a(a1,unaff_EDI);
    return uVar5;
  }
  a1_00 = local_10 + 1;
  fn_006054((uint)local_10,0x2a,unaff_EDI);
  *extraout_EDX_01 = 0;
  puVar1 = (uint *)(*(int *)(a1 + 0x6c) + 8);
  *puVar1 = *puVar1 | 1;
  fn_0017f6(a1);
  uVar5 = fn_005e06(a1,(uint)a1_00);
  if (uVar5 != 0) {
    uVar5 = fn_00673a(a1,unaff_EDI);
    return uVar5;
  }
  iVar3 = *(int *)(a1 + 0x70);
  if ((*(int *)(iVar3 + 0x38) != -0x24f4f03) && (*(int *)(iVar3 + 0x38) != -0x4f020250)) {
    uVar5 = fn_00673a(a1,unaff_EDI);
    return uVar5;
  }
  *(short *)(iVar3 + 0x4a) = *(short *)(iVar3 + 0x4a) + -1;
  if (*(short *)(iVar3 + 0x4a) == 0) {
    *(undefined4 *)(a1 + 0x7c) = 0;
    *(byte *)(a1 + 100) = *(byte *)(a1 + 100) | 0x80;
    uVar5 = fn_000a76(a1);
    if (uVar5 != 0) {
      uVar5 = fn_00673a(a1,unaff_EDI);
      return uVar5;
    }
    *(undefined4 *)(a1 + 0x88) = *(undefined4 *)(a1 + 0x13c);
    *(undefined4 *)(a1 + 0x84) = *(undefined4 *)(a1 + 0xc0);
    *(byte *)(a1 + 100) = *(byte *)(a1 + 100) & 0xfe;
    uVar5 = fn_000770(a1);
    if (uVar5 != 0) {
      uVar5 = fn_00673a(a1,unaff_EDI);
      return uVar5;
    }
  }
  else {
    uVar5 = fn_0034ee(a1);
    if (uVar5 != 0) {
      uVar5 = fn_00673a(a1,unaff_EDI);
      return uVar5;
    }
  }
  fn_0017f6(a1);
  uVar5 = fn_00673a(a1,unaff_EDI);
  return uVar5;
}
