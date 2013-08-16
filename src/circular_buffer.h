#ifndef _CIRCULAR_BUFFER_H_
#define _CIRCULAR_BUFFER_H_

#define CIRCULAR_BUFFER_SIZE (4096 * 4)

typedef struct {
	char *position;
	int length;
} buffer_cursor;

typedef struct {
	char data[CIRCULAR_BUFFER_SIZE];
	char *end;
	buffer_cursor read_cursor, write_cursor;
} circular_buffer;

void buffer_init(circular_buffer *buf);
int buffer_read_from_socket(circular_buffer *buf, int fd, int timeout);
int buffer_printf(circular_buffer *buf, const char *format, ...);
void buffer_readable_bytes(circular_buffer *buf, const char **ptr, int *length);
int buffer_write_to_socket(circular_buffer *buf, int fd);
void buffer_consume_bytes(circular_buffer *buf, int bytes_count);
void buffer_restore_consumed_bytes(circular_buffer *buf, int bytes_count);
void buffer_publish_bytes(circular_buffer *buf, int bytes_count);
int buffer_has_bytes(circular_buffer *buf);

#endif // _CIRCULAR_BUFFER_H_
