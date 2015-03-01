/* 
 * File:   srvevents.c
 * Author: Sergey Zinchenko
 *
 * Created on January 21, 2015, 4:15 PM
 */

#include "srvevents.h"

void on_client_message(EV_P_ server_ctx_t *srv_ctx, client_ctx_t *cli_ctx, char *msg, size_t msg_len) {
    char *unbase_msg = base64_decode(msg, msg_len);
    if (unbase_msg) {
        process_client_msg(loop, srv_ctx, cli_ctx, unbase_msg);
        free(unbase_msg);
    }
}

void client_read_write(EV_P_ struct ev_io *io, int revents) {
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
                    size_t need_to_free = cli_ctx->r_ctx.prev_parser_pos / 1024 * 1024;
                    size_t need_to_alloc = cli_ctx->r_ctx.read_buff_length - need_to_free;
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
                char time_buff[32];
                time_t now = time(NULL);
                strftime(time_buff, sizeof (time_buff), "%Y-%m-%d %H:%M:%S %Z", localtime(&now));
                char *addr = inet_ntoa(cli_ctx->client_addr.sin_addr);
                char uuid_buff[37];
                uuid_unparse_lower(cli_ctx->uuid, (char *) &uuid_buff);
                printf("client closed connection %s:%hu %s at %s\n", addr, cli_ctx->client_addr.sin_port, &uuid_buff, &time_buff);
                close_client(loop, cli_ctx);
                return;
            }
        }
    }
    if (revents & EV_WRITE) {
        write_ctx_t *w_ctx = &cli_ctx->w_ctx;
        message_buff_t *buffs = &w_ctx->buffs[0];

        while (w_ctx->buffs_count > 0) {
            while (buffs[0].data_pos < buffs[0].data_length) {
                ssize_t writed = write(io->fd, &buffs[0].data[buffs->data_pos], buffs[0].data_length - buffs[0].data_pos);
                if (writed > 0) {
                    buffs[0].data_pos += writed;
                } else {
                    if (errno == EAGAIN)
                        break;
                    if (errno == EINTR)
                        continue;
                    break;
                }
            }
            if (buffs[0].data_pos == buffs[0].data_length) {
                free(buffs[0].data);
                memmove(&buffs[0], &buffs[1], sizeof (message_buff_t) * (w_ctx->buffs_count - 1));
                memset(&buffs[w_ctx->buffs_count - 1], 0, sizeof(message_buff_t));
                w_ctx->buffs_count--;
            } else {
                break;
            }
        }
        if (w_ctx->buffs_count == 0) {
            ev_io *io = &cli_ctx->io.io;
            ev_io_stop(loop, io);
            ev_io_set(io, io->fd, EV_READ);
            ev_io_start(loop, io);
            cli_ctx->writing = 0;
        }


    }
}

void on_connect(EV_P_ struct ev_io *io, int revents) {
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof (struct sockaddr_in);
        int client_sock = accept(io->fd, (struct sockaddr *) &client_addr, &len);
        if (client_sock >= 0) {

            if (set_nonblock(client_sock) == -1) {
                shutdown_printerr(client_sock, "can't set the socket mode O_NONBLOCK for client\n");
                return;
            }

            if (set_linger(client_sock) == -1) {
                shutdown_printerr(client_sock, "can't set SO_LINGER sock option for client\n");
                return;
            }

            if (set_keepalive(client_sock) == -1) {
                shutdown_printerr(client_sock, "can't set SO_KEEPALIVE sock option for client\n");
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
