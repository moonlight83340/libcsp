#include <csp/interfaces/csp_if_udp.h>

#include <csp/csp_debug.h>

#include <csp/csp.h>
#include <csp/csp_interface.h>
#include <csp/csp_id.h>

#if (CSP_ZEPHYR)
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(libcsp, CONFIG_LIBCSP_LOG_LEVEL);
static K_THREAD_STACK_ARRAY_DEFINE(rx_stack,
								   CONFIG_CSP_UDP_RX_THREAD_NUM, CONFIG_CSP_UDP_RX_THREAD_STACK_SIZE);
static uint8_t rx_thread_idx = 0;
#else
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <endian.h>
#endif

#ifndef MSG_CONFIRM
#define MSG_CONFIRM (0)
#endif

static int csp_if_udp_tx(csp_iface_t * iface, uint16_t via, csp_packet_t * packet, int from_me) {

	csp_if_udp_conf_t * ifconf = iface->driver_data;

	if (ifconf->sockfd <= 0) {
		csp_print("Sockfd null\n");
		csp_buffer_free(packet);
		return CSP_ERR_NONE;
	}

	csp_id_prepend(packet);
	ifconf->peer_addr.sin_family = AF_INET;
	ifconf->peer_addr.sin_port = htons(ifconf->rport);
	sendto(ifconf->sockfd, packet->frame_begin, packet->frame_length, MSG_CONFIRM, (struct sockaddr *)&ifconf->peer_addr, sizeof(ifconf->peer_addr));
	csp_buffer_free(packet);

	return CSP_ERR_NONE;
}

int csp_if_udp_rx_work(int sockfd, size_t unused, csp_iface_t * iface) {

#if (CSP_ZEPHYR)
	LOG_INF("UDP csp_if_udp_rx_work\n");
#endif

	csp_packet_t * packet = csp_buffer_get(0);
	if (packet == NULL) {
#if (CSP_ZEPHYR)
		LOG_INF("UDP csp_if_udp_rx_work: CSP_ERR_NOMEM\n");
#endif
		return CSP_ERR_NOMEM;
	}

	/* Setup RX frane to point to ID */
	int header_size = csp_id_setup_rx(packet);

#if (CSP_ZEPHYR)
	LOG_INF("UDP csp_if_udp_rx_work: recvfrom\n");
#endif

	int received_len = recvfrom(sockfd, (char *)packet->frame_begin, sizeof(packet->data) + header_size, MSG_WAITALL, NULL, NULL);

#if (CSP_ZEPHYR)
	LOG_INF("UDP csp_if_udp_rx_work: received_len : %d\n", received_len);
#endif

	if (received_len < header_size) {
		csp_buffer_free(packet);
		return CSP_ERR_NOMEM;
	}

	packet->frame_length = received_len;

	/* Parse the frame and strip the ID field */
	if (csp_id_strip(packet) != 0) {
		csp_buffer_free(packet);
		return CSP_ERR_INVAL;
	}

#if (CSP_ZEPHYR)
	LOG_INF("UDP csp_if_udp_rx_work : csp_qfifo_write\n");
#endif

	csp_qfifo_write(packet, iface, NULL);

	return CSP_ERR_NONE;
}

static void check_socket_info(int sock) {
	struct sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);
	char addr_buf[NET_IPV4_ADDR_LEN];

	if (getsockname(sock, (struct sockaddr *)&addr, &addr_len) == 0) {
		if (net_addr_ntop(AF_INET, &addr.sin_addr, addr_buf, sizeof(addr_buf)) == NULL) {
			LOG_ERR("Failed to convert IP address\n");
		} else {
			LOG_INF("Listening on IP: %s, Port: %d", addr_buf, ntohs(addr.sin_port));
		}
	} else {
		LOG_ERR("Failed to get socket info, errno: %d", errno);
	}
}

