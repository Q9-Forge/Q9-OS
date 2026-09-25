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
    Q9IOMAN_u16 name_calls;
    Q9IOMAN_Operation last_name_operation;
    Q9IOMAN_u32 transferred_override;
    int override_transferred;
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

static Q9IOMAN_Status resolve_test_path(void *context,
                                        Q9IOMAN_u32 address,
                                        const char **path,
                                        Q9IOMAN_u32 *bytes_including_nul)
{
    const char *text = (const char *)context;
    if (address != 0x2000UL || text == 0 || path == 0 ||
        bytes_including_nul == 0)
        return Q9IOMAN_E_INVALID_ARGUMENT;
    *path = text;
    *bytes_including_nul = (Q9IOMAN_u32)strlen(text) + 1;
    return Q9IOMAN_OK;
}

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
    result->transferred = mock->override_transferred
        ? mock->transferred_override : arg2;
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

static Q9IOMAN_Status mock_create(void *opaque,
                                const char *name,
                                Q9IOMAN_u16 mode,
                                Q9IOMAN_u16 *backend_path)
{
    return mock_open(opaque, name, mode, backend_path);
}

static Q9IOMAN_Status mock_name_operation(void *opaque,
                                         Q9IOMAN_Operation operation,
                                         const char *name,
                                         Q9IOMAN_Result *result)
{
    MockBackend *mock = (MockBackend *)opaque;
    ++mock->name_calls;
    mock->last_name_operation = operation;
    if (name == 0 || result == 0 ||
        (operation != Q9IOMAN_OP_MAKE_DIR &&
         operation != Q9IOMAN_OP_DELETE))
        return Q9IOMAN_E_INVALID_ARGUMENT;
    result->value = 0;
    result->transferred = 0;
    return Q9IOMAN_OK;
}

