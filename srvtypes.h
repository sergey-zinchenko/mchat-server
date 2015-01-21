/* 
 * File:   srvtypes.h
 * Author: Sergey Zinchenko
 *
 * Created on January 21, 2015, 1:15 PM
 */

#ifndef SRVTYPES_H
#define	SRVTYPES_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include <ev.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <time.h>
#include <netinet/in.h>    

struct client_ctx_t;

typedef struct client_ctx_t client_ctx_t;

struct io_with_cctx_t {
    ev_io io;
    client_ctx_t *ctx;
};

typedef struct io_with_cctx_t io_with_cctx_t;

struct read_ctx_t {
    char *read_buff;
    ssize_t read_buff_length, read_buff_pos, parser_pos, prev_parser_pos;
};

typedef struct read_ctx_t read_ctx_t;

struct message_buff_t {
    char *data;
    ssize_t data_length, data_pos;
};

typedef struct message_buff_t message_buff_t;

struct write_ctx_t {
    message_buff_t *buffs;
    ssize_t buffs_count, buffs_length;
};

typedef struct write_ctx_t write_ctx_t;

struct client_ctx_t {
    uuid_t uuid;
    io_with_cctx_t io;
    struct sockaddr_in client_addr;
    time_t connected_at;
    write_ctx_t w_ctx;
    read_ctx_t r_ctx;
};

struct server_ctx_t {
    ev_io ss_io;
    client_ctx_t **clients;
    ssize_t clients_size;
    ssize_t clients_count;
    time_t started_at;
};

typedef struct server_ctx_t server_ctx_t;

#ifdef	__cplusplus
}
#endif

#endif	/* SRVTYPES_H */

