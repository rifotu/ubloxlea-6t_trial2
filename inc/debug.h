#ifndef __dbg_h__
#define __dbg_h__

/* For an introduction to how to use these Macros check
 * http://c.learncodethehardway.org/book/ex20.html
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "config.h"

#ifdef NDEBUG
#define debug(M, ...)
#else
#define debug(M, ...) fprintf(stderr, "DEBUG %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

//#define clean_errno() (errno == 0 ? "None" : strerror(errno))
//
//#define log_err(M, ...) fprintf(stderr, "[ERROR] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)
//
//#define log_warn(M, ...) fprintf(stderr, "[WARN] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)
//
//#define log_info(M, ...) fprintf(stderr, "[INFO] (%s:%d) " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
//
//#define check(A, M, ...) if(!(A)) { log_err(M, ##__VA_ARGS__); errno=0; goto error; }
//
//#define sentinel(M, ...)  { log_err(M, ##__VA_ARGS__); errno=0; goto error; }
//
//#define check_mem(A) check((A), "Out of memory.")
//
//#define check_debug(A, M, ...) if(!(A)) { debug(M, ##__VA_ARGS__); errno=0; goto error; }


#define LOG_FATAL    (1)
#define LOG_ERR      (2)
#define LOG_WARN     (3)
#define LOG_INFO     (4)
#define LOG_DBG      (5)

#define LOG(level, ...) do {  \
                            if (level <= dbgLevel_G) { \
                                fprintf(fpDbg_G,"%s:%d:", __FILE__, __LINE__); \
                                fprintf(fpDbg_G, __VA_ARGS__); \
                                fprintf(fpDbg_G, "\n"); \
                                fflush(fpDbg_G); \
                            } \
                        } while (0)

extern FILE *fpDbg_G;
extern int  dbgLevel_G;  // DEBUG_LEVEL is set at main.c in main()

#endif
