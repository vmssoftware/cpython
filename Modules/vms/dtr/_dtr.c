#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "structmember.h"

#define __NEW_STARLET 1
#include <str$routines.h>
#include <lib$routines.h>
#include <ssdef.h>
#include <stsdef.h>

#pragma names save
#pragma names uppercase
#include <dab.h>
#pragma names restore

#define MSG_BUF_LEN 255
#define AUX_BUF_LEN 255
#define TMP_BUF_LEN 255

#define DTR_MODULE_NAME "_dtr"
#define DTR_TYPE_NAME "dab"

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

#define ConvertPosArgToLongP(pos, value, func_name)             \
    if (nargs > pos && args[pos] != Py_None) {                  \
        if (!PyLong_Check(args[pos])) {                         \
            _PyArg_BadArgument(func_name, "args[" #pos "]", "long", args[pos]); \
            return NULL;                                        \
        }                                                       \
        value = PyLong_AsLong(args[pos]);                       \
        p##value = &value;                                      \
    }

typedef struct {
    PyObject_HEAD
    long                 status;
    long                 finished;
    DTR_access_block    dab;
} DTRObject;

static PyObject*
DTR_new(
    PyTypeObject *type,
    PyObject *args,
    PyObject *kwds)
{
    DTRObject *self;
    self = (DTRObject *) type->tp_alloc(type, 0);
    if (self != NULL) {
        self->status = 0;
        self->finished = 1;
        self->dab.dab$a_msg_buf = PyMem_Malloc(MSG_BUF_LEN + 1);
        if (NULL == self->dab.dab$a_msg_buf) {
            return PyErr_NoMemory();
        }
        self->dab.dab$w_msg_buf_len = MSG_BUF_LEN;

        self->dab.dab$a_aux_buf = PyMem_Malloc(MSG_BUF_LEN + 1);
        if (NULL == self->dab.dab$a_aux_buf) {
            return PyErr_NoMemory();
        }
        self->dab.dab$w_aux_buf_len = MSG_BUF_LEN;
    }
    return (PyObject *) self;
}

static void
DTR_dealloc(DTRObject *self)
{
    if (!self->finished) {
        dtr$finish(&self->dab);
        self->finished = 1;
    }
    if (self->dab.dab$a_msg_buf) {
        PyMem_Free(self->dab.dab$a_msg_buf);
        self->dab.dab$a_msg_buf = NULL;
    }
    if (self->dab.dab$a_aux_buf) {
        PyMem_Free(self->dab.dab$a_aux_buf);
        self->dab.dab$a_aux_buf = NULL;
    }
    PyObject_Del(self);
}

long decc$feature_set(const char* __name, long __mode, long __value);
void vms_set_crtl_values(void);

// unsigned long _init(unsigned long *obj, long size, long options)
static int
DTR_init(
    DTRObject *self,
    PyObject *args,
    PyObject *kwds)
{
    static char *kwlist[] = {"size", "options", NULL};
    long size = 0;
    long options = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|ii", kwlist, &size, &options)) {
        return -1;
    }

    decc$feature_set("DECC$UNIX_LEVEL", 1, 0);

    self->status = 0;
    Py_BEGIN_ALLOW_THREADS
    self->status = dtr$init(&self->dab, size? &size : NULL, NULL, NULL, options? &options : NULL);
    Py_END_ALLOW_THREADS

    vms_set_crtl_values();

    if (!$VMS_STATUS_SUCCESS(self->status)) {
        return -1;
    }
    self->finished = 0;
    return 0;
}

// unsigned long _finish(unsigned long obj)
static PyObject*
DTR_finish(
    DTRObject * self,
    PyObject * args)
{
    self->status = 0;
    Py_BEGIN_ALLOW_THREADS
    self->status = dtr$finish(&self->dab);
    Py_END_ALLOW_THREADS
    self->finished = 1;
    return PyLong_FromLong(self->status);
}

// unsigned long _command(unsigned long obj, char *str, long *condition, unsigned short *state)
static PyObject*
DTR_command(
    DTRObject *self,
    PyObject *args)
{
    char *cmd = NULL;
    Py_ssize_t cmd_size = 0;
    ConvertArgToStr(args, cmd, cmd_size, "command");

    struct dsc$descriptor_s str_dsc;
    str_dsc.dsc$w_length = cmd_size;
    str_dsc.dsc$b_class = DSC$K_CLASS_S;
    str_dsc.dsc$b_dtype = DSC$K_DTYPE_T;
    str_dsc.dsc$a_pointer = cmd;

    self->status = 0;
    Py_BEGIN_ALLOW_THREADS
    self->status = dtr$command(&self->dab, &str_dsc);
    Py_END_ALLOW_THREADS
    return PyLong_FromLong(self->status);
}

static PyObject *
DTR_get_message(DTRObject *self, void *closure)
{
    struct dsc$descriptor_s tmp_dsc;
    unsigned short result_len;
    tmp_dsc.dsc$w_length = MSG_BUF_LEN;
    tmp_dsc.dsc$b_class = DSC$K_CLASS_S;
    tmp_dsc.dsc$b_dtype = DSC$K_DTYPE_T;
    tmp_dsc.dsc$a_pointer = self->dab.dab$a_msg_buf;

    self->status = 0;
    Py_BEGIN_ALLOW_THREADS
    str$trim(&tmp_dsc, &tmp_dsc, &result_len);
    Py_END_ALLOW_THREADS

    self->dab.dab$a_msg_buf[result_len] = '\0';
    return PyUnicode_FromString(self->dab.dab$a_msg_buf);
}

