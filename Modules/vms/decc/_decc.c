#define PY_SSIZE_T_CLEAN
#include "Python.h"

#define __NEW_STARLET 1

#include <descrip.h>
#include <dlfcn.h>
#include <lib$routines.h>
#include <starlet.h>
#include <unixio.h>
#include <unixlib.h>

#define ConvertArgToStr(arg, value, size, func_name)            \
    if (PyUnicode_CheckExact(arg)) {                            \
        (value) = (char*)PyUnicode_AsUTF8AndSize(arg, &(size)); \
    } else if (PyBytes_CheckExact(arg)) {                       \
        PyBytes_AsStringAndSize(arg, &(value), &(size));        \
    }                                                           \
    if (!(value) || !(size)) {                                  \
        _PyArg_BadArgument(func_name, #arg, "str", arg);        \
        Py_RETURN_NONE;                                         \
    }

#define CheckArgAsLong(arg, func_name)                      \
    if (!PyLong_Check(arg)) {                               \
        _PyArg_BadArgument(func_name, #arg, "long", arg);   \
        Py_RETURN_NONE;                                     \
    }

// unsigned int _fix_time(long long vms_time)
static PyObject*
DECC_fix_time(
    PyObject * self,
    PyObject * args)
{
    CheckArgAsLong(args, "fix_time");
    unsigned long long vms_time = PyLong_AsUnsignedLongLong(args);

    unsigned int unix_time = -1;
    Py_BEGIN_ALLOW_THREADS
    unix_time = decc$fix_time(&vms_time);
    Py_END_ALLOW_THREADS
    return PyLong_FromUnsignedLong(unix_time);
}

#define UNIX_EPOCH "01-JAN-1970 00:00:00.00"
#define FAC 10000000L

static long long epoch() {
    static long long _epoch = 0;
    if (_epoch == 0)  {
        $DESCRIPTOR(epoch_dsc, UNIX_EPOCH);
        Py_BEGIN_ALLOW_THREADS
        sys$bintim(&epoch_dsc, (struct _generic_64 *) &_epoch);
        Py_END_ALLOW_THREADS
    }
    return _epoch;
}

static int offset() {
    static int _offset = -1;
    if (_offset == -1) {
        time_t gmt, rawtime = time(NULL);
        struct tm *ptm;
        struct tm gbuf;
        ptm = gmtime_r(&rawtime, &gbuf);
        ptm->tm_isdst = -1;
        gmt = mktime(ptm);
        _offset = (int) difftime(rawtime, gmt);
    }
    return _offset;
}

// unsigned int _unixtime(long long dt)
static PyObject*
DECC_unixtime(
    PyObject * self,
    PyObject * args)
{
    CheckArgAsLong(args, "unixtime");
    unsigned long long vms_time = PyLong_AsUnsignedLongLong(args);

    unsigned long long diff = vms_time - epoch();
    unsigned int sec = diff / FAC - offset();
    return PyLong_FromUnsignedLong(sec);
}

// long long _vmstime(unsigned int dt)
static PyObject*
DECC_vmstime(
    PyObject * self,
    PyObject * args)
{
    CheckArgAsLong(args, "vmstime");

    unsigned int unix_time = PyLong_AsUnsignedLong(args);

    unsigned long long val = epoch() + (((unsigned long long)unix_time) * FAC);

    return PyLong_FromUnsignedLongLong(val);
}

static int cb_from_vms(__char_ptr32 name, __void_ptr32 user_data)
{
    PyObject *pTuple = (PyObject *)*(void**)user_data;
    if (pTuple && PyTuple_CheckExact(pTuple) && PyTuple_Size(pTuple) == 2) {
        PyObject *pList = PyTuple_GetItem(pTuple, 0);
        if (pList && pList != Py_None && PyList_CheckExact(pList)) {
            PyObject *pName = PyUnicode_FromString(name);
            PyObject *pFunction = PyTuple_GetItem(pTuple, 1);
            if (pFunction && pFunction != Py_None && PyCallable_Check(pFunction)) {
                // call pFunction
                PyObject *pResult = PyObject_CallOneArg(pFunction, pName);
                if (PyErr_Occurred()) {
                    return 0;
                }
                if (pResult && pResult != Py_None && PyUnicode_CheckExact(pResult)) {
                    Py_DECREF(pName);
                    PyList_Append(pList, pResult);
                    Py_DECREF(pResult);
                }
            } else {
                PyList_Append(pList, pName);
                Py_DECREF(pName);
            }
        }
    }
    return 1;
}

// char **_from_vms(char *path, int wild_flag, PyObject *callback)
static PyObject*
DECC_from_vms(
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("from_vms", nargs, 1, 3)) {
        Py_RETURN_NONE;
    }

    char *path = NULL;
    Py_ssize_t path_size = 0;

    ConvertArgToStr(args[0], path, path_size, "from_vms");

    PyObject *pList = PyList_New(0);
    if (!pList) {
        Py_RETURN_NONE;
    }

    int wild_flag = 0;
    if (nargs > 1 && args[1] != Py_None) {
        if (args[1] == Py_True) {
            wild_flag = 1;
        } else if (args[1] == Py_False) {
            wild_flag = 0;
        } else {
            CheckArgAsLong(args[1], "from_vms");
            wild_flag = PyLong_AsLong(args[1]);
        }
    }

    PyObject *pFunction = NULL;

    if (nargs > 2 && args[2] != Py_None) {
        if (!PyCallable_Check(args[2])) {
            _PyArg_BadArgument("from_vms", "args[2]", "callable", args[2]);
            Py_RETURN_NONE;
        }
        pFunction = args[2];
    } else {
        pFunction = Py_None;
    }
    Py_INCREF(pFunction);

    PyObject *pTuple = PyTuple_New(2);
    if (!pTuple) {
        Py_DECREF(pList);
        Py_RETURN_NONE;
    }

    if (PyTuple_SetItem(pTuple, 0, pList)) {
        Py_DECREF(pTuple);
        Py_DECREF(pList);
        Py_RETURN_NONE;
    }
    if (PyTuple_SetItem(pTuple, 1, pFunction)) {
        Py_DECREF(pTuple);
        Py_DECREF(pFunction);
        Py_RETURN_NONE;
    }

    int num_files = decc$from_vms(path, (__from_vms_callback)cb_from_vms, wild_flag, &pTuple);

    Py_INCREF(pList);
    Py_DECREF(pTuple);

    return pList;
}

static int cb_to_vms(__char_ptr32 name, int file_type, __void_ptr32 user_data)
{
    PyObject *pTuple = (PyObject *)*(void**)user_data;
    if (pTuple && PyTuple_CheckExact(pTuple) && PyTuple_Size(pTuple) == 2) {
        PyObject *pList = PyTuple_GetItem(pTuple, 0);
        if (pList && pList != Py_None && PyList_CheckExact(pList)) {
            PyObject *pName = PyUnicode_FromString(name);
            PyObject *pFunction = PyTuple_GetItem(pTuple, 1);
            if (pFunction && pFunction != Py_None && PyCallable_Check(pFunction)) {
                // call pFunction
                PyObject *pArgs = PyTuple_New(2);
                PyTuple_SetItem(pArgs, 0, pName);
                PyTuple_SetItem(pArgs, 1, PyLong_FromLong(file_type));
                PyObject *pResult = PyObject_CallObject(pFunction, pArgs);
                if (PyErr_Occurred()) {
                    return 0;
                }
                Py_DECREF(pArgs);
                if (pResult && pResult != Py_None && PyUnicode_CheckExact(pResult)) {
                    PyList_Append(pList, pResult);
                    Py_DECREF(pResult);
                }
            } else {
                PyList_Append(pList, pName);
                Py_DECREF(pName);
            }
        }
    }
    return 1;
}

// char **_to_vms(char *path, int allow_wild, int no_directory)
static PyObject*
DECC_to_vms(
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("to_vms", nargs, 1, 4)) {
        Py_RETURN_NONE;
    }

    char *path = NULL;
    Py_ssize_t path_size = 0;

    ConvertArgToStr(args[0], path, path_size, "to_vms");

    PyObject *pList = PyList_New(0);
    if (!pList) {
        Py_RETURN_NONE;
    }

    int allow_wild = 0;
    if (nargs > 1 && args[1] != Py_None) {
        if (args[1] == Py_True) {
            allow_wild = 1;
        } else if (args[1] == Py_False) {
            allow_wild = 0;
        } else {
            CheckArgAsLong(args[1], "to_vms");
            allow_wild = PyLong_AsLong(args[1]);
        }
    }

    int no_directory = 0;
    if (nargs > 2 && args[2] != Py_None) {
        if (args[2] == Py_True) {
            no_directory = 1;
        } else if (args[2] == Py_False) {
            no_directory = 0;
        } else {
            CheckArgAsLong(args[2], "to_vms");
            no_directory = PyLong_AsLong(args[2]);
        }
    }

    PyObject *pFunction = NULL;

    if (nargs > 3 && args[3] != Py_None) {
        if (!PyCallable_Check(args[3])) {
            _PyArg_BadArgument("to_vms", "args[3]", "callable", args[2]);
            Py_RETURN_NONE;
        }
        pFunction = args[3];
    } else {
        pFunction = Py_None;
    }
    Py_INCREF(pFunction);

    PyObject *pTuple = PyTuple_New(2);
    if (!pTuple) {
        Py_DECREF(pList);
        Py_RETURN_NONE;
    }

    if (PyTuple_SetItem(pTuple, 0, pList)) {
        Py_DECREF(pTuple);
        Py_DECREF(pList);
        Py_RETURN_NONE;
    }
    if (PyTuple_SetItem(pTuple, 1, pFunction)) {
        Py_DECREF(pTuple);
        Py_DECREF(pFunction);
        Py_RETURN_NONE;
    }

    int num_files = decc$to_vms(path, (__to_vms_callback)cb_to_vms, allow_wild, no_directory, &pTuple);

    Py_INCREF(pList);
    Py_DECREF(pTuple);

    return pList;
}

