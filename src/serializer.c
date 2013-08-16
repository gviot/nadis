#include "macros.h"
#include <Python.h>
#include <stdarg.h>
#include <stdio.h>
#include "serializer.h"
#include "circular_buffer.h"

static inline int
serialize_pyarg(circular_buffer *buf, PyObject *arg)
{
    Py_ssize_t len;
    char *arg_str;
    PyObject *serialized_arg;
    int res;

    serialized_arg = PyObject_Bytes(arg);
    PyString_AsStringAndSize(serialized_arg, &arg_str, &len);
    res = buffer_printf(buf, "$%zd\r\n%*s\r\n", len, len, arg_str);
    Py_DECREF(serialized_arg);
    return res;
}

int
serialize_command(circular_buffer *buf, PyObject *args)
{
    Py_ssize_t count, len;
    PyObject *arg;

    count = PyTuple_Size(args);
    len = buffer_printf(buf, "*%zd\r\n", count);
    for(Py_ssize_t i = 0; i < count; i++){
        arg = PyTuple_GetItem(args, i);
        len += serialize_pyarg(buf, arg);
    }
    return len;
}