static PyObject *
DTR_get_aux_buf(DTRObject *self, void *closure)
{
    struct dsc$descriptor_s tmp_dsc;
    unsigned short result_len;
    tmp_dsc.dsc$w_length = AUX_BUF_LEN;
    tmp_dsc.dsc$b_class = DSC$K_CLASS_S;
    tmp_dsc.dsc$b_dtype = DSC$K_DTYPE_T;
    tmp_dsc.dsc$a_pointer = self->dab.dab$a_aux_buf;

    self->status = 0;
    Py_BEGIN_ALLOW_THREADS
    str$trim(&tmp_dsc, &tmp_dsc, &result_len);
    Py_END_ALLOW_THREADS

    self->dab.dab$a_aux_buf[result_len] = '\0';
    return PyUnicode_FromString(self->dab.dab$a_aux_buf);
}

// unsigned long _continue(unsigned long obj, long *condition, unsigned short *state)
static PyObject*
DTR_continue(
    DTRObject *self,
    PyObject *args)
{
    self->status = 0;
    Py_BEGIN_ALLOW_THREADS
    self->status = dtr$continue(&self->dab);
    Py_END_ALLOW_THREADS

    return PyLong_FromLong(self->status);
}

static PyObject*
DTR_skip(
    DTRObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    long rows = 1;
    if (nargs > 0 && args[0] != Py_None) {
        if (!PyLong_Check(args)) {
            _PyArg_BadArgument("skip", "args[0]", "long", args[0]);
            return NULL;
        }
        rows = PyLong_AsLong(args[0]);
    }

    long skipped = 0;
    self->status = 0;
    Py_BEGIN_ALLOW_THREADS
    while(skipped < rows && self->dab.dab$w_state == DTR$K_STL_LINE) {
        self->status = dtr$continue(&self->dab);
        if (!$VMS_STATUS_SUCCESS(self->status)) {
            break;
        }
        ++skipped;
    }
    Py_END_ALLOW_THREADS

    return PyLong_FromLong(skipped);
}

// unsigned long _create_udk(unsigned long obj, char *str, short index, short context)
static PyObject*
DTR_create_udk(
    DTRObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("create_udk", nargs, 3, 3)) {
        return NULL;
    }
    char *str = NULL;
    Py_ssize_t str_size = 0;
    ConvertArgToStr(args[0], str, str_size, "create_udk");

    short index = 0, *pindex = NULL;
    ConvertPosArgToLongP(1, index, "create_udk");

    short context = 0, *pcontext = NULL;
    ConvertPosArgToLongP(2, context, "create_udk");

    struct dsc$descriptor_s str_dsc;
    str_dsc.dsc$w_length = str_size;
    str_dsc.dsc$b_class = DSC$K_CLASS_S;
    str_dsc.dsc$b_dtype = DSC$K_DTYPE_T;
    str_dsc.dsc$a_pointer = str;

    self->status = 0;
    Py_BEGIN_ALLOW_THREADS
    self->status = dtr$create_udk(&self->dab, &str_dsc, &index, &context);
    Py_END_ALLOW_THREADS

    return PyLong_FromLong(self->status);
}

// unsigned long _end_udk(unsigned long obj)
static PyObject*
DTR_end_udk(
    DTRObject *self,
    PyObject *args)
{
    self->status = 0;
    Py_BEGIN_ALLOW_THREADS
    self->status = dtr$end_udk(&self->dab);
    Py_END_ALLOW_THREADS
    return PyLong_FromLong(self->status);
}

// unsigned long _get_port(unsigned long obj, void *loc)
static PyObject *
DTR_get_port(DTRObject *self, void *closure)
{
    char *buff = PyMem_Malloc(self->dab.dab$w_rec_length);
    self->status = 0;
    Py_BEGIN_ALLOW_THREADS
    self->status = dtr$get_port(&self->dab, buff);
    Py_END_ALLOW_THREADS
    return PyBytes_FromStringAndSize(buff, self->dab.dab$w_rec_length);
}

static int
DTR_set_port(DTRObject *self, PyObject *value, void *closure)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete the port attribute");
        return -1;
    }
    if (!PyBytes_CheckExact(value)) {
        PyErr_SetString(PyExc_TypeError,
                        "The port attribute value must be a bytes");
        return -1;
    }
    char *buff = NULL;
    Py_ssize_t size = 0;
    PyBytes_AsStringAndSize(value, &buff, &size);
    if (size != self->dab.dab$w_rec_length) {
        PyErr_SetString(PyExc_ValueError,
                        "The port attribute value must be the same size as the record");
        return -1;
    }
    self->status = 0;
    Py_BEGIN_ALLOW_THREADS
    self->status = dtr$put_port(&self->dab, buff);
    Py_END_ALLOW_THREADS

    if (!$VMS_STATUS_SUCCESS(self->status)) {
        return -1;
    }
    return 0;
}

