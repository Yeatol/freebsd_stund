# stund

Minimal STUN (RFC 8489) binding server for FreeBSD.

## Summary

`stund` is a lightweight UDP STUN server that answers RFC 8489 `BINDING`
requests and returns a mapped address in the `XOR-MAPPED-ADDRESS` attribute. The
project is intentionally small and does not require third-party libraries. It
is designed to use native FreeBSD networking primitives such as `kqueue(2)`,
`recvmmsg(2)`, and `SO_REUSEPORT_LB` to provide a simple and efficient NAT
traversal service.

## Features

- RFC 8489 STUN binding request support
- IPv4 and IPv6 address handling
- No third-party libraries; relies on the FreeBSD base system, C++ standard
  library, and pthread support
- FreeBSD rc.d startup script support
- Multi-threaded UDP server using FreeBSD `kqueue(2)`
- Starts one worker thread per CPU reported by `std::thread::hardware_concurrency()`
  and pins each thread to its corresponding CPU with `cpuset(2)`.
- **Zero dynamic memory allocation at runtime** (pre-allocated thread-local buffers and message rings)
- In-place response generation: rewrites each request in its receive buffer and sends it as the response

## Requirements

- FreeBSD 14 or later
- C++23-capable compiler and FreeBSD `make`

## Building

From the project root:

```sh
make
```

This produces the `stund` binary in the current working directory.

## Running

Start the server directly:

```sh
./stund
```

The server listens on UDP port `3478`. It runs in the foreground and
exits when it receives `SIGINT` or `SIGTERM`.

## Installation

Install into the system using the standard FreeBSD `make install` flow:

```sh
make install
```

This installs the `stund` executable to `/usr/local/sbin/stund` and the
`stund` rc.d service script to `/usr/local/etc/rc.d/stund`.

## rc.d service integration

The `stund.in` template is processed during the build to generate the rc.d
service script, which is installed as part of `make install`. The generated
script uses FreeBSD's `daemon(8)` utility to run and manage `stund` as a service.
The service runs the `stund` process as the `nobody` user.
Manage it using standard FreeBSD service controls such as:

```sh
service stund start
service stund status
service stund stop
```

To enable the service:

```sh
sysrc stund_enable="YES"
```

To disable the service:

```sh
sysrc -x stund_enable
```

## License

This project is licensed under the BSD 2-Clause "Simplified" License. See the
`LICENSE` file for full terms and conditions.
