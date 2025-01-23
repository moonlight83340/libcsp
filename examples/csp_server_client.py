"""
Usage: LD_LIBRARY_PATH=build PYTHONPATH=build python3 ./examples/csp_server_client.py
"""
import time
import threading
import sys
import argparse
import libcsp_py3 as csp
from typing import Any, Callable

server_received = 0
server_received_lock = threading.Lock()
stop_event = threading.Event()  # Event to signal threads to stop


def get_options():
    parser = argparse.ArgumentParser(description="Parses command.")
    parser.add_argument(
        "-t",
        "--test",
        type=int,
        help="Enable test mode and specify duration in seconds",
    )
    return parser.parse_args(sys.argv[1:])


def printer(node: str, color: str) -> Callable:
    def f(inp: str) -> None:
        print('{color}[{name}]: {inp}\033[0m'.format(
            color=color, name=node.upper(), inp=inp))

    return f


def server_task(addr: int, port: int) -> None:
    global server_received
    _print = printer('server', '\033[96m')
    _print('Starting server task')

    sock = csp.socket()
    csp.bind(sock, csp.CSP_ANY)

    csp.listen(sock, 10)

    while not stop_event.is_set():
        conn = csp.accept(sock, 10000)

        if conn is None:
            continue

        while (packet := csp.read(conn, 50)) is not None:
            if csp.conn_dport(conn) == port:
                _print('Received on {port}: {data}'.format(
                    port=port,
                    data=csp.packet_get_data(packet).decode('utf-8'))
                )
                with server_received_lock:
                    server_received += 1
            else:
                csp.service_handler(conn, packet)


def client_task(addr: int, port: int) -> None:
    _print = printer('client', '\033[92m')
    _print('Starting client task')

    count = ord('A')

    while not stop_event.is_set():
        time.sleep(1)

        ping = csp.ping(addr, 1000, 100, csp.CSP_O_NONE)
        _print('Ping {addr}: {ping}ms'.format(addr=addr, ping=ping))

        conn = csp.connect(csp.CSP_PRIO_NORM, addr, port, 1000, csp.CSP_O_NONE)
        if conn is None:
            raise Exception('Connection failed')

        packet = csp.buffer_get_always()

        if packet is None:
            raise Exception('Failed to get CSP buffer')

        data = bytes('Hello World {}'.format(chr(count)), 'ascii') + b'\x00'
        count += 1

        csp.packet_set_data(packet, data)
        csp.send(conn, packet)


def main() -> None:
    global server_received
    run_duration_in_sec = 3
    options = get_options()

    if options.test:
        run_duration_in_sec = options.test
        print(f"Running in test mode for {run_duration_in_sec} seconds...")

    csp.init("", "", "")
    csp.route_start_task()

    serv_addr = 0
    serv_port = 10
    threads = []

    for task in (server_task, client_task):
        t = threading.Thread(target=task, args=(serv_addr, serv_port))
        threads.append(t)
        t.start()

    print("Server and client started")


    while not stop_event.is_set():
        if options.test:
            time.sleep(run_duration_in_sec)
            with server_received_lock:
                if server_received < 5:
                    print(f"Server received {server_received} packets. Test failed!")
                    stop_event.set()
                    for t in threads:
                        t.join()
                    exit(1)
                elif server_received >= 5:
                    print(f"Server received {server_received} packets. Test passed!")
                    stop_event.set()
                    for t in threads:
                        t.join()
                    exit(0)


if __name__ == '__main__':
    main()
