#include "server_helper.h"

void transform_msg_udp(char *buffer, unsigned short port, char *ip,
                       tcp_msg *tcp) {
    // Cast message to UDP structure message
    udp_msg *udp = (udp_msg *)buffer;

    // Set ip, port and topic members
    memcpy(tcp->ip_src, ip, sizeof(tcp->ip_src));
    tcp->port = port;
    memcpy(tcp->topic, udp->topic, sizeof(tcp->topic));
    tcp->topic[50] = 0;

    uint32_t num_int;
    float num_real;

    // Based on the data type set the data_type and content members
    switch (udp->data_type) {
        case 0:
            strcpy(tcp->data_type, "INT");
            num_int = ntohl(*(uint32_t *)(udp->content + 1));

            if (udp->content[0]) {
                tcp->content[0] = '-';
                sprintf(tcp->content + 1, "%u", num_int);
            } else {
                sprintf(tcp->content, "%u", num_int);
            }

            break;
        case 1:
            strcpy(tcp->data_type, "SHORT_REAL");

            num_real = ntohs(*(uint16_t *)udp->content);
            num_real /= 100;
            sprintf(tcp->content, "%.2f", num_real);

            break;
        case 2:
            strcpy(tcp->data_type, "FLOAT");

            num_real = ntohl(*(uint32_t *)(udp->content + 1));
            num_real /= pow(10, udp->content[5]);

            if (udp->content[0]) {
                tcp->content[0] = '-';
                sprintf(tcp->content + 1, "%f", num_real);
            } else {
                sprintf(tcp->content, "%f", num_real);
            }

            break;
        case 3:
            strcpy(tcp->data_type, "STRING");
            memcpy(tcp->content, udp->content, sizeof(tcp->content));
            break;
        default:
            printf("Unknown data type.\n");
            break;
    }
}

void init_server(fd_set &main_fds, fd_set &tmp_fds, int &tcp_sock,
                 int &udp_sock, char *port, int &fd_max) {
    int portno, ret;
    struct sockaddr_in tcp_addr, udp_addr;

    // Clear main and temporary FD sets
    FD_ZERO(&main_fds);
    FD_ZERO(&tmp_fds);

    // Create UDP socket for receiving subscription messages
    udp_sock = socket(PF_INET, SOCK_DGRAM, 0);
    DIE(udp_sock < 0, "Error UDP socket");

    // Create TCP socket for listening
    tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    DIE(tcp_sock < 0, "Error TCP socket");

    // TCP and UDP sockets properties: IPv4, any IP address and server port
    memset((char *)&tcp_addr, 0, sizeof(tcp_addr));
    memset((char *)&udp_addr, 0, sizeof(udp_addr));
    portno = atoi(port);
    DIE(portno == 0, "Error atoi port.");
    tcp_addr.sin_family = udp_addr.sin_family = AF_INET;
    tcp_addr.sin_port = udp_addr.sin_port = htons(portno);
    tcp_addr.sin_addr.s_addr = udp_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind TCP and UDP sockets
    ret = bind(tcp_sock, (struct sockaddr *)&tcp_addr, sizeof(struct sockaddr));
    DIE(ret < 0, "Error bind TCP");
    ret = bind(udp_sock, (struct sockaddr *)&udp_addr, sizeof(struct sockaddr));
    DIE(ret < 0, "Error bind UDP");

    // Make the TCP socket passive (listening socket)
    ret = listen(tcp_sock, __INT_MAX__);
    DIE(ret < 0, "Error listen.");

    // Add STDIN FD in the main FD set
    FD_SET(STDIN_FILENO, &main_fds);

    // Add the TCP and UDP sockets in the main FD set, set max FD to TCP socket
    FD_SET(tcp_sock, &main_fds);
    FD_SET(udp_sock, &main_fds);
    fd_max = tcp_sock;
}

