#include <check.h>
#include <endian.h>
#include "../include/csp/csp.h"
#include "../include/csp/csp.h"
#include "../include/csp/csp_crc32.h"
#include "../include/csp/csp_id.h"

START_TEST(test_crc_verify_without_crc)
{
	csp_init();

	csp_packet_t *packet = csp_buffer_get(0);
	ck_assert_ptr_ne(packet, NULL);
	int ret;

	/* Check size error */
	packet->length = 0;
	ret = csp_crc32_verify(packet);
	ck_assert_int_eq(ret, CSP_ERR_CRC32);

	/* Check without crc */
	memcpy(packet->data, "Hello", 5);
	packet->length = 5;
	ret = csp_crc32_verify(packet);
	ck_assert_int_eq(ret, CSP_ERR_CRC32);

	csp_buffer_free(packet);
}
END_TEST

START_TEST(test_crc_without_header)
{
	csp_init();
	
	csp_packet_t *packet = csp_buffer_get(0);
	ck_assert_ptr_ne(packet, NULL);
	int ret;

	memcpy(packet->data, "Hello", 5);
	packet->length = 5;

	/* Check without header */
	csp_crc32_append(packet);
	ret = csp_crc32_verify(packet);
	ck_assert_int_eq(ret, CSP_ERR_NONE);

	csp_buffer_free(packet);
}
END_TEST

START_TEST(test_crc_with_header)
{
	csp_init();
	
	csp_packet_t *packet = csp_buffer_get(0);
	ck_assert_ptr_ne(packet, NULL);
	uint32_t crc;
	int ret;

	memcpy(packet->data, "Hello", 5);
	packet->length = 5;

	/* Check with header.
	 * This simulate CSP_21 define on csp_crc32_append().
	 */
	csp_id_prepend(packet);
	crc = csp_crc32_memory(packet->frame_begin, packet->frame_length);
	/* Convert to network byte order */
	crc = htobe32(crc);
	/* Copy checksum to packet */
	memcpy(&packet->data[packet->length], &crc, sizeof(crc));
	packet->length += sizeof(crc);

	ret = csp_crc32_verify(packet);
	ck_assert_int_eq(ret, CSP_ERR_NONE);

	csp_buffer_free(packet);
}
END_TEST

Suite *crc32_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("CRC32");
	tc_core = tcase_create("Core");

	tcase_add_test(tc_core, test_crc_verify_without_crc);
	tcase_add_test(tc_core, test_crc_without_header);
	tcase_add_test(tc_core, test_crc_with_header);

	suite_add_tcase(s, tc_core);

	return s;
}
