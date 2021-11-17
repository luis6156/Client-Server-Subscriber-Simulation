#ifndef _HELPERS_H
#define _HELPERS_H

#include <arpa/inet.h>
#include <math.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdio>
#include <queue>
#include <unordered_map>
#include <unordered_set>

using namespace std;

#define BUFLEN 1600

// Stores the server subscription update message that will be sent to the
// subscriber to be printed.
struct __attribute__((packed)) tcp_msg {
    char ip_src[16];      // UDP client's IP address
    unsigned short port;  // Port from which the message was received
    char topic[51];       // Topic
    char data_type[11];   // Data Type
    char content[1501];   // Content
};

// Stores the client subscription message that will be sent to the
// server.
struct __attribute__((packed)) client_msg {
    bool subscribe;  // Subscribe/Unsubscribe (true or false)
    char topic[51];  // Topic
    bool sf;         // SF (true of false)
};

// Macro for executable usage
#define runnable_usage(assertion, call_description) \
    do {                                            \
        if (assertion) {                            \
            fprintf(stderr, call_description);      \
            exit(EXIT_FAILURE);                     \
        }                                           \
    } while (0)

// Macro for errors
#define DIE(assertion, call_description)                       \
    do {                                                       \
        if (assertion) {                                       \
            fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__); \
            perror(call_description);                          \
            exit(EXIT_FAILURE);                                \
        }                                                      \
    } while (0)

#endif
