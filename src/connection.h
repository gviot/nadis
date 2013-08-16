#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include <Python.h>
#include "parser.h"
#include "circular_buffer.h"

typedef struct {
    PyObject_HEAD
    int sock, is_pipeline, port;
    const char *hostname;
    parser_state parser;
    circular_buffer in_buffer, out_buffer;
} Connection;

#endif // _CONNECTION_H_
