// Copyright (C) 2025 Comcast
// to compile: gcc -g -Wall seg_builder.c -o seg_builder.c

#include <stdio.h>
#include <stdint.h>
#define _GNU_SOURCE
#include <string.h>

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

int main(void) {
    uint64_t tfdt;

    get_tfdt("audio-0-128000-904577953.mp4a", &tfdt);
    printf("tfdt: %lu\n", tfdt);

    //open MPD and get the needed items
    
    //write seg

    return 0;
}

