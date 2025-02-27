#pragma once

/**
 *
 *  @file
 *
 *  ETH driver (Zephyr).
 *
 */
#include <csp/interfaces/csp_if_eth.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Open RAW socket and add CSP interface.
 *
 * @param[in] device network interface name (Linux device).
 * @param[in] ifname ifname CSP interface name.
 * @param[in] mtu MTU for the transmitted ethernet frames.
 * @param[in] node_id CSP address of the interface.
 * @param[in] promisc if true, receive all CAN frames. If false a filter
 *                    is set before forwarding packets to the router
 * @param[out] return_iface the added interface.
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_eth_init(const char * device, const char * ifname, int mtu, unsigned int node_id, bool promisc, csp_iface_t ** return_iface);

/**
 * @brief Stop receiving ETH packets on the specified interface.
 *
 * This function stops the reception of ETH packets on the given CSP interface.
 *
 * @param[in] iface Pointer to the CSP interface on which to stop receiving ETH packets.
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_eth_stop_rx(csp_iface_t * iface);

#ifdef __cplusplus
}
#endif