// unsigned long _get_string(unsigned long obj, long type, char **str, char *cmp)
static PyObject*
DTR_get_string(
    DTRObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("get_string", nargs, 1, 2)) {
        return NULL;
    }
    long type = 0, *ptype = NULL;
    ConvertPosArgToLongP(0, type, "get_string");

    char *cmp = NULL;
    Py_ssize_t cmp_size = 0;
    if (nargs > 1 && args[1] != Py_None) {
        ConvertArgToStr(args[1], cmp, cmp_size, "get_string");
    }

    char buffer[TMP_BUF_LEN + 1];
    unsigned short result_len = 0;
    struct dsc$descriptor_s tmp_dsc;
    struct dsc$descriptor_s cmp_dsc, *pcmp_dsc = NULL;

    if (cmp && cmp_size) {
        cmp_dsc.dsc$w_length = cmp_size;
        cmp_dsc.dsc$b_class = DSC$K_CLASS_S;
        cmp_dsc.dsc$b_dtype = DSC$K_DTYPE_T;
        cmp_dsc.dsc$a_pointer = cmp;
        pcmp_dsc = &cmp_dsc;
    }

    tmp_dsc.dsc$w_length = TMP_BUF_LEN;
    tmp_dsc.dsc$b_class = DSC$K_CLASS_S;
    tmp_dsc.dsc$b_dtype = DSC$K_DTYPE_T;
    tmp_dsc.dsc$a_pointer = buffer;

    self->status = 0;
    Py_BEGIN_ALLOW_THREADS
    self->status = dtr$get_string(&self->dab, &type, &tmp_dsc, &result_len, pcmp_dsc);
    Py_END_ALLOW_THREADS

    if (!$VMS_STATUS_SUCCESS(self->status)) {
        result_len = 0;
    }
    buffer[result_len] = 0;

    return PyUnicode_FromString(buffer);
}

// unsigned long _info(unsigned long obj, long id, char code, long *ret, char **str, long index)
static PyObject*
DTR_info(
    DTRObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("info", nargs, 2, 3)) {
        return NULL;
    }
    long id = 0, *pid = NULL;
    ConvertPosArgToLongP(0, id, "info");
    unsigned long code = 0;
    char *pcode = NULL;
    Py_ssize_t size = 0;
    if (PyUnicode_CheckExact(args[1])) {
        pcode = (char*)PyUnicode_AsUTF8AndSize(args[1], &size);
    } else if (PyBytes_CheckExact(args[1])) {
        PyBytes_AsStringAndSize(args[1], &pcode, &size);
    } else if (PyLong_Check(args[1])) {
        code = PyLong_AsUnsignedLong(args[1]);
        if (code > 255) {
            PyErr_SetString(PyExc_ValueError, "Too big char code");
            return NULL;
        }
        pcode = (char*)&code;
        size = 1;
    }
    if (size > 1) {
        PyErr_SetString(PyExc_ValueError, "One char string is allowed");
        return NULL;
    }
    long index = 0, *pindex = NULL;
    ConvertPosArgToLongP(2, index, "info");

    char buffer[TMP_BUF_LEN + 1];
    unsigned short result_len;
    struct dsc$descriptor_s tmp_dsc;

    tmp_dsc.dsc$w_length = TMP_BUF_LEN;
    tmp_dsc.dsc$b_class = DSC$K_CLASS_S;
    tmp_dsc.dsc$b_dtype = DSC$K_DTYPE_T;
    tmp_dsc.dsc$a_pointer = buffer;

    long ret = 0;
    self->status = 0;
    Py_BEGIN_ALLOW_THREADS
    self->status = dtr$info(&self->dab, &id, pcode, &ret, &tmp_dsc, index);
    Py_END_ALLOW_THREADS

    if (!$VMS_STATUS_SUCCESS(self->status)) {
        result_len = 0;
    }
    buffer[result_len] = 0;
    return Py_BuildValue("(l,s)", ret, buffer);
}


// unsigned long _lookup(unsigned long obj, char type, long *id, char *name)
static PyObject*
DTR_lookup(
    DTRObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("lookup", nargs, 2, 3)) {
        return NULL;
    }
    unsigned long code = 0;
    char *pcode = NULL;
    Py_ssize_t size = 0;
    if (PyUnicode_CheckExact(args[0])) {
        pcode = (char*)PyUnicode_AsUTF8AndSize(args[0], &size);
    } else if (PyBytes_CheckExact(args[0])) {
        PyBytes_AsStringAndSize(args[0], &pcode, &size);
    } else if (PyLong_Check(args[0])) {
        code = PyLong_AsUnsignedLong(args[0]);
        if (code > 255) {
            PyErr_SetString(PyExc_ValueError, "Too big char code");
            return NULL;
        }
        pcode = (char*)&code;
        size = 1;
    }
    if (size > 1) {
        PyErr_SetString(PyExc_ValueError, "One char string is allowed");
        return NULL;
    }
    char *name = NULL;
    Py_ssize_t name_size = 0;
    if (nargs > 1 && args[1] != Py_None) {
        ConvertArgToStr(args[1], name, name_size, "lookup")
    }

    struct dsc$descriptor_s name_dsc, *pname_dsc = NULL;

    if (name && name_size) {
        name_dsc.dsc$w_length = name_size;
        name_dsc.dsc$b_class = DSC$K_CLASS_S;
        name_dsc.dsc$b_dtype = DSC$K_DTYPE_T;
        name_dsc.dsc$a_pointer = name;
        pname_dsc = &name_dsc;
    }

    long id = 0;
    self->status = 0;
    Py_BEGIN_ALLOW_THREADS
    self->status = dtr$lookup(&self->dab, pcode, &id, pname_dsc);
    Py_END_ALLOW_THREADS

    return PyLong_FromLong(id);
}

