#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <Python.h>
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>

#include "G722_numpy_api.h"

typedef struct {
    PyObject_HEAD
    void *data;
} PyDataOwner;

static bool
is_i16_buffer_format(const char *format)
{
    if (format == NULL) {
        return false;
    }
    return strcmp(format, "h") == 0
        || strcmp(format, "=h") == 0
        || strcmp(format, "<h") == 0
        || strcmp(format, ">h") == 0;
}

static void
DataOwner_dealloc(PyDataOwner *self)
{
    free(self->data);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyTypeObject PyDataOwnerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "G722_numpy.DataOwner",
    .tp_basicsize = sizeof(PyDataOwner),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_dealloc = (destructor)DataOwner_dealloc,
};

static int
api_check_int16_1d(PyObject *item, int16_t **array, Py_ssize_t *length)
{
    PyArrayObject *arr;

    if (!PyArray_Check(item)) {
        return 0;
    }
    arr = (PyArrayObject *)item;
    if (PyArray_TYPE(arr) != NPY_INT16) {
        return 0;
    }
    if (PyArray_NDIM(arr) != 1) {
        return 0;
    }
    if (!PyArray_ISCARRAY_RO(arr)) {
        return 0;
    }
    *array = (int16_t *)PyArray_DATA(arr);
    *length = (Py_ssize_t)PyArray_SIZE(arr);
    return 1;
}

static PyObject *
api_from_pcm16_owned(int16_t *array, Py_ssize_t length)
{
    PyDataOwner *owner;
    npy_intp dims[1];
    PyObject *numpy_array;

    owner = PyObject_New(PyDataOwner, &PyDataOwnerType);
    if (owner == NULL) {
        free(array);
        return PyErr_NoMemory();
    }
    owner->data = array;

    dims[0] = (npy_intp)length;
    numpy_array = PyArray_SimpleNewFromData(1, dims, NPY_INT16, (void *)array);
    if (numpy_array == NULL) {
        Py_DECREF(owner);
        return NULL;
    }
    if (PyArray_SetBaseObject((PyArrayObject *)numpy_array, (PyObject *)owner) < 0) {
        Py_DECREF(owner);
        Py_DECREF(numpy_array);
        return NULL;
    }
    return numpy_array;
}

static PyObject *
py_pcm16_to_numpy(PyObject *self, PyObject *args)
{
    PyObject *item;
    Py_buffer view;
    npy_intp dims[1];
    PyObject *numpy_array;

    (void)self;
    if (!PyArg_ParseTuple(args, "O", &item)) {
        return NULL;
    }
    if (PyArray_Check(item)) {
        PyArrayObject *arr = (PyArrayObject *)item;
        if (PyArray_TYPE(arr) == NPY_INT16 && PyArray_NDIM(arr) == 1 && PyArray_ISCARRAY_RO(arr)) {
            Py_INCREF(item);
            return item;
        }
    }

    if (!PyObject_CheckBuffer(item) ||
        PyObject_GetBuffer(item, &view, PyBUF_CONTIG_RO | PyBUF_FORMAT) < 0) {
        PyErr_SetString(PyExc_TypeError, "Expected a contiguous 1-D 16-bit buffer");
        return NULL;
    }
    if (view.ndim != 1 || view.itemsize != (Py_ssize_t)sizeof(int16_t) ||
        !is_i16_buffer_format(view.format) ||
        view.len % sizeof(int16_t) != 0) {
        PyBuffer_Release(&view);
        PyErr_SetString(PyExc_TypeError, "Expected a contiguous 1-D 16-bit buffer");
        return NULL;
    }

    dims[0] = (npy_intp)(view.len / sizeof(int16_t));
    numpy_array = PyArray_SimpleNewFromData(1, dims, NPY_INT16, view.buf);
    if (numpy_array == NULL) {
        PyBuffer_Release(&view);
        return NULL;
    }
    Py_INCREF(item);
    if (PyArray_SetBaseObject((PyArrayObject *)numpy_array, item) < 0) {
        Py_DECREF(item);
        Py_DECREF(numpy_array);
        PyBuffer_Release(&view);
        return NULL;
    }
    PyBuffer_Release(&view);
    return numpy_array;
}

static PyMethodDef G722NumpyMethods[] = {
    {"pcm16_to_numpy", py_pcm16_to_numpy, METH_VARARGS, "Convert PCM16 buffer to a NumPy array."},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef G722NumpyModule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "G722_numpy",
    .m_doc = "Optional NumPy backend for G722.",
    .m_size = -1,
    .m_methods = G722NumpyMethods,
};

PyMODINIT_FUNC
PyInit_G722_numpy(void)
{
    PyObject *module;
    PyObject *capsule;
    static G722NumpyAPI api = {
        .check_int16_1d = api_check_int16_1d,
        .from_pcm16_owned = api_from_pcm16_owned,
    };

    import_array();
    if (PyErr_Occurred()) {
        return NULL;
    }
    if (PyType_Ready(&PyDataOwnerType) < 0) {
        return NULL;
    }
    module = PyModule_Create(&G722NumpyModule);
    if (module == NULL) {
        return NULL;
    }

    capsule = PyCapsule_New((void *)&api, G722_NUMPY_CAPSULE_NAME, NULL);
    if (capsule == NULL) {
        Py_DECREF(module);
        return NULL;
    }
    if (PyModule_AddObject(module, "_C_API", capsule) < 0) {
        Py_DECREF(capsule);
        Py_DECREF(module);
        return NULL;
    }

    return module;
}
