#include "subscriber_helper.h"

int main(int argc, char *argv[]) {
    int sockfd;
    char buffer[BUFLEN];
    
    fd_set main_fds;  // Set used to store all the FDs
    fd_set tmp_fds;   // Set used temporary for select()

    // Check number of arguments
    runnable_usage(
        argc < 4, "Usage: ./subscriber id_client server_address server_port\n");

    // Set print's buffer
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    init_subscriber(main_fds, tmp_fds, sockfd, argv[1], argv[2], argv[3]);

    // Infinite loop for commands
    while (1) {
        tmp_fds = main_fds;

        // Select currently active socket
        int sel = select(sockfd + 1, &tmp_fds, NULL, NULL, NULL);
        DIE(sel < 0, "select");

        // Check if current socket is STDIN or the TCP server socket
        if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
            if (recv_stdin_msg(buffer, sockfd)) {
                // Subscriber closed
                break;
            }
        } else if (FD_ISSET(sockfd, &tmp_fds)) {
            if (recv_tcp_msg(buffer, sockfd)) {
                // Server closed
                break;
            }
        }
    }

    // Close connection
    close(sockfd);
    return 0;
}
