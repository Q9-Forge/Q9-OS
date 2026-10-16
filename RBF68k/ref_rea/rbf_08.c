/*
 * rbf_08.c - rbf part 8
 *
 * Original address range: 0x008660 - 0x019117 of the V4.9 kernel module.
 * Functions 0x8660 to 0x910a (support routines)
 * Generated from the Ghidra export; see tools/gen_src.py. Hand edits belong in hand/*.inc.
 */
#include "kernel.h"

/* ----------------------------------------------------------------------
 * fn_008660   @0x008660, 247 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0054b8, fn_006194, fn_006b50, fn_006f10, fn_007518, fn_0081fa
 * calls: fn_00789c, fn_0079b6, lock_release
 */
 /* WARNING: Unknown calling convention */
uint fn_008660(a0, a1, a2)
    uint a0;
    uint a1;
    uint a2;
{
  int iVar1;
  ushort uVar2;
  uint uVar3;
  uint a0_00;
  int extraout_EDX;
  uint unaff_EDI;
  
  iVar1 = *(int *)(a0 + 0x70);
  uVar3 = *(uint *)(iVar1 + 0x40);
  uVar2 = (ushort)(uVar3 >> 8);
  if ((*(short *)(a0 + 8) == 0) || (*(short *)(iVar1 + 0x46) == *(short *)(a0 + 8))) {
    uVar2 = uVar2 | (ushort)(uVar3 >> 4);
  }
  if ((*(short *)(a0 + 8) == 0) ||
     ((*(short *)(iVar1 + 0x48) == *(short *)(a0 + 10) &&
      ((*(short *)(a0 + 10) != 0 || (*(short *)(iVar1 + 0x46) == *(short *)(a0 + 8))))))) {
    uVar2 = uVar2 | (ushort)uVar3;
  }
  if ((uVar2 & 0xf & (ushort)a1) == ((ushort)a1 & 0xf)) {
    if ((uint)((ushort)a1 & 0x8000) == (uVar3 & 0x8000)) {
      if (((uVar3 & 0x8000) == 0) &&
         (((a1 & 0xffff | *(ushort *)(*(int *)(a0 + 0x98) + 0x14) | uVar3) >> 0xe & 1) != 0)) {
        uVar3 = fn_00789c(*(int *)(*(int *)(a0 + 4) + 0x20) + 0x60);
        if (uVar3 != 0) {
          return uVar3;
        }
        if (extraout_EDX != *(int *)(extraout_EDX + 0x98)) {
          uVar3 = fn_0079b6(a0_00,unaff_EDI);
          return uVar3;
        }
        *(byte *)(extraout_EDX + 0x65) = *(byte *)(extraout_EDX + 0x65) | 2;
        lock_release(a0_00 + 0x60);
      }
      uVar3 = 0;
    }
    else {
      uVar3 = 0xd6;
    }
  }
  else {
    uVar3 = 0xd6;
  }
  return uVar3;
}

/* ----------------------------------------------------------------------
 * fn_008758   @0x008758, 29 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0081fa
 * calls: fn_0017f6
 */
 /* WARNING: Unknown calling convention */
