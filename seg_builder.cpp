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
    {   //extract 'base media decode time'
        uint8_t   version_buf[4];
        uint8_t   tfdt_buf[8];

        loc += 4;
        memcpy(version_buf, &inbuf[loc], 4);
        loc += 4;
        memcpy(tfdt_buf, &inbuf[loc], 8);
        
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

void int2buf32(uint32_t val, uint8_t* valbuf) {
    valbuf[0] = (uint32_t)val >> 24;
    valbuf[1] = (uint32_t)val >> 16;
    valbuf[2] = (uint32_t)val >> 8;
    valbuf[3] = (uint32_t)val;
}

void int2buf64(uint64_t val, uint8_t* valbuf) {
    valbuf[0] = (uint64_t)val >> 56;
    valbuf[1] = (uint64_t)val >> 48;
    valbuf[2] = (uint64_t)val >> 40;
    valbuf[3] = (uint64_t)val >> 32;
    valbuf[4] = (uint64_t)val >> 24;
    valbuf[5] = (uint64_t)val >> 16;
    valbuf[6] = (uint64_t)val >> 8;
    valbuf[7] = (uint64_t)val;
}

void write_styp(FILE* fp) {
    uint32_t styp_sz;
    uint8_t  styp_sz_buf[4];
    uint32_t zero = 0x00;
    uint8_t  styp[4] = {'s', 't', 'y', 'p'};
    uint8_t  mp41[4] = {'m', 'p', '4', '1'};

    styp_sz = 12; //4*3
    int2buf32(styp_sz, styp_sz_buf);

    fwrite(styp_sz_buf, 4, 1, fp);
    fwrite(styp, 1, 4, fp);
    fwrite(mp41, 1, 4, fp);
    fwrite(&zero, 4, 1, fp);
    fwrite(mp41, 1, 4, fp);
    fflush(fp);
}

void write_emsg(FILE* fp, context_t* ctx) {
    uint32_t emsg_sz;
    uint8_t  emsg_sz_buf[4];
    uint8_t  emsg[4] = {'e', 'm', 's', 'g'};
    uint32_t pubtime_sz;
    uint32_t siu_sz;
    uint8_t  version = 0x01;
    uint8_t  flags[3] = {0x00};
    uint8_t  timescale_buf[4];
    uint8_t  tfdt_buf[8];
    uint8_t  event_dur[4] = {0xFF, 0xFF, 0xFF, 0xFF};

    char*    scheme_id_uri = (char*)"urn:mpeg:dash:event:2012";

    if (ctx == NULL)
    {
        printf("context is NULL\n");
        exit(1);
    }

    pubtime_sz = strlen(ctx->pubtime);
    siu_sz = strlen(scheme_id_uri);
    emsg_sz = 24 + //len of constant fields
              siu_sz +
              pubtime_sz +
              2 + //len of 'value' field
              ctx->mpdsz;

    printf("emsg sz: %u\n", emsg_sz);
    int2buf32(emsg_sz, emsg_sz_buf);

    fwrite(emsg_sz_buf, 4, 1, fp);
    fwrite(emsg, 1, 4, fp);
    fwrite(&version, 1, 1, fp);
    fwrite(flags, sizeof(flags), 1, fp);

    int2buf32(ctx->timescale, timescale_buf);
    fwrite(timescale_buf, 4, 1, fp);

    int2buf64(ctx->tfdt, tfdt_buf);
    fwrite(tfdt_buf, 8, 1, fp);

    fwrite(event_dur, sizeof(event_dur), 1, fp);
    fflush(fp);
}

void write_seg(char* seg_filename, context_t* ctx) {

    FILE* fp;
    fp = fopen(seg_filename, "wb");
    if (fp == NULL)
    {
        printf("could not open file %s for writing \n", seg_filename);
        exit(1);
    }

    write_styp(fp);
    write_emsg(fp, ctx);
    fclose(fp);
}

int main(void) {
    context_t seg_ctx;

    get_tfdt((char*)"/home/ab/work/vid/audio-0-128000-904577953.mp4a", &seg_ctx);
    printf("tfdt   : %lu\n", seg_ctx.tfdt);
    printf("version: %u\n", seg_ctx.version);

    get_timescale((char*)"manifest.mpd", &seg_ctx);
    printf("timescale: %u\n", seg_ctx.timescale);
    printf("mpd size : %u\n", seg_ctx.mpdsz);
    printf("pub time : %s\n", seg_ctx.pubtime);
    printf("\n");

    write_seg((char*)"new_seg.mp4a", &seg_ctx);

    //write <InbandEventStream /> to mpd in audio AdaptationSet

    return 0;
}

#ifdef __cplusplus
}
#endif


