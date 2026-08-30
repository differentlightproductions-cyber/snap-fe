#!/usr/bin/env python3
"""Small, dependency-free Wikimedia Commons background browser for Snap FE."""

import argparse
import html
import json
import os
import re
import sys
import urllib.parse
import urllib.request

API = "https://commons.wikimedia.org/w/api.php"
UA = "SnapFE/1.1.6 (free-background-browser; https://github.com/differentlightproductions-cyber/snap-fe)"
FREE_LICENSES = ("cc0", "public domain", "cc by", "cc-by", "cc by-sa", "cc-by-sa")


def clean(value, limit=120):
    value = html.unescape(re.sub(r"<[^>]+>", "", value or ""))
    value = re.sub(r"[\t\r\n]+", " ", value).strip()
    return value[:limit]


def request_json(params):
    url = API + "?" + urllib.parse.urlencode(params)
    req = urllib.request.Request(url, headers={"User-Agent": UA})
    with urllib.request.urlopen(req, timeout=18) as response:
        return json.load(response)


def search(query, output):
    data = request_json({
        "action": "query",
        "format": "json",
        "formatversion": "2",
        "generator": "search",
        "gsrnamespace": "6",
        "gsrlimit": "24",
        "gsrsearch": query,
        "prop": "imageinfo",
        "iiprop": "url|mime|extmetadata",
        "iiurlwidth": "1280",
        "iiextmetadatafilter": "LicenseShortName|Artist|Credit",
    })
    rows = []
    for page in data.get("query", {}).get("pages", []):
        info = (page.get("imageinfo") or [{}])[0]
        mime = info.get("mime", "")
        if mime not in ("image/jpeg", "image/png"):
            continue
        meta = info.get("extmetadata") or {}
        license_name = clean((meta.get("LicenseShortName") or {}).get("value"), 60)
        if not any(tag in license_name.lower() for tag in FREE_LICENSES):
            continue
        url = info.get("thumburl") or info.get("url") or ""
        if not url:
            continue
        title = clean(page.get("title", "").removeprefix("File:"), 96)
        artist = clean((meta.get("Artist") or meta.get("Credit") or {}).get("value"), 80)
        source = info.get("descriptionurl") or page.get("canonicalurl") or "https://commons.wikimedia.org/"
        rows.append((title, url, license_name, artist or "Wikimedia Commons contributor", source))
        if len(rows) >= 12:
            break
    os.makedirs(os.path.dirname(output), exist_ok=True)
    with open(output, "w", encoding="utf-8") as fh:
        for row in rows:
            fh.write("\t".join(clean(v, 900) for v in row) + "\n")
    return 0 if rows else 2


def safe_name(title, url):
    stem = re.sub(r"[^A-Za-z0-9._-]+", "-", os.path.splitext(title)[0]).strip("-._")[:64]
    ext = os.path.splitext(urllib.parse.urlparse(url).path)[1].lower()
    if ext not in (".jpg", ".jpeg", ".png"):
        ext = ".jpg"
    return (stem or "commons-background") + ext


def download(results, index, dest_dir, receipt):
    with open(results, encoding="utf-8") as fh:
        rows = [line.rstrip("\n").split("\t") for line in fh if line.strip()]
    if index < 0 or index >= len(rows) or len(rows[index]) < 2:
        return 3
    title, url = rows[index][0], rows[index][1]
    os.makedirs(dest_dir, exist_ok=True)
    filename = safe_name(title, url)
    target = os.path.join(dest_dir, filename)
    req = urllib.request.Request(url, headers={"User-Agent": UA})
    with urllib.request.urlopen(req, timeout=30) as response, open(target + ".part", "wb") as out:
        while True:
            block = response.read(65536)
            if not block:
                break
            out.write(block)
    os.replace(target + ".part", target)
    with open(target + ".license.txt", "w", encoding="utf-8") as fh:
        fh.write("Title: " + title + "\n")
        if len(rows[index]) > 2:
            fh.write("License: " + rows[index][2] + "\n")
        if len(rows[index]) > 3:
            fh.write("Creator: " + rows[index][3] + "\n")
        if len(rows[index]) > 4:
            fh.write("Source: " + rows[index][4] + "\n")
    with open(receipt, "w", encoding="utf-8") as fh:
        fh.write(filename + "\n")
    return 0


def main():
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="command", required=True)
    find = sub.add_parser("search")
    find.add_argument("--query", required=True)
    find.add_argument("--output", required=True)
    get = sub.add_parser("download")
    get.add_argument("--results", required=True)
    get.add_argument("--index", required=True, type=int)
    get.add_argument("--dest-dir", required=True)
    get.add_argument("--receipt", required=True)
    args = parser.parse_args()
    try:
        if args.command == "search":
            return search(args.query, args.output)
        return download(args.results, args.index, args.dest_dir, args.receipt)
    except Exception as exc:
        print(f"background browser: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
