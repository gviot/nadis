import redis, hiredis, psutil, cProfile, pstats, sys

conn = redis.StrictRedis(host='localhost', port=6379, db=0)

key = "redis_py"

conn.delete(key)

for i in xrange(200):
	conn.zadd(key, i, "p%s" % i)

def run(id):
	c = conn.pipeline(transaction=False)
	for i in xrange(5):
		c.zrevrange(key, 0, 10, withscores=True)
	c.execute()

pr = cProfile.Profile()
user_start, nice_start, system_start, idle_start = psutil.cpu_times()
pr.enable()

for run_id in xrange(1000):
	run(run_id)

pr.disable()
user_end, nice_end, system_end, idle_end = psutil.cpu_times()
ps = pstats.Stats(pr, stream=sys.stdout)
ps.strip_dirs()
ps.sort_stats("cumulative")
ps.print_stats()

print "User CPU Time: %s" % (user_end - user_start)
print "System CPU Time: %s" % (system_end - system_start)
print "Total CPU Time: %s" % ((system_end - system_start) + (user_end - user_start))
