#include <stdio.h>
#include <string.h>

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, EEE3093S, at the University of Cape Town.
   It has been generously provided by J.F.Kurose, University of Massachusetts.
**********************************************************************/

#define BIDIRECTIONAL 0

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (your code).  It contains the data (characters) to be delivered */
/* to layer 5 running on the other side of the network.         */
struct msg {
  char data[20];
};

/* a packet is the data unit passed from layer 4 (your code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, you can not */
/* change it. */
struct pkt {
   int seqnum;
   int acknum;
   int checksum;
   char payload[20];
};

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

// Define states for sender A
#define WAITING_FOR_CALL 0
#define WAITING_FOR_ACK  1

// Global variables for sender A
int A_state;
int A_seqnum;
struct pkt A_last_packet;
float timer_increment = 30.0; // Timeout duration

// Global variable for receiver B
int B_expected_seqnum;

/* Helper function to calculate checksum */
int calculate_checksum(struct pkt packet) {
    int sum = 0;
    sum += packet.seqnum;
    sum += packet.acknum;
    for (int i = 0; i < 20; i++) {
        sum += (unsigned char)packet.payload[i];
    }
    return sum;
}

/* called from layer 5, passed the data to be sent to other side */
A_output(message)
  struct msg message;
{
    // If sender is not ready (still waiting for an ACK), drop the message.
    if (A_state == WAITING_FOR_ACK) {
        printf("  A_output: Sender busy. Dropping message.\n");
        return;
    }
    
    // Create the packet
    A_last_packet.seqnum = A_seqnum;
    A_last_packet.acknum = 0; // Not used for data packets
    memcpy(A_last_packet.payload, message.data, 20);
    A_last_packet.checksum = calculate_checksum(A_last_packet);

    // Send the packet and start the timer
    printf("  A_output: Sending packet with seq=%d\n", A_seqnum);
    tolayer3(0, A_last_packet);
    starttimer(0, timer_increment);
    A_state = WAITING_FOR_ACK;
}

/* called from layer 3, when a packet arrives for layer 4 */
A_input(packet)
  struct pkt packet;
{
    // Verify checksum and check if it's the expected ACK
    int received_checksum = calculate_checksum(packet);

    if (received_checksum != packet.checksum) {
        printf("  A_input: Received a CORRUPT ACK. Waiting for timeout.\n");
        return;
    }
    
    if (packet.acknum != A_seqnum) {
        printf("  A_input: Received a DUPLICATE ACK (ack=%d). Waiting for timeout.\n", packet.acknum);
        return;
    }

    // Correct ACK received
    printf("  A_input: Received correct ACK (ack=%d). Ready for next message.\n", packet.acknum);
    stoptimer(0);
    A_state = WAITING_FOR_CALL;
    A_seqnum = 1 - A_seqnum; // Flip the sequence number (0 -> 1, 1 -> 0)
}

/* called when A's timer goes off */
A_timerinterrupt()
{
    printf("  A_timerinterrupt: Timeout! Resending packet with seq=%d\n", A_last_packet.seqnum);
    tolayer3(0, A_last_packet);
    starttimer(0, timer_increment);
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
A_init()
{
    A_state = WAITING_FOR_CALL;
    A_seqnum = 0;
    printf("A_init: Sender initialized. Ready to accept messages.\n");
}


/* Note that with simplex transfer from a-to-b, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
B_input(packet)
  struct pkt packet;
{
    int received_checksum = calculate_checksum(packet);

    // Check for corruption and correct sequence number
    if (received_checksum == packet.checksum && packet.seqnum == B_expected_seqnum) {
        printf("  B_input: Received correct packet (seq=%d). Sending ACK and passing to layer 5.\n", packet.seqnum);
        
        // Deliver data to layer 5
        tolayer5(1, packet.payload);

        // Create and send ACK for the received packet
        struct pkt ack_packet;
        ack_packet.acknum = B_expected_seqnum;
        ack_packet.checksum = ack_packet.acknum; // Simple checksum for ACK
        tolayer3(1, ack_packet);
        
        // Flip the expected sequence number for the next packet
        B_expected_seqnum = 1 - B_expected_seqnum;
    } else {
        // Packet is corrupt or a duplicate. Resend ACK for the *last* correctly received packet.
        int last_ack = 1 - B_expected_seqnum;
        printf("  B_input: Received corrupt/duplicate packet. Resending ACK for seq=%d.\n", last_ack);
        
        struct pkt nack_packet;
        nack_packet.acknum = last_ack;
        nack_packet.checksum = nack_packet.acknum;
        tolayer3(1, nack_packet);
    }
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
B_init()
{
    B_expected_seqnum = 0;
    printf("B_init: Receiver initialized. Expecting packet with seq=0.\n");
}