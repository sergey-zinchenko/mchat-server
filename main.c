/* 
 * File:   main.c
 * Author: user
 *
 * Created on January 7, 2015, 6:23 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ev.h>
#include <errno.h>
#include <time.h>


struct client_ctx{
    ev_io io;
    struct sockaddr_in clinet_addr;
    time_t connected_at;
}; 
 


/* Prepare a server socket  
 Returns -1 for error, or configured socket otherwise.  */
int config_socket() {

    int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1) {
        fprintf(stderr, "failed to create socket\n");
        return -1;
    }
    printf("socket created\n");

    int flag = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof (flag)) == -1) {
        fprintf(stderr, "can't set SO_REUSEADDR sock option\n");
        close(sock);
        return -1;
    }
    printf("SO_REUSEADDR socket option set\n");

    long fl = fcntl(sock, F_GETFL);
    if (fl == -1) {
        fprintf(stderr, "can't get the socket access mode\n");
        close(sock);
        return -1;
    }
    if (fcntl(sock, F_SETFL, fl | O_NONBLOCK) == -1) {
        fprintf(stderr, "can't set the socket access mode O_NONBLOCK\n");
        close(sock);
        return -1;
    }
    printf("O_NONBLOCK socket access mode set\n");

    const short port = 9000;
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof (sa));
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    if (bind(sock, (struct sockaddr *) &sa, sizeof (struct sockaddr_in)) == -1) {
        fprintf(stderr, "bind failed\n");
        close(sock);
        return -1;
    }
    printf("bind succeeded\n");

    if (listen(sock, 10) == -1) {
        close(sock);
        fprintf(stderr, "listen failed\n");
        return -1;
    }
    printf("listen succeeded\n");

    return sock;
}

static void on_connect(EV_P_ struct ev_io *io, int revents) {
    while (1) {
        struct sockaddr_in clinet_addr;
        socklen_t len = sizeof (struct sockaddr_in);
        int client = accept(io->fd, (struct sockaddr *)&clinet_addr, &len);
        if (client >= 0) {
            char *addr = inet_ntoa(clinet_addr.sin_addr);
            printf("client accepted %s:%hu\n", addr, clinet_addr.sin_port);
            
        } else {
            if (errno == EAGAIN)
                return;
            if (errno == EMFILE || errno == ENFILE) {
                fprintf(stderr, "out of file descriptors\n");
                return;
            } else if (errno != EINTR) {
                fprintf(stderr, "accept connections error\n");
                return;
            }
        }
    }
}

int init_server_socket_ev(int sock, struct ev_loop *loop) {


    return 0;
}

int main(int argc, char** argv) {
    int sock;
    if ((sock = config_socket()) == -1) {
        return (EXIT_FAILURE);
    }

    struct ev_loop *loop = ev_default_loop(EVFLAG_AUTO);

    ev_io mc_io;
    memset(&mc_io, 0, sizeof (ev_io));
    ev_io_init(&mc_io, on_connect, sock, EV_READ);
    ev_io_start(loop, &mc_io);

    //    if (init_server_socket_ev(sock, loop) == -1) {
    //        return (EXIT_FAILURE);
    //    }
    ev_loop(loop, 0);

    return (EXIT_SUCCESS);
}

