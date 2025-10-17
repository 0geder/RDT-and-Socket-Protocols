#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, EEE3093S, at the University of Cape Town.
   It has been generously provided by J.F.Kurose, University of Massachusetts.
**********************************************************************/

#define TRUE 1
#define FALSE 0
#define BIDIRECTIONAL 0

struct msg {
  char data[20];
};

struct pkt {
   int seqnum;
   int acknum;
   int checksum;
   char payload[20];
};

void A_output(struct msg message);
void A_input(struct pkt packet);
void A_timerinterrupt();
void A_init();
void B_input(struct pkt packet);
void B_init();

void starttimer(int AorB, float increment);
void stoptimer(int AorB);
void tolayer3(int AorB, struct pkt packet);
void tolayer5(int AorB, char datasent[20]);
float jimsrand();


/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/
#define WINDOW_SIZE 8
#define BUFFER_SIZE 50

// Sender (A) variables
int a_base;
int a_nextseqnum;
struct pkt a_buffer[BUFFER_SIZE];
float timer_increment = 30.0;

// Receiver (B) variables
int b_expectedseqnum;

/* Helper function to calculate checksum */
int calculate_checksum(struct pkt packet) {
    int checksum = 0;
    checksum += packet.seqnum;
    checksum += packet.acknum;
    for (int i = 0; i < 20; i++) { checksum += (unsigned char)packet.payload[i]; }
    return checksum;
}

void A_output(struct msg message) {
    if (a_nextseqnum >= BUFFER_SIZE) {
        printf("  A_output: Buffer full, dropping message.\n");
        return;
    }
    if (a_nextseqnum >= a_base + WINDOW_SIZE) {
        printf("  A_output: Window is full, buffering message for later.\n");
        // Buffer the message data for when the window slides
        memcpy(a_buffer[a_nextseqnum].payload, message.data, 20);
        a_buffer[a_nextseqnum].seqnum = a_nextseqnum; // Store seqnum for later
        a_nextseqnum++; // Increment so we know we have a buffered message
        return;
    }

    // Create and store the packet in the buffer
    memcpy(a_buffer[a_nextseqnum].payload, message.data, 20);
    a_buffer[a_nextseqnum].seqnum = a_nextseqnum;
    a_buffer[a_nextseqnum].acknum = 0; // Not used
    a_buffer[a_nextseqnum].checksum = calculate_checksum(a_buffer[a_nextseqnum]);
    
    // Send the packet
    printf("  A_output: Sending packet with seq=%d\n", a_nextseqnum);
    tolayer3(0, a_buffer[a_nextseqnum]);
    
    if (a_base == a_nextseqnum) { starttimer(0, timer_increment); }
    a_nextseqnum++;
}

void A_input(struct pkt packet) {
    if (calculate_checksum(packet) != packet.checksum) {
        printf("  A_input: Received CORRUPT ACK. Ignoring.\n");
        return;
    }
    
    printf("  A_input: Received ACK for %d. Updating base.\n", packet.acknum);
    
    // Check if the ACK is for a packet within the current window
    if (packet.acknum >= a_base) {
        a_base = packet.acknum + 1;
        stoptimer(0); // Stop the old timer
        // If there are still unacknowledged packets in the window, start a new timer
        if (a_base < a_nextseqnum) {
            starttimer(0, timer_increment);
        }
    }
}

void A_timerinterrupt() {
    printf("  A_timerinterrupt: TIMEOUT! Resending window from base=%d\n", a_base);
    stoptimer(0); // Stop current timer before starting a new one
    starttimer(0, timer_increment);
    for (int i = a_base; i < a_nextseqnum; i++) {
        printf("  A_timerinterrupt: Resending packet seq=%d\n", i);
        tolayer3(0, a_buffer[i]);
    }
}

void A_init() {
    a_base = 0;
    a_nextseqnum = 0;
}

