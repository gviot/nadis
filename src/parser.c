#include "macros.h"
#include "parser.h"
#include "circular_buffer.h"

static int
parser_append_result(parser_state *s, PyObject *val, int size)
{
    int res;
    PyObject *obj_str = PyObject_Bytes(val);
    const char *str = PyString_AsString(obj_str);

    print("R: %s\n", str);
    Py_DECREF(obj_str);
    if (val == NULL) {
        val = Py_None;
        Py_INCREF(Py_None);
    }
    if (s->result_root == NULL) {
        s->result_root = PyList_New(0);
    }

    if (size > 1) {
        s->list = val;
        s->list_length = size;
        s->list_filled = 0;
        return PyList_Append(s->result_root, val);
    }
    if (s->list_length > 0) {
        // we are currently building a list
        print("setting item... %zd\n", s->list_filled);
        if(PyList_SET_ITEM(s->list, s->list_filled, val) != NULL)
            res = 1;
        else
            res = 0;
        print("item set.\n");
        s->list_filled++;
        if(s->list_filled >= s->list_length){
            print("finished list.\n");
            // list is full, we return to appending to root
            s->list_length = 0;
            s->list_filled = 0;
        }
        return res;
    }
    return PyList_Append(s->result_root, val);
}

PyObject *
parser_get_results(parser_state *s)
{
    PyObject *val;
    switch(PyList_Size(s->result_root)){
        case 0:
            Py_INCREF(Py_None);
            val = Py_None;
            break;
        case 1:
            val = PyList_GetItem(s->result_root, 0);
            PyList_SetSlice(s->result_root, 0, 1, NULL);
            break;
        default:
            val = s->result_root;
            s->result_root = NULL;
            break;
    }
    return val;
}

static inline parse_result
parse_integer(circular_buffer *buffer, long *integer)
{
    int remaining;
    const char *cur, *ncur, *end;

    buffer_readable_bytes(buffer, &cur, &remaining);
    end = memchr(cur, (int)'\r', remaining);
    if(end == NULL)
        return MISSING_DATA;
    *integer = strtol(cur, (char **)&ncur, 10);
    buffer_consume_bytes(buffer, (ncur - cur) + 2);
    return COMPLETE_DATA;
}

static inline parse_result
parse_part_size(circular_buffer *buffer, long *integer)
{
    int remaining;
    const char *cur;
    parse_result r;

    buffer_readable_bytes(buffer, &cur, &remaining);
    if(remaining < 4)
        return MISSING_DATA;
    assert(*cur == '$');
    buffer_consume_bytes(buffer, 1);
    r = parse_integer(buffer, integer);
    if(r != COMPLETE_DATA)
        buffer_restore_consumed_bytes(buffer, 1);
    return r;
}

static inline parse_result
parse_parts_count(circular_buffer *buffer, long *parts_count)
{
    int remaining;
    const char *cur;
    parse_result r;

    buffer_readable_bytes(buffer, &cur, &remaining);
    if(remaining < 4)
        return MISSING_DATA;
    assert(*cur == '*');
    buffer_consume_bytes(buffer, 1);
    r = parse_integer(buffer, parts_count);
    if(r != COMPLETE_DATA)
        buffer_restore_consumed_bytes(buffer, 1);
    return r;
}

static inline parse_result
parse_inline(circular_buffer *buffer, long *integer)
{
    int remaining;
    const char *cur;

    buffer_readable_bytes(buffer, &cur, &remaining);
    if(remaining < 1)
        return MISSING_DATA;
    parse_result r = COMPLETE_DATA;
    //assert(*cur == '*');
    *integer = 0;
    buffer_consume_bytes(buffer, 1);
    return r;
}

static inline parse_result
parse_single_sized(circular_buffer *buffer)
{
    int remaining;
    const char *cur;

    buffer_readable_bytes(buffer, &cur, &remaining);
    if(remaining < 1)
        return MISSING_DATA;
    assert(*cur == '$');
    return COMPLETE_DATA;
}

static inline parse_result
parse_error(circular_buffer *buffer, PyObject **pyerr)
{
    int remaining;
    const char *cur;

    buffer_consume_bytes(buffer, 1);
    buffer_readable_bytes(buffer, &cur, &remaining);
    *(char*)(cur + remaining) = '\0';
    print("E: %s\n", cur);
    PyErr_SetString(PyExc_RuntimeError, cur);
    // TODO: don't consume all the remaining data but only the error message
    buffer_consume_bytes(buffer, remaining);
    return COMPLETE_DATA;
}

static inline parse_result
header_choose(circular_buffer *buffer, part_type *next_type)
{
    int remaining;
    const char *cur;

    buffer_readable_bytes(buffer, &cur, &remaining);
    if(remaining < 1)
        return MISSING_DATA;
    if(*cur == '*')
    { // multi bulk reply
        *next_type = PART_COUNT;
        return COMPLETE_DATA;
    }
    else if ((*cur == '+'))
    { // status reply
        *next_type = PART_INLINE;
        return COMPLETE_DATA;
    }
    else if (*cur == ':')
    { // integer reply
        *next_type = PART_PYINT;
        return COMPLETE_DATA;
    }
    else if (*cur == '$')
    { // bulk reply $
        *next_type = PART_SINGLE_SIZED;
        return COMPLETE_DATA;
    }
    else if (*cur == '-')
    { // status error -
        *next_type = PART_ERROR;
        return COMPLETE_DATA;
    }
    else
    {
        return MISSING_DATA;
    }
}

