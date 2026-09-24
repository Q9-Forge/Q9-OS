#include "qioman_system.h"

static Q9IOMAN_Manager q9ioman_system_state;
static Q9IOMAN_Path q9ioman_system_paths[Q9IOMAN_SYSTEM_PATH_CAPACITY];
static Q9IOMAN_u16 q9ioman_system_initialized;

Q9IOMAN_Status q9ioman_system_init(void)
{
    Q9IOMAN_Status status;

    if (q9ioman_system_initialized != 0)
        return Q9IOMAN_OK;

    status = q9ioman_init(&q9ioman_system_state,
                          q9ioman_system_paths,
                          Q9IOMAN_SYSTEM_PATH_CAPACITY);
    if (status == Q9IOMAN_OK)
        q9ioman_system_initialized = 1;
    return status;
}

Q9IOMAN_Manager *q9ioman_system_manager(void)
{
    if (q9ioman_system_initialized == 0)
        return 0;
    return &q9ioman_system_state;
}
