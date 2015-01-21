/* 
 * File:   srvsockutils.c
 * Author: Sergey Zinchenko
 *
 * Created on January 21, 2015, 2:36 PM
 */

#include "srvsockutils.h"

/* Configure given socket to non blocking mode.
 * @param sock Socket to configure 
 * @return 0 if no errors and -1 otherwise.
 */
int set_nonblock(int sock) {
    long fl = fcntl(sock, F_GETFL);
    if (fl == -1)
        return -1;
    if (fcntl(sock, F_SETFL, fl | O_NONBLOCK) == -1)
        return -1;
    return 0;
}

/* Configure given socket to allow reuse address.
 * @param sock Socket to configure 
 * @return 0 if no errors and -1 otherwise.
 */
int set_reuseaddr(int sock) {
    int flag = 1;
    return setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof (flag));
}

/* Configure given socket with linger option {0, 0}.
 * @param sock Socket to configure 
 * @return 0 if no errors and -1 otherwise.
 */
int set_linger(int soc) {
    struct linger l = {0, 0};
    return setsockopt(soc, SOL_SOCKET, SO_LINGER, &l, sizeof (l));
}

/* Configure given socket to allow sending keep-alive.
 * @param sock Socket to configure.
 * @return 0 if no errors and -1 otherwise.
 */
int set_keepalive(int sock) {
    int keep_alive = 1;
    return setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keep_alive, sizeof (keep_alive));
}

/* Prepare a server socket  
 * @return -1 for error, or configured socket otherwise.  
 */
int config_socket() {

    int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1) {
        fprintf(stderr, "failed to create socket\n");
        return -1;
    }

    if (set_reuseaddr(sock) == -1) {
        fprintf(stderr, "can't set SO_REUSEADDR sock option\n");
        close(sock);
        return -1;
    }

    if (set_nonblock(sock) == -1) {
        fprintf(stderr, "can't set O_NONBLOCK socket access mode\n");
        close(sock);
        return -1;
    }

    const short port = 9000;
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof (sa));
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    if (bind(sock, (struct sockaddr *) &sa, sizeof (struct sockaddr_in)) == -1) {
        fprintf(stderr, "bind failed\n");
        close(sock);
        return -1;
    }

    if (listen(sock, 10) == -1) {
        close(sock);
        fprintf(stderr, "listen failed\n");
        return -1;
    }

    return sock;
}

/* Shutdown both read and write before closing socket. 
 * After that message will be printed into stderr.
 * @param sock Socket to close.
 * @param mag Message to print into stderr.
 */
void shutdown_printerr(int sock, char *msg) {
    if (msg)
        fprintf(stderr, msg);
    shutdown(sock, SHUT_RDWR);
    close(sock);
}