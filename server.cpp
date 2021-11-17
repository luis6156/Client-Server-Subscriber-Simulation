#include "server_helper.h"

int main(int argc, char *argv[]) {
    int tcp_sock, udp_sock, ret;
    bool stop = false;
    char buffer[BUFLEN];

    fd_set main_fds;  // Set used to store all the FDs
    fd_set tmp_fds;   // Set used temporary for select()
    int fd_max;       // Max FD from the main set

    // Check number of arguments
    runnable_usage(argc < 2, "Usage: ./server server_port\n");

    // Set print's buffer
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    /* Data Structures */
    // TOPIC:ID (OFFLINE clients)
    unordered_map<string, unordered_set<string>> topics_id_off_map;
    // TOPIC:ID (ALL client)
    unordered_map<string, unordered_set<string>> topics_id_all_map;
    // FD:ID
    unordered_map<int, string> fd_id_map;
    // ID:FD
    unordered_map<string, int> id_fd_map;
    // ID:QUEUE (SF = 1)
    unordered_map<string, queue<struct tcp_msg>> id_history_map;
    // ONLINE USERS
    unordered_set<string> online_set;

    // Init FD sets and TCP and UDP sockets
    init_server(main_fds, tmp_fds, tcp_sock, udp_sock, argv[1], fd_max);

    // Infinite loop for commands
    while (!stop) {
        tmp_fds = main_fds;  // copy the main FD set

        // Add to temporary FD set the currently active FDs (1 or more)
        ret = select(fd_max + 1, &tmp_fds, NULL, NULL, NULL);
        DIE(ret < 0, "select");

        // Iterate through all FDs and check which ones are active right now
        for (int i = 0; i <= fd_max; i++) {
            // Check if current FD is in the temporary FDs set
            if (FD_ISSET(i, &tmp_fds)) {
                if (i == tcp_sock) {
                    // TCP connection from client
                    tcp_connection(online_set, fd_id_map, id_fd_map,
                                   id_history_map, main_fds, fd_max, tcp_sock,
                                   buffer);
                } else if (i == udp_sock) {
                    // UDP message
                    recv_udp_msg(buffer, udp_sock, topics_id_off_map,
                                 topics_id_all_map, id_history_map, id_fd_map,
                                 online_set);
                } else if (i == STDIN_FILENO) {
                    // STDIN message
                    if (recv_stdin_msg(buffer, stop)) {
                        // Server closed
                        break;
                    }
                } else {
                    // TCP message
                    recv_tcp_msg(buffer, i, main_fds, fd_max, topics_id_off_map,
                                 topics_id_all_map, fd_id_map, online_set);
                }
            }
        }
    }

    // Close all sockets
    close_sockets(main_fds, fd_max);

    return 0;
}
