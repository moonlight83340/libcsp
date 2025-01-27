import argparse
import threading
import random
import ctypes
import libcsp_py3 as csp

# Global variables
sender_crc = 0
receiver_crc = 0xFFFFFFFF
received_size = 0


def read_from_buffer(buffer, size, offset, data, data_crc):
    for i in range(size):
        buffer[i] = random.randint(0, 255)
    data_crc = csp.crc32_update(data_crc, buffer)
    return csp.CSP_ERR_NONE, data_crc


def write_to_buffer(buffer, size, offset, totalsz, data, data_crc):
    global received_size
    data_crc = csp.crc32_update(data_crc, buffer)
    received_size += size
    return csp.CSP_ERR_NONE, data_crc


def sender(addr: int, port: int, test_opts):
    global sender_crc
    data_crc = 0
    conn = None
    data_crc = csp.crc32_init(data_crc)

    try:
        opts = csp.CSP_O_CRC32 | (csp.CSP_O_RDP if test_opts.rdp else 0)
        print(f"Attempting to connect to server at {addr}:{port} with options {opts}")
        conn = csp.connect(csp.CSP_PRIO_NORM, addr, port, 1000, opts)
        if not conn:
            raise Exception("Failed to connect")
        print("Connection established")

        # Simulate reading and sending data
        buffer = bytearray(test_opts.mtu)
        for _ in range(test_opts.size // test_opts.mtu):
            print(f"Sending buffer of size {len(buffer)}")
            result, data_crc = read_from_buffer(buffer, len(buffer), 0, None, data_crc)
            if result != csp.CSP_ERR_NONE:
                raise Exception("Failed to read buffer")
            csp.sfp_send(conn, buffer, len(buffer), test_opts.mtu, 1000)
            print(f"Send buffer: {buffer}")

        sender_crc = csp.crc32_final(data_crc)
        print(f"Sender CRC: {sender_crc}")

    finally:
        if conn:
            print("Closing connection")
            csp.close(conn)


def receiver(port: int,test_opts):
    global receiver_crc
    data_crc = 0
    conn = None
    data_crc = csp.crc32_init(data_crc)

    try:
        opts = csp.CSP_SO_CRC32REQ | (csp.CSP_SO_RDPREQ if test_opts.rdp else 0)
        print(f"Setting up receiver with options {opts}")
        sock = csp.socket(opts)
        csp.bind(sock, csp.CSP_ANY)
        csp.listen(sock, port)

        print("Waiting for connection")
        conn = csp.accept(sock, 1000)
        if not conn:
            raise Exception("Failed to accept connection")
        print("Connection accepted")

        while True:
            print("Waiting for data...")
            result = csp.sfp_recv(conn, 1000)
            if result is None:
                print("No data received, breaking...")
            break
            
            data, size = result
            
            print(f"Received data of size {size}")

            data_packet=csp.packet_get_data(data)
            print(f"Received response: {data_packet}")
            print(f"Received data of size {size}")
            write_result, data_crc = write_to_buffer(data_packet, size, 0, test_opts.size, None, data_crc)

            if write_result != csp.CSP_ERR_NONE:
                raise Exception("Failed to write buffer")

        receiver_crc = csp.crc32_final(data_crc)
        print(f"Receiver CRC: {receiver_crc}")

    finally:
        if conn:
            print("Closing connection")
            csp.close(conn)


def main():
    parser = argparse.ArgumentParser(description="Test CSP SFP Protocol")
    parser.add_argument("--mtu", type=int, default=128, help="Maximum Transfer Unit (MTU) in bytes")
    parser.add_argument("--size", type=int, default=1000000, help="Total transfer size in bytes")
    parser.add_argument("--rdp", action="store_true", help="Enable RDP mode")
    args = parser.parse_args()

    print(f"Test options: MTU={args.mtu}, Size={args.size}, RDP={args.rdp}")

    # Initialize CSP
    print("Initializing CSP...")
    csp.init("", "", "")
    csp.route_start_task()

    serv_addr = 0
    serv_port = 10

    # Threads for server (receiver) and client (sender)
    sender_thread = threading.Thread(target=sender, args=(serv_addr, serv_port, args))
    receiver_thread = threading.Thread(target=receiver, args=(serv_port, args,))

    print("Starting server and client threads...")

    receiver_thread.start()
    sender_thread.start()

    #sender_thread.join(100000)
    #receiver_thread.join(100000)

    # Check results
    print("Test completed, checking results...")
    if sender_crc != receiver_crc:
        print("CRC mismatch!")
    elif received_size != args.size:
        print("Size mismatch!")
    else:
        print("Test completed successfully!")


if __name__ == "__main__":
    main()
