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
#if __INITIAL_POINTER_SIZE == 64
    char **ppargv = malloc(argc * sizeof(char*));
    for(int i = 0; i < argc; ++i) {
        ppargv[i] = argv[i];
    }
    int ret = Py_BytesMain(argc, ppargv);
    free(ppargv);
    exit(ret);
#else
    exit(Py_BytesMain(argc, argv));
#endif
#else
    return Py_BytesMain(argc, argv);
#endif
}
#endif
