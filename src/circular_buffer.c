#include "macros.h"
#include "circular_buffer.h"
#include <Python.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>

#define SOCKET_TIMEOUT 5

extern PyObject *ConnectionError;

static inline void
mark_end_and_rewind(circular_buffer *buf)
{
	buf->end = buf->write_cursor.position;
	buf->write_cursor.position = buf->data;
	buf->write_cursor.length =
		(buf->read_cursor.position - buf->data);
}

void
buffer_init(circular_buffer *buf)
{
	buf->write_cursor.position = buf->read_cursor.position = buf->data;
	buf->write_cursor.length = CIRCULAR_BUFFER_SIZE;
    buf->read_cursor.length = 0;
	buf->end = buf->data + CIRCULAR_BUFFER_SIZE;
}

/* Helper function that does a select for read on a socket */
static inline int
select_read(int fd, int timeout){
    struct timeval timeout_val;
    struct fd_set master_set;
    int res = 0;

    timeout_val.tv_sec  = timeout;
    timeout_val.tv_usec = 0;
    FD_ZERO(&master_set);
    FD_SET(fd, &master_set);
    //print("SRS\n");
    res = select(fd + 1, &master_set, NULL, NULL, &timeout_val);
    //print("SRE: %d\n", res);
    if ((res == 0) && (timeout != 0)) {
        PyErr_SetString(ConnectionError,
            "Socket select for read timed out.");
    }
    else if (res < 0) {
        PyErr_SetString(ConnectionError,
            "Socket select for read error.");
    }
    return res;
}

int
buffer_read_from_socket(circular_buffer *buf, int fd, int timeout)
{
	int sel = 0, len = 0;

	sel = select_read(fd, timeout);
	if (sel < 1) {
        if ((sel < 0) && (errno != EWOULDBLOCK) && (errno != EAGAIN)) {
            PyErr_SetFromErrno(ConnectionError);
            return -1;
        }
		return 0;
	}
	len = read(fd, buf->write_cursor.position, buf->write_cursor.length);
	if (len > 0) {
        //print("<<\n%*s\n", len, buf->write_cursor.position);
		buffer_publish_bytes(buf, len);
		return 1;
	}
	return 0;
}

/* Helper function that does a select for read on a socket */
static inline int
select_write(int fd, int timeout){
    struct timeval timeout_val;
    struct fd_set master_set;
    int res = 0;

    timeout_val.tv_sec  = timeout;
    timeout_val.tv_usec = 0;
    FD_ZERO(&master_set);
    FD_SET(fd, &master_set);
    //print("SWS\n");
    res = select(fd + 1, NULL, &master_set, NULL, &timeout_val);
    //print("SWE %d\n", res);
    if(res == 0){
        PyErr_SetString(ConnectionError,
            "Socket select for read timed out.");
    }
    else if(res < 0){
        PyErr_SetString(ConnectionError,
            "Socket select for read error.");
    }
    return res;
}

int
buffer_write_to_socket(circular_buffer *buf, int fd)
{
	int sel = 0, len = 0;

	sel = select_write(fd, SOCKET_TIMEOUT);
    if (sel < 0) {
        if ((errno != EWOULDBLOCK) && (errno != EAGAIN)) {
            PyErr_SetFromErrno(ConnectionError);
            return -1;
        }
        else {
            return 0;
        }
    }
	len = write(fd, buf->read_cursor.position, buf->read_cursor.length);
	if (len > 0){
        //print(">>\n%*s\n", len, buf->read_cursor.position);
		buffer_consume_bytes(buf, len);
	}
	return len;
}

int
buffer_printf(circular_buffer *buf, const char *format, ...)
{
	va_list ap;
    const char *cur;
	int len = 0;

    cur = buf->write_cursor.position;
    va_start(ap, format);
    len = vsnprintf(buf->write_cursor.position, buf->write_cursor.length,
    	format, ap);
    if (len > 0) {
		buffer_publish_bytes(buf, len);
    } else {
    	mark_end_and_rewind(buf);
    	len = buffer_printf(buf, format, ap);
    }
    va_end(ap);
    return len;
}

void
buffer_readable_bytes(circular_buffer *buf, const char **ptr, int *length)
{
	*ptr = buf->read_cursor.position;
	*length = buf->read_cursor.length;
}

void
buffer_consume_bytes(circular_buffer *buf, int bytes_count)
{
    const char *cur;

    cur = buf->read_cursor.position;
    buf->read_cursor.position += bytes_count;
    buf->read_cursor.length -= bytes_count;
    if (buf->read_cursor.position > buf->write_cursor.position) {
        buf->write_cursor.length += bytes_count;
    }
    //print("%p: Consumed %d bytes: %*s\n", buf, bytes_count, bytes_count, cur);
}

void
buffer_publish_bytes(circular_buffer *buf, int bytes_count)
{
    const char *cur;

    cur = buf->write_cursor.position;
    buf->write_cursor.position += bytes_count;
    buf->write_cursor.length -= bytes_count;
    if (buf->read_cursor.position < buf->write_cursor.position) {
        buf->read_cursor.length += bytes_count;
    }
    //print("%p: Published %d bytes: %*s\n", buf, bytes_count, bytes_count, cur);
}

int
buffer_has_bytes(circular_buffer *buf)
{
	return buf->read_cursor.length;
}

void
buffer_restore_consumed_bytes(circular_buffer *buf, int bytes_count)
{
    buffer_consume_bytes(buf, -bytes_count);
}

