/* ghidra_compat.h - names and helpers used by the decompiler derived function bodies */
#ifndef GHIDRA_COMPAT_H
#define GHIDRA_COMPAT_H
typedef unsigned char undefined, undefined1, byte, uchar;
typedef unsigned short undefined2, ushort;
typedef unsigned int undefined4, uint, ulong;
typedef unsigned long long undefined8, ulonglong;
typedef long long longlong;
typedef signed char sbyte;
typedef int bool_t; typedef int bool;
typedef unsigned int code();
typedef unsigned int undefined3, undefined5, undefined6, undefined7;
typedef unsigned int int3, uint3, int5, uint5, int6, uint6, int7, uint7;
typedef unsigned long long BADSPACEBASE_t;
#define __regparm1
#define __regparm2
#define __regparm3
#define __stdcall
#define __cdecl
#define CONCAT44(a,b) ((((unsigned long long)(unsigned int)(a))<<32)|(unsigned int)(b))
#define CONCAT22(a,b) ((((unsigned int)(unsigned short)(a))<<16)|(unsigned short)(b))
#define CONCAT11(a,b) ((((unsigned short)(unsigned char)(a))<<8)|(unsigned char)(b))
#define CONCAT31(a,b) ((((unsigned int)(a))<<8)|(unsigned char)(b))
#define CONCAT13(a,b) ((((unsigned int)(unsigned char)(a))<<24)|((unsigned int)(b)&0xffffff))
#define SUB41(a,n) ((unsigned char)(((unsigned int)(a))>>(8*(n))))
#define SUB42(a,n) ((unsigned short)(((unsigned int)(a))>>(8*(n))))
#define SUB84(a,n) ((unsigned int)(((unsigned long long)(a))>>(8*(n))))
#define SUB82(a,n) ((unsigned short)(((unsigned long long)(a))>>(8*(n))))
#define SBORROW4(a,b) (((int)(a)-(int)(b)) > (int)(a))
#define SCARRY4(a,b) (((int)(a)+(int)(b)) < (int)(a))
#define CARRY4(a,b) (((unsigned int)(a)+(unsigned int)(b)) < (unsigned int)(a))
#define halt_baddata() do { } while (0)
#ifndef true
#define true 1
#define false 0
#endif

extern unsigned char ghidra_stack_dummy[];
#define GHIDRA_STACK_PTR(n) ((void *)(ghidra_stack_dummy + (n)))
#endif
