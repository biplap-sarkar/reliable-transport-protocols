
Following are the important variables used for the three protocols:-

1.) Alternating Bit (abt.c)

*) Current State variable:-   int state	// Can take two values, WAITING=1 AND READY=2
*) RTT/Timeout value:- #define TIMEOUT 10.0
*) Sequence number maintained by sender A:- int seqA=0;
*) Sequence number maintained by receiver B:- int seqB=0;
*) Buffer to keep the last sent message by sender A:- struct msg buffer;



2.) Go Back N (gbn.c) 

*) Window Size:- 	#define WINDOW [window size]
*) Buffer Length, Maximum number of segments which protocol can buffer:-
		#define BUFFLEN [buffer size]
*) Message Buffer:- struct msg buffer[BUFFLEN];
*) Index to buffer header:- int buffhead;	//Index pointing to location where next message would be kept
*)	Index to window base:- int win_base;	//Index pointing to base of the window
*)	Sequence number sent by sender A:- int seq;	//Sequence number
*) Sequence number maintained by receiver B:- int seqB;	//Sequence number
*) RTT/Timeout value:- float TIMEOUT = 30.0;


3.) Selective Repeat (sr.c)

*) Window Size:- 	#define WINDOW [window size]
*) Buffer Length, Maximum number of segments which protocol can buffer:-
		#define BUFFLEN [buffer size]
*) Message Buffer:- struct msg buffer[BUFFLEN];
*) Index to buffer header:- int buffhead;	//Index pointing to location where next message would be kept
*)	Index to window base of sender A:- int win_base;	//Index pointing to base of the window
*)	Sequence number sent by sender A:- int seq;	//Sequence number
*) Sequence number maintained by receiver B:- int seqB;	//Sequence number
*) RTT/Timeout value:- #define TIMEOUT 15.0
*) Last timestamp:- timestamp;		//Last time stamp when logical timers were updated
*) Count of logical timers:- int timercounts = 0;	// Count of logical timers in timer list
*) Buffer for keeping advanced acknowledgements for sender A:- int advanced_ack[WINDOW_SIZE];	
*) Index to window base of receiver B:- int win_baseB=0;
*) Buffer for keeping packets received in advance in receiver B:- struct pkt pkt_buffer[WINDOW_SIZE];
*) Buffer for keeping track of advanced acknowledgements sent by B:- int advanced_ackB[WINDOW_SIZE];
