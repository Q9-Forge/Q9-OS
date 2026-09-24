#ifndef Q9_IOMAN_H
#define Q9_IOMAN_H

/*
 * Q9-IOMAN public, kernel-neutral manager interface.
 *
 * This is an independent Q9 interface, not a Microware source/API header.
 * The kernel trap/register-frame adapter will be added separately.
 */

typedef unsigned short Q9IOMAN_u16;
typedef unsigned long Q9IOMAN_u32;

typedef enum {
    Q9IOMAN_OK = 0,
    Q9IOMAN_E_INVALID_ARGUMENT = 1,
    Q9IOMAN_E_INVALID_PATH = 2,
    Q9IOMAN_E_NO_PATH_SLOTS = 3,
    Q9IOMAN_E_UNSUPPORTED_OPERATION = 4,
    Q9IOMAN_E_NOT_FOUND = 5,
    Q9IOMAN_E_REGISTRY_FULL = 6
} Q9IOMAN_Status;

#define Q9IOMAN_BACKEND_CAPACITY 8

typedef enum {
    Q9IOMAN_OP_READ = 0,
    Q9IOMAN_OP_WRITE,
    Q9IOMAN_OP_READ_LINE,
    Q9IOMAN_OP_WRITE_LINE,
    Q9IOMAN_OP_SEEK,
    Q9IOMAN_OP_GET_STATUS,
    Q9IOMAN_OP_SET_STATUS
} Q9IOMAN_Operation;

typedef struct {
    Q9IOMAN_u32 value;
    Q9IOMAN_u32 transferred;
} Q9IOMAN_Result;

typedef Q9IOMAN_Status (*Q9IOMAN_OpenFn)(void *context,
                                        const char *path,
                                        Q9IOMAN_u16 mode,
                                        Q9IOMAN_u16 *backend_path);
typedef Q9IOMAN_Status (*Q9IOMAN_OperateFn)(void *context,
                                           Q9IOMAN_u16 backend_path,
                                           Q9IOMAN_Operation operation,
                                           Q9IOMAN_u32 arg0,
                                           Q9IOMAN_u32 arg1,
                                           Q9IOMAN_u32 arg2,
                                           Q9IOMAN_Result *result);
typedef Q9IOMAN_Status (*Q9IOMAN_CloseFn)(void *context,
                                         Q9IOMAN_u16 backend_path);

typedef struct Q9IOMAN_BackendOps {
    Q9IOMAN_OpenFn open;
    Q9IOMAN_OperateFn operate;
    Q9IOMAN_CloseFn close;
} Q9IOMAN_BackendOps;

typedef struct {
    Q9IOMAN_u16 state;
    Q9IOMAN_u16 backend_path;
    const Q9IOMAN_BackendOps *backend;
    void *backend_context;
} Q9IOMAN_Path;

typedef struct {
    const char *prefix;
    const Q9IOMAN_BackendOps *backend;
    void *context;
    Q9IOMAN_u16 state;
} Q9IOMAN_BackendRegistration;

typedef struct {
    Q9IOMAN_Path *paths;
    Q9IOMAN_u16 capacity;
    Q9IOMAN_BackendRegistration backends[Q9IOMAN_BACKEND_CAPACITY];
} Q9IOMAN_Manager;

/* Caller owns storage for both manager and path table; no allocator required. */
Q9IOMAN_Status q9ioman_init(Q9IOMAN_Manager *manager,
                            Q9IOMAN_Path *paths,
                            Q9IOMAN_u16 capacity);

/* The caller resolves the device/manager and supplies its backend here. */
Q9IOMAN_Status q9ioman_open(Q9IOMAN_Manager *manager,
                            const Q9IOMAN_BackendOps *backend,
                            void *backend_context,
                            const char *path,
                            Q9IOMAN_u16 mode,
                            Q9IOMAN_u16 *local_path);

/* Prefix strings and backend tables remain owned by the registering caller. */
Q9IOMAN_Status q9ioman_register_backend(Q9IOMAN_Manager *manager,
                                        const char *prefix,
                                        const Q9IOMAN_BackendOps *backend,
                                        void *context);
Q9IOMAN_Status q9ioman_open_resolved(Q9IOMAN_Manager *manager,
                                     const char *path,
                                     Q9IOMAN_u16 mode,
                                     Q9IOMAN_u16 *local_path);

Q9IOMAN_Status q9ioman_operate(Q9IOMAN_Manager *manager,
                               Q9IOMAN_u16 local_path,
                               Q9IOMAN_Operation operation,
                               Q9IOMAN_u32 arg0,
                               Q9IOMAN_u32 arg1,
                               Q9IOMAN_u32 arg2,
                               Q9IOMAN_Result *result);

Q9IOMAN_Status q9ioman_close(Q9IOMAN_Manager *manager,
                             Q9IOMAN_u16 local_path);

#endif
