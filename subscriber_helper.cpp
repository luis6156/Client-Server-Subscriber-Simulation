#include "subscriber_helper.h"

int create_client_msg(char *buffer, client_msg *msg) {
    char *token = strtok(buffer, " \n");

    // Check if no words were entered
    if (!token) {
        return 0;
    }

    // Check if message is subscribe or unsubscribe
    if (!strcmp(token, "subscribe")) {
        token = strtok(NULL, " \n");
        // Check if only one word was entered
        if (!token) {
            return 0;
        }

        // Subscribe and topic members
        msg->subscribe = true;
        strcpy(msg->topic, token);

        token = strtok(NULL, " \n");
        // Check if only two words were entered
        if (!token) {
            return 0;
        }

        // Check if SF has a valid value and complete SF member
        if (!strcmp(token, "1")) {
            msg->sf = true;
        } else if (!strcmp(token, "0")) {
            msg->sf = false;
        } else {
            return 0;
        }
    } else if (!strcmp(token, "unsubscribe")) {
        token = strtok(NULL, " \n");
        // Check if only one word was entered
        if (!token) {
            return 0;
        }
        // Subscribe and topic members
        msg->subscribe = false;
        strcpy(msg->topic, token);
    } else {
        return 0;
    }

    return 1;
}

void init_subscriber(fd_set &main_fds, fd_set &tmp_fds, int &sockfd, char *id,
                     char *ip_address, char *port) {
    int n, flag = 1;
    struct sockaddr_in serv_addr;

    // Clear main and temporary FD sets
    FD_ZERO(&main_fds);
    FD_ZERO(&tmp_fds);

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "socket");

    // Socket properties: IPv4, server IP address and server port
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(port));
    DIE(inet_aton(ip_address, &serv_addr.sin_addr) == 0, "inet_aton");

    // Connect TCP socket to the server
    DIE(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0,
        "connect");

    // Deactivate Nagle algorithm for TCP connection
    n = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag,
                   sizeof(int));
    DIE(n < 0, "Nagle");

    // Add TCP socket and STDIN in FD set
    FD_SET(sockfd, &main_fds);
    FD_SET(STDIN_FILENO, &main_fds);

    // Send the ID of the client to the server
    n = send(sockfd, id, strlen(id) + 1, 0);
    DIE(n < 0, "send");
}

int recv_stdin_msg(char *buffer, int sockfd) {
    int n;

    // Read line from STDIN
    memset(buffer, 0, BUFLEN);
    fgets(buffer, BUFLEN - 1, stdin);

    // Check if message is of type exit or subscribe/unsubscribe
    if (strncmp(buffer, "exit", 4) == 0) {
        return 1;
    } else {
        client_msg msg_send;
        // Check if message usage is correct
        if (create_client_msg(buffer, &msg_send)) {
            // Send message as struct
            n = send(sockfd, (char *)&msg_send, sizeof(msg_send), 0);
            DIE(n < 0, "send");

            if (msg_send.subscribe) {
                printf("Subscribed to topic.\n");
            } else {
                printf("Unsubscribed from topic.\n");
            }
        } else {
            printf(
                "Input allowed: \"subscribe topic SF\" (SF = 1/0) or "
                "\"unsubscribe topic\"\n");
        }
    }

    return 0;
}

int recv_tcp_msg(char *buffer, int sockfd) {
    int n;

    // Message received from server
    memset(buffer, 0, BUFLEN);
    n = recv(sockfd, buffer, sizeof(tcp_msg), 0);
    DIE(n < 0, "recv");

    // Check if the server closed
    if (n == 0) {
        return 1;
    }

    // Cast message to TCP struct and print it
    tcp_msg *msg_recv = (tcp_msg *)buffer;
    printf("%s:%d - %s - %s - %s\n", msg_recv->ip_src, msg_recv->port,
           msg_recv->topic, msg_recv->data_type, msg_recv->content);

    return 0;
}
