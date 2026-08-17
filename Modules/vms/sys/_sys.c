#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "structmember.h"

#define __NEW_STARLET 1

#include <ciadef.h>
#include <delprcsymdef.h>
#include <descrip.h>
#include <efndef.h>
#include <gen64def.h>
#include <iodef.h>
#include <iosbdef.h>
#include <lib$routines.h>
#include <opcdef.h>
#include <psldef.h>
#include <ssdef.h>
#include <starlet.h>
#include <stsdef.h>

#include "modules/vms/ile3/_ile3.h"

#ifndef DEF_TABNAM
#define DEF_TABNAM "LNM$FILE_DEV"
#endif

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#include "vms/vms_dsc.h"

#define ConvertArgToStr(arg, value, size, func_name)            \
    if (PyUnicode_CheckExact(arg)) {                            \
        (value) = (char*)PyUnicode_AsUTF8AndSize(arg, &(size)); \
    } else if (PyBytes_CheckExact(arg)) {                       \
        PyBytes_AsStringAndSize(arg, &(value), &(size));        \
    }                                                           \
    if (!(value) || !(size)) {                                  \
        _PyArg_BadArgument(func_name, #arg, "str", arg);        \
        return NULL;                                            \
    }

#define ConvertPosArgToUnsignedLong(pos, value, func_name)      \
    if (nargs > pos && args[pos] != Py_None) {                  \
        if (!PyLong_Check(args[pos])) {                         \
            _PyArg_BadArgument(func_name, "args[" #pos "]", "long", args[pos]); \
            return NULL;                                        \
        }                                                       \
        value = PyLong_AsUnsignedLong(args[pos]);               \
        if (value == (unsigned int)-1) {                       \
            _PyArg_BadArgument(func_name, "args[" #pos "]", "unsigned", args[pos]); \
            return NULL;                                        \
        }                                                       \
    }

#define ConvertPosArgToUnsignedLongP(pos, value, func_name)     \
    if (nargs > pos && args[pos] != Py_None) {                  \
        if (!PyLong_Check(args[pos])) {                         \
            _PyArg_BadArgument(func_name, "args[" #pos "]", "long", args[pos]); \
            return NULL;                                        \
        }                                                       \
        value = PyLong_AsUnsignedLong(args[pos]);               \
        if (value == (unsigned int)-1) {                       \
            _PyArg_BadArgument(func_name, "args[" #pos "]", "unsigned", args[pos]); \
            return NULL;                                        \
        }                                                       \
        p##value = &value;                                      \
    }

#define ConvertPosArgToUnsignedLongLong(pos, value, func_name)  \
    if (nargs > pos && args[pos] != Py_None) {                  \
        if (!PyLong_Check(args[pos])) {                         \
            _PyArg_BadArgument(func_name, "args[" #pos "]", "long", args[pos]); \
            return NULL;                                        \
        }                                                       \
        value = PyLong_AsUnsignedLongLong(args[pos]);           \
        if (value == (unsigned long long)-1) {                  \
            _PyArg_BadArgument(func_name, "args[" #pos "]", "unsigned", args[pos]); \
            return NULL;                                        \
        }                                                       \
    }

#define ConvertPosArgToLong(pos, value, func_name)              \
    if (nargs > pos && args[pos] != Py_None) {                  \
        if (!PyLong_Check(args[pos])) {                         \
            _PyArg_BadArgument(func_name, "args[" #pos "]", "long", args[pos]); \
            return NULL;                                        \
        }                                                       \
        value = PyLong_AsLong(args[pos]);                       \
    }

#define ConvertPosArgToLongLong(pos, value, func_name)          \
    if (nargs > pos && args[pos] != Py_None) {                  \
        if (!PyLong_Check(args[pos])) {                         \
            _PyArg_BadArgument(func_name, "args[" #pos "]", "long", args[pos]); \
            return NULL;                                        \
        }                                                       \
        value = PyLong_AsLongLong(args[pos]);                   \
    }

#define ConvertPosArgToLongP(pos, value, func_name)             \
    if (nargs > pos && args[pos] != Py_None) {                  \
        if (!PyLong_Check(args[pos])) {                         \
            _PyArg_BadArgument(func_name, "args[" #pos "]", "long", args[pos]); \
            return NULL;                                        \
        }                                                       \
        value = PyLong_AsLong(args[pos]);                       \
        p##value = &value;                                      \
    }

//_asctim(long long timaddr, char **timbuf, char cvtflg)
static PyObject*
SYS_asctim(
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("asctime", nargs, 1, 2)) {
        return NULL;
    }
    if (!PyLong_Check(args[0])) {
        _PyArg_BadArgument("asctime", "args[0]", "long", args[0]);
        return NULL;
    }
    long long vms_time = PyLong_AsLongLong(args[0]);
    int cvt_flag = 0;

    if (nargs > 1) {
        if (args[1] == Py_True) {
            cvt_flag = 1;
        } else if (args[1] == Py_False) {
            cvt_flag = 0;
        } else if (PyLong_Check(args[1])) {
            cvt_flag = PyLong_AsLong(args[1]);
        } else {
            _PyArg_BadArgument("asctime", "args[1]", "bool | long", args[1]);
            return NULL;
        }
    }

    char buffer[64];
    $DESCRIPTOR(val_dsc, buffer);

    int status = 0;
    unsigned short result_len = 0;

    Py_BEGIN_ALLOW_THREADS
    status = sys$asctim(&result_len, &val_dsc, (struct _generic_64 *) &vms_time, cvt_flag);
    Py_END_ALLOW_THREADS

    if (!$VMS_STATUS_SUCCESS(status)) {
        result_len = 0;
    }
    buffer[result_len] = 0;
    return Py_BuildValue("(i,s)", status, buffer);
}


// unsigned int _bintim(char *timbuf, long long *timadr)
static PyObject*
SYS_bintim(
    PyObject *self,
    PyObject *args)
{
    char *time_str = NULL;
    Py_ssize_t time_str_size = 0;
    ConvertArgToStr(args, time_str, time_str_size, "bintim");

    $DESCRIPTOR(tim_dsc, "");

    tim_dsc.dsc$w_length = time_str_size;
    set_dsc_string(tim_dsc, time_str);

    int status = 0;
    long long vms_time = 0;
    Py_BEGIN_ALLOW_THREADS
    status = sys$bintim(&tim_dsc, (struct _generic_64 *) &vms_time);
    Py_END_ALLOW_THREADS

    free_dsc_string(tim_dsc);

    return Py_BuildValue("(i,L)", status, vms_time);
}

// unsigned int _hiber()
static PyObject*
SYS_hiber(
    PyObject *self,
    PyObject *args)
{
    int status = 0;
    Py_BEGIN_ALLOW_THREADS
    status = sys$hiber();
    Py_END_ALLOW_THREADS
    return PyLong_FromLong(status);
}

// unsigned int _schdwk(unsigned int *pidadr, char *prcnam, long long daytim, long long reptim) {
static PyObject*
SYS_schdwk(
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("schdwk", nargs, 1, 3)) {
        return NULL;
    }
    if (!PyLong_Check(args[0])) {
        _PyArg_BadArgument("schdwk", "args[0]", "long", args[0]);
        return NULL;
    }
    struct _generic_64 vms_time;
    vms_time.gen64$q_quadword = PyLong_AsLongLong(args[0]);

    unsigned int pid = 0;
    char *prcnam = NULL;
    Py_ssize_t prcnam_size = 0;
    struct _generic_64 rep_time;
    rep_time.gen64$q_quadword = 0;

    if (nargs > 1) {
        if (PyLong_Check(args[1])) {
            pid = PyLong_AsUnsignedLong(args[1]);
        } else if (PyUnicode_CheckExact(args[1])) {
            prcnam = (char*)PyUnicode_AsUTF8AndSize(args[1], &prcnam_size);
        } else if (PyBytes_CheckExact(args[1])) {
            PyBytes_AsStringAndSize(args[1], &prcnam, &prcnam_size);
        } else if (args[1] != Py_None){
            _PyArg_BadArgument("schdwk", "args[1]", "str | bytes | long", args[1]);
            return NULL;
        }
    }
    if (nargs > 2) {
        if (PyLong_Check(args[2])) {
            rep_time.gen64$q_quadword = PyLong_AsLongLong(args[2]);
        } else {
            _PyArg_BadArgument("schdwk", "args[2]", "long", args[2]);
            return NULL;
        }
    }

    int status = 0;
    $DESCRIPTOR(prcnam_dsc, "");
    __void_ptr32 pprcnam_dsc = NULL;

    if (prcnam && prcnam_size) {
        prcnam_dsc.dsc$w_length = prcnam_size;
        set_dsc_string(prcnam_dsc, prcnam);
        pprcnam_dsc = &prcnam_dsc;
    }

    Py_BEGIN_ALLOW_THREADS
    status = sys$schdwk(&pid, pprcnam_dsc, &vms_time, &rep_time);
    Py_END_ALLOW_THREADS

    if (prcnam && prcnam_size) {
        free_dsc_string(prcnam_dsc);
    }

    return Py_BuildValue("(i,k)", status, pid);
}

// unsigned int _assign(char *devnam, unsigned short int *chan, unsigned int acmode, char *mbxnam, unsigned int flags)
static PyObject*
SYS_assign(
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("assign", nargs, 1, 4)) {
        return NULL;
    }
    char *devnam = NULL;
    Py_ssize_t devnam_size = 0;
    ConvertArgToStr(args[0], devnam, devnam_size, "assign");

    unsigned int acmode = PSL$C_USER;
    if (nargs > 1 && args[1] != Py_None) {
        if (PyLong_Check(args[1])) {
            acmode = PyLong_AsUnsignedLong(args[1]);
        } else {
            _PyArg_BadArgument("assign", "args[1]", "unsigned long", args[1]);
            return NULL;
        }
    }

    char *mbxnam = NULL;
    Py_ssize_t mbxnam_size = 0;
    if (nargs > 2 && args[2] != Py_None) {
        ConvertArgToStr(args[2], mbxnam, mbxnam_size, "assign");
    }

    unsigned int flags = 0;
    if (nargs > 3 && args[3] != Py_None) {
        if (PyLong_Check(args[3])) {
            flags = PyLong_AsUnsignedLong(args[3]);
        } else {
            _PyArg_BadArgument("assign", "args[3]", "unsigned long", args[3]);
            return NULL;
        }
    }

    $DESCRIPTOR(dev_dsc, "");
    $DESCRIPTOR(mbx_dsc, "");
    __void_ptr32 pmbx_dsc = NULL;
    int status = 0;
    unsigned short chan = 0;

    dev_dsc.dsc$w_length = devnam_size;
    set_dsc_string(dev_dsc, devnam);

    if (mbxnam != NULL && mbxnam_size) {
        mbx_dsc.dsc$w_length = mbxnam_size;
        set_dsc_string(mbx_dsc, mbxnam);
        pmbx_dsc = &mbx_dsc;
    }

    Py_BEGIN_ALLOW_THREADS
    status = sys$assign(&dev_dsc, &chan, acmode, pmbx_dsc, flags);
    Py_END_ALLOW_THREADS

    free_dsc_string(dev_dsc);
    if (mbxnam != NULL && mbxnam_size) {
        free_dsc_string(mbx_dsc);
    }

    return Py_BuildValue("(i,H)", status, chan);
}

// unsigned int _dassgn(unsigned short int chan)
static PyObject*
SYS_dassgn(
    PyObject *self,
    PyObject *args)
{
    if (!PyLong_Check(args)) {
        _PyArg_BadArgument("dassgn", "args", "unsigned long", args);
        return NULL;
    }
    unsigned short chan = PyLong_AsUnsignedLong(args);
    int status = 0;
    Py_BEGIN_ALLOW_THREADS
    status = sys$dassgn(chan);
    Py_END_ALLOW_THREADS
    return PyLong_FromLong(status);
}

// unsigned int _cancel(unsigned short int chan)
static PyObject*
SYS_cancel(
    PyObject *self,
    PyObject *args)
{
    if (!PyLong_Check(args)) {
        _PyArg_BadArgument("cancel", "args", "unsigned long", args);
        return NULL;
    }
    unsigned short chan = PyLong_AsUnsignedLong(args);
    int status = 0;
    Py_BEGIN_ALLOW_THREADS
    status = sys$cancel(chan);
    Py_END_ALLOW_THREADS
    return PyLong_FromLong(status);
}

// unsigned int _finish_rdb(unsigned int *context)
static PyObject*
SYS_finish_rdb(
    PyObject *self,
    PyObject *args)
{
    if (!PyLong_Check(args)) {
        _PyArg_BadArgument("finish_rdb", "args", "unsigned long", args);
        return NULL;
    }
    unsigned int context = PyLong_AsUnsignedLong(args);
    int status = 0;
    Py_BEGIN_ALLOW_THREADS
    status = sys$finish_rdb(&context);
    Py_END_ALLOW_THREADS

    return PyLong_FromLong(status);
}

// unsigned int _find_held(unsigned int holder, unsigned int *id, unsigned int *attrib, unsigned int *context)
static PyObject*
SYS_find_held(
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("find_held", nargs, 1, 2)) {
        return NULL;
    }
    if (!PyLong_Check(args[0])) {
        _PyArg_BadArgument("find_held", "args[0]", "unsigned long", args[0]);
        return NULL;
    }
    struct _generic_64 holder;
    holder.gen64$q_quadword = PyLong_AsUnsignedLongLong(args[0]);

    unsigned int context = 0;
    if (nargs > 1) {
        if (!PyLong_Check(args[1])) {
            _PyArg_BadArgument("find_held", "args[1]", "unsigned long", args[1]);
            return NULL;
        }
        context = PyLong_AsUnsignedLong(args[1]);
    }
    unsigned int id = 0;
    unsigned int attrib = 0;
    int status = 0;

    Py_BEGIN_ALLOW_THREADS
    status = sys$find_held(&holder, &id, &attrib, &context);
    Py_END_ALLOW_THREADS

    return Py_BuildValue("(i,I,I,I)", status, id, attrib, context);
}

// unsigned int _asctoid(char *name, unsigned int *id, unsigned int *attrib)
static PyObject*
SYS_asctoid(
    PyObject *self,
    PyObject *args)
{
    char *name = NULL;
    Py_ssize_t name_size = 0;
    ConvertArgToStr(args, name, name_size, "asctoid");

    $DESCRIPTOR(name_dsc, "");
    unsigned int id = 0;
    unsigned int attrib = 0;

    name_dsc.dsc$w_length = name_size;
    set_dsc_string(name_dsc, name);

    int status = 0;

    Py_BEGIN_ALLOW_THREADS
    status = sys$asctoid(&name_dsc, &id, &attrib);
    Py_END_ALLOW_THREADS

    free_dsc_string(name_dsc);

    return Py_BuildValue("(i,I,I)", status, id, attrib);
}

// unsigned int _getmsg(unsigned int msgid, char **msg, unsigned int flags, unsigned int *outadr)
static PyObject*
SYS_getmsg(
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("getmsg", nargs, 1, 2)) {
        return NULL;
    }
    if (!PyLong_Check(args[0])) {
        _PyArg_BadArgument("getmsg", "args[0]", "long", args[0]);
        return NULL;
    }
    unsigned int msgid = PyLong_AsUnsignedLong(args[0]);
    unsigned int flags = 0x0F;
    if (nargs > 1) {
        if (!PyLong_Check(args[1])) {
            _PyArg_BadArgument("getmsg", "args[1]", "long", args[1]);
            return NULL;
        }
        flags = PyLong_AsUnsignedLong(args[1]);
    }

    char buffer[257];
    $DESCRIPTOR(val_dsc, buffer);

    int status = 0;
    unsigned short result_len = 0;
    unsigned char out[4];

    Py_BEGIN_ALLOW_THREADS
    status = sys$getmsg(msgid, &result_len, &val_dsc, flags, out);
    Py_END_ALLOW_THREADS

    if (!$VMS_STATUS_SUCCESS(status)) {
        result_len = 0;
    }
    buffer[result_len] = 0;

    return Py_BuildValue("(i,s,(i,i))", status, buffer, out[1], out[2]);
}

// unsigned int _idtoasc(unsigned int id, char **nambuf, unsigned int *resid, unsigned int *attrib, unsigned int *ctxt)
static PyObject*
SYS_idtoasc(
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("idtoasc", nargs, 1, 2)) {
        return NULL;
    }
    if (!PyLong_Check(args[0])) {
        _PyArg_BadArgument("idtoasc", "args[0]", "long", args[0]);
        return NULL;
    }
    unsigned int id = PyLong_AsUnsignedLong(args[0]);
    unsigned int context = 0;
    if (nargs > 1) {
        if (!PyLong_Check(args[1])) {
            _PyArg_BadArgument("idtoasc", "args[1]", "long", args[1]);
            return NULL;
        }
        context = PyLong_AsUnsignedLong(args[1]);
    }

    char buffer[256];
    $DESCRIPTOR(val_dsc, buffer);
    int status = 0;
    unsigned short result_len = 0;
    unsigned int resid = 0;
    unsigned int attrib = 0;

    Py_BEGIN_ALLOW_THREADS
    status = sys$idtoasc(id, &result_len, &val_dsc, &resid, &attrib, &context);
    Py_END_ALLOW_THREADS

    if (!$VMS_STATUS_SUCCESS(status)) {
        result_len = 0;
    }
    buffer[result_len] = 0;

    return Py_BuildValue("(i,s,I,I,I)", status, buffer, resid, attrib, context);
}

// unsigned int _crembx(char prmflg, unsigned short int *chan, unsigned int maxmsg, unsigned int bufquo, unsigned int promsk, unsigned int acmode, char *lognam, unsigned int flags)
static PyObject*
SYS_crembx(
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("crembx", nargs, 0, 7)) {
        return NULL;
    }
    unsigned char prmflg = 0;
    if (nargs > 0 && args[0] != Py_None) {
        if (args[0] == Py_True) {
            prmflg = 1;
        } else if (args[0] == Py_False) {
            prmflg = 0;
        } else if (PyLong_CheckExact(args[0])) {
            prmflg = PyLong_AsUnsignedLong(args[0]);
        } else {
            _PyArg_BadArgument("crembx", "args[0]", "bool | long", args[0]);
            return NULL;
        }
    }
    unsigned int maxmsg = 0;
    ConvertPosArgToUnsignedLong(1, maxmsg, "crembx");
    unsigned int bufquo = 0;
    ConvertPosArgToUnsignedLong(2, bufquo, "crembx");
    unsigned int promsk = 0;
    ConvertPosArgToUnsignedLong(3, promsk, "crembx");
    unsigned int acmode = PSL$C_USER;
    ConvertPosArgToUnsignedLong(4, acmode, "crembx");
    char *logname = NULL;
    Py_ssize_t logname_size = 0;
    if (nargs > 5 && args[5] != Py_None) {
        ConvertArgToStr(args[5], logname, logname_size, "crembx");
    }
    unsigned int flags = 0;
    ConvertPosArgToUnsignedLong(6, flags, "crembx");

    $DESCRIPTOR(lnm_dsc, "");
    __void_ptr32 plnm_dsc = NULL;
    int status = 0;
    unsigned short chan = 0;

    if (logname != NULL && logname_size) {
        lnm_dsc.dsc$w_length = logname_size;
        set_dsc_string(lnm_dsc, logname);
        plnm_dsc = &lnm_dsc;
    }

    Py_BEGIN_ALLOW_THREADS
    status = sys$crembx(prmflg, &chan, maxmsg, bufquo, promsk, acmode, plnm_dsc, flags, 0);
    Py_END_ALLOW_THREADS

    if (logname != NULL && logname_size) {
        free_dsc_string(lnm_dsc);
    }

    return Py_BuildValue("(i,H)", status, chan);
}

// unsigned int _delmbx(unsigned short int chan)
static PyObject*
SYS_delmbx(
    PyObject *self,
    PyObject *args)
{
    if (!PyLong_Check(args)) {
        _PyArg_BadArgument("delmbx", "args", "unsigned long", args);
        return NULL;
    }
    unsigned short chan = PyLong_AsUnsignedLong(args);
    int status = 0;
    Py_BEGIN_ALLOW_THREADS
    status = sys$delmbx(chan);
    Py_END_ALLOW_THREADS
    return PyLong_FromLong(status);
}

// unsigned int _getrmi(void *addr)
static PyObject*
SYS_getrmi(
    PyObject *self,
    PyObject *args)
{
    if (strcmp(Py_TYPE(args)->tp_name, ILE3_MODULE_NAME "." ILE3_TYPE_NAME) != 0) {
        _PyArg_BadArgument("getrmi", "args", ILE3_MODULE_NAME "." ILE3_TYPE_NAME, args);
        return NULL;
    }
    ILE3Object *pILE3 = (ILE3Object *)args;
    int status = 0;
    IOSB iosb;
    unsigned int efn = 0;
    Py_BEGIN_ALLOW_THREADS
    status = lib$get_ef(&efn);
    Py_END_ALLOW_THREADS
    if ($VMS_STATUS_SUCCESS(status)) {
        Py_BEGIN_ALLOW_THREADS
        status = sys$getrmi(efn, 0, 0, pILE3->plist, &iosb, 0, 0);
        Py_END_ALLOW_THREADS
        if ($VMS_STATUS_SUCCESS(status)) {
            Py_BEGIN_ALLOW_THREADS
            status = sys$synch(efn, &iosb);
            Py_END_ALLOW_THREADS
            if (!$VMS_STATUS_SUCCESS(status)) {
                // PyErr_SetString(PyExc_SystemError, "sys$synch failed");
            } else if (!$VMS_STATUS_SUCCESS(iosb.iosb$l_getxxi_status)) {
                // PyErr_SetString(PyExc_SystemError, "sys$synch failed (inner status)");
                status = iosb.iosb$l_getxxi_status;
            }
        } else {
            // PyErr_SetString(PyExc_SystemError, "sys$getrmi failed");
        }
        lib$free_ef(&efn);
    } else {
        // PyErr_SetString(PyExc_SystemError, "Cannot allocate EFN");
    }
    return PyLong_FromLong(status);
}

// unsigned int _getuai(char *user, void *addr)
static PyObject*
SYS_getuai(
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("getuai", nargs, 2, 2)) {
        return NULL;
    }
    char *user = NULL;
    Py_ssize_t user_size = 0;
    ConvertArgToStr(args[0], user, user_size, "getuai");

    if (strcmp(Py_TYPE(args[1])->tp_name, ILE3_MODULE_NAME "." ILE3_TYPE_NAME) != 0) {
        _PyArg_BadArgument("getuai", "args", ILE3_MODULE_NAME "." ILE3_TYPE_NAME, args[1]);
        return NULL;
    }
    ILE3Object *pILE3 = (ILE3Object *)args[1];
    int status = 0;
    $DESCRIPTOR(user_dsc, "");

    user_dsc.dsc$w_length = user_size;
    set_dsc_string(user_dsc, user);

    Py_BEGIN_ALLOW_THREADS
    status = sys$getuai(0, NULL, &user_dsc, pILE3->plist, NULL, NULL, 0);
    Py_END_ALLOW_THREADS

    free_dsc_string(user_dsc);

    return PyLong_FromLong(status);
}

// unsigned int _setuai(char *user, void *addr)
static PyObject*
SYS_setuai(
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("setuai", nargs, 2, 2)) {
        return NULL;
    }
    char *user = NULL;
    Py_ssize_t user_size = 0;
    ConvertArgToStr(args[0], user, user_size, "setuai");

    if (strcmp(Py_TYPE(args[1])->tp_name, ILE3_MODULE_NAME "." ILE3_TYPE_NAME) != 0) {
        _PyArg_BadArgument("setuai", "args[1]", ILE3_MODULE_NAME "." ILE3_TYPE_NAME, args[1]);
        return NULL;
    }
    ILE3Object *pILE3 = (ILE3Object *)args[1];
    int status = 0;
    $DESCRIPTOR(user_dsc, "");

    user_dsc.dsc$w_length = user_size;
    set_dsc_string(user_dsc, user);

    Py_BEGIN_ALLOW_THREADS
    status = sys$setuai(0, NULL, &user_dsc, pILE3->plist, NULL, NULL, 0);
    Py_END_ALLOW_THREADS

    free_dsc_string(user_dsc);

    return PyLong_FromLong(status);
}


// unsigned int _getqui(unsigned short func, int *context, void *addr)
static PyObject*
SYS_getqui(
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("getqui", nargs, 2, 3)) {
        return NULL;
    }
    if (!PyLong_Check(args[0])) {
        _PyArg_BadArgument("getqui", "args[0]", "unsigned long", args[0]);
        return NULL;
    }
    unsigned short func = PyLong_AsUnsignedLong(args[0]);

    int context = 0;
    ConvertPosArgToLong(1, context, "getqui");

    ILE3Object *pILE3 = NULL;
    if (nargs > 2 && args[2] != Py_None) {
        if (strcmp(Py_TYPE(args[2])->tp_name, ILE3_MODULE_NAME "." ILE3_TYPE_NAME) != 0) {
            _PyArg_BadArgument("getqui", "args[2]", ILE3_MODULE_NAME "." ILE3_TYPE_NAME, args[2]);
            return NULL;
        }
        pILE3 = (ILE3Object *)args[2];
    }

    int status = 0;
    IOSB iosb;

    Py_BEGIN_ALLOW_THREADS
    status = sys$getquiw(EFN$C_ENF, func, (__void_ptr32)&context, pILE3 ? pILE3->plist : (__void_ptr32)NULL, &iosb, NULL, 0);
    Py_END_ALLOW_THREADS

    if ($VMS_STATUS_SUCCESS(status)) {
        if (!$VMS_STATUS_SUCCESS(iosb.iosb$l_getxxi_status)) {
            status = iosb.iosb$l_getxxi_status;
        }
    }

    return Py_BuildValue("(i,i)", status, context);
}

