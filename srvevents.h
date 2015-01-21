/* 
 * File:   srvevents.h
 * Author: Sergey Zinchenko
 *
 * Created on January 21, 2015, 4:12 PM
 */

#ifndef SRVEVENTS_H
#define	SRVEVENTS_H

#ifdef	__cplusplus
extern "C" {
#endif

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
#include <json/json.h>
#include "srvtypes.h"
#include "srvsockutils.h"
#include "srvclientutils.h"
#include "base64.h"

    void client_read_write(EV_P_ struct ev_io *io, int revents);
    void on_connect(EV_P_ struct ev_io *io, int revents);
    
    
#ifdef	__cplusplus
}
#endif

#endif	/* SRVEVENTS_H */

