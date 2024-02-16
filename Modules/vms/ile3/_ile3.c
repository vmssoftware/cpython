#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "structmember.h"

#define __NEW_STARLET 1

#include <descrip.h>
#include <starlet.h>
#include <ssdef.h>
#include <iledef.h>
#include <alloca.h>

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#include "_ile3.h"

#if defined(__VMS) && __INITIAL_POINTER_SIZE == 64
#   define  malloc_low      _malloc32
#   define  realloc_low     _realloc32
#else
#   define  malloc_low      malloc
#   define  realloc_low     realloc
#endif

static void ILE3_dealloc(ILE3Object * self)
{
    if (self->ptypes != NULL) {
        free(self->ptypes);
        self->ptypes = NULL;
    }
    if (self->plist != NULL) {
        ILE3 *ptr = (ILE3*)self->plist;
        while(self->size) {
            if (ptr->ile3$ps_bufaddr) {
                free(ptr->ile3$ps_bufaddr);
            }
            if (ptr->ile3$ps_retlen_addr) {
                free(ptr->ile3$ps_retlen_addr);
            }
            ++ptr;
            --self->size;
        }
        free(self->plist);
        self->plist = NULL;
    }
    PyObject_Del(self);
}

static void init_item(ILE3 *item) {
    item->ile3$w_length = 0;
    item->ile3$w_code = 0;
    item->ile3$ps_bufaddr = NULL;
    item->ile3$ps_retlen_addr = NULL;
}

static int size_from_type(int type) {
    switch (type) {
        case DSC$K_DTYPE_Z:
            return 0;           /* unspecified */
        case DSC$K_DTYPE_BU:
            return 1;           /* byte (unsigned);  8-bit unsigned quantity */
        case DSC$K_DTYPE_WU:
            return 2;           /* word (unsigned);  16-bit unsigned quantity */
        case DSC$K_DTYPE_LU:
            return 4;           /* longword (unsigned);  32-bit unsigned quantity */
        case DSC$K_DTYPE_QU:
            return 8;           /* quadword (unsigned);  64-bit unsigned quantity */
        case DSC$K_DTYPE_OU:
            return 16;          /* octaword (unsigned);  128-bit unsigned quantity */
        case DSC$K_DTYPE_B:
            return 1;           /* byte integer (signed);  8-bit signed 2's-complement integer */
        case DSC$K_DTYPE_W:
            return 2;           /* word integer (signed);  16-bit signed 2's-complement integer */
        case DSC$K_DTYPE_L:
            return 4;           /* longword integer (signed);  32-bit signed 2's-complement integer */
        case DSC$K_DTYPE_Q:
            return 8;           /* quadword integer (signed);  64-bit signed 2's-complement integer */
        case DSC$K_DTYPE_O:
            return 16;          /* octaword integer (signed);  128-bit signed 2's-complement integer */
        case DSC$K_DTYPE_F:
            return 4;           /* F_floating;  32-bit single-precision floating point */
        case DSC$K_DTYPE_D:
            return 8;           /* D_floating;  64-bit double-precision floating point */
        case DSC$K_DTYPE_G:
            return 8;           /* G_floating;  64-bit double-precision floating point */
        case DSC$K_DTYPE_H:
            return 16;          /* H_floating;  128-bit quadruple-precision floating point */
        case DSC$K_DTYPE_FC:
            return 4*2;         /* F_floating complex */
        case DSC$K_DTYPE_DC:
            return 8*2;         /* D_floating complex */
        case DSC$K_DTYPE_GC:
            return 8*2;         /* G_floating complex */
        case DSC$K_DTYPE_HC:
            return 16*2;        /* H_floating complex */
        case DSC$K_DTYPE_CIT:
            return 0;           /* COBOL Intermediate Temporary */
        default:
            return 0;
    }
}

static int sign_from_type(int type) {
    switch (type) {
        case DSC$K_DTYPE_B:
        case DSC$K_DTYPE_W:
        case DSC$K_DTYPE_L:
        case DSC$K_DTYPE_Q:
        case DSC$K_DTYPE_O:
            return 1;
        default:
            return 0;
    }
}