static inline parse_result
parse_pystring(circular_buffer *buffer, int expected_size, PyObject **pystr)
{
    int remaining;
    const char *pos, *cur;

    buffer_readable_bytes(buffer, &cur, &remaining);
    if(expected_size == 0)
    {
        pos = memchr(cur, (int)'\r', remaining);
        if(pos == NULL)
            return MISSING_DATA;
        else
            expected_size = (int)(pos - cur);
    }
    if(remaining < (expected_size + 2))
    {
        *pystr = NULL;
        return MISSING_DATA;
    }
    *pystr = Py_BuildValue("s#", cur, expected_size);
    buffer_consume_bytes(buffer, expected_size + 2);
    return COMPLETE_DATA;
}

static inline parse_result
parse_pyint(circular_buffer *buffer, int expected_size, PyObject **pyint)
{
    int remaining;
    const char *cur, *pos;

    buffer_consume_bytes(buffer, 1);
    buffer_readable_bytes(buffer, &cur, &remaining);
    if(expected_size == 0)
    {
        pos = memchr(cur, (int)'\r', remaining);
        if(pos == NULL)
            return MISSING_DATA;
        else
            expected_size = (int)(pos - cur);
    }
    else
    {
        pos = cur + expected_size;
    }
    if(remaining < (expected_size + 2))
    {
        *pyint = NULL;
        return MISSING_DATA;
    }
    *(char*)(pos + 1) = '\0';
    *pyint = PyLong_FromString((char*)cur, NULL, 10);
    buffer_consume_bytes(buffer, expected_size + 2);
    return COMPLETE_DATA;
}

parse_result
parser_parse_part(parser_state *s, circular_buffer *buffer)
{
    parse_result result;
    PyObject *val;
    switch(s->expected_type)
    {
    case PART_CHOOSER:
        print("P: CHOOSER\n");
        s->current_part = 1;
        result = header_choose(buffer, &s->expected_type);
        break;
    case PART_INLINE:
        print("P: INLINE\n");
        result = parse_inline(buffer, &(s->next_part_size));
        if(result == COMPLETE_DATA)
        {
            s->expected_type = PART_PYSTRING;
        }
        break;
    case PART_SINGLE_SIZED:
        print("P: SINGLE_SIZED\n");
        result = parse_single_sized(buffer);
        if(result == COMPLETE_DATA)
        {
            s->expected_type = PART_SIZE;
            s->parts_count = 1;
        }
        break;
    case PART_COUNT:
        print("P: COUNT\n");
        result = parse_parts_count(buffer, &(s->parts_count));
        if(result == COMPLETE_DATA)
        {
            s->expected_type = PART_SIZE;
            parser_append_result(s, PyList_New(s->parts_count), s->parts_count);
        }
        break;
    case PART_SIZE:
        print("P: SIZE\n");
        result = parse_part_size(buffer, &(s->next_part_size));
        if(result == COMPLETE_DATA)
        {
            s->expected_type = PART_PYSTRING;
        }
        break;
    case PART_PYSTRING:
        print("P: PYSTRING\n");
        result = parse_pystring(buffer, (int)s->next_part_size, &val);
        if(result == COMPLETE_DATA)
        {
            if(parser_append_result(s, val, 1) < 0)
                return PARSE_FATAL_ERROR;
            s->current_part++;
            if(s->current_part > s->parts_count)
                s->expected_type = PART_CHOOSER;
            else
                s->expected_type = PART_SIZE;
        }
        break;
    case PART_PYINT:
        print("P: PYINT\n");
        result = parse_pyint(buffer, 0, &val);
        if(result == COMPLETE_DATA)
        {
            if(parser_append_result(s, val, 1) < 0)
                return PARSE_FATAL_ERROR;
            s->current_part++;
            s->expected_type = PART_CHOOSER;
        }
        break;
    case PART_ERROR:
        print("P: ERROR\n");
        result = parse_error(buffer, &val);
        if(result == COMPLETE_DATA)
        {
            parser_append_result(s, NULL, 1);
            s->expected_type = PART_CHOOSER;
        }
        break;
    }
    return result;
}

parse_result
parser_parse_unit(parser_state *s, circular_buffer *buffer)
{
    parse_result r;

    s->expected_type = PART_CHOOSER;
    do{
        r = parser_parse_part(s, buffer);
    }
    while ( ((s->expected_type != PART_CHOOSER) && (r != MISSING_DATA)) ||
        ( ((s->expected_type == PART_CHOOSER) &&
            (buffer_has_bytes(buffer) > 0)) )
    );
    //while ( ((s->expected_type != PART_CHOOSER) && (r != MISSING_DATA)) );
    return r;
}