// unsigned int _getsyi(int *csidadr, char *node, void *addr)
static PyObject*
SYS_getsyi(
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("getsyi", nargs, 1, 2)) {
        return NULL;
    }

    if (strcmp(Py_TYPE(args[0])->tp_name, ILE3_MODULE_NAME "." ILE3_TYPE_NAME) != 0) {
        _PyArg_BadArgument("getsyi", "args[0]", ILE3_MODULE_NAME "." ILE3_TYPE_NAME, args[0]);
        return NULL;
    }
    ILE3Object *pILE3 = (ILE3Object *)args[0];

    int csi = 0, *pcsi = NULL;
    char *node = NULL;
    Py_ssize_t node_size = 0;
    if (nargs > 1 && args[1] != Py_None) {
        if (PyLong_Check(args[1])) {
            csi = PyLong_AsLong(args[1]);
            pcsi = &csi;
        } else if (PyUnicode_CheckExact(args[1])) {
            node = (char*)PyUnicode_AsUTF8AndSize(args[1], &node_size);
        } else if (PyBytes_CheckExact(args[1])) {
            PyBytes_AsStringAndSize(args[1], &node, &node_size);
        } else {
            _PyArg_BadArgument("getsyi", "args[1]", "str | bytes | long", args[1]);
            return NULL;
        }
    }

    int status = 0;
    $DESCRIPTOR(node_dsc, "");
    __void_ptr32 pnode_dsc = NULL;
    IOSB iosb;

    if (node && node_size) {
        node_dsc.dsc$w_length = node_size;
        set_dsc_string(node_dsc, node);
        pnode_dsc = &node_dsc;
    }

    Py_BEGIN_ALLOW_THREADS
    status = sys$getsyiw(EFN$C_ENF, (__void_ptr32)pcsi, pnode_dsc, pILE3->plist, &iosb, NULL, 0);
    Py_END_ALLOW_THREADS

    if (node && node_size) {
        free_dsc_string(node_dsc);
    }

    if ($VMS_STATUS_SUCCESS(status)) {
        if (!$VMS_STATUS_SUCCESS(iosb.iosb$l_getxxi_status)) {
            status = iosb.iosb$l_getxxi_status;
        }
    }

    return Py_BuildValue("(i,i)", status, csi);
}