static PyObject *
ILE3_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    ILE3Object *self;
    self = (ILE3Object *) type->tp_alloc(type, 0);
    if (self != NULL) {
        self->allocated = 1;
        self->size = 0;
        self->pos = -1;
        self->plist = malloc_low(1 * sizeof(ILE3));
        if (self->plist == NULL) {
            Py_DECREF(self);
            return NULL;
        }
        self->ptypes = malloc_low(1 * sizeof(long));
        if (self->ptypes == NULL) {
            free(self->plist);
            self->plist = NULL;
            Py_DECREF(self);
            return NULL;
        }
        init_item((ILE3*)self->plist);
    }
    return (PyObject *) self;
}

static PyObject *ILE3_iter(ILE3Object *self)
{
    Py_INCREF(self);
    self->pos = 0;
    return (PyObject *)self;
}

static PyObject *
_value_from_item(
    ILE3 *item,
    long type)
{
    long size = 0;
    if (type == DSC$K_DTYPE_T) {
        size = *(item->ile3$ps_retlen_addr);
        return PyUnicode_FromStringAndSize((char*)item->ile3$ps_bufaddr, size);
    } else if (type == DSC$K_DTYPE_VT) {
        size = *(unsigned char*)item->ile3$ps_bufaddr;
        return PyUnicode_FromStringAndSize(((char*)item->ile3$ps_bufaddr)+1, size);
    }
    size = size_from_type(type);
    if (size == 0) {
        PyErr_Format(PyExc_ValueError, "Unknown type %i", type);
        return NULL;
    }
    return _PyLong_FromByteArray(
        (unsigned char*)item->ile3$ps_bufaddr,
        size,
        1,
        sign_from_type(type));
}

static PyObject *
ILE3_getat_c(
    ILE3Object *self,
    long pos)
{
    if ((unsigned int)pos < self->size) {
        ILE3 *item = (ILE3*)self->plist + pos;
        long type = *((long*)self->ptypes + pos);
        PyObject *pValue = _value_from_item(item, type);
        if (pValue) {
            return Py_BuildValue("(H,i,O)", item->ile3$w_code, type, pValue);
        }
    } else {
        PyErr_SetNone(PyExc_IndexError);
    }
    return NULL;
}

static PyObject *ILE3_getat(ILE3Object *self, PyObject *args) {
    if (!PyLong_Check(args)) {
        _PyArg_BadArgument("getat", "args", "long", args);
        return NULL;
    }
    long pos = PyLong_AsLong(args);
    return ILE3_getat_c(self, pos);
}

static PyObject *ILE3_getat_bytes(ILE3Object *self, PyObject *args) {
    if (!PyLong_Check(args)) {
        _PyArg_BadArgument("getat", "args", "long", args);
        return NULL;
    }
    long pos = PyLong_AsLong(args);
    if ((unsigned int)pos < self->size) {
        ILE3 *item = (ILE3*)self->plist + pos;
        long type = *((long*)self->ptypes + pos);
        long size = *(item->ile3$ps_retlen_addr);
        PyObject *pValue = PyBytes_FromStringAndSize((char*)item->ile3$ps_bufaddr, size);
        if (pValue) {
            return Py_BuildValue("(H,i,O)", item->ile3$w_code, type, pValue);
        }
    } else {
        PyErr_SetNone(PyExc_IndexError);
    }
    return NULL;
}

static PyObject *ILE3_iternext(ILE3Object *self)
{
    PyObject *pResult = ILE3_getat_c(self, self->pos);
    if (pResult != NULL) {
        ++self->pos;
        return pResult;
    }
    if (PyErr_Occurred() && PyErr_ExceptionMatches(PyExc_IndexError)) {
        PyErr_Clear();
    }
    self->pos = -1;
    return NULL;
}

static int ILE3_increment(ILE3Object *self) {
    ++self->size;
    if (self->size >= self->allocated) {
        self->allocated *= 2;
        self->plist = realloc_low(self->plist, self->allocated * sizeof(ILE3));
        self->ptypes = realloc_low(self->ptypes, self->allocated * sizeof(long));
    }
    if (self->plist && self->ptypes) {
        init_item((ILE3*)self->plist + self->size);
        return 0;
    }
    PyErr_SetNone(PyExc_MemoryError);
    return -1;
}

static int ILE3_decrement(ILE3Object *self) {
    if (self->size > 0) {
        --self->size;
        ILE3 *ptr = (ILE3*)self->plist + self->size;
        if (ptr->ile3$ps_bufaddr) {
            free(ptr->ile3$ps_bufaddr);
            ptr->ile3$ps_bufaddr = NULL;
        }
        if (ptr->ile3$ps_retlen_addr) {
            free(ptr->ile3$ps_retlen_addr);
            ptr->ile3$ps_retlen_addr = NULL;
        }
        return 0;
    }
    return -1;
}

