# Tiny Modbus

**WARNING**: this is still an early-stage work inprogress!

A tiny, header-only, portable and compliant Modbus implementation, suitable for embedded systems.

## Goals

These are the goals of this library:

-   portability: this library shall be easily ported on any system. It relies only on a few C standard library headers, and doesn't do any internal memory allocation
-   correctness: this library is developed by implementing the exact specification of the Modbus protocol
-   completeness: all the features of the Modbus specification are implemented

## Transport

The transport is the interface that must be implemented to connect the library to a TCP/IP socket or serial port.

It requires implementing only two functions: `read()` and `write()`, that have the typically API for a function that reads/writes to a serial port or socket.

## Posix transport

To simplify library usage on systems that have a POSIX API (such as Linux and UNIX-like OS, including macOS) a compatibility layer is added.

This layer is enabled only on such systems, allowing to create a transport without manually opening and configuring sockets and serial ports.