// unsigned int _getjpi(int *pidadr, char *prcnam, void *addr)
static PyObject*
SYS_getjpi(
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("getjpi", nargs, 1, 2)) {
        return NULL;
    }
    if (strcmp(Py_TYPE(args[0])->tp_name, ILE3_MODULE_NAME "." ILE3_TYPE_NAME) != 0) {
        _PyArg_BadArgument("getjpi", "args[0]", ILE3_MODULE_NAME "." ILE3_TYPE_NAME, args[0]);
        return NULL;
    }
    ILE3Object *pILE3 = (ILE3Object *)args[0];

    int pid = 0, *ppid = &pid;
    char *prcnam = NULL;
    Py_ssize_t prcnam_size = 0;
    if (nargs > 1 && args[1] != Py_None) {
        if (PyLong_Check(args[1])) {
            pid = PyLong_AsLong(args[1]);
        } else if (PyUnicode_CheckExact(args[1])) {
            prcnam = (char*)PyUnicode_AsUTF8AndSize(args[1], &prcnam_size);
        } else if (PyBytes_CheckExact(args[1])) {
            PyBytes_AsStringAndSize(args[1], &prcnam, &prcnam_size);
        } else {
            _PyArg_BadArgument("getjpi", "args[1]", "str | bytes | long", args[1]);
            return NULL;
        }
    }

    int status = 0;

    $DESCRIPTOR(prcnam_dsc, "");
    __void_ptr32 pprcnam_dsc = NULL;
    IOSB iosb;

    if (prcnam && prcnam_size) {
        prcnam_dsc.dsc$w_length = prcnam_size;
        set_dsc_string(prcnam_dsc, prcnam);
        pprcnam_dsc = &prcnam_dsc;
    }

    Py_BEGIN_ALLOW_THREADS
    status = sys$getjpiw(EFN$C_ENF, (unsigned int*)ppid, pprcnam_dsc, pILE3->plist, &iosb, NULL, 0);
    Py_END_ALLOW_THREADS

    if (prcnam && prcnam_size) {
        free_dsc_string(prcnam_dsc);
    }

    if ($VMS_STATUS_SUCCESS(status)) {
        if (!$VMS_STATUS_SUCCESS(iosb.iosb$l_getxxi_status)) {
            status = iosb.iosb$l_getxxi_status;
        }
    }

    return Py_BuildValue("(i,i)", status, pid);
}


