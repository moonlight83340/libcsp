# Simple Send via UDP

This is a simple sample code to send `"abc"` to a server via the
UDP interface.

## How to Build

```
$ cmake -B builddir
$ ninja -C builddir simple-send-udp simple-udp-server
```

## How to Test

To run you will need a csp udp server.

First, you need to run a CSP udp server:

```
$ ./builddir/samples/posix/simple-udp-server/simple-udp-server

```

Then, in another terminal, run `simple-send-udp`

```
$ ./builddir/samples/posix/simple-send-udp/simple-send-udp
```

If you successfully run `simple-send-udp`, you see the following
message on the udp server terminal.

```
Packet received on SERVER_PORT: abc
```
