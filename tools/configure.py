#!/usr/bin/env python3
"""
Turn games.toml plus whatever is in roms/ into a partition table and a build manifest.

    python3 tools/configure.py            # report and generate
    python3 tools/configure.py --check    # report only, change nothing

A game is in the build when its ROM zip is present in roms/, unless games.toml
overrides that with `enabled`. Disabling a game FREES ITS SLOT - the partition
table is generated, not hand-maintained, so an eight-game build gets eight slots
and the rest of the flash stays empty.

Needs only the standard library.
"""
import argparse, json, os, sys, tomllib

ROOT   = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CONF   = os.path.join(ROOT, 'games.toml')
ROMS   = os.path.join(ROOT, 'roms')
ART    = os.path.join(ROOT, 'marquees')
GAMES  = os.path.join(ROOT, 'games')
BLOB   = os.path.join(ROOT, 'lcd', 'marquees.bin')
PARTS  = os.path.join(ROOT, 'partitions.csv')
MANIF  = os.path.join(ROOT, 'build', 'manifest.json')

OTA_MAX  = 16          # ESP-IDF hard cap: ota_0 .. ota_15
K        = 1024
ALIGN    = 0x10000     # app partitions must start on a 64 KB boundary

# fixed head of the table
HEAD = [
    ('nvs',      'data', 'nvs',     0x9000,  0x6000),
    ('otadata',  'data', 'ota',     0xF000,  0x2000),
    ('phy_init', 'data', 'phy',     0x11000, 0x1000),
]

def die(msg):
    print(f'\n  error: {msg}\n', file=sys.stderr)
    sys.exit(1)

def load():
    if not os.path.exists(CONF):
        die('games.toml not found')
    with open(CONF, 'rb') as f:
        return tomllib.load(f)

def resolve(cfg):
    """Decide what is in the build, and why."""
    b = cfg.get('build', {})
    default_slot = b.get('default_slot_kb', 768)
    rows, skipped = [], []

    for g in cfg.get('game', []):
        rom   = g['rom']
        zip_p = os.path.join(ROMS, rom + '.zip')
        art_p = os.path.join(ART,  rom + '.png')
        has_rom, has_art = os.path.exists(zip_p), os.path.exists(art_p)

        # A game whose payload is a data file rather than a ROM - a video clip - is switched
        # on by that file existing under its project, the same way a ROM zip switches on an
        # emulated game: put the file there and it is in the build.
        data_file = g.get('data_file')
        data_p = os.path.join(GAMES, g.get('project') or '', data_file) if data_file else None
        has_payload = os.path.exists(data_p) if data_p else has_rom
        payload_desc = (f"{g.get('project')}/{data_file}" if data_p else f'roms/{rom}.zip')

        forced = g.get('enabled')
        on = has_payload if forced is None else bool(forced)
        why = ('forced on' if forced is True else
               'disabled in games.toml' if forced is False else
               payload_desc if has_payload else ('no ROM' if not data_p else f'no {data_file}'))

        if forced is True and not has_payload:
            die(f'{rom} is forced on in games.toml but {payload_desc} is missing')
        if on and not has_art:
            die(f'{rom} is in the build but marquees/{rom}.png is missing')

        rec = dict(rom=rom, title=g.get('title', rom), project=g.get('project'),
                   binary=g.get('binary'),          # optional: where this game's .bin is, under the project
                   data_kb=g.get('data_kb', 0),     # optional: a data partition of its own, e.g. a video clip
                   data_file=g.get('data_file'),    # optional: the file to flash into it, under the project
                   slot_kb=g.get('slot_kb', default_slot), why=why, has_art=has_art)
        (rows if on else skipped).append(rec)

    return b, rows, skipped

