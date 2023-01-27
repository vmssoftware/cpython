/* Minimal main program -- everything is loaded from the library */

#include "Python.h"

#ifdef MS_WINDOWS
int
wmain(int argc, wchar_t **argv)
{
    return Py_Main(argc, argv);
}
#else
int
main(int argc, char **argv)
{
#ifdef __VMS

#include <unixio.h>

#undef _DO_TRACE_FILE_
// #define _DO_TRACE_FILE_ "PY_"
#ifndef _DO_TRACE_FILE_
#define _TRACE_LINE_(line)
#define _TRACE_LINE_V_(line, ...)
#else
#include <fcntl.h>
#include <unixlib.h>
#define _TRACE_LINE_(line) \
    do {    \
        char _TRACE_LINE_name[64];  \
        sprintf(_TRACE_LINE_name, _DO_TRACE_FILE_ "%x.txt", getpid());   \
        int _TRACE_LINE_fd = open(_TRACE_LINE_name, O_CREAT | O_APPEND | O_RDWR, 0600); \
        if (_TRACE_LINE_fd) {   \
            write(_TRACE_LINE_fd, (line), strlen((line)));  \
            close(_TRACE_LINE_fd);  \
        }   \
    } while(0)

#define _TRACE_LINE_V_(line, ...) \
    do {    \
        char _TRACE_LINE_V_buf[256];  \
        sprintf(_TRACE_LINE_V_buf, (line), __VA_ARGS__); \
        _TRACE_LINE_(_TRACE_LINE_V_buf);  \
    } while(0)
#endif

#ifdef _DO_TRACE_FILE_
    _TRACE_LINE_("start\n");
    char fd_name[256];
    for(int fd = 0; fd < 3; ++fd) {
        getname(fd, fd_name, 1);
        _TRACE_LINE_V_("%i: \"%s\", isapipe %i\n", fd, fd_name, isapipe(fd));
    }
#endif

#if __INITIAL_POINTER_SIZE == 64
    char **ppargv = malloc(argc * sizeof(char*));
    for(int i = 0; i < argc; ++i) {
        ppargv[i] = argv[i];
    }
    int ret = Py_BytesMain(argc, ppargv);
    free(ppargv);
    exit(ret);
#else
#ifdef _DO_TRACE_FILE_
    int ret = Py_BytesMain(argc, argv);
    _TRACE_LINE_("end\n");
    exit(ret);
#else
    exit(Py_BytesMain(argc, argv));
#endif
#endif
#else
    return Py_BytesMain(argc, argv);
#endif
}
#endif
