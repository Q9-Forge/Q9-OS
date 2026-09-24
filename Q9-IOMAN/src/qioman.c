#include "qioman.h"

#define Q9IOMAN_PATH_FREE 0
#define Q9IOMAN_PATH_OPEN 1
#define Q9IOMAN_PATH_OPENING 2
#define Q9IOMAN_PATH_CLOSING 3
#define Q9IOMAN_BACKEND_FREE 0
#define Q9IOMAN_BACKEND_USED 1

static Q9IOMAN_Status q9ioman_open_backend(Q9IOMAN_Manager *manager,
                                           const Q9IOMAN_BackendOps *backend,
                                           void *backend_context,
                                           const char *path,
                                           const char *route_prefix,
                                           Q9IOMAN_u16 mode,
                                           Q9IOMAN_u16 *local_path);

static Q9IOMAN_Path *q9ioman_find_path(Q9IOMAN_Manager *manager,
                                       Q9IOMAN_u16 local_path)
{
    Q9IOMAN_u16 index;
    Q9IOMAN_Path *path;

    if (manager == 0 || manager->paths == 0 || local_path == 0)
        return 0;
    if (local_path > manager->capacity)
        return 0;

    index = (Q9IOMAN_u16)(local_path - 1);
    path = manager->paths;
    while (index != 0) {
        path = path + 1;
        --index;
    }
    if (path->state != Q9IOMAN_PATH_OPEN)
        return 0;
    return path;
}

Q9IOMAN_Status q9ioman_init(Q9IOMAN_Manager *manager,
                            Q9IOMAN_Path *paths,
                            Q9IOMAN_u16 capacity)
{
    Q9IOMAN_u16 index;
    Q9IOMAN_Path *path;

    if (manager == 0 || paths == 0 || capacity == 0)
        return Q9IOMAN_E_INVALID_ARGUMENT;

    manager->paths = paths;
    manager->capacity = capacity;
    path = paths;
    for (index = 0; index < capacity; ++index) {
        path->state = Q9IOMAN_PATH_FREE;
        path->backend_path = 0;
        path->access_mode = 0;
        path->active_operations = 0;
        path->backend = 0;
        path->backend_context = 0;
        path->route_prefix = 0;
        path = path + 1;
    }
    {
        Q9IOMAN_BackendRegistration *entry = manager->backends;
        for (index = 0; index < Q9IOMAN_BACKEND_CAPACITY; ++index) {
            entry->prefix = 0;
            entry->backend = 0;
            entry->context = 0;
            entry->state = Q9IOMAN_BACKEND_FREE;
            entry = entry + 1;
        }
    }
    return Q9IOMAN_OK;
}

static Q9IOMAN_u16 q9ioman_prefix_length(const char *text)
{
    Q9IOMAN_u16 length = 0;
    if (text == 0)
        return 0;
    while (text[length] != '\0' && length != 65535U)
        ++length;
    return length;
}

static int q9ioman_text_equal(const char *left, const char *right)
{
    Q9IOMAN_u16 index = 0;
    if (left == 0 || right == 0)
        return 0;
    while (left[index] != '\0' && right[index] != '\0') {
        if (left[index] != right[index])
            return 0;
        ++index;
    }
    return left[index] == right[index];
}

Q9IOMAN_Status q9ioman_register_backend(Q9IOMAN_Manager *manager,
                                        const char *prefix,
                                        const Q9IOMAN_BackendOps *backend,
                                        void *context)
{
    Q9IOMAN_BackendRegistration *entry;
    Q9IOMAN_BackendRegistration *free_entry = 0;
    Q9IOMAN_u16 index;
    Q9IOMAN_u16 prefix_length = q9ioman_prefix_length(prefix);
    if (manager == 0 || manager->paths == 0 || prefix_length == 0 ||
        backend == 0 || backend->open == 0 || backend->close == 0)
        return Q9IOMAN_E_INVALID_ARGUMENT;
    entry = manager->backends;
    for (index = 0; index < Q9IOMAN_BACKEND_CAPACITY; ++index) {
        if (entry->state == Q9IOMAN_BACKEND_USED) {
            Q9IOMAN_u16 other_length = q9ioman_prefix_length(entry->prefix);
            Q9IOMAN_u16 pos;
            if (other_length == prefix_length) {
                for (pos = 0; pos < prefix_length &&
                     entry->prefix[pos] == prefix[pos]; ++pos) { }
                if (pos == prefix_length)
                    return Q9IOMAN_E_INVALID_ARGUMENT;
            }
        } else if (free_entry == 0) {
            free_entry = entry;
        }
        entry = entry + 1;
    }
    if (free_entry == 0)
        return Q9IOMAN_E_REGISTRY_FULL;
    free_entry->prefix = prefix;
    free_entry->backend = backend;
    free_entry->context = context;
    free_entry->state = Q9IOMAN_BACKEND_USED;
    return Q9IOMAN_OK;
}

