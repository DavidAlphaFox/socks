#ifndef __SOCKS_COMMON_H__
#define __SOCKS_COMMON_H__

#include <errno.h>

#define LEVEL_WARNING "warn"
#define LEVEL_ERROR "error"
#define LEVEL_INFORM "info"
#define LEVEL_DEBUG "debug"

#define LOG(level, format, ...) \
    do { \
        fprintf(stderr, "[%s|%s@%s,%d] " format "\n", \
            level, __func__, __FILE__, __LINE__, ##__VA_ARGS__ ); \
    } while (0)

#define GOTO_ERR \
 	do { \
 		LOG(LEVEL_ERROR, "go to error [%d].\n", errno); \
    	goto _err; \
    } while (0)


#endif