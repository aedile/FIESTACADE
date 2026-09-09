#!/usr/bin/env python3
"""Fetch marquee art for the games you have, from a third-party archive.

This project hosts no game art. This pulls it, on demand, from the libretro
thumbnails archive (the community's standard source, organised by MAME name),
the same way Galagino points you at a search engine for ROMs - we distribute
nothing, we point at where it lives, and running this is your call.

  ./fiestacade art            fetch for every enabled game that lacks a PNG
  ./fiestacade art <rom>...   fetch just these

Override the source with FIESTACADE_ART_BASE (a URL with {name} for the MAME
title, url-encoded). Whatever comes down is fitted to marquees/<rom>.png.
"""
import os, sys, urllib.parse, urllib.request, io
from PIL import Image
import configure

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ART  = os.path.join(ROOT, 'marquees')
BOX_W, BOX_H = 208, 104

# libretro stores MAME marquees under the "MAME" system, Named_Titles, .png.
# Their file names are the full game title, not the MAME short name, so we map a
# few; anything not mapped is tried as-is and reported if it 404s.
BASE = os.environ.get('FIESTACADE_ART_BASE',
    'https://raw.githubusercontent.com/libretro-thumbnails/MAME/master/Named_Titles/{name}.png')

TITLES = {
    'rbtapper': 'Tapper (Root Beer)', 'tapper': 'Tapper (Budweiser)',
    'joust': 'Joust (White Label)', 'mpatrol': 'Moon Patrol',
    'invaders': 'Space Invaders', 'galaxian': 'Galaxian (Namco)',
    'timeplt': 'Time Pilot', 'llander': 'Lunar Lander (rev 2)',
    'btime': 'Burger Time (Data East set 1)', 'mrdo': "Mr. Do!",
}

def fetch(rom):
    name = TITLES.get(rom, rom)
    url = BASE.format(name=urllib.parse.quote(name))
    try:
        with urllib.request.urlopen(url, timeout=20) as r:
            data = r.read()
    except Exception as e:
        print(f'  {rom:<11} could not fetch ({e}); name tried: "{name}"')
        print(f'             set the title in tools/fetch_marquees.py or drop marquees/{rom}.png yourself')
        return False
    im = Image.open(io.BytesIO(data)).convert('RGB')
    s = min(BOX_W / im.width, BOX_H / im.height)
    im = im.resize((max(1, round(im.width * s)), max(1, round(im.height * s))), Image.LANCZOS)
    im.save(os.path.join(ART, rom + '.png'))
    print(f'  {rom:<11} fetched -> marquees/{rom}.png')
    return True

def main():
    os.makedirs(ART, exist_ok=True)
    want = sys.argv[1:]
    if not want:
        cfg = configure.load(); _, rows, _ = configure.resolve(cfg)
        want = [r['rom'] for r in rows if not os.path.exists(os.path.join(ART, r['rom'] + '.png'))]
        if not want:
            print('  every enabled game already has a marquee'); return
    for rom in want:
        fetch(rom)

if __name__ == '__main__':
    main()
