/* dhf_descriptor: stores base path for root filesystem mapping
 * Provides getter/setter for driver/manager to use.
 */

#include "dhf_descriptor.h"
#include <string.h>

static char basepath[1024] = "/";

const char* dhf_descriptor_get_basepath(void) { return basepath; }
int dhf_descriptor_set_basepath(const char *p) {
    if (!p) return -1;
    strncpy(basepath, p, sizeof(basepath)-1);
    basepath[sizeof(basepath)-1] = '\0';
    return 0;
}

