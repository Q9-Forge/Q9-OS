#include "qioman_kernel.h"

Q9IOMAN_u32 q9ioman_frame_read32(const unsigned char *frame,
                                 Q9IOMAN_u16 offset)
{
    const unsigned char *byte;
    if (frame == 0)
        return 0;
    byte = frame + offset;
    return ((Q9IOMAN_u32)byte[0] << 24) |
           ((Q9IOMAN_u32)byte[1] << 16) |
           ((Q9IOMAN_u32)byte[2] << 8) |
           (Q9IOMAN_u32)byte[3];
}

void q9ioman_frame_write32(unsigned char *frame,
                           Q9IOMAN_u16 offset,
                           Q9IOMAN_u32 value)
{
    unsigned char *byte;
    if (frame == 0)
        return;
    byte = frame + offset;
    byte[0] = (unsigned char)((value >> 24) & 0xffUL);
    byte[1] = (unsigned char)((value >> 16) & 0xffUL);
    byte[2] = (unsigned char)((value >> 8) & 0xffUL);
    byte[3] = (unsigned char)(value & 0xffUL);
}

Q9IOMAN_u16 q9ioman_frame_read16(const unsigned char *frame,
                                 Q9IOMAN_u16 offset)
{
    const unsigned char *byte;
    if (frame == 0)
        return 0;
    byte = frame + offset;
    return (Q9IOMAN_u16)(((Q9IOMAN_u16)byte[0] << 8) | byte[1]);
}

void q9ioman_frame_write16(unsigned char *frame,
                           Q9IOMAN_u16 offset,
                           Q9IOMAN_u16 value)
{
    unsigned char *byte;
    if (frame == 0)
        return;
    byte = frame + offset;
    byte[0] = (unsigned char)((value >> 8) & 0xffU);
    byte[1] = (unsigned char)(value & 0xffU);
}

Q9IOMAN_u16 q9ioman_status_to_os9_error(Q9IOMAN_Status status)
{
    switch (status) {
    case Q9IOMAN_OK:
        return 0;
    case Q9IOMAN_E_INVALID_ARGUMENT:
        return Q9IOMAN_OS9_E_PARAM;
    case Q9IOMAN_E_INVALID_PATH:
        return Q9IOMAN_OS9_E_BPNUM;
    case Q9IOMAN_E_WRONG_MODE:
        return Q9IOMAN_OS9_E_BMODE;
    case Q9IOMAN_E_NO_PATH_SLOTS:
    case Q9IOMAN_E_REGISTRY_FULL:
        return Q9IOMAN_OS9_E_PTHFUL;
    case Q9IOMAN_E_NOT_FOUND:
        return Q9IOMAN_OS9_E_MNF;
    case Q9IOMAN_E_UNSUPPORTED_OPERATION:
    default:
        return Q9IOMAN_OS9_E_UNKSVC;
    }
}

