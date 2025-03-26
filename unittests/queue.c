#include <check.h>
#include "../include/csp/csp.h"

#define DEFAULT_TIMEOUT 1000

/* https://github.com/libcsp/libcsp/pull/707 */
START_TEST(test_queue_free_707)
{
	char item[] = "abc";

	int qlength = 10;
	int buf_size = qlength * sizeof(item);
	char buf[buf_size];

	csp_queue_handle_t qh;
	csp_static_queue_t q;

	/* zero clear */
	memset(buf, 0, buf_size);

	csp_init();

	/* create */
	qh = csp_queue_create_static(qlength, sizeof(item), buf, &q);
	ck_assert_int_eq(csp_queue_free(qh), qlength);

	/* enqueue */
	ck_assert_int_eq(csp_queue_enqueue(qh, item, DEFAULT_TIMEOUT), CSP_QUEUE_OK);
	ck_assert_int_eq(csp_queue_free(qh), qlength - 1);

}
END_TEST

START_TEST(test_queue_empty)
{
	char item1[] = "abc";

	int qlength = 10;
	int buf_size = qlength * sizeof(item1);
	char buf[buf_size];

	csp_queue_handle_t qh;
	csp_static_queue_t q;

	memset(buf, 0, buf_size);

	csp_init();

	qh = csp_queue_create_static(qlength, sizeof(item1), buf, &q);

	csp_queue_enqueue(qh, item1, 1000);
	csp_queue_enqueue(qh, item1, 1000);
	csp_queue_enqueue(qh, item1, 1000);
	csp_queue_enqueue(qh, item1, 1000);

	/* After enqueue, check the size */
	ck_assert_int_eq(4, csp_queue_size(qh));

	/* After empty, check the size */
	csp_queue_empty(qh);
	ck_assert_int_eq(0, csp_queue_size(qh));

	/* It's ok to call queue_empty on an empty queue */
	csp_queue_empty(qh);
	ck_assert_int_eq(0, csp_queue_size(qh));

	/* make it full */
	for (int i = 0; i < qlength; i++) {
		csp_queue_enqueue(qh, item1, 1000);
	}
	/* then empty it */
	csp_queue_empty(qh);
	ck_assert_int_eq(0, csp_queue_size(qh));
}
END_TEST

START_TEST(test_queue_dequeue_underflow)
{
	char item1[] = "abc";
	char item2[sizeof(item1)];

	int qlength = 10;
	int buf_size = qlength * sizeof(item1);
	char buf[buf_size];

	csp_queue_handle_t qh;
	csp_static_queue_t q;

	/* zero clear */
	memset(buf, 0, buf_size);
	memset(item2, 0, sizeof(item2));

	csp_init();

	/* create */
	qh = csp_queue_create_static(qlength, sizeof(item1), buf, &q);

	/* The queue is empty, so dequeue operation throws an error */
	ck_assert_int_eq(csp_queue_dequeue(qh, item2, 1000), CSP_QUEUE_ERROR);
}
END_TEST

Suite * queue_suite(void)
{
	Suite *s;
	TCase *tc_free;

	s = suite_create("Queue");

	tc_free = tcase_create("free");
	tcase_add_test(tc_free, test_queue_free_707);
	tcase_add_test(tc_free, test_queue_empty);
	tcase_add_test(tc_free, test_queue_dequeue_underflow);
	suite_add_tcase(s, tc_free);

	return s;
}
