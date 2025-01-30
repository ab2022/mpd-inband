/*
* Copyright (C) ab 1/29/25
*/

#include <iostream>
#include <sstream>
#include <string>

#include "pugixml.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


void process_audio(ngx_http_request_t* r) {
    if (r == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "INBAND process audio mpd r is NULL");
        exit(1);
    }

#if 0
    uint8_t styp[4] = {'s', 't', 'y', 'p'};
    size_t  num;
    uint8_t* out_buf = (uint8_t*)calloc(sz + 4, 1);
    memcpy(out_buf, styp, 4);

    FILE *read_fp = fopen(incoming, "rb");
    num = fread(&out_buf[4], sz, 1, read_fp);
    if (num < sz)
    {
        write(logfd, "num<sz\0", 7);
        exit(1);
    }
    fclose(read_fp);
    free(out_buf);
#endif

}

void process_mpd(ngx_http_request_t* r) {
    if (r == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "INBAND process mpd r is NULL");
        exit(1);
    }
}

void inband_process(ngx_http_request_t *r, u_char* path_str) {

    char* point = NULL;

    if ( (point = strrchr((char*)path_str,'.')) != NULL )
    {
        if (strcmp(point,".mp4a") == 0)
            process_audio(r);
		else
            process_mpd(r);
    }
}

#ifdef __cplusplus
}
#endif

