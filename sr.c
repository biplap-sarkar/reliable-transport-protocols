#include <stdio.h>
#include <stdlib.h>
/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional or bidirectional
   data transfer protocols (from A to B. Bidirectional transfer of data
   is for extra credit and is not required).  Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 0    /* change to 1 if you're doing extra credit */
                           /* and write a routine called B_output */

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct msg {
  char data[20];
  };

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt {
   int seqnum;
   int acknum;
   int checksum;
   char payload[20];
    };

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

/* Added after TA's post 
 * http://cse4589s2014.wordpress.com/2014/04/03/pa2-important-if-your-variables-are-randomly-changing-to-large-values/
 * in course blog */
 
float lossprob = 0.0; /* probability that a packet is dropped */
float corruptprob = 0.0; /* probability that one bit is packet is flipped */
float lambda = 0.0; /* arrival rate of messages from layer 5 */
int ntolayer3 = 0; /* number sent into layer 3 */
int nlost = 0; /* number lost in media */
int ncorrupt = 0; /* number corrupted by media*/

void starttimer(int, float);
void stoptimer(int);
void tolayer3(int, struct pkt);
void tolayer5(int, char *);
void update_timouts(float);
void A_send(struct msg, int);

#define   A    0
#define   B    1
#define WINDOW_SIZE 10	//Window Size
#define BUFFLEN 5000	//Buffer length for incoming segments
#define TIMEOUT 15.00	//Timeout value

int buffhead=0;		//Points to header of buffer where data is to be stored next
extern float time;	
int win_base=0;		//Points to base of the window
int seq=0;			//Initial sequence number
int number_1=0,number_2=0,number_3=0,number_4=0;
float timestamp;	//Last time stamp when logical timers were updated
int timercounts = 0;	// Count of logical timers in timer list
int advanced_ack[WINDOW_SIZE];	//Buffer to keep advance acknowledgements


/* Structure describing one logical timer */
struct pkt_timer {
	float timeout;	//Timeout value
	int seq;		//Sequence number with wich this logical timer is associated
	struct pkt_timer *next;	//Pointer to next logical timer in the logical timer list.
} *timer_head=NULL;		//Header for list of logical timer

struct msg buffer[BUFFLEN];	//Buffer for keeping incoming messages

/*
 * This function generates checksum for a given packet
 * 
 * @arg packet: pointer to packet whose checksum is to be created
 * @return: integer value of the checksum
 */
int generate_checksum(struct pkt *packet){
	int sum = 0;
	sum += packet->seqnum;
	sum += packet->acknum;
	int i;
	for(i=0;i<20;i++){
		sum += packet->payload[i];
	}
	return ~sum;
}

/*
 * This function verifies checksum of a packet
 * 
 * @arg packet: pointer to packet whose checksum is to be verified
 * @return: 0 if the checksum matches, non-zero integer 
 * representing difference between expected checksum and actual
 * checksum otherwise
 */
int verify_packet(struct pkt *packet){
	int sum = generate_checksum(packet);
	sum = sum - packet->checksum;
	return sum;
}


/*
 * This provides abstraction of a logical timer which enables me
 * to implement multiple logical timers using one actual timer.
 * It is implemented by maintaining a list of logical timers
 * (behaving as a queue where the node is inserted at the end of the list).
 * The head of this list corresponds to actual timer.
 * In the event of a timeout, the timeout values in each of the nodes in
 * the list are updated by the time elapsed between previous timestamp.
 * 
 * @arg seqnum: sequence number of the segment for which timeout is added
 * @arg timeout: timeout value for the segment
 */
void start_logical_timer(int seqnum, float timeout){
	if(timer_head == NULL){
		printf("A:- Timer list was empty, adding new header for %d\n",seqnum);
		timer_head = (struct pkt_timer *)malloc(sizeof(struct pkt_timer));
		timer_head->seq = seqnum;
		timer_head->timeout = timeout;
		timer_head->next = NULL;
		timestamp = time;
		printf("A:- Starting acual timer for %d\n",timer_head->seq);
		starttimer(A,timeout);
	}
	else{
		float time_elapsed = time - timestamp;
		timestamp = time;
		update_timouts(time_elapsed);
		struct pkt_timer *ptr = timer_head;
		while(ptr->next){
			ptr = ptr->next;
		}
		ptr->next = (struct pkt_timer *)malloc(sizeof(struct pkt_timer));
		ptr = ptr->next;
		ptr->seq = seqnum;
		ptr->timeout = timeout;
		ptr->next = NULL;
		printf("A:- Timer appended in list headed by %d\n",timer_head->seq);
	}
	timercounts++;
}

/*
 * This function removes a timer entry from the logical timer list.
 * 
 * arg t_seq: sequence number of the segment whose timer is to be removed
 */
