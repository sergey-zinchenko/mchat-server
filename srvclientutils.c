/* 
 * File:   srvclientutils.c
 * Author: Sergey Zinchenko
 *
 * Created on January 21, 2015, 4:43 PM
 */

#include "srvclientutils.h"

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

static void delete_client_ctx(server_ctx_t *srv_ctx, client_ctx_t *cli_ctx) {
    for (size_t i = 0; i < srv_ctx->clients_count; i++) {
        if (uuid_compare(cli_ctx->uuid, srv_ctx->clients[i]->uuid) == 0) {
            free(cli_ctx);
            memmove(&srv_ctx->clients[i], &srv_ctx->clients[i + 1], sizeof (client_ctx_t *) * (srv_ctx->clients_count - i - 1));
            srv_ctx->clients_count--;
            return;
        }
    }
}

static int add_message_to_send(client_ctx_t *cli_ctx, char *msg, size_t msg_size) {
    if (cli_ctx->w_ctx.buffs_count == cli_ctx->w_ctx.buffs_length) {
        message_buff_t *reallocated = (message_buff_t *) realloc(cli_ctx->w_ctx.buffs, (cli_ctx->w_ctx.buffs_length + 8) * sizeof (message_buff_t));
        if (!reallocated) {
            fprintf(stderr, "failed to reallocate buffers list");
            return -1;
        }
        cli_ctx->w_ctx.buffs = reallocated;
        cli_ctx->w_ctx.buffs_length += 8;

    }
    
    cli_ctx->w_ctx.buffs[cli_ctx->w_ctx.buffs_count].data = malloc(msg_size);
    if (!cli_ctx->w_ctx.buffs[cli_ctx->w_ctx.buffs_count].data) {
        fprintf(stderr, "failed to allocate buffer for message");
        return -1;
    }
    memcpy(cli_ctx->w_ctx.buffs[cli_ctx->w_ctx.buffs_count].data, msg, msg_size);
    cli_ctx->w_ctx.buffs[cli_ctx->w_ctx.buffs_count].data_length = msg_size;
    cli_ctx->w_ctx.buffs[cli_ctx->w_ctx.buffs_count].data_pos = 0;
    cli_ctx->w_ctx.buffs_count++;
    return 0;
}

void send_message(EV_P_ uuid_t recipient, char *msg, size_t msg_size) {
    server_ctx_t *srv_ctx = (server_ctx_t *) ev_userdata(loop);
    for (ssize_t j = 0; j < srv_ctx->clients_count; j++) {
        if (uuid_compare(recipient, srv_ctx->clients[j]->uuid) == 0) {
            if (add_message_to_send(srv_ctx->clients[j], msg, msg_size) == 0) {
                if (srv_ctx->clients[j]->writing == 0) {
                    ev_io *io = &srv_ctx->clients[j]->io.io;
                    ev_io_stop(loop, io);
                    ev_io_set(io, io->fd, EV_READ | EV_WRITE);
                    ev_io_start(loop, io);
                    srv_ctx->clients[j]->writing = 1;
                }
            }
            break;
        }
    }
}

static void erase_client(server_ctx_t *srv_ctx, client_ctx_t *cli_ctx) {

    free(cli_ctx->r_ctx.read_buff);
    cli_ctx->r_ctx.read_buff_length = 0;
    cli_ctx->r_ctx.read_buff_pos = 0;
    cli_ctx->r_ctx.parser_pos = 0;
    cli_ctx->r_ctx.prev_parser_pos = 0;

    for (ssize_t i = 0; i < cli_ctx->w_ctx.buffs_count; i++)
        free(cli_ctx->w_ctx.buffs[i].data);
    free(cli_ctx->w_ctx.buffs);
    cli_ctx->w_ctx.buffs_count = 0;
    cli_ctx->w_ctx.buffs_length = 0;
    cli_ctx->writing = 0;

    delete_client_ctx(srv_ctx, cli_ctx);
}

void close_client(EV_P_ client_ctx_t *cli_ctx) {
    server_ctx_t *srv_ctx = (server_ctx_t *) ev_userdata(loop);
    ev_io *io = &cli_ctx->io.io;
    ev_io_stop(loop, io);
    close(io->fd);
    erase_client(srv_ctx, cli_ctx);
}