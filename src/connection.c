#include "macros.h"
#include "connection.h"
#include "serializer.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

PyObject *ConnectionError;

/* Create a non-blocking socket and connects to redis
   TODO:
        - Make it less ulgy
        - Replace bzero and bcopy which are deprecated
        - Call another function for the select
*/
static int
redis_connect(const char *hostname, int port)
{
    struct sockaddr_in serv_addr;
    struct hostent *server;
    int sockfd = 0;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server = gethostbyname(hostname);
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(port);
    int flags = fcntl(sockfd, F_GETFL, 0);
    flags = (flags|O_NONBLOCK);
    fcntl(sockfd, F_SETFL, flags);
    int res = connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr));
    if (res < 0 && errno != EINPROGRESS) {
        return res;
    }
    struct timeval       timeout_val;
    timeout_val.tv_sec  = 5;
    timeout_val.tv_usec = 0;
    struct fd_set        master_set;
    FD_ZERO(&master_set);
    FD_SET(sockfd, &master_set);
    print("Connecting %d...\n", sockfd);
    int sres = select(sockfd + 1, NULL, &master_set, NULL, &timeout_val);
    if (sres > 0)
        return sockfd;
    else
        return sres;
}

/* Non-blocking functions that sends some data from the buffer to the socket */
static inline int
flush_some(Connection *c)
{
    if (!buffer_has_bytes(&c->out_buffer))
        return 0;
    return buffer_write_to_socket(&c->out_buffer, c->sock);
}

/* Blocking function that flushes as much data as possible.
   timeout is used for the select but the actual elapsed time
   may be longer because select can be called multiple times */
static inline int
flush_all(Connection *c, int timeout)
{
    int res = 0;

    do {
        res = flush_some(c);
        if(res < 0)
            return 0;
    }
    while (buffer_has_bytes(&c->out_buffer));
    return 1;
}

/* Non-blocking function to read some data from the socket */
static inline int
read_some(Connection *c, int timeout)
{
    return buffer_read_from_socket(&c->in_buffer, c->sock, timeout);
}

/* Blocking function that reads as much data as possible from the socket.
   timeout is used for the select but the actual elapsed time
   may be longer because select can be called multiple times
   TODO: Change the do while condition to make sure we read everything
         when reading responses for a pipeline */
static inline int
read_all(Connection *c, int timeout)
{
    int res = 0;
    const char *cur;

    do {
        res = read_some(c, timeout);
        if (res < 0)
            return 0;
        cur = c->in_buffer.read_cursor.position +
            c->in_buffer.read_cursor.length - 1;
        timeout = 0;
    }
    while ( (res > 0) );
    return 1;
}

/* This is used to check internally if a connection is still ok
   or if it's faulty. It is actually quite safe but quite ugly and limited.
   It should be improved and not exposed outside of this compile unit. */
static int
unsafe_ping(Connection *self)
{
    int len = 0, remaining;
    const char *cur;

    len = write(self->sock, "PING\r\n", 6);
    if (len < 6)
        return 0;
    if (!read_all(self, 5))
        return 0;
    buffer_printf(&self->out_buffer, "PING\r\n");
    buffer_write_to_socket(&self->out_buffer, self->sock);
    buffer_read_from_socket(&self->in_buffer, self->sock, 1);
    buffer_readable_bytes(&self->in_buffer, &cur, &remaining);
    buffer_consume_bytes(&self->in_buffer, strlen("+PONG\r\n"));
    if (memcmp(cur, "+PONG\r\n", 7) == 0)
        return 1;
    return 0;
}

/* Initializes the Connection object,
   this includes the associated parser state
   TODO: improve the timeout message */
static int
Connection_init(Connection *self, PyObject *args, PyObject *kwds)
{
    const char *hostname;
    int port;

    if (!PyArg_ParseTuple(args, "si", &hostname, &port))
        return -1;
    self->sock = redis_connect(hostname, port);
    if (self->sock < 0) {
        PyErr_SetFromErrno(ConnectionError);
        return -1;
    }
    else if (self->sock == 0) {
        PyErr_SetString(ConnectionError,
            "Host was not reached within the allowed time.");
        return -1;
    }
    self->hostname = hostname;
    self->port = port;
    self->is_pipeline = 0;
    self->parser.result_root = NULL;
    buffer_init(&self->in_buffer);
    buffer_init(&self->out_buffer);
    return 0;
}

