/* segbuilder: a standalone driver for the mpd-inband NGINX module */
/* copyright (C) Comcast 2025 */
/* author: ab */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/stat.h>
#include "inband_main.h"

#ifdef __cplusplus
extern "C" {
#endif

int main (int argc, char* argv[]) {
    int   c;
    int   sz;
    u_char* f = NULL;

    ngx_http_request_t r;
    ngx_http_request_body_t rb;
    r.request_body = &rb;
    ngx_temp_file_t tf;
    r.request_body->temp_file = &tf;

    while ((c = getopt(argc, argv, "a:h")) != -1)
    {
        switch (c)
        {
        case 'a':
            f = (u_char*)strdup(optarg);
            break;
        case 'h':
            printf("welcome to %s\n", argv[0]);
            return EXIT_SUCCESS;
        }
    }

    if (f == NULL)
    {
        printf("No input file, exiting\n");
        exit(1);
    }

    struct stat st;
    stat((const char*)f, &st);
    sz = st.st_size;

    ngx_file_t nfile;
    nfile.name.data = f;
    nfile.offset = sz;
    r.request_body->temp_file->file = nfile;

    printf("processing:\n");
    printf("      file: %s\n", r.request_body->temp_file->file.name.data);
    printf("   file sz: %ld\n", r.request_body->temp_file->file.offset);

    inband_process(&r, f); //NOTE: rewrites .mpd or .mp4a in place
                           //for mpd, it can be checked back out
                           //for audio seg, copy a new one to this dir

    //NOTE 2: 'cur.mpd' is written to '/dev/shm/'
    free(f);
    return EXIT_SUCCESS;
}

#ifdef __cplusplus
}
#endif


