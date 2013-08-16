#include <Python.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <assert.h>

#include "connection.h"

extern PyObject *ConnectionError;

extern PyTypeObject nadis_ConnectionType;
extern PyTypeObject nadis_PipelineType;

static PyMethodDef nadis_methods[] = {
    {NULL}  /* Sentinel */
};

#ifndef PyMODINIT_FUNC  /* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initnadis_impl(void) 
{
    PyObject* m;

    nadis_ConnectionType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&nadis_ConnectionType) < 0)
        return;

    m = Py_InitModule3("nadis_impl", nadis_methods,
                       "Module that provides a driver for a Redis database.");

    Py_INCREF(&nadis_ConnectionType);
    PyModule_AddObject(m, "ConnectionImpl", (PyObject *)&nadis_ConnectionType);

    ConnectionError = PyErr_NewException("nadis.ConnectionError", NULL, NULL);
    Py_INCREF(ConnectionError);
    PyModule_AddObject(m, "ConnectionError", ConnectionError);
}

