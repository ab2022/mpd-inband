// Copyright (C) 2025 Comcast

#include <stdio.h>
#include <stdint.h>

#include <iostream>
#include <filesystem>

#include "pugixml.hpp"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

using namespace std;

#define BUFSZ 256 

typedef struct context_s {
    uint64_t tfdt;
    uint32_t timescale;
    uint32_t mpdsz;
    uint8_t  version;
    char     pubtime[32];
} context_t;

void get_tfdt(char* seg_filename, context_t* ctx) {
    FILE*     fp;
    uint8_t   inbuf[BUFSZ];
    long      loc = -1;
    long      num;

    fp = fopen(seg_filename, "rb");
    if (fp == NULL)
    {
        printf("could not open file %s\n", seg_filename);
        exit(1);
    }

    num = fread(inbuf, 1, BUFSZ, fp);
    if (num < BUFSZ)
    {
        printf("Read less than BUFSZ, num read: %ld\n", num);
        exit(1);
    }

    for (int i = 0; i < BUFSZ - 4; i++)
    {
        if (inbuf[i] == 't')
            if (memcmp("fdt", &inbuf[i+1], 3) == 0)
            {
                loc = i;
                break;
            }
    }

    if (loc < 0)
    {
        printf("did not find tfdt, exiting\n");
        exit(1);
    }
    else
    {
        //extract 'base media decode time'
        printf("loc: %ld\n", loc);

        uint8_t   version_buf[4];
        uint8_t   tfdt_buf[8];

        loc += 4;
        memcpy(&version_buf[0], &inbuf[loc], 4);
        loc += 4;
        memcpy(&tfdt_buf[0], &inbuf[loc], 8);
        
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
            ((uint64_t)(tfdt_buf[7])      );
    }
}

void get_timescale(char* mpd, context_t* ctx) {
    ctx->timescale = 0;
    
    //get the 1) publish time 2) audio timescale and 3) size in bytes
    pugi::xml_document doc;
    if (!doc.load_file(mpd))
    {
        printf("\nCould not open MPD\n");
        exit(1);
    }

    pugi::xml_node mpdxml = doc.child("MPD");
    memcpy(ctx->pubtime, mpdxml.attribute("publishTime").value(), sizeof(ctx->pubtime));

    pugi::xml_node period = mpdxml.child("Period");
    for (pugi::xml_node adaptset = period.first_child(); adaptset; adaptset = adaptset.next_sibling())
        if (strncmp(adaptset.attribute("contentType").value(), "audio", 5) == 0)
            ctx->timescale = strtoul(adaptset.child("SegmentTemplate").attribute("timescale").value(), NULL, 10);

    if (ctx->timescale == 0)
    {
        printf("\nCould not find audio timescale\n");
        exit(1);
    }

    ctx->mpdsz = std::filesystem::file_size(mpd);
}

#if 0
void write_styp(FILE* fp, uint32_t seg_size) {
}

void write_seg(char* fname, uint64_t tfdt,  uint32_t timescale, uint32_t mpd_size) {
    FILE* fp;
    fp = fopen(fname, "wb");

    scheme_id_uri: "urn:mpeg:dash:event:2012\0"

    //compute total_seg_size
    write_styp(fp, total_seg_size);

    //write emsg

    //concat to existing seg

    //write <InbandEventStream /> to mpd in audio AdaptationSet

    return;
}
#endif

int main(void) {
    context_t seg_ctx;

    get_tfdt((char*)"/home/ab/work/vid/audio-0-128000-904577953.mp4a", &seg_ctx);
    printf("tfdt   : %lu\n", seg_ctx.tfdt);
    printf("version: %u\n", seg_ctx.version);

    get_timescale((char*)"manifest.mpd", &seg_ctx);
    printf("timescale: %u\n", seg_ctx.timescale);
    printf("mpd size : %u\n", seg_ctx.mpdsz);
    printf("pub time : %s\n", seg_ctx.pubtime);

    //write_seg((char*)"new_seg.mp4a", tfdt, timescale, sz);

    return 0;
}

#ifdef __cplusplus
}
#endif