// unsigned int _getdvi(char *dev, void *addr)
static PyObject*
SYS_getdvi(
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("getdvi", nargs, 2, 2)) {
        return NULL;
    }

    long chan = 0;
    struct dsc$descriptor_s *p_dev_dsc = NULL;
    $DESCRIPTOR(dev_dsc, "");

    if (PyLong_Check(args[0])) {
        chan = PyLong_AsUnsignedLong(args[0]);
    } else {
        char *dev = NULL;
        Py_ssize_t dev_size = 0;
        ConvertArgToStr(args[0], dev, dev_size, "getdvi");
        dev_dsc.dsc$w_length = dev_size;
        set_dsc_string(dev_dsc, dev);
        p_dev_dsc = &dev_dsc;
    }

    if (strcmp(Py_TYPE(args[1])->tp_name, ILE3_MODULE_NAME "." ILE3_TYPE_NAME) != 0) {
        _PyArg_BadArgument("getdvi", "args[1]", ILE3_MODULE_NAME "." ILE3_TYPE_NAME, args[1]);
        return NULL;
    }
    ILE3Object *pILE3 = (ILE3Object *)args[1];

    IOSB iosb;
    int status = 0;

    Py_BEGIN_ALLOW_THREADS
    status = sys$getdviw(EFN$C_ENF, chan, p_dev_dsc, pILE3->plist, &iosb, NULL, 0, NULL);
    Py_END_ALLOW_THREADS

    if (p_dev_dsc) {
        free_dsc_string(dev_dsc);
    }

    if ($VMS_STATUS_SUCCESS(status)) {
        if (!$VMS_STATUS_SUCCESS(iosb.iosb$l_getxxi_status)) {
            status = iosb.iosb$l_getxxi_status;
        }
    }
    return PyLong_FromLong(status);
}

// unsigned int _device_scan(char **name, char *patt, void *addr, unsigned long long *ctxt)
static PyObject*
SYS_device_scan(
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("device_scan", nargs, 0, 3)) {
        return NULL;
    }
    char *patt = NULL;
    Py_ssize_t patt_size = 0;
    if (nargs > 0 && args[0] != Py_None) {
        ConvertArgToStr(args[0], patt, patt_size, "device_scan");
    }
    ILE3Object *pILE3 = NULL;
    if (nargs > 1 && args[1] != Py_None) {
        if (strcmp(Py_TYPE(args[1])->tp_name, ILE3_MODULE_NAME "." ILE3_TYPE_NAME) != 0) {
            _PyArg_BadArgument("device_scan", "args[1]", ILE3_MODULE_NAME "." ILE3_TYPE_NAME, args[1]);
            return NULL;
        }
        pILE3 = (ILE3Object *)args[1];
    }
    struct _generic_64 ctxt;
    ctxt.gen64$q_quadword = 0;
    ConvertPosArgToLongLong(2, ctxt.gen64$q_quadword, "device_scan");

    char buffer[128];
    $DESCRIPTOR(ret_dsc, buffer);
    $DESCRIPTOR(dev_dsc, "");
    __void_ptr32 pdev_dsc = NULL;
    unsigned short result_len = 0;
    int status = 0;

    if (patt && patt_size) {
        dev_dsc.dsc$w_length = patt_size;
        set_dsc_string(dev_dsc, patt);
        pdev_dsc = &dev_dsc;
    }

    Py_BEGIN_ALLOW_THREADS
    status = sys$device_scan(&ret_dsc, &result_len, pdev_dsc, pILE3 ? pILE3->plist : (__void_ptr32)NULL, &ctxt);
    Py_END_ALLOW_THREADS

    if (patt && patt_size) {
        free_dsc_string(dev_dsc);
    }

    if (!$VMS_STATUS_SUCCESS(status)) {
        result_len = 0;
    }
    buffer[result_len] = 0;

    return Py_BuildValue("(i,s,L)", status, buffer, ctxt.gen64$q_quadword);
}

