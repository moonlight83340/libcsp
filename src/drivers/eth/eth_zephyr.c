#include <csp/csp.h>
#include <csp/csp_id.h>
#include <csp/csp_interface.h>
#include <csp/drivers/eth_zephyr.h>
#include <csp/interfaces/csp_if_eth.h>

#include <zephyr/device.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/kernel/thread.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_DECLARE(libcsp, CONFIG_LIBCSP_LOG_LEVEL);

extern bool eth_debug;

typedef struct {
	char name[CSP_IFLIST_NAME_MAX + 1];
	csp_eth_interface_data_t ifdata;
	struct net_context * eth_ctx;
	struct k_thread rx_thread;
	int if_index;
} eth_context_t;

static int get_ifindex_by_name(const char * iface_name) {
	struct net_if * iface = NULL;
	const struct device * dev = device_get_binding(iface_name);

	if (!dev) {
		LOG_ERR("Interface %s not found!", iface_name);
		return -1;
	}

	iface = net_if_lookup_by_dev(dev);
	if (!iface) {
		LOG_ERR("No net_if found for device %s", iface_name);
		return -1;
	}

	return net_if_get_by_iface(iface);
}

int csp_eth_tx_frame(void * driver_data, csp_eth_header_t * eth_frame) {
	eth_context_t * ctx = (eth_context_t *)driver_data;

	/* Destination socket address */
	struct sockaddr_ll remote_addr = {0};
	remote_addr.sll_ifindex = ctx->if_index;
	remote_addr.sll_halen = CSP_ETH_ALEN;
	memcpy(remote_addr.sll_addr, eth_frame->ether_dhost, CSP_ETH_ALEN);

	uint32_t txsize = sizeof(csp_eth_header_t) + sys_be16_to_cpu(eth_frame->seg_size);

	int ret = net_context_sendto(ctx->eth_ctx, (void *)eth_frame, txsize, (struct sockaddr *)&remote_addr, sizeof(remote_addr), NULL, K_NO_WAIT, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to send ETH packet");
		return CSP_ERR_DRIVER;
	}

	return CSP_ERR_NONE;
}

static void csp_eth_rx_frame(struct net_context * context, struct net_pkt * pkt, union net_ip_header * ip_hdr, union net_proto_header * proto_hdr, int status, void * user_data) {
	if (!pkt) return;
	eth_context_t * ctx = (eth_context_t *)user_data;
	static uint8_t recvbuf[CSP_ETH_BUF_SIZE];
	csp_eth_header_t * eth_frame = (csp_eth_header_t *)recvbuf;

	uint32_t received_len = net_pkt_remaining_data(pkt);
	if (received_len > CSP_ETH_BUF_SIZE) {
		received_len = CSP_ETH_BUF_SIZE;
	}

	net_pkt_read(pkt, recvbuf, received_len);
	net_pkt_unref(pkt);

	csp_eth_rx(&ctx->ifdata.iface, eth_frame, received_len, NULL);
}

int csp_eth_init_rx(eth_context_t * ctx) {
	LOG_INF("csp_eth_init_rx\n");
	int ret;

	/* fill sockaddr_ll struct to prepare binding */
	struct sockaddr_ll local_addr = {0};
	local_addr.sll_ifindex = ctx->if_index;
	local_addr.sll_family = AF_PACKET;
	local_addr.sll_protocol = htons(CSP_ETH_TYPE_CSP);

	/* bind socket  */
	ret = net_context_bind(ctx->eth_ctx, (struct sockaddr *)&local_addr, sizeof(struct sockaddr_ll));
	if (ret < 0) {
		LOG_ERR("Failed to bind net_context");
		k_free(ctx);
		return CSP_ERR_DRIVER;
	}

	ret = net_context_recv(ctx->eth_ctx, csp_eth_rx_frame, K_NO_WAIT, ctx);
	if (ret < 0) {
		LOG_ERR("Receiving from ETH port failed (%d)\n", ret);
		if (net_context_put(ctx->eth_ctx) < 0) {
			LOG_ERR("Cannot put ETH context\n");
		}
		return CSP_ERR_DRIVER;
	}

	return CSP_ERR_NONE;
}

static uint8_t csp_eth_tx_buffer[CSP_ETH_BUF_SIZE];

int csp_eth_init(const char * device, const char * ifname, int mtu, unsigned int node_id, bool promisc, csp_iface_t ** return_iface) {
	/* TODO: remove dynamic memory allocation */
	eth_context_t * ctx = k_calloc(1, sizeof(eth_context_t));
	if (ctx == NULL) {
		LOG_ERR("%s: [%s] Failed to allocate %zu bytes from the system heap\n", __func__, device, sizeof(eth_context_t));
		return CSP_ERR_NOMEM;
	}

	strcpy(ctx->name, ifname);
	ctx->ifdata.iface.name = ctx->name;
	ctx->ifdata.tx_func = &csp_eth_tx_frame;
	ctx->ifdata.tx_buf = (csp_eth_header_t *)&csp_eth_tx_buffer;
	ctx->ifdata.iface.nexthop = &csp_eth_tx,
	ctx->ifdata.iface.addr = node_id;
	ctx->ifdata.iface.driver_data = ctx;
	ctx->ifdata.iface.interface_data = &ctx->ifdata;
	ctx->ifdata.promisc = promisc;

	/* Ether header 14 byte, seg header 4 byte, CSP header 6 byte */
	if (mtu < 24) {
		csp_print("csp_if_eth_init: mtu < 24\n");
		k_free(ctx);
		return CSP_ERR_INVAL;
	}

	int ret = net_context_get(AF_PACKET, SOCK_RAW, htons(CSP_ETH_TYPE_CSP), &ctx->eth_ctx);
	if (ret < 0) {
		LOG_ERR("Failed to create net_context");
		k_free(ctx);
		return CSP_ERR_DRIVER;
	}

	/* Get the index of the interface to send on */
	ctx->if_index = get_ifindex_by_name(device);

	if (ctx->if_index > 0) {
		LOG_INF("Interface %s has index %d", device, ctx->if_index);
	} else {
		LOG_ERR("Failed to get index for interface %s", device);
		k_free(ctx);
		return CSP_ERR_DRIVER;
	}

	ctx->ifdata.tx_mtu = mtu;

	/* Init eth rx */
    if(csp_eth_init_rx(ctx) != CSP_ERR_NONE) {
		LOG_ERR("Failed to init ETH RX");
		return CSP_ERR_DRIVER;
	}

	/**
	 * CSP INTERFACE
	 */

	/* Register interface */
	csp_iflist_add(&ctx->ifdata.iface);

	if (return_iface) {
		*return_iface = &ctx->ifdata.iface;
	}

	return CSP_ERR_NONE;
}

int csp_eth_stop_rx(csp_iface_t * iface) {
	eth_context_t * ctx = iface->driver_data;

	if (ctx->eth_ctx) {
		/* Stop the rx thread */
		net_context_recv(ctx->eth_ctx, NULL, K_NO_WAIT, NULL);

		/* Free the net context */
		net_context_put(ctx->eth_ctx);
		ctx->eth_ctx = NULL;
		LOG_INF("ETH reception stopped");

		/* Remove the interface */
		csp_iflist_remove(iface);
		return CSP_ERR_NONE;
	}
	LOG_ERR("No ETH context to stop");
	return CSP_ERR_DRIVER;
}
