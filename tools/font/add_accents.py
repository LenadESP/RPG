# Alagard ships with plain ASCII only: no á, ñ, ¿... and the menus need them.
# The font is pure pixel art (every glyph sits on a 64-unit grid), so the
# missing letters are built here the honest way: take the base letter as
# pixels, paint the accent on top, write it back as squares.
#
# Run once, commit the result. The game only ever loads the output.
#
#   python3 tools/font/add_accents.py assets/Ui/Font/Alagard.ttf assets/Ui/Font/AlagardES.ttf
#
# Needs fontTools (pip install fonttools).

import sys

from fontTools.pens.pointInsidePen import PointInsidePen
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.ttLib import TTFont

PX = 64          # font units per pixel
ROWS = range(-4, 16)

# Accents, top row first. '#' = pixel.
ACUTE    = [".##",
            "##."]
TILDE    = [".##.#",
            "#.##."]
DIAERESIS = ["#...#"]

LOWER_TOP = 9    # first free row above the x-height (8 px) + 1 px gap
UPPER_TOP = 12   # same, above the caps (11 px)

# new char: (base glyph, accent, top row of the accent, drop rows >= this from the base)
LETTERS = {
    "á": ("a", ACUTE, LOWER_TOP, None),
    "é": ("e", ACUTE, LOWER_TOP, None),
    "í": ("i", ACUTE, LOWER_TOP, 8),      # 8: the i loses its dot first
    "ó": ("o", ACUTE, LOWER_TOP, None),
    "ú": ("u", ACUTE, LOWER_TOP, None),
    "ü": ("u", DIAERESIS, LOWER_TOP, None),
    "ñ": ("n", TILDE, LOWER_TOP, None),
    "Á": ("A", ACUTE, UPPER_TOP, None),
    "É": ("E", ACUTE, UPPER_TOP, None),
    "Í": ("I", ACUTE, UPPER_TOP, None),
    "Ó": ("O", ACUTE, UPPER_TOP, None),
    "Ú": ("U", ACUTE, UPPER_TOP, None),
    "Ü": ("U", DIAERESIS, UPPER_TOP, None),
    "Ñ": ("N", TILDE, UPPER_TOP, None),
}

# new char: (base glyph, how many rows to push it down after turning it 180°)
UPSIDE_DOWN = {
    "¿": ("question", 3),
    "¡": ("exclam", 3),
}

NAMES = {
    "á": "aacute", "é": "eacute", "í": "iacute", "ó": "oacute", "ú": "uacute",
    "ü": "udieresis", "ñ": "ntilde",
    "Á": "Aacute", "É": "Eacute", "Í": "Iacute", "Ó": "Oacute", "Ú": "Uacute",
    "Ü": "Udieresis", "Ñ": "Ntilde",
    "¿": "questiondown", "¡": "exclamdown",
}


def to_pixels(font, name):
    """The glyph as a set of (col, row), row 0 = sitting on the baseline."""
    glyphs = font.getGlyphSet()
    width = font["hmtx"][name][0] // PX
    pixels = set()
    for row in ROWS:
        for col in range(width):
            pen = PointInsidePen(glyphs, (col * PX + PX // 2, row * PX + PX // 2))
            glyphs[name].draw(pen)
            if pen.getResult():
                pixels.add((col, row))
    return pixels


def to_glyph(pixels):
    """One rectangle per horizontal run of pixels, clockwise like the rest of the font."""
    pen = TTGlyphPen(None)
    for row in sorted({r for _, r in pixels}):
        cols = sorted(c for c, r in pixels if r == row)
        start = cols[0]
        for i, col in enumerate(cols):
            last = i == len(cols) - 1
            if last or cols[i + 1] != col + 1:
                x0, x1, y0, y1 = start * PX, (col + 1) * PX, row * PX, (row + 1) * PX
                pen.moveTo((x0, y0))
                pen.lineTo((x0, y1))
                pen.lineTo((x1, y1))
                pen.lineTo((x1, y0))
                pen.closePath()
                if not last:
                    start = cols[i + 1]
    return pen.glyph()


def add(font, char, pixels, advance):
    name = NAMES[char]
    font["glyf"][name] = to_glyph(pixels)
    font["hmtx"][name] = (advance, 0)
    for table in font["cmap"].tables:
        if table.isUnicode():
            table.cmap[ord(char)] = name
    if name not in font.getGlyphOrder():
        font.setGlyphOrder(font.getGlyphOrder() + [name])


def main():
    if len(sys.argv) != 3:
        sys.exit("usage: add_accents.py IN.ttf OUT.ttf")

    font = TTFont(sys.argv[1])
    cmap = font.getBestCmap()

    for char, (base, accent, top, cut) in LETTERS.items():
        pixels = to_pixels(font, base)
        if cut is not None:
            pixels = {(c, r) for c, r in pixels if r < cut}

        # centred over the ink, not over the advance (the advance has the spacing in it)
        cols = [c for c, _ in pixels]
        left = (min(cols) + max(cols) + 1 - len(accent[0])) // 2
        for i, line in enumerate(accent):
            for j, ch in enumerate(line):
                if ch == "#":
                    pixels.add((left + j, top + len(accent) - 1 - i))

        add(font, char, pixels, font["hmtx"][base][0])

    for char, (base, drop) in UPSIDE_DOWN.items():
        pixels = to_pixels(font, base)
        cols = [c for c, _ in pixels]
        rows = [r for _, r in pixels]
        span_c, span_r = min(cols) + max(cols), min(rows) + max(rows)
        pixels = {(span_c - c, span_r - r - drop) for c, r in pixels}
        add(font, char, pixels, font["hmtx"][base][0])

    missing = [c for c in list(LETTERS) + list(UPSIDE_DOWN) if ord(c) not in font.getBestCmap()]
    if missing:
        sys.exit("still missing: " + "".join(missing))

    font.save(sys.argv[2])
    print("wrote", sys.argv[2], "with", "".join(list(LETTERS) + list(UPSIDE_DOWN)))


if __name__ == "__main__":
    main()