// unsigned int _dellnm(char *tabnam, char *lognam, unsigned char acmode) {
static PyObject*
SYS_dellnm(
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("dellnm", nargs, 0, 3)) {
        return NULL;
    }

    char *lognam = NULL;
    Py_ssize_t lognam_size = 0;
    if (nargs > 0 && args[0] != Py_None) {
        ConvertArgToStr(args[0], lognam, lognam_size, "dellnm");
    }

    char *tabnam = DEF_TABNAM;
    Py_ssize_t tabnam_size = sizeof(DEF_TABNAM) - 1;
    if (nargs > 1 && args[1] != Py_None) {
        ConvertArgToStr(args[1], tabnam, tabnam_size, "dellnm");
    }

    unsigned char acmode = PSL$C_USER, *pacmode = NULL;
    if (nargs > 2 && args[2] != Py_None) {
        if (!PyLong_Check(args[2])) {
            _PyArg_BadArgument("dellnm", "args[2]", "long", args[2]);
            return NULL;
        }
        acmode = PyLong_AsUnsignedLong(args[2]);
        pacmode = &acmode;
    }

    $DESCRIPTOR(tabnam_dsc, "");
    $DESCRIPTOR(lognam_dsc, "");
    __void_ptr32 plognam_dsc = NULL;

    tabnam_dsc.dsc$w_length = tabnam_size;
    set_dsc_string(tabnam_dsc, tabnam);

    if (lognam && lognam_size) {
        lognam_dsc.dsc$w_length = lognam_size;
        set_dsc_string(lognam_dsc, lognam);
        plognam_dsc = &lognam_dsc;
    }

    int status = 0;

    Py_BEGIN_ALLOW_THREADS
    status = sys$dellnm(&tabnam_dsc, plognam_dsc, pacmode);
    Py_END_ALLOW_THREADS

    free_dsc_string(tabnam_dsc);
    if (lognam && lognam_size) {
        free_dsc_string(lognam_dsc);
    }

    return PyLong_FromLong(status);
}


// unsigned int _trnlnm(unsigned int attr, char *tabnam, char *lognam, unsigned char acmode, void *addr)
static PyObject*
SYS_trnlnm(
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("trnlnm", nargs, 1, 5)) {
        return NULL;
    }

    char *lognam = NULL;
    Py_ssize_t lognam_size = 0;
    ConvertArgToStr(args[0], lognam, lognam_size, "trnlnm");

    char *tabnam = DEF_TABNAM;
    Py_ssize_t tabnam_size = sizeof(DEF_TABNAM) - 1;
    if (nargs > 1 && args[1] != Py_None) {
        ConvertArgToStr(args[1], tabnam, tabnam_size, "trnlnm");
    }

    ILE3Object *pILE3 = NULL;
    if (nargs > 2 && args[2] != Py_None) {
        if (strcmp(Py_TYPE(args[2])->tp_name, ILE3_MODULE_NAME "." ILE3_TYPE_NAME) != 0) {
            _PyArg_BadArgument("trnlnm", "args[2]", ILE3_MODULE_NAME "." ILE3_TYPE_NAME, args[2]);
            return NULL;
        }
        pILE3 = (ILE3Object *)args[2];
    }

    unsigned char acmode = PSL$C_USER, *pacmode = NULL;
    if (nargs > 3 && args[3] != Py_None) {
        if (!PyLong_Check(args[3])) {
            _PyArg_BadArgument("trnlnm", "args[3]", "long", args[3]);
            return NULL;
        }
        acmode = PyLong_AsUnsignedLong(args[3]);
        pacmode = &acmode;
    }

    unsigned int attr = 0, *pattr = NULL;
    if (nargs > 4 && args[4] != Py_None) {
        if (!PyLong_Check(args[4])) {
            _PyArg_BadArgument("trnlnm", "args[4]", "long", args[4]);
            return NULL;
        }
        attr = PyLong_AsUnsignedLong(args[4]);
        pattr = &attr;
    }

    $DESCRIPTOR(tabnam_dsc, "");
    $DESCRIPTOR(lognam_dsc, "");
    __void_ptr32 plognam_dsc = NULL;

    tabnam_dsc.dsc$w_length = tabnam_size;
    set_dsc_string(tabnam_dsc, tabnam);

    if (lognam && lognam_size) {
        lognam_dsc.dsc$w_length = lognam_size;
        set_dsc_string(lognam_dsc, lognam);
        plognam_dsc = &lognam_dsc;
    }

    int status = 0;

    Py_BEGIN_ALLOW_THREADS
    status = sys$trnlnm(pattr, &tabnam_dsc, plognam_dsc, pacmode, pILE3 ? pILE3->plist : NULL);
    Py_END_ALLOW_THREADS

    free_dsc_string(tabnam_dsc);
    if (lognam && lognam_size) {
        free_dsc_string(lognam_dsc);
    }

    return PyLong_FromLong(status);
}

// unsigned int _crelnm(unsigned int attr, char *tabnam, char *lognam, unsigned char acmode, void *addr)
static PyObject*
SYS_crelnm(
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("crelnm", nargs, 1, 5)) {
        return NULL;
    }

    char *lognam = NULL;
    Py_ssize_t lognam_size = 0;
    ConvertArgToStr(args[0], lognam, lognam_size, "crelnm");

    char *tabnam = DEF_TABNAM;
    Py_ssize_t tabnam_size = sizeof(DEF_TABNAM) - 1;
    if (nargs > 1 && args[1] != Py_None) {
        ConvertArgToStr(args[1], tabnam, tabnam_size, "crelnm");
    }

    ILE3Object *pILE3 = NULL;
    if (nargs > 2 && args[2] != Py_None) {
        if (strcmp(Py_TYPE(args[2])->tp_name, ILE3_MODULE_NAME "." ILE3_TYPE_NAME) != 0) {
            _PyArg_BadArgument("crelnm", "args[2]", ILE3_MODULE_NAME "." ILE3_TYPE_NAME, args[2]);
            return NULL;
        }
        pILE3 = (ILE3Object *)args[2];
    }

    unsigned char acmode = PSL$C_USER, *pacmode = NULL;
    if (nargs > 3 && args[3] != Py_None) {
        if (!PyLong_Check(args[3])) {
            _PyArg_BadArgument("crelnm", "args[3]", "long", args[3]);
            return NULL;
        }
        acmode = PyLong_AsUnsignedLong(args[3]);
        pacmode = &acmode;
    }

    unsigned int attr = 0, *pattr = NULL;
    if (nargs > 4 && args[4] != Py_None) {
        if (!PyLong_Check(args[4])) {
            _PyArg_BadArgument("crelnm", "args[4]", "long", args[4]);
            return NULL;
        }
        attr = PyLong_AsUnsignedLong(args[4]);
        pattr = &attr;
    }

    $DESCRIPTOR(tabnam_dsc, "");
    $DESCRIPTOR(lognam_dsc, "");
    __void_ptr32 plognam_dsc = NULL;

    tabnam_dsc.dsc$w_length = tabnam_size;
    set_dsc_string(tabnam_dsc, tabnam);

    if (lognam && lognam_size) {
        lognam_dsc.dsc$w_length = lognam_size;
        set_dsc_string(lognam_dsc, lognam);
        plognam_dsc = &lognam_dsc;
    }

    int status = 0;

    Py_BEGIN_ALLOW_THREADS
    status = sys$crelnm(pattr, &tabnam_dsc, plognam_dsc, pacmode, pILE3 ? pILE3->plist : NULL);
    Py_END_ALLOW_THREADS

    free_dsc_string(tabnam_dsc);
    if (lognam && lognam_size) {
        free_dsc_string(lognam_dsc);
    }

    return PyLong_FromLong(status);
}

// unsigned int _forcex(int pid, char *prcnam, unsigned int code)
static PyObject*
SYS_forcex(
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("forcex", nargs, 0, 2)) {
        return NULL;
    }

    unsigned int code = 0;
    ConvertPosArgToUnsignedLong(0, code, "");

    unsigned int pid = 0;
    __void_ptr32 ppid = NULL;
    char *prcnam = NULL;
    Py_ssize_t prcnam_size = 0;
    if (nargs > 1 && args[1] != Py_None) {
        if (PyLong_Check(args[1])) {
            pid = PyLong_AsUnsignedLong(args[1]);
            ppid = &pid;
        } else if (PyUnicode_CheckExact(args[1])) {
            prcnam = (char*)PyUnicode_AsUTF8AndSize(args[1], &prcnam_size);
        } else if (PyBytes_CheckExact(args[1])) {
            PyBytes_AsStringAndSize(args[1], &prcnam, &prcnam_size);
        } else {
            _PyArg_BadArgument("forcex", "args[1]", "str | bytes | long", args[1]);
            return NULL;
        }
    }

    $DESCRIPTOR(prcnam_dsc, "");
    __void_ptr32 pprcnam_dsc = NULL;
    if (prcnam && prcnam_size) {
        prcnam_dsc.dsc$w_length = prcnam_size;
        set_dsc_string(prcnam_dsc, prcnam);
        pprcnam_dsc = &prcnam_dsc;
    }

    int status = 0;
    Py_BEGIN_ALLOW_THREADS
    status = sys$forcex(ppid, pprcnam_dsc, code);
    Py_END_ALLOW_THREADS

    if (prcnam && prcnam_size) {
        free_dsc_string(prcnam_dsc);
    }

    return PyLong_FromLong(status);
}

