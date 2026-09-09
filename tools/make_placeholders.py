#!/usr/bin/env python3
"""Generate a plain text-banner marquee for any enabled game that has no art.

The repository ships no marquee artwork - game logos are copyrighted. So for a
turnkey build, every enabled game that lacks a real marquees/<rom>.png gets a
generated placeholder here: its title, white on black, fitted to the box. Drop a
real PNG in (or run fetch_marquees.py) and it takes precedence - placeholders are
only made for what is missing, and only in marquees/_placeholder/ so they never
shadow or get mistaken for real art.
"""
import os, sys
from PIL import Image, ImageDraw, ImageFont
import configure

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ART  = os.path.join(ROOT, 'marquees')
GEN  = os.path.join(ART, '_placeholder')
BOX_W, BOX_H = 208, 104

def a_font(size):
    for p in ('/System/Library/Fonts/SFNSMono.ttf',
              '/System/Library/Fonts/Menlo.ttc',
              '/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf'):
        if os.path.exists(p):
            try: return ImageFont.truetype(p, size)
            except Exception: pass
    return ImageFont.load_default()

def make(title, path):
    im = Image.new('RGB', (BOX_W, BOX_H), (0, 0, 0))
    d = ImageDraw.Draw(im)
    words = title.upper().split()
    # one or two lines, as large as fits
    for lines in ([title.upper()], None):
        if lines is None:                      # split roughly in half by words
            mid = (len(words) + 1) // 2
            lines = [' '.join(words[:mid]), ' '.join(words[mid:])] if len(words) > 1 else [title.upper()]
        for size in range(40, 8, -2):
            f = a_font(size)
            wh = [d.textbbox((0, 0), ln, font=f)[2:] for ln in lines]
            tw = max(w for w, h in wh); th = sum(h for w, h in wh) + (len(lines) - 1) * 4
            if tw <= BOX_W - 12 and th <= BOX_H - 12:
                y = (BOX_H - th) // 2
                for ln, (w, h) in zip(lines, wh):
                    d.text(((BOX_W - w) // 2, y), ln, font=f, fill=(235, 235, 245)); y += h + 4
                im.save(path); return
    im.save(path)

def main():
    os.makedirs(GEN, exist_ok=True)
    cfg = configure.load()
    _, rows, _ = configure.resolve(cfg)
    made = 0
    for r in rows:
        rom, title = r['rom'], r['title']
        if os.path.exists(os.path.join(ART, rom + '.png')):
            continue                            # a real (or hand-made) marquee wins
        make(title, os.path.join(GEN, rom + '.png'))
        made += 1
    print(f'  {made} text placeholder(s) generated for games without art')

if __name__ == '__main__':
    main()
