#include <stdio.h>
#include <csp/csp.h>
#include <csp/csp_debug.h>
#include <csp/interfaces/csp_if_udp.h>
#include <pthread.h>
#include <unistd.h>

#define SERVER_ADDR 5

#define DEFAULT_UDP_ADDRESS     "127.0.0.1"
#define DEFAULT_UDP_REMOTE_PORT (1500)
#define DEFAULT_UDP_LOCAL_PORT  (1500)

unsigned run_duration_in_sec = 10;

static int csp_pthread_create(void * (*routine)(void *)) {

	pthread_attr_t attributes;
	pthread_t handle;
	int ret;

	if (pthread_attr_init(&attributes) != 0) {
		return CSP_ERR_NOMEM;
	}
	/* no need to join with thread to free its resources */
	pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);

	ret = pthread_create(&handle, &attributes, routine, NULL);
	pthread_attr_destroy(&attributes);

	if (ret != 0) {
		return ret;
	}

	return CSP_ERR_NONE;
}

static void * task_router(void * param) {

	(void)param;

	/* Here there be routing */
	while (1) {
		csp_route_work();
	}

	return NULL;
}

static int router_start(void) {
	return csp_pthread_create(task_router);
}

void * server(void * param) {

	(void)param;

	csp_iface_t iface;
	csp_if_udp_conf_t conf = {
		.host = DEFAULT_UDP_ADDRESS,
		.lport = DEFAULT_UDP_LOCAL_PORT,
		.rport = DEFAULT_UDP_REMOTE_PORT};

	csp_if_udp_init(&iface, &conf);
	iface.addr = SERVER_ADDR;

	csp_socket_t sock = {0};
	csp_packet_t * packet;

	sock.opts = CSP_SO_CONN_LESS;

	/* Bind socket to all ports, e.g. all incoming connections will be handled here */
	csp_bind(&sock, CSP_ANY);

	/* Create a backlog of 10 connections, i.e. up to 10 new connections can be queued */
	csp_listen(&sock, 10);

	while (1) {
		packet = csp_recvfrom(&sock, 1000);
		if (packet == NULL) {
			continue;
		}

		printf("Packet received : %s\n", packet->data);

		csp_buffer_free(packet);
	}
}

int main(int argc, char * argv[]) {
	/* init */
	csp_init();

	router_start();

	/* start the server */
	csp_pthread_create(server);

	while (1) {
		sleep(run_duration_in_sec);
	}

	return 0;
}
