#ifndef _PARSER_H_
#define _PARSER_H_

#include <Python.h>
#include "circular_buffer.h"

typedef enum {
    COMPLETE_DATA,
    MISSING_DATA,
    INVALID_DATA,
    PARSE_FATAL_ERROR,
} parse_result;

typedef enum {
    PART_CHOOSER,
    PART_SINGLE_SIZED,
    PART_COUNT,
    PART_INLINE,
    PART_SIZE,
    PART_PYSTRING,
    PART_PYINT,
    PART_ERROR,
} part_type;

typedef enum {
    TYPE_SINGLE_VALUE,
    TYPE_LIST_OF_SINGLE_VALUES,
    TYPE_LIST_OF_TUPLES
} result_type;

typedef struct {
    long parts_count, current_part, next_part_size;
    part_type expected_type;
    PyObject *result_root, *list;
    int result_type;
    Py_ssize_t list_length, list_filled;
} parser_state;

PyObject *parser_get_results(void);

parse_result parser_parse_part(parser_state *s, circular_buffer *buffer);
parse_result parser_parse_unit(parser_state *s, circular_buffer *buffer);

#endif // _PARSER_H_