Q9IOMAN_Status q9ioman_unregister_backend(Q9IOMAN_Manager *manager,
                                          const char *prefix)
{
    Q9IOMAN_BackendRegistration *entry;
    Q9IOMAN_u16 entry_index;
    Q9IOMAN_u16 prefix_length = q9ioman_prefix_length(prefix);
    Q9IOMAN_Path *path;
    Q9IOMAN_u16 path_index;
    if (manager == 0 || manager->paths == 0 || prefix_length == 0)
        return Q9IOMAN_E_INVALID_ARGUMENT;
    entry = manager->backends;
    for (entry_index = 0; entry_index < Q9IOMAN_BACKEND_CAPACITY;
         ++entry_index) {
        Q9IOMAN_u16 pos;
        if (entry->state != Q9IOMAN_BACKEND_USED) {
            entry = entry + 1;
            continue;
        }
        if (q9ioman_prefix_length(entry->prefix) != prefix_length) {
            entry = entry + 1;
            continue;
        }
        for (pos = 0; pos < prefix_length &&
             entry->prefix[pos] == prefix[pos]; ++pos) { }
        if (pos == prefix_length)
            break;
        entry = entry + 1;
    }
    if (entry_index == Q9IOMAN_BACKEND_CAPACITY)
        return Q9IOMAN_E_NOT_FOUND;

    path = manager->paths;
    for (path_index = 0; path_index < manager->capacity; ++path_index) {
        if (path->state != Q9IOMAN_PATH_FREE &&
            q9ioman_text_equal(path->route_prefix, entry->prefix))
            return Q9IOMAN_E_BUSY;
        path = path + 1;
    }
    entry->prefix = 0;
    entry->backend = 0;
    entry->context = 0;
    entry->state = Q9IOMAN_BACKEND_FREE;
    return Q9IOMAN_OK;
}

Q9IOMAN_Status q9ioman_open_resolved(Q9IOMAN_Manager *manager,
                                     const char *path,
                                     Q9IOMAN_u16 mode,
                                     Q9IOMAN_u16 *local_path)
{
    Q9IOMAN_BackendRegistration *entry;
    Q9IOMAN_BackendRegistration *selected = 0;
    Q9IOMAN_u16 index;
    Q9IOMAN_u16 selected_length = 0;
    Q9IOMAN_u16 path_length;
    if (local_path == 0)
        return Q9IOMAN_E_INVALID_ARGUMENT;
    *local_path = 0;
    if (manager == 0 || path == 0)
        return Q9IOMAN_E_INVALID_ARGUMENT;
    path_length = q9ioman_prefix_length(path);
    entry = manager->backends;
    for (index = 0; index < Q9IOMAN_BACKEND_CAPACITY; ++index) {
        if (entry->state == Q9IOMAN_BACKEND_USED) {
            Q9IOMAN_u16 length = q9ioman_prefix_length(entry->prefix);
            Q9IOMAN_u16 pos;
            for (pos = 0; pos < length && pos < path_length &&
                 entry->prefix[pos] == path[pos]; ++pos) { }
            if (pos == length && length <= path_length &&
                (path[length] == '\0' || path[length] == '/') &&
                length > selected_length) {
                selected = entry;
                selected_length = length;
            }
        }
        entry = entry + 1;
    }
    if (selected == 0)
        return Q9IOMAN_E_NOT_FOUND;
    return q9ioman_open_backend(manager, selected->backend,
                                selected->context, path, selected->prefix,
                                mode, local_path);
}

