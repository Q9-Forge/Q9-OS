#include <types.h>
#include <io.h>
#include <errno.h>
#include "dhf.h"

/* Makro, um den Treiber via GetStat/SetStat anzusteuern */
#define CALL_DRIVER_IO(path) _os9_f_viread(((struct path_desc *)path)->pd_dev, &(path->cmd))

/* 
 * 1. CREATE
 */
error_code _sysio_create(DhfPath *path, char *name, u_int32 access_mode, u_int32 attrs) {
    path->cmd.command   = DHF_CMD_CREATE;
    path->cmd.path_name = name;
    path->cmd.param1    = access_mode;
    path->cmd.param2    = attrs;
    path->cmd.buffer    = NULL;

    error_code err = CALL_DRIVER_IO(path);
    if (err) return err;
    return path->cmd.status;
}

/* 
 * 2. OPEN
 */
error_code _sysio_open(DhfPath *path, char *name, u_int32 access_mode) {
    path->cmd.command   = DHF_CMD_OPEN;
    path->cmd.path_name = name;
    path->cmd.param1    = access_mode;
    path->cmd.buffer    = NULL;

    error_code err = CALL_DRIVER_IO(path);
    if (err) return err;
    return path->cmd.status;
}

/* 
 * 3. CLOSE
 */
error_code _sysio_close(DhfPath *path) {
    path->cmd.command   = DHF_CMD_CLOSE;
    path->cmd.buffer    = NULL;

    error_code err = CALL_DRIVER_IO(path);
    if (err) return err;
    return path->cmd.status;
}

/* 
 * 4. READ
 */
error_code _sysio_read(DhfPath *path, void *buf, u_int32 *size) {
    path->cmd.command   = DHF_CMD_READ;
    path->cmd.buffer    = buf;
    path->cmd.param1    = *size; /* Gewünschte Größe */

    error_code err = CALL_DRIVER_IO(path);
    if (err) return err;
    
    *size = path->cmd.param1; /* Vom Host tatsächlich gelesene Größe */
    return path->cmd.status;
}

/* 
 * 5. WRITE
 */
error_code _sysio_write(DhfPath *path, void *buf, u_int32 *size) {
    path->cmd.command   = DHF_CMD_WRITE;
    path->cmd.buffer    = buf;
    path->cmd.param1    = *size;

    error_code err = CALL_DRIVER_IO(path);
    if (err) return err;

    *size = path->cmd.param1; /* Vom Host tatsächlich geschriebene Größe */
    return path->cmd.status;
}

/* 
 * 6. SEEK
 */
error_code _sysio_seek(DhfPath *path, u_int32 position) {
    path->cmd.command   = DHF_CMD_SEEK;
    path->cmd.param1    = position;
    path->cmd.buffer    = NULL;

    error_code err = CALL_DRIVER_IO(path);
    if (err) return err;
    return path->cmd.status;
}

/* 
 * 7. DELETE
 */
error_code _sysio_delete(DhfPath *path, char *name) {
    path->cmd.command   = DHF_CMD_DELETE;
    path->cmd.path_name = name;
    path->cmd.buffer    = NULL;

    error_code err = CALL_DRIVER_IO(path);
    if (err) return err;
    return path->cmd.status;
}

/* 
 * 8. MAKDIR
 */
error_code _sysio_makdir(DhfPath *path, char *name, u_int32 attrs) {
    path->cmd.command   = DHF_CMD_MAKDIR;
    path->cmd.path_name = name;
    path->cmd.param1    = attrs;
    path->cmd.buffer    = NULL;

    error_code err = CALL_DRIVER_IO(path);
    if (err) return err;
    return path->cmd.status;
}

/* 
 * 9. CHGDIR
 */
error_code _sysio_chgdir(DhfPath *path, char *name, u_int32 mode) {
    path->cmd.command   = DHF_CMD_CHGDIR;
    path->cmd.path_name = name;
    path->cmd.param1    = mode;
    path->cmd.buffer    = NULL;

    error_code err = CALL_DRIVER_IO(path);
    if (err) return err;
    return path->cmd.status;
}

/* 
 * 10. READDIR (Verzeichnis lesen)
 */
error_code _sysio_readdir(DhfPath *path, void *buf, u_int32 *size) {
    path->cmd.command   = DHF_CMD_READDIR;
    path->cmd.buffer    = buf;
    path->cmd.param1    = *size;

    error_code err = CALL_DRIVER_IO(path);
    if (err) return err;

    *size = path->cmd.param1;
    return path->cmd.status;
}

/* 
 * 11. GETSTAT
 */
error_code _sysio_getstat(DhfPath *path, int code, void *status_reg) {
    path->cmd.command   = DHF_CMD_GETSTAT;
    path->cmd.param1    = (u_int32)code;
    path->cmd.buffer    = status_reg;

    error_code err = CALL_DRIVER_IO(path);
    if (err) return err;
    return path->cmd.status;
}

/* 
 * 12. SETSTAT
 */
error_code _sysio_setstat(DhfPath *path, int code, void *status_reg) {
    path->cmd.command   = DHF_CMD_SETSTAT;
    path->cmd.param1    = (u_int32)code;
    path->cmd.buffer    = status_reg;

    error_code err = CALL_DRIVER_IO(path);
    if (err) return err;
    return path->cmd.status;
}

/* 
 * 13. RENAME
 */
error_code _sysio_rename(DhfPath *path, char *old_name, char *new_name) {
    path->cmd.command   = DHF_CMD_RENAME;
    path->cmd.path_name = old_name;
    /* Wir missbrauchen den Puffer-Zeiger für den zweiten Namen */
    path->cmd.buffer    = (void *)new_name; 

    error_code err = CALL_DRIVER_IO(path);
    if (err) return err;
    return path->cmd.status;
}
