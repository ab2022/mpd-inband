/*
* Copyright (C) ab 1/29/25
*/

#include "pugixml.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#define CURMPD (const char*)"/dev/shm/cur.mpd"
#define SCANBUF 256


typedef struct context_s {
    uint64_t  tfdt;
    uint32_t  timescale;
    size_t    mpdsz;
    uint8_t   version;
    char      pubtime[32];
    const char*  mpd_name;
    const char*  audio_seg_name;
    uint8_t*     audio_seg_contents;
    size_t       audio_seg_sz;
} context_t;


void get_timescale(ngx_http_request_t* r, context_t* ctx) {
    char*  pt = NULL;
    size_t pt_sz;
    ctx->timescale = 0;

    //get the 1) publish time 2) audio timescale
    pugi::xml_document doc;
    if (!doc.load_file(CURMPD))
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "_INBAND_ could not open mpd file \"%s\" for reading\n", CURMPD);
        return;
    }

    pugi::xml_node mpdxml = doc.child("MPD");
    pt = (char*)mpdxml.attribute("publishTime").value();
    pt_sz = strlen(pt);
    if (pt_sz == 0)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "_INBAND_ publishTime string not found\n");
        return;
    }

    memcpy(ctx->pubtime, pt, pt_sz);
    ctx->pubtime[pt_sz] = '\0';

    pugi::xml_node period = mpdxml.child("Period");
    for (pugi::xml_node adaptset = period.first_child(); adaptset; adaptset = adaptset.next_sibling())
        if (strncmp(adaptset.attribute("contentType").value(), "audio", 5) == 0)
        {
            ctx->timescale = strtoul(adaptset.child("SegmentTemplate").attribute("timescale").value(), NULL, 10);
            break;
        }

    if (ctx->timescale == 0)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "_INBAND_ could not find audio timescale\n");
        return;
    }
}

void get_tfdt(ngx_http_request_t* r, context_t* ctx) {
    FILE*     fp;
    long      loc = -1;
    size_t    num;

    fp = fopen(ctx->audio_seg_name, "rb");
    if (fp == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "_INBAND_ could not open audio seg file \"%s\"\n");
        return;
    }

    ctx->audio_seg_contents = (uint8_t*)calloc(ctx->audio_seg_sz, 1);
    if (ctx->audio_seg_contents == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "_INBAND_ get_tfdt, calloc returned NULL\n");
        return;
    }

    num = fread(ctx->audio_seg_contents, 1, ctx->audio_seg_sz, fp);
    if (num < ctx->audio_seg_sz)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "_INBAND_ num less than file sz num: %d sz: %d\n",
                      num, ctx->audio_seg_sz);
        return;
    }

    fclose(fp);

    for (int i = 0; i < SCANBUF; i++)
    {
        if (ctx->audio_seg_contents[i] == 't')
            if (memcmp("fdt", &(ctx->audio_seg_contents[i+1]), 3) == 0)
            {
                loc = i;
                break;
            }
    }

    if (loc < 0)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "_INBAND_ did not find tfdf, exiting\n");
        exit(1);
    }
    else
    {   //extract 'base media decode time'
        uint8_t   version_buf[4];
        uint8_t   tfdt_buf[8];

        loc += 4;
        memcpy(version_buf, &ctx->audio_seg_contents[loc], 4);
        loc += 4;
        memcpy(tfdt_buf, &ctx->audio_seg_contents[loc], 8);

        ctx->version = ((uint32_t)(version_buf[3]) << 24) |
            ((uint32_t)(version_buf[2]) << 16) |
            ((uint32_t)(version_buf[1]) <<  8) |
            ((uint32_t)(version_buf[0])      );

        ctx->tfdt = ((uint64_t)(tfdt_buf[0]) << 56) |
            ((uint64_t)(tfdt_buf[1]) << 48) |
            ((uint64_t)(tfdt_buf[2]) << 40) |
            ((uint64_t)(tfdt_buf[3]) << 32) |
            ((uint64_t)(tfdt_buf[4]) << 24) |
            ((uint64_t)(tfdt_buf[5]) << 16) |
            ((uint64_t)(tfdt_buf[6]) <<  8) |
            ((uint64_t)(tfdt_buf[7]) );
    }
}

void process_audio(ngx_http_request_t* r) {
    context_t ctx;

    /* get the audio seg temp file name and size */
    ctx.audio_seg_name = (const char*)r->request_body->temp_file->file.name.data;
	ctx.audio_seg_sz = r->request_body->temp_file->file.offset;

    get_timescale(r, &ctx);
    get_tfdt(r, &ctx);

    printf("audio seg: %s\n", ctx.audio_seg_name);
    printf("audseg sz: %lu\n", ctx.audio_seg_sz);
    printf("timescale: %u\n", ctx.timescale);
    printf(" pub time: %s\n", ctx.pubtime);
    printf("     tfdt: %lu\n", ctx.tfdt);
    printf("  version: %u\n", ctx.version);

#if 0
    /*
    FILE* mpd_fp;
    mpd_fp = fopen(CURMPD, "rb");
    if (mpd_fp == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "_INBAND_ could not open mpd file \"%s\" for reading\n", CURMPD);
        return;
    }
    fclose(mpd_fp);
    */


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
#endif
}

void process_mpd(ngx_http_request_t* r) {

    const char* incoming = (const char*)r->request_body->temp_file->file.name.data;
    pugi::xml_document doc;
    if (!doc.load_file(incoming))
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "_INBAND_ could not open incoming mpd file \"%s\"\n", incoming);
        return;
    }

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

