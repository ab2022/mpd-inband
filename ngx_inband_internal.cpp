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
    /* get the temp file name and size */
    const char* incoming = (const char*)r->request_body->temp_file->file.name.data;
	size_t sz = r->request_body->temp_file->file.offset;

    /* create the out_buf, and write 'styp' to the first 4 bytes of out_buf */
    uint8_t styp[4] = {'s', 't', 'y', 'p'};
    size_t  num;
    uint8_t* out_buf = (uint8_t*)calloc(sz + 4, 1);
    memcpy(out_buf, styp, 4);

    /* read in the temp file to out_buf starting at fifth byte */
    FILE *read_fp = fopen(incoming, "rb");
    num = fread(&out_buf[4], 1, sz, read_fp);
    if (num < sz)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "INBAND process_audio num < sz");
        exit(1);
    }
    fclose(read_fp);

    /* write it out */
    FILE *write_fp = fopen(incoming, "wb");
    num = fwrite(out_buf, 1, sz + 4, write_fp);
    if (num < sz + 4)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "INBAND process_audio write_fp not enough");
        exit(1);
    }
    fclose(write_fp);
    free(out_buf);
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

