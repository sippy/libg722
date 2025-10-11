/*
 * Copyright (c) 2014-2025 Sippy Software, Inc., http://www.sippysoft.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <assert.h>
#include <stdbool.h>

#include <Python.h>
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>

#include "g722_encoder.h"
#include "g722_decoder.h"

#define MODULE_BASENAME G722

#define CONCATENATE_DETAIL(x, y) x##y
#define CONCATENATE(x, y) CONCATENATE_DETAIL(x, y)

#if !defined(DEBUG_MOD)
#define MODULE_NAME MODULE_BASENAME
#else
#define MODULE_NAME CONCATENATE(MODULE_BASENAME, _debug)
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define MODULE_NAME_STR TOSTRING(MODULE_NAME)
#define PY_INIT_FUNC CONCATENATE(PyInit_, MODULE_NAME)

typedef struct {
    PyObject_HEAD
    G722_DEC_CTX *g722_dctx;
    G722_ENC_CTX *g722_ectx;
    int sample_rate;
    int bit_rate;
} PyG722;

static int PyG722_init(PyG722* self, PyObject* args, PyObject* kwds) {
    int sample_rate, bit_rate, options;
    static char *kwlist[] = {"sample_rate", "bit_rate", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "ii", kwlist, &sample_rate, &bit_rate)) {
        return -1;
    }

    if (sample_rate != 8000 && sample_rate != 16000) {
        PyErr_SetString(PyExc_ValueError, "Sample rate must be 8000 or 16000");
        return -1;
    }

    if (bit_rate != 48000 && bit_rate != 56000 && bit_rate != 64000) {
        PyErr_SetString(PyExc_ValueError, "Bit rate must be 48000, 56000 or 64000");
        return -1;
    }
    options = (sample_rate == 8000) ? G722_SAMPLE_RATE_8000 : G722_DEFAULT;
    self->g722_ectx = g722_encoder_new(bit_rate, options);
    if(self->g722_ectx == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Error initializing G.722 encoder");
        return -1;
    }
    self->g722_dctx = g722_decoder_new(bit_rate, options);
    if(self->g722_dctx == NULL) {
        g722_encoder_destroy(self->g722_ectx);
        PyErr_SetString(PyExc_RuntimeError, "Error initializing G.722 decoder");
        return -1;
    }
    self->sample_rate = sample_rate;
    self->bit_rate = bit_rate;

    return 0;
}

// The __del__ method for PyG722 objects
static void PyG722_dealloc(PyG722* self) {
    g722_encoder_destroy(self->g722_ectx);
    g722_decoder_destroy(self->g722_dctx);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

// The encode method for PyG722 objects
static PyObject *
PyG722_encode(PyG722* self, PyObject* args) {
    PyObject* item;
    PyObject* seq;
    int16_t* array;
    Py_ssize_t length, i, olength;

    PyObject *rval = NULL;
    if (!PyArg_ParseTuple(args, "O", &item)) {
        PyErr_SetString(PyExc_TypeError, "Takes exactly one argument");
        goto e0;
    }

    if (PyArray_Check(item) && PyArray_TYPE((PyArrayObject*)item) == NPY_INT16) {
        array = (int16_t *)PyArray_DATA((PyArrayObject*)item);
        length = PyArray_SIZE((PyArrayObject*)item);
    } else {
        // Convert PyObject to a sequence if possible
        seq = PySequence_Fast(item, "Expected a sequence");
        if (seq == NULL) {
            PyErr_SetString(PyExc_TypeError, "Expected a sequence");
            goto e0;
        }

        // Get the length of the sequence
        length = PySequence_Size(seq);
        if (length == -1) {
            PyErr_SetString(PyExc_TypeError, "Error getting sequence length");
            goto e1;
        }

        // Allocate memory for the int array
        array = (int16_t*) malloc(length * sizeof(array[0]));
        if (!array) {
            rval = PyErr_NoMemory();
            goto e1;
        }
        for (i = 0; i < length; i++) {
            PyObject* temp_item = PySequence_Fast_GET_ITEM(seq, i);  // Borrowed reference, no need to Py_DECREF
            long tv = PyLong_AsLong(temp_item);
            if (PyErr_Occurred()) {
                goto e2;
            }
            if (tv < -32768 || tv > 32767) {
                PyErr_SetString(PyExc_ValueError, "Value out of range");
                goto e2;
            }
            array[i] = (int16_t)tv;
        }
    }
    olength = self->sample_rate == 8000 ? length : length / 2;
    PyObject *obuf_obj = PyBytes_FromStringAndSize(NULL, olength);
    if (obuf_obj == NULL) {
        rval = PyErr_NoMemory();
        goto e2;
    }
    uint8_t *buffer = (uint8_t *)PyBytes_AsString(obuf_obj);
    if (!buffer) {
        goto e3;
    }
    int obytes = g722_encode(self->g722_ectx, array, length, buffer);
    assert(obytes == olength);
    rval = obuf_obj;
    goto e2;
e3:
    Py_DECREF(obuf_obj);
e2:
    if (!PyArray_Check(item)) {
        free(array);
    }
e1:
    if (!PyArray_Check(item)) {
        Py_DECREF(seq);
    }
e0:
    return rval;
}

typedef struct {
    PyObject_HEAD
    void *data;  // Pointer to the data buffer
} PyDataOwner;

static void
DataOwner_dealloc(PyDataOwner* self) {
    free(self->data);  // Free the memory when the object is deallocated
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyTypeObject PyDataOwnerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "DataOwner",
    .tp_basicsize = sizeof(PyDataOwner),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_dealloc = (destructor)DataOwner_dealloc,
};

// The get method for PyG722 objects
static PyObject *
PyG722_decode(PyG722* self, PyObject* args) {
    PyObject* item;
    uint8_t* buffer;
    int16_t* array;
    Py_ssize_t length, olength;

    // Parse the input tuple to get a bytes object
    if (!PyArg_ParseTuple(args, "O", &item)) {
        PyErr_SetString(PyExc_TypeError, "Argument must be a bytes object");
        return NULL;
    }

    // Ensure the object is a bytes object
    if (!PyBytes_Check(item)) {
        PyErr_SetString(PyExc_TypeError, "Argument must be a bytes object");
        return NULL;
    }

    // Get the buffer and its length from the bytes object
    buffer = (uint8_t *)PyBytes_AsString(item);
    if (!buffer) {
        return NULL;  // PyErr_SetString is called by PyBytes_AsString if something goes wrong
    }
    length = PyBytes_Size(item);
    if (length < 0) {
        return NULL;  // PyErr_SetString is called by PyBytes_Size if something goes wrong
    }
    olength = self->sample_rate == 8000 ? length : length * 2;
    array = (int16_t*) malloc(olength * sizeof(array[0]));
    if (array == NULL) {
        return PyErr_NoMemory();
    }
    g722_decode(self->g722_dctx, buffer, length, array);

    PyDataOwner* owner = PyObject_New(PyDataOwner, &PyDataOwnerType);
    if (!owner) {
        free(array);
        return PyErr_NoMemory();
    }
    owner->data = array;

    // Create a new numpy array to hold the integers
    npy_intp dims[1] = {olength};
    PyObject* numpy_array = PyArray_SimpleNewFromData(1, dims, NPY_INT16, (void *)array);
    if (numpy_array == NULL) goto e1;
    PyArray_SetBaseObject((PyArrayObject*)numpy_array, (PyObject*)owner);
    return numpy_array;
e1:
    Py_DECREF(owner);
    return NULL;
}

static PyMethodDef PyG722_methods[] = {
    {"encode", (PyCFunction)PyG722_encode, METH_VARARGS, "Encode signed linear PCM samples to G.722 format"},
    {"decode", (PyCFunction)PyG722_decode, METH_VARARGS, "Decode G.722 format to signed linear PCM samples"},
    {NULL}  // Sentinel
};

static PyTypeObject PyG722Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = MODULE_NAME_STR "." MODULE_NAME_STR,
    .tp_doc = "Implementation of ITU-T G.722 audio codec in Python using C extension.",
    .tp_basicsize = sizeof(PyG722),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc)PyG722_init,
    .tp_dealloc = (destructor)PyG722_dealloc,
    .tp_methods = PyG722_methods,
};

static struct PyModuleDef G722_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = MODULE_NAME_STR,
    .m_doc = "Python interface for the ITU-T G.722 audio codec.",
    .m_size = -1,
};

// Module initialization function
PyMODINIT_FUNC PY_INIT_FUNC(void) {
    PyObject* module;
    if (PyType_Ready(&PyG722Type) < 0)
        return NULL;

    module = PyModule_Create(&G722_module);
    if (module == NULL)
        return NULL;

    Py_INCREF(&PyG722Type);
    PyModule_AddObject(module, MODULE_NAME_STR, (PyObject*)&PyG722Type);

    import_array();

    if (PyType_Ready(&PyDataOwnerType) < 0)
        return NULL;

    return module;
}

