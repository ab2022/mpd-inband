
## MPD Inband

MPD Inband is an NGINX module that concatenates MPDs and audio segments.

The goal of MPD Inband is to include MPDs in audio segments and thereby reduce HTTP request overhead. Once built and installed, MPD Inband processes every file ending in "mpd" or ".mp4a" that is uploaded to NGINX using HTTP POST or PUT.

### Building

MPD Inband compiles with g++ 13.3.0 on Ubuntu 24.04 with NGINX 1.24.0. No other compilers or versions have been tried. Pugixml is a dependency and is used for xml processing. The pugixml files are not included.

The build process is the same as compiling [morpheus](https://www.github.com/ab2022/morpheus)

### Seg_builder

Seg_builder is a standalone tool that can be run to test and verify the concatenation processing.