static Q9IOMAN_Status q9ioman_open_backend(Q9IOMAN_Manager *manager,
                                           const Q9IOMAN_BackendOps *backend,
                                           void *backend_context,
                                           const char *path,
                                           const char *route_prefix,
                                           Q9IOMAN_u16 mode,
                                           Q9IOMAN_u16 *local_path)
{
    Q9IOMAN_u16 index;
    Q9IOMAN_u16 backend_path;
    Q9IOMAN_Status status;
    Q9IOMAN_Path *path_slot;

    if (local_path == 0)
        return Q9IOMAN_E_INVALID_ARGUMENT;
    *local_path = 0;
    if (manager == 0 || manager->paths == 0 || backend == 0 ||
        backend->open == 0 || backend->close == 0 || path == 0 ||
        manager->capacity == 0)
        return Q9IOMAN_E_INVALID_ARGUMENT;

    path_slot = manager->paths;
    for (index = 0; index < manager->capacity; ++index) {
        if (path_slot->state == Q9IOMAN_PATH_FREE)
            break;
        path_slot = path_slot + 1;
    }
    if (index == manager->capacity)
        return Q9IOMAN_E_NO_PATH_SLOTS;

    /* Reserve before calling out: the backend may re-enter IOMan. */
    path_slot->state = Q9IOMAN_PATH_OPENING;
    path_slot->backend = backend;
    path_slot->backend_context = backend_context;
    path_slot->route_prefix = route_prefix;
    path_slot->active_operations = 0;
    path_slot->access_mode = (Q9IOMAN_u16)(mode & Q9IOMAN_ACCESS_MASK);
    if (mode == 0)
        path_slot->access_mode = Q9IOMAN_ACCESS_READ | Q9IOMAN_ACCESS_WRITE;

    backend_path = 0;
    status = backend->open(backend_context, path, mode, &backend_path);
    if (status != Q9IOMAN_OK) {
        path_slot->backend_path = 0;
        path_slot->access_mode = 0;
        path_slot->active_operations = 0;
        path_slot->backend = 0;
        path_slot->backend_context = 0;
        path_slot->route_prefix = 0;
        path_slot->state = Q9IOMAN_PATH_FREE;
        return status;
    }

    path_slot->backend_path = backend_path;
    path_slot->state = Q9IOMAN_PATH_OPEN;
    *local_path = (Q9IOMAN_u16)(index + 1);
    return Q9IOMAN_OK;
}

Q9IOMAN_Status q9ioman_open(Q9IOMAN_Manager *manager,
                            const Q9IOMAN_BackendOps *backend,
                            void *backend_context,
                            const char *path,
                            Q9IOMAN_u16 mode,
                            Q9IOMAN_u16 *local_path)
{
    return q9ioman_open_backend(manager, backend, backend_context, path, 0,
                                mode, local_path);
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
    result->value = 0;
    result->transferred = 0;
    if (operation < Q9IOMAN_OP_READ || operation > Q9IOMAN_OP_SET_STATUS)
        return Q9IOMAN_E_UNSUPPORTED_OPERATION;

    path = q9ioman_find_path(manager, local_path);
    if (path == 0)
        return Q9IOMAN_E_INVALID_PATH;
    if (path->backend == 0 || path->backend->operate == 0)
        return Q9IOMAN_E_UNSUPPORTED_OPERATION;
    if ((operation == Q9IOMAN_OP_READ ||
         operation == Q9IOMAN_OP_READ_LINE) &&
        (path->access_mode & Q9IOMAN_ACCESS_READ) == 0)
        return Q9IOMAN_E_WRONG_MODE;
    if ((operation == Q9IOMAN_OP_WRITE ||
         operation == Q9IOMAN_OP_WRITE_LINE) &&
        (path->access_mode & Q9IOMAN_ACCESS_WRITE) == 0)
        return Q9IOMAN_E_WRONG_MODE;

    if (path->active_operations == 65535U)
        return Q9IOMAN_E_BUSY;
    ++path->active_operations;
    {
        Q9IOMAN_Status status = path->backend->operate(
            path->backend_context, path->backend_path, operation,
            arg0, arg1, arg2, result);
        --path->active_operations;
        return status;
    }
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
    if (path->active_operations != 0)
        return Q9IOMAN_E_BUSY;

    path->state = Q9IOMAN_PATH_CLOSING;
    status = path->backend->close(path->backend_context, path->backend_path);
    if (status != Q9IOMAN_OK) {
        path->state = Q9IOMAN_PATH_OPEN;
        return status;
    }

    path->state = Q9IOMAN_PATH_FREE;
    path->backend_path = 0;
    path->access_mode = 0;
    path->active_operations = 0;
    path->backend = 0;
    path->backend_context = 0;
    path->route_prefix = 0;
    return Q9IOMAN_OK;
}
