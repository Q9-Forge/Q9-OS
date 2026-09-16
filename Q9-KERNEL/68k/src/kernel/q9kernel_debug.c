/*
 * q9kernel_debug.c -- Q9-OS kernel debug event routing.
 *
 * This first backend writes memory-allocation events through the existing
 * direct DUART console helper.  The event interface is deliberately kept
 * separate from the memory syscall so a later ring-buffer or pipe backend
 * can replace the console writer without changing allocator behaviour.
 */

#include "q9kernel_config.h"

typedef unsigned long Q9_u32;
typedef unsigned char Q9_u8;

#define Q9K_MEMTRACE_OP_REQUEST 1UL
#define Q9K_MEMTRACE_OP_RETURN  2UL
#define Q9K_MEMTRACE_OP_ALLOC   3UL
#define Q9K_MEMTRACE_OP_FREE    4UL
#define Q9K_MEMTRACE_OP_LARGEST 5UL
#define Q9K_MEMTRACE_OP_PROC_RELEASE 6UL
#define Q9K_MEMTRACE_OP_MODULE_LOAD  7UL

/* $1650-$1663 is F$VModul, $1664/$1668 is F$SetSys, and $1690-$16B4
 * is F$TLink.  The trace state therefore uses the verified gap
 * $16C0-$16F8, directly before the memory-owner table at $1710. */
#define Q9K_MEMTRACE_ENABLED    0x16C0UL
#define Q9K_MEMTRACE_OP         0x16C4UL
#define Q9K_MEMTRACE_REQUEST    0x16C8UL
#define Q9K_MEMTRACE_ADDRESS    0x16CCUL
#define Q9K_MEMTRACE_SIZE       0x16D0UL
#define Q9K_MEMTRACE_ERROR      0x16D4UL
#define Q9K_MEMTRACE_HEAD       0x16D8UL
#define Q9K_MEMTRACE_NAME       0x16DCUL
#define Q9K_MEMTRACE_CONTEXT_HDR 0x16E0UL
#define Q9K_MEMTRACE_CAPTURE_OP  0x16E4UL
#define Q9K_MEMTRACE_CAPTURE_ADDR 0x16E8UL
#define Q9K_MEMTRACE_CAPTURE_SIZE 0x16ECUL
#define Q9K_MEMTRACE_CAPTURE_ERROR 0x16F0UL
#define Q9K_MEMTRACE_CAPTURED 0x16F4UL
#define Q9_D_PROC               0x004CUL
#define Q9K_PROCDESC_MODHDR     0x0038UL
#define Q9K_MH68K_NAME          0x000CUL

extern void Q9K_MemTraceConsoleBackend(void);

static Q9_u32 Q9K_GetU32(Q9_u32 address)
{
    return *(volatile Q9_u32 *)address;
}

static void Q9K_SetU32(Q9_u32 address, Q9_u32 value)
{
    *(volatile Q9_u32 *)address = value;
}

/* Return the current process module name, or zero when no safe descriptor
 * and module header are available yet.  The name field is a module-relative
 * offset in the OS-9/68K header. */
static Q9_u32 Q9K_ModuleName(Q9_u32 header)
{
    Q9_u32 nameOffset;

    if (header == 0UL)
        return 0UL;

    nameOffset = Q9K_GetU32(header + Q9K_MH68K_NAME);
    if (nameOffset == 0UL || nameOffset >= 0x10000UL)
        return 0UL;

    return header + nameOffset;
}

/* Return the current process module name when a process exists. */
static Q9_u32 Q9K_CurrentProcessName(void)
{
    Q9_u32 process = Q9K_GetU32(Q9_D_PROC);
    Q9_u32 header;

    if (process == 0UL)
        return 0UL;

    header = Q9K_GetU32(process + Q9K_PROCDESC_MODHDR);
    return Q9K_ModuleName(header);
}

/* Select a temporary module context for early kernel operations which run
 * before D_Proc identifies a normal user process.  A zero header restores
 * the normal process-based lookup. */
