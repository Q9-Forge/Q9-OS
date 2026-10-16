/*
 * rbf_05.c - rbf part 5
 *
 * Original address range: 0x004bf8 - 0x005dee of the V4.9 kernel module.
 * Functions 0x4bf8 to 0x5cfc (support routines)
 * Generated from the Ghidra export; see tools/gen_src.py. Hand edits belong in hand/*.inc.
 */
#include "kernel.h"

/* ----------------------------------------------------------------------
 * fn_004bf8   @0x004bf8, 171 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * calls: fn_000d72, fn_0029e4, fn_005c60
 */
 /* WARNING: Unknown calling convention */
uint fn_004bf8(a0, a1)
    uint a0;
    uint a1;
{
  int iVar1;
  uint uVar2;
  uint uVar3;
  uint a0_00;
  uint a0_01;
  uint extraout_EDX;
  uint extraout_EDX_00;
  uint unaff_EDI;
  int iVar4;
  int *in_stack_00000004;
  int local_8;
  
  iVar1 = *(int *)(*(int *)(a0 + 4) + 0x20);
  local_8 = 0;
  uVar2 = fn_005c60(0x10,(uint)in_stack_00000004,a0,unaff_EDI);
  if ((uVar2 == 0) && (uVar2 = fn_0029e4(a0_00), uVar2 == 0)) {
    uVar2 = extraout_EDX;
    if ((*(int *)(iVar1 + 0x84) == 0) &&
       (uVar3 = fn_000d72(a0_01,unaff_EDI), uVar2 = extraout_EDX_00, uVar3 != 0)) {
      return uVar3;
    }
    iVar4 = *(int *)(iVar1 + 0x84);
    do {
      uVar3 = *(uint *)(iVar4 + 4);
      local_8 = local_8 + uVar3;
      if (uVar2 < uVar3) {
        uVar2 = uVar3;
      }
      iVar4 = *(int *)(iVar4 + 0xc);
    } while (iVar4 != *(int *)(iVar1 + 0x84));
    in_stack_00000004[1] = (uint)*(ushort *)(iVar1 + 0x14);
    in_stack_00000004[2] = *(int *)(iVar1 + 8);
    *in_stack_00000004 = local_8;
    in_stack_00000004[3] = uVar2;
    uVar2 = 0;
  }
  return uVar2;
}

/* ----------------------------------------------------------------------
 * fn_004ca4   @0x004ca4, 64 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: rbf_getstat, rbf_setstat
 */
 /* WARNING: Unknown calling convention */
uint fn_004ca4(a0, a1, a2, a3)
    uint a0;
    uint a1;
    uint a2;
    uint a3;
{
  uint uVar1;
  
  while( true ) {
    if (*(uint *)(a2 + 4) == 0) {
      return 0xd0;
    }
    if (a0 == *(uint *)a2) break;
    a2 = a2 + 8;
  }
  uVar1 = (**(code **)(a2 + 4))();
  return uVar1;
}

/* ----------------------------------------------------------------------
 * rbf_setstat   @0x004ce4, 70 bytes
 * RBF entry point #14: set status (I$SetStt) [file manager entry table]
 * called from: data@00009158
 * calls: fn_002b84, fn_004ca4
 */
 /* WARNING: Unknown calling convention */
 /* RBF entry point #14: set status (I$SetStt) [file manager entry table] */
uint rbf_setstat(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  uint uVar2;
  uint a3;
  uint extraout_ECX;
  uint in_EDX;
  uint a2;
  Statics *g;
  PathDesc *pd;
  
  uVar1 = fn_004ca4((uint)*(ushort *)(a0 + 0x10),(uint)pd,(uint)&g->f_094,in_EDX);
  *(uint *)(a3 + 4) = uVar1;
  uVar2 = a3;
  if (uVar1 == 0xd0) {
    uVar2 = fn_002b84(0,4,a2,a3,in_EDX);
    *(uint *)(extraout_ECX + 4) = uVar2;
    uVar2 = extraout_ECX;
  }
  return *(uint *)(uVar2 + 4);
}

/* ----------------------------------------------------------------------
 * fn_004d2a   @0x004d2a, 75 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * calls: fn_005c60, fn_006054
 */
 /* WARNING: Unknown calling convention */
