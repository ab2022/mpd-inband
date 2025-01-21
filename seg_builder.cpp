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
    char*    mpd_name;
    char*    audio_seg_name;
} context_t;

void get_tfdt(char* seg_filename, context_t* ctx) {
    FILE*     fp;
    uint8_t   inbuf[BUFSZ];
    long      loc = -1;
    long      num;

    ctx->audio_seg_name = seg_filename;
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
    ctx->mpd_name = mpd;
    
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
    uint8_t  styp_sz_buf[4];
    uint32_t zero = 0x00;
    uint8_t  styp[4] = {'s', 't', 'y', 'p'};
    uint8_t  mp41[4] = {'m', 'p', '4', '1'};

    uint8_t  seg_buf[20];
    uint32_t styp_sz = 12; //4*3

    int2buf32(styp_sz, styp_sz_buf);

    memcpy(seg_buf, styp_sz_buf, 4);
    memcpy(&seg_buf[4], styp, 4);
    memcpy(&seg_buf[8], mp41, 4);
    memcpy(&seg_buf[12], &zero, 4);
    memcpy(&seg_buf[16], mp41, 4);

    fwrite(seg_buf, 1, 20, fp);
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
    uint32_t id = 0xDEADBEEF;
    uint16_t value = 0x3300;

    char*    scheme_id_uri = (char*)"urn:mpeg:dash:event:2012";

    if (ctx == NULL)
    {
        printf("context is NULL\n");
        exit(1);
    }

    pubtime_sz = strlen(ctx->pubtime);
    siu_sz = strlen(scheme_id_uri);
    emsg_sz = 24 +         //len of constant fields
              siu_sz + 1 + //+1 for the '\0'
              2 +          //len of 'value' field
              pubtime_sz + 1 +
              ctx->mpdsz;
    //printf("emsg sz: %u\n", emsg_sz);

    uint8_t* seg_buf = (uint8_t*)calloc(emsg_sz + 8, 1);
    if (seg_buf == NULL)
    {
        printf("write_emsg, calloc returned NULL\n");
        exit(1);
    }

    int2buf32(emsg_sz, emsg_sz_buf);
    memcpy(seg_buf, emsg_sz_buf, 4);
    memcpy(&seg_buf[4], emsg, 4);
    memcpy(&seg_buf[8], &version, 1);
    memcpy(&seg_buf[9], flags, 3);

    int2buf32(ctx->timescale, timescale_buf);
    memcpy(&seg_buf[12], timescale_buf, 4);

    int2buf64(ctx->tfdt, tfdt_buf);
    memcpy(&seg_buf[16], tfdt_buf, 8);
    memcpy(&seg_buf[24], event_dur, 4);
    memcpy(&seg_buf[28], &id, 4);
    memcpy(&seg_buf[32], scheme_id_uri, 25); //25 = strlen(scheme_id_uri)+1
    memcpy(&seg_buf[57], &value, 2);
    memcpy(&seg_buf[59], ctx->pubtime, pubtime_sz+1);

    //read in the mpd to seg_buf[59+pubtime_sz+1]
    FILE *fmpd = fopen(ctx->mpd_name, "rb");
    fread(&seg_buf[59+pubtime_sz+1], ctx->mpdsz, 1, fmpd);
    fclose(fmpd);

    //write to file
    fwrite(seg_buf, emsg_sz + 8, 1, fp);
    fflush(fp);
    free(seg_buf);
    seg_buf = NULL;
}

void concat_audio_seg(FILE* fp, context_t* ctx) {
    if (ctx->audio_seg_name == NULL)
    {
        printf("could not audio seg\n");
        exit(1);
    }

    printf("audio seg name =%s\n", ctx->audio_seg_name);

    if (fp == NULL)
    {
        printf("concat_audio_seg, fp is NULL\n");
        exit(1);
    }
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
    //concat_audio_seg(fp, ctx);
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