uint fn_008758(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  uint extraout_ECX;
  
  *(byte *)(a0 + 0x6a) = *(byte *)(a0 + 0x6a) & 0xfd;
  uVar1 = fn_0017f6(a0);
  if ((uVar1 == 0) || (extraout_ECX != 0)) {
    uVar1 = extraout_ECX;
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_008776   @0x008776, 104 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_00517a, fn_0054b8, fn_005cfc, fn_0067ea, fn_006894, fn_006f10, fn_0081fa, fn_008840
 * calls: fn_002c74, fn_0039b0, fn_007da4, os_svc_24
 */
 /* WARNING: Unknown calling convention */
uint fn_008776(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  int extraout_ECX;
  uint a0_00;
  int extraout_ECX_00;
  int iVar2;
  int *extraout_EDX;
  int *extraout_EDX_00;
  int *piVar3;
  uint unaff_ESI;
  undefined4 local_8;
  
  local_8 = 0x40;
  uVar1 = fn_007da4(0x40,unaff_ESI);
  if ((uVar1 == 0) && (uVar1 = fn_0039b0((uint)&local_8,unaff_ESI), uVar1 == 0)) {
    iVar2 = extraout_ECX;
    piVar3 = extraout_EDX;
    if (*(int *)(extraout_ECX + 0x6c) == 0) {
      os_svc_24();
      uVar1 = fn_002c74(a0_00);
      iVar2 = extraout_ECX_00;
      piVar3 = extraout_EDX_00;
      if (uVar1 != 0) {
        return uVar1;
      }
    }
    *piVar3 = (*(uint *)(iVar2 + 0x78) & *(int *)(iVar2 + 0x13c) - 1U) +
              *(int *)(*(int *)(iVar2 + 0x6c) + 0x28);
    uVar1 = 0;
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_0087de   @0x0087de, 24 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0081fa
 */
 /* WARNING: Unknown calling convention */
uint fn_0087de(a0)
    uint a0;
{
  *(undefined4 *)(a0 + 0xc4) = *(undefined4 *)(a0 + 0xc0);
  *(undefined4 *)(a0 + 200) = *(undefined4 *)(a0 + 0x78);
  return a0;
}

/* ----------------------------------------------------------------------
 * fn_0087f6   @0x0087f6, 74 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_00517a, fn_0054b8, fn_0081fa
 */
 /* WARNING: Unknown calling convention */
uint fn_0087f6(a0, a1, a2)
    uint a0;
    uint a1;
    uint a2;
{
  uint uVar1;
  int in_stack_00000004;
  
  uVar1 = 0;
  do {
    if (((*(byte *)(in_stack_00000004 + uVar1) ^ *(byte *)(a0 + uVar1)) & 0xdf) != 0) {
      return 1;
    }
    uVar1 = uVar1 + 1;
  } while (uVar1 < a1);
  return (uint)(*(char *)(in_stack_00000004 + uVar1) != '\0');
}

/* ----------------------------------------------------------------------
 * fn_008840   @0x008840, 56 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0054b8, fn_005cfc, fn_006894, fn_006f10, fn_0081fa
 * calls: fn_0017f6, fn_008776
 */
 /* WARNING: Unknown calling convention */
uint fn_008840(a0, a1)
    uint a0;
    uint a1;
{
  uint extraout_ECX;
  uint unaff_ESI;
  uint uVar1;
  
  uVar1 = *(int *)(a0 + 0x78) + 0x40;
  if ((uVar1 & *(int *)(a0 + 0x13c) - 1U) == 0) {
    fn_0017f6(a0);
    a0 = extraout_ECX;
  }
  *(uint *)(a0 + 0x78) = uVar1;
  uVar1 = fn_008776(a0,unaff_ESI);
  return uVar1;
}

/* ----------------------------------------------------------------------
 * fn_008878   @0x008878, 32 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_006894, fn_007518
 * calls: fn_00286a, fn_002888
 */
 /* WARNING: Unknown calling convention */
uint fn_008878(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  uint uVar2;
  int extraout_ECX;
  int extraout_ECX_00;
  uint in_stack_00000004;
  
  uVar1 = fn_00286a(in_stack_00000004);
  uVar2 = 0;
  if (uVar1 != 0) {
    uVar2 = fn_002888(*(uint *)(extraout_ECX + 0x3c));
    *(uint *)(extraout_ECX_00 + 0x3c) = uVar2;
  }
  return uVar2;
}

/* ----------------------------------------------------------------------
 * lock_create   @0x008898, 262 bytes
 * same code as lock_create in IOMAN (shared support routine, match 0.99)
 * called from: fn_0076fa
 * calls: os_f_srqmem
 */
 /* WARNING: Unknown calling convention */
 /* same code as lock_create in IOMAN (shared support routine, match 0.99) */
uint lock_create(a0)
    uint a0;
{
  ushort uVar1;
  undefined4 *puVar2;
  uint uVar3;
  Statics *g;
  uint unaff_EDI;
  uint local_20;
  uint *local_1c;
  uint local_18;
  uint *local_14;
  uint **local_10;
  uint local_c;
  undefined4 *local_8;
  
  local_14 = &g->f_17c;
  local_10 = &local_1c;
  while( true ) {
    local_1c = local_14 + 2;
    uVar1 = (ushort)((short)*local_14 - 8U) >> 4;
    while (uVar1 = uVar1 - 1, local_8 = (undefined4 *)a0, -1 < (short)uVar1) {
      if (*local_1c == 0) {
        local_c = 0;
        goto LAB_0000895e;
      }
      local_1c = local_1c + 4;
    }
    if (local_14[1] == 0) break;
    local_14 = (uint *)local_14[1];
  }
  local_14 = local_14 + 1;
  local_20 = 0x800;
  local_18 = os_f_srqmem((uint)&local_20,0,unaff_EDI);
  if (local_18 == 0) {
    puVar2 = (undefined4 *)*local_14;
    uVar3 = local_20 >> 2;
    while (uVar3 != 0) {
      *puVar2 = 0;
      puVar2 = puVar2 + 1;
      uVar3 = uVar3 - 1;
    }
    *(uint *)*local_14 = local_20;
    *local_10 = (uint *)(*local_14 + 8);
    local_18 = 0;
  }
  local_c = local_18;
LAB_0000895e:
  if (local_c == 0) {
    *local_1c = (uint)local_1c;
    *local_8 = local_1c;
    local_1c[1] = 0;
    local_1c[2] = (uint)(local_1c + -0x13);
    local_1c[3] = (uint)(local_1c + -0x13);
    g->f_048 = g->f_048 + 1;
    local_c = 0;
  }
  return local_c;
}

/* ----------------------------------------------------------------------
 * os_f_srtmem   @0x00899e, 99 bytes
 * kernel call wrapper: builds the parameter block for F_SRTMEM (code 0x38, system state) and enters the kernel
 * called from: fn_00344a, fn_0076fa, fn_0079d2
 */
 /* WARNING: Unknown calling convention */
 /* kernel call wrapper: builds the parameter block for F_SRTMEM (code 0x38, system state) and enters the kernel */
uint os_f_srtmem(a0)
    uint a0;
{
  code *pcVar1;
  uint uVar2;
  uint extraout_EDX;
  Statics *g;
  
  *(undefined4 *)a0 = 0;
  g->f_048 = g->f_048 - 1;
  if (g->f_048 == 0) {
    uVar2 = g->f_180;
    g->f_180 = 0;
    while (uVar2 != 0) {
      pcVar1 = (code *)swi(0xff);
      (*pcVar1)();
      uVar2 = extraout_EDX;
    }
  }
  return 0;
}

/* ----------------------------------------------------------------------
 * lock_enqueue_by_priority   @0x008a02, 54 bytes
 * same code as lock_enqueue_by_priority in IOMAN (shared support routine, match 1.00)
 * called from: fn_008c20
 */
 /* WARNING: Unknown calling convention */
 /* same code as lock_enqueue_by_priority in IOMAN (shared support routine, match 1.00) */
uint lock_enqueue_by_priority(a0)
    uint a0;
{
  int iVar1;
  int iVar2;
  int in_stack_00000004;
  
  for (iVar2 = *(int *)(a0 + 8);
      (iVar2 != a0 - 0x4c && (*(uint *)(in_stack_00000004 + 0x20) <= *(uint *)(iVar2 + 0x20)));
      iVar2 = *(int *)(iVar2 + 0x54)) {
  }
  iVar1 = *(int *)(iVar2 + 0x58);
  *(int *)(in_stack_00000004 + 0x58) = iVar1;
  *(int *)(iVar1 + 0x54) = in_stack_00000004;
  *(int *)(in_stack_00000004 + 0x54) = iVar2;
  *(int *)(iVar2 + 0x58) = in_stack_00000004;
  return 0;
}

/* ----------------------------------------------------------------------
 * fn_008a38   @0x008a38, 280 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * calls: get_current_process
 */
 /* WARNING: Unknown calling convention */
uint fn_008a38(a0, a1)
    uint a0;
    uint a1;
{
  int iVar1;
  code *pcVar2;
  uint uVar3;
  int iVar4;
  int extraout_EDX;
  undefined1 *puVar5;
  byte bVar6;
  byte bVar7;
  byte in_AF;
  byte bVar8;
  byte bVar9;
  byte bVar10;
  byte in_TF;
  byte bVar11;
  byte in_IF;
  byte bVar12;
  byte bVar13;
  byte bVar14;
  byte in_NT;
  byte bVar15;
  byte in_AC;
  byte bVar16;
  byte in_VIF;
  byte bVar17;
  byte in_VIP;
  byte bVar18;
  byte in_ID;
  byte bVar19;
  undefined4 *in_stack_00000004;
  uint local_8;
  
  bVar13 = 0;
  puVar5 = GHIDRA_STACK_PTR(0xffffffcc);
  uVar3 = *(uint *)a0;
  bVar6 = a0 < uVar3;
  bVar14 = SBORROW4(a0,uVar3);
  uVar3 = a0 - uVar3;
  bVar10 = (int)uVar3 < 0;
  bVar9 = uVar3 == 0;
  bVar7 = (POPCOUNT(uVar3 & 0xff) & 1U) == 0;
  if ((bool)bVar9) {
    uVar3 = get_current_process();
    local_8 = get_eflags();
    if (*(int *)(a0 + 4) == 0) {
      *(undefined4 *)(a0 + 4) = *(undefined4 *)(uVar3 + 4);
    }
    else {
      puVar5 = GHIDRA_STACK_PTR(0xffffffcc);
      if (*(int *)(uVar3 + 4) != *(int *)(a0 + 4)) {
        for (iVar4 = *(int *)(a0 + 8);
            (iVar4 != a0 - 0x4c && (*(uint *)(uVar3 + 0x20) <= *(uint *)(iVar4 + 0x20)));
            iVar4 = *(int *)(iVar4 + 0x54)) {
        }
        iVar1 = *(int *)(iVar4 + 0x58);
        *(int *)(uVar3 + 0x58) = iVar1;
        *(uint *)(iVar1 + 0x54) = uVar3;
        *(int *)(uVar3 + 0x54) = iVar4;
        *(uint *)(iVar4 + 0x58) = uVar3;
        if ((*(uint *)(uVar3 + 0x24) >> 3 & 1) == 0) {
          *(ushort *)(uVar3 + 0x84) = *(ushort *)(uVar3 + 0x84) & 0xff7f;
        }
        bVar15 = (in_NT & 1) != 0;
        bVar14 = (bVar14 & 1) != 0;
        bVar13 = (bVar13 & 1) != 0;
        bVar12 = (in_IF & 1) != 0;
        bVar11 = (in_TF & 1) != 0;
        bVar10 = (bVar10 & 1) != 0;
        bVar9 = (bVar9 & 1) != 0;
        bVar8 = (in_AF & 1) != 0;
        bVar7 = (bVar7 & 1) != 0;
        bVar6 = (bVar6 & 1) != 0;
        bVar19 = (in_ID & 1) != 0;
        bVar16 = (in_AC & 1) != 0;
        bVar18 = 0;
        bVar17 = 0;
        pcVar2 = (code *)swi(0xff);
        (*pcVar2)();
        puVar5 = GHIDRA_STACK_PTR(0xffffffd0);
        local_8 = (uint)(bVar15 & 1) * 0x4000 | (uint)(bVar14 & 1) * 0x800 |
                  (uint)(bVar13 & 1) * 0x400 | (uint)(bVar12 & 1) * 0x200 |
                  (uint)(bVar11 & 1) * 0x100 | (uint)(bVar10 & 1) * 0x80 | (uint)(bVar9 & 1) * 0x40
                  | (uint)(bVar8 & 1) * 0x10 | (uint)(bVar7 & 1) * 4 | (uint)(bVar6 & 1) |
                  (uint)(bVar19 & 1) * 0x200000 | (uint)(bVar18 & 1) * 0x100000 |
                  (uint)(bVar17 & 1) * 0x80000 | (uint)(bVar16 & 1) * 0x40000;
        if (*(int *)(extraout_EDX + 4) != *(int *)(a0 + 4)) {
          *(undefined4 *)(*(int *)(extraout_EDX + 0x58) + 0x54) =
               *(undefined4 *)(extraout_EDX + 0x54);
          *(undefined4 *)(*(int *)(extraout_EDX + 0x54) + 0x58) =
               *(undefined4 *)(extraout_EDX + 0x58);
          *(undefined4 *)(extraout_EDX + 0x58) = 0;
          *(undefined4 *)(extraout_EDX + 0x54) = 0;
          *in_stack_00000004 = 0;
          return 0xe9;
        }
      }
    }
    *(uint *)(puVar5 + -4) = local_8;
    *in_stack_00000004 = 0;
    uVar3 = 0;
  }
  else {
    uVar3 = 0xc6;
  }
  return uVar3;
}

/* ----------------------------------------------------------------------
 * lock_release   @0x008b94, 140 bytes
 * same code as lock_release in IOMAN (shared support routine, match 0.98)
 * called from: fn_0000ea, fn_00057e, fn_0005ee, fn_000770, fn_000a76, fn_0011b0, fn_0015c4, fn_0017f6
 */
 /* WARNING: Unknown calling convention */
 /* same code as lock_release in IOMAN (shared support routine, match 0.98) */
uint lock_release(a0)
    uint a0;
{
  code *pcVar1;
  uint uVar2;
  int iVar3;
  
  if (a0 == *(uint *)a0) {
    if (a0 - 0x4c == *(int *)(a0 + 8)) {
      *(undefined4 *)(a0 + 4) = 0;
      iVar3 = 0;
    }
    else {
      iVar3 = *(int *)(a0 + 8);
      *(undefined4 *)(a0 + 4) = *(undefined4 *)(iVar3 + 4);
      *(undefined4 *)(*(int *)(iVar3 + 0x58) + 0x54) = *(undefined4 *)(iVar3 + 0x54);
      *(undefined4 *)(*(int *)(iVar3 + 0x54) + 0x58) = *(undefined4 *)(iVar3 + 0x58);
      *(undefined4 *)(iVar3 + 0x58) = 0;
      *(undefined4 *)(iVar3 + 0x54) = 0;
    }
    if (iVar3 != 0) {
      pcVar1 = (code *)swi(0xff);
      (*pcVar1)();
    }
    uVar2 = 0;
  }
  else {
    uVar2 = 0xc6;
  }
  return uVar2;
}

/* ----------------------------------------------------------------------
 * fn_008c20   @0x008c20, 262 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * calls: lock_enqueue_by_priority, os_f_aproc, get_current_process
 */
 /* WARNING: Unknown calling convention */
uint fn_008c20(a0, a1)
    uint a0;
    uint a1;
{
  int iVar1;
  code *pcVar2;
  uint uVar3;
  int extraout_ECX;
  int extraout_EDX;
  undefined4 *in_stack_00000004;
  
  if (a0 == *(uint *)a0) {
    if (a0 - 0x4c == *(int *)(a0 + 8)) {
      *(undefined4 *)(a0 + 4) = 0;
    }
    else {
      iVar1 = *(int *)(a0 + 8);
      *(undefined4 *)(a0 + 4) = *(undefined4 *)(iVar1 + 4);
      *(undefined4 *)(*(int *)(iVar1 + 0x58) + 0x54) = *(undefined4 *)(iVar1 + 0x54);
      *(undefined4 *)(*(int *)(iVar1 + 0x54) + 0x58) = *(undefined4 *)(iVar1 + 0x58);
      *(undefined4 *)(iVar1 + 0x58) = 0;
      *(undefined4 *)(iVar1 + 0x54) = 0;
    }
    get_current_process();
    lock_enqueue_by_priority(a0);
    if ((*(uint *)(extraout_ECX + 0x24) >> 3 & 1) == 0) {
      *(ushort *)(extraout_ECX + 0x84) = *(ushort *)(extraout_ECX + 0x84) & 0xff7f;
    }
    if (extraout_EDX != 0) {
      os_f_aproc(*(uint *)(extraout_EDX + 4));
    }
    pcVar2 = (code *)swi(0xff);
    (*pcVar2)();
    uVar3 = get_current_process();
    if (*(int *)(a0 + 4) == *(int *)(uVar3 + 4)) {
      *in_stack_00000004 = 0;
      uVar3 = 0;
    }
    else {
      *(undefined4 *)(*(int *)(uVar3 + 0x58) + 0x54) = *(undefined4 *)(uVar3 + 0x54);
      *(undefined4 *)(*(int *)(uVar3 + 0x54) + 0x58) = *(undefined4 *)(uVar3 + 0x58);
      *(undefined4 *)(uVar3 + 0x58) = 0;
      *(undefined4 *)(uVar3 + 0x54) = 0;
      *in_stack_00000004 = 0;
      uVar3 = 0xe9;
    }
  }
  else {
    uVar3 = 0xc6;
  }
  return uVar3;
}

/* ----------------------------------------------------------------------
 * fn_008d26   @0x008d26, 146 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_007da4
 * calls: get_current_process
 */
 /* WARNING: Unknown calling convention */
uint fn_008d26(a0)
    uint a0;
{
  code *pcVar1;
  uint *puVar2;
  uint uVar3;
  uint uVar4;
  uint *puVar5;
  byte in_AF;
  byte in_TF;
  byte in_IF;
  byte bVar6;
  byte in_NT;
  byte in_AC;
  byte in_VIF;
  byte in_VIP;
  byte in_ID;
  uint uVar7;
  uint uStack_34;
  uint local_1c;
  int local_10;
  
  bVar6 = 0;
  uVar3 = get_current_process();
  if (a0 == 0) {
    *(undefined4 *)(uVar3 + 0x48) = 0;
    uVar3 = 0;
  }
  else {
    uVar7 = get_eflags();
    uVar4 = a0;
    puVar2 = (uint *)GHIDRA_STACK_PTR(0xffffffd0);
    do {
      puVar5 = puVar2;
      if (uVar4 == 0) {
        *(uint *)(uVar3 + 0x48) = a0;
        puVar5[-1] = uVar7;
        return 0;
      }
      pcVar1 = (code *)swi(0xff);
      (*pcVar1)();
      if (local_1c != 0) {
        *puVar5 = uVar7;
        return local_1c;
      }
      uVar4 = *(uint *)(local_10 + 0x48);
      puVar2 = puVar5 + 1;
    } while (uVar4 != *(uint *)(uVar3 + 4));
    *puVar5 = uVar7;
    uVar3 = 0xfe;
  }
  return uVar3;
}

/* ----------------------------------------------------------------------
 * os_f_srtmem_8db8   @0x008db8, 48 bytes
 * kernel call wrapper: builds the parameter block for F_SRTMEM (code 0x38, system state) and enters the kernel
 * called from: fn_0000ea, fn_0005ee, fn_000770, fn_000d72, fn_0011b0, fn_0017f6, fn_001bc8, fn_00486a
 */
 /* WARNING: Unknown calling convention */
 /* kernel call wrapper: builds the parameter block for F_SRTMEM (code 0x38, system state) and enters the kernel */
uint os_f_srtmem_8db8(a0, a1)
    uint a0;
    uint a1;
{
  code *pcVar1;
  int extraout_ECX;
  
  pcVar1 = (code *)swi(0xff);
  (*pcVar1)(0x38);
  return *(uint *)(extraout_ECX + 4);
}

/* ----------------------------------------------------------------------
 * os_f_srqmem   @0x008de8, 84 bytes
 * kernel call wrapper: builds the parameter block for F_SRQMEM (code 0x37, system state) and enters the kernel
 * called from: fn_0000ea, fn_0005d2, fn_000770, fn_000d72, fn_0011b0, fn_0013a8, fn_0015c4, fn_001a00
 */
 /* WARNING: Unknown calling convention */
 /* kernel call wrapper: builds the parameter block for F_SRQMEM (code 0x37, system state) and enters the kernel */
uint os_f_srqmem(a0, a1, a2)
    uint a0;
    uint a1;
    uint a2;
{
  undefined4 uVar1;
  uint uVar2;
  code *pcVar3;
  int extraout_ECX;
  undefined4 *extraout_EDX;
  undefined4 *in_stack_00000004;
  undefined4 local_10;
  
  uVar1 = *(undefined4 *)a0;
  pcVar3 = (code *)swi(0xff);
  (*pcVar3)();
  uVar2 = *(uint *)(extraout_ECX + 4);
  if (uVar2 == 0) {
    *in_stack_00000004 = local_10;
    *extraout_EDX = uVar1;
  }
  return uVar2;
}

/* ----------------------------------------------------------------------
 * os_f_prsnam   @0x008e5a, 84 bytes
 * kernel call wrapper: builds the parameter block for F_PRSNAM (code 0x2d, system state) and enters the kernel
 * called from: fn_00517a, fn_006194, fn_00851a
 */
 /* WARNING: Unknown calling convention */
 /* kernel call wrapper: builds the parameter block for F_PRSNAM (code 0x2d, system state) and enters the kernel */
uint os_f_prsnam(a0, a1, a2, a3)
    uint a0;
    uint a1;
    uint a2;
    uint a3;
{
  uint uVar1;
  code *pcVar2;
  int extraout_ECX;
  undefined4 *extraout_EDX;
  undefined4 local_14;
  undefined1 local_10;
  undefined4 local_e;
  
  pcVar2 = (code *)swi(0xff);
  (*pcVar2)();
  uVar1 = *(uint *)(extraout_ECX + 4);
  *extraout_EDX = local_14;
  *(undefined1 *)a1 = local_10;
  *(undefined4 *)a2 = local_e;
  return uVar1;
}

/* ----------------------------------------------------------------------
 * os_i_getpd   @0x008eb8, 60 bytes
 * kernel call wrapper: builds the parameter block for I_GETPD (code 0x81, system state) and enters the kernel
 * called from: fn_0054b8
 */
 /* WARNING: Unknown calling convention */
 /* kernel call wrapper: builds the parameter block for I_GETPD (code 0x81, system state) and enters the kernel */
uint os_i_getpd(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  code *pcVar2;
  int extraout_ECX;
  undefined4 *extraout_EDX;
  undefined4 local_8;
  
  pcVar2 = (code *)swi(0xff);
  (*pcVar2)();
  uVar1 = *(uint *)(extraout_ECX + 4);
  if (uVar1 == 0) {
    *extraout_EDX = local_8;
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * os_i_close   @0x008ef4, 42 bytes
 * kernel call wrapper: builds the parameter block for I_CLOSE (code 0x98, system state) and enters the kernel
 * called from: fn_005dee
 */
 /* WARNING: Unknown calling convention */
 /* kernel call wrapper: builds the parameter block for I_CLOSE (code 0x98, system state) and enters the kernel */
uint os_i_close(a0)
    uint a0;
{
  code *pcVar1;
  int extraout_ECX;
  
  pcVar1 = (code *)swi(0xff);
  (*pcVar1)(0x98);
  return *(uint *)(extraout_ECX + 4);
}

/* ----------------------------------------------------------------------
 * os_f_findpd   @0x008f1e, 60 bytes
 * kernel call wrapper: builds the parameter block for F_FINDPD (code 0x12, system state) and enters the kernel
 * called from: fn_005c60
 */
 /* WARNING: Unknown calling convention */
 /* kernel call wrapper: builds the parameter block for F_FINDPD (code 0x12, system state) and enters the kernel */
uint os_f_findpd(a0, a1)
    uint a0;
    uint a1;
{
  uint uVar1;
  code *pcVar2;
  int extraout_ECX;
  undefined4 *extraout_EDX;
  undefined4 local_8;
  
  pcVar2 = (code *)swi(0xff);
  (*pcVar2)();
  uVar1 = *(uint *)(extraout_ECX + 4);
  if (uVar1 == 0) {
    *extraout_EDX = local_8;
  }
  return uVar1;
}

/* ----------------------------------------------------------------------
 * os_f_chkmem   @0x008f5a, 68 bytes
 * kernel call wrapper: builds the parameter block for F_CHKMEM (code 0x6, system state) and enters the kernel
 * called from: fn_005c60
 */
 /* WARNING: Unknown calling convention */
 /* kernel call wrapper: builds the parameter block for F_CHKMEM (code 0x6, system state) and enters the kernel */
uint os_f_chkmem(a0, a1, a2, a3)
    uint a0;
    uint a1;
    uint a2;
    uint a3;
{
  code *pcVar1;
  int extraout_ECX;
  
  pcVar1 = (code *)swi(0xff);
  (*pcVar1)();
  return *(uint *)(extraout_ECX + 4);
}

/* ----------------------------------------------------------------------
 * os_f_send   @0x008f9e, 48 bytes
 * kernel call wrapper: builds the parameter block for F_SEND (code 0x30, system state) and enters the kernel
 */
 /* WARNING: Unknown calling convention */
 /* kernel call wrapper: builds the parameter block for F_SEND (code 0x30, system state) and enters the kernel */
uint os_f_send(a0, a1)
    uint a0;
    uint a1;
{
  code *pcVar1;
  int extraout_ECX;
  
  pcVar1 = (code *)swi(0xff);
  (*pcVar1)(0x30);
  return *(uint *)(extraout_ECX + 4);
}

/* ----------------------------------------------------------------------
 * os_f_sleep   @0x008fce, 71 bytes
 * kernel call wrapper: builds the parameter block for F_SLEEP (code 0x36, system state) and enters the kernel
 */
 /* WARNING: Unknown calling convention */
 /* kernel call wrapper: builds the parameter block for F_SLEEP (code 0x36, system state) and enters the kernel */
uint os_f_sleep(a0, a1)
    uint a0;
    uint a1;
{
  undefined4 uVar1;
  uint uVar2;
  code *pcVar3;
  int extraout_ECX;
  undefined4 *extraout_EDX;
  undefined4 *in_stack_00000004;
  undefined4 local_8;
  
  uVar1 = *(undefined4 *)a0;
  pcVar3 = (code *)swi(0xff);
  (*pcVar3)();
  uVar2 = *(uint *)(extraout_ECX + 4);
  *extraout_EDX = uVar1;
  *in_stack_00000004 = local_8;
  return uVar2;
}

/* ----------------------------------------------------------------------
 * os_svc_24   @0x009016, 39 bytes
 * kernel call wrapper: builds the parameter block for SVC_24 (code 0x18, system state) and enters the kernel
 * called from: fn_008776
 */
 /* WARNING: Unknown calling convention */
 /* kernel call wrapper: builds the parameter block for SVC_24 (code 0x18, system state) and enters the kernel */
uint os_svc_24()
{
  code *pcVar1;
  int extraout_ECX;
  
  pcVar1 = (code *)swi(0xff);
  (*pcVar1)(0x18);
  return *(uint *)(extraout_ECX + 4);
}

/* ----------------------------------------------------------------------
 * os_f_aproc   @0x00903e, 42 bytes
 * kernel call wrapper: builds the parameter block for F_APROC (code 0x4, system state) and enters the kernel
 * called from: fn_008c20
 */
 /* WARNING: Unknown calling convention */
 /* kernel call wrapper: builds the parameter block for F_APROC (code 0x4, system state) and enters the kernel */
uint os_f_aproc(a0)
    uint a0;
{
  code *pcVar1;
  int extraout_ECX;
  
  pcVar1 = (code *)swi(0xff);
  (*pcVar1)(4);
  return *(uint *)(extraout_ECX + 4);
}

/* ----------------------------------------------------------------------
 * fn_00909f   @0x00909f, 20 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_001fda, fn_002180, fn_006054
 */
 /* WARNING: Unknown calling convention */
uint fn_00909f(a0)
    uint a0;
{
  undefined1 *puVar1;
  undefined1 *in_stack_00000004;
  int in_stack_00000008;
  
  puVar1 = (undefined1 *)a0;
  for (; in_stack_00000008 != 0; in_stack_00000008 = in_stack_00000008 + -1) {
    *in_stack_00000004 = *puVar1;
    puVar1 = puVar1 + 1;
    in_stack_00000004 = in_stack_00000004 + 1;
  }
  return a0;
}

/* ----------------------------------------------------------------------
 * fn_0090b3   @0x0090b3, 21 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_001fda, fn_002180, fn_006054
 */
 /* WARNING: Unknown calling convention */
uint fn_0090b3(a0)
    uint a0;
{
  undefined2 *puVar1;
  undefined2 *in_stack_00000004;
  int in_stack_00000008;
  
  puVar1 = (undefined2 *)a0;
  for (; in_stack_00000008 != 0; in_stack_00000008 = in_stack_00000008 + -1) {
    *in_stack_00000004 = *puVar1;
    puVar1 = puVar1 + 1;
    in_stack_00000004 = in_stack_00000004 + 1;
  }
  return a0;
}

/* ----------------------------------------------------------------------
 * fn_0090c8   @0x0090c8, 20 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_001fda, fn_002180, fn_006054
 */
 /* WARNING: Unknown calling convention */
uint fn_0090c8(a0)
    uint a0;
{
  undefined4 *puVar1;
  undefined4 *in_stack_00000004;
  int in_stack_00000008;
  
  puVar1 = (undefined4 *)a0;
  for (; in_stack_00000008 != 0; in_stack_00000008 = in_stack_00000008 + -1) {
    *in_stack_00000004 = *puVar1;
    puVar1 = puVar1 + 1;
    in_stack_00000004 = in_stack_00000004 + 1;
  }
  return a0;
}

/* ----------------------------------------------------------------------
 * fn_0090dc   @0x0090dc, 28 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_001fda, fn_002180, fn_006054
 */
 /* WARNING: Unknown calling convention */
uint fn_0090dc(a0)
    uint a0;
{
  undefined1 *puVar1;
  undefined1 *in_stack_00000004;
  int in_stack_00000008;
  
  puVar1 = (undefined1 *)a0;
  while( true ) {
    in_stack_00000004 = in_stack_00000004 + -1;
    puVar1 = puVar1 + -1;
    if (in_stack_00000008 == 0) break;
    in_stack_00000008 = in_stack_00000008 + -1;
    *in_stack_00000004 = *puVar1;
  }
  return a0;
}

/* ----------------------------------------------------------------------
 * fn_0090f8   @0x0090f8, 18 bytes
 * Not yet analysed in detail (decompiler output, names are provisional).
 * called from: fn_0011b0, fn_0014e6, fn_00380a, fn_0058f4, fn_006194, fn_006894, fn_006b50, fn_006f10
 */
 /* WARNING: Unknown calling convention */
uint fn_0090f8(a0)
    uint a0;
{
  uint uVar1;
  uint in_stack_00000004;
  uint *in_stack_00000008;
  
  for (uVar1 = in_stack_00000004 >> 2; uVar1 != 0; uVar1 = uVar1 - 1) {
    *in_stack_00000008 = a0;
    in_stack_00000008 = in_stack_00000008 + 1;
  }
  return a0;
}

/* ----------------------------------------------------------------------
 * get_current_process   @0x00910a, 13 bytes
 * same code as get_current_process in IOMAN (shared support routine, match 1.00)
 * called from: fn_00789c, fn_007b6a, fn_008a38, fn_008c20, fn_008d26
 */
 /* WARNING: Unknown calling convention */
 /* same code as get_current_process in IOMAN (shared support routine, match 1.00) */
uint get_current_process()
{
  int *in_FS_OFFSET;
  
  return *(uint *)(*in_FS_OFFSET + 0x80);
}