int main(void)
{
    static const Q9IOMAN_BackendOps backend = {
        mock_open,
        mock_operate,
        mock_close,
        mock_create,
        mock_name_operation
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
    check("Q9IOMAN_u32 is exactly one 32-bit register value",
          sizeof(Q9IOMAN_u32) == 4);
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
        Q9IOMAN_Manager dispatch_manager;
        Q9IOMAN_Path dispatch_paths[2];
        MockBackend dispatch_mock;
        const char *dispatch_name = "/dd/file";
        unsigned char frame[Q9IOMAN_R_SIZE];
        Q9IOMAN_u16 dispatch_path;
        Q9IOMAN_u16 create_path;

        memset(&dispatch_mock, 0, sizeof(dispatch_mock));
        dispatch_mock.expected_name = dispatch_name;
        dispatch_mock.close_status = Q9IOMAN_OK;
        q9ioman_init(&dispatch_manager, dispatch_paths, 2);
        memset(frame, 0, sizeof(frame));
        q9ioman_frame_write16(frame, Q9IOMAN_R_SR, 0x2700U);
        check("missing manager reports a complete syscall error frame",
              q9ioman_dispatch_kernel_request(0x0084, 0, frame,
                  resolve_test_path, (void *)dispatch_name) ==
                  Q9IOMAN_E_INVALID_ARGUMENT &&
              q9ioman_frame_read16(frame, Q9IOMAN_R_D1 + 2) ==
                  Q9IOMAN_OS9_E_PARAM &&
              (q9ioman_frame_read16(frame, Q9IOMAN_R_SR) & 1U) != 0);

        memset(frame, 0, sizeof(frame));
        q9ioman_frame_write32(frame, Q9IOMAN_R_D0, 1);
        q9ioman_frame_write32(frame, Q9IOMAN_R_A0, 0x2000UL);
        q9ioman_frame_write16(frame, Q9IOMAN_R_SR, 0x2700U);
        check("I$Open without a backend returns E$MNF and sets Carry",
              q9ioman_dispatch_kernel_request(0x0084, &dispatch_manager,
                  frame, resolve_test_path, (void *)dispatch_name) ==
                  Q9IOMAN_E_NOT_FOUND &&
              q9ioman_frame_read16(frame, Q9IOMAN_R_D1 + 2) ==
                  Q9IOMAN_OS9_E_MNF &&
              (q9ioman_frame_read16(frame, Q9IOMAN_R_SR) & 1U) != 0);

        q9ioman_register_backend(&dispatch_manager, "/dd", &backend,
                                 &dispatch_mock);
        memset(frame, 0, sizeof(frame));
        q9ioman_frame_write32(frame, Q9IOMAN_R_D0, 1);
        q9ioman_frame_write32(frame, Q9IOMAN_R_A0, 0x2000UL);
        q9ioman_frame_write16(frame, Q9IOMAN_R_SR, 0x2700U);
        check("dispatches I$Open and returns local path and advanced name pointer",
              q9ioman_dispatch_kernel_request(0x0084, &dispatch_manager,
                  frame, resolve_test_path, (void *)dispatch_name) ==
                  Q9IOMAN_OK &&
              q9ioman_frame_read16(frame, Q9IOMAN_R_D0 + 2) == 1 &&
              q9ioman_frame_read32(frame, Q9IOMAN_R_A0) == 0x2009UL &&
              (q9ioman_frame_read16(frame, Q9IOMAN_R_SR) & 1U) == 0);
        dispatch_path = q9ioman_frame_read16(frame, Q9IOMAN_R_D0 + 2);

        q9ioman_frame_write32(frame, Q9IOMAN_R_D0, 2);
        q9ioman_frame_write32(frame, Q9IOMAN_R_A0, 0x2000UL);
        check("dispatches I$Create through backend create and reserves a path",
              q9ioman_dispatch_kernel_request(0x0083, &dispatch_manager,
                  frame, resolve_test_path, (void *)dispatch_name) ==
                  Q9IOMAN_OK &&
              q9ioman_frame_read16(frame, Q9IOMAN_R_D0 + 2) == 2 &&
              q9ioman_frame_read32(frame, Q9IOMAN_R_A0) == 0x2009UL);
        create_path = q9ioman_frame_read16(frame, Q9IOMAN_R_D0 + 2);

        q9ioman_frame_write32(frame, Q9IOMAN_R_D0, dispatch_path);
        q9ioman_frame_write32(frame, Q9IOMAN_R_D1, 10);
        q9ioman_frame_write32(frame, Q9IOMAN_R_A0, 0x3000UL);
        check("dispatches I$Read and returns transferred length",
              q9ioman_dispatch_kernel_request(0x0089, &dispatch_manager,
                  frame, 0, 0) == Q9IOMAN_OK &&
              q9ioman_frame_read32(frame, Q9IOMAN_R_D1) == 10 &&
              dispatch_mock.last_operation == Q9IOMAN_OP_READ &&
              dispatch_mock.last_arg0 == 0x3000UL &&
              dispatch_mock.last_arg2 == 10 &&
              (q9ioman_frame_read16(frame, Q9IOMAN_R_SR) & 1U) == 0);

        dispatch_paths[0].access_mode = Q9IOMAN_ACCESS_MASK;
        q9ioman_frame_write32(frame, Q9IOMAN_R_D0, dispatch_path);
        q9ioman_frame_write32(frame, Q9IOMAN_R_D1, 6);
        q9ioman_frame_write32(frame, Q9IOMAN_R_A0, 0x3100UL);
        check("dispatches I$Write with buffer and length",
              q9ioman_dispatch_kernel_request(0x008a, &dispatch_manager,
                  frame, 0, 0) == Q9IOMAN_OK &&
              dispatch_mock.last_operation == Q9IOMAN_OP_WRITE &&
              dispatch_mock.last_arg0 == 0x3100UL &&
              dispatch_mock.last_arg2 == 6 &&
              q9ioman_frame_read32(frame, Q9IOMAN_R_D1) == 6);

        q9ioman_frame_write32(frame, Q9IOMAN_R_A0, 0x2000UL);
        check("dispatches I$MakDir through the backend name operation",
              q9ioman_dispatch_kernel_request(0x0085, &dispatch_manager,
                  frame, resolve_test_path, (void *)dispatch_name) ==
                  Q9IOMAN_OK &&
              dispatch_mock.last_name_operation == Q9IOMAN_OP_MAKE_DIR);
        check("dispatches I$Delete through the backend name operation",
              q9ioman_dispatch_kernel_request(0x0087, &dispatch_manager,
                  frame, resolve_test_path, (void *)dispatch_name) ==
                  Q9IOMAN_OK &&
              dispatch_mock.last_name_operation == Q9IOMAN_OP_DELETE);
        check("keeps I$ChgDir outside filesystem-backend dispatch",
              q9ioman_dispatch_kernel_request(0x0086, &dispatch_manager,
                  frame, resolve_test_path, (void *)dispatch_name) ==
                  Q9IOMAN_E_UNSUPPORTED_OPERATION &&
              q9ioman_frame_read16(frame, Q9IOMAN_R_D1 + 2) ==
                  Q9IOMAN_OS9_E_UNKSVC &&
              (q9ioman_frame_read16(frame, Q9IOMAN_R_SR) & 1U) != 0);

        q9ioman_frame_write32(frame, Q9IOMAN_R_D0, dispatch_path);
        q9ioman_frame_write32(frame, Q9IOMAN_R_D1, 0x12345678UL);
        check("dispatches I$Seek with the absolute position",
              q9ioman_dispatch_kernel_request(0x0088, &dispatch_manager,
                  frame, 0, 0) == Q9IOMAN_OK &&
              dispatch_mock.last_operation == Q9IOMAN_OP_SEEK &&
              dispatch_mock.last_arg0 == 0x12345678UL);

        q9ioman_frame_write32(frame, Q9IOMAN_R_D0, dispatch_path);
        q9ioman_frame_write16(frame, Q9IOMAN_R_D1 + 2, 0x12);
        q9ioman_frame_write32(frame, Q9IOMAN_R_A0, 0x3200UL);
        check("dispatches I$GetStt with status and output pointer",
              q9ioman_dispatch_kernel_request(0x008d, &dispatch_manager,
                  frame, 0, 0) == Q9IOMAN_OK &&
              dispatch_mock.last_operation == Q9IOMAN_OP_GET_STATUS &&
              dispatch_mock.last_arg0 == 0x12 &&
              dispatch_mock.last_arg1 == 0x3200UL);

        q9ioman_frame_write32(frame, Q9IOMAN_R_D0, dispatch_path);
        q9ioman_frame_write16(frame, Q9IOMAN_R_D1 + 2, 0x13);
        q9ioman_frame_write32(frame, Q9IOMAN_R_A0, 0x3300UL);
        check("dispatches I$SetStt with status and input pointer",
              q9ioman_dispatch_kernel_request(0x008e, &dispatch_manager,
                  frame, 0, 0) == Q9IOMAN_OK &&
              dispatch_mock.last_operation == Q9IOMAN_OP_SET_STATUS &&
              dispatch_mock.last_arg0 == 0x13 &&
              dispatch_mock.last_arg1 == 0x3300UL);

        q9ioman_frame_write32(frame, Q9IOMAN_R_D0, dispatch_path);
        q9ioman_frame_write32(frame, Q9IOMAN_R_D1, 7);
        q9ioman_frame_write32(frame, Q9IOMAN_R_A0, 0x3400UL);
        check("dispatches I$ReadLn and returns its byte count",
              q9ioman_dispatch_kernel_request(0x008b, &dispatch_manager,
                  frame, 0, 0) == Q9IOMAN_OK &&
              dispatch_mock.last_operation == Q9IOMAN_OP_READ_LINE &&
              dispatch_mock.last_arg0 == 0x3400UL &&
              dispatch_mock.last_arg2 == 7 &&
              q9ioman_frame_read32(frame, Q9IOMAN_R_D1) == 7);

        q9ioman_frame_write32(frame, Q9IOMAN_R_D0, dispatch_path);
        q9ioman_frame_write32(frame, Q9IOMAN_R_D1, 8);
        q9ioman_frame_write32(frame, Q9IOMAN_R_A0, 0x3500UL);
        check("dispatches I$WritLn and returns its byte count",
              q9ioman_dispatch_kernel_request(0x008c, &dispatch_manager,
                  frame, 0, 0) == Q9IOMAN_OK &&
              dispatch_mock.last_operation == Q9IOMAN_OP_WRITE_LINE &&
              dispatch_mock.last_arg0 == 0x3500UL &&
              dispatch_mock.last_arg2 == 8 &&
              q9ioman_frame_read32(frame, Q9IOMAN_R_D1) == 8);

        dispatch_mock.override_transferred = 1;
        dispatch_mock.transferred_override = 4;
        q9ioman_frame_write32(frame, Q9IOMAN_R_D1, 10);
        q9ioman_frame_write16(frame, Q9IOMAN_R_SR, 0x2700U);
        check("I$Read reports a backend short read without treating it as failure",
              q9ioman_dispatch_kernel_request(0x0089, &dispatch_manager,
                  frame, 0, 0) == Q9IOMAN_OK &&
              q9ioman_frame_read32(frame, Q9IOMAN_R_D1) == 4 &&
              (q9ioman_frame_read16(frame, Q9IOMAN_R_SR) & 1U) == 0);

        dispatch_mock.transferred_override = 11;
        q9ioman_frame_write32(frame, Q9IOMAN_R_D1, 10);
        q9ioman_frame_write16(frame, Q9IOMAN_R_SR, 0x2700U);
        check("I$Read rejects a backend count larger than the request",
              q9ioman_dispatch_kernel_request(0x0089, &dispatch_manager,
                  frame, 0, 0) == Q9IOMAN_E_INVALID_ARGUMENT &&
              q9ioman_frame_read16(frame, Q9IOMAN_R_D1 + 2) ==
                  Q9IOMAN_OS9_E_PARAM &&
              (q9ioman_frame_read16(frame, Q9IOMAN_R_SR) & 1U) != 0 &&
              dispatch_mock.operate_calls == 9);
        dispatch_mock.override_transferred = 0;

        q9ioman_frame_write32(frame, Q9IOMAN_R_D0, dispatch_path);
        q9ioman_frame_write32(frame, Q9IOMAN_R_D1, 1);
        q9ioman_frame_write32(frame, Q9IOMAN_R_A0, 0);
        q9ioman_frame_write16(frame, Q9IOMAN_R_SR, 0x2700U);
        check("I$Read rejects a null buffer for a nonempty request",
              q9ioman_dispatch_kernel_request(0x0089, &dispatch_manager,
                  frame, 0, 0) == Q9IOMAN_E_INVALID_ARGUMENT &&
              q9ioman_frame_read16(frame, Q9IOMAN_R_D1 + 2) ==
                  Q9IOMAN_OS9_E_PARAM &&
              (q9ioman_frame_read16(frame, Q9IOMAN_R_SR) & 1U) != 0 &&
              dispatch_mock.operate_calls == 9);

        q9ioman_frame_write32(frame, Q9IOMAN_R_A0, 0xffffffffUL);
        q9ioman_frame_write32(frame, Q9IOMAN_R_D1, 2);
        q9ioman_frame_write16(frame, Q9IOMAN_R_SR, 0x2700U);
        check("I$Read rejects a buffer range that wraps the 32-bit address space",
              q9ioman_dispatch_kernel_request(0x0089, &dispatch_manager,
                  frame, 0, 0) == Q9IOMAN_E_INVALID_ARGUMENT &&
              q9ioman_frame_read16(frame, Q9IOMAN_R_D1 + 2) ==
                  Q9IOMAN_OS9_E_PARAM &&
              (q9ioman_frame_read16(frame, Q9IOMAN_R_SR) & 1U) != 0 &&
              dispatch_mock.operate_calls == 9);

        q9ioman_frame_write32(frame, Q9IOMAN_R_D0, dispatch_path);
        check("dispatches I$Close and reports invalid path using Carry/D1.w",
              q9ioman_dispatch_kernel_request(0x008f, &dispatch_manager,
                  frame, 0, 0) == Q9IOMAN_OK &&
              q9ioman_dispatch_kernel_request(0x008f, &dispatch_manager,
                  frame, 0, 0) == Q9IOMAN_E_INVALID_PATH &&
              (q9ioman_frame_read16(frame, Q9IOMAN_R_SR) & 1U) != 0 &&
              q9ioman_frame_read16(frame, Q9IOMAN_R_D1 + 2) ==
                  Q9IOMAN_OS9_E_BPNUM);
        check("closes the path returned by I$Create",
              q9ioman_close(&dispatch_manager, create_path) == Q9IOMAN_OK);
    }

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
        Q9IOMAN_KernelRequest request;
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

        q9ioman_frame_write32(frame, Q9IOMAN_R_D0, 0x00000102UL);
        check("decodes I$Open mode and preserves caller path pointer",
              q9ioman_decode_kernel_request(0x0084, frame, &request) ==
                  Q9IOMAN_OK &&
              request.type == Q9IOMAN_KERNEL_OPEN && request.mode == 2 &&
              request.buffer == 0x89abcdefUL && request.path == 0 &&
              request.length == 0);

        q9ioman_frame_write32(frame, Q9IOMAN_R_D0, 7);
        q9ioman_frame_write32(frame, Q9IOMAN_R_D1, 0x1234);
        check("decodes I$Read path, length and buffer address",
              q9ioman_decode_kernel_request(0x0089, frame, &request) ==
                  Q9IOMAN_OK &&
              request.type == Q9IOMAN_KERNEL_READ && request.path == 7 &&
              request.length == 0x1234UL &&
              request.buffer == 0x89abcdefUL);

        check("decodes I$Close path without inventing buffer parameters",
              q9ioman_decode_kernel_request(0x008f, frame, &request) ==
                  Q9IOMAN_OK &&
              request.type == Q9IOMAN_KERNEL_CLOSE && request.path == 7 &&
              request.mode == 0 && request.buffer == 0 && request.length == 0);

        q9ioman_frame_write32(frame, Q9IOMAN_R_D0, 2);
        q9ioman_frame_write32(frame, Q9IOMAN_R_D1, 0x12345678UL);
        q9ioman_frame_write32(frame, Q9IOMAN_R_A0, 0x76543210UL);
        check("decodes I$Create mode and path pointer",
              q9ioman_decode_kernel_request(0x0083, frame, &request) ==
                  Q9IOMAN_OK && request.type == Q9IOMAN_KERNEL_CREATE &&
              request.mode == 2 && request.buffer == 0x76543210UL);
        check("decodes I$MakDir and I$Delete path pointers",
              q9ioman_decode_kernel_request(0x0085, frame, &request) ==
                  Q9IOMAN_OK && request.type == Q9IOMAN_KERNEL_MAKDIR &&
              request.buffer == 0x76543210UL &&
              q9ioman_decode_kernel_request(0x0087, frame, &request) ==
                  Q9IOMAN_OK && request.type == Q9IOMAN_KERNEL_DELETE &&
              request.buffer == 0x76543210UL);
        q9ioman_frame_write16(frame, Q9IOMAN_R_D0 + 2, 3);
        check("decodes I$ChgDir selector and path pointer",
              q9ioman_decode_kernel_request(0x0086, frame, &request) ==
                  Q9IOMAN_OK && request.type == Q9IOMAN_KERNEL_CHGDIR &&
              request.selector == 3 && request.buffer == 0x76543210UL);
        q9ioman_frame_write32(frame, Q9IOMAN_R_D0, 9);
        q9ioman_frame_write32(frame, Q9IOMAN_R_D1, 0x23456789UL);
        check("decodes I$Seek path and absolute position",
              q9ioman_decode_kernel_request(0x0088, frame, &request) ==
                  Q9IOMAN_OK && request.type == Q9IOMAN_KERNEL_SEEK &&
              request.path == 9 && request.position == 0x23456789UL);
        check("decodes I$Write, I$ReadLn and I$WritLn data arguments",
              q9ioman_decode_kernel_request(0x008a, frame, &request) ==
                  Q9IOMAN_OK && request.type == Q9IOMAN_KERNEL_WRITE &&
              request.path == 9 && request.length == 0x23456789UL &&
              request.buffer == 0x76543210UL &&
              q9ioman_decode_kernel_request(0x008b, frame, &request) ==
                  Q9IOMAN_OK && request.type == Q9IOMAN_KERNEL_READ_LINE &&
              q9ioman_decode_kernel_request(0x008c, frame, &request) ==
                  Q9IOMAN_OK && request.type == Q9IOMAN_KERNEL_WRITE_LINE);
        q9ioman_frame_write16(frame, Q9IOMAN_R_D1 + 2, 0x55aa);
        check("decodes I$GetStt and I$SetStt status arguments",
              q9ioman_decode_kernel_request(0x008d, frame, &request) ==
                  Q9IOMAN_OK && request.type == Q9IOMAN_KERNEL_GET_STATUS &&
              request.status_code == 0x55aa && request.path == 9 &&
              request.buffer == 0x76543210UL &&
              q9ioman_decode_kernel_request(0x008e, frame, &request) ==
                  Q9IOMAN_OK && request.type == Q9IOMAN_KERNEL_SET_STATUS &&
              request.status_code == 0x55aa);

        check("rejects unsupported callcodes and null decoder inputs",
              q9ioman_decode_kernel_request(0x0090, frame, &request) ==
                  Q9IOMAN_E_UNSUPPORTED_OPERATION &&
              q9ioman_decode_kernel_request(0x0089, 0, &request) ==
                  Q9IOMAN_E_INVALID_ARGUMENT &&
              q9ioman_decode_kernel_request(0x0089, frame, 0) ==
                  Q9IOMAN_E_INVALID_ARGUMENT);
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