// char *_getenv(char *name, char *def)
static PyObject*
DECC_getenv(
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("getenv", nargs, 1, 2)) {
        Py_RETURN_NONE;
    }

    char *name = NULL;
    Py_ssize_t name_size = 0;

    ConvertArgToStr(args[0], name, name_size, "getenv");

    char *val = NULL;

    Py_BEGIN_ALLOW_THREADS

    val = getenv(name);

    Py_END_ALLOW_THREADS

    if (!val) {
        if (nargs > 1 && args[1] != Py_None) {
            char *def = NULL;
            Py_ssize_t def_size = 0;
            if (PyUnicode_CheckExact(args[1])) {
                def = (char*)PyUnicode_AsUTF8AndSize(args[1], &def_size);
            } else if (PyBytes_CheckExact(args[1])) {
                PyBytes_AsStringAndSize(args[1], &def, &def_size);
            }
            if (def && def_size) {
                val = def;
            }
        }
    }

    if (val) {
        return PyUnicode_FromString(val);
    }

    Py_RETURN_NONE;
}

// long _sysconf(int name)
static PyObject*
DECC_sysconf(
    PyObject * self,
    PyObject * args)
{
    CheckArgAsLong(args, "sysconf");
    int name = PyLong_AsLong(args);
    int value = 0;
    Py_BEGIN_ALLOW_THREADS
    value = sysconf(name);
    Py_END_ALLOW_THREADS
    return PyLong_FromLong(value);
}

