#include <stdio.h>
#include <string.h>

#include "qioman.h"

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
    Q9IOMAN_Path paths[1];
    Q9IOMAN_Result result;
    Q9IOMAN_u16 local_path = 0;
    Q9IOMAN_u16 opened_path = 0;
    MockBackend mock;

    memset(&mock, 0, sizeof(mock));
    mock.close_status = Q9IOMAN_OK;
    check("initializes caller-owned path table",
          q9ioman_init(&manager, paths, 1) == Q9IOMAN_OK &&
          manager.paths == paths && manager.capacity == 1 &&
          paths[0].state == 0 && paths[0].backend_path == 0 &&
          paths[0].backend == 0 && paths[0].backend_context == 0);
    check("opens through the selected backend",
          q9ioman_open(&manager, &backend, &mock, "/dd/SYS/motd", 1,
                       &local_path) == Q9IOMAN_OK && local_path == 1);
    opened_path = local_path;
    check("rejects open when local path table is full",
          q9ioman_open(&manager, &backend, &mock, "/dd/SYS/motd", 1,
                       &local_path) == Q9IOMAN_E_NO_PATH_SLOTS &&
          mock.open_calls == 1);
    check("routes operation and arguments using backend path",
          q9ioman_operate(&manager, opened_path, Q9IOMAN_OP_READ,
                          0x1000, 0, 32, &result) == Q9IOMAN_OK &&
          mock.last_path == 0x42 && mock.last_operation == Q9IOMAN_OP_READ &&
          mock.last_arg0 == 0x1000 && mock.last_arg1 == 0 &&
          mock.last_arg2 == 32 && result.transferred == 32);
    check("rejects an unallocated local path",
          q9ioman_operate(&manager, 2, Q9IOMAN_OP_READ,
                          0, 0, 0, &result) == Q9IOMAN_E_INVALID_PATH);
    check("retains a path when backend close fails",
          (mock.close_status = Q9IOMAN_E_UNSUPPORTED_OPERATION,
          q9ioman_close(&manager, opened_path)) ==
              Q9IOMAN_E_UNSUPPORTED_OPERATION &&
          q9ioman_operate(&manager, opened_path, Q9IOMAN_OP_READ,
                          0, 0, 0, &result) == Q9IOMAN_OK);
    check("releases path after successful close",
          (mock.close_status = Q9IOMAN_OK,
          q9ioman_close(&manager, opened_path)) == Q9IOMAN_OK &&
          q9ioman_operate(&manager, opened_path, Q9IOMAN_OP_READ,
                          0, 0, 0, &result) == Q9IOMAN_E_INVALID_PATH);
    check("backend was called only along valid routes",
          mock.open_calls == 1 && mock.operate_calls == 2 &&
          mock.close_calls == 2);

    if (failures != 0)
        return 1;
    puts("All Q9-IOMAN host tests passed.");
    return 0;
}
