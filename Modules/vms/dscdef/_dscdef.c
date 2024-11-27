#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "structmember.h"

#define __NEW_STARLET 1

#include <descrip.h>
#include <starlet.h>
#include <ssdef.h>
#include <dscdef.h>

PyDoc_STRVAR(doc__dscdef,
"DSC definitions");

/********************************************************************
  Type
*/

#if defined(__VMS) && __INITIAL_POINTER_SIZE == 64
#   define  malloc_low      _malloc32
#   define  realloc_low     _realloc32
#else
#   define  malloc_low      malloc
#   define  realloc_low     realloc
#endif

typedef struct {
    PyObject_HEAD
    char  *buffer;
} DESCObject;

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
DESC_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    DESCObject *self;
    self = (DESCObject *) type->tp_alloc(type, 0);
    if (self != NULL) {
        char           *pvalue = NULL;
        unsigned char   value_buf[32];
        PyObject       *argvalue = NULL;
        int             b_dtype = DSC$K_DTYPE_T;
        int             b_class = DSC$K_CLASS_S;
        Py_ssize_t      size = 128;
        if (args && PyTuple_CheckExact(args)) {
            if (PyTuple_Size(args) == 3) {
                PyArg_ParseTuple(args, "iiO", &b_dtype, &b_class, &argvalue);
            } else {
                PyArg_ParseTuple(args, "|O", &argvalue);
            }
            if (argvalue) {
                if (b_dtype == DSC$K_DTYPE_T) {
                    if (PyUnicode_CheckExact(argvalue)) {
                        pvalue = (char*)PyUnicode_AsUTF8AndSize(argvalue, &size);
                    } else if (PyBytes_CheckExact(argvalue)){
                        PyBytes_AsStringAndSize(argvalue, &pvalue, &size);
                    } else if (PyLong_Check(argvalue)) {
                        size = PyLong_AsSsize_t(argvalue);
                    }
                } else {
                    size = size_from_type(b_dtype);
                    if (PyLong_Check(argvalue)) {
                        if (!_PyLong_AsByteArray(
                                (PyLongObject*)argvalue,
                                value_buf,
                                size,
                                1,
                                sign_from_type(b_dtype)))
                        {
                            pvalue = (char*)value_buf;
                        }
                    }
                }
            }
        }
        self->buffer = malloc_low(size + 1 + sizeof(struct dsc$descriptor));
        if (self->buffer == NULL) {
            Py_DECREF(self);
            return NULL;
        }
        memset(self->buffer, 0, size + 1 + sizeof(struct dsc$descriptor));
        struct dsc$descriptor *pdesc = (struct dsc$descriptor *)self->buffer;
        pdesc->dsc$w_length = size;
        pdesc->dsc$b_dtype = b_dtype;
        pdesc->dsc$b_class = b_class;
        pdesc->dsc$a_pointer = self->buffer + sizeof(struct dsc$descriptor);
        if (pvalue) {
            memcpy(pdesc->dsc$a_pointer, pvalue, size);
        }
    }
    return (PyObject *) self;
}

static void DESC_dealloc(DESCObject * self)
{
    if (self->buffer != NULL) {
        free(self->buffer);
        self->buffer = NULL;
    }
    PyObject_Del(self);
}

static PyObject *DECC_as_tuple(DESCObject *self) {
    struct dsc$descriptor *pdesc = (struct dsc$descriptor *)self->buffer;
    PyObject *pValue = NULL;
    if (pdesc->dsc$b_dtype == DSC$K_DTYPE_T) {
        pValue = PyUnicode_FromStringAndSize(pdesc->dsc$a_pointer, pdesc->dsc$w_length);
    } else {
        pValue = _PyLong_FromByteArray((unsigned char*)pdesc->dsc$a_pointer, pdesc->dsc$w_length, 1, sign_from_type(pdesc->dsc$b_dtype));
    }
    return Py_BuildValue("(i,i,i,O)", pdesc->dsc$w_length, pdesc->dsc$b_dtype, pdesc->dsc$b_class, pValue);
}

