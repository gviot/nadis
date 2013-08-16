from collections import deque
from nadis_impl import ConnectionImpl, ConnectionError


class Connection(object):
    def __init__(self, hostname="localhost", port=6379):
        self.impl = ConnectionImpl(hostname, port)

    def auth(self, password):
        return self.impl.generic_command("auth", password)

    def reset(self):
        return self.impl.reset()

    def flushdb(self):
        return self.impl.generic_command("flushdb")

    def quit(self):
        return self.impl.generic_command("quit")

    def set_buffering(self, val=True):
        return self.impl.set_buffering(val)

    def close(self):
        return self.impl.close()

    def flush(self):
        return self.impl.flush()

    def ping(self):
        return self.impl.generic_command("ping")

    def echo(self, val):
        return self.impl.generic_command("echo", val)

    def expire(self, key, val):
        return self.impl.generic_command("expire", key, val)

    def pexpire(self, key, val):
        return self.impl.generic_command("pexpire", key, val)

    def ttl(self, key):
        return self.impl.generic_command("ttl", key)

    def pttl(self, key):
        return self.impl.generic_command("pttl", key)

    def set(self, key, value):
        return self.impl.generic_command("set", key, value)

    def get(self, key):
        return self.impl.generic_command("get", key)

    def delete(self, key):
        return self.impl.generic_command("del", key)

    def incr(self, key):
        return self.impl.generic_command("incr", key)

    def incrby(self, key, value):
        return self.impl.generic_command("incrby", key, value)

    def zcard(self, key):
        return self.impl.generic_command("zcard", key)

    def zincrby(self, key, value, member):
        return self.impl.generic_command("zincrby", key, value, member)

    def zrevrank(self, key, member):
        return self.impl.generic_command("zrevrank", key, member)

    def zscore(self, key, member):
        return self.impl.generic_command("zscore", key, member)

    def zrevrange(self, key, start, stop, withscores=False):
        if withscores:
            return self.impl.generic_command("zrevrange", key,
                                             start, stop, "withscores")
        return self.impl.generic_command("zrevrange", key, start, stop)


class ConnectionPoolError(Exception):
    pass


class ConnectionPool(object):
    """A connection pool linked to a host and port
    with a limited number of connections.
    It is currently NOT thread safe and should be used by a single thread
    for async operations on multiple connections"""

    def __init__(self, host, port, size=5):
        """Construct the connection pool for the specified [host/port]
        with [size] number of connections.
        The default number of connections is 5
        """

        self.available = deque([], size)
        self.closed = False
        for i in xrange(size):
            self.available.appendleft(Connection(host, port))

    def acquire(self):
        """Acquire a connection from the pool. The connection is guaranteed
        to be connected when acquired."""

        if self.closed:
            raise ConnectionPoolError(
                "ConnectionPool has been closed and cannot be used anymore.")
        c = self.available.pop()
        c.reset()
        return c

    def reclaim(self, conn):
        """Reclaim a connection and put it back in the pool.
        If the connection is faulty, the pool will take care
        of renewing it when appropriate"""

        if self.closed:
            raise ConnectionPoolError(
                "ConnectionPool has been closed and cannot be used anymore.")
        self.available.appendleft(conn)
        conn.set_buffering(False)

    def close(self):
        """Empty the pools and disconnect every active connection"""

        self.closed = True
        try:
            while True:
                c = self.available.pop()
                c.close()
        except IndexError, e:
            return

    def pipeline(self):
        """Acquire a pipeline object that allows to send requests
        to a connection with buffering.
        This works like a standard connection, except that you need to flush
        when you need to send your requests and get your results"""
        c = self.acquire()
        c.set_buffering(True)
        return c
