#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "commands.h"
#include <arpa/inet.h>
#include <sys/errno.h>
#include <netdb.h>
#define SOCKADDR struct sockaddr

static unsigned long frameCount = 1;

void argumentError();
void sendData(int*, int*, double*);
void generateHeader(unsigned long*);
void ipv6AddrConnection(int*, char*, int*);
void setSockAddr(struct sockaddr_in*, char*, int*);
void generateFinalMinorFrame(unsigned long*, int*);
void setSock6Addr(struct sockaddr_in6*, char*, int*);
void generateSOHCheck(char*, const unsigned long*, int*);
void argumentHandler(int*, char**, char**, int*, double*, int*);
void ipv4AddrConnection(struct sockaddr_in*, int*, char*, int*);
int simulateSOHActivity(int*, int*, double*, const unsigned long*);
void establishConnection(struct addrinfo**, struct sockaddr_in*, int*, int*, int*, char*);

/**
* A simple implementation of a space vehicle acting as a client to a ground systems'
* processor, sending data over a socket to some server to process the data. Can connect
* to either IPV4 or IPV6 server utility mdp; mdp must be started up with the appropriate
* protocol. To limit only IPV4 connections run mdp with INET option, otherwise INET6.
*
* @author Vincent.Nigro
* @version 0.0.2
*/

/**
* Main function that starts a client connection to a server utility acting as a
* Mission Data Processor (MDP). Will continue to send frames data until a frame
* contains the KILL command.
*/
int main(int argc, char **argv)
{
    double SEC = 0;
    char *HOST = "";
    struct sockaddr_in servaddr;
    struct addrinfo hint, *res = NULL;
    int socket_fd, conn_fd, PORT = -1, debug = 0, ret;

    // Processes and populates command line argument fields
    argumentHandler(&argc, argv, &HOST, &PORT, &SEC, &debug);

    // Gets info about host address to be connected to
    ret = getaddrinfo(HOST, NULL, &hint, &res);

    // Attempts to establish connection to the host:port for determined protocol
    establishConnection(&res, &servaddr, &ret, &socket_fd, &PORT, HOST);
    freeaddrinfo(res);

    // Dumps binary data onto socket.
    sendData(&socket_fd, &debug, &SEC);

    // close the socket
    close(socket_fd);

    return 0;
}

/**
* Forwards data to the Mission Data Processor (MDP) server utility
* until the KILL command has been sent to the MDP. In this case, the
* simulated spacecraft will end the TCP connection.
*
* @param fd
* @param debug_mode
* @return void
*/
void sendData(int *fd, int *debug_mode, double *seconds)
{
    int executing = 1;

    while (executing)
        executing = simulateSOHActivity(fd, debug_mode, seconds, &GOOD);
}

/**
* A basic timed simulation producing a continuous flow of SOH checks with some
* passed health value and continuing to send this type of frame until the elapsed
* time has exceeded the configured time reference, seconds.
*
* @param fd
* @param debug_mode
* @param seconds
* @return int
*/
int simulateSOHActivity(int *fd, int *debug_mode,
        double *seconds, const unsigned long *health)
{
    int finished = 0;
    clock_t start, end;
    double elapsed = 0;
    unsigned long command;
    char buff[FRAME_SIZE * sizeof(unsigned long)];

    start = clock();

    while (!finished)
    {
        bzero(&buff, sizeof(buff));

        delay(1); // Necessary to prevent infinite loop.
        end = clock();
        elapsed = ((double)(end - start)) / CLOCKS_PER_SEC;

        if (elapsed >= *seconds)
            finished = 1;

        generateSOHCheck(buff, health, &finished);
        write(*fd, buff, sizeof(buff));

        // If statement prevents printout of dump that is never sent.
        if (!finished)
        {
            if (*debug_mode)
            {
                printf("\nMajor Frame %lu Dump\n", frameCount);
                printBits(sizeof(buff), buff);
            }
            printf("Major Frame %lu has been sent to MDP.\n", frameCount++);
        }
    }
    return finished;
}

/**
* A simulated SOH telemetry frame for a simple spacecraft language definition.
* The SOH check frame is repeated over the entire frame in case of lost minor
* frames occur.
*
* @param buffer
* @param health
* @param kill
* @return void
*/
void generateSOHCheck(char *buffer, const unsigned long *health, int *kill)
{
    generateHeader((unsigned long *)buffer);
    int minorFrameCount = 0;

    // Loops through each minor frame remaining in major frame.
    for (unsigned long *cmdItr = (unsigned long *)
            (buffer + (sizeof(unsigned long) * HEADER_WIDTH));
                minorFrameCount < (FRAME_SIZE - HEADER_WIDTH); cmdItr++)
    {
        if (minorFrameCount == (FRAME_SIZE - HEADER_WIDTH) - 1)
            generateFinalMinorFrame(cmdItr, kill);
        else if (minorFrameCount % 2 == 0)
            memcpy(cmdItr, &SOH, sizeof(*cmdItr));
        else
            memcpy(cmdItr, health, sizeof(*cmdItr));

        minorFrameCount++;
    }
}

/**
* Generates a basic header of some predefined header length of a repeated pattern.
* The header is captured and removed by the MDP server utility.
*
* @param buffer
* @return void
*/
void generateHeader(unsigned long *buffer)
{
    int headerCount = 0;

    // Creates header width by iterating through minor frames.
    for (unsigned long *cmdItr = buffer;
            headerCount < HEADER_WIDTH; cmdItr++)
    {
        if (headerCount % 2 == 0)
            memcpy(cmdItr, &H1, sizeof(*cmdItr));
        else
            memcpy(cmdItr, &H2, sizeof(*cmdItr));

        headerCount++;
    }
}

