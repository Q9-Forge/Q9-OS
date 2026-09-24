#include <stdio.h>
#include <string.h>

#include "qioman.h"
#include "qioman_kernel.h"
#include "qioman_system.h"

typedef struct {
    const char *expected_name;
    Q9IOMAN_u16 opened_path;
    Q9IOMAN_u16 last_path;
    Q9IOMAN_Operation last_operation;
    Q9IOMAN_u32 last_arg0;
    Q9IOMAN_u32 last_arg1;
    Q9IOMAN_u32 last_arg2;
    Q9IOMAN_u16 open_calls;
    Q9IOMAN_u16 operate_calls;
    Q9IOMAN_u16 close_calls;
    Q9IOMAN_Manager *nested_manager;
    Q9IOMAN_u16 nested_local_path;
    Q9IOMAN_Status nested_open_status;
    Q9IOMAN_Status nested_unregister_status;
    Q9IOMAN_Manager *reentrant_manager;
    Q9IOMAN_u16 reentrant_path;
    Q9IOMAN_Status reentrant_close_status;
    Q9IOMAN_Status open_status;
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
    if (mock->open_status != Q9IOMAN_OK)
        return mock->open_status;
    if (mock->expected_name == 0 || strcmp(name, mock->expected_name) != 0 ||
        (mode != 0 && mode != 1 && mode != 2) || backend_path == 0)
        return Q9IOMAN_E_INVALID_ARGUMENT;
    if (mock->nested_manager != 0) {
        Q9IOMAN_Manager *nested_manager = mock->nested_manager;
        mock->nested_manager = 0;
        mock->nested_unregister_status =
            q9ioman_unregister_backend(nested_manager, "/nest");
        mock->nested_open_status =
            q9ioman_open_resolved(nested_manager, "/nest/file", 1,
                                  &mock->nested_local_path);
    }
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
    if (mock->reentrant_manager != 0) {
        Q9IOMAN_Manager *manager = mock->reentrant_manager;
        Q9IOMAN_u16 path = mock->reentrant_path;
        mock->reentrant_manager = 0;
        mock->reentrant_close_status = q9ioman_close(manager, path);
    }
    result->value = 0;
    result->transferred = arg2;
    return Q9IOMAN_OK;
}

