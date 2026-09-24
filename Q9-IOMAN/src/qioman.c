#include "qioman.h"

#define Q9IOMAN_PATH_FREE 0
#define Q9IOMAN_PATH_OPEN 1

static Q9IOMAN_Path *q9ioman_find_path(Q9IOMAN_Manager *manager,
                                       Q9IOMAN_u16 local_path)
{
    Q9IOMAN_u16 index;

    if (manager == 0 || manager->paths == 0 || local_path == 0)
        return 0;
    if (local_path > manager->capacity)
        return 0;

    index = (Q9IOMAN_u16)(local_path - 1);
    if (manager->paths[index].state != Q9IOMAN_PATH_OPEN)
        return 0;
    return &manager->paths[index];
}

Q9IOMAN_Status q9ioman_init(Q9IOMAN_Manager *manager,
                            Q9IOMAN_Path *paths,
                            Q9IOMAN_u16 capacity)
{
    Q9IOMAN_u16 index;

    if (manager == 0 || paths == 0 || capacity == 0)
        return Q9IOMAN_E_INVALID_ARGUMENT;

    manager->paths = paths;
    manager->capacity = capacity;
    for (index = 0; index < capacity; ++index) {
        paths[index].state = Q9IOMAN_PATH_FREE;
        paths[index].backend_path = 0;
        paths[index].backend = 0;
        paths[index].backend_context = 0;
    }
    return Q9IOMAN_OK;
}

Q9IOMAN_Status q9ioman_open(Q9IOMAN_Manager *manager,
                            const Q9IOMAN_BackendOps *backend,
                            void *backend_context,
                            const char *path,
                            Q9IOMAN_u16 mode,
                            Q9IOMAN_u16 *local_path)
{
    Q9IOMAN_u16 index;
    Q9IOMAN_u16 backend_path;
    Q9IOMAN_Status status;

    if (local_path == 0)
        return Q9IOMAN_E_INVALID_ARGUMENT;
    *local_path = 0;
    if (manager == 0 || manager->paths == 0 || backend == 0 ||
        backend->open == 0 || backend->close == 0 || path == 0 ||
        manager->capacity == 0)
        return Q9IOMAN_E_INVALID_ARGUMENT;

    for (index = 0; index < manager->capacity; ++index) {
        if (manager->paths[index].state == Q9IOMAN_PATH_FREE)
            break;
    }
    if (index == manager->capacity)
        return Q9IOMAN_E_NO_PATH_SLOTS;

    status = backend->open(backend_context, path, mode, &backend_path);
    if (status != Q9IOMAN_OK)
        return status;

    manager->paths[index].backend_path = backend_path;
    manager->paths[index].backend = backend;
    manager->paths[index].backend_context = backend_context;
    manager->paths[index].state = Q9IOMAN_PATH_OPEN;
    *local_path = (Q9IOMAN_u16)(index + 1);
    return Q9IOMAN_OK;
}

Q9IOMAN_Status q9ioman_operate(Q9IOMAN_Manager *manager,
                               Q9IOMAN_u16 local_path,
                               Q9IOMAN_Operation operation,
                               Q9IOMAN_u32 arg0,
                               Q9IOMAN_u32 arg1,
                               Q9IOMAN_u32 arg2,
                               Q9IOMAN_Result *result)
{
    Q9IOMAN_Path *path;

    if (result == 0)
        return Q9IOMAN_E_INVALID_ARGUMENT;
    if (operation < Q9IOMAN_OP_READ || operation > Q9IOMAN_OP_SET_STATUS)
        return Q9IOMAN_E_UNSUPPORTED_OPERATION;

    path = q9ioman_find_path(manager, local_path);
    if (path == 0)
        return Q9IOMAN_E_INVALID_PATH;
    if (path->backend == 0 || path->backend->operate == 0)
        return Q9IOMAN_E_UNSUPPORTED_OPERATION;

    return path->backend->operate(path->backend_context,
                                  path->backend_path,
                                  operation,
                                  arg0,
                                  arg1,
                                  arg2,
                                  result);
}

Q9IOMAN_Status q9ioman_close(Q9IOMAN_Manager *manager,
                             Q9IOMAN_u16 local_path)
{
    Q9IOMAN_Path *path = q9ioman_find_path(manager, local_path);
    Q9IOMAN_Status status;

    if (path == 0)
        return Q9IOMAN_E_INVALID_PATH;
    if (path->backend == 0 || path->backend->close == 0)
        return Q9IOMAN_E_UNSUPPORTED_OPERATION;

    status = path->backend->close(path->backend_context, path->backend_path);
    if (status != Q9IOMAN_OK)
        return status;

    path->state = Q9IOMAN_PATH_FREE;
    path->backend_path = 0;
    path->backend = 0;
    path->backend_context = 0;
    return Q9IOMAN_OK;
}
