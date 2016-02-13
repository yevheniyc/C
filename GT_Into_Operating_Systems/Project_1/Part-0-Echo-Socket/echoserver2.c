/*
  echoserver2.c - a stream socket that repeats and sends back client's message
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#if 0
/* 
 * Structs exported from netinet/in.h (for easy reference)
 */

/* Internet address */
struct in_addr {
    unsigned int s_addr; 
};

/* Internet style socket address */
struct sockaddr_in  {
    unsigned short int sin_family; /* Address family */
    unsigned short int sin_port;   /* Port number */
    struct in_addr sin_addr;	 /* IP address */
    unsigned char sin_zero[...];   /* Pad to size of 'struct sockaddr' */
};

/*
 * Struct exported from netdb.h
 */

/* Domain name service (DNS) host entry */
struct hostent {
    char    *h_name;        /* official name of host */
    char    **h_aliases;    /* alias list */
    int     h_addrtype;     /* host address type */
    int     h_length;       /* length of address */
    char    **h_addr_list;  /* list of addresses */
}
#endif

#define BUFFER_SIZE  16
#define MAX_PENDING  5
#define USAGE                                                                 \
"usage:\n"                                                                    \
"  echoserver [options]\n"                                                    \
"options:\n"                                                                  \
"  -p                  Port (Default: 8888)\n"                                \
"  -n                  Maximum pending connections\n"                         \
"  -h                  Show this help message\n"                              


void * get_in_addr(struct sockaddr * sa);

int main(int argc, char **argv) {
    int sockfd, new_fd;                 // listen on sock_fd, new connection on new_fd
    int option_char;                    // for handling commandline argv
    int portno = 8888;                  // default port to listen on 
    int maxnpending = MAX_PENDING;      // max of pending connections in the queue
    struct addrinfo hints,              // hints - preload know server info; 
                    *servinfo;          // copy return from getaddrinfo() to servinfo
    struct sockaddr_storage their_addr; // connector's address informmation that handle both sockaddr_in and sockaddr_in6
    socklen_t sin_size;                 // their_addr' struct size
    char message_buff[BUFFER_SIZE];     // for receiving and sending messages
    int num_bytes;                      // for receiving - how many bytes were actuall received
    int res;                            // return result for getaddrinfo
    int yes;                            // used for setsockopt - to reap zombies
    char ip_info[INET6_ADDRSTRLEN];     // enough to handle IPv6 as well as IPv4
  
    // Parse and set command line arguments
    while ((option_char = getopt(argc, argv, "p:n:h")) != -1){
        switch (option_char) {
            case 'p': // listen-port
                portno = atoi(optarg);
                break;                                        
            case 'n': // server
                maxnpending = atoi(optarg);
                break; 
            case 'h': // help
                fprintf(stdout, "%s", USAGE);
                exit(0);
                break;           
            default:
                fprintf(stderr, "%s", USAGE);
                exit(1);
        }
    }

    /* Socket Code Here */
    memset(&hints, 0, sizeof hints);  // make sure hints is empty
    hints.ai_family = AF_INET;        // IPv4 only for now
    hints.ai_socktype = SOCK_STREAM;  // steaming 
    hints.ai_flags = AI_PASSIVE;      // my ip


    // get more detailed server information
    if ((res = getaddrinfo(NULL, portno, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(res));
        return 1;
    }
    printf("getaddrinfo is executed\n");

    // use that information to create a socket
    if ((sockfd = socket(servinfo->ai_family, 
                         servinfo->ai_socktype, 
                         servinfo->ai_protocol)) == -1) {
        perror("server: socket");
        continue;
    }
    printf("socket is created\n");

    // get rid of zombie processes = lose the pesky "Address already in use" error message
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }
    printf("zombies are killed\n");

    // let's bind to the socket
    if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        close(sockfd);
        perror("server:bind");
        continue;
    }
    printf("bind to socket\n");

    // now let's listen to the socket
    if (listen(sockfd, maxnpending) == -1) {
        perror("listen");
        exit(1)
    }

    printf("server: waiting for connections...\n");

    while(1) { // main accept loop
        sin_size = sizeof their_addr; // we want to know client's information
        // let's accept
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size); // accept and cast to sockaddr pointer
        if (new_fd = -1) {
            perror("accept");
            continue;
        }

        // let's find connector's information
        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr),
                  ip_info, sizeof ip_info);
        printf("server: got connection from %s\n", ip_info);

        // let's get connector's (client's message)
        if ((num_bytes = recv(sockfd, message_buff, BUFFER_SIZE-1, 0)) == -1) {
            perror("recv");
            exit(1);
        }

        // make sure the received message in null-terminated
        message_buff[num_bytes] = '\0';
        printf("server: i've got this message from client: %s\n", message_buff);

        // let's send this message back to the client
        if (send(new_fd, message_buff, num_bytes, 0) == -1) {
            perror("send");
        }

        printf("server: continue waiting.........\n");
    }

}

/*
    get_in_addr - takes in pointer to struct of type sockaddr 
                - returns address of a string representing internet address
*/
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}