void B_input(struct pkt packet) {
    if (calculate_checksum(packet) == packet.checksum && packet.seqnum == b_expectedseqnum) {
        printf("  B_input: Received correct packet (seq=%d). Delivering and sending ACK.\n", packet.seqnum);
        tolayer5(1, packet.payload);
        
        struct pkt ack_pkt;
        ack_pkt.acknum = b_expectedseqnum;
        ack_pkt.checksum = ack_pkt.acknum;
        tolayer3(1, ack_pkt);
        
        b_expectedseqnum++;
    } else {
        int last_ack = b_expectedseqnum - 1;
        printf("  B_input: Received out-of-order/corrupt packet. Resending last good ACK=%d.\n", last_ack);
        if (last_ack >= 0) { // Don't send ACK -1
            struct pkt ack_pkt;
            ack_pkt.acknum = last_ack;
            ack_pkt.checksum = ack_pkt.acknum;
            tolayer3(1, ack_pkt);
        }
    }
}

void B_init() {
    b_expectedseqnum = 0;
}


/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
******************************************************************/

// (The rest of this file is the exact same boilerplate simulator code as the ABP file)
// (It starts with `struct event` and ends with `tolayer5`)

struct event {
   float evtime; int evtype; int eventity; struct pkt *pktptr;
   struct event *prev; struct event *next;
};
struct event *evlist = NULL;
#define TIMER_INTERRUPT 0
#define FROM_LAYER5 1
#define FROM_LAYER3 2
#define OFF 0
#define ON 1
#define A 0
#define B 1
int TRACE = 1; int nsim = 0; int nsimmax = 0; float time = 0.000;
float lossprob; float corruptprob; float lambda;
int ntolayer3; int nlost; int ncorrupt;
long random_seed = 12345;

void init();
void generate_next_arrival();
void insertevent(struct event*);

int main() {
   struct event *eventptr;
   struct msg  msg2give;
   struct pkt  pkt2give;
   int i,j;
   
   init();
   A_init();
   B_init();
   
   while (1) {
        eventptr = evlist;
        if (eventptr==NULL) goto terminate;
        evlist = evlist->next;
        if (evlist!=NULL) evlist->prev=NULL;
        if (TRACE>=2) {
           printf("\nEVENT time: %f,",eventptr->evtime);
           printf("  type: %d",eventptr->evtype);
           if (eventptr->evtype==0) printf(", timerinterrupt");
           else if (eventptr->evtype==1) printf(", fromlayer5 ");
           else printf(", fromlayer3 ");
           printf(" entity: %d\n",eventptr->eventity);
        }
        time = eventptr->evtime;
        if (nsim==nsimmax && evlist==NULL) break;
        if (eventptr->evtype == FROM_LAYER5) {
            if (nsim < nsimmax) {
                generate_next_arrival();
                j = nsim % 26; 
                for (i=0; i<20; i++) msg2give.data[i] = 97 + j;
                nsim++;
                if (eventptr->eventity == A) A_output(msg2give);
            }
        } else if (eventptr->evtype == FROM_LAYER3) {
            pkt2give.seqnum = eventptr->pktptr->seqnum;
            pkt2give.acknum = eventptr->pktptr->acknum;
            pkt2give.checksum = eventptr->pktptr->checksum;
            for (i=0; i<20; i++) pkt2give.payload[i] = eventptr->pktptr->payload[i];
            if (eventptr->eventity ==A) A_input(pkt2give);
            else B_input(pkt2give);
            free(eventptr->pktptr);
        } else if (eventptr->evtype == TIMER_INTERRUPT) {
            if (eventptr->eventity == A) A_timerinterrupt();
        } else {
	     printf("INTERNAL PANIC: unknown event type \n");
        }
        free(eventptr);
    }

terminate:
   printf(" Simulator terminated at time %f\n after sending %d msgs from layer5\n",time,nsim);
   return 0;
}

void init() {
   printf("-----  Go-Back-N Network Simulator Version 1.1 -------- \n\n");
   printf("Enter the number of messages to simulate: ");
   scanf("%d",&nsimmax);
   printf("Enter  packet loss probability [enter 0.0 for no loss]:");
   scanf("%f",&lossprob);
   printf("Enter packet corruption probability [0.0 for no corruption]:");
   scanf("%f",&corruptprob);
   printf("Enter average time between messages from sender's layer5 [ > 0.0]:");
   scanf("%f",&lambda);
   printf("Enter TRACE:");
   scanf("%d",&TRACE);

   ntolayer3 = 0; nlost = 0; ncorrupt = 0;
   time=0.0;
   generate_next_arrival();
}

