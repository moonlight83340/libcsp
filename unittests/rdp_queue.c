#include <check.h>
#include "../include/csp/csp.h"

void csp_rdp_queue_init(void);
int csp_rdp_queue_tx_size(void);
int csp_rdp_queue_rx_size(void);
void csp_rdp_queue_tx_add(csp_conn_t * conn, csp_packet_t * packet);
void csp_rdp_queue_rx_add(csp_conn_t * conn, csp_packet_t * packet);
csp_packet_t * csp_rdp_queue_tx_get(csp_conn_t * conn);
csp_packet_t * csp_rdp_queue_rx_get(csp_conn_t * conn);

START_TEST(test_rdp_queue_empty) {
	csp_rdp_queue_init();
	ck_assert_int_eq(csp_rdp_queue_tx_size(), 0);
	ck_assert_int_eq(csp_rdp_queue_rx_size(), 0);
	ck_assert_ptr_null(csp_rdp_queue_tx_get(NULL));
	ck_assert_ptr_null(csp_rdp_queue_rx_get(NULL));
}
END_TEST

START_TEST(test_rdp_queue_tx_add_and_get) {
	csp_packet_t *p1;
	csp_packet_t *p2;

	csp_rdp_queue_init();
	ck_assert_int_eq(csp_rdp_queue_tx_size(), 0);
	ck_assert_int_eq(csp_rdp_queue_rx_size(), 0);

	csp_buffer_init();
	p1 = csp_buffer_get(0);
	ck_assert_ptr_nonnull(p1);

	csp_rdp_queue_tx_add(NULL, p1);
	ck_assert_int_eq(csp_rdp_queue_tx_size(), 1);

	p2 = csp_rdp_queue_tx_get(NULL);
	ck_assert_int_eq(csp_rdp_queue_tx_size(), 0);
	ck_assert_ptr_nonnull(p2);

	ck_assert_ptr_eq(p1, p2);
}
END_TEST

START_TEST(test_rdp_queue_rx_add_and_get) {
	csp_packet_t *p1;
	csp_packet_t *p2;

	csp_rdp_queue_init();
	ck_assert_int_eq(csp_rdp_queue_rx_size(), 0);

	csp_buffer_init();
	p1 = csp_buffer_get(0);
	ck_assert_ptr_nonnull(p1);

	csp_rdp_queue_rx_add(NULL, p1);
	ck_assert_int_eq(csp_rdp_queue_rx_size(), 1);

	p2 = csp_rdp_queue_rx_get(NULL);
	ck_assert_int_eq(csp_rdp_queue_rx_size(), 0);
	ck_assert_ptr_nonnull(p2);

	ck_assert_ptr_eq(p1, p2);
}
END_TEST

Suite * rdp_queue_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("RDP_Queue");

	tc_core = tcase_create("Core");
	tcase_add_test(tc_core, test_rdp_queue_empty);
	tcase_add_test(tc_core, test_rdp_queue_tx_add_and_get);
	tcase_add_test(tc_core, test_rdp_queue_rx_add_and_get);
	suite_add_tcase(s, tc_core);

	return s;
}
