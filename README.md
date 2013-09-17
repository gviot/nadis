Nadis
=====

[![Build Status](https://travis-ci.org/gviot/nadis.png)](https://travis-ci.org/gviot/nadis)

Nadis is a redis driver for Python, implemented mostly in C.

I have had the opportunity to work on servers using redis as a database and needed high performance. But I noticed when using redis-py that it was causing significant overhead to redis requests, even with hiredis installed.

The redis protocol is pretty simple so I decided to start an experiment to find out if a faster driver would be worth implementing.

Nadis stands for "No Allocation reDIS driver", although "No Allocation" is not completly true.
The driver makes use of pre-allocated buffers for serialization/parsing data.

Right now, nadis is far from being feature-complete. It is definitely a work in progress and the interface might change so you should not use it in production.
If you are looking for a production-ready driver, use redis-py.

Missing features
================

 - Transactions
 - A lot of basic commands
 - Lua
 - PUB/SUB
 - Thread-safety

Short term goals
================

 - Replace the POSIX/C socket interface with the python socket module in order to enable monkey-patching for gevent
 - Add the missing core commands to manipulate all the data types

Benchmarks
==========

I have started adding benchmarks. At the moment there is only one bad benchmark, but it helps keep track of where this driver stands in comparison to other drivers, and also helps keep track of performance hits when adding new features or doing big changes.

