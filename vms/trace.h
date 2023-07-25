#ifndef _DO_TRACE_FILE_
#define _TRACE_LINE_(line)
#define _TRACE_LINE_V_(line, ...)
#else
#include <fcntl.h>
#include <unixlib.h>
#define _TRACE_LINE_(line) \
    do {    \
        char _TRACE_LINE_name[64];  \
        sprintf(_TRACE_LINE_name, "/SYS$SCRATCH/" _DO_TRACE_FILE_ "%x.txt", getpid());   \
        int _TRACE_LINE_fd = open(_TRACE_LINE_name, O_CREAT | O_APPEND | O_RDWR, 0600); \
        if (_TRACE_LINE_fd) {   \
            write(_TRACE_LINE_fd, (line), strlen((line)));  \
            close(_TRACE_LINE_fd);  \
        }   \
    } while(0)

#define _TRACE_LINE_V_(line, ...) \
    do {    \
        char _TRACE_LINE_V_buf[256];  \
        snprintf(_TRACE_LINE_V_buf, sizeof(_TRACE_LINE_V_buf), (line), __VA_ARGS__); \
        _TRACE_LINE_(_TRACE_LINE_V_buf);  \
    } while(0)
#endif
