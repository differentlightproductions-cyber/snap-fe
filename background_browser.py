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
from concurrent.futures import ThreadPoolExecutor

API = "https://wallhaven.cc/api/v1/search"
INFO_API = "https://wallhaven.cc/api/v1/w/"
UA = "SnapFE/1.2.0 (background-browser; https://github.com/differentlightproductions-cyber/snap-fe)"


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


def request_json(params, endpoint=API):
    url = endpoint + ("?" + urllib.parse.urlencode(params) if params else "")
    req = urllib.request.Request(url, headers={"User-Agent": UA})
    with urllib.request.urlopen(req, timeout=18) as response:
        return json.load(response)


def wallpaper_info(page, key, query):
    """Add the descriptive fields omitted from Wallhaven search listings."""
    wid = clean(page.get("id", "wallpaper"), 24)
    detail = page
    try:
        params = {"apikey": key} if key else {}
        detail = request_json(params, INFO_API + wid).get("data", page)
    except Exception:
        # A listing is still useful if an individual metadata lookup times out.
        detail = page

    tags = [clean(tag.get("name", ""), 48) for tag in detail.get("tags", [])]
    tags = [tag for tag in tags if tag]
    subject = ", ".join(tags[:3]) or clean(query.title(), 80) or "Wallpaper"
    title = f"{subject} [{wid}]"
    artist = clean(detail.get("uploader", {}).get("username") or "Wallhaven contributor", 80)
    source = clean(detail.get("source") or detail.get("url") or ("https://wallhaven.cc/w/" + wid), 900)
    resolution = clean(detail.get("resolution") or page.get("resolution") or "Unknown size", 32)
    category = clean(detail.get("category") or page.get("category") or "general", 32).title()
    views = int(detail.get("views") or page.get("views") or 0)
    favorites = int(detail.get("favorites") or page.get("favorites") or 0)
    created = clean(detail.get("created_at") or page.get("created_at") or "", 32)
    stats = f"{resolution} | {category} | {views:,} views | {favorites:,} saves"
    direct_url = detail.get("path") or page.get("path") or ""
    original_filename = os.path.basename(urllib.parse.urlparse(direct_url).path)
    thumbs = detail.get("thumbs") or page.get("thumbs") or {}
    preview_url = thumbs.get("large") or thumbs.get("original") or thumbs.get("small") or ""
    return (title, direct_url, "Wallhaven", artist, source, stats, wid, resolution,
            category, str(views), str(favorites), created, original_filename, preview_url)


def downloaded_identities(dest_dir):
    """Return stable Wallhaven IDs/direct filenames already saved in a system folder."""
    ids, direct_names = set(), set()
    if not dest_dir or not os.path.isdir(dest_dir):
        return ids, direct_names
    for name in os.listdir(dest_dir):
        if not name.endswith(".license.txt"):
            match = re.search(r"wallhaven[-_ ]([a-z0-9]{6})(?:[-_. ]|$)", name, re.I)
            if match:
                ids.add(match.group(1).lower())
            continue
        try:
            with open(os.path.join(dest_dir, name), encoding="utf-8") as fh:
                for line in fh:
                    label, sep, value = line.partition(":")
                    if not sep:
                        continue
                    value = value.strip()
                    if label == "Wallhaven ID" and value:
                        ids.add(value.lower())
                    elif label == "Original Filename" and value:
                        direct_names.add(value.lower())
                    elif label == "Source":
                        match = re.search(r"wallhaven\.cc/w/([a-z0-9]{6})", value, re.I)
                        if match:
                            ids.add(match.group(1).lower())
        except OSError:
            pass
    return ids, direct_names


def cache_preview(row, preview_dir):
    """Cache a small search thumbnail and replace its remote URL with a local path."""
    preview_url = row[13] if len(row) > 13 else ""
    local_path = ""
    if preview_dir and preview_url:
        os.makedirs(preview_dir, exist_ok=True)
        local_path = os.path.join(preview_dir, clean(row[6], 24) + ".jpg")
        if not os.path.isfile(local_path) or os.path.getsize(local_path) == 0:
            part = local_path + ".part"
            try:
                req = urllib.request.Request(preview_url, headers={"User-Agent": UA})
                with urllib.request.urlopen(req, timeout=20) as response, open(part, "wb") as out:
                    while True:
                        block = response.read(65536)
                        if not block:
                            break
                        out.write(block)
                os.replace(part, local_path)
            except Exception:
                try:
                    os.remove(part)
                except OSError:
                    pass
                local_path = ""
    return tuple(row[:13]) + (local_path,)


def search(query, output, dest_dir="", preview_dir=""):
    params = {"q": query, "categories": "111", "purity": "100", "sorting": "relevance", "atleast": "640x480"}
    key = api_key()
    if key:
        params["apikey"] = key
    data = request_json(params)
    saved_ids, saved_names = downloaded_identities(dest_dir)
    pages = []
    for page in data.get("data", []):
        direct_name = os.path.basename(urllib.parse.urlparse(page.get("path") or "").path).lower()
        if not page.get("path") or str(page.get("id", "")).lower() in saved_ids or direct_name in saved_names:
            continue
        pages.append(page)
        if len(pages) >= 12:
            break
    with ThreadPoolExecutor(max_workers=4) as pool:
        rows = list(pool.map(lambda page: wallpaper_info(page, key, query), pages))
    if preview_dir:
        with ThreadPoolExecutor(max_workers=4) as pool:
            rows = list(pool.map(lambda row: cache_preview(row, preview_dir), rows))
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
        labels = ((2, "License"), (3, "Creator"), (4, "Source"), (5, "Stats"),
                  (6, "Wallhaven ID"), (7, "Resolution"), (8, "Category"), (11, "Created"),
                  (12, "Original Filename"))
        for field, label in labels:
            if len(rows[index]) > field and rows[index][field]:
                fh.write(label + ": " + rows[index][field] + "\n")
    with open(receipt, "w", encoding="utf-8") as fh:
        fh.write(filename + "\n")
    return 0


def main():
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="command", required=True)
    find = sub.add_parser("search")
    find.add_argument("--query", required=True)
    find.add_argument("--output", required=True)
    find.add_argument("--dest-dir", default="")
    find.add_argument("--preview-dir", default="")
    get = sub.add_parser("download")
    get.add_argument("--results", required=True)
    get.add_argument("--index", required=True, type=int)
    get.add_argument("--dest-dir", required=True)
    get.add_argument("--receipt", required=True)
    args = parser.parse_args()
    try:
        if args.command == "search":
            return search(args.query, args.output, args.dest_dir, args.preview_dir)
        return download(args.results, args.index, args.dest_dir, args.receipt)
    except Exception:
        # Do not echo request URLs: authenticated URLs contain the user's key.
        print("background browser: request failed", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