Q9IOMAN_Status q9ioman_decode_kernel_request(
    Q9IOMAN_u16 callcode,
    const unsigned char *frame,
    Q9IOMAN_KernelRequest *request)
{
    if (frame == 0 || request == 0)
        return Q9IOMAN_E_INVALID_ARGUMENT;

    request->type = Q9IOMAN_KERNEL_NONE;
    request->path = 0;
    request->mode = 0;
    request->selector = 0;
    request->status_code = 0;
    request->buffer = 0;
    request->length = 0;
    request->position = 0;

    if (callcode == 0x0083) {
        request->type = Q9IOMAN_KERNEL_CREATE;
        request->mode = (Q9IOMAN_u16)(q9ioman_frame_read32(
            frame, Q9IOMAN_R_D0) & 0xffUL);
        request->buffer = q9ioman_frame_read32(frame, Q9IOMAN_R_A0);
        return Q9IOMAN_OK;
    } else if (callcode == 0x0084) {
        request->type = Q9IOMAN_KERNEL_OPEN;
        request->mode = (Q9IOMAN_u16)(q9ioman_frame_read32(
            frame, Q9IOMAN_R_D0) & 0xffUL);
        request->buffer = q9ioman_frame_read32(frame, Q9IOMAN_R_A0);
        return Q9IOMAN_OK;
    } else if (callcode == 0x0085) {
        request->type = Q9IOMAN_KERNEL_MAKDIR;
        request->buffer = q9ioman_frame_read32(frame, Q9IOMAN_R_A0);
        return Q9IOMAN_OK;
    } else if (callcode == 0x0086) {
        request->type = Q9IOMAN_KERNEL_CHGDIR;
        request->selector = q9ioman_frame_read16(frame, Q9IOMAN_R_D0 + 2);
        request->buffer = q9ioman_frame_read32(frame, Q9IOMAN_R_A0);
        return Q9IOMAN_OK;
    } else if (callcode == 0x0087) {
        request->type = Q9IOMAN_KERNEL_DELETE;
        request->buffer = q9ioman_frame_read32(frame, Q9IOMAN_R_A0);
        return Q9IOMAN_OK;
    } else if (callcode == 0x0088) {
        request->type = Q9IOMAN_KERNEL_SEEK;
        request->path = (Q9IOMAN_u16)q9ioman_frame_read32(
            frame, Q9IOMAN_R_D0);
        request->position = q9ioman_frame_read32(frame, Q9IOMAN_R_D1);
        return Q9IOMAN_OK;
    } else if (callcode == 0x0089) {
        request->type = Q9IOMAN_KERNEL_READ;
        request->path = (Q9IOMAN_u16)q9ioman_frame_read32(
            frame, Q9IOMAN_R_D0);
        request->length = q9ioman_frame_read32(frame, Q9IOMAN_R_D1);
        request->buffer = q9ioman_frame_read32(frame, Q9IOMAN_R_A0);
        return Q9IOMAN_OK;
    } else if (callcode == 0x008a || callcode == 0x008b ||
               callcode == 0x008c) {
        request->type = callcode == 0x008a ? Q9IOMAN_KERNEL_WRITE :
            (callcode == 0x008b ? Q9IOMAN_KERNEL_READ_LINE :
                                  Q9IOMAN_KERNEL_WRITE_LINE);
        request->path = (Q9IOMAN_u16)q9ioman_frame_read32(
            frame, Q9IOMAN_R_D0);
        request->length = q9ioman_frame_read32(frame, Q9IOMAN_R_D1);
        request->buffer = q9ioman_frame_read32(frame, Q9IOMAN_R_A0);
        return Q9IOMAN_OK;
    } else if (callcode == 0x008d || callcode == 0x008e) {
        request->type = callcode == 0x008d ? Q9IOMAN_KERNEL_GET_STATUS :
                                             Q9IOMAN_KERNEL_SET_STATUS;
        request->path = (Q9IOMAN_u16)q9ioman_frame_read32(
            frame, Q9IOMAN_R_D0);
        request->status_code = q9ioman_frame_read16(frame, Q9IOMAN_R_D1 + 2);
        request->buffer = q9ioman_frame_read32(frame, Q9IOMAN_R_A0);
        return Q9IOMAN_OK;
    } else if (callcode == 0x008f) {
        request->type = Q9IOMAN_KERNEL_CLOSE;
        request->path = (Q9IOMAN_u16)q9ioman_frame_read32(
            frame, Q9IOMAN_R_D0);
        return Q9IOMAN_OK;
    }
    return Q9IOMAN_E_UNSUPPORTED_OPERATION;
}

static void q9ioman_set_carry(unsigned char *frame, int set)
{
    Q9IOMAN_u16 status = q9ioman_frame_read16(frame, Q9IOMAN_R_SR);
    if (set)
        status = (Q9IOMAN_u16)(status | 1U);
    else
        status = (Q9IOMAN_u16)(status & 0xfffeU);
    q9ioman_frame_write16(frame, Q9IOMAN_R_SR, status);
}

