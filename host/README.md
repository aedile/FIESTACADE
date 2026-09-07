# host — see the menu without a medal

`preview` compiles the launcher's real UI on a desktop and writes what the panel
would show, at the real 240×280, using the real artwork blob.

```sh
cd host && make
./preview ../lcd/marquees.bin /tmp/menu
```

One PPM per game plus the three states the browse screen has (not installed,
mid-hold, launching).

Nothing here reimplements the layout: `main/menu.cpp`, `components/gfx` and
`components/mqart` are compiled as they are, and the only things stubbed are the
flash partition (read from the blob file), the panel (collected into an image)
and the buttons. So the preview cannot drift from the firmware — if it looks
wrong here, it looks wrong on the medal.

It also renders with a single full-height band, which is how the band
compositor gets checked: `-DGFX_BAND_H=280` must produce byte-identical output.
