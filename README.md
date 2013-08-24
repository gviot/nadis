Nadis
=====

Nadis is a redis driver for Python, implemented mostly in C.

I have had the opportunity to work on servers using redis as a database and needed high performance. But I noticed when using redis-py that it was causing significant overhead to redis requests, even with hiredis installed.

The redis protocol is pretty simple so I decided to start an experiment to find out if a faster driver would be worth implementing. The results so far are very encouraging so I decided to continue this project and open source it.

This is how nadis is born. Nadis stands for "No Allocation reDIS driver", although "No Allocation" is not completly true. Basically, the only allocations made are when creating a connection or when creating python objects/lists for the request results.

For IO (this includes serialization and parsing), 2 pre-allocated circular buffers are used for each connection.

Right now, nadis is far from being feature-complete. It is definitely a work in progress, the interface might change. You should not use it in production.

If you want to use nadis
========================

 - Try redis-py first, if it fast enough for you, use it.
 - Read the source code. It is not complicated and will give an idea of what is implemented and what is dangerous.

Missing features
================

 - Transactions
 - A lot of basic commands
 - Lua
 - PUB/SUB
 - Thread-safety

Short term goals
================

In the short term, I would like to add features to match redis-py.
I would also like an option to do async IO. For example using multiple connections from a connection pool owned by a single thread, and using a method from the pool to do the IO for all connections and return when all pending operations are finished. I am not quite sure yet how to implement that in a nice way and not burn users who use it improperly.
Async IO would give a nice performance improvement for me as I use redis in a heavily sharded and replicated environment so I have to do requests to a lot of different redis instances. Doing it one by one is inefficient.

What might never be added
=========================

Lua, PUB/SUB (or any other blocking operation) and thread-safety are not useful for my current use of redis, so they will probably never see the light of day, unless someone else is interested in this driver and wants to add them.

Benchmarks
==========

I have started adding benchmarks. At the moment there is only one bad benchmark, but it helps keep track of where this driver stands in comparison to other drivers, and also helps keep track of performance hits when adding new features or doing big changes.

