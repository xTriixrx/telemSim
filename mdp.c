#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "commands.h"
#include <sys/errno.h>
#include <netinet/in.h>

#define SOCKADDR struct sockaddr

static unsigned long frameCount = 1;

void argumentError();
int establishClient(int*);
int removeHeader(unsigned long*);
void extractTelmetry(int*, int*);
int handleMajorFrame(char*, int*);
int commandHandler(unsigned long*);
void ipv6ServerStartup(int*, int*);
void setSockAddr(struct sockaddr_in*, int*);
void setSock6Addr(struct sockaddr_in6*, int*);
void argumentHandler(int*, char**, char**, int*, int*);
void ipv4ServerStartup(struct sockaddr_in*, int*, int*);
void startServer(struct sockaddr_in*, int*, int*, char*);

/**
* A simple implementation of a Mission Data Processor acting as a server,
* waiting for some spacecraft simulation to connect as a client. Can accept either
* IPV4 or IPV6 protocols; can also limit the mdp server utility to only the IPV4 protocol.
*
* @author Vincent.Nigro
* @version 0.0.2
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
int main(int argc, char **argv)
{
    char *PROTOCOL = "";
    struct sockaddr_in servaddr;
    int socket_fd, conn_fd, PORT = -1, debug = 0;

    // Processes and populates command line argument fields
    argumentHandler(&argc, argv, &PROTOCOL, &PORT, &debug);

    // Starts up server utility based on cmd line protocol assignment
    startServer(&servaddr, &socket_fd, &PORT, PROTOCOL);

    // Establishes client connection returning file descriptor
    conn_fd = establishClient(&socket_fd);

    // Handling incoming telemetry from socket
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
* Establishes client connection and returns the file descriptor for the client
* socket connection.
*
* @param sock_fd
* @return int
*/
int establishClient(int *sock_fd)
{
    int cli_fd;
    socklen_t len;
    struct sockaddr_in cli;

    // Listening for client(s) connection requests
    if ((listen(*sock_fd, 32)) == -1)
    {
        printf("BSD listen call failed: %s.\n", strerror(errno));
        exit(1);
    }

    printf("Waiting for client connection to be accepted...\n");

    len = sizeof(cli);

    // Accept incoming client connection request
    if ((cli_fd = accept(*sock_fd, (SOCKADDR*) &cli, &len)) == -1)
    {
        printf("BSD accept call has failed: %s.\n", strerror(errno));
        exit(1);
    }
    printf("Spacecraft has connected to MDP.");
    return cli_fd;
}

/**
* Starts server socket after determining the protocol to start up with.
*
* @param adr
* @param sock_fd
* @param port
* @param protocol
* @return void
*/
void startServer(struct sockaddr_in *adr, int *sock_fd, int *port, char *protocol)
{
    if (strcmp(protocol, IPV4) == 0)
    {
        ipv4ServerStartup(adr, sock_fd, port);
    }
    else if (strcmp(protocol, IPV6) == 0)
    {
        ipv6ServerStartup(sock_fd, port);
    }
    else
    {
        printf("Unknown address protocol %s.\n", protocol);
        exit(1);
    }
}

/**
* Takes the command line arguments and processes the input line, setting variables
* from the caller when validation has been established.
*
* @param argc
* @param argv
* @param protocol
* @param port
* @param dbg
* @return void
*/
void argumentHandler(int *argc, char **argv, char **protocol, int *port, int *dbg)
{
    // Must pass a parameter containing port to open socket interface.
    if (*argc < 3)
        argumentError();

    // If a third argument is passed handle the argument
    if (*argc == 4)
    {
        if (strcmp(argv[3], "--debug") == 0)
            *dbg = 1;
    }

    *port = atoi(argv[1]);
    *protocol = argv[2];
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

/**
* Start up process for creating a server socket TCP stream with the IPV4 protocol.
*
* @param adr
* @param sock_fd
* @param port
* @return void
*/
void ipv4ServerStartup(struct sockaddr_in *adr, int *sock_fd, int *port)
{
    // socket creation with IPV4  & SOCK_STREAM (TCP)
    if ((*sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("BSD socket call failed: %s.\n", strerror(errno));
        exit(1);
    }

    // Zero out sockaddr_in struct
    bzero(adr, sizeof(*adr));

    // assign PORT to all local interfaces (INADDR_ANY)
    setSockAddr(adr, port);

    // Binding newly created socket to given sockaddr_in structure
    if ((bind(*sock_fd, (SOCKADDR*) adr, sizeof(*adr))) == -1)
    {
        printf("BSD bind call failed: %s.\n", strerror(errno));
        exit(1);
    }
}

/**
* Takes a reference to a sockaddr_in6 structure and populates the necessary
* fields for binding a socket descriptor to the referenced structure.
*
* @param adr
* @param port
* @return void
*/
void setSock6Addr(struct sockaddr_in6 *adr, int *port)
{
    adr->sin6_family = AF_INET6;
    adr->sin6_addr = in6addr_any;
    adr->sin6_port = htons(*port);
}

/**
* Start up process for creating a server socket TCP stream with the IPV6 protocol.
*
* @param sock_fd
* @param port
* @return void
*/
void ipv6ServerStartup(int *sock_fd, int *port)
{
    struct sockaddr_in6 adr;

    // socket creation with IPV4  & SOCK_STREAM (TCP)
    if ((*sock_fd = socket(AF_INET6, SOCK_STREAM, 0)) == -1)
    {
        printf("BSD socket call failed: %s.\n", strerror(errno));
        exit(1);
    }

    // Zero out sockaddr_in struct
    bzero(&adr, sizeof(adr));

    // assign PORT to all local interfaces (INADDR_ANY)
    setSock6Addr(&adr, port);

    // Binding newly created socket to given sockaddr_in structure
    if ((bind(*sock_fd, (SOCKADDR*) &adr, sizeof(adr))) == -1)
    {
        printf("BSD bind call failed: %s.\n", strerror(errno));
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
    printf("Need the following arguments 1: PORT 2: PROTOCOL.\n");
    printf("./mdp 8080 --INET\n");
    printf("./mdp 8080 --INET6\n");
    printf("./mdp 8080 --INET --debug\n");
    exit(1);
}
