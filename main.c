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
#include <uuid/uuid.h>


struct client_ctx {
    uuid_t uuid;
    ev_io io;
    struct sockaddr_in client_addr;
    time_t connected_at;
    
};

struct server_ctx {
    ev_io ss_io;
    struct client_ctx *clients;
    int clients_size;
    int clients_count;
    time_t started_at;
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
        fprintf(stderr, "can't get the socket mode\n");
        close(sock);
        return -1;
    }
    if (fcntl(sock, F_SETFL, fl | O_NONBLOCK) == -1) {
        fprintf(stderr, "can't set the socket mode O_NONBLOCK\n");
        close(sock);
        return -1;
    }
    printf("O_NONBLOCK socket access mode set\n");

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
    printf("bind succeeded\n");

    if (listen(sock, 10) == -1) {
        close(sock);
        fprintf(stderr, "listen failed\n");
        return -1;
    }
    printf("listen succeeded\n");

    return sock;
}

struct client_ctx* get_unused_client_ctx(struct server_ctx *srv_ctx) {
    if (srv_ctx->clients_count == srv_ctx->clients_size) {
        struct client_ctx *reallocated = realloc(srv_ctx->clients, (srv_ctx->clients_size + 64) * sizeof(struct client_ctx));
        if (!reallocated) {
            fprintf(stderr, "failed to reallocate client list");
            return NULL;
        }
        srv_ctx->clients = reallocated;
        srv_ctx->clients_size += 64;
        return &srv_ctx->clients[(srv_ctx->clients_count)++];
    } else {
        return &srv_ctx->clients[(srv_ctx->clients_count)++];
    }
}

static void client_read(struct ev_loop *loop, struct ev_io *io, int revents) {
    printf("client read");
}

static void on_connect(struct ev_loop *loop, struct ev_io *io, int revents) {
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof (struct sockaddr_in);
        int client_sock = accept(io->fd, (struct sockaddr *)&client_addr, &len);
        if (client_sock >= 0) {
            struct server_ctx *srv_ctx =  (struct server_ctx *) ev_userdata(loop);
            struct client_ctx* cli_ctx = get_unused_client_ctx(srv_ctx);
            memset(cli_ctx, 0, sizeof (struct client_ctx));
            cli_ctx->connected_at = time(NULL);
            uuid_generate(cli_ctx->uuid);
            memcpy(&cli_ctx->client_addr, &client_addr, sizeof(struct sockaddr_in));
            ev_io_init(&cli_ctx->io, client_read, client_sock, EV_READ);
            ev_io_start(loop, &cli_ctx->io);          
            char time_buff[32];
            strftime(time_buff, sizeof(time_buff), "%Y-%m-%d %H:%M:%S %Z", localtime(&cli_ctx->connected_at));
            char *addr = inet_ntoa(cli_ctx->client_addr.sin_addr);
            char uuid_buff[37];
            uuid_unparse_lower(cli_ctx->uuid, &uuid_buff);
            printf("client accepted %s:%hu %s at %s\n", addr, client_addr.sin_port, &uuid_buff, &time_buff);            
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

struct ev_loop *init_server_context(int sock) {
    struct ev_loop *loop = ev_default_loop(EVFLAG_AUTO);
    if (!loop)
        return NULL;
    struct server_ctx *srv_ctx = (struct server_ctx *)calloc(1, sizeof(struct server_ctx));
    srv_ctx->started_at = time(NULL);
    srv_ctx->clients_size = 64;
    srv_ctx->clients = (struct client_ctx *)calloc(srv_ctx->clients_size, sizeof(struct client_ctx));
    char time_buff[32];
    strftime(time_buff, sizeof(time_buff), "%Y-%m-%d %H:%M:%S %Z", localtime(&srv_ctx->started_at));
    printf("server started at %s\n", &time_buff);
    
    ev_io_init(&srv_ctx->ss_io, on_connect, sock, EV_READ);
    ev_io_start(loop, &srv_ctx->ss_io);
    ev_set_userdata(loop, (void*)srv_ctx);
    
    return loop;
}

int main(int argc, char** argv) {
    int sock;
    if ((sock = config_socket()) == -1) {
        return (EXIT_FAILURE);
    }
  
    struct ev_loop *loop = init_server_context(sock); 
    if (!loop) {
        fprintf(stderr, "failed to initialize event loop and server context");
    }
    ev_loop(loop, 0);
      
    return (EXIT_SUCCESS);
}