static PyObject *DESC_str(DESCObject *self) {
    struct dsc$descriptor *pdesc = (struct dsc$descriptor *)self->buffer;
    if (pdesc->dsc$b_dtype == DSC$K_DTYPE_T) {
        return PyUnicode_FromStringAndSize(pdesc->dsc$a_pointer, pdesc->dsc$w_length);
    }
    long long value = 0;
    switch(size_from_type(pdesc->dsc$b_dtype)) {
        default:
            return PyUnicode_FromFormat("vms descriptor (len: %i, type %i, class: %i)", size_from_type(pdesc->dsc$b_dtype), pdesc->dsc$b_dtype, pdesc->dsc$b_class);
        case 1:
            value = *(char*)pdesc->dsc$a_pointer;
            break;
        case 2:
            value = *(short*)pdesc->dsc$a_pointer;
            break;
        case 4:
            value = *(int*)pdesc->dsc$a_pointer;
            break;
        case 8:
            value = *(long long*)pdesc->dsc$a_pointer;
            break;
    }
    return PyUnicode_FromFormat("vms descriptor (len: %i, type %i, class: %i, value: %lli)", size_from_type(pdesc->dsc$b_dtype), pdesc->dsc$b_dtype, pdesc->dsc$b_class, value);
}

static PyMethodDef DESC_methods[] = {
    {"as_tuple", (PyCFunction) DECC_as_tuple, METH_NOARGS,
        PyDoc_STR("as_tuple()   Returns tuple(len, type, class, object_value)")},
    {NULL, NULL}
};

static PyMemberDef DESC_members[] = {
    {"buffer", T_LONG, offsetof(DESCObject, buffer), READONLY,
     "Address of the descriptor, 32-bit long"},
    {NULL}
};

PyTypeObject DESC_Type = {
    PyObject_HEAD_INIT(NULL)
    "_dscdef.descriptor",
    .tp_basicsize = sizeof(DESCObject),
    .tp_dealloc = (destructor) DESC_dealloc,
    .tp_getattro = PyObject_GenericGetAttr,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = DESC_new,
    .tp_methods = DESC_methods,
    .tp_members = DESC_members,
    .tp_str = (reprfunc) DESC_str,
};

/********************************************************************
  Module
*/

