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
#include "vms/trace.h"
#ifdef _DO_TRACE_FILE_
    extern int vms_isapipe(int fd);
    _TRACE_LINE_("start\n");
    char fd_name[256];
    for(int fd = 0; fd < 3; ++fd) {
        getname(fd, fd_name, 1);
        _TRACE_LINE_V_("%i: \"%s\", isapipe %i vms_isapipe %i\n", fd, fd_name, isapipe(fd), vms_isapipe(fd));
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
