import unittest
from itertools import islice
import nadis

class ConnectionTest(unittest.TestCase):
    
    @classmethod
    def setUpClass(cls):
        cls.pool = nadis.ConnectionPool("localhost", 6379, size=5)
        for i in xrange(5):
            c = cls.pool.acquire()
            c.flushdb()
            c.quit()
            cls.pool.reclaim(c)

    @classmethod
    def tearDownClass(cls):
        cls.pool.close()

    def setUp(self):
        self.c = self.pool.acquire()

    def tearDown(self):
        self.pool.reclaim(self.c)

    def test_ping(self):
        self.assertEqual(self.c.ping(), "PONG")

    def test_echo(self):
        test = "test"
        self.assertEqual(self.c.echo(test), test)

    def test_set_get_del(self):
        key = "test_key"
        self.c.set(key, "Expected")
        self.assertEqual(self.c.get(key), "Expected")
        self.assertEqual(self.c.delete(key), 1)

    def test_expire_ttl(self):
        key = "test_key"
        self.c.set(key, "test")
        self.c.expire(key, "5")
        ttl = self.c.ttl(key)
        self.assertTrue(int(ttl) > 3)

    def test_pexpire_pttl(self):
        key = "test_key"
        self.c.set(key, "test")
        self.c.pexpire(key, "3000")
        pttl = self.c.pttl(key)
        self.assertTrue(int(pttl) > 2000)

    def test_incr(self):
        key = "test_key"
        self.c.set(key, "3")
        self.c.incr(key)
        self.c.incrby(key, "2")
        self.assertTrue(self.c.get(key), "6")

    def test_parallel_incr(self):
        key = "test_key"
        c2 = self.pool.acquire()
        c3 = self.pool.acquire()
        self.c.set(key, "1")
        c2.incr(key)
        c3.incr(key)
        self.pool.reclaim(c2)
        self.pool.reclaim(c3)
        self.assertEqual(self.c.get(key), "3")

    def test_sorted_set(self):
        key = "zset_test"
        self.c.zincrby(key, "100", "toto")
        self.c.zincrby(key, "110", "tata")
        self.assertEqual(self.c.zrevrank(key, "toto"), 1)
        self.assertEqual(self.c.zscore(key, "toto"), "100")
        self.assertEqual(self.c.zcard(key), 2)
        res = self.c.zrevrange(key, "0", "100", withscores=True)
        p1, p2 = zip(islice(res, 0, None, 2), islice(res, 1, None, 2))
        self.assertEqual(p1[0], "tata")
        self.assertEqual(p1[1], "110")
        self.assertEqual(p2[0], "toto")
        self.assertEqual(p2[1], "100")
        self.assertEqual(self.c.zrevrank(key, "toto"), 1)

    def test_pipeline(self):
        p = self.pool.pipeline()
        p.set("k", "3")
        p.incr("k")
        p.delete("k")
        res = p.flush()
        self.assertEqual(res, ['OK', 4L, 1L])

    def test_nested_results(self):
        key = "nested_test"
        p = self.pool.pipeline()
        p.zincrby(key, "100", "toto")
        p.zincrby(key, "110", "tata")
        p.zrevrange(key, "0", "100", withscores=True)
        p.zrevrange(key, "0", "100")
        res = p.flush()
        self.assertEqual(res[2], ['tata', '110', 'toto', '100'])

    def test_long_pipeline(self):
        key = "test"
        p = self.pool.pipeline()
        for i in xrange(1000):
            p.zrevrange(key, 0, -1, withscores=True)
        p.flush()

if __name__ == "__main__":
    unittest.main()
