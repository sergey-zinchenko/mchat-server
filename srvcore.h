/* 
 * File:   srvcore.h
 * Author: user
 *
 * Created on January 21, 2015, 5:29 PM
 */

#ifndef SRVCORE_H
#define	SRVCORE_H

#ifdef	__cplusplus
extern "C" {
#endif

    #include <ev.h>
    #include <uuid/uuid.h>
    #include <json/json.h>
    #include "srvtypes.h"
    #include "base64.h"
    #include "srvclientutils.h"

    void process_client_msg(EV_P_ server_ctx_t *srv_ctx, client_ctx_t *cli_ctx, char *msg);


#ifdef	__cplusplus
}
#endif

#endif	/* SRVCORE_H */

