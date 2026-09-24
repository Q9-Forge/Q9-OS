#include <stdio.h>
#include <string.h>

#include "qioman.h"
#include "qioman_kernel.h"
#include "qioman_system.h"

typedef struct {
    Q9IOMAN_u16 opened_path;
    Q9IOMAN_u16 last_path;
    Q9IOMAN_Operation last_operation;
    Q9IOMAN_u32 last_arg0;
    Q9IOMAN_u32 last_arg1;
    Q9IOMAN_u32 last_arg2;
    Q9IOMAN_u16 open_calls;
    Q9IOMAN_u16 operate_calls;
    Q9IOMAN_u16 close_calls;
    Q9IOMAN_Status close_status;
} MockBackend;

static int failures;

static void check(const char *name, int condition)
{
    if (condition) {
        printf("[OK]   %s\n", name);
    } else {
        printf("[FAIL] %s\n", name);
        ++failures;
    }
}

static Q9IOMAN_Status mock_open(void *opaque,
                               const char *name,
                               Q9IOMAN_u16 mode,
                               Q9IOMAN_u16 *backend_path)
{
    MockBackend *mock = (MockBackend *)opaque;
    ++mock->open_calls;
    if (strcmp(name, "/dd/SYS/motd") != 0 || mode != 1 || backend_path == 0)
        return Q9IOMAN_E_INVALID_ARGUMENT;
    mock->opened_path = 0x42;
    *backend_path = mock->opened_path;
    return Q9IOMAN_OK;
}

static Q9IOMAN_Status mock_operate(void *opaque,
                                  Q9IOMAN_u16 backend_path,
                                  Q9IOMAN_Operation operation,
                                  Q9IOMAN_u32 arg0,
                                  Q9IOMAN_u32 arg1,
                                  Q9IOMAN_u32 arg2,
                                  Q9IOMAN_Result *result)
{
    MockBackend *mock = (MockBackend *)opaque;
    ++mock->operate_calls;
    mock->last_path = backend_path;
    mock->last_operation = operation;
    mock->last_arg0 = arg0;
    mock->last_arg1 = arg1;
    mock->last_arg2 = arg2;
    if (result == 0)
        return Q9IOMAN_E_INVALID_ARGUMENT;
    result->value = 0;
    result->transferred = arg2;
    return Q9IOMAN_OK;
}

static Q9IOMAN_Status mock_close(void *opaque, Q9IOMAN_u16 backend_path)
{
    MockBackend *mock = (MockBackend *)opaque;
    ++mock->close_calls;
    mock->last_path = backend_path;
    return mock->close_status;
}

