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
#define BIDIRECTIONAL 0    /* change to 1 if you're doing extra credit */
                           /* and write a B_output routine */

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

/* Function prototypes for student routines */
void A_output(struct msg message);
void A_input(struct pkt packet);
void A_timerinterrupt();
void A_init();
void B_input(struct pkt packet);
void B_init();

/* Function prototypes for simulator routines */
void starttimer(int AorB, float increment);
void stoptimer(int AorB);
void tolayer3(int AorB, struct pkt packet);
void tolayer5(int AorB, char datasent[20]);
float jimsrand();


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
void A_output(struct msg message)
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
void A_input(struct pkt packet)
{
    // Verify checksum and check if it's the expected ACK
    if (calculate_checksum(packet) != packet.checksum) {
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
void A_timerinterrupt()
{
    printf("  A_timerinterrupt: Timeout! Resending packet with seq=%d\n", A_last_packet.seqnum);
    tolayer3(0, A_last_packet);
    starttimer(0, timer_increment);
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
    A_state = WAITING_FOR_CALL;
    A_seqnum = 0;
    printf("A_init: Sender initialized. Ready to accept messages.\n");
}


/* Note that with simplex transfer from a-to-b, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
    // Check if packet is corrupt OR has the wrong sequence number
    if (calculate_checksum(packet) != packet.checksum || packet.seqnum != B_expected_seqnum) {
        int last_ack = 1 - B_expected_seqnum;
        printf("  B_input: Received corrupt or out-of-order packet. Resending last ACK=%d.\n", last_ack);
        
        struct pkt ack_pkt;
        ack_pkt.acknum = last_ack;
        ack_pkt.checksum = ack_pkt.acknum; // Simple checksum for ACK is just the ACK number
        tolayer3(1, ack_pkt);
        return;
    }

    // Packet is correct and in order
    printf("  B_input: Received correct packet (seq=%d). Sending ACK and delivering to layer 5.\n", packet.seqnum);
    tolayer5(1, packet.payload);

    // Send ACK for the packet we just received
    struct pkt ack_pkt;
    ack_pkt.acknum = B_expected_seqnum;
    ack_pkt.checksum = ack_pkt.acknum;
    tolayer3(1, ack_pkt);
    
    // Flip the expected sequence number for the next packet
    B_expected_seqnum = 1 - B_expected_seqnum;
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
    B_expected_seqnum = 0;
    printf("B_init: Receiver initialized. Expecting packet with seq=0.\n");
}


/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
  - emulates the tranmission and delivery (possibly with bit-level corruption
    and packet loss) of packets across the network
  - handles the starting/stopping of a timer, and generates timer
    interrupts (resulting in calling students timer handler).
  - generates message to be sent (passed from later 5 to 4)

THERE IS NO REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you definitely should not have to modify
******************************************************************/

struct event {
   float evtime;           /* event time */
   int evtype;             /* event type code */
   int eventity;           /* entity where event occurs */
   struct pkt *pktptr;     /* ptr to packet (if any) assoc w/ this event */
   struct event *prev;
   struct event *next;
 };
struct event *evlist = NULL;   /* the event list */

/* possible events: */
#define  TIMER_INTERRUPT 0  
#define  FROM_LAYER5     1
#define  FROM_LAYER3     2

#define  OFF             0
#define  ON              1
#define   A    0
#define   B    1



int TRACE = 1;             /* for my debugging */
int nsim = 0;              /* number of messages from 5 to 4 so far */ 
int nsimmax = 0;           /* number of msgs to generate, then stop */
float time = 0.000;
float lossprob;            /* probability that a packet is dropped  */
float corruptprob;         /* probability that one bit is packet is flipped */
float lambda;              /* arrival rate of messages from layer 5 */   
int   ntolayer3;           /* number sent into layer 3 */
int   nlost;               /* number lost in media */
int ncorrupt;              /* number corrupted by media */

void init();
void generate_next_arrival();
void insertevent(struct event* p);

int main()
{
   struct event *eventptr;
   struct msg  msg2give;
   struct pkt  pkt2give;
   
   int i,j;
   char c; 
  
   init();
   A_init();
   B_init();
   
   while (1) {
        eventptr = evlist;            /* get next event to simulate */
        if (eventptr==NULL)
           goto terminate;
        evlist = evlist->next;        /* remove this event from event list */
        if (evlist!=NULL)
           evlist->prev=NULL;
        if (TRACE>=2) {
           printf("\nEVENT time: %f,",eventptr->evtime);
           printf("  type: %d",eventptr->evtype);
           if (eventptr->evtype==0)
	       printf(", timerinterrupt  ");
             else if (eventptr->evtype==1)
               printf(", fromlayer5 ");
             else
	     printf(", fromlayer3 ");
           printf(" entity: %d\n",eventptr->eventity);
           }
        time = eventptr->evtime;        /* update time to next event time */
        if (nsim==nsimmax)
	  break;                        /* all done with simulation */
        if (eventptr->evtype == FROM_LAYER5 ) {
            generate_next_arrival();   /* set up future arrival */
            /* fill in msg to give with string of same letter */    
            j = nsim % 26; 
            for (i=0; i<20; i++)  
               msg2give.data[i] = 97 + j;
            if (TRACE>2) {
               printf("          MAINLOOP: data given to student: ");
                 for (i=0; i<20; i++) 
                  printf("%c", msg2give.data[i]);
               printf("\n");
	     }
            nsim++;
            if (eventptr->eventity == A) 
               A_output(msg2give);
             else
               ;
	}
          else if (eventptr->evtype ==  FROM_LAYER3) {
            pkt2give.seqnum = eventptr->pktptr->seqnum;
            pkt2give.acknum = eventptr->pktptr->acknum;
            pkt2give.checksum = eventptr->pktptr->checksum;
            for (i=0; i<20; i++)  
                pkt2give.payload[i] = eventptr->pktptr->payload[i];
	    if (eventptr->eventity ==A)      /* deliver packet to A */
               A_input(pkt2give);            
	    else B_input(pkt2give);          /* deliver packet to B */
	    free(eventptr->pktptr);          /* free the memory for packet */
            }
          else if (eventptr->evtype ==  TIMER_INTERRUPT) {
            if (eventptr->eventity == A) 
	       A_timerinterrupt();
	    else
               ;
	    }
          else  {
	     printf("INTERNAL PANIC: unknown event type \n");
             }
        free(eventptr);
        }

terminate:
   printf(" Simulator terminated at time %f\n after sending %d msgs from layer5\n",time,nsim);
   return 0;
}

void init()                         /* initialize the simulator */
{
  int i;
  float sum, avg;
  float jimsrand();
  
  
   printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
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

   srand(9999);              /* init random number generator */
   sum = 0.0;                /* test random number generator for students */
   for (i=0; i<1000; i++)
      sum=sum+jimsrand();    /* jimsrand() should be uniform in [0,1] */
   avg = sum/1000.0;
   if (avg < 0.25 || avg > 0.75) {
    printf("It is likely that random number generation on your machine\n"); 
    printf("is different from what this emulator expects.  Please follow\n");
    printf("the advice in the assignment manual.\n");
    exit(0);
    }

   ntolayer3 = 0;
   nlost = 0;
   ncorrupt = 0;

   time=0.0;                    /* initialize time to 0.0 */
   generate_next_arrival();     /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used by */
/* hosts A and B to send packets to layer 3.  собеседник is the RANDOM     */
/* numbers generated by rand() which returns an integer in range [0, SRT_MAX]*/
/****************************************************************************/
// float jimsrand() 
// {
//   double mmm = 2147483647;   /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
//   float x;                   /* individual students may need to change mmm */ 
//   x = rand()/mmm;            /* x should be uniform in [0,1] */
//   return(x);
// }  

/****************************************************************************/
/* jimsrand(): return a float in range [0,1]. A simple LCG. */
/****************************************************************************/
long random_seed = 12345; // A seed for our own random number generator

float jimsrand() 
{
    // A simple linear congruential generator (LCG) to ensure consistency
    // across different systems.
    random_seed = (random_seed * 1103515245 + 12345) & 0x7fffffff;
    return ((float)random_seed / (float)0x7fffffff);
}

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/
 
void generate_next_arrival()
{
   double x,log(),ceil();
   struct event *evptr;
    //char *malloc();
   float ttime;
   int tempint;

   if (TRACE>2)
       printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");
 
   x = lambda*jimsrand()*2;  /* x is uniform on [0,2*lambda] */
                             /* having mean of lambda        */
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time + x;
   evptr->evtype =  FROM_LAYER5;
   if (BIDIRECTIONAL && (jimsrand()>0.5) )
      evptr->eventity = B;
    else
      evptr->eventity = A;
   insertevent(evptr);
} 


void insertevent(p)
   struct event *p;
{
   struct event *q,*qold;

   if (TRACE>2) {
      printf("            INSERTEVENT: time is %lf\n",time);
      printf("            INSERTEVENT: future time will be %lf\n",p->evtime); 
      }
   q = evlist;      /* q points to header of list in which p struct inserted */
   if (q==NULL) {   /* list is empty */
        evlist=p;
        p->next=NULL;
        p->prev=NULL;
        }
     else {
        for (qold = q; q !=NULL && p->evtime > q->evtime; q=q->next)
              qold=q; 
        if (q==NULL) {   /* end of list */
             qold->next = p;
             p->prev = qold;
             p->next = NULL;
             }
           else if (q==evlist) { /* front of list */
             p->next=evlist;
             p->prev=NULL;
             p->next->prev=p;
             evlist = p;
             }
           else {     /* middle of list */
             p->next=q;
             p->prev=q->prev;
             q->prev->next=p;
             q->prev=p;
             }
         }
}

void printevlist()
{
  struct event *q;
  int i;
  printf("--------------\nEvent List Follows:\n");
  for(q = evlist; q!=NULL; q=q->next) {
    printf("Event time: %f, type: %d entity: %d\n",q->evtime,q->evtype,q->eventity);
    }
  printf("--------------\n");
}



/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
void stoptimer(AorB)
int AorB;  /* A or B is trying to stop timer */
{
 struct event *q,*qold;

 if (TRACE>2)
    printf("          STOP TIMER: stopping timer at %f\n",time);
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
 for (q=evlist; q!=NULL ; q = q->next) 
    if ( (q->evtype==TIMER_INTERRUPT) && (q->eventity==AorB) ) { 
       /* remove this event */
       if (q->next==NULL && q->prev==NULL)
          evlist=NULL;         /* remove first and only event on list */
          else if (q->next==NULL) /* end of list - there is one in front */
          q->prev->next = NULL;
          else if (q==evlist) { /* front of list - there must be event after */
             q->next->prev=NULL;
             evlist = q->next;
             }
           else {     /* middle of list */
             q->next->prev = q->prev;
             q->prev->next =  q->next;
             }
       free(q);
       return;
     }
  printf("Warning: unable to cancel your timer. It wasn't running.\n");
}


void starttimer(AorB,increment)
int AorB;  /* A or B is trying to start timer */
float increment;
{

 struct event *q;
 struct event *evptr;
 //char *malloc();

 if (TRACE>2)
    printf("          START TIMER: starting timer at %f\n",time);
 /* be nice: check if timer is already started, if so, then  warn */
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
   for (q=evlist; q!=NULL ; q = q->next)  
    if ( (q->evtype==TIMER_INTERRUPT) && (q->eventity==AorB) ) { 
      printf("Warning: attempt to start a timer that is already started\n");
      return;
      }
 
/* create future event for timer interrupt */
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time + increment;
   evptr->evtype =  TIMER_INTERRUPT;
   evptr->eventity = AorB;
   insertevent(evptr);
} 


/************************** TOLAYER3 ***************/
void tolayer3(AorB,packet)
int AorB;  /* A or B is sending this packet */
struct pkt packet;
{
 struct pkt *mypktptr;
 struct event *evptr,*q;
 //char *malloc();
 float lastime, x, jimsrand();
 int i;


 ntolayer3++;

 /* simulate losses: */
 if (jimsrand() < lossprob)  {
      nlost++;
      if (TRACE>0)    
	printf("          TOLAYER3: packet being lost\n");
      return;
    }  

/* make a copy of the packet student just gave me since he/she may decide */
/* to do something with the packet after we return back to him/her */ 
 mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
 mypktptr->seqnum = packet.seqnum;
 mypktptr->acknum = packet.acknum;
 mypktptr->checksum = packet.checksum;
 for (i=0; i<20; i++)
    mypktptr->payload[i] = packet.payload[i];
 if (TRACE>2)  {
   printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
	  mypktptr->acknum,  mypktptr->checksum);
    for (i=0; i<20; i++)
        printf("%c",mypktptr->payload[i]);
    printf("\n");
   }

/* create future event for arrival of packet at the other side */
  evptr = (struct event *)malloc(sizeof(struct event));
  evptr->evtype =  FROM_LAYER3;   /* packet will pop out from layer3 */
  evptr->eventity = (AorB+1) % 2; /* event occurs at other entity */
  evptr->pktptr = mypktptr;       /* save ptr to my copy of packet */
/* finally, compute the arrival time of packet at the other end.
   medium can not reorder, so make sure packet arrives between previous packet
   and next packet scheduled delivery time.  At the beginning, lastime is 0.0. */
  lastime = time;
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
 for (q=evlist; q!=NULL ; q = q->next)
    if ( (q->evtype==FROM_LAYER3  && q->eventity==evptr->eventity) ) 
      lastime = q->evtime;
 evptr->evtime =  lastime + 1 + 9*jimsrand();
 


 /* simulate corruption: */
 if (jimsrand() < corruptprob)  {
    ncorrupt++;
    if ( (x = jimsrand()) < .75)
       mypktptr->payload[0]='Z';   /* corrupt payload */
      else if (x < .875)
       mypktptr->seqnum = 999999;
      else
       mypktptr->acknum = 999999;
    if (TRACE>0)    
	printf("          TOLAYER3: packet being corrupted\n");
    }  

  if (TRACE>2)  
     printf("          TOLAYER3: scheduling arrival on other side\n");
  insertevent(evptr);
} 

void tolayer5(AorB,datasent)
  int AorB;
  char datasent[20];
{
  int i;  
  if (TRACE>2) {
     printf("          TOLAYER5: data received: ");
     for (i=0; i<20; i++)  
        printf("%c",datasent[i]);
     printf("\n");
   }
  
}