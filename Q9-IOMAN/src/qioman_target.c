#include "qioman_kernel.h"
#include "qioman_system.h"

Q9IOMAN_Status q9ioman_target_resolve_path(
    void *context,
    Q9IOMAN_u32 address,
    const char **path,
    Q9IOMAN_u32 *bytes_including_nul);

Q9IOMAN_Status q9ioman_target_dispatch(Q9IOMAN_u16 callcode,
                                       unsigned char *frame)
{
    Q9IOMAN_Manager *manager = q9ioman_system_manager();
    if (manager == 0)
        return Q9IOMAN_E_INVALID_ARGUMENT;
    return q9ioman_dispatch_kernel_request(callcode, manager, frame,
                                            q9ioman_target_resolve_path, 0);
}
