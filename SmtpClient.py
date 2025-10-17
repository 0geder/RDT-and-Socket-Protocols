from socket import *

def smtp_client():
    msg = "\r\n I love computer networks!"
    endmsg = "\r\n.\r\n"
    
    # Choose a mail server and call it mailserver
    # You MUST replace this with a valid, accessible SMTP server.
    # Port 25 is the standard, but many ISPs block it.
    # #Fill in start
    mailserver = ("localhost", 1025) # e.g., your university's SMTP server
    # #Fill in end
    
    # Create socket called clientSocket and establish a TCP connection with mailserver
    # #Fill in start
    clientSocket = socket(AF_INET, SOCK_STREAM)
    clientSocket.connect(mailserver)
    # #Fill in end
    
    recv = clientSocket.recv(1024).decode()
    print("S:", recv)
    if recv[:3] != '220':
        print('220 reply not received from server.')
        return

    # Send HELO command and print server response.
    heloCommand = 'HELO Alice\r\n'
    clientSocket.send(heloCommand.encode())
    recv1 = clientSocket.recv(1024).decode()
    print("S:", recv1)
    if recv1[:3] != '250':
        print('250 reply not received from server.')
        return
        
    # Send MAIL FROM command and print server response.
    # #Fill in start
    mailFrom = "MAIL FROM:<samson@test.com>\r\n" # Replace with your email
    clientSocket.send(mailFrom.encode())
    recv2 = clientSocket.recv(1024).decode()
    print("S:", recv2)
    if recv2[:3] != '250':
        print('250 reply not received from server.')
        return
    # #Fill in end
    
    # Send RCPT TO command and print server response.
    # #Fill in start
    rcptTo = "RCPT TO:<okuthe@test.com>\r\n" # Replace with recipient's email
    clientSocket.send(rcptTo.encode())
    recv3 = clientSocket.recv(1024).decode()
    print("S:", recv3)
    if recv3[:3] != '250':
        print('250 reply not received from server.')
        return
    # #Fill in end
    
    # Send DATA command and print server response.
    # #Fill in start
    dataCommand = "DATA\r\n"
    clientSocket.send(dataCommand.encode())
    recv4 = clientSocket.recv(1024).decode()
    print("S:", recv4)
    if recv4[:3] != '354':
        print('354 reply not received from server.')
        return
    # #Fill in end
    
    # Send message data.
    # #Fill in start
    # You can add email headers here for a proper email
    subject = "Subject: EEE3093S SMTP Test\r\n"
    clientSocket.send(subject.encode())
    clientSocket.send(msg.encode())
    # #Fill in end
    
    # Message ends with a single period.
    # #Fill in start
    clientSocket.send(endmsg.encode())
    recv5 = clientSocket.recv(1024).decode()
    print("S:", recv5)
    if recv5[:3] != '250':
        print('250 reply not received from server.')
        return
    # #Fill in end
    
    # Send QUIT command and get server response.
    # #Fill in start
    quitCommand = "QUIT\r\n"
    clientSocket.send(quitCommand.encode())
    recv6 = clientSocket.recv(1024).decode()
    print("S:", recv6)
    if recv6[:3] != '221':
        print('221 reply not received from server.')
    # #Fill in end

    clientSocket.close()

if __name__ == '__main__':
    smtp_client()