void * if_udp_rx_loop_impl(void * param) {
	csp_iface_t * iface = param;
	csp_if_udp_conf_t * ifconf = iface->driver_data;

	while (ifconf->sockfd <= 0) {

		ifconf->sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		struct sockaddr_in server_addr;
		(void)memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		server_addr.sin_port = htons(ifconf->lport);

#if (CSP_ZEPHYR)
		LOG_INF("UDP server ifconf->lport %d\n", ifconf->lport);
#endif

		bind(ifconf->sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));

		check_socket_info(ifconf->sockfd);

#if (CSP_ZEPHYR)
		LOG_INF("UDP server ifconf->sockfd %d\n", ifconf->sockfd);
		k_sleep(K_NSEC(1));
#else
		csp_print("UDP server ifconf->sockfd %d\n", ifconf->sockfd);
		sleep(1);
#endif

		if (ifconf->sockfd < 0) {
			// #if (CSP_ZEPHYR)
			// 	LOG_INF("UDP server waiting for port %d\n", ifconf->lport);
			// 	k_sleep(K_NSEC(1));
			// #else
			csp_print("UDP server waiting for port %d\n", ifconf->lport);
			sleep(1);
			// #endif
			continue;
		}
		break;
	}

	while (1) {
		int ret;
		ret = csp_if_udp_rx_work(ifconf->sockfd, 0, iface);
		if (ret == CSP_ERR_INVAL) {
			iface->rx_error++;
		} else if (ret == CSP_ERR_NOMEM) {
#if (CSP_ZEPHYR)
			k_sleep(K_MSEC(10));
#else
			usleep(10000);
#endif
		}
	}

	return NULL;
}

#if (CSP_ZEPHYR)
void csp_if_udp_rx_loop(void * arg1, void * arg2, void * arg3) {

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	if_udp_rx_loop_impl(arg1);
}
#else
void * csp_if_udp_rx_loop(void * param) {
	if_udp_rx_loop_impl(param);
}
#endif

void csp_if_udp_init(csp_iface_t * iface, csp_if_udp_conf_t * ifconf) {

#if (!CSP_ZEPHYR)
	pthread_attr_t attributes;
#endif

	iface->driver_data = ifconf;

#if (CSP_ZEPHYR)
	if (inet_pton(AF_INET, ifconf->host, &ifconf->peer_addr.sin_addr) == 0) {
		LOG_ERR("Invalid peer address: %s", ifconf->host);
	}
#else
	if (inet_aton(ifconf->host, &ifconf->peer_addr.sin_addr) == 0) {
		csp_print("Invalid peer address %s\n", ifconf->host);
	}
#endif

#if (CSP_ZEPHYR)
	LOG_INF("UDP peer address: %s:%d (listening on port %d)", ifconf->host, ifconf->rport, ifconf->lport);
#else
	csp_print("  UDP peer address: %s:%d (listening on port %d)\n", inet_ntoa(ifconf->peer_addr.sin_addr), ifconf->rport, ifconf->lport);
#endif

/* Start server thread */
#if (CSP_ZEPHYR)
	k_tid_t rx_tid = k_thread_create(&ifconf->server_handle, rx_stack[rx_thread_idx],
									 K_THREAD_STACK_SIZEOF(rx_stack[rx_thread_idx]),
									 (k_thread_entry_t)csp_if_udp_rx_loop, iface, NULL, NULL,
									 CONFIG_CSP_UDP_RX_THREAD_PRIORITY, 0, K_NO_WAIT);
	if (!rx_tid) {
		LOG_ERR("[UDP] k_thread_create() failed");
		return;
	}
	rx_thread_idx++;
#else
	ret = pthread_attr_init(&attributes);
	if (ret != 0) {
		csp_print("csp_if_udp_init: pthread_attr_init failed: %s: %d\n", strerror(ret), ret);
	}
	ret = pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
	if (ret != 0) {
		csp_print("csp_if_udp_init: pthread_attr_setdetachstate failed: %s: %d\n", strerror(ret), ret);
	}
	ret = pthread_create(&ifconf->server_handle, &attributes, csp_if_udp_rx_loop, iface);
	if (ret != 0) {
		csp_print("csp_if_udp_init: pthread_create failed: %s: %d\n", strerror(ret), ret);
	}
	ret = pthread_attr_destroy(&attributes);
	if (ret != 0) {
		csp_print("csp_if_udp_init: pthread_attr_destroy failed: %s: %d\n", strerror(ret), ret);
	}
#endif

	/* Register interface */
	iface->name = "UDP",
	iface->nexthop = csp_if_udp_tx,
	csp_iflist_add(iface);
}
