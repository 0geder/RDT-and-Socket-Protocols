from socket import *
import sys

def http_client():
    # Check for correct number of command-line arguments
    if len(sys.argv) != 4:
        print("Usage: python HttpClient.py <server_host> <server_port> <filename>")
        sys.exit()

    # Parse arguments
    server_host = sys.argv[1]
    server_port = int(sys.argv[2])
    filename = sys.argv[3]

    try:
        # Create a TCP socket
        clientSocket = socket(AF_INET, SOCK_STREAM)
        
        # Connect to the server
        print(f"Connecting to {server_host} on port {server_port}...")
        clientSocket.connect((server_host, server_port))
        
        # Construct the HTTP GET request
        request = f"GET /{filename} HTTP/1.1\r\nHost: {server_host}\r\n\r\n"
        
        # Send the request
        clientSocket.send(request.encode())
        
        # Receive and print the response from the server
        print("\n--- Server Response ---")
        response = ""
        while True:
            # Receive data in chunks
            data = clientSocket.recv(1024)
            if not data:
                break
            response += data.decode()
        
        print(response)

    except Exception as e:
        print(f"An error occurred: {e}")
        
    finally:
        # Close the socket
        clientSocket.close()

if __name__ == '__main__':
    http_client()