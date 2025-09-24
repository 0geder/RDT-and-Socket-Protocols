import time
from socket import *

def pinger_client():
    # Server details
    server_host = '127.0.0.1'  # localhost
    server_port = 12000
    
    # Create a UDP socket
    clientSocket = socket(AF_INET, SOCK_DGRAM)
    
    # Set a timeout of 1 second for the socket
    clientSocket.settimeout(1)
    
    print(f"Pinging {server_host}:{server_port}")
    
    # Send 10 pings
    for sequence_number in range(1, 11):
        # Get the current time as a float
        start_time = time.time()
        
        # Format the message
        message = f'Ping {sequence_number} {start_time}'
        
        try:
            # Send the message to the server
            clientSocket.sendto(message.encode(), (server_host, server_port))
            
            # Wait to receive the reply from the server
            modifiedMessage, serverAddress = clientSocket.recvfrom(1024)
            
            # Get the time when reply was received
            end_time = time.time()
            
            # Calculate Round Trip Time (RTT)
            rtt = end_time - start_time
            
            # Print the response and RTT
            print(f'Reply from {serverAddress[0]}: {modifiedMessage.decode()} | RTT: {rtt:.6f}s')
            
        except timeout:
            # If a 'timeout' exception occurs, the packet was lost
            print('Request timed out')
            
    # Close the socket
    clientSocket.close()

if __name__ == '__main__':
    pinger_client()