void remove_timer_entry(int t_seq){
	struct pkt_timer *ptr = timer_head;
	struct pkt_timer *tmp;
	if(timer_head->seq == t_seq){
		timer_head = timer_head->next;
		printf("A:- Timer entry removed from head for %d\n",ptr->seq);
		free(ptr);
		timercounts--;
		if(timer_head==NULL){
			printf("A:- Stopping acual timer for %d\n",t_seq);
			stoptimer(A);
		}
	}
	else{
		while(ptr->next != NULL && ptr->next->seq != t_seq){
			ptr=ptr->next;
		}
		if(ptr->next){
			tmp = ptr->next;
			ptr->next = tmp->next;
			//printf("A:- Timer entry removed for %d\n",tmp->seq);
			free(tmp);
			timercounts--;
		}
	}
}

/*
 * Update the timeout values in the list of logical timers
 * 
 * arg elapsed: time value by which the timouts should be updated
 */
void update_timouts(float elapsed){
	struct pkt_timer *ptr = timer_head;
	while(ptr){
		ptr->timeout = ptr->timeout - elapsed;
		ptr = ptr->next;
	}
}


/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{
	number_1++;
	struct pkt packet;
	if(buffhead >= BUFFLEN){
		printf("A:- Buffer full, exiting\n");
		exit(0);
	}
	int i;
	for(i=0;i<20;i++)
		buffer[buffhead].data[i]=message.data[i];
	printf("A:- New segment %d added in buffer\n",buffhead);
	if(seq < win_base + WINDOW_SIZE){
		printf("A:- Sending segment %d to layer 3\n",seq);
		A_send(buffer[buffhead],seq);
	}
	else{
		printf("A:- Window full, segment %d buffered to be sent in future\n",buffhead);
	}
	buffhead++;
}

void B_output(message)  /* need be completed only for extra credit */
  struct msg message;
{

}

/*
 * This function sends a message from A to layer 3 and also
 * starts logical timer for it
 * 
 * @arg message: message to be sent
 * @arg seqnum: sequence number of the message to be sent
 */
void A_send(struct msg message,int seqnum){
	struct pkt packet;
	packet.seqnum = seqnum;
	int i;
	for(i=0;i<20;i++)
		packet.payload[i]=message.data[i];
	packet.checksum = generate_checksum(&packet);
	printf("A:- sending segment %d to layer3\n",seqnum);
	tolayer3(A,packet);
	number_2++;
	start_logical_timer(seqnum,TIMEOUT);
	printf("A:- Timer added for %d\n",seqnum);
	if(seqnum == seq)
		seq++;
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
	if(verify_packet(&packet)!=0){
		printf("A:- ack %d corrupted, dropping it\n",packet.acknum);
		return;
	}
	if(win_base == packet.acknum){
		printf("A:- ack %d received, moving window base by 1\n",packet.acknum);
		remove_timer_entry(packet.acknum);
		win_base++;
		while(advanced_ack[win_base%WINDOW_SIZE]){
			printf("A:- ack %d already received, moving window base by 1\n",win_base);
			advanced_ack[win_base%WINDOW_SIZE]=0;
			win_base++;
		}
		while(seq < (win_base + WINDOW_SIZE) && seq < buffhead){
			A_send(buffer[seq],seq);
		}
	}
	else if(packet.acknum < win_base){
		printf("A:- obsolete ack %d, discarding it\n",packet.acknum);
	}
	else {
		if(advanced_ack[packet.acknum%WINDOW_SIZE] == 1){
			printf("A:- ack %d already received, discarding\n",packet.acknum);
			return;
		}
		advanced_ack[packet.acknum%WINDOW_SIZE] = 1;
		printf("A:- advanced ack %d received, buffering acknowledgement\n",packet.acknum);
		remove_timer_entry(packet.acknum);
	}
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	printf("A:- timeout for sequence %d\n",timer_head->seq);
	int t_seq = timer_head->seq;
	printf("A:- resending packet %d\n",t_seq);
	struct pkt_timer *ptr = timer_head;
	timer_head = timer_head->next;
	free(ptr);
	printf("A:- Timer removed for %d\n",t_seq);
	timercounts--;
	update_timouts(ptr->timeout);
	timestamp = time;
	if(timer_head){
		printf("A:- Start actual timer for %d\n",timer_head->seq);
		starttimer(A, timer_head->timeout);
	}
	A_send(buffer[t_seq],t_seq);
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
}