int main(void)
{
    static const Q9IOMAN_BackendOps backend = {
        mock_open,
        mock_operate,
        mock_close
    };
    Q9IOMAN_Manager manager;
    Q9IOMAN_Path paths[2];
    Q9IOMAN_Result result;
    Q9IOMAN_u16 local_path = 0;
    Q9IOMAN_u16 opened_path = 0;
    MockBackend mock;

    memset(&mock, 0, sizeof(mock));
    mock.close_status = Q9IOMAN_OK;
    check("system manager is unavailable before startup",
          q9ioman_system_manager() == 0);
    check("system startup initializes resident path storage",
          q9ioman_system_init() == Q9IOMAN_OK &&
          q9ioman_system_manager() != 0 &&
          q9ioman_system_manager()->capacity == Q9IOMAN_SYSTEM_PATH_CAPACITY);
    check("repeated startup preserves the resident manager",
          q9ioman_system_init() == Q9IOMAN_OK &&
          q9ioman_system_manager() != 0 &&
          q9ioman_system_manager()->capacity == Q9IOMAN_SYSTEM_PATH_CAPACITY);
    check("initializes caller-owned path table",
          q9ioman_init(&manager, paths, 2) == Q9IOMAN_OK &&
          manager.paths == paths && manager.capacity == 2 &&
          paths[0].state == 0 && paths[0].backend_path == 0 &&
          paths[0].backend == 0 && paths[0].backend_context == 0);
    check("registers filesystem prefix and narrower mount",
          q9ioman_register_backend(&manager, "/dd", &backend, &mock) == Q9IOMAN_OK &&
          q9ioman_register_backend(&manager, "/dd/SYS", &backend, &mock) == Q9IOMAN_OK);
    check("rejects duplicate backend prefix",
          q9ioman_register_backend(&manager, "/dd", &backend, &mock) ==
              Q9IOMAN_E_INVALID_ARGUMENT);
    check("resolves longest matching prefix at path boundary",
          q9ioman_open_resolved(&manager, "/dd/SYS/motd", 1,
                                &local_path) == Q9IOMAN_OK && local_path == 1);
    opened_path = local_path;
    check("does not match a mere prefix of a device name",
          q9ioman_open_resolved(&manager, "/ddx/SYS/motd", 1,
                                &local_path) == Q9IOMAN_E_NOT_FOUND &&
          local_path == 0 && mock.open_calls == 1);
    check("explicit backend open remains available",
          q9ioman_open(&manager, &backend, &mock, "/dd/SYS/motd", 1,
                       &local_path) == Q9IOMAN_OK && local_path == 2);
    check("rejects open when local path table is full",
          q9ioman_open(&manager, &backend, &mock, "/dd/SYS/motd", 1,
                       &local_path) == Q9IOMAN_E_NO_PATH_SLOTS &&
          mock.open_calls == 2);
    check("routes operation and arguments using backend path",
          q9ioman_operate(&manager, opened_path, Q9IOMAN_OP_READ,
                          0x1000, 0, 32, &result) == Q9IOMAN_OK &&
          mock.last_path == 0x42 && mock.last_operation == Q9IOMAN_OP_READ &&
          mock.last_arg0 == 0x1000 && mock.last_arg1 == 0 &&
          mock.last_arg2 == 32 && result.transferred == 32);
    check("rejects an unallocated local path",
          q9ioman_operate(&manager, 3, Q9IOMAN_OP_READ,
                          0, 0, 0, &result) == Q9IOMAN_E_INVALID_PATH);
    check("retains a path when backend close fails",
          (mock.close_status = Q9IOMAN_E_UNSUPPORTED_OPERATION,
          q9ioman_close(&manager, opened_path)) ==
              Q9IOMAN_E_UNSUPPORTED_OPERATION &&
          q9ioman_operate(&manager, opened_path, Q9IOMAN_OP_READ,
                          0, 0, 0, &result) == Q9IOMAN_OK);
    check("refuses to unregister a backend with an active path",
          q9ioman_unregister_backend(&manager, "/dd/SYS") == Q9IOMAN_E_BUSY);
    check("releases path after successful close",
          (mock.close_status = Q9IOMAN_OK,
          q9ioman_close(&manager, opened_path)) == Q9IOMAN_OK &&
          q9ioman_operate(&manager, opened_path, Q9IOMAN_OP_READ,
                          0, 0, 0, &result) == Q9IOMAN_E_INVALID_PATH);
    check("unregisters backend after its paths are closed",
          q9ioman_unregister_backend(&manager, "/dd/SYS") == Q9IOMAN_OK &&
          q9ioman_unregister_backend(&manager, "/dd") == Q9IOMAN_OK &&
          q9ioman_unregister_backend(&manager, "/dd") == Q9IOMAN_E_NOT_FOUND);
    check("open resolution fails after final backend detach",
          q9ioman_open_resolved(&manager, "/dd/SYS/motd", 1,
                                &local_path) == Q9IOMAN_E_NOT_FOUND &&
          local_path == 0);
    check("backend was called only along valid routes",
          mock.open_calls == 2 && mock.operate_calls == 2 &&
          mock.close_calls == 2);

    {
        unsigned char frame[Q9IOMAN_R_SIZE];
        memset(frame, 0, sizeof(frame));
        q9ioman_frame_write32(frame, Q9IOMAN_R_D0, 0x12345678UL);
        q9ioman_frame_write32(frame, Q9IOMAN_R_A0, 0x89abcdefUL);
        q9ioman_frame_write16(frame, Q9IOMAN_R_SR, 0x2700U);
        q9ioman_frame_write16(frame, Q9IOMAN_R_PC, 0x4321U);
        check("register-frame accessors use big-endian D0/A0 offsets",
              q9ioman_frame_read32(frame, Q9IOMAN_R_D0) == 0x12345678UL &&
              q9ioman_frame_read32(frame, Q9IOMAN_R_A0) == 0x89abcdefUL);
        check("register-frame accessors use SR/PC word offsets",
              q9ioman_frame_read16(frame, Q9IOMAN_R_SR) == 0x2700U &&
              q9ioman_frame_read16(frame, Q9IOMAN_R_PC) == 0x4321U);
    }
    check("maps manager errors to the Q9 kernel convention",
          q9ioman_status_to_os9_error(Q9IOMAN_OK) == 0 &&
          q9ioman_status_to_os9_error(Q9IOMAN_E_INVALID_ARGUMENT) ==
              Q9IOMAN_OS9_E_PARAM &&
          q9ioman_status_to_os9_error(Q9IOMAN_E_INVALID_PATH) ==
              Q9IOMAN_OS9_E_BPNUM &&
          q9ioman_status_to_os9_error(Q9IOMAN_E_NO_PATH_SLOTS) ==
              Q9IOMAN_OS9_E_PTHFUL &&
          q9ioman_status_to_os9_error(Q9IOMAN_E_NOT_FOUND) ==
              Q9IOMAN_OS9_E_MNF &&
          q9ioman_status_to_os9_error(Q9IOMAN_E_UNSUPPORTED_OPERATION) ==
              Q9IOMAN_OS9_E_UNKSVC);

    if (failures != 0)
        return 1;
    puts("All Q9-IOMAN host tests passed.");
    return 0;
}
