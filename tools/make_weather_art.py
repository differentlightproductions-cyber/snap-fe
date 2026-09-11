#!/usr/bin/env python3
"""Generate the soft-shaded weather wallpaper art used by SNAP FE.

The wallpaper/sleep-screen weather layer used to be drawn with raw SDL
primitives -- flat discs and hard straight rays that fought with the text on
top of them. These PNGs replace that: every edge is a smooth alpha falloff, so
the art dissolves into whatever background is behind it instead of stamping a
shape onto it.

Both images are painted white and shaded only in the alpha channel, because
main.c tints them with SDL_SetTextureColorMod for warm/cool conditions.

No third-party imaging libraries: the PNG is encoded here with zlib + struct so
the art can be regenerated on a bare Python install.

    python3 tools/make_weather_art.py
"""

import math
import os
import struct
import zlib

SIZE = 512
OUT_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                       "assets", "weather")


def smoothstep(edge0, edge1, x):
    if edge0 == edge1:
        return 0.0 if x < edge0 else 1.0
    t = (x - edge0) / (edge1 - edge0)
    if t < 0.0:
        t = 0.0
    elif t > 1.0:
        t = 1.0
    return t * t * (3.0 - 2.0 * t)


def falloff(dist, inner, outer):
    """1.0 inside `inner`, easing to 0.0 at `outer`."""
    return 1.0 - smoothstep(inner, outer, dist)


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


def render(shader, size=SIZE):
    buf = bytearray(size * size * 4)
    half = size / 2.0
    for y in range(size):
        ny = (y + 0.5 - half) / half            # -1 .. 1
        row = y * size * 4
        for x in range(size):
            nx = (x + 0.5 - half) / half
            alpha = shader(nx, ny)
            if alpha <= 0.0:
                continue
            if alpha > 1.0:
                alpha = 1.0
            i = row + x * 4
            buf[i] = 255
            buf[i + 1] = 255
            buf[i + 2] = 255
            buf[i + 3] = int(alpha * 255.0 + 0.5)
    return buf


def sun_shader(nx, ny):
    """A warm disc with a soft corona and gentle rays that dissolve outward.

    The rays are a low-amplitude ripple on the corona alpha rather than drawn
    lines, so they never produce a hard edge across text.
    """
    d = math.hypot(nx, ny)
    if d > 1.0:
        return 0.0
    theta = math.atan2(ny, nx)

    core = falloff(d, 0.20, 0.30) ** 0.75            # solid centre
    inner_glow = 0.42 * falloff(d, 0.26, 0.56) ** 1.6
    corona = 0.20 * falloff(d, 0.30, 1.0) ** 2.1

    # 12 soft rays, strongest just outside the disc and gone by the rim.
    ripple = 0.5 + 0.5 * math.cos(12.0 * theta)
    ray_band = falloff(d, 0.30, 0.94) * smoothstep(0.22, 0.40, d)
    rays = 0.20 * (ripple ** 2.2) * (ray_band ** 1.5)

    return core + inner_glow + corona + rays


def moon_shader(nx, ny):
    """A soft crescent with a shaded terminator, faint maria and a halo."""
    d = math.hypot(nx, ny)
    if d > 1.0:
        return 0.0

    disc = falloff(d, 0.44, 0.50)

    # Carve the crescent with a second, offset disc -- soft-edged so the
    # terminator reads as shading rather than a cut-out.
    sx, sy = nx - 0.26, ny + 0.10
    shadow = falloff(math.hypot(sx, sy), 0.38, 0.50)
    lit = disc * (1.0 - shadow)

    # Limb darkening: brightest along the outer edge of the crescent.
    lit *= 0.72 + 0.28 * smoothstep(0.10, 0.46, d)

    # A couple of very faint maria so the face is not a flat blob.
    for cx, cy, r, depth in ((-0.20, -0.16, 0.13, 0.16),
                             (-0.28, 0.14, 0.10, 0.12),
                             (-0.06, 0.26, 0.08, 0.10)):
        lit *= 1.0 - depth * falloff(math.hypot(nx - cx, ny - cy), r * 0.4, r)

    halo = 0.13 * falloff(d, 0.46, 1.0) ** 2.4
    return lit + halo


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    for name, shader in (("weather-sun.png", sun_shader),
                         ("weather-moon.png", moon_shader)):
        path = os.path.join(OUT_DIR, name)
        write_png(path, render(shader), SIZE, SIZE)
        print("wrote %s (%d bytes)" % (path, os.path.getsize(path)))


if __name__ == "__main__":
    main()
