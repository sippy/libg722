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
#include <string.h>

#include <Python.h>
#if defined(WITH_NUMPY) && WITH_NUMPY
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>
#endif

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

typedef struct {
    PyObject_HEAD
    G722_DEC_CTX *g722_dctx;
    G722_ENC_CTX *g722_ectx;
    int sample_rate;
    int bit_rate;
    bool use_numpy;
} PyG722;

static PyObject *
build_pcm16_array(int16_t *array, Py_ssize_t olength) {
    PyObject* array_mod = PyImport_ImportModule("array");
    if (!array_mod) {
        free(array);
        return NULL;
    }
    PyObject* array_ctor = PyObject_GetAttrString(array_mod, "array");
    Py_DECREF(array_mod);
    if (!array_ctor) {
        free(array);
        return NULL;
    }
    PyObject* result = PyObject_CallFunction(array_ctor, "s", "h");
    Py_DECREF(array_ctor);
    if (!result) {
        free(array);
        return NULL;
    }
    PyObject* pcm_bytes = PyBytes_FromStringAndSize((const char *)array, olength * sizeof(array[0]));
    free(array);
    if (!pcm_bytes) {
        Py_DECREF(result);
        return NULL;
    }
    PyObject* frombytes_ret = PyObject_CallMethod(result, "frombytes", "O", pcm_bytes);
    Py_DECREF(pcm_bytes);
    if (!frombytes_ret) {
        Py_DECREF(result);
        return NULL;
    }
    Py_DECREF(frombytes_ret);
    return result;
}

static int PyG722_init(PyG722* self, PyObject* args, PyObject* kwds) {
    int sample_rate, bit_rate, options;
    PyObject *use_numpy_obj = NULL;
    static char *kwlist[] = {"sample_rate", "bit_rate", "use_numpy", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "ii|O", kwlist, &sample_rate, &bit_rate, &use_numpy_obj)) {
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
#if defined(WITH_NUMPY) && WITH_NUMPY
    self->use_numpy = true;
#else
    self->use_numpy = false;
#endif
    if (use_numpy_obj != NULL) {
        int use_numpy = PyObject_IsTrue(use_numpy_obj);
        if (use_numpy < 0) {
            return -1;
        }
#if defined(WITH_NUMPY) && WITH_NUMPY
        self->use_numpy = use_numpy;
#else
        if (use_numpy) {
            PyErr_SetString(PyExc_RuntimeError,
              "NumPy output requested, but this build has no NumPy support. "
              "Reinstall without LIBG722_NO_NUMPY=1.");
            return -1;
        }
#endif
    }

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
    PyObject* seq = NULL;
    int16_t* array;
    Py_ssize_t length, i, olength;
    bool from_numpy = false;
    bool from_buffer = false;
    Py_buffer view;

    PyObject *rval = NULL;
    if (!PyArg_ParseTuple(args, "O", &item)) {
        PyErr_SetString(PyExc_TypeError, "Takes exactly one argument");
        goto e0;
    }

#if defined(WITH_NUMPY) && WITH_NUMPY
    if (PyArray_Check(item) && PyArray_TYPE((PyArrayObject*)item) == NPY_INT16) {
        array = (int16_t *)PyArray_DATA((PyArrayObject*)item);
        length = PyArray_SIZE((PyArrayObject*)item);
        from_numpy = true;
    } else {
#endif
        if (PyObject_CheckBuffer(item) &&
            PyObject_GetBuffer(item, &view, PyBUF_CONTIG_RO | PyBUF_FORMAT) == 0) {
            if (view.ndim != 1 || view.itemsize != (Py_ssize_t)sizeof(array[0]) ||
                !is_i16_buffer_format(view.format)) {
                PyBuffer_Release(&view);
            } else if (view.len % sizeof(array[0]) != 0) {
                PyBuffer_Release(&view);
                PyErr_SetString(PyExc_TypeError, "Expected buffer with 16-bit samples");
                goto e0;
            } else {
                array = (int16_t *)view.buf;
                length = view.len / sizeof(array[0]);
                from_buffer = true;
                goto have_input;
            }
        } else {
            PyErr_Clear();
        }

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
#if defined(WITH_NUMPY) && WITH_NUMPY
    }
#endif
have_input:
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
    if (!from_numpy && !from_buffer) {
        free(array);
    }
e1:
    if (!from_numpy && !from_buffer) {
        Py_DECREF(seq);
    }
    if (from_buffer) {
        PyBuffer_Release(&view);
    }
e0:
    return rval;
}

#if defined(WITH_NUMPY) && WITH_NUMPY
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
#endif

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

#if defined(WITH_NUMPY) && WITH_NUMPY
    if (self->use_numpy) {
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
#endif
    return build_pcm16_array(array, olength);
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

#if defined(WITH_NUMPY) && WITH_NUMPY
    import_array();

    if (PyType_Ready(&PyDataOwnerType) < 0)
        return NULL;
#endif

    return module;
}
