from socket import *
import sys
import threading

# This function will handle a single client connection.
# It will run in its own separate thread.
def handle_client(connectionSocket, addr):
    print(f"Accepted connection from {addr}")
    try:
        message = connectionSocket.recv(1024).decode()
        if not message:
            connectionSocket.close()
            return

        filename = message.split()[1]
        f = open(filename[1:])
        outputdata = f.read()
        f.close()

        # Send HTTP OK header and the file content
        header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
        connectionSocket.send(header.encode())
        connectionSocket.send(outputdata.encode())
        
    except IOError:
        # Send 404 Not Found response
        header = "HTTP/1.1 404 Not Found\r\n\r\n"
        error_message = "<html><head></head><body><h1>404 Not Found</h1></body></html>"
        connectionSocket.send(header.encode())
        connectionSocket.send(error_message.encode())
        
    finally:
        # Close the connection with this specific client
        print(f"Closing connection with {addr}")
        connectionSocket.close()

def main():
    serverSocket = socket(AF_INET, SOCK_STREAM)
    serverPort = 6789
    serverSocket.bind(('', serverPort))
    serverSocket.listen(5) # Listen for up to 5 connections
    
    print(f"Server is ready and listening on port {serverPort}")

    while True:
        # Main thread waits for a new connection
        connectionSocket, addr = serverSocket.accept()
        
        # Create a new thread to handle this client
        client_thread = threading.Thread(target=handle_client, args=(connectionSocket, addr))
        client_thread.start()

if __name__ == "__main__":
    main()