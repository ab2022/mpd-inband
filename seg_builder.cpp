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

void get_tfdt(char* seg_filename, uint64_t* tfdt_val) {
    FILE*     fp;
    uint8_t   inbuf[BUFSZ];
    long      loc = -1;
    long      num;

    fp = fopen(seg_filename, "rb");

    num = fread(inbuf, 1, BUFSZ, fp);
    if (num < BUFSZ)
    {
        printf("Read less than BUFSZ, num read: %ld\n", num);
        return;
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
        return;
    }
    else
    {
        //extract 'base media decode time'
        printf("loc: %ld\n", loc);

        uint8_t   version_buf[4];
        uint8_t   tfdt_buf[8];
        uint32_t  version;

        loc += 4;
        memcpy(&version_buf[0], &inbuf[loc], 4);

        loc += 4;
        memcpy(&tfdt_buf[0], &inbuf[loc], 8);
        
        //for (i = 0; i < 4; i++) printf("%d\n", (uint8_t)(version_buf[i]));

        version = ((uint32_t)(version_buf[3]) << 24) |
                  ((uint32_t)(version_buf[2]) << 16) |
                  ((uint32_t)(version_buf[1]) <<  8) |
                  ((uint32_t)(version_buf[0])      );

        //for (i = 0; i < 8; i++) printf("%x\n", (uint8_t)(tfdt_buf[i]));

        *tfdt_val = ((uint64_t)(tfdt_buf[0]) << 56) |
               ((uint64_t)(tfdt_buf[1]) << 48) |
               ((uint64_t)(tfdt_buf[2]) << 40) |
               ((uint64_t)(tfdt_buf[3]) << 32) |
               ((uint64_t)(tfdt_buf[4]) << 24) |
               ((uint64_t)(tfdt_buf[5]) << 16) |
               ((uint64_t)(tfdt_buf[6]) <<  8) |
               ((uint64_t)(tfdt_buf[7])      );

        printf("version: %u\n", version);
    }
}

void get_timescale(char* mpd, uint32_t* timescale, uint32_t* file_size) {
    *timescale = 0;
    
    //get the 1) audio timescale and 2) size in bytes
    pugi::xml_document doc;
    if (!doc.load_file(mpd))
    {
        printf("\nCould not open MPD\n");
        return;
    }

    pugi::xml_node period = doc.child("MPD").child("Period");

    for (pugi::xml_node adaptset = period.first_child(); adaptset; adaptset = adaptset.next_sibling())
        if (strncmp(adaptset.attribute("contentType").value(), "audio", 5) == 0)
            *timescale = strtoul(adaptset.child("SegmentTemplate").attribute("timescale").value(), NULL, 10);
            //std::cout << "audio timescale: " << adaptset.child("SegmentTemplate").attribute("timescale").value() << std::endl;

    if (*timescale == 0)
    {
        printf("\nCould not find audio timescale\n");
        return;
    }

    *file_size = std::filesystem::file_size(mpd);
}

int main(void) {
    uint64_t tfdt;
    uint32_t timescale;
    uint32_t sz;

    get_tfdt((char*)"/home/ab/zz_segs/audio-0-128000-904577953.mp4a", &tfdt);
    printf("tfdt: %lu\n", tfdt);

    get_timescale((char*)"manifest.mpd", &timescale, &sz);
    printf("timescale: %u\n", timescale);
    printf("size     : %u\n", sz);

   //write seg

    return 0;
}

#ifdef __cplusplus
}
#endif


