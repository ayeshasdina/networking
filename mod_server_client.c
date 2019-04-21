#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() and alarm() */
#include <errno.h>      /* for errno and EINTR */
#include <signal.h>     /* for sigaction() */

#define ECHOMAX         400     /* Longest string to echo */
#define TIMEOUT_SECS    2       /* Seconds between retransmits */
#define NUMNODES        6

int tries=0;   /* Count of times sent - GLOBAL for signal-handler access */

char myHost;
unsigned short myPort;

/*Creating structure of configs to build neighbor table*/
struct config{
    char node;
    int distance;
    char ip[30];
};

struct config neighbors[NUMNODES];
/* structure for rout table*/
struct route_element{
    char destination;
    int distance;
    char next_hop;
};

/*Routing table array of structures*/
struct route_element routing_table[NUMNODES];


struct element{
    char dest;
    int dist;
};
/**Structure for a distance vector */
struct distance_vector{
    char sender;
    int num_of_dests;
    struct element content[NUMNODES];
    };

/*Utility and helper functions.*/
void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}

void CatchAlarm(int ignored)     /* Handler for SIGALRM */
{
    tries += 1;
}


/*Function for reading config files*/
void ReadConfigFile(char* filename)
{
    int  MAXLEN = 25;   /* The maximum number of characters in a line of the text file */
    FILE *fp;
    char buf[MAXLEN];   /* Buffer for a line from the text file */
    int i=0,j=0;
    char *token;
    fp = fopen(filename, "r");
    if (fp==NULL)
    {
        printf("File %s does not exist!\n", filename);
        exit(1);
    }
    while (fgets(buf, MAXLEN, fp) != NULL ) {   // read from the file until the end
        if(i==0)
        {
            myPort= atoi(buf);
        }
        else
        {
            token = strtok(buf, " ");
            neighbors[i-1].node = token[0];
            printf("%c\n",token[0]);

            token = strtok(NULL, " ");
            char p[30];
            strcpy(p,token);
            neighbors[i-1].distance = atoi(p);
            printf("%d\n",atoi(p));

            token = strtok(NULL, " ");
            printf("%s\n",token);
            strcpy(neighbors[i-1].ip,token);
        }
        i++;
    }
    neighbors[i-1].distance = -1;
}

/*Printer functions*/

/*Function for printing configuration files*/
void print_config(struct config c)
{
    printf("neighbor : %c\n", c.node);
    printf("distance : %d\n", c.distance);
    printf("IP : %s\n\n", c.ip);
}

/*printing the routing table*/
void print_route_table(struct route_element r){
        if(r.next_hop==-1){
            printf("destination: %c\t dist : %d\t next_hop : %d\t\n", r.destination,r.distance,r.next_hop);
        }else{
            printf("destination: %c\t dist : %d\t next_hop : %c\t\n", r.destination,r.distance,r.next_hop);
        }
	}

void print_all_route_table()
{
    int i;
    //printf("\n\nRouting Table\n");
    printf("---------------\n");
    for(i=0;i<NUMNODES;i++)
    {
        print_route_table(routing_table[i]);
    }
}

/*Function for updating routing table from neightbors table*/
void update_routing_from_neighbor()
{
    int i,j;

    for(i=0;i<NUMNODES;i++)
    {
        routing_table[i].destination = 65+i;
        routing_table[i].distance = -1;
        routing_table[i].next_hop = -1;

        for(j=0;j<NUMNODES;j++)
        {
            if(routing_table[i].destination == neighbors[j].node)
            {
                routing_table[i].distance = neighbors[j].distance;
                routing_table[i].next_hop = neighbors[j].node;
            }
        }
    }

}




