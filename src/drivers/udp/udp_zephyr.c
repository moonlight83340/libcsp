#include <csp/drivers/udp_zephyr.h>

#include <csp/csp_debug.h>

#include <csp/csp.h>
#include <csp/csp_id.h>
#include <csp/csp_interface.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(libcsp, CONFIG_LIBCSP_LOG_LEVEL);

static int csp_udp_tx(csp_iface_t * iface, uint16_t via, csp_packet_t * packet, int from_me) {
	csp_if_udp_conf_t * ifconf = iface->driver_data;

	if (!ifconf->udp_ctx) {
		LOG_ERR("Network context is NULL\n");
		csp_buffer_free(packet);
		return CSP_ERR_NONE;
	}

	csp_id_prepend(packet);
	ifconf->peer_addr.sin_family = AF_INET;
	ifconf->peer_addr.sin_port = htons(ifconf->rport);

	int ret = net_context_sendto(ifconf->udp_ctx, packet->frame_begin, packet->frame_length, (struct sockaddr *)&ifconf->peer_addr, sizeof(ifconf->peer_addr), NULL, K_NO_WAIT, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to send UDP packet (%d)\n", ret);
		csp_buffer_free(packet);
		return CSP_ERR_INVAL;
	}

	csp_buffer_free(packet);
	return CSP_ERR_NONE;
}

static void csp_udp_rx(struct net_context * context, struct net_pkt * pkt, union net_ip_header * ip_hdr, union net_proto_header * proto_hdr, int status, void * user_data) {
	if (!pkt) return;

	csp_iface_t * iface = (csp_iface_t *)user_data;
	csp_packet_t * packet = csp_buffer_get(0);

	if (!packet) {
		LOG_ERR("Can't get csp buffer : CSP_ERR_NOMEM\n");
		net_pkt_unref(pkt);
		return;
	}

	/* Setup RX frame to point to ID */
	int header_size = csp_id_setup_rx(packet);

	int received_len = net_pkt_remaining_data(pkt);
	if (received_len < header_size) {
		csp_buffer_free(packet);
		net_pkt_unref(pkt);
		return;
	}

	net_pkt_read(pkt, packet->frame_begin, received_len);
	packet->frame_length = received_len;
	net_pkt_unref(pkt);

	/* Parse the frame and strip the ID field */
	if (csp_id_strip(packet) != 0) {
		csp_buffer_free(packet);
		return;
	}

	csp_qfifo_write(packet, iface, NULL);
}

static int csp_udp_init_rx(csp_iface_t * iface) {
	int ret;
	struct net_if * net_iface;

	csp_if_udp_conf_t * ifconf = iface->driver_data;

	ret = net_context_get(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &ifconf->udp_ctx);
	if (ret < 0) {
		LOG_ERR("Cannot get UDP context (%d)\n", ret);
		return CSP_ERR_DRIVER;
	}

	struct sockaddr_in server_addr = {0};
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(ifconf->lport);

	net_iface = net_if_ipv4_select_src_iface(&server_addr.sin_addr);
	if (!net_iface) {
		LOG_ERR("No interface to send to given host\n");
		goto release_ctx;
	}

	net_context_set_iface(ifconf->udp_ctx, net_iface);

	ret = net_context_bind(ifconf->udp_ctx, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (ret < 0) {
		LOG_ERR("Binding to UDP port failed (%d)\n", ret);
		goto release_ctx;
	}

	ret = net_context_recv(ifconf->udp_ctx, csp_udp_rx, K_NO_WAIT, iface);
	if (ret < 0) {
		LOG_ERR("Receiving from UDP port failed (%d)\n", ret);
		goto release_ctx;
	}
	return CSP_ERR_NONE;
release_ctx:
	if (net_context_put(ifconf->udp_ctx) < 0) {
		LOG_ERR("Cannot put UDP context\n");
	}

	return CSP_ERR_DRIVER;
}

int csp_udp_init(csp_iface_t * iface, csp_if_udp_conf_t * ifconf) {
	iface->driver_data = ifconf;

	if (net_addr_pton(AF_INET, ifconf->host, &ifconf->peer_addr.sin_addr) < 0) {
		LOG_ERR("Invalid peer address: %s", ifconf->host);
		return CSP_ERR_INVAL;
	}

	char ip_str[NET_IPV4_ADDR_LEN];

	if (!net_addr_ntop(AF_INET, &ifconf->peer_addr.sin_addr, ip_str, sizeof(ip_str))) {
		LOG_ERR("Failed to convert IP address to string");
		return CSP_ERR_INVAL;
	}

	LOG_INF("UDP peer address: %s:%d (listening on port %d)\n", ip_str, ifconf->rport, ifconf->lport);

	/* Init udp rx */
	csp_udp_init_rx(iface);

	/* Register interface */
	iface->name = CSP_IF_UDP_DEFAULT_NAME,
	iface->nexthop = csp_udp_tx,
	csp_iflist_add(iface);

	return CSP_ERR_NONE;
}

int csp_udp_stop_rx(csp_iface_t * iface) {
	csp_if_udp_conf_t * ifconf = iface->driver_data;

	if (ifconf->udp_ctx) {
		/* Stop the rx thread */
		net_context_recv(ifconf->udp_ctx, NULL, K_NO_WAIT, NULL);

		/* Free the net context */
		net_context_put(ifconf->udp_ctx);
		ifconf->udp_ctx = NULL;
		LOG_INF("UDP reception stopped");

		/* Remove the interface */
		csp_iflist_remove(iface);
		return CSP_ERR_NONE;
	}
	LOG_ERR("NO UDP context to stop");
	return CSP_ERR_DRIVER;
}
