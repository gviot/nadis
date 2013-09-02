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
	size_t initial_available_size = b.write_cursor.length;
	buffer_printf(&b, "Let's grow some %s !", "bytes");
	size_t new_available_size = b.write_cursor.length;
	ck_assert(b.read_cursor.length > 0);
	ck_assert(initial_available_size > new_available_size);
}
END_TEST

START_TEST (test_reach_end_of_buffer)
{
	circular_buffer b;
	const char *buf;
	int len, written, total_written;

	buffer_init(&b);
	const char * const dummy_text = "DUMMY_TEXT...";
	mark_point();
	while (total_written < (CIRCULAR_BUFFER_SIZE * 3 + 100)) {
		written = buffer_printf(&b, "%s%s", dummy_text, dummy_text);
		total_written += written;
		buffer_readable_bytes(&b, &buf, &len);
		buffer_consume_bytes(&b, len);
		ck_assert_int_eq(len, (strlen(dummy_text) * 2));
		ck_assert_int_eq(written, (strlen(dummy_text) * 2));
		ck_assert_int_gt(b.write_cursor.length, -1);
		ck_assert_int_eq(b.data + CIRCULAR_BUFFER_SIZE, (b.write_cursor.position + b.write_cursor.length));
		ck_assert_int_gt(CIRCULAR_BUFFER_SIZE, (b.write_cursor.position-b.data));
		mark_point();
	}
	mark_point();
}
END_TEST

Suite *
buffer_suite (void)
{
	Suite *s = suite_create("Circular Buffer");
	TCase *tc_write = tcase_create("write");
	tcase_add_test(tc_write, test_write_grows_size);
	tcase_add_test(tc_write, test_reach_end_of_buffer);
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
