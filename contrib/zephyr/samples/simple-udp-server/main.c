#include <csp/csp.h>
#include <csp/drivers/udp_zephyr.h>

#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_mgmt.h>

LOG_MODULE_REGISTER(csp_sample_udp_server);

#define SERVER_ADDR 5
#define SERVER_PORT 10

#define DEFAULT_UDP_ADDRESS     "192.168.3.3"
#define DEFAULT_UDP_REMOTE_PORT 1500
#define DEFAULT_UDP_LOCAL_PORT  55984

/* These two functions must be provided in arch specific way */
void router_start(void);
void server_start(void);

/* test mode, used for verifying that host & client can exchange packets */
static bool test_mode = false;
static unsigned int server_received = 0;

/* Server task - handles requests from clients */
void server(void) {

	LOG_INF("Server task started");

	csp_iface_t iface;
	csp_if_udp_conf_t conf;

	/* open */
	conf.host = DEFAULT_UDP_ADDRESS;
	conf.lport = DEFAULT_UDP_LOCAL_PORT;
	conf.rport = DEFAULT_UDP_REMOTE_PORT;
	csp_udp_init(&iface, &conf);
	iface.addr = SERVER_ADDR;

	csp_socket_t sock = {0};
	csp_packet_t * packet;

	sock.opts = CSP_SO_CONN_LESS;

	/* Bind socket to all ports, e.g. all incoming connections will be handled here */
	csp_bind(&sock, CSP_ANY);

	/* Create a backlog of 10 connections, i.e. up to 10 new connections can be queued */
	csp_listen(&sock, 10);

	/* Wait for connections and then process packets on the connection */
	while (1) {
		packet = csp_recvfrom(&sock, 1000);
		if (packet == NULL) {
			continue;
		}

		printf("Packet received : %s\n", packet->data);
		server_received++;

		csp_buffer_free(packet);
	}

	return;
}
/* End of server task */

/* main - initialization of CSP and start of server/client tasks */
int main(void) {
	int ret;

	LOG_INF("Initialising CSP");

	/* Init CSP */
	csp_init();

	csp_conf.version = 2;

	/* Start router */
	router_start();

	/* Start server thread */
	server_start();

	/* Wait for execution to end (ctrl+c) */
	while (1) {
		k_sleep(K_SECONDS(3));

		if (test_mode) {
			/* Test mode is intended for checking that host & client can exchange packets over loopback */
			if (server_received < 5) {
				LOG_INF("Server received %u packets", server_received);
				ret = 1;
				goto end;
			}
			LOG_INF("Server received %u packets", server_received);
			ret = 0;
			goto end;
		}
	}

end:
	return ret;
}
