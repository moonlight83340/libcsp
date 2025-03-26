#include <check.h>
#include "../include/csp/csp.h"

/* when using csp_buffer_get(), CSP will try to reserve the last two buffers */
#define BUFFER_RESERVED 2

/* https://github.com/libcsp/libcsp/issues/734 */
START_TEST(test_alloc_clean_734)
{
	uint8_t expected[CSP_BUFFER_SIZE];

	csp_init();

    /* use all buffer and free them */
    for (unsigned int i = 0; i < CSP_BUFFER_COUNT; i++) {
        csp_packet_t * packet = csp_buffer_get_always();
        memset(packet->data, 0, sizeof(packet->data)); /* clear buffer data */
        memcpy(packet->data, "previous_data!!", i+1);  /* put some data inside */
		packet->length = i + 1;
        csp_buffer_free(packet);
    }

	memset(expected, 0, sizeof(expected));

    /* access the data of previously used buffers */
    for (unsigned int i = 0; i < CSP_BUFFER_COUNT; i++) {
        csp_packet_t * packet = csp_buffer_get_always();
		ck_assert_mem_eq(packet->data, expected, sizeof(expected));
		ck_assert_int_eq(packet->length, 0);
        csp_buffer_free(packet);
    }
}
END_TEST

START_TEST(test_out_of_buffers) {
	int buffer_count = CSP_BUFFER_COUNT - BUFFER_RESERVED;
	csp_packet_t * packets[buffer_count];
	csp_packet_t * p;
	int i;

	csp_init();

	memset(packets, 0, sizeof(packets));

	for (i = 0; i < buffer_count; i++) {
		packets[i] = csp_buffer_get(0);
		ck_assert_ptr_nonnull(packets[i]);
	}

	ck_assert_int_eq(csp_buffer_remaining() - BUFFER_RESERVED, 0);
	p = csp_buffer_get(0);
	ck_assert_ptr_null(p);

	for (i = 0; i < buffer_count; i++) {
		csp_buffer_free(packets[i]);
	}
}
END_TEST

/* For test purpose we declare the internal struct here */
typedef struct csp_skbf_s {
	unsigned int refcount;
	void * skbf_addr;
	csp_packet_t skbf_data;
} csp_skbf_t;

#define CONTAINER_OF(ptr, type, member) \
	((type *)(void *)((char *)(ptr) - offsetof(type, member)))

START_TEST(test_corrupt_buffer) {
	csp_init();
	csp_packet_t * packet = csp_buffer_get_always();
	csp_skbf_t * buf = CONTAINER_OF(packet, csp_skbf_t, skbf_data);
	buf->skbf_addr = (void *)0xDEADBEEF;

	csp_buffer_free(packet);
	ck_assert_int_eq(csp_dbg_errno, CSP_DBG_ERR_CORRUPT_BUFFER);
}
END_TEST

START_TEST(test_buffer_clone) {
	csp_init();
	csp_packet_t * orig = csp_buffer_get_always();
	memcpy(orig->data, "original", 9);
	orig->length = 9;

	csp_packet_t * clone = csp_buffer_clone(orig);
	ck_assert_mem_eq(clone->data, orig->data, orig->length);
	ck_assert_int_eq(clone->length, orig->length);

	csp_buffer_free(orig);
	csp_buffer_free(clone);
}
END_TEST

Suite * buffer_suite(void) {
	Suite * s;
	TCase * tc_alloc;

	s = suite_create("Packet Buffer");

	tc_alloc = tcase_create("allocate");
	tcase_add_test(tc_alloc, test_alloc_clean_734);
	tcase_add_test(tc_alloc, test_out_of_buffers);
	tcase_add_test(tc_alloc, test_corrupt_buffer);
	tcase_add_test(tc_alloc, test_buffer_clone);
	suite_add_tcase(s, tc_alloc);

	return s;
}
