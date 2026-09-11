#!/usr/bin/env python3
"""Generate the Pocket Crossing runner icons.

The four runners used to be drawn at run time out of overlapping filled
circles, which is why they read as blobs rather than as animals. These are
proper little sprites instead: 24x24 pixel art, drawn once here and shipped as
PNGs, so the game only has to blit them.

24 is not arbitrary. The runner is drawn at 24px in the play area and at 48px
in the picker, both whole-number scales, so the pixels stay square and crisp
instead of being resampled into mush at some awkward fraction.

Silhouettes are built first, then a one-pixel outline is derived from the alpha
channel -- that way the outline always follows the shape and never has to be
hand-traced -- and the face goes on last, on top of the outline.

No third-party imaging libraries: the PNG is encoded here with zlib + struct so
the art can be regenerated on a bare Python install.

    python3 tools/make_crossing_icons.py
"""

import os
import struct
import zlib

SIZE = 24
OUT_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                       "assets", "minigames", "crossing")

OUTLINE = (34, 28, 38, 255)
EYE = (26, 22, 30, 255)
GLINT = (255, 255, 255, 255)


class Canvas:
    def __init__(self, size=SIZE):
        self.size = size
        self.px = [[(0, 0, 0, 0)] * size for _ in range(size)]

    def set(self, x, y, color):
        if 0 <= x < self.size and 0 <= y < self.size and color[3]:
            self.px[y][x] = color

    def opaque(self, x, y):
        return 0 <= x < self.size and 0 <= y < self.size and self.px[y][x][3] > 0

    def disc(self, cx, cy, r, color):
        """Filled circle. Half-pixel centres keep it symmetric at these sizes."""
        rr = (r + 0.35) ** 2
        for y in range(int(cy - r) - 1, int(cy + r) + 2):
            for x in range(int(cx - r) - 1, int(cx + r) + 2):
                if (x + 0.5 - cx) ** 2 + (y + 0.5 - cy) ** 2 <= rr:
                    self.set(x, y, color)

    def ellipse(self, cx, cy, rx, ry, color):
        for y in range(int(cy - ry) - 1, int(cy + ry) + 2):
            for x in range(int(cx - rx) - 1, int(cx + rx) + 2):
                if ((x + 0.5 - cx) / (rx + 0.35)) ** 2 + ((y + 0.5 - cy) / (ry + 0.35)) ** 2 <= 1.0:
                    self.set(x, y, color)

    def rect(self, x, y, w, h, color):
        for yy in range(y, y + h):
            for xx in range(x, x + w):
                self.set(xx, yy, color)

    def tri(self, ax, ay, bx, by, cx, cy, color):
        """Filled triangle, by half-plane test on pixel centres."""
        def side(px, py, x0, y0, x1, y1):
            return (x1 - x0) * (py - y0) - (y1 - y0) * (px - x0)
        xs = [ax, bx, cx]
        ys = [ay, by, cy]
        for y in range(int(min(ys)) - 1, int(max(ys)) + 2):
            for x in range(int(min(xs)) - 1, int(max(xs)) + 2):
                px, py = x + 0.5, y + 0.5
                d1 = side(px, py, ax, ay, bx, by)
                d2 = side(px, py, bx, by, cx, cy)
                d3 = side(px, py, cx, cy, ax, ay)
                if not ((d1 < 0 or d2 < 0 or d3 < 0) and (d1 > 0 or d2 > 0 or d3 > 0)):
                    self.set(x, y, color)

    def outline(self, color=OUTLINE):
        """Every clear pixel touching the silhouette becomes the outline."""
        edge = []
        for y in range(self.size):
            for x in range(self.size):
                if self.opaque(x, y):
                    continue
                if (self.opaque(x - 1, y) or self.opaque(x + 1, y)
                        or self.opaque(x, y - 1) or self.opaque(x, y + 1)):
                    edge.append((x, y))
        for x, y in edge:
            self.px[y][x] = color

    def eyes(self, lx, rx, y, color=EYE, glint=True):
        for x in (lx, rx):
            self.rect(x, y, 2, 2, color)
            if glint:
                self.set(x, y, GLINT)

    def to_bytes(self):
        buf = bytearray()
        for row in self.px:
            for r, g, b, a in row:
                buf += bytes((r, g, b, a))
        return buf


