# Operating Systems Exercise 2 - A Variant of Netcat

## Overview

This project is a series of tasks designed to create a simplified version of the Netcat tool, a versatile utility used for network communications. The tasks include developing a tic-tac-toe game with a deliberately poor AI, and implementing various socket communication functionalities in a custom Netcat-like tool called `mync`.

## Assignment Tasks

### Step 1: The Worst Tic-Tac-Toe Player

Develop a program (`ttt`) that plays tic-tac-toe with a very simple strategy:
- The program receives a 9-digit number representing its strategy.
- The number must contain each digit from 1 to 9 exactly once.
- The program prioritizes moves based on the digits' positions from MSD (Most Significant Digit) to LSD (Least Significant Digit).
- On its turn, the program chooses the highest-priority available move.
- The program alternates moves with the user, printing the chosen move each turn.
- The game ends with the program printing `I win`, `I lost`, or `DRAW` depending on the outcome.

### Step 2: Basic Netcat-like Functionality 

Implement `mync`, a program that:
- Executes another program specified with the `-e` option and redirects its input and output.
- Example: `mync -e date` runs the `date` command.

### Step 3: Extended Functionality 

Enhance `mync` to support:
- Redirecting input with the `-i` option.
- Redirecting output with the `-o` option.
- Redirecting both input and output with the `-b` option.
- Support for TCP server and client.
- `TCPS<PORT>`: Start TCP server on port `<PORT>`.
- `TCPC<IP, PORT>`: Start TCP client connecting to `<IP>` on `<PORT>`.
- Example: `mync -e "ttt 123456789" -i TCPS4050` opens a TCP server on port 4050 for input.

### Step 4: Support for UDP Communication

Further enhance `mync` to support:
- `UDPS<PORT>`: Start a UDP server on port `<PORT>` (input only).
- `UDPC<IP, PORT>`: Start a UDP client connecting to `<IP>` on `<PORT>` (output only).
- Support for a timeout with the `-t` option.
- Example: `mync -e "ttt 123456789" -i UDPS4050 -t 10` starts a UDP server on port 4050 and terminates after 10 seconds.

### Step 5: TCP MUX Support

Enhance `mync` to support TCP MUX servers:
- `TCPMUXS<PORT>`: Start a TCP MUX server on port `<PORT>` supporting multiple clients using `select` or `poll`.
- Example: `mync -e "ttt 123456789" -b TCPMUXS4050`.

### Step 6: Unix Domain Sockets Support

Enhance `mync` to support Unix Domain Sockets:
- `UDSSD<path>`: Unix domain socket server for datagram communication (input only).
- `UDSCD<path>`: Unix domain socket client for datagram communication (output only).
- `UDSSS<path>`: Unix domain socket server for stream communication.
- `UDSCS<path>`: Unix domain socket client for stream communication.
