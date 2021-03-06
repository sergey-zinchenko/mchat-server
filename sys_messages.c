/* 
 * File:   sys_messages.h
 * Author: Sergey
 *
 * Created on March 2, 2015, 1:21 AM
 */

#include <alloca.h>

#include "sys_messages.h"

char *server_welcome(server_ctx_t *srv_ctx, client_ctx_t *cli_ctx) {
    json_object *obj = json_object_new_object();
    json_object *type_string = json_object_new_string("welcome");
    json_object *version_string = json_object_new_string("0.1");
    json_object *version_int = json_object_new_int(1);
    json_object *client_list = json_object_new_array();
    for (ssize_t i = 0; i < srv_ctx->clients_count; i++) {
        if (uuid_compare(srv_ctx->clients[i]->uuid, cli_ctx->uuid) != 0) {
            char uuid_buff[37];
            uuid_unparse_lower(srv_ctx->clients[i]->uuid, (char *) &uuid_buff);
            json_object *uuid_string = json_object_new_string((char *) &uuid_buff);
            json_object_array_add(client_list, uuid_string);
        }
    }
    json_object_object_add(obj, "type", type_string);
    json_object_object_add(obj, "version_string", version_string);
    json_object_object_add(obj, "version_int", version_int);
    json_object_object_add(obj, "clients", client_list);
    const char *out = json_object_to_json_string(obj);
    char *base64_out = base64_encode(out, strlen(out));
    json_object_put(obj);
    return base64_out;
}

static char *server_client_connect_disconnect(client_ctx_t *cli_ctx, const char *message_type) {
    json_object *obj = json_object_new_object();
    json_object *type_string = json_object_new_string(message_type);
    char uuid_buff[37];
    uuid_unparse_lower(cli_ctx->uuid, (char *) &uuid_buff);
    json_object *uuid_string = json_object_new_string((char *) &uuid_buff);
    json_object_object_add(obj, "type", type_string);
    json_object_object_add(obj, "client", uuid_string);
    const char *out = json_object_to_json_string(obj);
    char *base64_out = base64_encode(out, strlen(out));
    json_object_put(obj);
    return base64_out;    
}

char *server_client_connected(client_ctx_t *cli_ctx) {
    return server_client_connect_disconnect(cli_ctx, "connected");
}

char *server_client_disconnected(client_ctx_t *cli_ctx) {
    return server_client_connect_disconnect(cli_ctx, "disconnected");
}
