/* 
 * File:   main.c
 * Author: Sergey Zinchenko
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
#include <openssl/evp.h>
#include <openssl/bio.h>

struct client_ctx;

struct io_with_cctx {
    ev_io io;
    struct client_ctx *ctx;
};


struct read_ctx {
    char *read_buff;
    ssize_t read_buff_length, read_buff_pos, parser_pos, prev_parser_pos;
};

struct message_to_write {
    char *write_buff;
    ssize_t write_buf_length, write_buff_pos;
};

struct write_ctx {
    struct message_to_write *buffs;
    ssize_t buffs_cout, buffs_length;
};

struct client_ctx {
    uuid_t uuid;
    struct io_with_cctx io;
    struct sockaddr_in client_addr;
    time_t connected_at;
    struct write_ctx w_ctx;
    struct read_ctx r_ctx;
};

struct server_ctx {
    ev_io ss_io;
    struct client_ctx **clients;
    ssize_t clients_size;
    ssize_t clients_count;
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

struct client_ctx* get_client_ctx(struct server_ctx *srv_ctx) {
    if (srv_ctx->clients_count == srv_ctx->clients_size) {
        struct client_ctx **reallocated = (struct client_ctx **)realloc(srv_ctx->clients, (srv_ctx->clients_size + 64) * sizeof (struct client_ctx *));
        if (!reallocated) {
            fprintf(stderr, "failed to reallocate client list");
            return NULL;
        }
        srv_ctx->clients = reallocated;
        srv_ctx->clients_size += 64;
    } 
    return (srv_ctx->clients[(srv_ctx->clients_count)++] = (struct client_ctx *) calloc(1, sizeof(struct client_ctx)));
}

void delete_client_ctx(struct server_ctx *srv_ctx, struct client_ctx *cli_ctx) {
    for (ssize_t i = 0; i < srv_ctx->clients_count; i++) {
        if (uuid_compare(cli_ctx->uuid, srv_ctx->clients[i]->uuid) == 0) {
            free(cli_ctx);
            memmove(&srv_ctx->clients[i], &srv_ctx->clients[i + 1], sizeof(struct client_ctx *) * (srv_ctx->clients_count - i - 1));
            srv_ctx->clients_count--;
            return;
        }
    }
}


static void on_client_message(struct ev_loop *loop, struct server_ctx *srv_ctx, struct client_ctx *cli_ctx, char *msg, ssize_t msg_len) {
   
    BIO *bin, *b64;
    bin = BIO_new_mem_buf(msg, msg_len + 2);
    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_push(b64, bin);
    ssize_t buff_len = 0;
    ssize_t buff_pos = 0;
    char* buff = NULL;
    int bio_result;
    do {
        if (buff_len - buff_pos < 1024) {
           char *new_buff = realloc(buff, buff_len + 1024);
           memset(&new_buff[buff_pos], 0, buff_len - buff_pos);
           if (!new_buff) {
               bio_result = -1;
               break;
           }
           buff = new_buff;
           buff_len += 1024;
        }
        bio_result = BIO_read(b64, &buff[buff_pos], buff_len - buff_pos);
        buff_pos += bio_result;
    } while (bio_result > 0);
    BIO_free_all(b64);
    if ((bio_result == 0)&&(buff_pos > 0)) {
        printf("%s\n", buff);
    }
    free(buff);
}

static void client_read_write(struct ev_loop *loop, struct ev_io *io, int revents) {
    struct server_ctx *srv_ctx = (struct server_ctx *) ev_userdata(loop);
    struct client_ctx *cli_ctx = (struct client_ctx *) (((struct io_with_cctx*) io)->ctx);
    if (revents & EV_READ) {
        while (1) {
            if (cli_ctx->r_ctx.read_buff_length == cli_ctx->r_ctx.read_buff_pos) {
                char *new_buff = realloc(cli_ctx->r_ctx.read_buff, cli_ctx->r_ctx.read_buff_length + 1024);
                if (!new_buff)
                    break;
                cli_ctx->r_ctx.read_buff = new_buff;
                cli_ctx->r_ctx.read_buff_length += 1024;
            }
            ssize_t readed = read(io->fd, &cli_ctx->r_ctx.read_buff[cli_ctx->r_ctx.read_buff_pos], cli_ctx->r_ctx.read_buff_length - cli_ctx->r_ctx.read_buff_pos);
            if (readed > 0) {
                cli_ctx->r_ctx.read_buff_pos += readed;
                for (; cli_ctx->r_ctx.parser_pos < cli_ctx->r_ctx.read_buff_pos - 1; cli_ctx->r_ctx.parser_pos++) {
                    if ((cli_ctx->r_ctx.read_buff[cli_ctx->r_ctx.parser_pos] == '\r')&&(cli_ctx->r_ctx.read_buff[cli_ctx->r_ctx.parser_pos + 1] == '\n')) {
                        if (cli_ctx->r_ctx.parser_pos - cli_ctx->r_ctx.prev_parser_pos > 0)
                            on_client_message(loop, srv_ctx, cli_ctx, &cli_ctx->r_ctx.read_buff[cli_ctx->r_ctx.prev_parser_pos], cli_ctx->r_ctx.parser_pos - cli_ctx->r_ctx.prev_parser_pos);   
                        cli_ctx->r_ctx.parser_pos++;
                        cli_ctx->r_ctx.prev_parser_pos = cli_ctx->r_ctx.parser_pos + 1;
                    }
                }
                if (cli_ctx->r_ctx.prev_parser_pos >= 1024) {
                    ssize_t need_to_free = cli_ctx->r_ctx.prev_parser_pos / 1024 * 1024;
                    ssize_t need_to_alloc = cli_ctx->r_ctx.read_buff_length - need_to_free;
                    char *new_buf = (char *)malloc(need_to_alloc);
                    if (!new_buf)
                        break;
                    memcpy(new_buf, &cli_ctx->r_ctx.read_buff[need_to_free], need_to_alloc);
                    free(cli_ctx->r_ctx.read_buff);
                    cli_ctx->r_ctx.read_buff = new_buf;
                    cli_ctx->r_ctx.prev_parser_pos -= need_to_free;
                    cli_ctx->r_ctx.parser_pos -= need_to_free;
                    cli_ctx->r_ctx.read_buff_pos -= need_to_free;
                    cli_ctx->r_ctx.read_buff_length = need_to_alloc;   
                }
            } else if (readed < 0) {
                if (errno == EAGAIN)
                    break;
                if (errno == EINTR)
                    continue;
                return;
            } else {
                ev_io_stop(loop, io);
                close(io->fd);
                free(cli_ctx->r_ctx.read_buff);
                cli_ctx->r_ctx.read_buff = NULL;
                cli_ctx->r_ctx.read_buff_length = 0;
                cli_ctx->r_ctx.read_buff_pos = 0;
                cli_ctx->r_ctx.parser_pos = 0;
                cli_ctx->r_ctx.prev_parser_pos = 0;
                char time_buff[32];
                time_t now = time(NULL);
                strftime(time_buff, sizeof (time_buff), "%Y-%m-%d %H:%M:%S %Z", localtime(&now));
                char *addr = inet_ntoa(cli_ctx->client_addr.sin_addr);
                char uuid_buff[37];
                uuid_unparse_lower(cli_ctx->uuid, (char *) &uuid_buff);
                printf("client closed connection %s:%hu %s at %s\n", addr, cli_ctx->client_addr.sin_port, &uuid_buff, &time_buff);
                delete_client_ctx(srv_ctx, cli_ctx);
                return;
            }
        }
    }
    if (revents & EV_WRITE) {

    }
}

static void on_connect(struct ev_loop *loop, struct ev_io *io, int revents) {
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof (struct sockaddr_in);
        int client_sock = accept(io->fd, (struct sockaddr *) &client_addr, &len);
        if (client_sock >= 0) {

            long fl = fcntl(client_sock, F_GETFL);
            if (fl == -1) {
                fprintf(stderr, "can't get the socket mode for client\n");
                shutdown(client_sock, SHUT_RDWR);
                close(client_sock);
                return;
            }
            if (fcntl(client_sock, F_SETFL, fl | O_NONBLOCK) == -1) {
                fprintf(stderr, "can't set the socket mode O_NONBLOCK for client\n");
                shutdown(client_sock, SHUT_RDWR);
                close(client_sock);
                return;
            }
            struct linger l = {0, 0};
            if (setsockopt(client_sock, SOL_SOCKET, SO_LINGER, &l, sizeof (l)) == -1) {
                fprintf(stderr, "can't set SO_LINGER sock option for client\n");
                shutdown(client_sock, SHUT_RDWR);
                close(client_sock);
                return;
            }
            int keep_alive = 1;
            if (setsockopt(client_sock, SOL_SOCKET, SO_KEEPALIVE, &keep_alive, sizeof (keep_alive)) == -1) {
                fprintf(stderr, "can't set SO_KEEPALIVE sock option for client\n");
                shutdown(client_sock, SHUT_RDWR);
                close(client_sock);
                return;
            }

            struct server_ctx *srv_ctx = (struct server_ctx *) ev_userdata(loop);
            struct client_ctx* cli_ctx = get_client_ctx(srv_ctx);
            cli_ctx->io.ctx = cli_ctx;
            cli_ctx->connected_at = time(NULL);
            uuid_generate(cli_ctx->uuid);
            memcpy(&cli_ctx->client_addr, &client_addr, sizeof (struct sockaddr_in));
            ev_io_init((ev_io *) & cli_ctx->io, client_read_write, client_sock, EV_READ | EV_WRITE);

            ev_io_start(loop, (ev_io *) & cli_ctx->io);
            char time_buff[32];
            strftime(time_buff, sizeof (time_buff), "%Y-%m-%d %H:%M:%S %Z", localtime(&cli_ctx->connected_at));
            char *addr = inet_ntoa(cli_ctx->client_addr.sin_addr);
            char uuid_buff[37];
            uuid_unparse_lower(cli_ctx->uuid, (char *) &uuid_buff);
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
    struct server_ctx * srv_ctx = (struct server_ctx *) calloc(1, sizeof (struct server_ctx));
    srv_ctx->started_at = time(NULL);
    srv_ctx->clients_size = 64;
    srv_ctx->clients = (struct client_ctx **) calloc(srv_ctx->clients_size, sizeof (struct client_ctx *));
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
        fprintf(stderr, "failed to initialize event loop and server context");
    }
    ev_loop(loop, 0);

    return (EXIT_SUCCESS);
}

