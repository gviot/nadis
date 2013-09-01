#include <stdlib.h>
#include <check.h>
#include <Python.h>
#include "../src/circular_buffer.h"

PyObject *ConnectionError;

START_TEST (test_write_grows_size)
{
	circular_buffer b;
	buffer_init(&b);
	ck_assert(b.read_cursor.length == 0);
	buffer_printf(&b, "Let's grow some %s !", "bytes");
	ck_assert(b.read_cursor.length > 0);
}
END_TEST

Suite *
buffer_suite (void)
{
	Suite *s = suite_create("Circular Buffer");
	TCase *tc_write = tcase_create("write");
	tcase_add_test(tc_write, test_write_grows_size);
	suite_add_tcase(s, tc_write);
	return s;
}

int
main (void)
{
  int number_failed;
  Suite *s = buffer_suite ();
  SRunner *sr = srunner_create (s);
  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
