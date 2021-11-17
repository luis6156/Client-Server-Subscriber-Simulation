#ifndef SUBSCRIBER_HELPER_H
#define SUBSCRIBER_HELPER_H

#include "helpers.h"

/**
 * @brief Create a client_msg structure if the message content is in the right
 * format.
 *
 * @param buffer used to extract information
 * @param msg structure used to store message format
 * @return int 1 if the structure was created successfully, otherwise 0
 */
int create_client_msg(char *buffer, client_msg *msg);

/**
 * @brief Initializes the FDs sets and TCP socket used for communication with
 * the server.
 *
 * @param main_fds main set of FDs
 * @param tmp_fds temporary set of FDs
 * @param sockfd TCP socket
 * @param id client's ID
 * @param ip_address server's IP address
 * @param port server's port
 */
void init_subscriber(fd_set &main_fds, fd_set &tmp_fds, int &sockfd, char *id,
                     char *ip_address, char *port);

/**
 * @brief Receives a message from subscriber's STDIN. The message could be an
 * "exit" command or a subscription update that needs to be sent to the server.
 * The correct usage of the subscribe/unsubscribe command is verified.
 *
 * @param buffer used to receive the input
 * @param sockfd server's FD
 * @return int "1" if subscriber's input is "exit", otherwise "0"
 */
int recv_stdin_msg(char *buffer, int sockfd);

/**
 * @brief Receives a message from the server. The message could indicate that
 * the server closed or a subscription update has been sent and it needs to be
 * printed.
 *
 * @param buffer used to receive the message
 * @param sockfd server's FD
 * @return int "1" if the server closed, otherwise "0"
 */
int recv_tcp_msg(char *buffer, int sockfd);

#endif  // SUBSCRIBER_HELPER_H