static PyObject*
SYS_delprc(
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("delprc", nargs, 0, 1)) {
        return NULL;
    }

    unsigned int pid = 0;
    __void_ptr32 ppid = NULL;
    char *prcnam = NULL;
    Py_ssize_t prcnam_size = 0;
    if (nargs > 0 && args[0] != Py_None) {
        if (PyLong_Check(args[0])) {
            pid = PyLong_AsUnsignedLong(args[0]);
            ppid = &pid;
        } else if (PyUnicode_CheckExact(args[0])) {
            prcnam = (char*)PyUnicode_AsUTF8AndSize(args[0], &prcnam_size);
        } else if (PyBytes_CheckExact(args[0])) {
            PyBytes_AsStringAndSize(args[0], &prcnam, &prcnam_size);
        } else {
            _PyArg_BadArgument("delprc", "args[0]", "str | bytes | long", args[0]);
            return NULL;
        }
    }

    $DESCRIPTOR(prcnam_dsc, "");
    __void_ptr32 pprcnam_dsc = NULL;
    if (prcnam && prcnam_size) {
        prcnam_dsc.dsc$w_length = prcnam_size;
        set_dsc_string(prcnam_dsc, prcnam);
        pprcnam_dsc = &prcnam_dsc;
    }

    int status = 0;
    Py_BEGIN_ALLOW_THREADS
    status = sys$delprc(ppid, pprcnam_dsc, DELPRC$M_NOEXIT);
    Py_END_ALLOW_THREADS

    if (prcnam && prcnam_size) {
        free_dsc_string(prcnam_dsc);
    }

    return PyLong_FromLong(status);
}

// unsigned int _rem_ident(unsigned int id)
static PyObject*
SYS_rem_ident(
    PyObject *self,
    PyObject *args)
{
    unsigned int id = 0;
    if (!PyLong_Check(args)) {
        _PyArg_BadArgument("rem_ident", "args", "long", args);
        return NULL;
    }
    id = PyLong_AsUnsignedLong(args);
    int status = 0;
    Py_BEGIN_ALLOW_THREADS
    status = sys$rem_ident(id);
    Py_END_ALLOW_THREADS
    return PyLong_FromLong(status);
}

// unsigned int _add_ident(char *name, unsigned int id, unsigned int attrib, unsigned int *resid)
static PyObject*
SYS_add_ident(
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("add_ident", nargs, 1, 3)) {
        return NULL;
    }

    char *name = NULL;
    Py_ssize_t name_size = 0;
    ConvertArgToStr(args[0], name, name_size, "add_ident");

    unsigned int id = 0;
    ConvertPosArgToUnsignedLong(1, id, "add_ident");

    unsigned int attrib = 0;
    ConvertPosArgToUnsignedLong(2, attrib, "add_ident");

    unsigned int resid = 0;
    int status = 0;

    $DESCRIPTOR(name_dsc, "");

    name_dsc.dsc$w_length = name_size;
    set_dsc_string(name_dsc, name);

    Py_BEGIN_ALLOW_THREADS
    status = sys$add_ident(&name_dsc, id, attrib, &resid);
    Py_END_ALLOW_THREADS

    free_dsc_string(name_dsc);

    return Py_BuildValue("(i,I)", status, resid);
}


// unsigned int _rem_holder(unsigned int id, unsigned long long holder)
static PyObject*
SYS_rem_holder(
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("rem_holder", nargs, 2, 2)) {
        return NULL;
    }
    unsigned int id = 0;
    ConvertPosArgToUnsignedLong(0, id, "rem_holder");

    struct _generic_64 holder;
    holder.gen64$q_quadword = 0;
    ConvertPosArgToUnsignedLongLong(1, holder.gen64$q_quadword, "rem_holder");

    int status = 0;
    Py_BEGIN_ALLOW_THREADS
    status = sys$rem_holder(id, &holder);
    Py_END_ALLOW_THREADS

    return PyLong_FromLong(status);
}


// unsigned int _add_holder(unsigned int id, unsigned long long holder, unsigned int attrib)
static PyObject*
SYS_add_holder(
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("add_holder", nargs, 2, 3)) {
        return NULL;
    }
    unsigned int id = 0;
    ConvertPosArgToUnsignedLong(0, id, "add_holder");

    struct _generic_64 holder;
    holder.gen64$q_quadword = 0;
    ConvertPosArgToUnsignedLongLong(1, holder.gen64$q_quadword, "add_holder");

    unsigned int attrib = 0;
    ConvertPosArgToUnsignedLong(2, attrib, "add_holder");

    int status = 0;
    Py_BEGIN_ALLOW_THREADS
    status = sys$add_holder(id, &holder, attrib);
    Py_END_ALLOW_THREADS

    return PyLong_FromLong(status);
}

// unsigned int _getlki(unsigned int *lkidadr, void *addr)
static PyObject*
SYS_getlki(
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("getlki", nargs, 1, 2)) {
        return NULL;
    }
    if (strcmp(Py_TYPE(args[0])->tp_name, ILE3_MODULE_NAME "." ILE3_TYPE_NAME) != 0) {
        _PyArg_BadArgument("getlki", "args[0]", ILE3_MODULE_NAME "." ILE3_TYPE_NAME, args[0]);
        return NULL;
    }
    ILE3Object *pILE3 = (ILE3Object *)args[0];
    unsigned int lkid = 0;
    ConvertPosArgToUnsignedLong(1, lkid, "getlki");
    IOSB iosb;
    int status = 0;

    Py_BEGIN_ALLOW_THREADS
    status = sys$getlkiw(EFN$C_ENF, &lkid, pILE3->plist, &iosb, NULL, 0, 0);
    Py_END_ALLOW_THREADS

    if ($VMS_STATUS_SUCCESS(status)) {
        if (!$VMS_STATUS_SUCCESS(iosb.iosb$l_getxxi_status)) {
            status = iosb.iosb$l_getxxi_status;
        }
    }

    return Py_BuildValue("(i,I)", status, lkid);
}

// unsigned int _uicstr(long int val, char **res, int flag)
static PyObject*
SYS_uicstr(
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("uicstr", nargs, 1, 2)) {
        return NULL;
    }

    int val = 0;
    ConvertPosArgToLong(0, val, "uicstr");

    int flag = 0;
    if (nargs > 1) {
        if (args[1] == Py_True) {
            flag = 1;
        } else if (args[1] == Py_False) {
            flag = 0;
        } else if (PyLong_Check(args[1])) {
            flag = PyLong_AsLong(args[1]);
        } else {
            _PyArg_BadArgument("uicstr", "args[1]", "bool | long", args[1]);
            return NULL;
        }
    }

    char buffer[32], fmt[16];
    $DESCRIPTOR(str_dsc, buffer);
    $DESCRIPTOR(fmt_dsc, fmt);
    unsigned short result_len = 0;
    int status = 0;

    if (flag) {
        strcpy(fmt, "!%U");
    } else {
        strcpy(fmt, "!%I");
    }

    Py_BEGIN_ALLOW_THREADS
    status = sys$fao(&fmt_dsc, &result_len, &str_dsc, val);
    Py_END_ALLOW_THREADS
    if (!$VMS_STATUS_SUCCESS(status)) {
        result_len = 0;
    }
    buffer[result_len] = 0;
    return Py_BuildValue("(i,s)", status, buffer);
}

// unsigned int _gettim(long long *timadr)
static PyObject*
SYS_gettim(
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("gettim", nargs, 0, 1)) {
        return NULL;
    }
    int flag = 0;
    if (nargs > 0) {
        if (args[0] == Py_True) {
            flag = 1;
        } else if (args[0] == Py_False) {
            flag = 0;
        } else if (PyLong_Check(args[0])) {
            flag = PyLong_AsLong(args[0]);
        } else {
            _PyArg_BadArgument("gettim", "args[0]", "bool | long", args[0]);
            return NULL;
        }
    }
    unsigned long long time = 0;
    int status = 0;
    Py_BEGIN_ALLOW_THREADS
    status = sys$gettim((struct _generic_64 *) &time, flag);
    Py_END_ALLOW_THREADS

    return Py_BuildValue("(i,K)", status, time);
}


