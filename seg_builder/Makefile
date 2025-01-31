#Makefile for seg_builder

CC = g++
CFLAGS = -Wall -W -g
SRCS = seg_builder.cpp
OBJS = $(SRCS:.cpp=.o)

all: seg_builder

seg_builder: $(OBJS)
	$(CC) $(CFLAGS) $(SRCS) -o $@

clean:
	rm -f *.o *.mp4a seg_builder cur.mpd core
