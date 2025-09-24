#import socket module 
from socket import *
import sys # In order to terminate the program

def web_server():
    serverSocket = socket(AF_INET, SOCK_STREAM)
    
    serverPort = 6789
    serverSocket.bind(('', serverPort))
    serverSocket.listen(1)
    
    while True:
        print('Ready to serve...')
        connectionSocket, addr = serverSocket.accept()
        
        try:
            message = connectionSocket.recv(1024).decode()
            
            if not message:
                continue

            filename = message.split()[1]
            f = open(filename[1:])
            outputdata = f.read()
            f.close()
            
            # Send one HTTP header line into socket
            header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
            connectionSocket.send(header.encode())
            
            # --- CHANGE 1: Send the entire file content at once ---
            connectionSocket.send(outputdata.encode())
            
            connectionSocket.close()
            
        except IOError:
            # Send response message for file not found
            header = "HTTP/1.1 404 Not Found\r\n\r\n"
            error_message = "<html><head></head><body><h1>404 Not Found</h1></body></html>\r\n"
            connectionSocket.send(header.encode())
            connectionSocket.send(error_message.encode())
            
            connectionSocket.close()

    serverSocket.close()
    sys.exit()
    
if __name__ == "__main__":
    web_server()