#ifndef G722_NUMPY_API_H
#define G722_NUMPY_API_H

#include <stdint.h>
#include <Python.h>

typedef struct {
    int (*check_int16_1d)(PyObject *item, int16_t **array, Py_ssize_t *length);
    PyObject *(*from_pcm16_owned)(int16_t *array, Py_ssize_t length);
} G722NumpyAPI;

#define G722_NUMPY_CAPSULE_NAME "G722_numpy._C_API"

#endif
