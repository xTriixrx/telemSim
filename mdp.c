#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "commands.h"
#include <sys/errno.h>
#include <netinet/in.h>

#define SOCKADDR struct sockaddr

static unsigned long frameCount = 1;

int removeHeader(unsigned long*);
void extractTelmetry(int*, int*);
int handleMajorFrame(char*, int*);
int commandHandler(unsigned long*);
void setSockAddr(struct sockaddr_in*, int*);

/**
* A simple implementation of a Mission Data Processor acting as a server,
* waiting for some spacecraft simulation to connect as a client.
*
* @author Vincent.Nigro
* @version 0.0.1
*/

/**
* Main function that starts up a server socket connection and allows a client
* connection to forward telemetry data until the KILL telemetry command has been
* sent from the client.
*
* @param argc
* @param argv
* @return int
*/
int main(int argc, char *argv[])
{
    struct sockaddr_in servaddr, cli;
    int socket_fd, conn_fd, PORT = -1, debug = 0;

    // Must pass a parameter containing port to open socket interface.
    if (argc < 2)
    {
        printf("Need the following arguments 1: PORT.");
        exit(1);
    }

    // If a third argument is passed handle the argument
    if (argc == 3)
    {
        if (strcmp(argv[2], "--debug") == 0)
            debug = 1;
    }

    PORT = atoi(argv[1]);
    socklen_t len = sizeof(cli);

    // socket creation with IPV4  & SOCK_STREAM (TCP)
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("BSD socket call failed: %s.\n", strerror(errno));
        exit(1);
    }

    // Zero out sockaddr_in struct
    bzero(&servaddr, sizeof(servaddr));

    // assign PORT to all local interfaces (INADDR_ANY)
    setSockAddr(&servaddr, &PORT);

    // Binding newly created socket to given sockaddr_in structure
    if ((bind(socket_fd, (SOCKADDR*) &servaddr, sizeof(servaddr))) == -1)
    {
        printf("BSD bind call failed: %s.\n", strerror(errno));
        exit(1);
    }

    // Listening for client(s) connection requests
    if ((listen(socket_fd, 32)) == -1)
    {
        printf("BSD listen call failed: %s.\n", strerror(errno));
        exit(1);
    }

    printf("Waiting for client connection to port: %d.\n", PORT);

    // Accept incoming client connection request
    if ((conn_fd = accept(socket_fd, (SOCKADDR*) &cli, &len)) == -1)
    {
        printf("BSD accept call has failed: %s.\n", strerror(errno));
        exit(1);
    }

    printf("Spacecraft has connected to MDP on port: %d.\n", PORT);

    // Function for handling incoming telemetry
    extractTelmetry(&conn_fd, &debug);

    // close socket descriptor
    close(socket_fd);

    return 0;
}

/**
* Control loop for receiving simulated major frames of telemetry created by a
* simulation client and halts once the simulated spacecraft send the KILL command
* within a major frame packet.
*
* @param fd
* @param debug_mode
* @return void
*/
void extractTelmetry(int *fd, int *debug_mode)
{
    int headerSize = -1, executing = 1;
    char buff[FRAME_SIZE * sizeof(unsigned long)];

    while (executing)
    {
        bzero(buff, sizeof(buff));
        read(*fd, buff, sizeof(buff));

        if (*debug_mode)
        {
            printf("\nMajor Frame %lu Dump\n", frameCount);
            printBits(sizeof(buff), &buff);
        }

        // Counts minor frames consumed by header
        headerSize = removeHeader((unsigned long *)buff);

        printf("Major Frame %lu\n", frameCount);

        // Handles the major frame commands
        executing = handleMajorFrame(buff, &headerSize);

        frameCount++;
    }
}

/**
* Processes a single major frame and runs the commands within each minor frame.
*
* @param buffer
* @param size
* @return int
*/
int handleMajorFrame(char *buffer, int *size)
{
    int executing = 1;
    unsigned long command;

    /*
        Loops through buffer pointer by the size of unsigned long
        and starts at the beginning of the buffer but past the header.
    */
    for (unsigned long *minorFrame = (unsigned long *)
        (buffer + (sizeof(unsigned long) * (*size))); *minorFrame; minorFrame++)
    {
        memcpy(&command, minorFrame, sizeof(*minorFrame));

        executing = commandHandler(&command);

        if (!executing || command == END)
            break;
    }
    return executing;
}

/**
* Counts the header minor frames from the simulated major frame
* and returns the minor frame count that is consumed by the header
* in order to jump over during processing of commands.
*
* @param buffer
* @return int
*/
int removeHeader(unsigned long *buffer)
{
    int headerSize = 0;
    unsigned long minorFrame;

    // Loops through buffer pointer by the size of unsigned long
    for (unsigned long *p = buffer; *p; p++)
    {
        memcpy(&minorFrame, p, sizeof(unsigned long));

        if (minorFrame == H1 || minorFrame == H2)
            headerSize++;
        else
            break;
    }
    return headerSize;
}

/**
* Handles the commands coming into the MDP and simply prints out what command
* has been issued.
*
* @param command
* @return int
*/
int commandHandler(unsigned long *command)
{
    switch (*command)
    {
        case KILL:
            printf("KILL command %lX has been issued.\n", KILL);
            return 0;
        case SOH:
            printf("SOH command %lX has been issued.\n", SOH);
            return 1;
        case GOOD:
            printf("GOOD Health command %lX has been issued.\n", GOOD);
            return 1;
        case BAD:
            printf("BAD Health command %lX has been issued.\n", BAD);
            return 1;
        case END:
            printf("\n");
            return 1;
        case ICING_ALARM:
            printf("ICING command %lX has been issued.\n", ICING_ALARM);
            return 1;
        case OVERHEAT_ALARM:
            printf("OVERHEAT command %lX has been issued.\n", OVERHEAT_ALARM);
            return 1;
        case SENSOR_1_ALARM:
            printf("SENSOR_1_ALARM command %lX has been issued.\n", SENSOR_1_ALARM);
            return 1;
        case SENSOR_2_ALARM:
            printf("SENSOR_2_ALARM command %lX has been issued.\n", SENSOR_2_ALARM);
            return 1;
        case SENSOR_3_ALARM:
            printf("SENSOR_3_ALARM command %lX has been issued.\n", SENSOR_3_ALARM);
            return 1;
        case SENSOR_4_ALARM:
            printf("SENSOR_4_ALARM command %lX has been issued.\n", SENSOR_4_ALARM);
            return 1;
        case SENSOR_5_ALARM:
            printf("SENSOR_5_ALARM command %lX has been issued.\n", SENSOR_5_ALARM);
            return 1;
        default:
            printf("Default case\n");
            return 0;
    }
}

/**
* Takes a reference to a sockaddr_in structure and populates the necessary
* fields for binding a socket descriptor to the referenced structure.
*
* @param adr
* @param port
* @return void
*/
void setSockAddr(struct sockaddr_in *adr, int *port)
{
    adr->sin_family = AF_INET;
    adr->sin_addr.s_addr = htonl(INADDR_ANY);
    adr->sin_port = htons(*port);
}
