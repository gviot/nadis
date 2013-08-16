#ifndef _SERIALIZER_H_
#define _SERIALIZER_H_

#include "circular_buffer.h"

int serialize_command(circular_buffer *buf, PyObject *args);

#endif // _SERIALIZER_H_
