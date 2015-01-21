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
    #include <fcntl.h>
    #include <arpa/inet.h>
    
    int config_socket();

#ifdef	__cplusplus
}
#endif

#endif	/* SRVSOCKUTILS_H */