float jimsrand() {
    random_seed = (random_seed * 1103515245 + 12345) & 0x7fffffff;
    return ((float)random_seed / (float)0x7fffffff);
}  

void generate_next_arrival() {
   double x;
   struct event *evptr;
   if (nsim >= nsimmax) return;
   x = lambda*jimsrand()*2;
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time + x;
   evptr->evtype =  FROM_LAYER5;
   if (BIDIRECTIONAL && (jimsrand()>0.5) ) evptr->eventity = B;
   else evptr->eventity = A;
   insertevent(evptr);
} 

void insertevent(struct event *p) {
   struct event *q,*qold;
   q = evlist;
   if (q==NULL) {
        evlist=p; p->next=NULL; p->prev=NULL;
   } else {
        for (qold=q; q!=NULL && p->evtime > q->evtime; q=q->next) qold=q; 
        if (q==NULL) {
             qold->next=p; p->prev=qold; p->next=NULL;
        } else if (q==evlist) {
             p->next=evlist; p->prev=NULL; p->next->prev=p; evlist=p;
        } else {
             p->next=q; p->prev=q->prev; q->prev->next=p; q->prev=p;
        }
   }
}

void stoptimer(int AorB) {
 struct event *q;
 for (q=evlist; q!=NULL; q=q->next) 
    if ((q->evtype==TIMER_INTERRUPT) && (q->eventity==AorB)) { 
       if (q->next==NULL && q->prev==NULL) evlist=NULL;
       else if (q->next==NULL) q->prev->next = NULL;
       else if (q==evlist) { q->next->prev=NULL; evlist = q->next; }
       else { q->next->prev = q->prev; q->prev->next = q->next; }
       free(q);
       return;
     }
}

void starttimer(int AorB, float increment) {
 struct event *q;
 struct event *evptr;
 for (q=evlist; q!=NULL; q=q->next)  
    if ((q->evtype==TIMER_INTERRUPT) && (q->eventity==AorB)) { 
      printf("Warning: attempt to start a timer that is already started\n");
      return;
    }
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time + increment;
   evptr->evtype =  TIMER_INTERRUPT;
   evptr->eventity = AorB;
   insertevent(evptr);
} 

void tolayer3(int AorB, struct pkt packet) {
 struct pkt *mypktptr;
 struct event *evptr, *q;
 float lastime, x;
 int i;
 ntolayer3++;
 if (jimsrand() < lossprob)  {
      nlost++;
      if (TRACE>0) printf("          TOLAYER3: packet being lost\n");
      return;
 }  
 mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
 *mypktptr = packet;
 if (TRACE>2)  {
   printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum, mypktptr->acknum, mypktptr->checksum);
   for (i=0; i<20; i++) printf("%c",mypktptr->payload[i]);
   printf("\n");
 }
 evptr = (struct event *)malloc(sizeof(struct event));
 evptr->evtype =  FROM_LAYER3;
 evptr->eventity = (AorB+1) % 2;
 evptr->pktptr = mypktptr;
 lastime = time;
 for (q=evlist; q!=NULL; q=q->next)
    if ((q->evtype==FROM_LAYER3 && q->eventity==evptr->eventity)) 
      lastime = q->evtime;
 evptr->evtime =  lastime + 1 + 9*jimsrand();
 
 if (jimsrand() < corruptprob) {
    ncorrupt++;
    if ((x = jimsrand()) < .75) mypktptr->payload[0]='Z';
    else if (x < .875) mypktptr->seqnum = 999999;
    else mypktptr->acknum = 999999;
    if (TRACE>0) printf("          TOLAYER3: packet being corrupted\n");
 }  
 insertevent(evptr);
} 

void tolayer5(int AorB, char datasent[20]) {
  /* Do nothing */
}