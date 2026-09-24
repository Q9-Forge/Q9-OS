#ifndef Q9_IOMAN_SYSTEM_H
#define Q9_IOMAN_SYSTEM_H

#include "qioman.h"

#ifndef Q9IOMAN_SYSTEM_PATH_CAPACITY
#define Q9IOMAN_SYSTEM_PATH_CAPACITY 32
#endif

/* Initialize the resident manager state once; safe to call again. */
Q9IOMAN_Status q9ioman_system_init(void);

/* Returns the resident manager after initialization, otherwise null. */
Q9IOMAN_Manager *q9ioman_system_manager(void);

#endif