// unsigned long _port_eof(unsigned long obj)
static PyObject*
DTR_port_eof(
    DTRObject * self,
    PyObject * args)
{
    self->status = 0;
    Py_BEGIN_ALLOW_THREADS
    self->status = dtr$port_eof(&self->dab);
    Py_END_ALLOW_THREADS
    return PyLong_FromLong(self->status);
}


// unsigned long _put_value(unsigned long obj, char *val)
static PyObject*
DTR_put_value(
    DTRObject *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("put_value", nargs, 0, 1)) {
        return NULL;
    }
    char *value = NULL;
    Py_ssize_t value_size = 0;
    if (nargs > 0) {
        ConvertArgToStr(args[0], value, value_size, "put_value");
    }
    struct dsc$descriptor_s val_dsc, *pval_dsc;

    if (value && value_size) {
        val_dsc.dsc$w_length = value_size;
        val_dsc.dsc$b_class = DSC$K_CLASS_S;
        val_dsc.dsc$b_dtype = DSC$K_DTYPE_T;
        val_dsc.dsc$a_pointer = value;
        pval_dsc = &val_dsc;
    }
    self->status = 0;
    Py_BEGIN_ALLOW_THREADS
    self->status = dtr$put_value(&self->dab, pval_dsc);
    Py_END_ALLOW_THREADS
    return PyLong_FromLong(self->status);
}

// unsigned long _unwind(unsigned long obj)
static PyObject*
DTR_unwind(
    DTRObject * self,
    PyObject * args)
{
    self->status = 0;
    Py_BEGIN_ALLOW_THREADS
    self->status = dtr$unwind(&self->dab);
    Py_END_ALLOW_THREADS
    return PyLong_FromLong(self->status);
}

// unsigned long _dtr(unsigned long obj, long opt)
static PyObject*
DTR_dtr(
    DTRObject *self,
    PyObject *args)
{
    if (!PyLong_Check(args)) {
        _PyArg_BadArgument("dtr", "args", "long", args);
        return NULL;
    }
    long opt = PyLong_AsLong(args);
    self->status = 0;
    Py_BEGIN_ALLOW_THREADS
    self->status = dtr$dtr(&self->dab, &opt);
    Py_END_ALLOW_THREADS
    return PyLong_FromLong(self->status);
}


// extern long rsscanf(const char *buffer, const char *format, char ***argv);

// char **_row(unsigned long obj, char *fmt)
// {
//     DTR_access_block *dab = (DTR_access_block *) obj;
//     char **arr = NULL;

//     dtr_Assert(dab);
//     dtr_Assert(fmt);

//     rsscanf(dab->dab$a_msg_buf, fmt, &arr);
//     dtr$continue(dab);
//     return (arr);
// }

/********************************************************************
  Type
*/

static PyMethodDef DTR_methods[] = {
    {"finish", (PyCFunction) DTR_finish, METH_NOARGS,
        PyDoc_STR("finish()->status: int   Finish")},
    {"command", (PyCFunction) DTR_command, METH_O,
        PyDoc_STR("command(cmd: str)->status: int   Command")},
    {"cont", (PyCFunction) DTR_continue, METH_NOARGS,
        PyDoc_STR("cont()->status: int   Continue")},
    {"skip", (PyCFunction) DTR_skip, METH_FASTCALL,
        PyDoc_STR("skip(?n: int)->skipped: int   Skip n rows")},
    {"create_udk", (PyCFunction) DTR_create_udk, METH_FASTCALL,
        PyDoc_STR("create_udk(str: str, index: int, context: int)->status: int   Create UDK")},
    {"dtr", (PyCFunction) DTR_dtr, METH_O,
        PyDoc_STR("dtr(opt: int)->status: int   Dtr")},
    {"unwind", (PyCFunction) DTR_unwind, METH_NOARGS,
        PyDoc_STR("unwind()->status: int   Unwind")},
    {"port_eof", (PyCFunction) DTR_port_eof, METH_NOARGS,
        PyDoc_STR("port_eof()->status: int   Port EOF")},
    {"get_string", (PyCFunction) DTR_get_string, METH_FASTCALL,
        PyDoc_STR("get_string(type: int, ?cmp: str)->str: str   Get string")},
    {"info", (PyCFunction) DTR_info, METH_FASTCALL,
        PyDoc_STR("info(id: int, code: int, ?index: int)->[ret: int, info: str]   Get info")},
    {"lookup", (PyCFunction) DTR_lookup, METH_FASTCALL,
        PyDoc_STR("lookup(type: int, ?name: str)->id: int   Lookup")},
    {"put_value", (PyCFunction) DTR_put_value, METH_FASTCALL,
        PyDoc_STR("put_value(&value: str)->status: int   Put a value")},

    {NULL, NULL}
};

