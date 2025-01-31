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

#define CURMPD (const char*)"/dev/shm/cur.mpd"


void process_audio(ngx_http_request_t* r) {
    FILE* mpd_fp;
    mpd_fp = fopen(CURMPD, "rb");
    if (mpd_fp == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "could not open mpd file \"%s\" for reading", CURMPD);
        return;
    }
    fclose(mpd_fp);

    /*
    get_tfdt((char*)"/home/ab/work/vid/audio-0-128000-904577953.mp4a", &seg_ctx);
    get_timescale((char*)"sample_manifest.mpd", &seg_ctx);
    FILE* fp;
    fp = fopen(seg_filename, "wb");
    if (fp == NULL)
    {
        printf("could not open file %s for writing \n", seg_filename);
        exit(1);
    }

    write_styp(fp);
    write_emsg(fp, ctx);
    concat_audio_seg(fp, ctx);
    fclose(fp);
    */

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
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "_INBAND_ process_audio num < sz");
        exit(1);
    }
    fclose(read_fp);

    /* write it out */
    FILE *write_fp = fopen(incoming, "wb");
    num = fwrite(out_buf, 1, sz + 4, write_fp);
    if (num < sz + 4)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "_INBAND_ process_audio write_fp not enough");
        exit(1);
    }
    fclose(write_fp);
    free(out_buf);
}

void process_mpd(ngx_http_request_t* r) {
    if (r == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "_INBAND_ process_mpd r is NULL");
        exit(1);
    }

    const char* incoming = (const char*)r->request_body->temp_file->file.name.data;
    pugi::xml_document doc;
    doc.load_file((const char*)incoming);

    pugi::xpath_query query_scan_type("/MPD/Period/AdaptationSet");
    pugi::xpath_node_set scan_items = query_scan_type.evaluate_node_set(doc);
    for (pugi::xpath_node_set::const_iterator it = scan_items.begin(); it != scan_items.end(); ++it)
    {
        pugi::xpath_node node = *it;
        std::string val = node.node().attribute("contentType").value();
        if (val == "audio")
        {
            pugi::xml_node ies = node.node().append_child("InbandEventStream");

            pugi::xml_attribute siu = ies.append_attribute("schemeIdUri");
            siu.set_value("urn:mpeg:dash:event:2012");

            pugi::xml_attribute val = ies.append_attribute("value");
            val.set_value("3");
            break;
        }
    }

    //cache this mpd as our CURMPD
    doc.save_file(CURMPD);
    //save for serving to requests
    doc.save_file((const char*)incoming);
}

void inband_process(ngx_http_request_t* r, u_char* path_str) {
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