// int _sleep(unsigned int nsec)
static PyObject*
DECC_sleep(
    PyObject * self,
    PyObject * args)
{
    CheckArgAsLong(args, "sleep");
    unsigned int sleep_seconds = PyLong_AsUnsignedLong(args);
    unsigned int slept_seconds = 0;
    Py_BEGIN_ALLOW_THREADS
    slept_seconds = sleep(sleep_seconds);
    Py_END_ALLOW_THREADS
    return PyLong_FromUnsignedLong(slept_seconds);
}

// int _dlopen_test(char *name)
static PyObject*
DECC_dlopen_test(
    PyObject * self,
    PyObject * args)
{
    char *name = NULL;
    Py_ssize_t size = 0;

    ConvertArgToStr(args, name, size, "dlopen_test");

    int status = 0;
    void *handle = dlopen(name, 0);
    if (handle) {
        dlclose(handle);
        status = 1;
    }

    return PyLong_FromLong(status);
}

static PyObject*
DECC_fd_name(
    PyObject * self,
    PyObject * args)
{
    int fd = 0;
    if (!PyLong_Check(args)) {
        _PyArg_BadArgument("fd_name", "args", "long", args);
        return NULL;
    }
    fd = PyLong_AsLong(args);
    char devicename[256];
    if (NULL == getname(fd, devicename, sizeof(devicename), 1)) {
        Py_RETURN_NONE;
    }
    return PyUnicode_FromString(devicename);
}

