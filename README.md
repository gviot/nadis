nadis
=====

Nadis is a redis driver for Python, implemented mostly in C.

I have had the opportunity to work on servers using redis as a database and needed very high performance. But I noticed when using redis-py that it was causing significant overhead to redis requests, even with hiredis installed.

The redis protocol is pretty simple so I decided to start an experiment to find out if a faster driver would be worth implementing. The results so far are very encouraging so I decided to continue this project and open source it.

This is how nadis is born. Nadis stands for "No Allocation reDIS driver", although "No Allocation" is not completly true. Basically, the only allocations made are when creating a connection or when creating python objects/lists for the request results.

For IO (this includes serialization and parsing), 2 pre-allocated circular buffers are used for each connection.

Right now, nadis is far from being feature-complete. It is definitely a work in progress, the interface might change. You should not use it in production.

If you want to use nadis
========================

 - Try redis-py first, if it fast enough for you, use it.
 - Read the source code. It is not complicated and will give an idea of what is implemented and what is dangerous.
 - You should also consider using hiredis-py which is fast and more mature.

