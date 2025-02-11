/* segbuilder: a standalone driver for the mpd-inband NGINX module */
/* copyright (C) Comcast 2025 */
/* author: ab */
/* 2/4/2025 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/stat.h>
#include "inband_main.h"

#ifdef __cplusplus
extern "C" {
#endif

ngx_file_t create_ngx_file(u_char* fname) {
    int   sz;
    ngx_file_t nfile;
    struct stat st;

    stat((const char*)fname, &st);
    sz = st.st_size;

    nfile.name.data = fname;
    nfile.offset = sz;

    return nfile;
}

void run_it(u_char* f) {

    ngx_http_request_t r;
    ngx_http_request_body_t rb;
    r.request_body = &rb;
    ngx_temp_file_t tf;
    r.request_body->temp_file = &tf;

    if (!(access((const char*)f, F_OK) == 0))
    {
        printf("Could not open \"%s\" Exiting :(\n", f);
        exit(1);
    }

    r.request_body->temp_file->file = create_ngx_file(f);
    printf("processing:\n");
    printf("      file: %s\n", r.request_body->temp_file->file.name.data);
    printf("   file sz: %ld\n", r.request_body->temp_file->file.offset);
    inband_process(&r, f);
    free(f);
}

int main (int argc, char* argv[]) {
    int   c;
    u_char* aud_f = NULL;
    u_char* mpd_f = NULL;

    while ((c = getopt(argc, argv, "a:m:h")) != -1)
    {
        switch (c)
        {
        case 'a':
            aud_f = (u_char*)strdup(optarg);
            break;
        case 'm':
            mpd_f = (u_char*)strdup(optarg);
            break;
        case 'h':
            printf("welcome to %s\n", argv[0]);
            printf("usage: \n");
            printf("  -m mpd   (if not given, \"cur.mpd\" will be used)\n");
            printf("  -a seg   (audio seg: optional)\n");
            return EXIT_SUCCESS;
        }
    }

    if (mpd_f == NULL)
    {
        const char* curmpd = "cur.mpd";
        if (!(access(curmpd, F_OK) == 0))
        {
            //curmpd mpd does not exist. we must exit
            printf("No mpd file given and no curmpd found. Exiting :(\n");
            exit(1);
        }
    }
    else //mpd was given or curmpd exists
        run_it(mpd_f);

    if (aud_f != NULL) //audio seg was given
        run_it(aud_f);

    return EXIT_SUCCESS;
}

#ifdef __cplusplus
}
#endif


