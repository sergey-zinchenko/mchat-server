/* 
 * File:   main.c
 * Author: Sergey Zinchenko
 *
 * Created on January 7, 2015, 6:23 PM
 */

#include "main.h"

client_ctx_t* get_client_ctx(server_ctx_t *srv_ctx) {
    if (srv_ctx->clients_count == srv_ctx->clients_size) {
        client_ctx_t **reallocated = (client_ctx_t **) realloc(srv_ctx->clients, (srv_ctx->clients_size + 64) * sizeof (client_ctx_t *));
        if (!reallocated) {
            fprintf(stderr, "failed to reallocate client list");
            return NULL;
        }
        srv_ctx->clients = reallocated;
        srv_ctx->clients_size += 64;
    }
    return (srv_ctx->clients[(srv_ctx->clients_count)++] = (client_ctx_t *) calloc(1, sizeof (client_ctx_t)));
}

void delete_client_ctx(server_ctx_t *srv_ctx, client_ctx_t *cli_ctx) {
    for (ssize_t i = 0; i < srv_ctx->clients_count; i++) {
        if (uuid_compare(cli_ctx->uuid, srv_ctx->clients[i]->uuid) == 0) {
            free(cli_ctx);
            memmove(&srv_ctx->clients[i], &srv_ctx->clients[i + 1], sizeof (client_ctx_t *) * (srv_ctx->clients_count - i - 1));
            srv_ctx->clients_count--;
            return;
        }
    }
}



static void add_message_to_send(client_ctx_t *cli_ctx, char *msg, size_t msg_size) {

   if (cli_ctx->w_ctx.buffs_count == cli_ctx->w_ctx.buffs_length) {
        message_buff_t *reallocated = (message_buff_t *) realloc(cli_ctx->w_ctx.buffs, (cli_ctx->w_ctx.buffs_length + 8) * sizeof (message_buff_t));
        if (!reallocated) {
            fprintf(stderr, "failed to reallocate buffers list");
            return NULL;
        }
        cli_ctx->w_ctx.buffs = reallocated;
        cli_ctx->w_ctx.buffs_length += 8;
    }
    return (cli_ctx->w_ctx.buffs[(cli_ctx->w_ctx.buffs_count)++]);
    
    
    
}

static void send_msg(EV_P_ client_ctx_t* cli_ctx) {
    
}

static void process_client_msg(EV_P_ server_ctx_t *srv_ctx, client_ctx_t *cli_ctx, char *msg) {

    json_object * msg_obj = json_tokener_parse(msg);
    json_object * to_array_obj = json_object_object_get(msg_obj, "to");
    int to_array_len = json_object_array_length(to_array_obj);
    uuid_t *uuids_to = calloc(to_array_len, sizeof (uuid_t));
    for (int i = 0; i < to_array_len; i++) {
        json_object *to_uuid_str = json_object_array_get_idx(to_array_obj, i);
        uuid_parse(json_object_get_string(to_uuid_str), uuids_to[i]);
        json_object_put(to_uuid_str);
    }
    json_object_put(to_array_obj);
    json_object_object_del(msg_obj, "to");
    json_object_object_del(msg_obj, "from");
    char uuid_buff[37];
    uuid_unparse_lower(cli_ctx->uuid, (char *) &uuid_buff);
    json_object *from_obj = json_object_new_string((char *) &uuid_buff);
    json_object_object_add(msg_obj, "from", from_obj);
    const char *out = json_object_to_json_string(msg_obj);
    char *base64_out = base64_encode(out, strlen(out));
    printf("%s\n", base64_out);
    
    free(base64_out);


    json_object_put(msg_obj);


}

static void on_client_message(EV_P_ server_ctx_t *srv_ctx, client_ctx_t *cli_ctx, char *msg, ssize_t msg_len) {

    char *unbase_msg = base64_decode(msg, msg_len);
    if (unbase_msg) {
        process_client_msg(loop, srv_ctx, cli_ctx, unbase_msg);
        free(unbase_msg);
    }
}

static void client_read_write(EV_P_ struct ev_io *io, int revents) {
    server_ctx_t *srv_ctx = (server_ctx_t *) ev_userdata(loop);
    client_ctx_t *cli_ctx = (client_ctx_t *) (((io_with_cctx_t*) io)->ctx);
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
                            on_client_message(loop, srv_ctx, cli_ctx, &cli_ctx->r_ctx.read_buff[cli_ctx->r_ctx.prev_parser_pos], cli_ctx->r_ctx.parser_pos - cli_ctx->r_ctx.prev_parser_pos + 2);
                        cli_ctx->r_ctx.parser_pos++;
                        cli_ctx->r_ctx.prev_parser_pos = cli_ctx->r_ctx.parser_pos + 1;
                    }
                }
                if (cli_ctx->r_ctx.prev_parser_pos >= 1024) {
                    ssize_t need_to_free = cli_ctx->r_ctx.prev_parser_pos / 1024 * 1024;
                    ssize_t need_to_alloc = cli_ctx->r_ctx.read_buff_length - need_to_free;
                    char *new_buf = (char *) malloc(need_to_alloc);
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

static void on_connect(EV_P_ struct ev_io *io, int revents) {
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

            server_ctx_t *srv_ctx = (server_ctx_t *) ev_userdata(loop);
            client_ctx_t* cli_ctx = get_client_ctx(srv_ctx);
            cli_ctx->io.ctx = cli_ctx;
            cli_ctx->connected_at = time(NULL);
            uuid_generate(cli_ctx->uuid);
            memcpy(&cli_ctx->client_addr, &client_addr, sizeof (struct sockaddr_in));
            ev_io_init((ev_io *) & cli_ctx->io, client_read_write, client_sock, EV_READ);

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
    EV_P = ev_default_loop(EVFLAG_AUTO);
    if (!loop)
        return NULL;
    server_ctx_t * srv_ctx = (server_ctx_t *) calloc(1, sizeof (server_ctx_t));
    srv_ctx->started_at = time(NULL);
    srv_ctx->clients_size = 64;
    srv_ctx->clients = (client_ctx_t **) calloc(srv_ctx->clients_size, sizeof (client_ctx_t *));
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

    EV_P = init_server_context(sock);
    if (!loop) {
        fprintf(stderr, "failed to initialize event loop and server context");
    }
    ev_loop(loop, 0);

    return (EXIT_SUCCESS);
}