int win_baseB=0;
struct pkt pkt_buffer[WINDOW_SIZE];
int advanced_ackB[WINDOW_SIZE];

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
	number_3++;
	if(verify_packet(&packet)!=0){
		printf("B:- packet %d corrupted, dropping it\n",packet.seqnum);
		return;
	}
	struct pkt ack;
	ack.acknum = packet.seqnum;
	ack.checksum = generate_checksum(&ack);
	if(win_baseB == packet.seqnum){
		printf("B:- expected packet %d received, delivering it\n",packet.seqnum);
		tolayer5(B,packet.payload);
		number_4++;
		win_baseB++;
		while(advanced_ackB[win_baseB%WINDOW_SIZE]){
			printf("B:- expected packet %d already received, delivering it\n",win_baseB);
			tolayer5(B,pkt_buffer[win_baseB%WINDOW_SIZE].payload);
			advanced_ackB[win_baseB%WINDOW_SIZE]=0;
			number_4++;
			win_baseB++;
		}
	}
	else if(win_baseB < packet.seqnum){
		printf("B:- advanced packet %d received, buffering it\n",packet.seqnum);
		int index = packet.seqnum%WINDOW_SIZE;
		advanced_ackB[index]=1;
		pkt_buffer[index].seqnum = packet.seqnum;
		int i;
		for(i=0;i<20;i++)
			pkt_buffer[index].payload[i] = packet.payload[i];
	}
	else {
		printf("B:- obsolete packet %d received, discarding\n",packet.seqnum);
	}
	printf("B:- Sending ack %d\n",ack.acknum);
	tolayer3(B,ack);
}

/* called when B's timer goes off */
void B_timerinterrupt()
{
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
}

/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
  - emulates the tranmission and delivery (possibly with bit-level corruption
    and packet loss) of packets across the layer 3/4 interface
  - handles the starting/stopping of a timer, and generates timer
    interrupts (resulting in calling students timer handler).
  - generates message to be sent (passed from later 5 to 4)

THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
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

//forward declarations
void init();
void generate_next_arrival();
void insertevent(struct event*);

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
int ncorrupt;              /* number corrupted by media*/

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
               B_output(msg2give);  
            }
          else if (eventptr->evtype ==  FROM_LAYER3) {
            pkt2give.seqnum = eventptr->pktptr->seqnum;
            pkt2give.acknum = eventptr->pktptr->acknum;
            pkt2give.checksum = eventptr->pktptr->checksum;
            for (i=0; i<20; i++)  
                pkt2give.payload[i] = eventptr->pktptr->payload[i];
	    if (eventptr->eventity ==A)      /* deliver packet by calling */
   	       A_input(pkt2give);            /* appropriate entity */
            else
   	       B_input(pkt2give);
	    free(eventptr->pktptr);          /* free the memory for packet */
            }
          else if (eventptr->evtype ==  TIMER_INTERRUPT) {
            if (eventptr->eventity == A) 
	       A_timerinterrupt();
             else
	       B_timerinterrupt();
             }
          else  {
	     printf("INTERNAL PANIC: unknown event type \n");
             }
        free(eventptr);
        }

terminate:
   printf(" Simulator terminated at time %f\n after sending %d msgs from layer5\n",time,nsim);
   printf("\nProtocol: [Selective-Repeat Protocol]\n");
   printf("[%d] packets sent from the Application Layer of Sender A\n",number_1);
   printf("[%d] packets sent from the Transport Layer of Sender A\n",number_2);
   printf("[%d] packets received at the Transport layer of Receiver B\n",number_3);
   printf("[%d] packets received at the Application layer of Receiver B\n",number_4);
   printf("Total time: [%f] time units\n",time);
   printf("Throughput = [%f] packets/time units\n",(float)number_4/time);
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
    printf("It is likely that random number generation on your machine\n" ); 
    printf("is different from what this emulator expects.  Please take\n");
    printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
    exit(0);
    }

   ntolayer3 = 0;
   nlost = 0;
   ncorrupt = 0;

   time=0.0;                    /* initialize time to 0.0 */
   generate_next_arrival();     /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand() 
{
  double mmm = 2147483647;   /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
  float x;                   /* individual students may need to change mmm */ 
  x = rand()/mmm;            /* x should be uniform in [0,1] */
  return(x);
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
   q = evlist;     /* q points to header of list in which p struct inserted */
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
    if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) { 
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
int AorB;  /* A or B is trying to stop timer */
float increment;
{

 struct event *q;
 struct event *evptr;
 //char *malloc();

 if (TRACE>2)
    printf("          START TIMER: starting timer at %f\n",time);
 /* be nice: check to see if timer is already started, if so, then  warn */
/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
   for (q=evlist; q!=NULL ; q = q->next)  
    if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) { 
      printf("Warning: attempt to start a timer that is already started\n");
      return;
      }
 
/* create future event for when timer goes off */
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time + increment;
   evptr->evtype =  TIMER_INTERRUPT;
   evptr->eventity = AorB;
   insertevent(evptr);
} 


/************************** TOLAYER3 ***************/
void tolayer3(AorB,packet)
int AorB;  /* A or B is trying to stop timer */
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
   medium can not reorder, so make sure packet arrives between 1 and 10
   time units after the latest arrival time of packets
   currently in the medium on their way to the destination */
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