// append(code, type, ?value)
static PyObject*
ILE3_append(
    ILE3Object *self,
    PyObject *const *args,
    Py_ssize_t nargs)
{
    if (!_PyArg_CheckPositional("append", nargs, 2, 3)) {
        return NULL;
    }

    if (!PyLong_Check(args[0])) {
        _PyArg_BadArgument("append", "args[0]", "long", args[0]);
        return NULL;
    }

    if (ILE3_increment(self)) {
        return NULL;
    }

    ILE3 *item = (ILE3*)self->plist + (self->size - 1);

    item->ile3$w_code = PyLong_AsLong(args[0]);
    item->ile3$ps_retlen_addr = (__unsigned_short_ptr32)malloc_low(sizeof(short));
    if (item->ile3$ps_retlen_addr == NULL) {
        ILE3_decrement(self);
        PyErr_SetNone(PyExc_MemoryError);
        return NULL;
    }

    long type = DSC$K_DTYPE_Z;
    if (!PyLong_Check(args[1])) {
        _PyArg_BadArgument("append", "args[1]", "long", args[1]);
        ILE3_decrement(self);
        return NULL;
    }
    type = PyLong_AsLong(args[1]);

    Py_ssize_t size = 0;
    char *pvalue = NULL;
    long long value = 0;

    if (type == DSC$K_DTYPE_T || type == DSC$K_DTYPE_VT) {
        // try to append text
        if (nargs > 2) {
            if (PyUnicode_CheckExact(args[2])) {
                pvalue = (char*)PyUnicode_AsUTF8AndSize(args[2], &size);
            } else if (PyBytes_CheckExact(args[2])){
                PyBytes_AsStringAndSize(args[2], &pvalue, &size);
            } else if (PyLong_Check(args[2])) {
                size = PyLong_AsSsize_t(args[2]);
            }
        }
        if (size <= 0 || size > 65535) {
            PyErr_Format(PyExc_ValueError, "String size is %i, must be in range [1, 65535] ", size);
            ILE3_decrement(self);
            return NULL;
        }
        if (type == DSC$K_DTYPE_VT && size > 255) {
            PyErr_Format(PyExc_ValueError, "String size is %i, must be in range [1, 255] ", size);
            ILE3_decrement(self);
            return NULL;
        }
    } else {
        // try to append number
        size = size_from_type(type);
        if (size == 0) {
            PyErr_Format(PyExc_ValueError, "Unknown type %i", type);
            ILE3_decrement(self);
            return NULL;
        }
        if (nargs > 2) {
            if (PyLong_Check(args[2])) {
                pvalue = (char*)alloca(size);
                if (_PyLong_AsByteArray(
                        (PyLongObject*)args[2],
                        (unsigned char*)pvalue,
                        size,
                        1,
                        sign_from_type(type)))
                {
                    ILE3_decrement(self);
                    return NULL;
                }
            } else {
                _PyArg_BadArgument("append", "args[2]", "long", args[2]);
                ILE3_decrement(self);
                return NULL;
            }
        }
    }

    *item->ile3$ps_retlen_addr = size;
    item->ile3$w_length = size;
    item->ile3$ps_bufaddr = malloc_low(size + 1);
    if (item->ile3$ps_bufaddr == NULL) {
        PyErr_SetNone(PyExc_MemoryError);
        ILE3_decrement(self);
        return NULL;
    }

    if (type == DSC$K_DTYPE_T) {
        if (pvalue) {
            memcpy(item->ile3$ps_bufaddr, pvalue, size);
        } else {
            memset(item->ile3$ps_bufaddr, ' ', size);
        }
        ((char*)item->ile3$ps_bufaddr)[size] = 0;
    } else if (type == DSC$K_DTYPE_VT) {
        *(unsigned char*)item->ile3$ps_bufaddr = (unsigned char)size;
        if (pvalue) {
            memcpy(((char*)item->ile3$ps_bufaddr) + 1, pvalue, size);
        } else {
            memset(((char*)item->ile3$ps_bufaddr) + 1, ' ', size);
        }
    } else {
        if (pvalue) {
            memcpy(item->ile3$ps_bufaddr, pvalue, size);
        } else {
            memset(item->ile3$ps_bufaddr, 0, size);
        }
    }
    *((long*)self->ptypes + (self->size - 1)) = type;
    Py_RETURN_NONE;
}

