/* kernel.h - common include */
#ifndef KERNEL_H
#define KERNEL_H
#include "ghidra_compat.h"
#include "kernel_types.h"
/* The static storage pointer of the module lives in register EBX for the whole module in the original binary. */
extern Statics *g;
/* Within a module DS maps the start of the module: absolute data addresses below the code read the module header. */
#define MODULE_DATA32(off) (*(uint32 *)((uint8 *)module_base() + (off)))
extern void *module_base(void);
#include "kernel_data.h"
#include "kernel_proto.h"
#endif