def write_png(path, pixels, width, height):
    """pixels: flat bytearray of RGBA rows."""
    raw = bytearray()
    stride = width * 4
    for y in range(height):
        raw.append(0)                                   # filter type 0 (None)
        raw += pixels[y * stride:(y + 1) * stride]

    def chunk(tag, data):
        out = struct.pack(">I", len(data)) + tag + data
        return out + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    header = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    png = (b"\x89PNG\r\n\x1a\n"
           + chunk(b"IHDR", header)
           + chunk(b"IDAT", zlib.compress(bytes(raw), 9))
           + chunk(b"IEND", b""))
    with open(path, "wb") as f:
        f.write(png)


def kangaroo():
    body = (196, 126, 68, 255)
    light = (232, 182, 134, 255)
    c = Canvas()
    # Tall upright ears, then the head under them.
    c.ellipse(7.5, 5.5, 1.8, 4.0, body)
    c.ellipse(16.5, 5.5, 1.8, 4.0, body)
    c.disc(12, 14, 7.2, body)
    c.outline()
    c.ellipse(7.5, 5.5, 0.8, 2.6, light)
    c.ellipse(16.5, 5.5, 0.8, 2.6, light)
    c.ellipse(12, 17.5, 4.0, 3.0, light)                # long muzzle
    c.rect(11, 16, 2, 2, (86, 58, 44, 255))             # nose
    c.eyes(8, 14, 11)
    return c


def lemur():
    body = (170, 174, 184, 255)
    light = (238, 242, 248, 255)
    dark = (58, 56, 66, 255)
    c = Canvas()
    c.disc(4.5, 8, 3.4, body)                           # round side ears
    c.disc(19.5, 8, 3.4, body)
    c.disc(12, 13.5, 7.2, body)
    c.outline()
    c.disc(4.5, 8, 1.7, light)
    c.disc(19.5, 8, 1.7, light)
    # The wide pale eye patches are what makes a lemur read as a lemur.
    c.disc(9, 12, 3.4, light)
    c.disc(15, 12, 3.4, light)
    c.disc(9, 12, 1.7, dark)
    c.disc(15, 12, 1.7, dark)
    c.set(8, 11, GLINT)
    c.set(14, 11, GLINT)
    c.ellipse(12, 18, 2.6, 2.2, dark)                   # dark muzzle
    return c


def chicken():
    body = (247, 243, 231, 255)
    comb = (216, 72, 64, 255)
    beak = (243, 176, 54, 255)
    c = Canvas()
    c.disc(9, 5, 2.2, comb)                             # three-bump comb
    c.disc(12, 4, 2.4, comb)
    c.disc(15, 5, 2.0, comb)
    c.disc(12, 14, 7.4, body)
    c.tri(9, 15, 15, 15, 12, 21, beak)                  # beak, centred
    c.disc(9.5, 19, 1.3, comb)                          # wattles
    c.disc(14.5, 19, 1.3, comb)
    c.outline()
    c.eyes(9, 14, 11)
    return c


def fox():
    body = (228, 126, 60, 255)
    light = (250, 246, 238, 255)
    inner = (150, 74, 62, 255)
    c = Canvas()
    c.tri(3, 10, 6, 1, 11, 8, body)                     # pointed ears
    c.tri(21, 10, 18, 1, 13, 8, body)
    c.disc(12, 13, 7.4, body)
    c.outline()
    c.tri(5, 9, 6.5, 4, 9.5, 8, inner)
    c.tri(19, 9, 17.5, 4, 14.5, 8, inner)
    c.ellipse(12, 17, 4.4, 3.4, light)                  # white snout
    c.rect(11, 16, 2, 2, (48, 40, 44, 255))             # nose
    c.eyes(8, 14, 11)
    return c


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    for name, build in (("kangaroo", kangaroo), ("lemur", lemur),
                        ("chicken", chicken), ("fox", fox)):
        path = os.path.join(OUT_DIR, name + ".png")
        write_png(path, build().to_bytes(), SIZE, SIZE)
        print("wrote", path)


if __name__ == "__main__":
    main()
