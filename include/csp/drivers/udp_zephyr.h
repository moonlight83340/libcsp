#pragma once

/**
 *
 *  @file
 *
 *  UDP driver (zephyr).
 *
 */

#include <csp/csp.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_context.h>

/**
 * Default interface name.
 */
#define CSP_IF_UDP_DEFAULT_NAME "UDP"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	/* Should be set before calling if_udp_init */
	char * host;
	int lport;
	int rport;

	/* Internal parameters */
	struct k_thread server_handle;
	struct net_context * udp_ctx;
	struct sockaddr_in peer_addr;
} csp_if_udp_conf_t;

/**
 * Setup UDP peer
 *
 * RX task:
 *   A server task will attempt at binding to ip 0.0.0.0 and the port chosen by the user in ifconf.
 *   If this fails, it is because another udp server is already running.
 *   The server task will continue attemting the bind and will not exit before the application is closed.
 *
 * TX peer:
 *   Outgoing CSP packets will be transferred to the peer specified by the host argument
 */
int csp_udp_init(csp_iface_t * iface, csp_if_udp_conf_t * ifconf);

#ifdef __cplusplus
}
#endif