void sendMsg(char *servIP, char* echoString, unsigned short echoServPort){
    int sock;                        /* Socket descriptor */
    struct sockaddr_in echoServAddr; /* Echo server address */
    struct sockaddr_in fromAddr;     /* Source address of echo */
    unsigned int fromSize;           /* In-out of address size for recvfrom() */
    struct sigaction myAction;       /* For setting signal handler */

    struct distance_vector echoBuffer[ECHOMAX+1];      /* Buffer for echo string */
    int echoStringLen;               /* Length of string to echo */
    int respStringLen;                /* Size of received datagram */

    echoStringLen = 50;//sizeof(*echoString);

    /* Create a best-effort datagram socket using UDP */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    /* Set signal handler for alarm signal */
    myAction.sa_handler = CatchAlarm;
    if (sigfillset(&myAction.sa_mask) < 0) /* block everything in handler */
        DieWithError("sigfillset() failed");
    myAction.sa_flags = 0;

    if (sigaction(SIGALRM, &myAction, 0) < 0)
        DieWithError("sigaction() failed for SIGALRM");

    /* Construct the server address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));    /* Zero out structure */
    echoServAddr.sin_family = AF_INET;
    echoServAddr.sin_addr.s_addr = inet_addr(servIP);  /* Server IP address */
    echoServAddr.sin_port = htons(echoServPort);       /* Server port */


    //printf("Sending Message: %s\t length : %d",echoString,echoStringLen);
    /* Send the string to the server */
    if (sendto(sock, echoString, echoStringLen, 0, (struct sockaddr *)
               &echoServAddr, sizeof(echoServAddr)) != echoStringLen)
        DieWithError("sendto() sent a different number of bytes than expected");

    close(sock);
}

void print_distance_vector(struct distance_vector * dv)
{
    int i;
    //printf("\nDistance Vectors\n");
    printf("---------------");
    printf("\nSender: %c\n# of destinations: %d",dv->sender,dv->num_of_dests);
    for(i=0;i<NUMNODES;i++)
    {
        printf("\nDestination : %c \t Dist: %d",dv->content[i].dest,dv->content[i].dist);
    }
}


struct distance_vector* distance_vector_from_routing_table()        //creates a distance vector from the routing table
{
    struct distance_vector* dv = (struct distance_vector*)malloc(sizeof(struct distance_vector));
    int i=0;
    dv->sender = myHost;


    while(neighbors[i].distance!=-1)    //calculating the number of neighbors
    {
        i++;
    }
    dv->num_of_dests = i;

    for(i=0;i<NUMNODES;i++)     //getting distances to each nodes from the routing table
    {
        dv->content[i].dest = routing_table[i].destination;
        dv->content[i].dist = routing_table[i].distance;
    }

    return dv;

}


void convert_to_string(char arr[NUMNODES*5],struct distance_vector* d);

void sendToNeighbors(struct distance_vector* dv)    //send a distance vector to all the neighbors.
{
    int i=0;

    char msg[NUMNODES*7];
    convert_to_string(msg,dv);  //map the vector to strint so that it can be sent through a socket

    while(neighbors[i].distance!=-1)
    {
        //printf("Distance vector sent to : %c \t IP: %s",neighbors[i].node,neighbors[i].ip);
        sendMsg(neighbors[i].ip,msg, myPort);
        i++;
    }
}


int getNeighbor(char name)  //get the index of the neighbor array to neighbor name given as the argument
{
    int i;
    for(i=0;i<NUMNODES;i++)
    {
        if(neighbors[i].node == name)
            break;
    }
    return i;
}

void update_routing_from_distance_vector(struct distance_vector* msg)
{
    int i,index,changed=0;
    index = getNeighbor(msg->sender);   //get the index of the sender in the neighbor array.
    for(i=0;i<NUMNODES;i++)
    {
        if(routing_table[i].destination == myHost)  //dont update the distance to the same node
        {
            continue;
        }

        if(msg->content[i].dist == -1)  //dont update the routing table from a infinite distance
        {
            continue;
        }

        else if(routing_table[i].distance == -1 && msg->content[i].dist != -1)  //if currently the distance value is infinity for a particular destination.
        {
            routing_table[i].distance = msg->content[i].dist + neighbors[index].distance;
            routing_table[i].next_hop = msg->sender;
            changed++;
        }
        else if(msg->content[i].dist + neighbors[index].distance < routing_table[i].distance)   //if the cost of distance vector provided path is better
        {
            routing_table[i].distance = msg->content[i].dist + neighbors[index].distance;   //update the routing table
            routing_table[i].next_hop = msg->sender;
            changed++;  //checking whether the routing table has been changed.
        }
    }

    if(changed>0)
    {
        struct distance_vector* d = distance_vector_from_routing_table();
        sendToNeighbors(d);
    }

}


void convert_to_distance_vector(struct distance_vector* d,char* arr)        //deserializing a distance vector to be sent through a socket
{
    int i=0;
    char* token;
    char zero[] = "0";

    token = strtok(arr, ",");
    d->sender = token[0];

    token = strtok(NULL, ",");

    d->num_of_dests = atoi(token) - atoi(zero);

    for(i=0;i<NUMNODES;i++)
    {
        token = strtok(NULL, ",");
        d->content[i].dest = token[0];

        token = strtok(NULL, ",");
        d->content[i].dist = atoi(token) - atoi(zero);

    }

}