static PyMemberDef DTR_members[] = {
    {"condition", T_LONG, offsetof(DTRObject, dab.dab$l_condition), READONLY,
     "Condition"},
    {"state", T_USHORT, offsetof(DTRObject, dab.dab$w_state), READONLY,
     "State"},
    {"status", T_INT, offsetof(DTRObject, status), READONLY,
     "State of the last function"},
    {"options", T_ULONG, offsetof(DTRObject, dab.dab$l_options), 0,
     "Options"},
    {"udk_index", T_SHORT, offsetof(DTRObject, dab.dab$w_udk_index), READONLY,
     "UDK index"},
    {"flags", T_ULONG, offsetof(DTRObject, dab.dab$l_flags), READONLY,
     "Flags"},
    {"rec_length", T_USHORT, offsetof(DTRObject, dab.dab$w_rec_length), READONLY,
     "Record length"},

    {NULL}
};

static PyGetSetDef DTR_getsetters[] = {
    {"message", (getter) DTR_get_message, NULL,
     "message", NULL},
    {"aux_buf", (getter) DTR_get_aux_buf, NULL,
     "aux_buf", NULL},
    {"port", (getter) DTR_get_port, (setter) DTR_set_port,
     "port", NULL},
    {NULL}  /* Sentinel */
};

PyTypeObject DTR_Type = {
    PyObject_HEAD_INIT(NULL)
    DTR_MODULE_NAME "." DTR_TYPE_NAME,
    .tp_doc = "DTR object",
    .tp_basicsize = sizeof(DTRObject),
    .tp_itemsize = 0,
    .tp_getattro = PyObject_GenericGetAttr,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = DTR_new,
    .tp_init = (initproc)DTR_init,
    .tp_dealloc = (destructor) DTR_dealloc,
    .tp_methods = DTR_methods,
    .tp_members = DTR_members,
    .tp_getset = DTR_getsetters,
};

/********************************************************************
  Module
*/

static PyMethodDef _module_methods[] = {
    {NULL, NULL}
};

static struct PyModuleDef _module_definition = {
    PyModuleDef_HEAD_INIT,
    .m_name = DTR_MODULE_NAME,
    .m_doc = "OpenVMS DTR implementation.",
    .m_size = -1,
    .m_methods = _module_methods,
};

