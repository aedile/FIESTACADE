# Third-party code

## libhelix-mp3 (RealNetworks Public Source License)

`components/helix_mp3` is RealNetworks' fixed-point MP3 decoder, copyright
1995-2004 RealNetworks, Inc., under the RPSL. It decodes the clip's audio track.
The licence text is in that component's directory.

## TJpgDec

The JPEG frames are decoded by TJpgDec (TinyJPEG) by ChaN, which lives in the
ESP32-C6's mask ROM and is reached through ESP-IDF's `esp32c6/rom/tjpgd.h`.
Nothing of it is in this repository.

## The player

`main/egg.cpp` was written for the TRENCHRUNNER medal as an easter egg and
lifted out here as a project of its own. It is covered by the project licence.

## The clip

No video is included and none ever will be. Street Fighter II is © Capcom.
Whatever you pack with `tools/pack_media.py` is your own responsibility.