Q9IOMAN_Status q9ioman_dispatch_kernel_request(
    Q9IOMAN_u16 callcode,
    Q9IOMAN_Manager *manager,
    unsigned char *frame,
    Q9IOMAN_PathAddressFn resolve_path,
    void *address_context)
{
    Q9IOMAN_KernelRequest request;
    Q9IOMAN_Result result;
    Q9IOMAN_Status status;
    Q9IOMAN_u16 path;

    if (frame == 0)
        return Q9IOMAN_E_INVALID_ARGUMENT;

    if (manager == 0) {
        status = Q9IOMAN_E_INVALID_ARGUMENT;
        goto failed;
    }

    status = q9ioman_decode_kernel_request(callcode, frame, &request);
    if (status != Q9IOMAN_OK)
        goto failed;

    if (request.type == Q9IOMAN_KERNEL_OPEN ||
        request.type == Q9IOMAN_KERNEL_CREATE) {
        const char *name = 0;
        Q9IOMAN_u32 name_bytes = 0;
        if (resolve_path == 0) {
            status = Q9IOMAN_E_INVALID_ARGUMENT;
            goto failed;
        }
        status = resolve_path(address_context, request.buffer,
                              &name, &name_bytes);
        if (status != Q9IOMAN_OK)
            goto failed;
        if (name == 0 || name_bytes == 0 ||
            request.buffer > 0xffffffffUL - name_bytes) {
            status = Q9IOMAN_E_INVALID_ARGUMENT;
            goto failed;
        }
        if (request.type == Q9IOMAN_KERNEL_CREATE)
            status = q9ioman_create_resolved(manager, name, request.mode,
                                              &path);
        else
            status = q9ioman_open_resolved(manager, name, request.mode,
                                            &path);
        if (status != Q9IOMAN_OK)
            goto failed;
        q9ioman_frame_write16(frame, Q9IOMAN_R_D0 + 2, path);
        q9ioman_frame_write32(frame, Q9IOMAN_R_A0,
                              request.buffer + name_bytes);
    } else if (request.type == Q9IOMAN_KERNEL_MAKDIR ||
               request.type == Q9IOMAN_KERNEL_DELETE) {
        const char *name = 0;
        Q9IOMAN_u32 name_bytes = 0;
        if (resolve_path == 0) {
            status = Q9IOMAN_E_INVALID_ARGUMENT;
            goto failed;
        }
        status = resolve_path(address_context, request.buffer,
                              &name, &name_bytes);
        if (status != Q9IOMAN_OK)
            goto failed;
        if (name == 0 || name_bytes == 0 ||
            request.buffer > 0xffffffffUL - name_bytes) {
            status = Q9IOMAN_E_INVALID_ARGUMENT;
            goto failed;
        }
        status = q9ioman_name_operation(
            manager, name,
            request.type == Q9IOMAN_KERNEL_MAKDIR
                ? Q9IOMAN_OP_MAKE_DIR : Q9IOMAN_OP_DELETE,
            &result);
        if (status != Q9IOMAN_OK)
            goto failed;
    } else if (request.type == Q9IOMAN_KERNEL_READ ||
               request.type == Q9IOMAN_KERNEL_WRITE ||
               request.type == Q9IOMAN_KERNEL_READ_LINE ||
               request.type == Q9IOMAN_KERNEL_WRITE_LINE) {
        Q9IOMAN_Operation operation = Q9IOMAN_OP_READ;
        if (request.type == Q9IOMAN_KERNEL_WRITE)
            operation = Q9IOMAN_OP_WRITE;
        else if (request.type == Q9IOMAN_KERNEL_READ_LINE)
            operation = Q9IOMAN_OP_READ_LINE;
        else if (request.type == Q9IOMAN_KERNEL_WRITE_LINE)
            operation = Q9IOMAN_OP_WRITE_LINE;
        if (request.length != 0 &&
            (request.buffer == 0 ||
             request.buffer > 0xffffffffUL - (request.length - 1))) {
            status = Q9IOMAN_E_INVALID_ARGUMENT;
            goto failed;
        }
        status = q9ioman_operate(manager, request.path, operation,
                                 request.buffer, 0, request.length, &result);
        if (status != Q9IOMAN_OK)
            goto failed;
        if (result.transferred > request.length) {
            status = Q9IOMAN_E_INVALID_ARGUMENT;
            goto failed;
        }
        q9ioman_frame_write32(frame, Q9IOMAN_R_D1, result.transferred);
    } else if (request.type == Q9IOMAN_KERNEL_SEEK) {
        status = q9ioman_operate(manager, request.path, Q9IOMAN_OP_SEEK,
                                 request.position, 0, 0, &result);
        if (status != Q9IOMAN_OK)
            goto failed;
    } else if (request.type == Q9IOMAN_KERNEL_GET_STATUS) {
        status = q9ioman_operate(manager, request.path, Q9IOMAN_OP_GET_STATUS,
                                 request.status_code, request.buffer, 0,
                                 &result);
        if (status != Q9IOMAN_OK)
            goto failed;
    } else if (request.type == Q9IOMAN_KERNEL_SET_STATUS) {
        status = q9ioman_operate(manager, request.path, Q9IOMAN_OP_SET_STATUS,
                                 request.status_code, request.buffer, 0,
                                 &result);
        if (status != Q9IOMAN_OK)
            goto failed;
    } else if (request.type == Q9IOMAN_KERNEL_CLOSE) {
        status = q9ioman_close(manager, request.path);
        if (status != Q9IOMAN_OK)
            goto failed;
    } else {
        status = Q9IOMAN_E_UNSUPPORTED_OPERATION;
        goto failed;
    }

    q9ioman_set_carry(frame, 0);
    return Q9IOMAN_OK;

failed:
    q9ioman_frame_write16(frame, Q9IOMAN_R_D1 + 2,
                          q9ioman_status_to_os9_error(status));
    q9ioman_set_carry(frame, 1);
    return status;
}
