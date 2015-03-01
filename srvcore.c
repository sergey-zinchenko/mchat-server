/* 
 * File:   srvcore.c
 * Author: Sergey Zinchenko
 *
 * Created on January 21, 2015, 5:29 PM
 */

#include "srvcore.h"


void process_client_msg(EV_P_ server_ctx_t *srv_ctx, client_ctx_t *cli_ctx, char *msg) {

    int to_array_real_count = 0;
    json_object * msg_obj = json_tokener_parse(msg);
    if (msg_obj && (json_object_get_type(msg_obj) == json_type_object)) {
        json_object * to_array_obj = json_object_object_get(msg_obj, "to");
        if (to_array_obj && (json_object_get_type(to_array_obj) == json_type_array)) {
            int to_array_len = json_object_array_length(to_array_obj);
            uuid_t *uuids_to = calloc(to_array_len, sizeof (uuid_t));
            for (int i = 0; i < to_array_len; i++) {
                json_object *to_uuid_str = json_object_array_get_idx(to_array_obj, i);
                if (to_uuid_str) {
                    if (json_object_get_type(to_uuid_str) == json_type_string) {
                        if (uuid_parse(json_object_get_string(to_uuid_str), uuids_to[i]) == -1) {
                            uuid_clear(uuids_to[i]);
                        } else {
                            to_array_real_count++;
                        }
                    } else {
                        uuid_clear(uuids_to[i]);
                    }
                    json_object_put(to_uuid_str);
                }
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
            if (to_array_real_count > 0) {
                for (int i = 0; i < to_array_real_count; i++) {
                    if (uuid_is_null(uuids_to[i]) == 1)
                        continue;
                    send_message(loop, uuids_to[i], base64_out, strlen(base64_out));
                }
            }

            free(base64_out);
            free(uuids_to);
        }
        json_object_put(msg_obj);
    }
}
