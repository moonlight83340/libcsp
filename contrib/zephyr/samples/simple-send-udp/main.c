#include <csp/csp.h>
#include <csp/drivers/udp_zephyr.h>

#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_mgmt.h>

LOG_MODULE_REGISTER(csp_sample_server_client);

#define CLIENT_ADDR 4
#define SERVER_ADDR 5
#define SERVER_PORT 10

#define DEFAULT_UDP_ADDRESS     "192.168.3.13"
#define DEFAULT_UDP_REMOTE_PORT 1500
#define DEFAULT_UDP_LOCAL_PORT  1500

/* main - initialization of CSP and start of client tasks */
int main(void) {
	LOG_INF("Initialising CSP");

	/* Init CSP */
	csp_init();
	csp_conf.version = 2;

    /* Interface config */
    csp_iface_t iface;
    csp_if_udp_conf_t conf = {
        .host = DEFAULT_UDP_ADDRESS,
        .lport = DEFAULT_UDP_LOCAL_PORT,
        .rport = DEFAULT_UDP_REMOTE_PORT
    };

    csp_udp_init(&iface, &conf);
    iface.is_default = 1;
	iface.addr = CLIENT_ADDR;

	while (1) {
		k_sleep(K_USEC(1000000));

		/* Prepare data */
		csp_packet_t *packet = csp_buffer_get_always();
		memcpy(packet->data, "abc", 3);
		packet->length = 3;

		/* Send */
		csp_sendto(CSP_PRIO_NORM, SERVER_ADDR, SERVER_PORT, SERVER_PORT, CSP_O_NONE, packet);
	}

	return 0;
}
