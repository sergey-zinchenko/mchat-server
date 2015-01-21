/* 
 * File:   base64.h
 * Author: user
 *
 * Created on January 21, 2015, 2:07 PM
 */

#ifndef BASE64_H
#define	BASE64_H

#ifdef	__cplusplus
extern "C" {
#endif

    #include <openssl/evp.h>
    #include <openssl/bio.h>
    #include <unistd.h>
    #include <string.h>
    
    char * base64_encode(const char *data, ssize_t data_len);
    char * base64_decode(char *data, ssize_t data_len);
    
#ifdef	__cplusplus
}
#endif

#endif	/* BASE64_H */

