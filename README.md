# TCP Shannon-Fano Encoder (Client & Server)

This project implements a networked compression system using TCP sockets.

## Features
- Multi-threaded client (POSIX `pthread`) sends multiple messages concurrently
- Forking TCP server encodes incoming messages using the Shannon-Fano algorithm
- Custom rules for sorting frequency tables and symbol prioritization

## Files
- `client.cpp` — reads messages and sends them to server in parallel
- `server.cpp` — encodes each message and responds with results

## Technologies
C++, Sockets, Threads, Fork, Shannon-Fano Encoding
