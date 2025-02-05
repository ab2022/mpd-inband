/* seg_builder: a standalone driver for the mpd-inband NGINX module */
/* copyright (C) Comcast 2025 */
/* author: ab */

#include <stdio.h>
#include "inband_main.h"

#ifdef __cplusplus
extern "C" {
#endif

int main (void) {
    ngx_http_request_t      r;
    ngx_http_request_body_t rb;

    r.request_body = &rb;

    //FIXME change to cmd line input and use fseek to get the offset
    /*
    u_char* mpdfile = (u_char*)"seg_builder/sample_manifest.mpd";
    u_char* path_str = mpdfile;
    r.request_body->temp_file->file.name.data = path_str;
	r.request_body->temp_file->file.offset = 2704;
    */

    u_char* audiofile = (u_char*)"audio-0-128000-904577953.mp4a";
    u_char* path_str = audiofile;
    r.request_body->temp_file->file.name.data = path_str;
	r.request_body->temp_file->file.offset = 31715;

    printf("   file: %s\n", r.request_body->temp_file->file.name.data);
    printf("file sz: %ld\n", r.request_body->temp_file->file.offset);

    inband_process(&r, path_str); //NOTE: rewrites .mpd or .mp4a in place
                                  //for mpd, it can be checked back out
                                  //for audio seg, copy a new one to this dir
    return EXIT_SUCCESS;
}

#ifdef __cplusplus
}
#endif