// unsigned int _show_intrusion(char *criteria, char **intruder,
// 			     unsigned long long *expires,
// 			     unsigned int *type, unsigned int *count,
// 			     unsigned int flags, unsigned int *context)
// {
static PyObject*
SYS_show_intrusion(
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("show_intrusion", nargs, 1, 3)) {
        return NULL;
    }
    ILE3Object *pILE3 = NULL;
    char *match = NULL;
    Py_ssize_t match_size = 0;
    if (strcmp(Py_TYPE(args[0])->tp_name, ILE3_MODULE_NAME "." ILE3_TYPE_NAME) == 0) {
        pILE3 = (ILE3Object *)args[0];
    } else if (PyUnicode_CheckExact(args[0])) {
        match = (char*)PyUnicode_AsUTF8AndSize(args[0], &match_size);
    } else if (PyBytes_CheckExact(args[0])) {
        PyBytes_AsStringAndSize(args[0], &match, &match_size);
    } else {
        _PyArg_BadArgument("show_intrusion", "args[0]", "ile3list | str | bytes", args[0]);
    }

    unsigned int flags = 0;
    ConvertPosArgToUnsignedLong(1, flags, "show_intrusion");

    unsigned int context = 0;
    ConvertPosArgToUnsignedLong(2, context, "show_intrusion");

    __void_ptr32 pcriteria = NULL;
    $DESCRIPTOR(criteria_dsc, "");
    if (match && match_size) {
        flags &= ~CIA$M_ITEMLIST;
        criteria_dsc.dsc$w_length = match_size;
        set_dsc_string(criteria_dsc, match);
        pcriteria = &criteria_dsc;
    } else if (pILE3) {
        flags |= CIA$M_ITEMLIST;
        pcriteria = pILE3->plist;
    }

    char buffer[1059];
    $DESCRIPTOR(intruder_dsc, buffer);
    
    unsigned short result_len = 0;
    struct {
        unsigned int type;
        unsigned int count;
        unsigned long long expires;
    } blk = {0};

    int status = 0;
    Py_BEGIN_ALLOW_THREADS
    status = sys$show_intrusion(pcriteria, &intruder_dsc, &result_len, &blk, flags, &context);
    Py_END_ALLOW_THREADS

    if (match && match_size) {
        free_dsc_string(criteria_dsc);
    }

    if (!$VMS_STATUS_SUCCESS(status)) {
        result_len = 0;
    }
    buffer[result_len] = 0;

    return Py_BuildValue("(i,s,I,I,I,K)", status, buffer, context, blk.type, blk.count, blk.expires);
}

static PyObject*
SYS_setprv(
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("setprv", nargs, 1, 3)) {
        return NULL;
    }
    struct _generic_64 prv;
    if (!PyLong_Check(args[0])) {
        _PyArg_BadArgument("setprv", "args[0]", "long", args[0]);
        return NULL;
    }
    prv.gen64$q_quadword = PyLong_AsUnsignedLongLong(args[0]);
    unsigned char enbflg = 1;
    if (nargs > 1) {
        if (args[1] == Py_True) {
            enbflg = 1;
        } else if (args[1] == Py_False) {
            enbflg = 0;
        } else if (PyLong_Check(args[1])) {
            enbflg = PyLong_AsUnsignedLong(args[1]);
        } else {
            _PyArg_BadArgument("setprv", "args[1]", "bool | long", args[1]);
            return NULL;
        }
    }
    unsigned char prmflg = 0;
    if (nargs > 2) {
        if (args[2] == Py_True) {
            prmflg = 1;
        } else if (args[2] == Py_False) {
            prmflg = 0;
        } else if (PyLong_Check(args[2])) {
            prmflg = PyLong_AsUnsignedLong(args[2]);
        } else {
            _PyArg_BadArgument("setprv", "args[2]", "bool | long", args[2]);
            return NULL;
        }
    }

    int status = 0;
    struct _generic_64 previous;
    Py_BEGIN_ALLOW_THREADS
    status = sys$setprv(enbflg, &prv, prmflg, &previous);
    Py_END_ALLOW_THREADS
    return Py_BuildValue("(i,K)", status, previous);
}

// unsigned int _readvblk(unsigned short int chan, void *rbuffer, long long *rlen, unsigned short *iostatus, long long p3, unsigned int func_mod) {
static PyObject*
SYS_readvblk(
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("readvblk", nargs, 2, 4)) {
        return NULL;
    }

    unsigned short int chan = 0;
    ConvertPosArgToUnsignedLong(0, chan, "readvblk");

    void* buffer = NULL;
    long long size = 0;
    int free_buffer = 0;
    if (PyBytes_CheckExact(args[1])) {
        if (-1 == PyBytes_AsStringAndSize(args[1], (char**)&buffer, (Py_ssize_t*)&size)) {
            return NULL;
        }
    } else if (PyLong_CheckExact(args[1])) {
        size = PyLong_AsLongLong(args[1]);
        buffer = PyMem_Malloc(size);
        if (buffer == NULL) {
            PyErr_SetNone(PyExc_MemoryError);
            return NULL;
        }
        free_buffer = 1;
    }

    long long start = 0;
    ConvertPosArgToLongLong(2, start, "readvblk");

    unsigned int func_mod = 0;
    ConvertPosArgToUnsignedLong(3, func_mod, "readvblk");

    struct _iosb iosb;
    int status = 0;
    Py_BEGIN_ALLOW_THREADS
    status = SYS$QIOW(
        0,                          /* efn - event flag */
        chan,                       /* chan - channel number */
        IO$_READVBLK | func_mod,    /* func - function modifier */
        &iosb,                      /* iosb - I/O status block */
        0,                          /* astadr - AST routine */
        0,                          /* astprm - AST parameter */
        buffer,                     /* p1 - input buffer */
        size,                       /* p2 - size of buffer */
        start,                      /* starting vblock */
        0,0,0);                     /* p4-p6*/
    Py_END_ALLOW_THREADS
    PyObject *pRet = Py_BuildValue("(i,y#,H)", status, buffer, iosb.iosb$w_bcnt, iosb.iosb$w_status);
    if (free_buffer) {
        PyMem_Free(buffer);
    }
    return pRet;
}

// unsigned int _writevblk(unsigned short int chan, void *wbuffer, long long *wlen, unsigned short *iostatus, long long p3, unsigned int func_mod) {
static PyObject*
SYS_writevblk(
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("writevblk", nargs, 2, 4)) {
        return NULL;
    }

    unsigned short int chan = 0;
    ConvertPosArgToUnsignedLong(0, chan, "writevblk");

    if (!PyBytes_CheckExact(args[1])) {
        _PyArg_BadArgument("writevblk", "args[1]", "bytes", args[1]);
        return NULL;
    }
    void* buffer = NULL;
    long long size = 0;
    if (-1 == PyBytes_AsStringAndSize(args[1], (char**)&buffer, (Py_ssize_t*)&size)) {
        return NULL;
    }

    long long start = 0;
    ConvertPosArgToLongLong(2, start, "writevblk");

    unsigned int func_mod = 0;
    ConvertPosArgToUnsignedLong(3, func_mod, "writevblk");

    struct _iosb iosb;
    int status = 0;
    Py_BEGIN_ALLOW_THREADS
    status = SYS$QIOW(
        0,                          /* efn - event flag */
        chan,                       /* chan - channel number */
        IO$_WRITEVBLK | func_mod,   /* func - function modifier */
        &iosb,                      /* iosb - I/O status block */
        0,                          /* astadr - AST routine */
        0,                          /* astprm - AST parameter */
        buffer,                     /* p1 - output buffer */
        size,                       /* p2 - size of buffer */
        start,                      /* starting vblock */
        0,0,0);                     /* p4-p6*/
    Py_END_ALLOW_THREADS
    return Py_BuildValue("(i,H,H)", status, iosb.iosb$w_bcnt, iosb.iosb$w_status);
}

static PyObject*
SYS_sndopr(
    PyObject *self,
    PyObject *args)
{
    char *message = NULL;
    Py_ssize_t message_size = 0;
    ConvertArgToStr(args, message, message_size, "sndopr");

    $DESCRIPTOR(msg_dsc, "");
    OPCDEF  msg;

    msg.opc$b_ms_type   = OPC$_RQ_RQST;
    msg.opc$b_ms_target = OPC$M_NM_CENTRL;
    msg.opc$l_ms_rqstid = 0;

    message_size = min(message_size, 128);

    memcpy((char *) &msg.opc$l_ms_text, message, message_size);

    msg_dsc.dsc$w_length = message_size + 8;
    msg_dsc.dsc$a_pointer = (__char_ptr32) &msg;

    int status = 0;
    Py_BEGIN_ALLOW_THREADS
    status = sys$sndopr(&msg_dsc, 0);
    Py_END_ALLOW_THREADS

    return PyLong_FromLong(status);
}


