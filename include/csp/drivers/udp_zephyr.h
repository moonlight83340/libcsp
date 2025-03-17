#pragma once

/**
 *
 * @file
 * @brief UDP zephyr driver using zephyr net context.
 * This driver implements a UDP interface on Zephyr using `net_context` API for network communication.
 *
 */

#include <csp/csp.h>

#include <zephyr/net/net_context.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	/* Should be set before calling csp_udp_init */
	char * host;
	int lport;
	int rport;

	/* Internal parameters */
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
 *
 * @param[in] iface Pointer to the CSP interface structure to be initialized.
 * @param[in] ifconf Pointer to the UDP interface configuration structure.
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_udp_init(csp_iface_t * iface, csp_if_udp_conf_t * ifconf);

/**
 * @brief Stops the UDP interface.
 *
 * This function stops the specified UDP interface, terminating any ongoing
 * communication and releasing associated resources.
 *
 * @param[in] iface Pointer to the CSP interface structure.
 *
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_udp_stop(csp_iface_t * iface);

#ifdef __cplusplus
}
#endif
