#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "commands.h"
#include <arpa/inet.h>
#include <sys/errno.h>

#define SOCKADDR struct sockaddr

static unsigned long frameCount = 1;

void sendData(int*, int*, double*);
void generateHeader(unsigned long*);
void setSockAddr(struct sockaddr_in*, char*, int*);
void generateFinalMinorFrame(unsigned long*, int*);
void generateSOHCheck(char*, const unsigned long*, int*);
int simulateSOHActivity(int*, int*, double*, const unsigned long*);

/**
* A simple implementation of a space vehicle acting as a client to a ground systems'
* processor, sending data over a socket to some server to process the data.
*
* @author Vincent.Nigro
* @version 0.0.1
*/

/**
* Main function that starts a client connection to a server utility acting as a
* Mission Data Processor (MDP). Will continue to send frames data until a frame
* contains the KILL command.
*/
int main(int argc, char *argv[])
{
    double SEC = 0;
    char *HOST = "";
    struct sockaddr_in servaddr, cli;
    int socket_fd, conn_fd, PORT = -1, debug = 0;

    // Must pass parameters containing host and port to open socket interface.
    if (argc < 4)
    {
        printf("Need the following arguments 1: HOST 2: PORT 3: SEC");
        exit(1);
    }

    // Handle special case arguments
    if (argc == 5)
    {
        if (strcmp(argv[4], "--debug") == 0)
            debug = 1;
    }

    HOST = argv[1];
    PORT = atoi(argv[2]);
    SEC = atof(argv[3]);

    // socket creation with IPV4  & SOCK_STREAM (TCP)
    ;
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("BSD socket call failed: %s.\n", strerror(errno));
        exit(1);
    }

    // Zero out sockaddr_in struct
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT to where server socket interface is located.
    setSockAddr(&servaddr, HOST, &PORT);

    // connect to server socket interface
    if (connect(socket_fd, (SOCKADDR*) &servaddr, sizeof(servaddr)) == -1)
    {
        printf("BSD connect call has failed: %s.\n", strerror(errno));
        exit(1);
    }

    printf("Have connected to MDP, preparing data dump sequence.\n");

    if (SEC == 0)
        SEC = HALF_MINUTE;

    // Dumps binary data onto socket.
    sendData(&socket_fd, &debug, &SEC);

    // close the socket
    close(socket_fd);
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
