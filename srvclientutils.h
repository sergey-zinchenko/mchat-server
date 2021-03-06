/* 
 * File:   srvclientutils.h
 * Author: Sergey Zinchenko
 *
 * Created on January 21, 2015, 4:43 PM
 */

#ifndef SRVCLIENTUTILS_H
#define	SRVCLIENTUTILS_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "srvtypes.h"
#include "srvcore.h"

    
client_ctx_t* get_client_ctx(server_ctx_t *srv_ctx);
void close_client(EV_P_ client_ctx_t *cli_ctx);
void send_message(EV_P_ uuid_t recipient, char *msg, size_t msg_size);


#ifdef	__cplusplus
}
#endif

#endif	/* SRVCLIENTUTILS_H */

