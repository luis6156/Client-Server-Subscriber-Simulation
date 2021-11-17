#ifndef SERVER_HELPER_H
#define SERVER_HELPER_H

#include "helpers.h"

struct __attribute__((packed)) udp_msg {
    char topic[50];      // topic
    uint8_t data_type;   // data type
    char content[1501];  // content
};

/**
 * @brief Transforms an UDP message to a ready to send TCP structure message.
 * The type of message can be "INT", "SHORT_REAL", "FLOAT" and "STRING".
 *
 * @param buffer input from a UDP client
 * @param port port used to receive the message
 * @param ip IP address of the UDP client
 * @param tcp structure used to store the result message format
 */
void transform_msg_udp(char *buffer, unsigned short port, char *ip,
                       tcp_msg *tcp);

/**
 * @brief Initializes the FDs sets, UDP socket and TCP passive socket (used for
 * listening).
 *
 * @param main_fds main set of FDs
 * @param tmp_fds temporary set of FDs
 * @param tcp_sock TCP passive socket
 * @param udp_sock UDP socket
 * @param port port used for server
 * @param fd_max maximum FD
 */
void init_server(fd_set &main_fds, fd_set &tmp_fds, int &tcp_sock,
                 int &udp_sock, char *port, int &fd_max);

/**
 * @brief Used for receiving a connection request from a TCP client. The
 * connection is refused if the client ID is already online.
 *
 * @param online_set used to get the online clients by ID
 * @param fd_id_map used to get the FD to ID relationship
 * @param id_fd_map used to get the ID to FD relationship
 * @param id_history_map used to get the queued messages for a client by ID
 * @param main_fds main set of FDs
 * @param fd_max maximum FD
 * @param tcp_sock TCP passive socket
 * @param buffer used to receive the client's ID
 */
void tcp_connection(
    unordered_set<string> &online_set, unordered_map<int, string> &fd_id_map,
    unordered_map<string, int> &id_fd_map,
    unordered_map<string, queue<struct tcp_msg>> &id_history_map,
    fd_set &main_fds, int &fd_max, int tcp_sock, char *buffer);

/**
 * @brief Receives an UDP message which will be sent to the online subscribers
 * or be stored in an user specific queue if they have the SF active.
 *
 * @param buffer used to receive the message
 * @param udp_sock UDP socket
 * @param topics_id_off_map used to store all topics to active SF clients IDs
 * relationships
 * @param topics_id_all_map used to store all topics to clients IDs
 * relationships
 * @param id_history_map used to store the queue of messages for the offline
 * clients with the SF active
 * @param id_fd_map used to get the ID to FD relationship
 * @param online_set used to get the online clients by ID
 */
void recv_udp_msg(
    char *buffer, int udp_sock,
    unordered_map<string, unordered_set<string>> &topics_id_off_map,
    unordered_map<string, unordered_set<string>> &topics_id_all_map,
    unordered_map<string, queue<struct tcp_msg>> &id_history_map,
    unordered_map<string, int> id_fd_map, unordered_set<string> online_set);

/**
 * @brief Receives a message from server's STDIN. Only accepted message is
 * "exit".
 *
 * @param buffer used to receive the input
 * @param stop flag that stops the server's execution
 * @return int "1" if the server's input was "exit", "0" otherwise
 */
int recv_stdin_msg(char *buffer, bool &stop);

/**
 * @brief Receives a message from a TCP client socket. The message could either
 * signal that the client has closed its connection or that it has sent a
 * subscription update.
 *
 * @param buffer used to receive the message
 * @param i current TCP client FD
 * @param main_fds main set of FDs
 * @param fd_max maximum FD
 * @param topics_id_off_map used to store all topics to active SF clients IDs
 * relationships
 * @param topics_id_all_map used to store all topics to clients IDs
 * relationships
 * @param fd_id_map used to get the FD to ID relationship
 * @param online_set used to get the online clients by ID
 */
void recv_tcp_msg(
    char *buffer, int i, fd_set &main_fds, int &fd_max,
    unordered_map<string, unordered_set<string>> &topics_id_off_map,
    unordered_map<string, unordered_set<string>> &topics_id_all_map,
    unordered_map<int, string> fd_id_map, unordered_set<string> &online_set);

/**
 * @brief Closes all sockets from the main FD set.
 *
 * @param main_fds
 * @param fd_max
 */
void close_sockets(fd_set &main_fds, int fd_max);

#endif
