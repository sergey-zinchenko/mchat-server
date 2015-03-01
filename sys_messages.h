/* 
 * File:   sys_messages.h
 * Author: Sergey
 *
 * Created on March 2, 2015, 1:21 AM
 */

#ifndef SYS_MESSAGES_H
#define	SYS_MESSAGES_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "srvtypes.h"
#include "base64.h"
#include <uuid/uuid.h>
#include <json/json.h>
    
    char *server_welcome(server_ctx_t *srv_ctx, client_ctx_t *cli_ctx);
    char *server_client_connected(client_ctx_t *cli_ctx);

#ifdef	__cplusplus
}
#endif

#endif	/* SYS_MESSAGES_H */

