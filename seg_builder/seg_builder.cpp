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

#define SCANBUF 256

typedef struct context_s {
    uint64_t tfdt;
    uint32_t timescale;
    uint32_t mpdsz;
    uint8_t  version;
    char     pubtime[32];
    char*    mpd_name;
    char*    audio_seg_name;
    uint8_t* audio_seg_contents;
    int      audio_seg_sz;
} context_t;

void get_tfdt(char* seg_filename, context_t* ctx) {
    FILE*     fp;
    long      loc = -1;
    long      num;

    ctx->audio_seg_name = seg_filename;
    ctx->audio_seg_contents = NULL;

    fp = fopen(seg_filename, "rb");
    if (fp == NULL)
    {
        printf("could not open audio seg file %s\n", seg_filename);
        exit(1);
    }

    fseek(fp, 0L, SEEK_END);
    ctx->audio_seg_sz = ftell(fp);
    fseek(fp,0L,SEEK_SET);

    ctx->audio_seg_contents = (uint8_t*)calloc(ctx->audio_seg_sz, 1);
    if (ctx->audio_seg_contents == NULL)
    {
        printf("get_tfdt, calloc returned NULL\n");
        exit(1);
    }

    num = fread(ctx->audio_seg_contents, 1, ctx->audio_seg_sz, fp);
    if (num < ctx->audio_seg_sz)
    {
        printf("Read less than size of file, num read: %ld\n", num);
        exit(1);
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
        printf("did not find tfdt, exiting\n");
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
            ((uint64_t)(tfdt_buf[7])      );
    }
}

void get_timescale(char* mpd, context_t* ctx) {
    ctx->timescale = 0;
    ctx->mpd_name = mpd;
    
    //get the 1) publish time 2) audio timescale and 3) mpd size in bytes
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
    printf(" emsg sz : %u\n\n", emsg_sz);

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
    if (ctx->audio_seg_contents == NULL)
    {
        printf("could not get audio seg contents\n");
        exit(1);
    }

    printf("audio seg name = %s ... size = %d\n", ctx->audio_seg_name, ctx->audio_seg_sz);

    fwrite(ctx->audio_seg_contents, ctx->audio_seg_sz, 1, fp);
    fflush(fp);
    free(ctx->audio_seg_contents);
    ctx->audio_seg_contents = NULL;
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
    concat_audio_seg(fp, ctx);
    fclose(fp);
}

void write_ies(char* incoming) {

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

    //save mpd as cur.mpd
    doc.save_file((const char*)"cur.mpd");
}

int main(void) {
    context_t seg_ctx;

    get_tfdt((char*)"/home/ab/work/vid/audio-0-128000-904577953.mp4a", &seg_ctx);
    printf("tfdt   : %lu\n", seg_ctx.tfdt);
    printf("version: %u\n", seg_ctx.version);

    get_timescale((char*)"sample_manifest.mpd", &seg_ctx);
    printf("timescale: %u\n", seg_ctx.timescale);
    printf("mpd size : %u\n", seg_ctx.mpdsz);
    printf("pub time : %s\n", seg_ctx.pubtime);

    write_seg((char*)"new_seg.mp4a", &seg_ctx);

    //write <InbandEventStream /> to mpd in audio AdaptationSet
    write_ies((char*)"sample_manifest.mpd");

    return 0;
}

#ifdef __cplusplus
}
#endif