/* This function reads data from the socket and parses that data
   as it arrives using the parser state machine.
   TODO: rename the function because it does not only parse */
static inline int 
Connection_parse_results(Connection *self)
{
    int res;
    parse_result r = MISSING_DATA;

    do
    {
        if (r == MISSING_DATA)
            res = read_some(self, 5);
        else
            res = read_some(self, 0);
        if (res > 0) {
            r = parser_parse_unit(&self->parser, &self->in_buffer);
        } else if (res < 0) {
            return 0;
        }
    }
    /* we read and parse as long as the last data was not the end
    of a message or as long as we have data available on the socket */
    while ((r == MISSING_DATA) || (res > 0));
    buffer_init(&self->in_buffer);
    buffer_init(&self->out_buffer);
    return 1;
}

static PyObject *
Connection_set_buffering(Connection *self, PyObject *args, PyObject *kwds)
{
    PyObject *new_state;

    if (!PyArg_ParseTuple(args, "O", &new_state))
        return NULL;
    self->is_pipeline = PyObject_IsTrue(new_state);
    if (!self->is_pipeline){
        buffer_init(&self->in_buffer);
        buffer_init(&self->out_buffer);
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Connection_flush(Connection *self, PyObject *args, PyObject *kwds)
{
    int fres;

    fres = flush_all(self, 5);
    if (fres < 1) {
        return NULL;
    }
    if (Connection_parse_results(self)) {
        return parser_get_results(&self->parser);
    }
    else {
        return NULL;
    }
}

/* TODO: check the return value from close and handle errors */
static PyObject *
Connection_close(Connection *self, PyObject *args, PyObject *kwds)
{
    close(self->sock);
    Py_INCREF(Py_None);
    return Py_None;
}

/* TODO: check the return value of redis_connect,
   if redis_connect fails, we must raise an exception. */
static PyObject *
Connection_reset(Connection *self, PyObject *args, PyObject *kwds)
{
    if ((!read_some(self, 0)) || (!unsafe_ping(self))) {
        /* here we handle the errors set by the above functions
           so we clear PyErr to stop the exception from being raised
           */
        PyErr_Clear();
        close(self->sock);
        self->sock = redis_connect(self->hostname, self->port);
    }
    self->is_pipeline = 0;
    buffer_init(&self->in_buffer);
    buffer_init(&self->out_buffer);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
Connection_generic_command(Connection *self, PyObject *args)
{
    int fres;

    serialize_command(&self->out_buffer, args);
    if (!self->is_pipeline) {
        fres = flush_all(self, 5);
        if (fres < 1) {
            return NULL;
        }
        if (Connection_parse_results(self)) {
            return parser_get_results(&self->parser);
        }
        else {
            return NULL;
        }
    }
    Py_INCREF(Py_None);
    return Py_None;
}

#define DECLARE_METHOD(METHOD, DOC) \
    {#METHOD, (PyCFunction)Connection_##METHOD, METH_VARARGS, \
     DOC \
    },

#define DECLARE_METHOD_WITH_KEYWORDS(METHOD, DOC) \
    {#METHOD, (PyCFunction)Connection_##METHOD, METH_VARARGS|METH_KEYWORDS, \
     DOC \
    },

static PyMethodDef Connection_methods[] = {
    DECLARE_METHOD(generic_command,
        "Sends a command and its arguments to this connection, "
        "will parse and return result if not a pipeline")
    /* DRIVER METHODS */
    DECLARE_METHOD(set_buffering,
        "Sets the output buffering mode for this connection (True=buffered)")
    DECLARE_METHOD(flush,
        "Flushes the connection's output buffer")
    DECLARE_METHOD(close,
        "Close the connection")
    DECLARE_METHOD(reset,
        "Reset the connection state")
    {NULL}  /* Sentinel */
};

PyTypeObject nadis_ConnectionType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "nadis.ConnectionImpl",    /*tp_name*/
    sizeof(Connection),	       /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    0,                         /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "A connection object containing a socket "
    "and other resources used to communicate "
    "with a Redis server",     /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    Connection_methods,        /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Connection_init, /* tp_init */
    0,                         /* tp_alloc */
    0,                         /* tp_new */
};


