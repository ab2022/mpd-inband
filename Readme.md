
## MPD Inband

MPD Inband is an NGINX module that concatenates MPDs and audio segments.

The goal of MPD Inband is to include MPDs in audio segments and thereby reduce HTTP request overhead. Once built and installed, MPD Inband processes every file ending in "mpd" or ".mp4a" that is uploaded to NGINX using HTTP POST or PUT.

### Building

MPD Inband compiles with g++ 13.3.0 on Ubuntu 24.04 with NGINX 1.24.0. No other compilers or versions have been tried. Pugixml is a dependency and is used for xml processing. The pugixml files are not included.

The build process is the same as compiling [morpheus](https://www.github.com/ab2022/morpheus)

### segbuilder

segbuilder is a standalone tool that can be run to test and verify the concatenation processing. It can be compiled with `make`.

### Format

Audio segments are written in the following format:

```
0       7 8     15 16    23 24     31
+--------+--------+--------+--------+
|            stype size             |
+--------+--------+--------+--------+
|   s        t         y       p    | #<--'styp' (same defintion as ftyp)
+--------+--------+--------+--------+
|   i        s         o       9    | #<--'iso9'
+--------+--------+--------+--------+
|           minor version           | #<--0
+--------+--------+--------+--------+
|   d        a         s       h    | #<--'dash'
+--------+--------+--------+--------+
|             emsg size             |
+--------+--------+--------+--------+
|   e        m         s       g    |
+--------+--------+--------+--------+
|   ver  |           flags          | #<--ver = 1, flags = 0x00
+--------+--------+--------+--------+
|            timescale              | #<--from SegmentTemplate in audio AdaptSet
+--------+--------+--------+--------+
|         presentation_time         | #<--8 bytes, from tfdt in segment
|                                   |
+--------+--------+--------+--------+
|          event_duration           | #<--0xFFFFFFFF
+--------+--------+--------+--------+
|                id                 | #<--random number
+--------+--------+--------+--------+
|   u         r        n       :    | #<--scheme_id_uri copied from DASH spec
|   m         p        e       g    |
|   :         d        a       s    |
|   h         :        e       v    |
|   e         n        t       :    |
|   2         0        1       2    |
+        +--------+--------+--------+
|  \0    |      value      | msg    | #<--value = {'3', '\0'}
+--------+--------+--------+--------+
|         message_data con't        | #<--bytes of xml equal to
|                ...                | #<--emsg size - other fields
+--------+--------+--------+--------+
|             moof size             |
+--------+--------+--------+--------+
|   m        o         o       f    |
+--------+--------+--------+--------+
|                ...                | #<--rest of the audio segment
+--------+--------+--------+--------+
```

