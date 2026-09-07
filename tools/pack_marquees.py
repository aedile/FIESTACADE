#!/usr/bin/env python3
"""
Fit marquee artwork to the panel and pack it into the mqart blob.

  python3 tools/pack_marquees.py            # marquees/*.png -> lcd/marquees.bin

Source art is any resolution; each image is scaled to fit inside 208x104 with its
aspect ratio preserved, then written as RGB565 in BIG-ENDIAN byte order so the
firmware can hand it straight to display_write_preswapped() with no per-pixel swap.

Blob layout:
    "MQ02" | u16 count | u16 entry_size | entry[count] | pixels (4-byte aligned)
    entry  = 12s rom | 24s title | u16 w | u16 h | u32 offset | u32 length
"""
import os, struct, sys
from PIL import Image
import configure                      # single source of truth for what is in the build

ROOT  = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC   = os.path.join(ROOT, 'marquees')
PREV  = os.path.join(ROOT, 'lcd', 'preview')
OUT   = os.path.join(ROOT, 'lcd', 'marquees.bin')
BOX_W, BOX_H = 208, 104
ENTRY = struct.Struct('<12s24sHHII')

def enabled_games():
    """The games in this build, straight from games.toml + roms/ - so the menu can
    never disagree with the partition table about what exists."""
    cfg = configure.load()
    _, rows, _ = configure.resolve(cfg)
    return [(r['rom'], r['title']) for r in rows]


def rgb565_be(r, g, b):
    v = ((round(r * 31 / 255) & 0x1F) << 11) | \
        ((round(g * 63 / 255) & 0x3F) << 5)  | \
         (round(b * 31 / 255) & 0x1F)
    return struct.pack('>H', v)

def main():
    os.makedirs(PREV, exist_ok=True)
    os.makedirs(os.path.dirname(OUT), exist_ok=True)

    games = enabled_games()
    if not games:
        sys.exit('no games in this build - put an approved ROM zip in roms/')
    titles = dict(games)
    roms = sorted(titles)

    hdr_sz   = 8 + ENTRY.size * len(roms)
    data_off = (hdr_sz + 3) & ~3

    entries, blobs, cur = [], [], data_off
    for rom in roms:
        im = Image.open(os.path.join(SRC, rom + '.png')).convert('RGB')
        s  = min(BOX_W / im.width, BOX_H / im.height)
        w, h = max(1, round(im.width * s)), max(1, round(im.height * s))
        im = im.resize((w, h), Image.LANCZOS)
        im.save(os.path.join(PREV, rom + '.png'))

        px = b''.join(rgb565_be(*p) for p in im.get_flattened_data())
        entries.append(ENTRY.pack(rom.encode(), titles[rom].encode(), w, h, cur, len(px)))
        blobs.append(px)
        cur += len(px)
        print(f'  {rom:<11}{titles[rom]:<20}{w:>4}x{h:<4}{len(px)/1024:>7.1f}K')

    with open(OUT, 'wb') as f:
        f.write(b'MQ02' + struct.pack('<HH', len(roms), ENTRY.size))
        f.write(b''.join(entries))
        f.write(b'\0' * (data_off - hdr_sz))
        f.write(b''.join(blobs))

    size = os.path.getsize(OUT)
    print(f'\n{len(roms)} marquees -> {OUT} ({size/1024:.1f} KB)')
    print(f'mqart partition must be at least 0x{(size + 0xFFFF) & ~0xFFFF:X}')

if __name__ == '__main__':
    main()