void Q9K_MemTraceSetModule(Q9_u32 header)
{
    Q9K_SetU32(Q9K_MEMTRACE_CONTEXT_HDR, header);
}

void Q9K_MemTraceClearModule(void)
{
    Q9K_SetU32(Q9K_MEMTRACE_CONTEXT_HDR, 0UL);
}

/* Capture allocator events until the surrounding syscall emits its
 * transaction-level event.  This keeps one external syscall on one line. */
void Q9K_MemTraceBeginCapture(void)
{
#if Q9K_MEMTRACE_COMPILETIME
    Q9K_SetU32(Q9K_MEMTRACE_CAPTURED, 1UL);
    Q9K_SetU32(Q9K_MEMTRACE_CAPTURE_OP, 0UL);
    Q9K_SetU32(Q9K_MEMTRACE_CAPTURE_ADDR, 0UL);
    Q9K_SetU32(Q9K_MEMTRACE_CAPTURE_SIZE, 0UL);
    Q9K_SetU32(Q9K_MEMTRACE_CAPTURE_ERROR, 0UL);
#endif
}

void Q9K_MemTraceEndCapture(void)
{
#if Q9K_MEMTRACE_COMPILETIME
    Q9K_SetU32(Q9K_MEMTRACE_CAPTURED, 0UL);
#endif
}

/* Emit one structured memory event through the currently selected backend. */
void Q9K_MemTraceEmit(Q9_u32 operation,
                      Q9_u32 requested,
                      Q9_u32 address,
                      Q9_u32 size,
                      Q9_u32 error,
                      Q9_u32 freeHead)
{
#if !Q9K_MEMTRACE_COMPILETIME
    (void)operation;
    (void)requested;
    (void)address;
    (void)size;
    (void)error;
    (void)freeHead;
    return;
#else
    if (Q9K_GetU32(Q9K_MEMTRACE_ENABLED) == 0UL)
        return;

    if (Q9K_GetU32(Q9K_MEMTRACE_CAPTURED) != 0UL && operation >= 3UL && operation <= 5UL) {
        Q9K_SetU32(Q9K_MEMTRACE_CAPTURE_OP, operation);
        Q9K_SetU32(Q9K_MEMTRACE_CAPTURE_ADDR, address);
        Q9K_SetU32(Q9K_MEMTRACE_CAPTURE_SIZE, size);
        Q9K_SetU32(Q9K_MEMTRACE_CAPTURE_ERROR, error);
        return;
    }

    Q9K_SetU32(Q9K_MEMTRACE_OP, operation);
    Q9K_SetU32(Q9K_MEMTRACE_REQUEST, requested);
    Q9K_SetU32(Q9K_MEMTRACE_ADDRESS, address);
    Q9K_SetU32(Q9K_MEMTRACE_SIZE, size);
    Q9K_SetU32(Q9K_MEMTRACE_ERROR, error);
    Q9K_SetU32(Q9K_MEMTRACE_HEAD, freeHead);
    if (Q9K_GetU32(Q9K_MEMTRACE_CONTEXT_HDR) != 0UL)
        Q9K_SetU32(Q9K_MEMTRACE_NAME,
                   Q9K_ModuleName(Q9K_GetU32(Q9K_MEMTRACE_CONTEXT_HDR)));
    else
        Q9K_SetU32(Q9K_MEMTRACE_NAME, Q9K_CurrentProcessName());
    Q9K_MemTraceConsoleBackend();
#endif
}

/* Runtime control point reserved for the future debug-control syscall. */
void Q9K_MemTraceConfigure(Q9_u32 enabled)
{
    Q9K_SetU32(Q9K_MEMTRACE_ENABLED, enabled != 0UL ? 1UL : 0UL);
}

/* Enable the initial diagnostic backend during kernel initialization. */
void Q9K_MemTraceInit(void)
{
#if Q9K_MEMTRACE_COMPILETIME
    Q9K_MemTraceConfigure(1UL);
#endif
}
