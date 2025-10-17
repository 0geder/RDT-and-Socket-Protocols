# RDT and Socket Protocols - EEE3093S

This repository contains my solutions for the programming assignments for the EEE3093S Computer Networks course at the University of Cape Town. The tasks cover fundamental networking concepts, from basic socket programming to implementing reliable data transfer protocols.

## Table of Contents
- [Prerequisites](#prerequisites)
- [Usage and Running Instructions](#usage-and-running-instructions)
  - [Task 1: TCP Web Server](#task-1-tcp-web-server)
  - [Task 2: UDP Pinger](#task-2-udp-pinger)
  - [Task 3: SMTP Mail Client](#task-3-smtp-mail-client)
  - [Task 4: RDT (Alternating-Bit-Protocol)](#task-4-rdt-alternating-bit-protocol)
  - [Task 5: RDT (Go-Back-N)](#task-5-rdt-go-back-n)
- [Author](#author)
- [License](#license)

---

## Prerequisites

To run the code in this repository, you will need the following installed on your system:

- **Python 3.x** for Tasks 1, 2, and 3.
- A **C compiler** (such as GCC) for Tasks 4 and 5.

All Python scripts use standard libraries, so no additional packages need to be installed via `pip`.

---

## Usage and Running Instructions

Below are the specific instructions for running the code for each task.

### Task 1: TCP Web Server

This task implements a simple, single-threaded TCP server that can serve one HTML file.

1.  Place an HTML file (e.g., `HelloWorld.html`) in the same directory as the script.
2.  Run the server from your terminal:
    ```bash
    python WebServer.py
    ```
3.  Open a web browser and navigate to `http://localhost:6789/HelloWorld.html` to see the file.
4.  To test the error handling, navigate to a file that does not exist, such as `http://localhost:6789/NoSuchFile.html`.

### Task 2: UDP Pinger

This task consists of a UDP server that simulates packet loss and a client that calculates Round Trip Time (RTT).

1.  Open two separate terminal windows.
2.  In the **first terminal**, start the server:
    ```bash
    python UDPPingerServer.py
    ```
3.  In the **second terminal**, run the client:
    ```bash
    python UDPPingerClient.py
    ```
4.  The client will send 10 pings. The output will show successful replies with their RTTs or "Request timed out" for simulated lost packets.

### Task 3: SMTP Mail Client

Implements a minimal SMTP client that sends an unencrypted email. Because most public mail providers require TLS and authentication, this repository uses a local debugging SMTP server to test the client without credentials.

Prerequisites

- Python 3.x
- The development-only package `aiosmtpd` (only required for the local debug server):

```powershell
pip install aiosmtpd
```

Quick steps (PowerShell)

1. Open two separate terminal windows.

2. In the first terminal, start a local debugging SMTP server (it will print received emails to the console):

```powershell
python -m aiosmtpd -n -l localhost:1025 -c aiosmtpd.handlers.Debugging
```

3. In the second terminal, run the SMTP client (the client is preconfigured to use localhost:1025):

```powershell
python SmtpClient.py
```

What to expect

- The debugging server (first terminal) will print the full SMTP transaction and the received message, including headers and the body (the example client sends the message "I love computer networks!").
- No real email is sent — this is a local test only.

Troubleshooting

- If you see an import error for `aiosmtpd`, make sure you installed it in the same Python environment you use to run the server.
- If the client fails to connect, verify the server is running on `localhost:1025` and no firewall is blocking that port.
- To test with a real SMTP provider (Gmail, Outlook), you will need to update `SmtpClient.py` to use TLS and proper authentication. Do not use plaintext credentials in public repos.

Notes

- The local debugging server is intended for development and testing only. It accepts unauthenticated, unencrypted connections and prints incoming messages to stdout.
- For production use or sending real mail, use an authenticated TLS-capable SMTP library or service.

### Task 4: RDT (Alternating-Bit-Protocol)

This task implements a stop-and-wait reliable data transfer protocol (`rdt3.0`) in C.

1.  Compile the program using GCC (or another C compiler):
    ```bash
    gcc prog2_abp.c -o rdt_abp_simulator
    ```
2.  Run the compiled simulator:
    ```bash
    ./rdt_abp_simulator
    ```
3.  The program will prompt you to enter parameters for the simulation, such as packet loss and corruption probability.

### Task 5: RDT (Go-Back-N)

This task implements the Go-Back-N reliable data transfer protocol in C.

1.  Compile the program using GCC:
    ```bash
    gcc prog2_gbn.c -o rdt_gbn_simulator
    ```
2.  Run the compiled simulator:
    ```bash
    ./rdt_gbn_simulator
    ```
3.  The program will prompt you to enter simulation parameters.

---

## Author

👤 **Samson Okuthe**

- GitHub: [@0geder](https://github.com/0geder)
- Email: [oktsam001@myuct.ac.za](mailto:oktsam001@myuct.ac.za)

---

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details.