/********************************************************************
  Module
*/

static PyMethodDef _module_methods[] = {
    {"fix_time", (PyCFunction) DECC_fix_time, METH_O,
        PyDoc_STR("fix_time(time: number)->time: number   Converts VMS time to Unix time")},
    {"unixtime", (PyCFunction) DECC_unixtime, METH_O,
        PyDoc_STR("unixtime(time: number)->time: number   Converts VMS system time to Unix time")},
    {"vmstime", (PyCFunction) DECC_vmstime, METH_O,
        PyDoc_STR("vmstime(time: number)->time: number   Converts Unix time to the VMS system time")},
    {"sleep", (PyCFunction) DECC_sleep, METH_O,
        PyDoc_STR("sleep(seconds: number)->seconds: number   Sleep (in seconds)")},
    {"sysconf", (PyCFunction) DECC_sysconf, METH_O,
        PyDoc_STR("sysconf(name: number)->value: number   Returns system configuration value")},
    {"getenv", (PyCFunction) DECC_getenv, METH_FASTCALL,
        PyDoc_STR("getenv(name: str, ?default: str)->value: str   getenv() wrapper")},
    {"from_vms", (PyCFunction) DECC_from_vms, METH_FASTCALL,
        PyDoc_STR("from_vms(vms_path: str, ?wild_flag: boolean, ?callback: callable)->[unix_path: str, ...]   from_vms() wrapper")},
    {"to_vms", (PyCFunction) DECC_to_vms, METH_FASTCALL,
        PyDoc_STR("to_vms(unix_path: str, ?allow_wild: boolean, ?no_directory: boolean, ?callback: callable)->[vms_path: str, ...]   to_vms() wrapper")},
    {"dlopen_test", (PyCFunction) DECC_dlopen_test, METH_O,
        PyDoc_STR("dlopen_test(name: str)->status: number   Returns 1 on success")},
    {"fd_name", (PyCFunction) DECC_fd_name, METH_O,
        PyDoc_STR("fd_name(fd: int)->name: str   Returns vms name for given fd")},
    {NULL, NULL}
};

static struct PyModuleDef _module_definition = {
    PyModuleDef_HEAD_INIT,
    "_decc",
    PyDoc_STR("OpenVMS DECC wrapper"),
    -1,
    _module_methods,
    NULL,
    NULL,
    NULL,
    NULL,
};