uint fn_004d2a(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  int extraout_ECX;
  uint in_EDX;
  uint *in_stack_00000004;
  
  if (*in_stack_00000004 < 0x58) {
    uVar1 = 0xb2;
  }
  else {
    uVar1 = fn_005c60(0x58,in_stack_00000004[1],a0,in_EDX);
    if (uVar1 == 0) {
      fn_006054(*(uint *)(extraout_ECX + 4),0x58,in_EDX);
      uVar1 = 0;
    }
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_004d76   @0x004d76, 83 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * calls: fn_005c60, fn_006054
 */
 /* WARNING: Unknown calling convention */
uint fn_004d76(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  int extraout_ECX;
  uint unaff_ESI;
  uint *in_stack_00000004;
  
  if (*in_stack_00000004 < 0x18) {
    uVar1 = 0xb2;
  }
  else {
    uVar1 = fn_005c60(0x18,in_stack_00000004[1],a0,unaff_ESI);
    if (uVar1 == 0) {
      fn_006054(*(uint *)(extraout_ECX + 4),0x18,unaff_ESI);
      uVar1 = 0;
    }
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_004dca   @0x004dca, 154 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * calls: fn_000a76, fn_004464, os_f_time, fn_00789c, lock_release
 */
 /* WARNING: Unknown calling convention */
uint fn_004dca(a0, a1)
    uint a0;
    uint a1;
{
  undefined4 uVar1;
  undefined4 uVar2;
  uint uVar3;
  int extraout_ECX;
  uint a0_00;
  int extraout_ECX_00;
  undefined4 *extraout_EDX;
  uint unaff_ESI;
  uint *in_stack_00000004;
  
  if ((*(ushort *)(a0 + 0x14) & 2) == 0) {
    uVar3 = 0xcb;
  }
  else if (*(short *)(*(int *)(a0 + 4) + 0x56) == 0x4143) {
    uVar3 = 0xf2;
  }
  else if (*(uint *)(a0 + 0x7c) < *in_stack_00000004) {
    uVar3 = fn_004464(*in_stack_00000004,unaff_ESI);
  }
  else if (*(int *)(a0 + 0xc0) == 0) {
    uVar3 = 0xe1;
  }
  else {
    fn_00789c(*(int *)(a0 + 0x70) + 0x28);
    uVar3 = os_f_time();
    *(uint *)(*(int *)(extraout_ECX + 0x70) + 0x5c) = uVar3;
    *(byte *)(extraout_ECX + 100) = *(byte *)(extraout_ECX + 100) | 1;
    lock_release(*(int *)(extraout_ECX + 0x70) + 0x28);
    uVar1 = *extraout_EDX;
    uVar2 = *(undefined4 *)(a0_00 + 0x78);
    *(undefined4 *)(a0_00 + 0x7c) = uVar1;
    *(undefined4 *)(a0_00 + 0x78) = uVar1;
    uVar3 = fn_000a76(a0_00);
    *(undefined4 *)(extraout_ECX_00 + 0x78) = uVar2;
  }
  return uVar3;
}

/* ----------------------------------------------------------------------
 * fn_004e64   @0x004e64, 315 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * calls: fn_00351a, fn_005c60, os_f_id, fn_00789c, lock_release
 */
 /* WARNING: Unknown calling convention */
uint fn_004e64(a0, a1)
    uint a0;
    uint a1;
{
  int iVar1;
  uint uVar2;
  uint extraout_ECX;
  uint a1_00;
  uint extraout_EDX;
  uint unaff_EDI;
  
  if (*(short *)(*(int *)(a0 + 4) + 0x56) == 0x4143) {
    return 0xf2;
  }
  if (*(int *)(a0 + 0x70) == 0) {
    return 0xe1;
  }
  iVar1 = *(int *)(a0 + 0x70);
  fn_00789c(*(int *)(a0 + 0x70) + 0x28);
  if ((*(int *)(a1_00 + 0x28) == *(int *)(iVar1 + 0x60)) ||
     (uVar2 = a1_00, (*(ushort *)(a0 + 0x14) & 2) != 0)) {
    if ((*(ushort *)(a0 + 0x14) & 2) == 0) {
      lock_release(*(int *)(a0 + 0x70) + 0x28);
      return 0xcb;
    }
    uVar2 = fn_005c60(0x30,a1_00,a0,unaff_EDI);
    if (uVar2 != 0) goto LAB_00004f27;
    uVar2 = os_f_id();
    if (uVar2 >> 0x10 == 0) {
      *(undefined2 *)(iVar1 + 0x46) = *(undefined2 *)(extraout_EDX + 0xe);
      *(undefined2 *)(iVar1 + 0x48) = *(undefined2 *)(extraout_EDX + 0x10);
    }
    *(undefined4 *)(iVar1 + 0x54) = *(undefined4 *)(extraout_EDX + 0x1c);
    *(undefined4 *)(iVar1 + 0x58) = *(undefined4 *)(extraout_EDX + 0x20);
    *(undefined4 *)(iVar1 + 0x5c) = *(undefined4 *)(extraout_EDX + 0x24);
    uVar2 = extraout_EDX;
  }
  *(undefined4 *)(iVar1 + 0x60) = *(undefined4 *)(uVar2 + 0x28);
  fn_00351a(a0);
LAB_00004f27:
  lock_release(*(int *)(a0 + 0x70) + 0x28);
  return extraout_ECX;
}

/* ----------------------------------------------------------------------
 * fn_004fa0   @0x004fa0, 22 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 */
 /* WARNING: Unknown calling convention */
uint fn_004fa0(a0, a1)
    uint a0;
    uint a1;
{
  undefined4 *in_stack_00000004;
  
  *(undefined4 *)(a0 + 0xb8) = *in_stack_00000004;
  return 0;
}

/* ----------------------------------------------------------------------
 * fn_004fb6   @0x004fb6, 30 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * calls: fn_007da4
 */
 /* WARNING: Unknown calling convention */
uint fn_004fb6(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  uint in_ECX;
  uint *in_stack_00000004;
  
  if (*in_stack_00000004 == 0xffffffff) {
    *(byte *)(a0 + 0x65) = *(byte *)(a0 + 0x65) | 2;
  }
  uVar1 = fn_007da4(*in_stack_00000004,in_ECX);
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_004fd4   @0x004fd4, 422 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * calls: fn_0034ee, os_f_id, fn_005cfc, fn_0073d0, fn_007b6a
 */
 /* WARNING: Unknown calling convention */
uint fn_004fd4(a0, a1)
    uint a0;
    uint a1;
{
  int iVar1;
  undefined4 uVar2;
  uint uVar3;
  uint uVar4;
  uint extraout_ECX;
  uint a0_00;
  uint a1_00;
  uint a1_01;
  uint a1_02;
  uint unaff_EDI;
  bool bVar5;
  uint *in_stack_00000004;
  
  if (*(short *)(*(int *)(a0 + 4) + 0x56) == 0x4143) {
    uVar3 = 0xf2;
  }
  else if (*(int *)(a0 + 0x70) == 0) {
    uVar3 = 0xe1;
  }
  else {
    iVar1 = *(int *)(a0 + 0x70);
    uVar2 = *(undefined4 *)(iVar1 + 0x40);
    uVar3 = os_f_id();
    if (uVar3 >> 0x10 != 0) {
      if (*(short *)(iVar1 + 0x48) == 0) {
        bVar5 = uVar3 >> 0x10 != (uint)*(ushort *)(iVar1 + 0x46);
      }
      else if (((uVar3 & 0xffff) == 0) || ((uVar3 & 0xffff) == (uint)*(ushort *)(iVar1 + 0x48))) {
        bVar5 = false;
      }
      else {
        bVar5 = true;
      }
      if (bVar5) {
        return 0xd6;
      }
    }
    if ((*(uint *)(iVar1 + 0x40) & 0x8000) == (*in_stack_00000004 & 0x8000)) {
      *(uint *)(iVar1 + 0x40) = *in_stack_00000004 & 0xffff | *(uint *)(iVar1 + 0x40) & 0xffff0000;
      uVar3 = fn_0034ee(a0_00);
    }
    else {
      if ((*in_stack_00000004 >> 0xf & 1) != 0) {
        return 0xee;
      }
      if (*(int *)(a0_00 + 0xc0) == *(int *)(*(int *)(*(int *)(a0_00 + 4) + 0x20) + 0x28)) {
        return 0xd6;
      }
      uVar3 = fn_005cfc(a0_00,unaff_EDI);
      if (uVar3 == 0) {
        *(uint *)(iVar1 + 0x40) = *in_stack_00000004 & 0xffff | *(uint *)(iVar1 + 0x40) & 0xffff0000
        ;
        uVar3 = fn_0073d0(0xffffffff,a1_00,unaff_EDI);
        if (uVar3 == 0) {
          uVar4 = fn_0073d0(0xffffffff,a1_01,unaff_EDI);
          uVar3 = 0;
          if (uVar4 != 0) {
            *(undefined4 *)(iVar1 + 0x40) = uVar2;
            uVar3 = fn_0073d0(1,a1_02,unaff_EDI);
          }
        }
        fn_007b6a(CONCAT31((int3)(uVar3 >> 8),2),unaff_EDI);
        uVar3 = extraout_ECX;
      }
    }
    if (uVar3 != 0) {
      *(undefined4 *)(iVar1 + 0x40) = uVar2;
    }
  }
  return uVar3;
}

/* ----------------------------------------------------------------------
 * fn_00517a   @0x00517a, 830 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * calls: fn_0017f6, fn_005c60, fn_006054, fn_006a22, fn_0076fa, fn_00789c, fn_0079d2, fn_0081fa
 */
 /* WARNING: Unknown calling convention */
uint fn_00517a(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  uint extraout_ECX;
  int iVar2;
  int extraout_ECX_00;
  int extraout_ECX_01;
  uint extraout_ECX_02;
  uint a2;
  int extraout_EDX;
  undefined4 *extraout_EDX_00;
  int extraout_EDX_01;
  int extraout_EDX_02;
  int extraout_EDX_03;
  int extraout_EDX_04;
  int iVar3;
  int extraout_EDX_05;
  int extraout_EDX_06;
  undefined4 *puVar4;
  uint unaff_EDI;
  undefined4 *puVar5;
  byte bVar6;
  uint *in_stack_00000004;
  undefined4 local_30;
  undefined4 local_2c;
  undefined4 *local_28;
  undefined4 *local_24;
  uint local_20;
  uint local_1c;
  char local_18 [4];
  uint local_14;
  undefined2 local_10;
  uint local_8;
  
  bVar6 = 0;
  local_28 = (undefined4 *)0x0;
  local_24 = (undefined4 *)0x0;
  local_20 = 0;
  if ((*(ushort *)(a0 + 0x14) & 2) == 0) {
    uVar1 = 0xd6;
  }
  else if (*(short *)(*(int *)(a0 + 4) + 0x56) == 0x4143) {
    uVar1 = 0xf2;
  }
  else if (*(int *)(a0 + 0x70) == 0) {
    uVar1 = 0xe1;
  }
  else {
    local_8 = *in_stack_00000004;
    uVar1 = os_f_prsnam(local_8,(uint)local_18,(uint)&local_14,unaff_EDI);
    if ((uVar1 == 0) && (uVar1 = fn_005c60(local_1c,local_8,a2,unaff_EDI), uVar1 == 0)) {
      if (((local_1c < 0x2c) && (local_18[0] != '/')) &&
         ((((*(char *)(extraout_EDX + 0xd4) != '.' || (*(char *)(extraout_EDX + 0xd5) != '.')) ||
           (*(char *)(extraout_EDX + 0xd6) != '\0')) &&
          ((*(char *)(extraout_EDX + 0xd4) != '.' || (*(char *)(extraout_EDX + 0xd5) != '\0')))))) {
        local_2c = 0x168;
        uVar1 = os_f_srqmem((uint)&local_2c,0,unaff_EDI);
        if (uVar1 == 0) {
          local_30 = 0x168;
          uVar1 = os_f_srqmem((uint)&local_30,0,unaff_EDI);
          if (uVar1 == 0) {
            puVar4 = extraout_EDX_00;
            puVar5 = local_28;
            for (iVar2 = 0x5a; iVar2 != 0; iVar2 = iVar2 + -1) {
              *puVar5 = *puVar4;
              puVar4 = puVar4 + (uint)bVar6 * -2 + 1;
              puVar5 = puVar5 + (uint)bVar6 * -2 + 1;
            }
            puVar4 = extraout_EDX_00;
            puVar5 = local_24;
            for (iVar2 = 0x5a; iVar2 != 0; iVar2 = iVar2 + -1) {
              *puVar5 = *puVar4;
              puVar4 = puVar4 + (uint)bVar6 * -2 + 1;
              puVar5 = puVar5 + (uint)bVar6 * -2 + 1;
            }
            local_24[0x26] = 0;
            local_28[0x26] = 0;
            local_24[0x25] = 0;
            local_28[0x25] = 0;
            local_24[0x24] = 0;
            local_28[0x24] = 0;
            local_24[0x1b] = 0;
            local_28[0x1b] = 0;
            local_24[0x1c] = 0;
            local_28[0x1c] = 0;
            uVar1 = fn_0087f6(local_8,local_1c,unaff_EDI);
            if (uVar1 == 0) {
              local_28[0x30] = *(undefined4 *)(extraout_EDX_01 + 0xc4);
              local_28[0x2d] = local_28;
              fn_0076fa((uint)local_28);
              fn_006a22((uint)local_28,unaff_EDI);
              iVar2 = extraout_ECX_01;
              iVar3 = extraout_EDX_04;
            }
            else {
              local_14 = local_8;
              local_24[0x30] = *(undefined4 *)(extraout_EDX_01 + 0xc4);
              local_28[4] = local_24;
              local_10 = *(undefined2 *)(extraout_EDX_01 + 0x14);
              uVar1 = fn_0081fa((uint)local_28,(uint)&local_14,unaff_EDI);
              if ((uVar1 == 0) || (uVar1 != 0xd8)) {
                fn_0079d2((uint)local_28);
                iVar2 = extraout_ECX_00;
                iVar3 = extraout_EDX_03;
              }
              else {
                iVar2 = 0;
                iVar3 = extraout_EDX_02;
              }
            }
            if (iVar2 == 0) {
              local_28[0x1e] = *(undefined4 *)(iVar3 + 200);
              uVar1 = fn_008776((uint)local_28,unaff_EDI);
              if (uVar1 == 0) {
                *(uint *)(local_28[0x1b] + 8) = *(uint *)(local_28[0x1b] + 8) | 1;
                uVar1 = local_20;
                fn_006054(local_8,local_1c,unaff_EDI);
                *(undefined1 *)(local_20 + local_1c) = 0;
                fn_006054(local_8,local_1c,uVar1);
                *(undefined1 *)(extraout_EDX_05 + 0xd4 + local_1c) = 0;
                fn_00789c(*(int *)(extraout_EDX_05 + 0x70) + 0x28);
                iVar2 = *(int *)(extraout_EDX_06 + 0x70);
                *(undefined4 *)(iVar2 + 0x58) = *(undefined4 *)(iVar2 + 0x5c);
                *(undefined4 *)(iVar2 + 0x54) = *(undefined4 *)(iVar2 + 0x5c);
                *(byte *)(extraout_EDX_06 + 100) = *(byte *)(extraout_EDX_06 + 100) | 1;
                lock_release(*(int *)(extraout_EDX_06 + 0x70) + 0x28);
                fn_0017f6((uint)local_28);
              }
              fn_0079d2((uint)local_28);
            }
            puVar4 = local_28;
            os_f_srtmem_8db8(0x168,unaff_EDI);
            os_f_srtmem_8db8(0x168,(uint)puVar4);
            uVar1 = extraout_ECX_02;
          }
          else {
            os_f_srtmem_8db8(0x168,unaff_EDI);
            uVar1 = extraout_ECX;
          }
        }
      }
      else {
        uVar1 = 0xd7;
      }
    }
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_0054b8   @0x0054b8, 1084 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * calls: fn_0017f6, fn_00284a, fn_002c74, fn_00351a, fn_005dee, fn_005e06, fn_006054, fn_006894
 */
 /* WARNING: Unknown calling convention */
uint fn_0054b8(a0, a1)
    uint a0;
    uint a1;
{
  code *pcVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  int extraout_ECX;
  uint a0_00;
  uint extraout_ECX_00;
  int *extraout_EDX;
  uint *extraout_EDX_00;
  int extraout_EDX_01;
  uint extraout_EDX_02;
  char *extraout_EDX_03;
  char *extraout_EDX_04;
  char *extraout_EDX_05;
  char *pcVar5;
  uint unaff_ESI;
  uint unaff_EDI;
  uint *in_stack_00000004;
  undefined2 uVar6;
  int local_48;
  char local_42;
  uint local_40;
  undefined2 local_3c;
  undefined2 local_3a;
  undefined4 local_34;
  uint local_30;
  ushort local_2c;
  uint local_2a;
  uint local_24;
  uint local_20;
  char *local_1c;
  int local_18;
  uint local_14;
  uint local_10;
  uint local_c;
  int local_8;
  
  if (*(short *)(*(int *)(a0 + 4) + 0x56) == 0x4143) {
    uVar2 = 0xf2;
  }
  else if (*(int *)(a0 + 0x70) == 0) {
    uVar2 = 0xe1;
  }
  else {
    local_40 = *in_stack_00000004;
    local_14 = a0;
    do {
      local_30 = local_40;
      uVar2 = fn_00851a((uint)&local_30,(uint)&local_24,unaff_EDI);
      if (uVar2 != 0) {
        return uVar2;
      }
    } while (local_42 == '/');
    local_10 = local_30;
    if (local_30 - *extraout_EDX == 0) {
      local_30 = 0xda;
    }
    else {
      local_48 = local_30 - *extraout_EDX;
      uVar2 = os_f_srqmem((uint)&local_48,0,unaff_EDI);
      if (uVar2 != 0) {
        return uVar2;
      }
      uVar2 = local_30;
      fn_006054(*extraout_EDX_00,extraout_ECX - 1,unaff_EDI);
      fn_006054(0xd8,1,uVar2);
    }
    local_3c = 0x9f;
    local_3a = 1;
    local_34 = 0x16;
    local_2c = *(ushort *)(local_14 + 0x14) & 4 | 0x8002;
    pcVar1 = (code *)swi(0xff);
    (*pcVar1)();
    uVar2 = *(uint *)(extraout_EDX_01 + 4);
    if (a0_00 != 0) {
      os_f_srtmem_8db8(a0_00,unaff_ESI);
      uVar2 = extraout_EDX_02;
    }
    if (uVar2 == 0) {
      uVar6 = (undefined2)((uint)&local_20 >> 0x10);
      uVar2 = os_i_getpd(local_2a,unaff_ESI);
      if (uVar2 == 0) {
        local_c = local_20;
        if (*(int *)(local_14 + 4) == *(int *)(local_20 + 4)) {
          uVar2 = fn_008660(local_20,CONCAT22(uVar6,0x8004),unaff_ESI);
          if (uVar2 == 0) {
            local_18 = 0;
            uVar2 = fn_008776(local_c,unaff_ESI);
            if (uVar2 == 0) {
              do {
                if (*local_1c == '\0') {
                  if (local_18 == 0) {
                    local_18 = *(int *)(local_c + 0x78);
                  }
                }
                else {
                  uVar3 = fn_0087f6(local_10,local_24,unaff_ESI);
                  uVar2 = extraout_ECX_00;
                  if (uVar3 == 0) break;
                }
                uVar2 = fn_008840(local_c,unaff_ESI);
              } while (uVar2 == 0);
              if (uVar2 == 0) {
                uVar2 = 0xda;
              }
            }
            if (uVar2 == 0xd3) {
              if ((*(ushort *)(local_14 + 0x14) & 0x8000) == 0) {
LAB_00005838:
                fn_00789c(*(int *)(local_14 + 0x70) + 0x28);
                uVar2 = os_f_time();
                *(uint *)(*(int *)(local_14 + 0x70) + 0x5c) = uVar2;
                lock_release(*(int *)(local_14 + 0x70) + 0x28);
                uVar2 = fn_0073d0(1,local_14,unaff_ESI);
                if (uVar2 == 0) {
                  *(int *)(local_c + 200) = local_18;
                  uVar2 = fn_006894(local_c,local_10,local_24,unaff_ESI);
                  if ((uVar2 == 0) && (fn_0017f6(local_c), (*(byte *)(local_c + 100) & 1) != 0))
                  {
                    fn_00351a(local_c);
                  }
                  uVar2 = fn_005dee((uint)&local_3c,unaff_ESI);
                }
                else {
                  uVar2 = fn_005dee((uint)&local_3c,unaff_ESI);
                }
              }
              else {
                local_8 = *(int *)(local_c + 0xc0);
                uVar2 = *(uint *)(local_c + 0xc0);
                do {
                  uVar3 = uVar2;
                  if (uVar2 == *(uint *)(local_14 + 0xc0)) {
LAB_000057d9:
                    if (uVar3 == *(uint *)(local_14 + 0xc0)) {
                      uVar2 = fn_005dee((uint)&local_3c,unaff_ESI);
                      return uVar2;
                    }
                    if (local_8 == *(int *)(local_c + 0xc0)) {
                      uVar2 = fn_0017f6(local_c);
                    }
                    else {
                      uVar2 = fn_005e06(local_c,unaff_ESI);
                    }
                    if (uVar2 != 0) {
                      uVar2 = fn_005dee((uint)&local_3c,unaff_ESI);
                      return uVar2;
                    }
                    goto LAB_00005838;
                  }
                  *(undefined4 *)(local_c + 0x78) = 0;
                  uVar3 = fn_002c74(local_c);
                  pcVar5 = extraout_EDX_03;
                  if (uVar3 == 0) {
                    uVar3 = fn_008776(local_c,unaff_ESI);
                    pcVar5 = extraout_EDX_04;
                    while ((uVar3 == 0 &&
                           (((*local_1c != '.' || (local_1c[1] != '.')) ||
                            (pcVar5 = local_1c, local_1c[2] != '\0'))))) {
                      uVar3 = fn_008840(local_c,unaff_ESI);
                      pcVar5 = extraout_EDX_05;
                    }
                  }
                  if (uVar3 != 0) {
                    uVar2 = fn_005dee((uint)&local_3c,unaff_ESI);
                    return uVar2;
                  }
                  uVar3 = fn_00284a(*(uint *)(pcVar5 + 0x3c),unaff_ESI);
                  if (uVar2 == uVar3) goto LAB_000057d9;
                  uVar4 = fn_005e06(local_c,unaff_ESI);
                  uVar2 = uVar3;
                } while (uVar4 == 0);
                uVar2 = fn_005dee((uint)&local_3c,unaff_ESI);
              }
            }
            else {
              uVar2 = fn_005dee((uint)&local_3c,unaff_ESI);
            }
          }
          else {
            uVar2 = fn_005dee((uint)&local_3c,unaff_ESI);
          }
        }
        else {
          uVar2 = fn_005dee((uint)&local_3c,unaff_ESI);
        }
      }
      else {
        uVar2 = fn_005dee((uint)&local_3c,unaff_ESI);
      }
    }
  }
  return uVar2;
}

/* ----------------------------------------------------------------------
 * fn_0058f4   @0x0058f4, 537 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * calls: fn_002724, os_f_id, fn_005e56, fn_00789c, lock_release, os_f_srqmem, fn_0090f8
 */
 /* WARNING: Type propagation algorithm not settling */
 /* WARNING: Unknown calling convention */
uint fn_0058f4(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  int *extraout_ECX;
  int extraout_ECX_00;
  uint extraout_ECX_01;
  int iVar2;
  uint unaff_EDI;
  int local_1c [3];
  int local_10;
  uint local_c;
  int local_8;
  
  local_c = a0;
  uVar1 = os_f_id();
  if (uVar1 >> 0x10 == 0) {
    local_8 = *(int *)(*(int *)(local_c + 4) + 0x20);
    uVar1 = fn_00789c(local_8 + 0xb0);
    if (uVar1 == 0) {
      local_1c[2] = *(uint *)(local_8 + 0xac);
      if (*extraout_ECX == 0) {
        if (local_1c[2] != 0) {
          fn_002724(1,local_1c[2],1,unaff_EDI);
          fn_005e56(local_1c[2]);
          lock_release(local_8 + 0xb0);
          *(undefined4 *)(local_8 + 0xac) = 0;
        }
      }
      else {
        if (local_1c[2] != 0) {
          lock_release(local_8 + 0xb0);
          return 0xda;
        }
        local_1c[1] = 0x870;
        uVar1 = os_f_srqmem((uint)(local_1c + 1),0,unaff_EDI);
        if (uVar1 != 0) {
          lock_release(local_8 + 0xb0);
          return uVar1;
        }
        uVar1 = 0x870;
        fn_0090f8(0);
        *(undefined4 *)(local_1c[2] + 0x80c) = *(undefined4 *)(local_c + 0x13c);
        local_1c[0] = extraout_ECX_00 * (*(int *)(local_c + 0x13c) + 0x20);
        uVar1 = os_f_srqmem((uint)local_1c,0,uVar1);
        if (uVar1 != 0) {
          fn_005e56(local_1c[2]);
          lock_release(local_8 + 0xb0);
          return uVar1;
        }
        for (uVar1 = 0; uVar1 < extraout_ECX_01; uVar1 = uVar1 + 1) {
          *(int *)(local_10 + 0x14) = local_10 + 0x20;
          *(undefined4 *)(local_10 + 8) = *(undefined4 *)(local_1c[2] + 0x81c);
          *(int *)(local_1c[2] + 0x81c) = local_10;
          local_10 = local_10 + *(int *)(local_c + 0x13c) + 0x20;
        }
        iVar2 = 0x101;
        uVar1 = local_1c[2];
        while (iVar2 = iVar2 + -1, iVar2 != 0) {
          *(uint *)(uVar1 + 4) = uVar1;
          *(uint *)uVar1 = uVar1;
          uVar1 = uVar1 + 8;
        }
        *(undefined1 *)(local_1c[2] + 0x801) = 0;
        *(undefined1 *)(local_1c[2] + 0x800) = 0;
        *(uint *)(local_1c[2] + 0x818) = local_1c[2] + 0x80cU;
        *(uint *)(local_1c[2] + 0x814) = local_1c[2] + 0x80cU;
        *(int *)(local_8 + 0xac) = local_1c[2];
      }
      lock_release(local_8 + 0xb0);
      uVar1 = 0;
    }
  }
  else {
    uVar1 = 0xa4;
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_005b0e   @0x005b0e, 110 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * calls: fn_001332, fn_00789c, lock_release
 */
 /* WARNING: Unknown calling convention */
uint fn_005b0e(a0)
    uint a0;
{
  uint uVar1;
  uint a0_00;
  uint extraout_ECX;
  uint extraout_EDX;
  
  if (*(short *)(*(int *)(a0 + 4) + 0x56) == 0x4143) {
    uVar1 = 0xf2;
  }
  else if ((*(int *)(a0 + 0xc0) == 0) && ((*(ushort *)(a0 + 0x14) & 2) != 0)) {
    uVar1 = fn_00789c(*(int *)(*(int *)(a0 + 4) + 0x20) + 0x74);
    if (uVar1 == 0) {
      uVar1 = a0_00;
      if (*(int *)(a0_00 + 0x84) != 0) {
        fn_001332(a0_00);
        uVar1 = extraout_ECX;
      }
      lock_release(uVar1 + 0x74);
      uVar1 = extraout_EDX;
    }
  }
  else {
    uVar1 = 0xd6;
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_005b7c   @0x005b7c, 189 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * calls: os_f_id, fn_0073d0, fn_0076fa, fn_0079d2, os_f_srtmem_8db8, os_f_srqmem
 */
 /* WARNING: Unknown calling convention */
uint fn_005b7c(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  int iVar2;
  uint extraout_ECX;
  uint a1_00;
  uint a0_00;
  uint unaff_EDI;
  undefined4 *puVar3;
  byte bVar4;
  undefined4 *in_stack_00000004;
  undefined4 local_10;
  undefined4 *local_c;
  undefined4 *local_8;
  
  bVar4 = 0;
  local_8 = in_stack_00000004;
  if (*(short *)(*(int *)(a0 + 4) + 0x56) == 0x4143) {
    uVar1 = 0xf2;
  }
  else {
    uVar1 = os_f_id();
    if (uVar1 >> 0x10 == 0) {
      local_10 = 0x168;
      uVar1 = os_f_srqmem((uint)&local_10,0,unaff_EDI);
      if (uVar1 == 0) {
        puVar3 = local_c;
        for (iVar2 = 0x5a; iVar2 != 0; iVar2 = iVar2 + -1) {
          *puVar3 = *(undefined4 *)a0;
          a0 = (uint)(a0 + ((uint)bVar4 * -2 + 1) * 4);
          puVar3 = puVar3 + (uint)bVar4 * -2 + 1;
        }
        local_c[0x30] = *local_8;
        local_c[0x1c] = 0;
        *(undefined1 *)((int)local_c + 0x65) = 0;
        local_c[0x2d] = local_c;
        uVar1 = fn_0076fa((uint)local_c);
        if (uVar1 == 0) {
          fn_0073d0(local_8[1],a1_00,unaff_EDI);
          fn_0079d2(a0_00);
        }
        os_f_srtmem_8db8(0x168,unaff_EDI);
        uVar1 = extraout_ECX;
      }
    }
    else {
      uVar1 = 0xd6;
    }
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_005c3a   @0x005c3a, 37 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 */
 /* WARNING: Unknown calling convention */
uint fn_005c3a(a0, a1)
    uint a0;
    uint a1;
{
  int *in_stack_00000004;
  
  if (*in_stack_00000004 == 0) {
    *(undefined2 *)(*(int *)(a0 + 4) + 0x56) = 0;
  }
  else {
    *(undefined2 *)(*(int *)(a0 + 4) + 0x56) = 0x4143;
  }
  return 0;
}

/* ----------------------------------------------------------------------
 * fn_005c60   @0x005c60, 91 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_004694, fn_0046e4, fn_00473c, fn_0047f4, fn_00486a, fn_004a20, fn_004aa8, fn_004bf8
 * calls: os_f_findpd, os_f_chkmem
 */
 /* WARNING: Unknown calling convention */
uint fn_005c60(a0, a1, a2, a3)
    uint a0;
    uint a1;
    uint a2;
    uint a3;
{
  uint uVar1;
  uint a1_00;
  uint a0_00;
  uint unaff_EDI;
  undefined4 local_8;
  
  if ((*(uint *)(a2 + 0x40) & 1) == 0) {
    uVar1 = os_f_findpd(*(uint *)(a2 + 0x1c),unaff_EDI);
    if (uVar1 == 0) {
      uVar1 = os_f_chkmem(a0_00,a1_00,local_8,unaff_EDI);
      if ((uVar1 == 0) || (uVar1 == 0xd0)) {
        uVar1 = 0;
      }
    }
  }
  else {
    uVar1 = 0;
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * os_f_id   @0x005cbc, 63 bytes
 * kernel call wrapper: builds the parameter block for F_ID (code 0x1a, system state) and enters the kernel
 * called from: fn_00486a, fn_004e64, fn_004fd4, fn_0058f4, fn_005b7c, fn_006b50
 */
 /* WARNING: Unknown calling convention */
 /* kernel call wrapper: builds the parameter block for F_ID (code 0x1a, system state) and enters the kernel */
uint os_f_id()
{
  code *pcVar1;
  uint uVar2;
  int extraout_ECX;
  undefined2 local_8;
  undefined2 local_6;
  
  pcVar1 = (code *)swi(0xff);
  (*pcVar1)(0x1a);
  if (*(int *)(extraout_ECX + 4) == 0) {
    uVar2 = CONCAT22(local_8,local_6);
  }
  else {
    uVar2 = 0x10001;
  }
  return uVar2;
}

/* ----------------------------------------------------------------------
 * fn_005cfc   @0x005cfc, 241 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_004fd4, fn_006f10
 * calls: fn_0017f6, fn_00284a, fn_002c74, fn_00789c, fn_0079b6, fn_008776, fn_008840, lock_release
 */
 /* WARNING: Unknown calling convention */
uint fn_005cfc(a0, a1)
    uint a0;
    uint a1;
{
  undefined4 uVar1;
  uint uVar2;
  uint a0_00;
  uint extraout_ECX;
  int extraout_EDX;
  uint a0_01;
  uint a0_02;
  uint extraout_EDX_00;
  uint extraout_EDX_01;
  uint a0_03;
  uint extraout_EDX_02;
  uint extraout_EDX_03;
  uint unaff_EDI;
  uint *in_stack_00000004;
  char *local_c;
  
  uVar2 = fn_00789c(*(int *)(*(int *)(a0 + 4) + 0x20) + 0x60);
  if (uVar2 == 0) {
    if (extraout_EDX == *(int *)(extraout_EDX + 0x98)) {
      *(byte *)(extraout_EDX + 0x65) = *(byte *)(extraout_EDX + 0x65) | 2;
      lock_release(a0_00 + 0x60);
      uVar1 = *(undefined4 *)(a0_01 + 0x78);
      *(undefined4 *)(a0_01 + 0x78) = 0;
      uVar2 = fn_002c74(a0_01);
      a0_03 = a0_02;
      if (uVar2 == 0) {
        uVar2 = fn_008776(a0_02,unaff_EDI);
        a0_03 = extraout_EDX_00;
        while (uVar2 == 0) {
          if (*local_c != '\0') {
            if (((*local_c == '.') && (local_c[1] == '.')) && (local_c[2] == '\0')) {
              uVar2 = fn_00284a(*(uint *)(local_c + 0x3c),unaff_EDI);
              *in_stack_00000004 = uVar2;
              a0_03 = extraout_EDX_01;
            }
            else if ((*local_c != '.') || (local_c[1] != '\0')) {
              uVar2 = 0xee;
              break;
            }
          }
          uVar2 = fn_008840(a0_03,unaff_EDI);
          a0_03 = extraout_EDX_02;
        }
      }
      if (*(int *)(a0_03 + 0x6c) != 0) {
        fn_0017f6(a0_03);
        uVar2 = extraout_ECX;
        a0_03 = extraout_EDX_03;
      }
      *(undefined4 *)(a0_03 + 0x78) = uVar1;
      if (uVar2 == 0xd3) {
        uVar2 = 0;
      }
    }
    else {
      uVar2 = fn_0079b6(a0_00,unaff_EDI);
    }
  }
  return uVar2;
}
