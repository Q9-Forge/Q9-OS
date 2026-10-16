/*
 * rbf_04.c - rbf part 4
 *
 * Original address range: 0x003944 - 0x004bf8 of the V4.9 kernel module.
 * Functions 0x3944 to 0x4aa8 (support routines)
 * Generated from the Ghidra export; see tools/gen_src.py. Hand edits belong in hand/*.inc.
 */
#include "kernel.h"

/* ----------------------------------------------------------------------
 * fn_003944   @0x003944, 79 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_000a76
 * calls: fn_000770, fn_0037d2
 */
 /* WARNING: Unknown calling convention */
uint fn_003944(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  int *a0_00;
  uint unaff_ESI;
  uint in_stack_00000004;
  
  *(undefined4 *)(in_stack_00000004 + 0x88) = *(undefined4 *)(in_stack_00000004 + 0x13c);
  *(undefined4 *)(in_stack_00000004 + 0x84) = *(undefined4 *)(a0 + 0x10);
  uVar1 = fn_000770(in_stack_00000004);
  if (uVar1 == 0) {
    if (a0_00 != (int *)a0_00[1]) {
      *(int *)a0_00[1] = *a0_00;
      *(int *)(*a0_00 + 4) = a0_00[1];
    }
    uVar1 = fn_0037d2((uint)a0_00,unaff_ESI);
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_003994   @0x003994, 27 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * calls: fn_0039b0
 */
 /* WARNING: Unknown calling convention */
uint fn_003994(a0)
    uint a0;
{
  uint uVar1;
  
  uVar1 = fn_0039b0((uint)GHIDRA_STACK_PTR(0xfffffff8),1);
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_0039b0   @0x0039b0, 89 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_003994, fn_003a26, fn_0047d8, fn_006de4, fn_008776
 */
 /* WARNING: Unknown calling convention */
uint fn_0039b0(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  int in_stack_00000004;
  
  if (*(uint *)(in_stack_00000004 + 0x78) < *(uint *)(in_stack_00000004 + 0x7c)) {
    uVar1 = *(int *)(in_stack_00000004 + 0x7c) - *(int *)(in_stack_00000004 + 0x78);
    if ((*(byte *)(in_stack_00000004 + 0x6a) & 1) == 0) {
      if (uVar1 < *(uint *)a0) {
        *(uint *)a0 = uVar1;
      }
    }
    else if (uVar1 < *(uint *)a0 >> (*(byte *)(in_stack_00000004 + 0x67) & 0x1f)) {
      *(uint *)a0 = uVar1 << (*(byte *)(in_stack_00000004 + 0x67) & 0x1f);
    }
    uVar1 = 0;
  }
  else {
    *(undefined4 *)a0 = 0;
    uVar1 = 0xd3;
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * rbf_readln   @0x003a0a, 27 bytes
 * RBF entry point #12: read a line from a path [file manager entry table]
 * called from: data@00009150
 * calls: fn_003a26
 */
 /* WARNING: Unknown calling convention */
 /* RBF entry point #12: read a line from a path [file manager entry table] */
uint rbf_readln(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  uint in_ECX;
  int extraout_ECX;
  PathDesc *pd;
  
  uVar1 = fn_003a26(0,(uint)pd,in_ECX);
  *(uint *)(extraout_ECX + 4) = uVar1;
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_003a26   @0x003a26, 959 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: rbf_readln, rbf_read
 * calls: fn_0017f6, fn_001af2, fn_002988, fn_002c74, fn_0039b0, fn_003e04, fn_003ef4, fn_003f8c
 */
 /* WARNING: Unknown calling convention */
uint fn_003a26(a0, a1, a2)
    uint a0;
    uint a1;
    uint a2;
{
  uint uVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  int extraout_EDX;
  int extraout_EDX_00;
  int extraout_EDX_01;
  uint unaff_EDI;
  int in_stack_00000004;
  uint local_20;
  uint local_1c;
  int local_18;
  int local_14;
  uint local_10;
  uint local_c;
  uint local_8;
  
  local_18 = in_stack_00000004;
  local_14 = 0;
  local_10 = *(uint *)(in_stack_00000004 + 0x10);
  local_1c = a0;
  local_c = fn_007da4(*(uint *)(in_stack_00000004 + 0x14),unaff_EDI);
  if ((local_c == 0) && (local_c = fn_0039b0(local_18 + 0x14,unaff_EDI), local_c == 0)) {
    local_8 = *(uint *)(local_18 + 0x14);
    iVar2 = 0;
    if (*(int *)(a1 + 0xc0) != 0) {
      fn_00789c(*(int *)(a1 + 0x70) + 0x28);
      if (((*(uint *)(a1 + 0x140) >> 5 & 1) == 0) && (*(short *)(*(int *)(a1 + 4) + 0x56) != 0x4143)
         ) {
        *(byte *)(a1 + 100) = *(byte *)(a1 + 100) | 1;
      }
      if (*(int *)(a1 + 0x70) == 0) {
        return 0xd3;
      }
      uVar1 = os_f_time();
      iVar2 = *(int *)(a1 + 0x70) + 0x38;
      *(uint *)(*(int *)(a1 + 0x70) + 0x54) = uVar1;
    }
    if ((*(int *)(a1 + 0xc0) == 0) || ((*(byte *)(iVar2 + 10) & 1) == 0)) {
      if ((*(int *)(a1 + 0xc0) == 0) && ((*(byte *)(a1 + 0x6a) & 1) != 0)) {
        if ((*(ushort *)(a1 + 0x14) & 2) == 0) {
          fn_007b6a(5,unaff_EDI);
        }
        fn_001af2(a1);
        uVar1 = fn_003e04(local_18 + 0x14,0x2ad0,a1,unaff_EDI);
        return uVar1;
      }
      if (*(int *)(a1 + 0xc0) != 0) {
        lock_release(*(int *)(a1 + 0x70) + 0x28);
      }
      while ((local_8 != 0 && (local_14 == 0))) {
        if ((local_1c == 0) ||
           (((local_8 < *(uint *)(a1 + 0x13c) ||
             ((*(uint *)(a1 + 0x78) & *(int *)(a1 + 0x13c) - 1U) != 0)) || ((local_10 & 1) != 0))))
        {
          local_c = fn_002c74(a1);
          if (local_c != 0) {
            if ((*(ushort *)(a1 + 0x14) & 2) == 0) {
              fn_007b6a(5,unaff_EDI);
            }
            goto LAB_00003c71;
          }
          uVar3 = *(int *)(a1 + 0x13c) - 1U & *(uint *)(a1 + 0x78);
          uVar1 = *(int *)(a1 + 0x13c) - uVar3;
          if (local_8 < uVar1) {
            uVar1 = local_8;
          }
          uVar3 = uVar3 + *(int *)(*(int *)(a1 + 0x6c) + 0x28);
          if (local_1c == 0) {
            for (uVar4 = 0; (*(char *)(uVar3 + uVar4) != '\r' && (uVar4 < uVar1)); uVar4 = uVar4 + 1
                ) {
            }
            if (uVar4 < uVar1) {
              uVar1 = uVar4 + 1;
              local_14 = 1;
            }
          }
          fn_006054(uVar3,uVar1,unaff_EDI);
          iVar2 = extraout_EDX_00;
          if (((extraout_EDX_00 + *(int *)(a1 + 0x78) & *(int *)(a1 + 0x13c) - 1U) == 0) &&
             (local_c = fn_0017f6(a1), iVar2 = extraout_EDX_01, local_c != 0)) {
            *(int *)(a1 + 0x78) = *(int *)(a1 + 0x78) + extraout_EDX_01;
            if ((*(ushort *)(a1 + 0x14) & 2) == 0) {
              fn_007b6a(5,unaff_EDI);
            }
            goto LAB_00003c71;
          }
        }
        else {
          if (*(int *)(a1 + 0xc0) == 0) {
            local_c = fn_0017f6(a1);
            if (local_c != 0) {
              if ((*(ushort *)(a1 + 0x14) & 2) == 0) {
                fn_007b6a(5,unaff_EDI);
              }
              goto LAB_00003c71;
            }
          }
          else {
            local_c = fn_002988(a1);
            if (local_c != 0) {
              if ((*(ushort *)(a1 + 0x14) & 2) == 0) {
                fn_007b6a(5,unaff_EDI);
              }
              goto LAB_00003c71;
            }
          }
          uVar1 = local_8;
          fn_003ef4((uint)&local_20,a1,unaff_EDI);
          local_c = fn_003f8c(1,local_20,a1,uVar1);
          iVar2 = extraout_EDX;
          if (local_c != 0) {
            if ((*(ushort *)(a1 + 0x14) & 2) == 0) {
              fn_007b6a(5,unaff_EDI);
            }
            goto LAB_00003c71;
          }
        }
        local_8 = local_8 - iVar2;
        local_10 = local_10 + iVar2;
        *(int *)(a1 + 0x78) = *(int *)(a1 + 0x78) + iVar2;
      }
      *(uint *)(local_18 + 0x14) = *(int *)(local_18 + 0x14) - local_8;
    }
    else {
      uVar1 = iVar2 + 0x30 + *(int *)(a1 + 0x78);
      if (local_1c == 0) {
        for (uVar3 = 0; (*(char *)(uVar1 + uVar3) != '\r' && (uVar3 < local_8)); uVar3 = uVar3 + 1)
        {
        }
        if (uVar3 < local_8) {
          local_8 = uVar3 + 1;
        }
      }
      fn_006054(uVar1,local_8,unaff_EDI);
      lock_release(*(int *)(a1 + 0x70) + 0x28);
      *(uint *)(local_18 + 0x14) = local_8;
      *(int *)(a1 + 0x78) = *(int *)(a1 + 0x78) + local_8;
    }
    if ((*(ushort *)(a1 + 0x14) & 2) == 0) {
      fn_007b6a(5,unaff_EDI);
    }
    fn_001af2(a1);
    return 0;
  }
  if ((*(ushort *)(a1 + 0x14) & 2) == 0) {
    fn_007b6a(5,unaff_EDI);
  }
LAB_00003c71:
  fn_001af2(a1);
  return local_c;
}

/* ----------------------------------------------------------------------
 * rbf_read   @0x003de6, 30 bytes
 * RBF entry point #11: read bytes from a path [file manager entry table]
 * called from: data@0000914c
 * calls: fn_003a26
 */
 /* WARNING: Unknown calling convention */
 /* RBF entry point #11: read bytes from a path [file manager entry table] */
uint rbf_read(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  uint in_ECX;
  int extraout_ECX;
  PathDesc *pd;
  
  uVar1 = fn_003a26(1,(uint)pd,in_ECX);
  *(uint *)(extraout_ECX + 4) = uVar1;
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_003e04   @0x003e04, 239 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_003a26, rbf_write
 */
 /* WARNING: Unknown calling convention */
uint fn_003e04(a0, a1, a2, a3)
    uint a0;
    uint a1;
    uint a2;
    uint a3;
{
  byte bVar1;
  uint uVar2;
  uint uVar3;
  int iVar4;
  undefined8 uVar5;
  undefined4 in_stack_00000004;
  uint local_c;
  
  if ((*(uint *)a0 & *(int *)(a2 + 0x13c) - 1U) == 0) {
    local_c = *(uint *)a0 >> (*(byte *)(a2 + 0x67) & 0x1f);
    uVar3 = *(int *)(a2 + 0x7c) - *(int *)(a2 + 0x78);
    if (uVar3 < local_c) {
      *(uint *)a0 = uVar3 << (*(byte *)(a2 + 0x67) & 0x1f);
      local_c = uVar3;
    }
    *(undefined4 *)(a2 + 0x60) = in_stack_00000004;
    if (*(uint *)(a2 + 0x154) < *(uint *)(a2 + 0x13c)) {
      uVar3 = 0xff;
    }
    else {
      uVar3 = *(uint *)(a2 + 0x154) >> (*(byte *)(a2 + 0x67) & 0x1f);
    }
    bVar1 = *(byte *)(a2 + 0x67);
    while( true ) {
      uVar5 = (*(code *)a1)(*(undefined4 *)(a2 + 0x78),a2);
      iVar4 = (int)((ulonglong)uVar5 >> 0x20);
      uVar2 = (uint)uVar5;
      if (uVar2 != 0) break;
      *(int *)(a2 + 0x78) = *(int *)(a2 + 0x78) + iVar4;
      local_c = local_c - iVar4;
      if (local_c == 0) {
        return 0;
      }
      *(int *)(a2 + 0x60) = *(int *)(a2 + 0x60) + (uVar3 << (bVar1 & 0x1f));
    }
    *(uint *)a0 = *(int *)a0 - (local_c << (*(byte *)(a2 + 0x67) & 0x1f));
  }
  else {
    uVar2 = 0xf1;
  }
  return uVar2;
}

/* ----------------------------------------------------------------------
 * fn_003ef4   @0x003ef4, 152 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_003a26, rbf_write
 */
 /* WARNING: Unknown calling convention */
uint fn_003ef4(a0, a1, a2)
    uint a0;
    uint a1;
    uint a2;
{
  byte bVar1;
  uint uVar2;
  uint uVar3;
  int in_stack_00000004;
  
  if ((*(uint *)(a1 + 0x140) >> 1 & 1) == 0) {
    *(undefined4 *)a0 = 1;
  }
  else {
    bVar1 = *(byte *)(a1 + 0x67);
    uVar3 = *(uint *)(a1 + 0x78);
    uVar2 = (uVar3 + in_stack_00000004 >> (bVar1 & 0x1f)) - (uVar3 >> (bVar1 & 0x1f));
    uVar3 = (*(int *)(a1 + 0x80) + *(int *)(a1 + 0x88)) - uVar3 >> (bVar1 & 0x1f);
    if (uVar3 < uVar2) {
      uVar2 = uVar3;
    }
    if (*(uint *)(a1 + 0x154) < *(uint *)(a1 + 0x13c)) {
      if (0xff < uVar2) {
        uVar2 = 0xff;
      }
    }
    else if (*(uint *)(a1 + 0x154) < uVar2 << (bVar1 & 0x1f)) {
      uVar2 = *(uint *)(a1 + 0x154) >> (bVar1 & 0x1f);
    }
    *(uint *)a0 = uVar2;
  }
  return a0;
}

/* ----------------------------------------------------------------------
 * fn_003f8c   @0x003f8c, 539 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_003a26, rbf_write
 * calls: fn_0015c4, fn_001740, fn_0017f6, fn_001b80, fn_001d1a, fn_002724, fn_002b5e, fn_002cd2
 */
 /* WARNING: Unknown calling convention */
uint fn_003f8c(a0, a1, a2, a3)
    uint a0;
    uint a1;
    uint a2;
    uint a3;
{
  uint *puVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  undefined4 extraout_ECX;
  undefined4 extraout_ECX_00;
  undefined4 extraout_ECX_01;
  undefined4 uVar6;
  uint extraout_ECX_02;
  int extraout_EDX;
  int extraout_EDX_00;
  uint a1_00;
  uint a1_01;
  uint a2_00;
  uint extraout_EDX_01;
  uint extraout_EDX_02;
  uint extraout_EDX_03;
  uint uVar7;
  int extraout_EDX_04;
  int extraout_EDX_05;
  uint extraout_EDX_06;
  uint a0_00;
  uint extraout_EDX_07;
  uint extraout_EDX_08;
  int extraout_EDX_09;
  int iVar8;
  uint unaff_EDI;
  uint in_stack_00000004;
  uint local_14;
  uint local_10;
  uint local_c;
  uint local_8;
  
  local_14 = in_stack_00000004;
  local_10 = a1;
  uVar2 = fn_002b5e(a2);
  if (uVar2 == 0) {
    local_c = ((uint)(*(int *)(extraout_EDX + 0x78) - *(int *)(extraout_EDX + 0x80)) >>
              (*(byte *)(extraout_EDX + 0x67) & 0x1f)) + *(int *)(extraout_EDX + 0x84);
    local_8 = 0;
    iVar8 = extraout_EDX;
    do {
      while( true ) {
        uVar5 = local_10;
        if (local_10 == 0) {
          return 0;
        }
        if (*(int *)(iVar8 + 0xc0) != 0) {
          if (*(int *)(iVar8 + 0x70) == 0) {
            return 0xd3;
          }
          local_8 = *(int *)(iVar8 + 0x70) + 0x18;
          fn_00789c(local_8);
          iVar8 = extraout_EDX_00;
        }
        uVar4 = *(uint *)(*(int *)(*(int *)(iVar8 + 4) + 0x18) + 0x1c);
        fn_00789c(uVar4);
        uVar2 = 0;
        fn_001b80(local_c,a1_00,unaff_EDI);
        if (local_10 != 0) {
          *(uint *)(a1_01 + 0x60) = local_14;
          if (a0 == 1) {
            uVar3 = fn_001d1a(local_10,a1_01,unaff_EDI);
            uVar6 = extraout_ECX;
            uVar7 = a2_00;
            if (uVar3 == 0) {
              uVar2 = fn_002cd2(local_10,1,a2_00,0,unaff_EDI);
              uVar6 = extraout_ECX_00;
              uVar7 = extraout_EDX_01;
            }
          }
          else {
            uVar2 = *(uint *)(*(int *)(*(int *)(a1_01 + 4) + 0x20) + 0xac);
            uVar7 = a1_01;
            if (uVar2 != 0) {
              fn_002724(local_c,uVar2,2,unaff_EDI);
              uVar7 = extraout_EDX_02;
            }
            uVar2 = fn_002cd2(local_10,a0,uVar7,0,unaff_EDI);
            uVar6 = extraout_ECX_01;
            uVar7 = extraout_EDX_03;
          }
          *(undefined4 *)(uVar7 + 0x60) = uVar6;
          local_c = local_c + local_10;
          local_14 = local_14 + (local_10 << (*(byte *)(uVar7 + 0x67) & 0x1f));
          local_10 = 0;
        }
        uVar4 = lock_release(uVar4);
        if (uVar4 != 0) {
          return uVar4;
        }
        iVar8 = extraout_EDX_04;
        if ((*(int *)(extraout_EDX_04 + 0xc0) != 0) &&
           (uVar4 = lock_release(local_8), iVar8 = extraout_EDX_05, uVar4 != 0)) {
          return uVar4;
        }
        if (uVar5 != 0) break;
        uVar2 = fn_0015c4(local_c,unaff_EDI);
        if (uVar2 != 0) {
          return uVar2;
        }
        if (a0 == 1) {
          uVar2 = extraout_EDX_06;
          if (((*(uint *)(*(uint *)(extraout_EDX_06 + 0x6c) + 8) >> 1 & 1) == 0) &&
             (uVar5 = fn_001740(*(uint *)(extraout_EDX_06 + 0x6c),unaff_EDI), uVar2 = a0_00,
             uVar5 != 0)) {
            fn_0017f6(a0_00);
            return extraout_ECX_02;
          }
          fn_006054(*(uint *)(*(int *)(uVar2 + 0x6c) + 0x28),*(uint *)(uVar2 + 0x13c),unaff_EDI);
          uVar2 = extraout_EDX_07;
        }
        else {
          fn_006054(local_14,*(uint *)(extraout_EDX_06 + 0x13c),unaff_EDI);
          puVar1 = (uint *)(*(int *)(extraout_EDX_08 + 0x6c) + 8);
          *puVar1 = *puVar1 | 1;
          uVar2 = extraout_EDX_08;
        }
        uVar2 = fn_0017f6(uVar2);
        if (uVar2 != 0) {
          return uVar2;
        }
        local_14 = local_14 + *(int *)(extraout_EDX_09 + 0x13c);
        local_10 = local_10 - 1;
        local_c = local_c + 1;
        iVar8 = extraout_EDX_09;
      }
    } while (uVar2 == 0);
  }
  return uVar2;
}

/* ----------------------------------------------------------------------
 * rbf_writln   @0x0041a8, 65 bytes
 * RBF entry point #16: write a line to a path [file manager entry table]
 * called from: data@00009160
 * calls: rbf_write
 */
 /* WARNING: Unknown calling convention */
 /* RBF entry point #16: write a line to a path [file manager entry table] */
uint rbf_writln(a0, a1)
    uint a0;
    uint a1;
{
  char cVar1;
  uint uVar2;
  int extraout_ECX;
  char *pcVar3;
  uint unaff_EDI;
  int iVar4;
  
  iVar4 = *(int *)(a0 + 0x14);
  pcVar3 = *(char **)(a0 + 0x10);
  if (iVar4 == 0) {
    uVar2 = 0;
  }
  else {
    do {
      iVar4 = iVar4 + -1;
      if (iVar4 == 0) break;
      cVar1 = *pcVar3;
      pcVar3 = pcVar3 + 1;
    } while (cVar1 != '\r');
    *(int *)(a0 + 0x14) = *(int *)(a0 + 0x14) - iVar4;
    uVar2 = rbf_write(a0,unaff_EDI);
    *(uint *)(extraout_ECX + 4) = uVar2;
  }
  return uVar2;
}

/* ----------------------------------------------------------------------
 * rbf_write   @0x0041ea, 633 bytes
 * RBF entry point #15: write bytes to a path [file manager entry table]
 * called from: rbf_writln, data@0000915c
 * calls: fn_0017f6, fn_001af2, fn_002988, fn_002c74, fn_003e04, fn_003ef4, fn_003f8c, fn_004464
 */
 /* WARNING: Unknown calling convention */
 /* RBF entry point #15: write bytes to a path [file manager entry table] */
uint rbf_write(a0, a1)
    uint a0;
    uint a1;
{
  uint *puVar1;
  uint uVar2;
  int iVar3;
  int extraout_EDX;
  int extraout_EDX_00;
  uint unaff_EDI;
  PathDesc *pd;
  uint local_18;
  uint local_14;
  uint local_10;
  uint local_c;
  uint local_8;
  
  if (pd->pd_dev->v_res == 0x4143) {
    return 0xf2;
  }
  if ((pd->pd_mode & 0x1000) != 0) {
    *(undefined1 *)((int)&pd->f_068 + 1) = 1;
    pd->f_078 = pd->f_07c;
  }
  local_c = *(uint *)(a0 + 0x14);
  local_14 = a0;
  local_10 = fn_007da4(local_c,unaff_EDI);
  if (local_10 == 0) {
    uVar2 = pd->f_078 + local_c;
    if (uVar2 < pd->f_078) {
      *(undefined1 *)((int)&pd->f_068 + 1) = 0;
      fn_001af2((uint)pd);
      *(undefined4 *)(local_14 + 4) = 0xf8;
      return 0xf8;
    }
    local_10 = fn_004464(uVar2,unaff_EDI);
    if (local_10 == 0) {
      local_8 = *(uint *)(local_14 + 0x10);
      if (((pd->f_0c0 != 0) && (pd->f_070 != 0)) && ((*(byte *)(pd->f_070 + 0x42) & 1) != 0)) {
        fn_00789c(pd->f_070 + 0x28);
        fn_006054(local_8,local_c,unaff_EDI);
        pd->f_078 = pd->f_078 + local_c;
        lock_release(pd->f_070 + 0x28);
LAB_00004441:
        *(undefined1 *)((int)&pd->f_068 + 1) = 0;
        uVar2 = fn_001af2((uint)pd);
        uVar2 = fn_007b6a(CONCAT31((int3)(uVar2 >> 8),1),unaff_EDI);
        *(uint *)(local_14 + 4) = uVar2;
        return uVar2;
      }
      if ((pd->f_0c0 == 0) && ((pd->f_068 & 0x10000) != 0)) {
        uVar2 = fn_003e04(local_14 + 0x14,0x2bdc,(uint)pd,unaff_EDI);
        return uVar2;
      }
      do {
        if (local_c == 0) goto LAB_00004441;
        if (((local_c < pd->f_13c) || ((pd->f_078 & pd->f_13c - 1) != 0)) || ((local_8 & 1) != 0)) {
          local_10 = fn_002c74((uint)pd);
          if (local_10 != 0) break;
          uVar2 = pd->f_13c - (pd->f_13c - 1 & pd->f_078);
          if (local_c < uVar2) {
            uVar2 = local_c;
          }
          fn_006054(local_8,uVar2,unaff_EDI);
          puVar1 = (uint *)(pd->f_06c + 8);
          *puVar1 = *puVar1 | 1;
          local_10 = 0;
          iVar3 = extraout_EDX;
          if ((extraout_EDX + pd->f_078 & pd->f_13c - 1) == 0) {
            local_10 = fn_0017f6((uint)pd);
            iVar3 = extraout_EDX_00;
          }
        }
        else {
          if (pd->f_0c0 == 0) {
            local_10 = fn_0017f6((uint)pd);
            uVar2 = local_c;
          }
          else {
            local_10 = fn_002988((uint)pd);
            uVar2 = local_c;
          }
          local_c = uVar2;
          if (local_10 != 0) break;
          fn_003ef4((uint)&local_18,(uint)pd,unaff_EDI);
          local_10 = fn_003f8c(2,local_18,(uint)pd,uVar2);
          if (local_10 != 0) break;
          iVar3 = local_18 << (*(byte *)((int)&pd->f_064 + 3) & 0x1f);
        }
        local_c = local_c - iVar3;
        local_8 = local_8 + iVar3;
        pd->f_078 = pd->f_078 + iVar3;
      } while (local_10 == 0);
    }
  }
  *(undefined1 *)((int)&pd->f_068 + 1) = 0;
  fn_001af2((uint)pd);
  *(uint *)(local_14 + 4) = local_10;
  return local_10;
}

/* ----------------------------------------------------------------------
 * fn_004464   @0x004464, 152 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: rbf_write, fn_004dca, fn_006194, fn_006894, fn_007518
 * calls: fn_0000ea, fn_000a76, os_f_time, fn_00789c, lock_release
 */
 /* WARNING: Unknown calling convention */
uint fn_004464(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  uint extraout_ECX;
  uint extraout_ECX_00;
  int extraout_EDX;
  undefined4 extraout_EDX_00;
  uint in_stack_00000004;
  
  if (*(int *)(in_stack_00000004 + 0xc0) != 0) {
    *(byte *)(in_stack_00000004 + 100) = *(byte *)(in_stack_00000004 + 100) | 1;
    if (*(int *)(in_stack_00000004 + 0x70) == 0) {
      return 0xd3;
    }
    fn_00789c(*(int *)(in_stack_00000004 + 0x70) + 0x28);
    uVar1 = os_f_time();
    *(uint *)(extraout_EDX + 0x24) = uVar1;
    *(uint *)(extraout_EDX + 0x20) = uVar1;
    *(uint *)(extraout_EDX + 0x1c) = uVar1;
    lock_release(*(int *)(in_stack_00000004 + 0x70) + 0x28);
    a0 = extraout_ECX;
  }
  if (*(uint *)(in_stack_00000004 + 0x7c) < a0) {
    *(uint *)(in_stack_00000004 + 0x7c) = a0;
    uVar1 = fn_0000ea(in_stack_00000004);
    if (uVar1 != 0) {
      if ((*(ushort *)(in_stack_00000004 + 0x14) & 0x8000) != 0) {
        return uVar1;
      }
      *(undefined4 *)(in_stack_00000004 + 0x7c) = extraout_EDX_00;
      uVar1 = fn_000a76(in_stack_00000004);
      if (uVar1 != 0) {
        return uVar1;
      }
      return extraout_ECX_00;
    }
  }
  return 0;
}

/* ----------------------------------------------------------------------
 * rbf_seek   @0x0044fc, 27 bytes
 * RBF entry point #13: seek to a position [file manager entry table]
 * called from: data@00009154
 * calls: fn_004518
 */
 /* WARNING: Unknown calling convention */
 /* RBF entry point #13: seek to a position [file manager entry table] */
uint rbf_seek(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  uint in_ECX;
  int extraout_ECX;
  
  uVar1 = fn_004518(*(uint *)(a0 + 0x10),in_ECX);
  *(uint *)(extraout_ECX + 4) = uVar1;
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_004518   @0x004518, 310 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: rbf_seek
 * calls: fn_0015c4, fn_0017f6, fn_001b80, fn_00789c, lock_release
 */
 /* WARNING: Removing unreachable block (ram,0x000045fd) */
 /* WARNING: Removing unreachable block (ram,0x0000460b) */
 /* WARNING: Removing unreachable block (ram,0x0000461b) */
 /* WARNING: Removing unreachable block (ram,0x00004626) */
 /* WARNING: Unknown calling convention */
uint fn_004518(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  uint uVar2;
  uint extraout_ECX;
  uint unaff_ESI;
  uint in_stack_00000004;
  
  if ((*(int *)(in_stack_00000004 + 0xc0) == 0) && ((*(byte *)(in_stack_00000004 + 0x6a) & 1) != 0))
  {
    if (*(uint *)(in_stack_00000004 + 0x7c) < a0) {
      return 0xf1;
    }
  }
  else {
    uVar2 = *(uint *)(in_stack_00000004 + 0x78);
    uVar1 = ~(*(int *)(in_stack_00000004 + 0x13c) - 1U);
    if (((uVar2 & uVar1) != (uVar1 & a0)) &&
       ((*(uint *)(in_stack_00000004 + 0x80) <= uVar2 &&
        (uVar2 - *(int *)(in_stack_00000004 + 0x80) < *(uint *)(in_stack_00000004 + 0x88))))) {
      if (*(int *)(in_stack_00000004 + 0xc0) == 0) {
        if (*(short *)(*(int *)(in_stack_00000004 + 4) + 0x56) == 0x4143) goto LAB_00004641;
        uVar2 = fn_0017f6(in_stack_00000004);
      }
      else {
        if (*(int *)(in_stack_00000004 + 0x70) == 0) {
          return 0xd3;
        }
        fn_00789c(*(int *)(in_stack_00000004 + 0x70) + 0x18);
        fn_001b80(((uint)(*(int *)(in_stack_00000004 + 0x78) - *(int *)(in_stack_00000004 + 0x80)
                            ) >> (*(byte *)(in_stack_00000004 + 0x67) & 0x1f)) +
                     *(int *)(in_stack_00000004 + 0x84),in_stack_00000004,unaff_ESI);
        lock_release(*(int *)(in_stack_00000004 + 0x70) + 0x18);
        uVar2 = extraout_ECX;
      }
      if (uVar2 != 0) {
        return uVar2;
      }
    }
  }
LAB_00004641:
  *(uint *)(in_stack_00000004 + 0x78) = a0;
  return 0;
}

/* ----------------------------------------------------------------------
 * rbf_getstat   @0x00464e, 70 bytes
 * RBF entry point #8: get status (I$GetStt) [file manager entry table]
 * called from: data@00009140
 * calls: fn_002b84, fn_004ca4
 */
 /* WARNING: Unknown calling convention */
 /* RBF entry point #8: get status (I$GetStt) [file manager entry table] */
uint rbf_getstat(a0, a1)
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
  
  uVar1 = fn_004ca4((uint)*(ushort *)(a0 + 0x10),(uint)pd,(uint)&g->f_104,in_EDX);
  *(uint *)(a3 + 4) = uVar1;
  uVar2 = a3;
  if (uVar1 == 0xd0) {
    uVar2 = fn_002b84(0,3,a2,a3,in_EDX);
    *(uint *)(extraout_ECX + 4) = uVar2;
    uVar2 = extraout_ECX;
  }
  return *(uint *)(uVar2 + 4);
}

/* ----------------------------------------------------------------------
 * fn_004694   @0x004694, 80 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * calls: fn_005c60, fn_006054
 */
 /* WARNING: Unknown calling convention */
uint fn_004694(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  uint in_EDX;
  int extraout_EDX;
  uint *in_stack_00000004;
  
  if (*in_stack_00000004 < 0x58) {
    uVar1 = 0xb2;
  }
  else {
    *in_stack_00000004 = 0x58;
    uVar1 = fn_005c60(0x58,in_stack_00000004[1],a0,in_EDX);
    if (uVar1 == 0) {
      fn_006054(extraout_EDX + 0x110,0x58,in_EDX);
      uVar1 = 0;
    }
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_0046e4   @0x0046e4, 88 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * calls: fn_005c60, fn_006054
 */
 /* WARNING: Unknown calling convention */
uint fn_0046e4(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  int extraout_EDX;
  uint unaff_ESI;
  uint *in_stack_00000004;
  
  if (*in_stack_00000004 < 0x18) {
    uVar1 = 0xb2;
  }
  else {
    *in_stack_00000004 = 0x18;
    uVar1 = fn_005c60(0x18,in_stack_00000004[1],a0,unaff_ESI);
    if (uVar1 == 0) {
      fn_006054(extraout_EDX + 0x108,0x18,unaff_ESI);
      uVar1 = 0;
    }
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_00473c   @0x00473c, 90 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * calls: fn_005c60, fn_006054
 */
 /* WARNING: Unknown calling convention */
uint fn_00473c(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  int extraout_EDX;
  uint unaff_ESI;
  uint *in_stack_00000004;
  
  if (*in_stack_00000004 < 0x58) {
    uVar1 = 0xb2;
  }
  else {
    *in_stack_00000004 = 0x58;
    uVar1 = fn_005c60(0x58,in_stack_00000004[1],a0,unaff_ESI);
    if (uVar1 == 0) {
      fn_006054(extraout_EDX + *(int *)(extraout_EDX + 0x24) + 0x18,0x58,unaff_ESI);
      uVar1 = 0;
    }
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_004796   @0x004796, 22 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 */
 /* WARNING: Unknown calling convention */
uint fn_004796(a0, a1)
    uint a0;
    uint a1;
{
  int *in_stack_00000004;
  
  *in_stack_00000004 = *(int *)(a0 + 0x7c) - *(int *)(a0 + 0x78);
  return 0;
}

/* ----------------------------------------------------------------------
 * fn_0047ac   @0x0047ac, 21 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 */
 /* WARNING: Unknown calling convention */
uint fn_0047ac(a0)
    uint a0;
{
  undefined4 *in_stack_00000004;
  
  *in_stack_00000004 = *(undefined4 *)(a0 + 0x7c);
  return 0;
}

/* ----------------------------------------------------------------------
 * fn_0047c2   @0x0047c2, 21 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 */
 /* WARNING: Unknown calling convention */
uint fn_0047c2(a0)
    uint a0;
{
  undefined4 *in_stack_00000004;
  
  *in_stack_00000004 = *(undefined4 *)(a0 + 0x78);
  return 0;
}

/* ----------------------------------------------------------------------
 * fn_0047d8   @0x0047d8, 27 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * calls: fn_0039b0
 */
 /* WARNING: Unknown calling convention */
uint fn_0047d8(a0)
    uint a0;
{
  uint uVar1;
  
  uVar1 = fn_0039b0((uint)GHIDRA_STACK_PTR(0xfffffff8),1);
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_0047f4   @0x0047f4, 95 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * calls: fn_005c60, fn_006054
 */
 /* WARNING: Unknown calling convention */
uint fn_0047f4(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  int extraout_ECX;
  uint a1_00;
  uint unaff_ESI;
  uint *in_stack_00000004;
  
  if (*(int *)(a0 + 0x70) == 0) {
    uVar1 = 0xe1;
  }
  else {
    if (*(uint *)(a0 + 0x13c) < *in_stack_00000004) {
      uVar1 = *(uint *)(a0 + 0x13c);
    }
    else {
      uVar1 = *in_stack_00000004;
    }
    uVar1 = fn_005c60(uVar1,in_stack_00000004[1],a0,unaff_ESI);
    if (uVar1 == 0) {
      fn_006054(*(int *)(extraout_ECX + 0x70) + 0x38,a1_00,unaff_ESI);
      uVar1 = 0;
    }
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_004854   @0x004854, 22 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 */
 /* WARNING: Unknown calling convention */
uint fn_004854(a0, a1)
    uint a0;
    uint a1;
{
  undefined4 *in_stack_00000004;
  
  *in_stack_00000004 = *(undefined4 *)(a0 + 0xc0);
  return 0;
}

/* ----------------------------------------------------------------------
 * fn_00486a   @0x00486a, 437 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * calls: fn_002ad0, fn_0035d6, fn_005c60, os_f_id, fn_006054, fn_00789c, lock_release, os_f_srtmem_8db8
 */
 /* WARNING: Unknown calling convention */
uint fn_00486a(a0, a1)
    uint a0;
    uint a1;
{
  int iVar1;
  uint uVar2;
  int extraout_ECX;
  int iVar3;
  int extraout_ECX_00;
  int extraout_ECX_01;
  uint extraout_ECX_02;
  uint extraout_EDX;
  undefined4 extraout_EDX_00;
  uint unaff_EDI;
  uint *in_stack_00000004;
  uint a2;
  undefined4 local_18;
  uint local_14;
  uint local_10;
  uint *local_c;
  uint local_8;
  
  local_c = in_stack_00000004;
  if (*(uint *)(a0 + 0x13c) < *in_stack_00000004) {
    local_8 = *(uint *)(a0 + 0x13c);
  }
  else {
    local_8 = *in_stack_00000004;
  }
  local_10 = a0;
  uVar2 = os_f_id();
  if (uVar2 >> 0x10 == 0) {
    uVar2 = fn_005c60(local_8,local_c[2],local_10,unaff_EDI);
    if (uVar2 == 0) {
      a2 = local_c[1];
      iVar1 = *(int *)(*(int *)(local_10 + 4) + 0x20);
      uVar2 = fn_00789c(iVar1 + 0x60);
      iVar3 = extraout_ECX;
      if (uVar2 == 0) {
        while (iVar3 = *(int *)(iVar3 + 0x94), iVar3 != 0) {
          if ((a2 <= *(uint *)(iVar3 + 0xc0)) &&
             ((a2 != *(uint *)(iVar3 + 0xc0) ||
              (*(int *)(iVar3 + 0x8c) == *(int *)(local_10 + 0x8c))))) break;
        }
        if ((iVar3 == 0) || (a2 != *(uint *)(iVar3 + 0xc0))) {
          uVar2 = lock_release(iVar1 + 0x60);
          if (uVar2 == 0) {
            local_18 = *(undefined4 *)(local_10 + 0x13c);
            uVar2 = os_f_srqmem((uint)&local_18,0,unaff_EDI);
            if (uVar2 == 0) {
              *(uint *)(local_10 + 0x60) = local_14;
              uVar2 = 0x49cd;
              fn_002ad0(1,local_10,unaff_EDI);
              *(undefined4 *)(local_10 + 0x60) = extraout_EDX_00;
              fn_0035d6(local_14,uVar2 & 0xffffff00,a2);
              if (extraout_ECX_01 == 0) {
                fn_006054(local_14,local_8,unaff_EDI);
              }
              os_f_srtmem_8db8(*(uint *)(local_10 + 0x13c),unaff_EDI);
              uVar2 = extraout_ECX_02;
            }
          }
        }
        else {
          uVar2 = lock_release(iVar1 + 0x60);
          if (uVar2 == 0) {
            if (*(int *)(extraout_ECX_00 + 0x70) == 0) {
              uVar2 = 0xe1;
            }
            else {
              fn_006054(*(int *)(extraout_ECX_00 + 0x70) + 0x38,local_8,unaff_EDI);
              uVar2 = extraout_EDX;
            }
          }
        }
      }
    }
  }
  else {
    uVar2 = 0xa4;
  }
  return uVar2;
}

/* ----------------------------------------------------------------------
 * fn_004a20   @0x004a20, 85 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * calls: fn_005c60, fn_006054
 */
 /* WARNING: Unknown calling convention */
uint fn_004a20(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  int extraout_ECX;
  uint unaff_ESI;
  uint *in_stack_00000004;
  
  if (*(int *)(*(int *)(*(int *)(a0 + 4) + 0x20) + 0xac) == 0) {
    uVar1 = 0xd8;
  }
  else {
    uVar1 = fn_005c60(0x50,*in_stack_00000004,a0,unaff_ESI);
    if (uVar1 == 0) {
      fn_006054(extraout_ECX + 0x820,0x50,unaff_ESI);
      uVar1 = 0;
    }
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_004a76   @0x004a76, 50 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * calls: fn_0000c0
 */
 /* WARNING: Unknown calling convention */
uint fn_004a76(a0, a1)
    uint a0;
    uint a1;
{
  undefined4 uVar1;
  uint uVar2;
  int extraout_ECX;
  int extraout_EDX;
  uint *in_stack_00000004;
  
  uVar2 = *in_stack_00000004;
  uVar1 = *(undefined4 *)(uVar2 + 4);
  *(undefined4 *)(uVar2 + 4) = 0xffffffff;
  uVar2 = fn_0000c0(uVar2);
  *(uint *)(extraout_ECX + 4) = uVar2;
  *(undefined4 *)(extraout_EDX + 4) = uVar1;
  return 0;
}

/* ----------------------------------------------------------------------
 * fn_004aa8   @0x004aa8, 336 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * calls: fn_005c60, fn_006054
 */
 /* WARNING: Unknown calling convention */
uint fn_004aa8(a0, a1)
    uint a0;
    uint a1;
{
  uint8 cVar1;
  char cVar2;
  uint uVar3;
  uint uVar4;
  uint unaff_EDI;
  uint8 *pcVar5;
  char *pcVar6;
  uint *in_stack_00000004;
  char *local_10;
  uint local_c;
  uint local_8;
  
  uVar3 = *in_stack_00000004;
  local_10 = (char *)0x0;
  local_c = 0;
  uVar4 = 0xffffffff;
  pcVar5 = (uint8 *)0xdc;
  do {
    if (uVar4 == 0) break;
    uVar4 = uVar4 - 1;
    cVar1 = *pcVar5;
    pcVar5 = pcVar5 + 1;
  } while (cVar1 != (uint8)0);
  local_8 = ~uVar4 - 1;
  if (0x123 < *(uint *)(*(int *)(a0 + 4) + 0x4c)) {
    local_10 = *(char **)(*(int *)(*(int *)(a0 + 4) + 0x20) + 0x120);
  }
  if (local_10 != (char *)0x0) {
    uVar4 = 0xffffffff;
    pcVar6 = local_10;
    do {
      if (uVar4 == 0) break;
      uVar4 = uVar4 - 1;
      cVar2 = *pcVar6;
      pcVar6 = pcVar6 + 1;
    } while (cVar2 != '\0');
    local_c = ~uVar4 - 1;
  }
  *in_stack_00000004 = local_c + local_8 + 1;
  if (uVar3 == 0) {
    uVar4 = 0;
  }
  else {
    uVar4 = fn_005c60(uVar3,in_stack_00000004[1],a0,unaff_EDI);
    if (uVar4 == 0) {
      if (uVar3 <= local_8) {
        local_8 = uVar3 - 1;
      }
      fn_006054(0xdc,local_8,unaff_EDI);
      uVar3 = uVar3 - local_8;
      if ((1 < uVar3) && (local_10 != (char *)0x0)) {
        if (uVar3 <= local_c) {
          local_c = uVar3 - 1;
        }
        fn_006054((uint)local_10,local_c,unaff_EDI);
      }
      fn_006054(0xd8,1,unaff_EDI);
      uVar4 = 0;
    }
  }
  return uVar4;
}