void tcp_connection(
    unordered_set<string> &online_set, unordered_map<int, string> &fd_id_map,
    unordered_map<string, int> &id_fd_map,
    unordered_map<string, queue<struct tcp_msg>> &id_history_map,
    fd_set &main_fds, int &fd_max, int tcp_sock, char *buffer) {
    struct sockaddr_in tcp_addr;
    socklen_t tcp_len = sizeof(tcp_addr);
    tcp_msg tmp;
    int newsockfd, n, flag = 1;

    // Accept connection request from TCP client
    newsockfd = accept(tcp_sock, (struct sockaddr *)&tcp_addr, &tcp_len);
    DIE(newsockfd < 0, "Error accept.");

    // Receive client ID
    memset(buffer, 0, BUFLEN);
    n = recv(newsockfd, buffer, BUFLEN - 1, 0);
    DIE(n < 0, "recv");

    // If ID is already online, close current client connection
    if (online_set.find(buffer) != online_set.end()) {
        printf("Client %s already connected.\n", buffer);
        close(newsockfd);
        return;
    }

    // Deactivate Nagle algorithm for TCP connection
    n = setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag,
                   sizeof(int));
    DIE(n < 0, "Nagle");

    // Add new socket to the FDs set and update fd max
    FD_SET(newsockfd, &main_fds);
    if (newsockfd > fd_max) {
        fd_max = newsockfd;
    }

    // Add ID to online users
    online_set.insert(buffer);
    // Create ID:FD and FD:ID associations
    fd_id_map[newsockfd] = buffer;
    id_fd_map[buffer] = newsockfd;

    // Send queued messages to the client if he was subscribed to topics with SF
    // active
    while (!id_history_map[buffer].empty()) {
        tmp = id_history_map[buffer].front();
        n = send(newsockfd, (char *)&tmp, sizeof(tcp_msg), 0);
        DIE(n == -1, "send");
        id_history_map[buffer].pop();
    }

    printf("New client %s connected from %s:%d.\n", buffer,
           inet_ntoa(tcp_addr.sin_addr), ntohs(tcp_addr.sin_port));
}

void recv_udp_msg(
    char *buffer, int udp_sock,
    unordered_map<string, unordered_set<string>> &topics_id_off_map,
    unordered_map<string, unordered_set<string>> &topics_id_all_map,
    unordered_map<string, queue<struct tcp_msg>> &id_history_map,
    unordered_map<string, int> id_fd_map, unordered_set<string> online_set) {
    struct sockaddr_in udp_addr;
    socklen_t udp_len = sizeof(udp_addr);
    int n;
    tcp_msg tcp_send;

    // Receive UDP message
    memset(buffer, 0, BUFLEN);
    n = recvfrom(udp_sock, buffer, BUFLEN - 1, 0, (struct sockaddr *)&udp_addr,
                 &udp_len);
    // Transform message to TCP structure format
    transform_msg_udp(buffer, udp_addr.sin_port, inet_ntoa(udp_addr.sin_addr),
                      &tcp_send);

    // Iterate through all clients subscribed to the topic received
    for (string id : topics_id_all_map[tcp_send.topic]) {
        // If the client is online send the TCP structure, otherwise if he has
        // SF active add the structure to the queue for when he becomes online
        if (online_set.find(id) != online_set.end()) {
            n = send(id_fd_map[id], (char *)&tcp_send, sizeof(struct tcp_msg),
                     0);
            DIE(n == -1, "send");
        } else if (topics_id_off_map[tcp_send.topic].find(id) !=
                   topics_id_off_map[tcp_send.topic].end()) {
            id_history_map[id].push(tcp_send);
        }
    }
}

int recv_stdin_msg(char *buffer, bool &stop) {
    // Read input from STDIN
    fgets(buffer, BUFLEN - 1, stdin);

    // Check if input is "exit"
    if (strncmp(buffer, "exit", 4) == 0) {
        stop = true;
        return 1;
    }

    printf("Only input allowed is \"exit\".\n");

    return 0;
}

void recv_tcp_msg(
    char *buffer, int i, fd_set &main_fds, int &fd_max,
    unordered_map<string, unordered_set<string>> &topics_id_off_map,
    unordered_map<string, unordered_set<string>> &topics_id_all_map,
    unordered_map<int, string> fd_id_map, unordered_set<string> &online_set) {
    int n;

    // Receive TCP message
    memset(buffer, 0, BUFLEN);
    n = recv(i, buffer, BUFLEN - 1, 0);
    DIE(n < 0, "recv");

    // Check if client has closed
    if (n == 0) {
        // Set the client status to offline
        online_set.erase(fd_id_map[i]);
        // Close his connection
        close(i);

        // Clear the client from the main FDs set
        FD_CLR(i, &main_fds);

        // Update fd max
        for (int j = fd_max; j >= 3; --j) {
            if (FD_ISSET(j, &main_fds)) {
                fd_max = j;
                break;
            }
        }

        printf("Client %s disconnected.\n", fd_id_map[i].c_str());
    } else {
        // Cast the message to client message structure
        client_msg *msg = (client_msg *)buffer;

        // Check if message is of type subscribe or unsubscribe
        if (msg->subscribe) {
            // Update topics clients IDs relationships (online/offline)
            if (msg->sf) {
                topics_id_off_map[msg->topic].insert(fd_id_map[i]);
            }
            topics_id_all_map[msg->topic].insert(fd_id_map[i]);
        } else {
            // Update topics clients IDs relationships (online/offline)
            topics_id_all_map[msg->topic].erase(fd_id_map[i]);
            topics_id_off_map[msg->topic].erase(fd_id_map[i]);
        }
    }
}

void close_sockets(fd_set &main_fds, int fd_max) {
    for (int j = 3; j <= fd_max; ++j) {
        if (FD_ISSET(j, &main_fds)) {
            close(j);
        }
    }
}