PyMODINIT_FUNC
PyInit__decc(void)
{
    PyObject *m = PyModule_Create(&_module_definition);
    if (m == NULL) {
        return NULL;
    }
    PyModule_AddIntConstant(m, "DECC_SC_ARG_MAX", 100);
    PyModule_AddIntConstant(m, "DECC_SC_CHILD_MAX", 101);
    PyModule_AddIntConstant(m, "DECC_SC_CLK_TCK", 102);
    PyModule_AddIntConstant(m, "DECC_SC_NGROUPS_MAX", 103);
    PyModule_AddIntConstant(m, "DECC_SC_OPEN_MAX", 104);
    PyModule_AddIntConstant(m, "DECC_SC_STREAM_MAX", 105);
    PyModule_AddIntConstant(m, "DECC_SC_TZNAME_MAX", 106);
    PyModule_AddIntConstant(m, "DECC_SC_JOB_CONTROL", 107);
    PyModule_AddIntConstant(m, "DECC_SC_SAVED_IDS", 108);
    PyModule_AddIntConstant(m, "DECC_SC_VERSION", 109);
    PyModule_AddIntConstant(m, "DECC_SC_BC_BASE_MAX", 121);
    PyModule_AddIntConstant(m, "DECC_SC_BC_DIM_MAX", 122);
    PyModule_AddIntConstant(m, "DECC_SC_BC_SCALE_MAX", 123);
    PyModule_AddIntConstant(m, "DECC_SC_BC_STRING_MAX", 124);
    PyModule_AddIntConstant(m, "DECC_SC_COLL_WEIGHTS_MAX", 125);
    PyModule_AddIntConstant(m, "DECC_SC_EXPR_NEST_MAX", 126);
    PyModule_AddIntConstant(m, "DECC_SC_LINE_MAX", 127);
    PyModule_AddIntConstant(m, "DECC_SC_RE_DUP_MAX", 128);
    PyModule_AddIntConstant(m, "DECC_SC_2_VERSION", 129);
    PyModule_AddIntConstant(m, "DECC_SC_2_C_BIND", 130);
    PyModule_AddIntConstant(m, "DECC_SC_2_C_DEV", 131);
    PyModule_AddIntConstant(m, "DECC_SC_2_FORT_DEV", 132);
    PyModule_AddIntConstant(m, "DECC_SC_2_SW_DEV", 133);
    PyModule_AddIntConstant(m, "DECC_SC_2_FORT_RUN", 134);
    PyModule_AddIntConstant(m, "DECC_SC_2_LOCALEDEF", 135);
    PyModule_AddIntConstant(m, "DECC_SC_2_UPE", 136);
    PyModule_AddIntConstant(m, "DECC_SC_GETGR_R_SIZE_MAX", 150);
    PyModule_AddIntConstant(m, "DECC_SC_GETPW_R_SIZE_MAX", 151);
    PyModule_AddIntConstant(m, "DECC_SC_THREAD_DESTRUCTOR_ITERATIONS", 140);
    PyModule_AddIntConstant(m, "DECC_SC_THREAD_KEYS_MAX", 141);
    PyModule_AddIntConstant(m, "DECC_SC_THREAD_STACK_MIN", 142);
    PyModule_AddIntConstant(m, "DECC_SC_THREAD_THREADS_MAX", 143);
    PyModule_AddIntConstant(m, "DECC_SC_THREAD_SAFE_FUNCTIONS", 144);
    PyModule_AddIntConstant(m, "DECC_SC_THREADS", 145);
    PyModule_AddIntConstant(m, "DECC_SC_THREAD_ATTR_STACKSIZE", 146);
    PyModule_AddIntConstant(m, "DECC_SC_THREAD_PRIORITY_SCHEDULING", 147);
    PyModule_AddIntConstant(m, "DECC_SC_XOPEN_VERSION", 110);
    PyModule_AddIntConstant(m, "DECC_SC_PASS_MAX", 111);
    PyModule_AddIntConstant(m, "DECC_SC_XOPEN_CRYPT", 118);
    PyModule_AddIntConstant(m, "DECC_SC_XOPEN_ENH_I18N", 119);
    PyModule_AddIntConstant(m, "DECC_SC_XOPEN_SHM", 120);
    PyModule_AddIntConstant(m, "DECC_SC_PAGESIZE", 117);
    PyModule_AddIntConstant(m, "DECC_SC_PAGE_SIZE", 117); 		/* _SC_PAGESIZE */
    PyModule_AddIntConstant(m, "DECC_SC_ATEXIT_MAX", 138); 		/* not in POSIX, but in XPG4 */
    PyModule_AddIntConstant(m, "DECC_SC_IOV_MAX", 139); 		/* not in POSIX, but in XPG4 */
    PyModule_AddIntConstant(m, "DECC_SC_XOPEN_UNIX", 148);
    PyModule_AddIntConstant(m, "DECC_SC_NPROC_CONF", 200);
    PyModule_AddIntConstant(m, "DECC_SC_NPROCESSORS_CONF", 200);	/* _SC_NPROC_CONF */
    PyModule_AddIntConstant(m, "DECC_SC_NPROC_ONLN", 201);
    PyModule_AddIntConstant(m, "DECC_SC_NPROCESSORS_ONLN", 201);	/* _SC_NPROC_ONLN */
    PyModule_AddIntConstant(m, "DECC_SC_2_C_VERSION", 129);		/* _SC_2_VERSION  */
    PyModule_AddIntConstant(m, "DECC_SC_2_CHAR_TERM", 137);
    PyModule_AddIntConstant(m, "DECC_SC_DELAYTIMER_MAX", 112);
    PyModule_AddIntConstant(m, "DECC_SC_TIMER_MAX", 113);
    PyModule_AddIntConstant(m, "DECC_SC_MAPPED_FILES", 114);
    PyModule_AddIntConstant(m, "DECC_SC_FSYNC", 115);
    PyModule_AddIntConstant(m, "DECC_SC_TIMERS", 116);
    PyModule_AddIntConstant(m, "DECC_SC_CPU_CHIP_TYPE", 160);

    return (m);
}
