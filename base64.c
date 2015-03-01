/* 
 * File:   base64.c
 * Author: Sergey Zinchenko
 *
 * Created on January 21, 2015, 2:08 PM
 */

#include "base64.h"

char * base64_encode(const char *data, ssize_t data_len) {
    BIO *bout, *b64;
    bout = BIO_new(BIO_s_mem());
    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_push(b64, bout);
    BIO_write(b64, data, data_len);
    BIO_flush(b64);
    ssize_t buff_len = 0;
    ssize_t buff_pos = 0;
    char* buff = NULL;
    int bout_result;
    do {
        if (buff_len - buff_pos < 1024) {
            char *new_buff = realloc(buff, buff_len + 1024);
            if (!new_buff) {
                bout_result = -1;
                break;
            }
            buff = new_buff;
            buff_len += 1024;
        }
        bout_result = BIO_read(bout, &buff[buff_pos], buff_len - buff_pos);
        buff_pos += bout_result;
    } while (!BIO_eof(bout));
    BIO_free_all(b64);
    if ((bout_result >= 0)&&(buff_pos > 0)) {
        if (buff_pos == buff_len) {
            char *new_buff = realloc(buff, buff_len + 1);
            if (!new_buff) {
                free(buff);
                return NULL;
            }
            buff = new_buff;
        }
        buff[buff_pos] = '\0';
        return buff;
    } else {
        free(buff);
        return NULL;
    }
}

char * base64_decode(char *data, ssize_t data_len) {
    BIO *bin, *b64;
    bin = BIO_new_mem_buf(data, data_len);
    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_push(b64, bin);
    ssize_t buff_len = 0;
    ssize_t buff_pos = 0;
    char* buff = NULL;
    int bio_result;
    do {
        if (buff_len - buff_pos < 1024) {
            char *new_buff = realloc(buff, buff_len + 1024);
            memset(&new_buff[buff_pos], 0, buff_len - buff_pos);
            if (!new_buff) {
                bio_result = -1;
                break;
            }
            buff = new_buff;
            buff_len += 1024;
        }
        bio_result = BIO_read(b64, &buff[buff_pos], buff_len - buff_pos);
        buff_pos += bio_result;
    } while (!BIO_eof(b64));
    BIO_free_all(b64);
    if ((bio_result >= 0)&&(buff_pos > 0)) {
        if (buff_pos == buff_len) {
            char *new_buff = realloc(buff, buff_len + 1);
            if (!new_buff) {
                free(buff);
                return NULL;
            }
            buff = new_buff;
        }
        buff[buff_pos] = '\0';
        return buff;
    } else {
        free(buff);
        return NULL;
    }
}
