#!/usr/bin/env python3
"""Fetch a user's complete RetroAchievements unlock history for Snap FE.

The public history API requires a Web API key, which is intentionally stored
separately from RetroArch's connect token. Output is a small, sanitized TSV so
the SDL frontend can render the book without needing a JSON library.
"""

import argparse
import json
import os
import re
import sys
import time
import urllib.parse
import urllib.request

API = "https://retroachievements.org/API/API_GetAchievementsEarnedBetween.php"
UA = "SnapFE/1.1.6 (achievements-book; https://github.com/differentlightproductions-cyber/snap-fe)"


def clean(value, limit):
    text = str(value or "")
    text = re.sub(r"[\t\r\n|]+", " ", text)
    text = re.sub(r"\s+", " ", text).strip()
    return text[:limit]


def atomic_text(path, text):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    part = path + ".part"
    with open(part, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(text)
    os.replace(part, path)


def status(path, state, message):
    atomic_text(path, f"{state}|{clean(message, 220)}\n")


def read_config(path):
    values = {}
    with open(path, encoding="utf-8") as handle:
        for raw in handle:
            key, sep, value = raw.rstrip("\r\n").partition("=")
            if sep:
                values[key] = value
    return values


def fetch_all(username, web_key):
    params = {
        "y": web_key,
        "u": username,
        "f": 0,
        "t": int(time.time()) + 86400,
    }
    request = urllib.request.Request(
        API + "?" + urllib.parse.urlencode(params),
        headers={"User-Agent": UA, "Accept": "application/json"},
    )
    with urllib.request.urlopen(request, timeout=45) as response:
        payload = json.load(response)
    if isinstance(payload, dict):
        error = payload.get("Error") or payload.get("error") or payload.get("message")
        raise RuntimeError(clean(error or "RetroAchievements returned an unexpected response", 180))
    if not isinstance(payload, list):
        raise RuntimeError("RetroAchievements returned an unexpected response")
    return payload


def normalize(rows):
    # The same achievement can appear once in softcore and later in hardcore.
    # Keep one book entry per achievement, preferring the hardcore unlock.
    unique = {}
    for item in rows:
        achievement_id = int(item.get("AchievementID") or item.get("achievementId") or 0)
        if not achievement_id:
            continue
        hardcore = 1 if item.get("HardcoreMode", item.get("hardcoreMode", False)) else 0
        date = clean(item.get("Date") or item.get("date"), 24)
        previous = unique.get(achievement_id)
        if previous and (previous[1] > hardcore or (previous[1] == hardcore and previous[0] >= date)):
            continue
        unique[achievement_id] = (date, hardcore, item)

    result = []
    for achievement_id, (date, hardcore, item) in unique.items():
        result.append((
            date,
            str(hardcore),
            str(achievement_id),
            str(int(item.get("Points") or item.get("points") or 0)),
            clean(item.get("Title") or item.get("title"), 120),
            clean(item.get("Description") or item.get("description"), 260),
            clean(item.get("GameTitle") or item.get("gameTitle"), 140),
            clean(item.get("ConsoleName") or item.get("consoleName"), 80),
            clean(item.get("BadgeURL") or item.get("badgeUrl"), 180),
        ))
    result.sort(key=lambda row: (row[0], int(row[2])), reverse=True)
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--data", required=True)
    args = parser.parse_args()

    root = os.path.abspath(args.data)
    config = os.path.join(root, "config", "retroachievements.cfg")
    output = os.path.join(root, "ra_achievements.tsv")
    status_file = os.path.join(root, "ra_achievements_status.txt")
    status(status_file, "loading", "Contacting RetroAchievements...")

    try:
        values = read_config(config)
        username = values.get("username", "").strip()
        web_key = values.get("web_api_key", "").strip()
        if not username:
            raise RuntimeError("Enter your RetroAchievements username first")
        if not web_key:
            raise RuntimeError("Enter your RetroAchievements Web API key first")
        rows = normalize(fetch_all(username, web_key))
        text = "".join("\t".join(row) + "\n" for row in rows)
        atomic_text(output, text)
        status(status_file, "done", f"Loaded {len(rows)} earned achievement{'s' if len(rows) != 1 else ''}")
        return 0
    except Exception as exc:
        status(status_file, "error", clean(exc, 200))
        print(f"achievements book: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