static struct PyModuleDef _dscdef_module = {
    PyModuleDef_HEAD_INIT,
    "_dscdef",
    doc__dscdef,
    -1,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit__dscdef(void) {
    if (PyType_Ready(&DESC_Type) < 0) {
        return NULL;
    }
    PyObject *m = PyModule_Create(&_dscdef_module);
    if (m == NULL) {
        return NULL;
    }

    Py_INCREF(&DESC_Type);
    if (PyModule_AddObject(m, "descriptor", (PyObject *) &DESC_Type) < 0) {
        Py_DECREF(&DESC_Type);
        Py_DECREF(m);
        return NULL;
    }

    PyModule_AddIntConstant(m, "DSC_K_DTYPE_Z", 0);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_Z", 0);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_BU", 2);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_BU", 2);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_WU", 3);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_WU", 3);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_LU", 4);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_LU", 4);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_QU", 5);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_QU", 5);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_OU", 25);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_OU", 25);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_B", 6);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_B", 6);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_W", 7);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_W", 7);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_L", 8);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_L", 8);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_Q", 9);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_Q", 9);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_O", 26);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_O", 26);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_F", 10);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_F", 10);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_D", 11);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_D", 11);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_G", 27);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_G", 27);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_H", 28);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_H", 28);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_FC", 12);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_FC", 12);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_DC", 13);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_DC", 13);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_GC", 29);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_GC", 29);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_HC", 30);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_HC", 30);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_FS", 52);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_FS", 52);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_FT", 53);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_FT", 53);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_FSC", 54);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_FSC", 54);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_FTC", 55);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_FTC", 55);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_FX", 57);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_FX", 57);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_FXC", 58);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_FXC", 58);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_CIT", 31);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_CIT", 31);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_T", 14);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_T", 14);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_VT", 37);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_VT", 37);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_T2", 38);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_T2", 38);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_NU", 15);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_NU", 15);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_NL", 16);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_NL", 16);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_NLO", 17);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_NLO", 17);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_NR", 18);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_NR", 18);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_NRO", 19);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_NRO", 19);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_NZ", 20);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_NZ", 20);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_P", 21);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_P", 21);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_V", 1);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_V", 1);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_VU", 34);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_VU", 34);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_ZI", 22);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_ZI", 22);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_ZEM", 23);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_ZEM", 23);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_DSC", 24);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_DSC", 24);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_BPV", 32);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_BPV", 32);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_BLV", 33);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_BLV", 33);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_ADT", 35);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_ADT", 35);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_CAD", 178);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_CAD", 178);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_ENT", 179);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_ENT", 179);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_GBL", 180);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_GBL", 180);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_EPT", 181);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_EPT", 181);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_R11", 182);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_R11", 182);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_FLD", 183);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_FLD", 183);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_PCT", 184);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_PCT", 184);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_DPC", 185);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_DPC", 185);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_LBL", 186);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_LBL", 186);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_SLB", 187);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_SLB", 187);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_MOD", 188);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_MOD", 188);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_EOM", 189);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_EOM", 189);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_RTN", 190);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_RTN", 190);
    PyModule_AddIntConstant(m, "DSC_K_DTYPE_EOR", 191);
    PyModule_AddIntConstant(m, "DSC64_K_DTYPE_EOR", 191);
    PyModule_AddIntConstant(m, "DSC_K_CLASS_Z", 0);
    PyModule_AddIntConstant(m, "DSC64_K_CLASS_Z", 0);
    PyModule_AddIntConstant(m, "DSC_K_CLASS_S", 1);
    PyModule_AddIntConstant(m, "DSC64_K_CLASS_S", 1);
    PyModule_AddIntConstant(m, "DSC_K_CLASS_D", 2);
    PyModule_AddIntConstant(m, "DSC64_K_CLASS_D", 2);
    PyModule_AddIntConstant(m, "DSC_K_CLASS_V", 3);
    PyModule_AddIntConstant(m, "DSC64_K_CLASS_V", 3);
    PyModule_AddIntConstant(m, "DSC_K_CLASS_A", 4);
    PyModule_AddIntConstant(m, "DSC64_K_CLASS_A", 4);
    PyModule_AddIntConstant(m, "DSC_K_CLASS_P", 5);
    PyModule_AddIntConstant(m, "DSC64_K_CLASS_P", 5);
    PyModule_AddIntConstant(m, "DSC_K_CLASS_PI", 6);
    PyModule_AddIntConstant(m, "DSC64_K_CLASS_PI", 6);
    PyModule_AddIntConstant(m, "DSC_K_CLASS_J", 7);
    PyModule_AddIntConstant(m, "DSC64_K_CLASS_J", 7);
    PyModule_AddIntConstant(m, "DSC_K_CLASS_JI", 8);
    PyModule_AddIntConstant(m, "DSC_K_CLASS_SD", 9);
    PyModule_AddIntConstant(m, "DSC64_K_CLASS_SD", 9);
    PyModule_AddIntConstant(m, "DSC_K_CLASS_NCA", 10);
    PyModule_AddIntConstant(m, "DSC64_K_CLASS_NCA", 10);
    PyModule_AddIntConstant(m, "DSC_K_CLASS_VS", 11);
    PyModule_AddIntConstant(m, "DSC64_K_CLASS_VS", 11);
    PyModule_AddIntConstant(m, "DSC_K_CLASS_VSA", 12);
    PyModule_AddIntConstant(m, "DSC64_K_CLASS_VSA", 12);
    PyModule_AddIntConstant(m, "DSC_K_CLASS_UBS", 13);
    PyModule_AddIntConstant(m, "DSC64_K_CLASS_UBS", 13);
    PyModule_AddIntConstant(m, "DSC_K_CLASS_UBA", 14);
    PyModule_AddIntConstant(m, "DSC64_K_CLASS_UBA", 14);
    PyModule_AddIntConstant(m, "DSC_K_CLASS_SB", 15);
    PyModule_AddIntConstant(m, "DSC64_K_CLASS_SB", 15);
    PyModule_AddIntConstant(m, "DSC_K_CLASS_UBSB", 16);
    PyModule_AddIntConstant(m, "DSC64_K_CLASS_UBSB", 16);
    PyModule_AddIntConstant(m, "DSC_K_CLASS_BFA", 191);
    PyModule_AddIntConstant(m, "DSC_S_DSCDEF", 14);
    PyModule_AddIntConstant(m, "DSC_S_DSCDEF1", 8);
    PyModule_AddIntConstant(m, "DSC_K_Z_BLN", 8);
    PyModule_AddIntConstant(m, "DSC_C_Z_BLN", 8);
    PyModule_AddIntConstant(m, "DSC_K_S_BLN", 8);
    PyModule_AddIntConstant(m, "DSC_C_S_BLN", 8);
    PyModule_AddIntConstant(m, "DSC_K_D_BLN", 8);
    PyModule_AddIntConstant(m, "DSC_C_D_BLN", 8);
    PyModule_AddIntConstant(m, "DSC_K_P_BLN", 8);
    PyModule_AddIntConstant(m, "DSC_C_P_BLN", 8);
    PyModule_AddIntConstant(m, "DSC_K_J_BLN", 8);
    PyModule_AddIntConstant(m, "DSC_C_J_BLN", 8);
    PyModule_AddIntConstant(m, "DSC_K_VS_BLN", 8);
    PyModule_AddIntConstant(m, "DSC_C_VS_BLN", 8);
    PyModule_AddIntConstant(m, "DSC_S_DSCDEF2", 8);
    PyModule_AddIntConstant(m, "DSC_K_UBS_BLN", 12);
    PyModule_AddIntConstant(m, "DSC_C_UBS_BLN", 12);
    PyModule_AddIntConstant(m, "DSC_S_DSCDEF3", 12);
    PyModule_AddIntConstant(m, "DSC_S_DSCDEF4", 12);
    PyModule_AddIntConstant(m, "DSC_K_SD_BLN", 12);
    PyModule_AddIntConstant(m, "DSC_C_SD_BLN", 12);
    PyModule_AddIntConstant(m, "DSC_S_DSCDEF5", 20);
    PyModule_AddIntConstant(m, "DSC_S_DSCDEF6", 28);
    PyModule_AddIntConstant(m, "DSC_S_DSCDEF7", 28);
    PyModule_AddIntConstant(m, "DSC_K_PI_BLN", 12);
    PyModule_AddIntConstant(m, "DSC_C_PI_BLN", 12);
    PyModule_AddIntConstant(m, "DSC_K_JI_BLN", 12);
    PyModule_AddIntConstant(m, "DSC_C_JI_BLN", 12);
    PyModule_AddIntConstant(m, "DSC_S_DSCDEF8", 12);
    PyModule_AddIntConstant(m, "DSC_S_DSCDEF9", 16);
    PyModule_AddIntConstant(m, "DSC_S_DSCDEF10", 20);
    PyModule_AddIntConstant(m, "DSC64_S_DSCDEF64", 16);
    PyModule_AddIntConstant(m, "DSC64_S_LENGTH", 8);
    PyModule_AddIntConstant(m, "DSC64_S_DSCDEF1_64", 24);
    PyModule_AddIntConstant(m, "DSC64_S_MAXSTRLEN", 8);
    PyModule_AddIntConstant(m, "DSC64_S_POINTER", 8);
    PyModule_AddIntConstant(m, "DSC64_K_Z_BLN", 24);
    PyModule_AddIntConstant(m, "DSC64_C_Z_BLN", 24);
    PyModule_AddIntConstant(m, "DSC64_K_S_BLN", 24);
    PyModule_AddIntConstant(m, "DSC64_C_S_BLN", 24);
    PyModule_AddIntConstant(m, "DSC64_K_D_BLN", 24);
    PyModule_AddIntConstant(m, "DSC64_C_D_BLN", 24);
    PyModule_AddIntConstant(m, "DSC64_K_P_BLN", 24);
    PyModule_AddIntConstant(m, "DSC64_C_P_BLN", 24);
    PyModule_AddIntConstant(m, "DSC64_K_J_BLN", 24);
    PyModule_AddIntConstant(m, "DSC64_C_J_BLN", 24);
    PyModule_AddIntConstant(m, "DSC64_K_VS_BLN", 24);
    PyModule_AddIntConstant(m, "DSC64_C_VS_BLN", 24);
    PyModule_AddIntConstant(m, "DSC64_S_DSCDEF2_64", 24);
    PyModule_AddIntConstant(m, "DSC64_S_BASE", 8);
    PyModule_AddIntConstant(m, "DSC64_K_UBS_BLN", 32);
    PyModule_AddIntConstant(m, "DSC64_C_UBS_BLN", 32);
    PyModule_AddIntConstant(m, "DSC64_S_DSCDEF3_64", 32);
    PyModule_AddIntConstant(m, "DSC64_S_POS", 8);
    PyModule_AddIntConstant(m, "DSC64_K_SD_BLN", 32);
    PyModule_AddIntConstant(m, "DSC64_C_SD_BLN", 32);
    PyModule_AddIntConstant(m, "DSC64_S_DSCDEF5_64", 48);
    PyModule_AddIntConstant(m, "DSC64_S_ARSIZE", 8);
    PyModule_AddIntConstant(m, "DSC64_S_A0", 8);
    PyModule_AddIntConstant(m, "DSC64_S_DSCDEF6_64", 64);
    PyModule_AddIntConstant(m, "DSC64_S_V0", 8);
    PyModule_AddIntConstant(m, "DSC64_S_S1", 8);
    PyModule_AddIntConstant(m, "DSC64_S_S2", 8);
    PyModule_AddIntConstant(m, "DSC64_S_DSCDEF7_64", 64);
    PyModule_AddIntConstant(m, "DSC64_S_M1", 8);
    PyModule_AddIntConstant(m, "DSC64_S_M2", 8);
    PyModule_AddIntConstant(m, "DSC64_K_PI_BLN", 32);
    PyModule_AddIntConstant(m, "DSC64_C_PI_BLN", 32);
    PyModule_AddIntConstant(m, "DSC64_K_JI_BLN", 32);
    PyModule_AddIntConstant(m, "DSC64_C_JI_BLN", 32);
    PyModule_AddIntConstant(m, "DSC64_S_DSCDEF8_64", 32);
    PyModule_AddIntConstant(m, "DSC64_S_FRAME", 8);
    PyModule_AddIntConstant(m, "DSC64_S_DSCDEF9_64", 40);
    PyModule_AddIntConstant(m, "DSC64_S_SB_L1", 8);
    PyModule_AddIntConstant(m, "DSC64_S_SB_U1", 8);
    PyModule_AddIntConstant(m, "DSC64_S_DSCDEF10_64", 48);
    PyModule_AddIntConstant(m, "DSC64_S_UBSB_L1", 8);
    PyModule_AddIntConstant(m, "DSC64_S_UBSB_U1", 8);
    return m;
}