/**
* Generates the final minor frame for a major frame and signals either
* the end of the frame or the end of the communications.
*
* @param cmdBuffer
* @param kill
* @return void
*/
void generateFinalMinorFrame(unsigned long *cmdBuffer, int *kill)
{
    if (*kill)
        memcpy(cmdBuffer, &KILL, sizeof(*cmdBuffer));
    else
        memcpy(cmdBuffer, &END, sizeof(*cmdBuffer));
}

/**
* Establishes a client connection based on the client protocol type determined by
* the addrinfo pointer res.
*
* @param res
* @param adr
* @param ret
* @param sock_fd
* @param port
* @param host
* @return void
*/
void establishConnection(struct addrinfo **res, struct sockaddr_in *adr,
    int *ret, int *sock_fd, int *port, char *host)
{
    // If invalid address
    if (*ret)
    {
        printf("Invalid address: %s.\n", gai_strerror(*ret));
        exit(1);
    }

    // Determine protocol type
    if((*res)->ai_family == AF_INET)
    {
        printf("Attempting to connect to %s:%d with AF_NET protocol...\n", host, *port);
        ipv4AddrConnection(adr, sock_fd, host, port);
    }
    else if ((*res)->ai_family == AF_INET6)
    {
        printf("Attempting to connect to %s:%d with AF_NET6 protocol...\n", host, *port);
        ipv6AddrConnection(sock_fd, host, port);
    }
    else
    {
        printf("%s is an unknown address format %d\n", host, (*res)->ai_family);
        exit(1);
    }
    printf("Have connected to MDP, preparing data dump sequence.\n");
}

/**
* Takes the command line arguments and processes the input line, setting variables
* from the caller when validation has been established.
*
* @param argc
* @param argv
* @param host
* @param port
* @param sec
* @param dbg
* @return void
*/
void argumentHandler(int *argc, char **argv, char **host, int *port, double *sec, int *dbg)
{
    // Must pass parameters containing host, port, and seconds to open socket interface.
    if (*argc < 4)
        argumentError();

    // Handle special case arguments
    if (*argc == 5)
    {
        if (strcmp(argv[4], "--debug") == 0)
            *dbg = 1;
    }

    *host = argv[1];
    *port = atoi(argv[2]);
    *sec = atof(argv[3]);
}

/**
* Takes a reference to a sockadd_in structure and populates the necessary fields
* for binding a socket descriptor to the referenced structure.
*
* @param adr
* @param host
* @param port
* @return void
*/
void setSockAddr(struct sockaddr_in *adr, char *host, int *port)
{
    adr->sin_family = AF_INET;
    adr->sin_addr.s_addr = inet_addr(host);
    adr->sin_port = htons(*port);
}

/**
* Handles IPV4 client connection by setting up a sockaddr_in structure
* with the appropriate host, port and additional settings and connecting
* to a socket via the AF_INET protocol.
*
* @param adr
* @param sock_fd
* @param host
* @param port
* @return void
*/
void ipv4AddrConnection(struct sockaddr_in *adr, int *sock_fd, char *host, int *port)
{
    // socket creation with IPV4  & SOCK_STREAM (TCP)
    if ((*sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("BSD socket call failed: %s.\n", strerror(errno));
        exit(1);
    }

    // Zero out sockaddr_in struct
    bzero(adr, sizeof(*adr));

    // assign IP, PORT to where server socket interface is located.
    setSockAddr(adr, host, port);

    // connect to server socket interface
    if (connect(*sock_fd, (SOCKADDR*) adr, sizeof(*adr)) == -1)
    {
        printf("BSD connect call has failed: %s.\n", strerror(errno));
        exit(1);
    }
}

/**
* Takes a reference to a sockadd_in6 structure and populates the necessary fields
* for binding a socket descriptor to the referenced structure.
*
* @param adr
* @param host
* @param port
* @return void
*/
void setSock6Addr(struct sockaddr_in6 *adr, char *host, int *port)
{
    adr->sin6_family = AF_INET6;
    adr->sin6_port = htons(*port);
    inet_pton(AF_INET6, host, &adr->sin6_addr);
}

/**
* Handles IPV6 client connection by setting up a sockaddr_in6 structure
* with the appropriate host, port and additional settings and connecting
* to a socket via the AF_INET6 protocol.
*
* @param sock_fd
* @param host
* @param port
* @return void
*/
void ipv6AddrConnection(int *sock_fd, char *host, int *port)
{
    struct sockaddr_in6 servaddr_6;

    // socket creation with IPV6  & SOCK_STREAM (TCP)
    if ((*sock_fd = socket(AF_INET6, SOCK_STREAM, 0)) == -1)
    {
        printf("BSD socket call failed: %s.\n", strerror(errno));
        exit(1);
    }

    // Zero out sockaddr_in6 struct
    bzero(&servaddr_6, sizeof(servaddr_6));

    // assign IP, PORT to where server socket interface is located.
    setSock6Addr(&servaddr_6, host, port);

    // connect to server socket interface
    if (connect(*sock_fd, (SOCKADDR*) &servaddr_6, sizeof(servaddr_6)) == -1)
    {
        printf("BSD connect call has failed: %s.\n", strerror(errno));
        exit(1);
    }
}

/**
* Prints out a set of requirements for the process to start properly and example
* calls to the process.
*
* @return void
*/
void argumentError()
{
    printf("Need the following arguments 1: HOST 2: PORT 3: SEC\n");
    printf("./sim 127.0.0.1 8080 45\n");
    printf("./sim ::1 8080 45\n");
    printf("./sim 127.0.0.1 8080 45 --debug");
    exit(1);
}
