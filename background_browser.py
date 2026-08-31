#!/usr/bin/env python3
"""Small, dependency-free Wallhaven background browser for Snap FE."""

import argparse
import html
import json
import os
import re
import sys
import urllib.parse
import urllib.request

API = "https://wallhaven.cc/api/v1/search"
UA = "SnapFE/1.1.9 (background-browser; https://github.com/differentlightproductions-cyber/snap-fe)"


def api_key():
    """Load a private key without ever embedding it in a command or package."""
    key = os.environ.get("WALLHAVEN_API_KEY", "").strip()
    if key:
        return key
    key_file = os.environ.get(
        "SNAPFE_WALLHAVEN_KEY_FILE",
        os.path.join(os.path.dirname(os.path.abspath(__file__)), "config", "wallhaven.key"),
    )
    try:
        with open(key_file, encoding="utf-8") as fh:
            key = fh.readline().strip()
    except OSError:
        return ""
    # Wallhaven keys are simple URL-safe tokens. Refuse malformed file content
    # rather than accidentally putting arbitrary text into a request URL.
    return key if re.fullmatch(r"[A-Za-z0-9_-]{20,128}", key) else ""


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
    params = {"q": query, "categories": "111", "purity": "100", "sorting": "relevance", "atleast": "640x480"}
    key = api_key()
    if key:
        params["apikey"] = key
    data = request_json(params)
    rows = []
    for page in data.get("data", []):
        url = page.get("path") or ""
        if not url:
            continue
        wid = clean(page.get("id", "wallpaper"), 24)
        title = f"Wallhaven {wid} ({page.get('resolution', 'wallpaper')})"
        artist = clean(page.get("uploader", {}).get("username") or "Wallhaven contributor", 80)
        source = page.get("url") or ("https://wallhaven.cc/w/" + wid)
        rows.append((title, url, "Wallhaven", artist, source))
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