def mqart_kb(n):
    """Size the artwork partition: exact if the blob exists, else estimated."""
    if os.path.exists(BLOB):
        need = os.path.getsize(BLOB)
        exact = True
    else:
        need, exact = n * 30 * K, False           # ~30 KB per marquee
    kb = ((need + need // 4 + 0xFFFF) // 0x10000) * 64   # +25% headroom, 64 KB granules
    return max(kb, 64), exact

def build_table(b, rows, art_kb):
    parts = list(HEAD)
    off = 0x20000
    parts.append(('launcher', 'app', 'factory', off, b.get('launcher_kb', 512) * K))
    off += b.get('launcher_kb', 512) * K
    parts.append(('mqart', 'data', '0x40', off, art_kb * K))
    off += art_kb * K
    off = (off + ALIGN - 1) & ~(ALIGN - 1)

    for i, r in enumerate(rows):
        parts.append((r['rom'], 'app', f'ota_{i}', off, r['slot_kb'] * K))
        r['offset'] = off
        off += r['slot_kb'] * K

    # A game may ask for a data partition of its own - a video clip, say. It is laid out
    # after the app slots and labelled "media", which is the label the player looks for,
    # so only one game in a build may have one.
    data_games = [r for r in rows if r.get('data_kb')]
    if len(data_games) > 1:
        die('only one game per build may have data_kb: ' + ', '.join(r['rom'] for r in data_games))
    for r in data_games:
        off = (off + ALIGN - 1) & ~(ALIGN - 1)
        parts.append(('media', 'data', '0x40', off, r['data_kb'] * K))
        r['data_offset'] = off
        off += r['data_kb'] * K
    return parts, off

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--check', action='store_true', help='report only, write nothing')
    args = ap.parse_args()

    cfg = load()
    b, rows, skipped = resolve(cfg)
    flash = b.get('flash_mb', 16) * 1024 * K

    if not rows:
        die('nothing to build - put an approved ROM zip in roms/ (see games.toml)')
    if len(rows) > OTA_MAX:
        die(f'{len(rows)} games enabled but ESP-IDF allows at most {OTA_MAX} '
            f'(ota_0..ota_{OTA_MAX-1}). Disable {len(rows)-OTA_MAX} in games.toml.')

    art_kb, exact = mqart_kb(len(rows))
    parts, end = build_table(b, rows, art_kb)
    if end > flash:
        die(f'needs {end/1024/1024:.2f} MB but the part is {flash/1024/1024:.0f} MB. '
            f'Disable a game or reduce a slot_kb in games.toml.')

    # --- report -------------------------------------------------------------
    print(f'\n  MINIMAME  -  {len(rows)} game{"" if len(rows)==1 else "s"} in this build\n')
    w = max(len(r['title']) for r in rows)
    for i, r in enumerate(rows):
        proj = r['project'] or '-'
        print(f'   ota_{i:<2} {r["title"]:<{w}}  {r["slot_kb"]:>5} KB  '
              f'0x{r["offset"]:06X}  {proj:<13} {r["why"]}')
    if skipped:
        print(f'\n  not in this build ({len(skipped)}):')
        for r in skipped:
            print(f'    {r["title"]:<{w}}  {r["why"]}')

    stray = sorted(f[:-4] for f in os.listdir(ROMS) if f.endswith('.zip')) if os.path.isdir(ROMS) else []
    known = {g['rom'] for g in cfg.get('game', [])}
    extra = [s for s in stray if s not in known]
    if extra:
        print(f'\n  ignored, not on the approved list: {", ".join(extra)}')

    used = end / 1024 / K
    print(f'\n  launcher {b.get("launcher_kb",512)} KB'
          f'   artwork {art_kb} KB{"" if exact else " (estimated - run pack_marquees.py)"}'
          f'   slots {len(rows)}/{OTA_MAX}')
    print(f'  flash {used:.2f} / {flash/1024/K:.0f} MB   {flash/1024/K - used:.2f} MB free\n')

    if args.check:
        print('  --check: nothing written\n')
        return

    # --- emit ---------------------------------------------------------------
    out = ['# Generated by tools/configure.py from games.toml - DO NOT EDIT BY HAND.',
           '# Game slots are labelled with the ROM name; the launcher finds them by label.',
           '# Name,     Type, SubType,  Offset,    Size,      Flags']
    for n, t, s, o, z in parts:
        out.append(f'{n+",":<11}{t+",":<6}{s+",":<9}0x{o:06X},  0x{z:06X},')
    with open(PARTS, 'w') as f:
        f.write('\n'.join(out) + '\n')

    os.makedirs(os.path.dirname(MANIF), exist_ok=True)
    with open(MANIF, 'w') as f:
        json.dump({'games': [{k: r.get(k) for k in ('rom', 'title', 'project', 'binary', 'slot_kb', 'offset', 'data_kb', 'data_file', 'data_offset')}
                             for r in rows],
                   'mqart_kb': art_kb, 'flash_mb': b.get('flash_mb', 16)}, f, indent=2)

    print(f'  wrote {os.path.relpath(PARTS, ROOT)} and {os.path.relpath(MANIF, ROOT)}\n')

if __name__ == '__main__':
    main()