static Q9IOMAN_Status mock_close(void *opaque, Q9IOMAN_u16 backend_path)
{
    MockBackend *mock = (MockBackend *)opaque;
    ++mock->close_calls;
    mock->last_path = backend_path;
    if (mock->reentrant_manager != 0) {
        Q9IOMAN_Manager *manager = mock->reentrant_manager;
        Q9IOMAN_u16 path = mock->reentrant_path;
        mock->reentrant_manager = 0;
        mock->reentrant_close_status = q9ioman_close(manager, path);
    }
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
    Q9IOMAN_Manager nested_manager;
    Q9IOMAN_Path paths[2];
    Q9IOMAN_Path nested_paths[2];
    Q9IOMAN_Result result;
    Q9IOMAN_u16 local_path = 0;
    Q9IOMAN_u16 opened_path = 0;
    Q9IOMAN_u16 explicit_path = 0;
    MockBackend mock;

    memset(&mock, 0, sizeof(mock));
    mock.expected_name = "/dd/SYS/motd";
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
    check("rolls back path reservation after backend open failure",
          (mock.open_status = Q9IOMAN_E_UNSUPPORTED_OPERATION,
           q9ioman_open_resolved(&manager, "/dd/SYS/motd", 1,
                                 &local_path)) ==
              Q9IOMAN_E_UNSUPPORTED_OPERATION && local_path == 0 &&
          (mock.open_status = Q9IOMAN_OK, 1));
    check("resolves longest matching prefix at path boundary",
          q9ioman_open_resolved(&manager, "/dd/SYS/motd", 1,
                                &local_path) == Q9IOMAN_OK && local_path == 1);
    opened_path = local_path;
    check("does not match a mere prefix of a device name",
          q9ioman_open_resolved(&manager, "/ddx/SYS/motd", 1,
                                &local_path) == Q9IOMAN_E_NOT_FOUND &&
          local_path == 0 && mock.open_calls == 2);
    check("explicit backend open remains available",
          q9ioman_open(&manager, &backend, &mock, "/dd/SYS/motd", 0,
                       &local_path) == Q9IOMAN_OK && local_path == 2);
    explicit_path = local_path;
    check("Q9 zero open mode defaults to read and write access",
          q9ioman_operate(&manager, explicit_path, Q9IOMAN_OP_WRITE,
                          0x2000, 0, 4, &result) == Q9IOMAN_OK &&
          result.transferred == 4);
    check("rejects open when local path table is full",
          q9ioman_open(&manager, &backend, &mock, "/dd/SYS/motd", 1,
                       &local_path) == Q9IOMAN_E_NO_PATH_SLOTS &&
          mock.open_calls == 3);
    check("routes operation and arguments using backend path",
          q9ioman_operate(&manager, opened_path, Q9IOMAN_OP_READ,
                          0x1000, 0, 32, &result) == Q9IOMAN_OK &&
          mock.last_path == 0x42 && mock.last_operation == Q9IOMAN_OP_READ &&
          mock.last_arg0 == 0x1000 && mock.last_arg1 == 0 &&
          mock.last_arg2 == 32 && result.transferred == 32);
    check("rejects write on a read-only path before backend dispatch",
          q9ioman_operate(&manager, opened_path, Q9IOMAN_OP_WRITE,
                          0x1000, 0, 32, &result) == Q9IOMAN_E_WRONG_MODE &&
          mock.operate_calls == 2);
    check("rejects an unallocated local path",
          (result.value = 99, result.transferred = 99,
           q9ioman_operate(&manager, 3, Q9IOMAN_OP_READ,
                          0, 0, 0, &result) == Q9IOMAN_E_INVALID_PATH &&
           result.value == 0 && result.transferred == 0));
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
    check("releases direct-open path",
          q9ioman_close(&manager, explicit_path) == Q9IOMAN_OK);
    check("opens write-only path and rejects read",
          q9ioman_open_resolved(&manager, "/dd/SYS/motd", 2,
                                &local_path) == Q9IOMAN_OK &&
          q9ioman_operate(&manager, local_path, Q9IOMAN_OP_READ,
                          0, 0, 1, &result) == Q9IOMAN_E_WRONG_MODE);
    check("permits write on write-only path",
          q9ioman_operate(&manager, local_path, Q9IOMAN_OP_WRITE,
                          0x2000, 0, 8, &result) == Q9IOMAN_OK &&
          result.transferred == 8);
    check("closes write-only path before unregister",
          q9ioman_close(&manager, local_path) == Q9IOMAN_OK);
    check("unregisters backend after its paths are closed",
          q9ioman_unregister_backend(&manager, "/dd/SYS") == Q9IOMAN_OK &&
          q9ioman_unregister_backend(&manager, "/dd") == Q9IOMAN_OK &&
          q9ioman_unregister_backend(&manager, "/dd") == Q9IOMAN_E_NOT_FOUND);
    check("open resolution fails after final backend detach",
          q9ioman_open_resolved(&manager, "/dd/SYS/motd", 1,
                                &local_path) == Q9IOMAN_E_NOT_FOUND &&
          local_path == 0);
    check("backend was called only along valid routes",
          mock.open_calls == 4 && mock.operate_calls == 4 &&
          mock.close_calls == 4);

    {
        MockBackend nested_mock;
        Q9IOMAN_u16 outer_path = 0;
        memset(&nested_mock, 0, sizeof(nested_mock));
        nested_mock.expected_name = "/nest/file";
        nested_mock.close_status = Q9IOMAN_OK;
        check("initializes separate state for a reentrant backend test",
              q9ioman_init(&nested_manager, nested_paths, 2) == Q9IOMAN_OK &&
              q9ioman_register_backend(&nested_manager, "/nest", &backend,
                                       &nested_mock) == Q9IOMAN_OK);
        nested_mock.nested_manager = &nested_manager;
        check("reserves an opening slot before reentrant backend callback",
              q9ioman_open_resolved(&nested_manager, "/nest/file", 1,
                                    &outer_path) == Q9IOMAN_OK &&
              nested_mock.nested_open_status == Q9IOMAN_OK &&
              nested_mock.nested_unregister_status == Q9IOMAN_E_BUSY &&
              outer_path != nested_mock.nested_local_path);
        nested_mock.reentrant_manager = &nested_manager;
        nested_mock.reentrant_path = outer_path;
        check("does not close a path while its backend operation is active",
              q9ioman_operate(&nested_manager, outer_path, Q9IOMAN_OP_READ,
                              0, 0, 1, &result) == Q9IOMAN_OK &&
              nested_mock.reentrant_close_status == Q9IOMAN_E_BUSY);
        nested_mock.reentrant_manager = &nested_manager;
        nested_mock.reentrant_path = outer_path;
        check("guards against recursively closing a path already closing",
              q9ioman_close(&nested_manager, outer_path) == Q9IOMAN_OK &&
              nested_mock.reentrant_close_status == Q9IOMAN_E_INVALID_PATH &&
              q9ioman_close(&nested_manager, nested_mock.nested_local_path) ==
                  Q9IOMAN_OK &&
              q9ioman_unregister_backend(&nested_manager, "/nest") ==
                  Q9IOMAN_OK);
    }

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
          q9ioman_status_to_os9_error(Q9IOMAN_E_WRONG_MODE) ==
              Q9IOMAN_OS9_E_BMODE &&
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
