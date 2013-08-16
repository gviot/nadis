from setuptools import setup, Extension

nadis_impl = Extension('nadis_impl',
    sources = [
      'src/driver.c',
      'src/parser.c',
      'src/serializer.c',
      'src/connection.c',
      'src/circular_buffer.c'
    ])

setup (name = "Nadis",
    version = "1.0",
    author = "Guillaume Viot",
    author_email = "guillaume.gvt@gmail.com",
    description = 'This is a Redis Python driver',
    keywords = "redis",
    ext_modules = [nadis_impl],
    py_modules = ["nadis"],
    test_suite = "nose.collector",
    tests_require = "nose")
