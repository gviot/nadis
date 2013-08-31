import nadis, psutil, cProfile, pstats, sys

c = nadis.Connection('localhost', 6379)

key = "nadis"

c.delete(key)

for i in xrange(100):
	c.zadd(key, i, "p%s" % i)

def run(id):
	for i in xrange(5):
		c.zrevrange(key, 0, 10, withscores=True)
	c.flush()

pr = cProfile.Profile()
user_start, nice_start, system_start, idle_start = psutil.cpu_times()
pr.enable()

c.set_buffering(1)

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
