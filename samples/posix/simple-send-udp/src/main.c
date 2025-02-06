#include <string.h>
#include <csp/csp.h>
#include <csp/interfaces/csp_if_udp.h>

#define CLIENT_ADDR 4
#define SERVER_ADDR 5
#define SERVER_PORT 10

#define DEFAULT_UDP_ADDRESS     "127.0.0.1"
#define DEFAULT_UDP_REMOTE_PORT 1500
#define DEFAULT_UDP_LOCAL_PORT  1500

int main(int argc, char * argv[]) {
	/* Init */
	csp_init();

	/* Interface config */
	csp_iface_t iface;
	csp_if_udp_conf_t conf = {
		.host = DEFAULT_UDP_ADDRESS,
		.lport = DEFAULT_UDP_LOCAL_PORT,
		.rport = DEFAULT_UDP_REMOTE_PORT};

	/* init udp interface */
	csp_if_udp_init(&iface, &conf);
	iface.is_default = 1;
	iface.addr = CLIENT_ADDR;

	/* Prepare data */
	csp_packet_t * packet = csp_buffer_get_always();
	memcpy(packet->data, "abc", 3);
	packet->length = 3;

	/* Send */
	csp_sendto(CSP_PRIO_NORM, SERVER_ADDR, SERVER_PORT, SERVER_PORT, CSP_O_NONE, packet);

	return 0;
}