PyMODINIT_FUNC PyInit__dtr(void)
{
    if (PyType_Ready(&DTR_Type) < 0) {
        return NULL;
    }
    PyObject *m = PyModule_Create(&_module_definition);
    if (m == NULL) {
        return NULL;
    }
    Py_INCREF(&DTR_Type);
    if (PyModule_AddObject(m, DTR_TYPE_NAME, (PyObject *) &DTR_Type) < 0) {
        Py_DECREF(&DTR_Type);
        Py_DECREF(m);
        return NULL;
    }
    PyModule_AddIntConstant(m, "DTRDTR_K_STL_CMD", DTR$K_STL_CMD);
    PyModule_AddIntConstant(m, "DTR_K_STL_PRMPT", DTR$K_STL_PRMPT);
    PyModule_AddIntConstant(m, "DTR_K_STL_LINE", DTR$K_STL_LINE);
    PyModule_AddIntConstant(m, "DTR_K_STL_MSG", DTR$K_STL_MSG);
    PyModule_AddIntConstant(m, "DTR_K_STL_PGET", DTR$K_STL_PGET);
    PyModule_AddIntConstant(m, "DTR_K_STL_PPUT", DTR$K_STL_PPUT);
    PyModule_AddIntConstant(m, "DTR_K_STL_CONT", DTR$K_STL_CONT);
    PyModule_AddIntConstant(m, "DTR_K_STL_UDK", DTR$K_STL_UDK);
    PyModule_AddIntConstant(m, "DTR_K_STL_END_UDK", DTR$K_STL_END_UDK);
    PyModule_AddIntConstant(m, "DTR_K_SEMI_COLON_OPT", DTR_K_SEMI_COLON_OPT);
    PyModule_AddIntConstant(m, "DTR_K_UNQUOTED_LIT", DTR_K_UNQUOTED_LIT);
    PyModule_AddIntConstant(m, "DTR_K_SYNTAX_PROMPT", DTR_K_SYNTAX_PROMPT);
    PyModule_AddIntConstant(m, "DTR_K_IMMED_RETURN", DTR_K_IMMED_RETURN);
    PyModule_AddIntConstant(m, "DTR_K_FORMS_ENABLE", DTR_K_FORMS_ENABLE);
    PyModule_AddIntConstant(m, "DTR_K_VERIFY", DTR_K_VERIFY);
    PyModule_AddIntConstant(m, "DTR_K_BIG_ENDIAN", DTR_K_BIG_ENDIAN);
    PyModule_AddIntConstant(m, "DTR_K_IEEE_FLOAT", DTR_K_IEEE_FLOAT);
    PyModule_AddIntConstant(m, "DTR_K_CONTEXT_SEARCH", DTR_K_CONTEXT_SEARCH);
    PyModule_AddIntConstant(m, "DTR_K_HYPHEN_DISABLED", DTR_K_HYPHEN_DISABLED);
    PyModule_AddIntConstant(m, "DTR_K_MORE_COMMANDS", DTR_K_MORE_COMMANDS);
    PyModule_AddIntConstant(m, "DTR_K_ABORT", DTR_K_ABORT);
    PyModule_AddIntConstant(m, "DTR_K_LOCK_WAIT", DTR_K_LOCK_WAIT);
    PyModule_AddIntConstant(m, "DTR_K_FN_PORT_SUPPORT", DTR_K_FN_PORT_SUPPORT);
    PyModule_AddIntConstant(m, "DTR_K_DATE_STR", DTR_K_DATE_STR);
    PyModule_AddIntConstant(m, "DTR_K_RET_ON_PRINT_ERROR", DTR_K_RET_ON_PRINT_ERROR);
    PyModule_AddIntConstant(m, "DTR_K_GET_ROUTINES_SUPPORT", DTR_K_GET_ROUTINES_SUPPORT);
    PyModule_AddIntConstant(m, "DTR_M_OPT_CMD", DTR$M_OPT_CMD);
    PyModule_AddIntConstant(m, "DTR_M_OPT_PRMPT", DTR$M_OPT_PRMPT);
    PyModule_AddIntConstant(m, "DTR_M_OPT_LINE", DTR$M_OPT_LINE);
    PyModule_AddIntConstant(m, "DTR_M_OPT_MSG", DTR$M_OPT_MSG);
    PyModule_AddIntConstant(m, "DTR_M_OPT_PGET", DTR$M_OPT_PGET);
    PyModule_AddIntConstant(m, "DTR_M_OPT_PPUT", DTR$M_OPT_PPUT);
    PyModule_AddIntConstant(m, "DTR_M_OPT_CONT", DTR$M_OPT_CONT);
    PyModule_AddIntConstant(m, "DTR_M_OPT_UDK", DTR$M_OPT_UDK);
    PyModule_AddIntConstant(m, "DTR_M_OPT_DTR_UDK", DTR$M_OPT_DTR_UDK);
    PyModule_AddIntConstant(m, "DTR_M_OPT_END_UDK", DTR$M_OPT_END_UDK);
    PyModule_AddIntConstant(m, "DTR_M_OPT_UNWIND", DTR$M_OPT_UNWIND);
    PyModule_AddIntConstant(m, "DTR_M_OPT_CONTROL_C", DTR$M_OPT_CONTROL_C);
    PyModule_AddIntConstant(m, "DTR_M_OPT_STARTUP", DTR$M_OPT_STARTUP);
    PyModule_AddIntConstant(m, "DTR_M_OPT_FOREIGN", DTR$M_OPT_FOREIGN);
    PyModule_AddIntConstant(m, "DTR_M_OPT_BANNER", DTR$M_OPT_BANNER);
    PyModule_AddIntConstant(m, "DTR_M_OPT_REMOVE_CTLC", DTR$M_OPT_REMOVE_CTLC);
    PyModule_AddIntConstant(m, "DTR_M_OPT_KEYDEFS", DTR$M_OPT_KEYDEFS);
    PyModule_AddIntConstant(m, "DTR_K_UDK_SET", DTR$K_UDK_SET);
    PyModule_AddIntConstant(m, "DTR_K_UDK_SET_NO", DTR$K_UDK_SET_NO);
    PyModule_AddIntConstant(m, "DTR_K_UDK_SHOW", DTR$K_UDK_SHOW);
    PyModule_AddIntConstant(m, "DTR_K_UDK_STATEMENT", DTR$K_UDK_STATEMENT);
    PyModule_AddIntConstant(m, "DTR_K_UDK_COMMAND", DTR$K_UDK_COMMAND);
    PyModule_AddIntConstant(m, "DTR_K_TOK_TOKEN", DTR$K_TOK_TOKEN);
    PyModule_AddIntConstant(m, "DTR_K_TOK_PICTURE", DTR$K_TOK_PICTURE);
    PyModule_AddIntConstant(m, "DTR_K_TOK_FILENAME", DTR$K_TOK_FILENAME);
    PyModule_AddIntConstant(m, "DTR_K_TOK_COMMAND", DTR$K_TOK_COMMAND);
    PyModule_AddIntConstant(m, "DTR_K_TOK_TEST_TOKEN", DTR$K_TOK_TEST_TOKEN);
    PyModule_AddIntConstant(m, "DTR_K_TOK_LIST_ELEMENT", DTR$K_TOK_LIST_ELEMENT);
    PyModule_AddIntConstant(m, "DTR_K_TOK_TEST_EOL", DTR$K_TOK_TEST_EOL);
    PyModule_AddIntConstant(m, "DTR_SUCCESS", DTR_SUCCESS);
    PyModule_AddIntConstant(m, "DTR_K_INF_TYPE_DOMAIN", DTR$K_INF_TYPE_DOMAIN);
    PyModule_AddIntConstant(m, "DTR_K_INF_TYPE_COLLECTION", DTR$K_INF_TYPE_COLLECTION);
    PyModule_AddIntConstant(m, "DTR_K_INF_TYPE_KEYWORD", DTR$K_INF_TYPE_KEYWORD);
    PyModule_AddIntConstant(m, "DTR_K_INF_TYPE_DIC_NAME", DTR$K_INF_TYPE_DIC_NAME);
    PyModule_AddIntConstant(m, "DTR_K_INF_TYPE_GLV", DTR$K_INF_TYPE_GLV);
    PyModule_AddIntConstant(m, "DTR_K_INF_TYPE_PLOT", DTR$K_INF_TYPE_PLOT);
    PyModule_AddIntConstant(m, "DTR_K_INF_DOM_FLD", DTR$K_INF_DOM_FLD);
    PyModule_AddIntConstant(m, "DTR_K_INF_DOM_FORM", DTR$K_INF_DOM_FORM);
    PyModule_AddIntConstant(m, "DTR_K_INF_DOM_SHARE", DTR$K_INF_DOM_SHARE);
    PyModule_AddIntConstant(m, "DTR_K_INF_DOM_ACCESS", DTR$K_INF_DOM_ACCESS);
    PyModule_AddIntConstant(m, "DTR_K_INF_DOM_NAME", DTR$K_INF_DOM_NAME);
    PyModule_AddIntConstant(m, "DTR_K_INF_DOM_NEXT_DOM", DTR$K_INF_DOM_NEXT_DOM);
    PyModule_AddIntConstant(m, "DTR_K_INF_DOM_SSC", DTR$K_INF_DOM_SSC);
    PyModule_AddIntConstant(m, "DTR_K_INF_DOM_REC_LEN", DTR$K_INF_DOM_REC_LEN);
    PyModule_AddIntConstant(m, "DTR_K_INF_FLD_NAME", DTR$K_INF_FLD_NAME);
    PyModule_AddIntConstant(m, "DTR_K_INF_FLD_QNAME", DTR$K_INF_FLD_QNAME);
    PyModule_AddIntConstant(m, "DTR_K_INF_FLD_PICTURE", DTR$K_INF_FLD_PICTURE);
    PyModule_AddIntConstant(m, "DTR_K_INF_FLD_EDIT", DTR$K_INF_FLD_EDIT);
    PyModule_AddIntConstant(m, "DTR_K_INF_FLD_DTYPE", DTR$K_INF_FLD_DTYPE);
    PyModule_AddIntConstant(m, "DTR_K_INF_FLD_OFFSET", DTR$K_INF_FLD_OFFSET);
    PyModule_AddIntConstant(m, "DTR_K_INF_FLD_LENGTH", DTR$K_INF_FLD_LENGTH);
    PyModule_AddIntConstant(m, "DTR_K_INF_FLD_SCALE", DTR$K_INF_FLD_SCALE);
    PyModule_AddIntConstant(m, "DTR_K_INF_FLD_CHILD", DTR$K_INF_FLD_CHILD);
    PyModule_AddIntConstant(m, "DTR_K_INF_FLD_CNT", DTR$K_INF_FLD_CNT);
    PyModule_AddIntConstant(m, "DTR_K_INF_FLD_LIST", DTR$K_INF_FLD_LIST);
    PyModule_AddIntConstant(m, "DTR_K_INF_FLD_REDEFINES", DTR$K_INF_FLD_REDEFINES);
    PyModule_AddIntConstant(m, "DTR_K_INF_FLD_VIRTUAL", DTR$K_INF_FLD_VIRTUAL);
    PyModule_AddIntConstant(m, "DTR_K_INF_FLD_FILLER", DTR$K_INF_FLD_FILLER);
    PyModule_AddIntConstant(m, "DTR_K_INF_FLD_MISSING", DTR$K_INF_FLD_MISSING);
    PyModule_AddIntConstant(m, "DTR_K_INF_FLD_MISSING_TXT", DTR$K_INF_FLD_MISSING_TXT);
    PyModule_AddIntConstant(m, "DTR_K_INF_FLD_SEG_STRING", DTR$K_INF_FLD_SEG_STRING);
    PyModule_AddIntConstant(m, "DTR_K_INF_COL_CURSOR", DTR$K_INF_COL_CURSOR);
    PyModule_AddIntConstant(m, "DTR_K_INF_COL_SIZE", DTR$K_INF_COL_SIZE);
    PyModule_AddIntConstant(m, "DTR_K_INF_COL_FLD", DTR$K_INF_COL_FLD);
    PyModule_AddIntConstant(m, "DTR_K_INF_COL_DROPPED", DTR$K_INF_COL_DROPPED);
    PyModule_AddIntConstant(m, "DTR_K_INF_COL_ERASED", DTR$K_INF_COL_ERASED);
    PyModule_AddIntConstant(m, "DTR_K_INF_COL_INVISIBLE", DTR$K_INF_COL_INVISIBLE);
    PyModule_AddIntConstant(m, "DTR_K_INF_COL_NAME", DTR$K_INF_COL_NAME);
    PyModule_AddIntConstant(m, "DTR_K_INF_COL_NEXT_COL", DTR$K_INF_COL_NEXT_COL);
    PyModule_AddIntConstant(m, "DTR_K_INF_GLV_FIRST_DOM", DTR$K_INF_GLV_FIRST_DOM);
    PyModule_AddIntConstant(m, "DTR_K_INF_GLV_FIRST_COL", DTR$K_INF_GLV_FIRST_COL);
    PyModule_AddIntConstant(m, "DTR_K_INF_GLV_FIRST_SSC", DTR$K_INF_GLV_FIRST_SSC);
    PyModule_AddIntConstant(m, "DTR_K_INF_GLV_DEF_DIC", DTR$K_INF_GLV_DEF_DIC);
    PyModule_AddIntConstant(m, "DTR_K_INF_FRM_NAME", DTR$K_INF_FRM_NAME);
    PyModule_AddIntConstant(m, "DTR_K_INF_FRM_LIBRARY", DTR$K_INF_FRM_LIBRARY);
    PyModule_AddIntConstant(m, "DTR_K_INF_SSC_NAME", DTR$K_INF_SSC_NAME);
    PyModule_AddIntConstant(m, "DTR_K_INF_SSC_SET", DTR$K_INF_SSC_SET);
    PyModule_AddIntConstant(m, "DTR_K_INF_SSC_NEXT", DTR$K_INF_SSC_NEXT);
    PyModule_AddIntConstant(m, "DTR_K_INF_SET_NAME", DTR$K_INF_SET_NAME);
    PyModule_AddIntConstant(m, "DTR_K_INF_SET_NEXT", DTR$K_INF_SET_NEXT);
    PyModule_AddIntConstant(m, "DTR_K_INF_SET_SDP", DTR$K_INF_SET_SDP);
    PyModule_AddIntConstant(m, "DTR_K_INF_SET_SINGULAR", DTR$K_INF_SET_SINGULAR);
    PyModule_AddIntConstant(m, "DTR_K_INF_SDP_NEXT", DTR$K_INF_SDP_NEXT);
    PyModule_AddIntConstant(m, "DTR_K_INF_SDP_DOMAIN", DTR$K_INF_SDP_DOMAIN);
    PyModule_AddIntConstant(m, "DTR_K_INF_SDP_TENANCY", DTR$K_INF_SDP_TENANCY);
    PyModule_AddIntConstant(m, "DTR_K_INF_SDP_INSERT", DTR$K_INF_SDP_INSERT);
    PyModule_AddIntConstant(m, "DTR_K_INF_SDP_RETAIN", DTR$K_INF_SDP_RETAIN);
    PyModule_AddIntConstant(m, "DTR_K_INF_FLD_QHDR", DTR$K_INF_FLD_QHDR);
    PyModule_AddIntConstant(m, "DTR_K_INF_HDR_CNT", DTR$K_INF_HDR_CNT);
    PyModule_AddIntConstant(m, "DTR_K_INF_HDR_STRING", DTR$K_INF_HDR_STRING);
    PyModule_AddIntConstant(m, "DTR_K_INF_GLV_STA_OBJ", DTR$K_INF_GLV_STA_OBJ);
    PyModule_AddIntConstant(m, "DTR_K_INF_GLV_STA_CNT", DTR$K_INF_GLV_STA_CNT);
    PyModule_AddIntConstant(m, "DTR_K_INF_GLV_STA_LINE", DTR$K_INF_GLV_STA_LINE);
    PyModule_AddIntConstant(m, "DTR_K_INF_PLO_CNT", DTR$K_INF_PLO_CNT);
    PyModule_AddIntConstant(m, "DTR_K_INF_PLO_PAI", DTR$K_INF_PLO_PAI);
    PyModule_AddIntConstant(m, "DTR_K_INF_PAI_PROMPT", DTR$K_INF_PAI_PROMPT);
    PyModule_AddIntConstant(m, "DTR_K_INF_PAI_DTYPE", DTR$K_INF_PAI_DTYPE);
    PyModule_AddIntConstant(m, "DTR_K_INF_DOM_ACCESS_READ", DTR$K_INF_DOM_ACCESS_READ);
    PyModule_AddIntConstant(m, "DTR_K_INF_DOM_ACCESS_WRITE", DTR$K_INF_DOM_ACCESS_WRITE);
    PyModule_AddIntConstant(m, "DTR_K_INF_DOM_ACCESS_MODIFY", DTR$K_INF_DOM_ACCESS_MODIFY);
    PyModule_AddIntConstant(m, "DTR_K_INF_DOM_ACCESS_EXTEND", DTR$K_INF_DOM_ACCESS_EXTEND);
    PyModule_AddIntConstant(m, "DTR_K_INF_DOM_SHARE_EXCLUSIVE", DTR$K_INF_DOM_SHARE_EXCLUSIVE);
    PyModule_AddIntConstant(m, "DTR_K_INF_DOM_SHARE_SHARED", DTR$K_INF_DOM_SHARE_SHARED);
    PyModule_AddIntConstant(m, "DTR_K_INF_DOM_SHARE_PROTECT", DTR$K_INF_DOM_SHARE_PROTECT);
    PyModule_AddIntConstant(m, "DTR_NTS", DTR_NTS);
    return m;
}
