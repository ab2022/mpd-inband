/* seg_builder: a standalone driver for the mpd-inband NGINX module */
/* copyright (C) Comcast 2025 */
/* author: ab */

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ngx_log_error(x, y, z, ...) printf(__VA_ARGS__)

typedef struct {
    size_t      len;
    u_char     *data;
} ngx_str_t;

typedef struct  {
    ngx_str_t   name;
    off_t       offset;
} ngx_file_t;

typedef struct {
    ngx_file_t  file;
    off_t       offset;
} ngx_temp_file_t;

typedef struct {
    ngx_temp_file_t* temp_file;
} ngx_http_request_body_t;

typedef struct {
    ngx_http_request_body_t* request_body;
} ngx_http_request_t;

void inband_process(ngx_http_request_t* r, u_char* path_str);

#ifdef __cplusplus
}
#endif