/********************************************************************
  Module
*/

static PyMethodDef _module_methods[] = {
    {"asctim", (PyCFunction) SYS_asctim, METH_FASTCALL,
        PyDoc_STR("asctim(time: int, cvtflg: bool)->[status: int, result: str]   Converts time to string")},
    {"bintim", (PyCFunction) SYS_bintim, METH_O,
        PyDoc_STR("bintim(time: str)->[status: int, result: int]   Converts time string to time")},
    {"hiber", (PyCFunction) SYS_hiber, METH_NOARGS,
        PyDoc_STR("hiber()->status: int   Hibernates process")},
    {"schdwk", (PyCFunction) SYS_schdwk, METH_FASTCALL,
        PyDoc_STR("schdwk(time: int, ?proc: str | int, ?rep_time: int)->[status: int, pid: int]   Schedules wake up time")},
    {"assign", (PyCFunction) SYS_assign, METH_FASTCALL,
        PyDoc_STR("assign(devnam: str, ?acmode: int, ?mbxnam: str, ?flags: int)->[status: int, chan: int]   Provides a process with an I/O channel")},
    {"dassgn", (PyCFunction) SYS_dassgn, METH_O,
        PyDoc_STR("dassgn(chan: int)->status: int   Closes an I/O channel")},
    {"cancel", (PyCFunction) SYS_cancel, METH_O,
        PyDoc_STR("cancel(chan: int)->status: int   Cancels an I/O channel")},
    {"finish_rdb", (PyCFunction) SYS_finish_rdb, METH_O,
        PyDoc_STR("finish_rdb(context: int)->status: int   Deallocates the record stream")},
    {"find_held", (PyCFunction) SYS_find_held, METH_FASTCALL,
        PyDoc_STR("find_held(holder: int, context: int)->[status: int, id: int, attrib: int, context: int]   Returns the identifiers held by the specified holder")},
    {"asctoid", (PyCFunction) SYS_asctoid, METH_O,
        PyDoc_STR("asctoid(name: int)->[status: int, id: int, attrib: int]   Translates the specified identifier name into its binary identifier value")},
    {"getmsg", (PyCFunction) SYS_getmsg, METH_FASTCALL,
        PyDoc_STR("getmsg(msgid: int, flags: int)->[status: int, msg: str, out: (int, int)]   Returns message text associated with a given message identification code")},
    {"idtoasc", (PyCFunction) SYS_idtoasc, METH_FASTCALL,
        PyDoc_STR("idtoasc(id: int, ?context: int)->[status: int, name: str, id: int, attrib: int, context: int]   Translates the specified identifier value to the identifier name")},
    {"crembx", (PyCFunction) SYS_crembx, METH_FASTCALL,
        PyDoc_STR("crembx(?prmflg: bool | int, ?maxmsg: int, ?bufquo: int, ?promsk: int, ?acmode: int, ?mbx_name: str, ?flags: int)->[status: int, chan: int]   Creates mailbox")},
    {"delmbx", (PyCFunction) SYS_delmbx, METH_O,
        PyDoc_STR("delmbx(chan: int)->status: int   Deletes mailbox")},
    {"getrmi", (PyCFunction) SYS_getrmi, METH_O,
        PyDoc_STR("getrmi(list: ile3list)->status: int   Returns system performance information")},
    {"getuai", (PyCFunction) SYS_getuai, METH_FASTCALL,
        PyDoc_STR("getuai(user: str, list: ile3list)->status: int   Reads the user authorization file")},
    {"setuai", (PyCFunction) SYS_setuai, METH_FASTCALL,
        PyDoc_STR("setuai(user: str, list: ile3list)->status: int   Modifies the user authorization file")},
    {"getjpi", (PyCFunction) SYS_getjpi, METH_FASTCALL,
        PyDoc_STR("getjpi(list: ile3list, ?process: int | str)->[status: int, pid: int]   Returns information about process(es)")},
    {"getqui", (PyCFunction) SYS_getqui, METH_FASTCALL,
        PyDoc_STR("getqui(func: int , context: int, list: ile3list)->[status: int, context: int]   Returns information about queues")},
    {"getsyi", (PyCFunction) SYS_getsyi, METH_FASTCALL,
        PyDoc_STR("getsyi(list: ile3list, ?node: int | str)->[status: int, csid: int]   Returns information about the system")},
    {"getdvi", (PyCFunction) SYS_getdvi, METH_FASTCALL,
        PyDoc_STR("getdvi(dev: str | int, list: ile3list)->status: int   Returns device/channel information")},
    {"device_scan", (PyCFunction) SYS_device_scan, METH_FASTCALL,
        PyDoc_STR("device_scan(?patt: str, ?list: ile3list, ?ctxt: int)->[status: int, dev: str, ctxt: int]   Scans devices")},
    {"dellnm", (PyCFunction) SYS_dellnm, METH_FASTCALL,
        PyDoc_STR("dellnm(?name: str, ?table: str, ?acmode: int)->status: int   Deletes logical name")},
    {"trnlnm", (PyCFunction) SYS_trnlnm, METH_FASTCALL,
        PyDoc_STR("trnlnm(name: str, ?table: str, ?list: ile3list, ?acmode: int, ?attr: int)->status: int   Translates logical name")},
    {"crelnm", (PyCFunction) SYS_crelnm, METH_FASTCALL,
        PyDoc_STR("crelnm(name: str, ?table: str, ?list: ile3list, ?acmode: int, ?attr: int)->status: int   Translates logical name")},
    {"forcex", (PyCFunction) SYS_forcex, METH_FASTCALL,
        PyDoc_STR("forcex(code: int, proc: str | int)->status: int   Forses process exit")},
    {"delprc", (PyCFunction) SYS_delprc, METH_FASTCALL,
        PyDoc_STR("delprc(proc: str | int)->status: int   Deletes a process")},
    {"rem_ident", (PyCFunction) SYS_rem_ident, METH_O,
        PyDoc_STR("rem_ident(id: int)->status: int   Removes identifier")},
    {"add_ident", (PyCFunction) SYS_add_ident, METH_FASTCALL,
        PyDoc_STR("add_ident(name: str, id: int, attrib: int)->[status: int, id: int]   Adds the specified identifier")},
    {"rem_holder", (PyCFunction) SYS_rem_holder, METH_FASTCALL,
        PyDoc_STR("rem_holder(id: int, holder: int)->status: int   Removes a holder")},
    {"add_holder", (PyCFunction) SYS_add_holder, METH_FASTCALL,
        PyDoc_STR("add_holder(id: int, holder: int, ?attr: int)->status: int   Adds a holder")},
    {"getlki", (PyCFunction) SYS_getlki, METH_FASTCALL,
        PyDoc_STR("getlki(list: ile3list, ?lkid: int)->[status: int, lkid: int]   Returns locking information")},
    {"uicstr", (PyCFunction) SYS_uicstr, METH_FASTCALL,
        PyDoc_STR("uicstr(uic: int, ?flag: bool | int)->[status: int, uic: str]   Converts UIC to a string")},
    {"gettim", (PyCFunction) SYS_gettim, METH_FASTCALL,
        PyDoc_STR("gettim(?flag: bool | int)->[status: int, time: int]   Returns the current system time")},
    {"show_intrusion", (PyCFunction) SYS_show_intrusion, METH_FASTCALL,
        PyDoc_STR("show_intrusion(criteria: str | ile3list, ?flags: int, ?context: int)->[status: int, intruder: str, context: int, type: int, count: int, expires: int]   Shows intrusions")},
    {"setprv", (PyCFunction) SYS_setprv, METH_FASTCALL,
        PyDoc_STR("setprv(prv: int, ?enable: bool | int, ?perm: bool | int)->[status: int, prv: int]   Changes privileges")},
    {"readvblk", (PyCFunction) SYS_readvblk, METH_FASTCALL,
        PyDoc_STR("readvblk(chan: int, data: bytes | int, ?start: int, ?func: int)->[status: int, data: bytes, iostatus: int]   Reads from the channel")},
    {"writevblk", (PyCFunction) SYS_writevblk, METH_FASTCALL,
        PyDoc_STR("writevblk(chan: int, data: bytes, ?start: int, ?func: int)->[status: int, written: int, iostatus: int]   Writes to the channel")},
    {"sndopr", (PyCFunction) SYS_sndopr, METH_O,
        PyDoc_STR("sndopr(message: str)->status: int   Sends message to operator terminals")},

    {NULL, NULL}
};

static struct PyModuleDef _module_definition = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_sys",
    .m_doc = "OpenVMS SYS$ implementation.",
    .m_size = -1,
    .m_methods = _module_methods,
};

PyMODINIT_FUNC PyInit__sys(void)
{
    PyObject *m = PyModule_Create(&_module_definition);
    return m;
}
