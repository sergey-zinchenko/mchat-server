/* 
 * File:   srvsockutils.h
 * Author: Sergey Zinchenko
 *
 * Created on January 21, 2015, 2:36 PM
 */

#ifndef SRVSOCKUTILS_H
#define	SRVSOCKUTILS_H

#ifdef	__cplusplus
extern "C" {
#endif
    
    #include <stdio.h>
    #include <string.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <arpa/inet.h>
    
    int config_socket();
    int set_nonblock(int sock);
    int set_reuseaddr(int sock); 

#ifdef	__cplusplus
}
#endif

#endif	/* SRVSOCKUTILS_H */