int main(int argc, char *argv[])
{
    //get my host name
    char hostname[3000];
    hostname[2999] = '\0';
    gethostname(hostname, 2999);
    myHost=hostname[0]-32;
    //printf("Hostname: %c\n", myHost);

    //step1 read conf file and initialize neighbour table
    ReadConfigFile(argv[1]);

    //step2 initialize routing table from neighbor table store in routing_table[i]
    update_routing_from_neighbor();
	//print initialize routing table
	printf("\nInitialized routing table:\n");
    print_all_route_table();

    //step3 send to all neighbors
     struct distance_vector* d = distance_vector_from_routing_table(); //vectors to send
     sendToNeighbors(d);        //send the distance vector to all the neighbors


     struct distance_vector* received_dv = (struct distance_vector*)malloc(sizeof(struct distance_vector)); //vectors to receive
     unsigned short echoServPort;     /* Echo server port */
     int echoStringLen;               /* Length of string to echo */
     struct sigaction myAction;       /* For setting signal handler */

     echoServPort = myPort;  /* local port */

     //socket connection
     int sock;                        /* Socket */
     struct sockaddr_in echoServAddr; /* Local address */
     struct sockaddr_in echoClntAddr; /* Client address */ //stores the address of an incoming message
     unsigned int cliAddrLen;         /* Length of incoming message */
     char echoBuffer[ECHOMAX];        /* Buffer for received message*/

     int respStringLen;                 /* Size of received message */


     /* Create socket for sending/receiving datagrams */
     if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
         DieWithError("socket() failed");

     /* Construct local address structure */
     memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
     echoServAddr.sin_family = AF_INET;                /* Internet address family */
     echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
     echoServAddr.sin_port = htons(echoServPort);      /* Local port */

     /* Bind to the local address */
     if (bind(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
         DieWithError("bind() failed");

     /* Set signal handler for alarm signal */
     myAction.sa_handler = CatchAlarm;
     if (sigfillset(&myAction.sa_mask) < 0) /* block everything in handler */
         DieWithError("sigfillset() failed");
     myAction.sa_flags = 0;

     if (sigaction(SIGALRM, &myAction, 0) < 0)
         DieWithError("sigaction() failed for SIGALRM");


   alarm(TIMEOUT_SECS);        // Set the timeout //

  for (;;) /* Run forever */    //server
     {
         /* Set the size of the in-out parameter */
         cliAddrLen = sizeof(echoClntAddr);



         //if not receid the message, check whether a timeout has occured
         while((respStringLen = recvfrom(sock, echoBuffer, ECHOMAX, 0,
             (struct sockaddr *) &echoClntAddr, &cliAddrLen)) < 0){

      if (errno == EINTR)     // Alarm went off.. timeout has occured.. send distance vector to neighbors.
      {
        printf("\n\nPeriodically sends out the Distance Vector\n\n");
        d = distance_vector_from_routing_table();
        sendToNeighbors(d);
        alarm(TIMEOUT_SECS);
      }

    }

    echoBuffer[respStringLen] = '\0';
    /*Converting the echo buffer to distance vectors*/
    convert_to_distance_vector(received_dv,echoBuffer);

	/*Print received dist vector*/
	printf("\nReceived distance vectors are:");
    print_distance_vector(received_dv); //print the received distance vector

	/*Print updated routing table*/
	printf("\n---------------\n");
	printf("\n Updated routing table \n");
   /*update the routing table from the received distance vector. if changed, distance vectors will be sent to all the neighbors*/
	update_routing_from_distance_vector(received_dv);
    print_all_route_table();
    }
     exit(0);
}


void convert_to_string(char arr[NUMNODES*5],struct distance_vector* d)  //serializing a distance vector to be sent from through a socket
{
    int i;
    char nll ='\0';

     sprintf(arr,"%c,%d,%c,%d,%c,%d,%c,%d,%c,%d,%c,%d,%c,%d,%c,%d,%c",d->sender,d->num_of_dests,d->content[0].dest,d->content[0].dist,d->content[1].dest,d->content[1].dist,d->content[2].dest,d->content[2].dist,d->content[3].dest,d->content[3].dist,d->content[4].dest,d->content[4].dist,d->content[5].dest,d->content[5].dist,d->content[6].dest,d->content[6].dist,nll);

}
