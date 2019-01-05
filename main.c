/* 
 * File:   main.c
 * Author: Sergey Zinchenko
 *
 * Created on January 7, 2015, 6:23 PM
 */

#include "main.h"

struct ev_loop *init_server_context(int sock) {
    struct ev_loop *loop = ev_default_loop(EVFLAG_AUTO);
    if (!loop) {
        fprintf(stderr, "Failed to initialize event loop\n");
        return NULL;
    }    
    server_ctx_t * srv_ctx = (server_ctx_t *) calloc(1, sizeof (server_ctx_t));
    if (!srv_ctx) {
       fprintf(stderr, "Failed to allocate server context\n"); 
       return NULL;
    }
    srv_ctx->started_at = time(NULL);
    srv_ctx->clients_size = 64;
    srv_ctx->clients = (client_ctx_t **) calloc(srv_ctx->clients_size, sizeof (client_ctx_t *));
    if (!srv_ctx->clients) {
       fprintf(stderr, "Failed to allocate list of clients context\n"); 
       free(srv_ctx);
       return NULL;
    }
    char time_buff[32];
    strftime(time_buff, sizeof (time_buff), "%Y-%m-%d %H:%M:%S %Z", localtime(&srv_ctx->started_at));
    printf("server started at %s\n", &time_buff);
    
    ev_io_init(&srv_ctx->ss_io, on_connect, sock, EV_READ);
    ev_io_start(loop, &srv_ctx->ss_io);
    ev_set_userdata(loop, (void*) srv_ctx);

    return loop;
}

int main(int argc, char** argv) {
    
    int sock;
    if ((sock = config_socket()) == -1) {
        return (EXIT_FAILURE);
    }

    struct ev_loop *loop = init_server_context(sock);
    if (!loop) {
        fprintf(stderr, "failed to initialize event loop and server context\n");
        close(sock);
        return (EXIT_FAILURE);
    }
    ev_loop(loop, 0);

    return (EXIT_SUCCESS);
}