static PyObject *
ILE3_item(
    ILE3Object *self,
    Py_ssize_t i)
{
    if ((size_t)i < self->size) {
        ILE3 *item = (ILE3*)self->plist + i;
        long type = *((long*)self->ptypes + i);
        return _value_from_item(item, type);
    }
    PyErr_SetNone(PyExc_IndexError);
    return NULL;
}

// static PyObject *
// ILE3_subscript(
//     ILE3Object *self,
//     PyObject *index)
// {
//     if (PyIndex_Check(index)) {
//         Py_ssize_t i;
//         i = PyNumber_AsSsize_t(index, PyExc_IndexError);
//         if (i == -1 && PyErr_Occurred())
//             return NULL;
//         if (i < 0)
//             i += self->size;
//         return ILE3_item(self, i);
//     }
//     PyErr_Format(PyExc_TypeError,
//                     "list indices must be integers, not %.200s",
//                     Py_TYPE(index)->tp_name);
//     return NULL;
// }

static Py_ssize_t
ILE3_length(ILE3Object *a)
{
    return a->size;
}


/********************************************************************
  Type
*/

static PySequenceMethods ILE3_as_sequence = {
    .sq_length = (lenfunc)ILE3_length,
    .sq_item = (ssizeargfunc)ILE3_item,
};

static PyMethodDef ILE3_methods[] = {
    // void _addstr(void *addr, int code, char *val, unsigned short len)
    {"append", (PyCFunction) ILE3_append, METH_FASTCALL,
        PyDoc_STR("append(code: number, type: number, ?value: (str | number) | len: number)->None   Add item")},
    {"getat", (PyCFunction) ILE3_getat, METH_O,
        PyDoc_STR("getat(i: number)->(code: number, type: number, value: number | str)   Get value at the index")},
    {"getat_bytes", (PyCFunction) ILE3_getat_bytes, METH_O,
        PyDoc_STR("getat_bytes(i: number)->(code: number, type: number, value: bytes)   Get value as bytes at the index")},
    // {"__getitem__", (PyCFunction)ILE3_subscript, METH_O | METH_COEXIST,
    //     PyDoc_STR("x[y] Gets value only")},
    {NULL, NULL}
};

static PyMemberDef ILE3_members[] = {
    {"size", T_INT, offsetof(ILE3Object, size), READONLY,
     "size"},
    {NULL}
};

PyTypeObject ILE3_Type = {
    PyObject_HEAD_INIT(NULL)
    ILE3_MODULE_NAME "." ILE3_TYPE_NAME,
    .tp_basicsize = sizeof(ILE3Object),
    .tp_dealloc = (destructor) ILE3_dealloc,
    .tp_as_sequence = &ILE3_as_sequence,
    .tp_getattro = PyObject_GenericGetAttr,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = ILE3_new,
    .tp_methods = ILE3_methods,
    .tp_members = ILE3_members,
    .tp_iter = (getiterfunc)ILE3_iter,
    .tp_iternext = (iternextfunc)ILE3_iternext,
};

/********************************************************************
  Module
*/

static struct PyModuleDef _module_definition = {
    PyModuleDef_HEAD_INIT,
    .m_name = ILE3_MODULE_NAME,
    .m_doc = "OpenVMS ILE3 implementation.",
    .m_size = -1,
};

PyMODINIT_FUNC PyInit__ile3(void)
{
    #define my_offset(a,b) (((unsigned long long)&a)-((unsigned long long)&b))

    // printf("my_offset(tp_name, ILE3_Type) %lli = %s\n", my_offset(ILE3_Type.tp_name, ILE3_Type), ILE3_Type.tp_name);
    // printf("my_offset(tp_base, ILE3_Type) %lli = %llx\n", my_offset(ILE3_Type.tp_base, ILE3_Type), ILE3_Type.tp_base);

    if (PyType_Ready(&ILE3_Type) < 0) {
        return NULL;
    }
    PyObject *m = PyModule_Create(&_module_definition);
    if (m == NULL) {
        return NULL;
    }
    Py_INCREF(&ILE3_Type);
    if (PyModule_AddObject(m, ILE3_TYPE_NAME, (PyObject *) &ILE3_Type) < 0) {
        Py_DECREF(&ILE3_Type);
        Py_DECREF(m);
        return NULL;
    }
    return m;
}
