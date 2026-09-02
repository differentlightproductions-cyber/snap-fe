#!/usr/bin/env python3
"""
Snap OS art scraper.

Scans <roms>/<system>/ for ROM files, looks each one up on the chosen
source, and downloads art into <data>/boxart/<system>/<slug>/ using the
same base filename as the ROM. Snap OS checks this cache at scan time.

Sources:
  --source=gamesdb        TheGamesDB   (needs config/thegamesdb.key)
  --source=screenscraper  ScreenScraper (needs config/screenscraper.cfg with
                          ssid=/sspassword=, optionally devid=/devpassword=)

Paths (Snap OS passes these; sensible ~/snapos-ui defaults otherwise):
  --data=<dir>   Snap OS data root   (config/, boxart/, scrape_status.txt)
  --roms=<dir>   ROM library root

Other flags: --types=a,b,c   --systems=gba,gbc,...
             --description=0|1   --only-missing=0|1
"""

import os
import re
import sys
import json
import time
from urllib.request import urlopen, Request
from urllib.parse import urlencode
from urllib.error import HTTPError, URLError

HOME = os.path.expanduser("~")


def _arg(name, default=None):
    pref = f"--{name}="
    for a in sys.argv[1:]:
        if a.startswith(pref):
            return a[len(pref):].strip()
    return default


DATA_ROOT = _arg("data") or f"{HOME}/snapos-ui"
ROMS_BASE = _arg("roms") or f"{HOME}/snapos-ui/roms"
BOXART_BASE = os.path.join(DATA_ROOT, "boxart")
CONFIG_DIR = os.path.join(DATA_ROOT, "config")
STATUS_PATH = os.path.join(DATA_ROOT, "scrape_status.txt")
TGDB_KEY_PATH = os.path.join(CONFIG_DIR, "thegamesdb.key")
SS_CFG_PATH = os.path.join(CONFIG_DIR, "screenscraper.cfg")

# Unified art slugs -- must match art_type_slugs[] in main.c.
ALL_ART_TYPES = ["box2d", "box3d", "screenshot", "titlescreen", "logo", "fanart", "mix", "cartridge"]
ART_TYPE_DISPLAY = {
    "box2d": "2D Box Art", "box3d": "3D Box Art", "screenshot": "Screenshot",
    "titlescreen": "Title Screen", "logo": "Logo", "fanart": "Fan Art", "mix": "Mix",
    "cartridge": "Cartridge",
}

# Folder short-name -> (TheGamesDB platform name, ScreenScraper system id).
PLATFORMS = {
    "gba":     ("Nintendo Game Boy Advance", 12),
    "gbc":     ("Nintendo Game Boy Color", 10),
    "gb":      ("Nintendo Game Boy", 9),
    "nes":     ("Nintendo Entertainment System (NES)", 3),
    "snes":    ("Super Nintendo (SNES)", 4),
    "genesis": ("Sega Genesis", 1),
    "n64":     ("Nintendo 64", 14),
    "psx":     ("Sony Playstation", 57),
    "mastersystem": ("Sega Master System", 2),
    "gamegear": ("Sega Game Gear", 21),
    "pcengine": ("TurboGrafx 16", 31),
    "neogeo":  ("Neo Geo", 142),
    "atari2600": ("Atari 2600", 26),
    "fbneo":   ("Arcade", 75),
}
# extra on-disk folder names that map to the same short name
DIR_ALIASES = {
    "megadrive": "genesis", "playstation": "psx", "sms": "mastersystem",
    "tg16": "pcengine", "arcade": "fbneo",
}

ROM_EXTENSIONS = (".gba", ".gbc", ".gb", ".nes", ".fds", ".sfc", ".smc",
                  ".md", ".gen", ".bin", ".smd", ".68k", ".sgd", ".n64",
                  ".z64", ".v64", ".cue", ".chd", ".pbp", ".m3u", ".iso",
                  ".img", ".mdf", ".ecm", ".sms", ".gg", ".pce", ".sgx",
                  ".neo", ".a26", ".zip", ".7z")

SYSTEM_FILTER = {s.strip() for s in (_arg("systems", "") or "").split(",") if s.strip()}

# slug -> TheGamesDB image "type" (+ side for box art)
TGDB_MAP = {
    "box2d": ("boxart", "front"),
    "box3d": ("boxart", "front"),   # TGDB has no 3D box; front box is the best it can do
    "screenshot": ("screenshot", None),
    "titlescreen": ("titlescreen", None),
    "logo": ("clearlogo", None),
    "fanart": ("fanart", None),
    "mix": (None, None),            # TGDB has nothing mix-like
    "cartridge": (None, None),      # nor cartridge/"support" media
}
# slug -> ordered list of acceptable ScreenScraper media "type" values
SS_MAP = {
    "box2d": ["box-2D"],
    "box3d": ["box-3D"],
    "screenshot": ["ss"],
    "titlescreen": ["sstitle"],
    "logo": ["wheel", "wheel-hd", "screenmarquee"],
    "fanart": ["fanart"],
    # ScreenScraper calls the physical media "support": a flat label scan
    # first, then the 3D render if that is all the game has.
    "cartridge": ["support-2D", "support-3D"],
    "mix": ["mixrbv2", "mixrbv1"],
}
SS_REGION_PREF = ["wor", "us", "ss", "eu", "jp"]  # preferred order for regional media variants


# --------------------------------------------------------------------------- #
def write_status(idx, total, label, state):
    try:
        os.makedirs(os.path.dirname(STATUS_PATH), exist_ok=True)
        label = str(label).replace("|", "/").replace("\n", " ")
        with open(STATUS_PATH, "w") as f:
            f.write(f"{idx}|{total}|{label}|{state}")
    except Exception:
        pass


def parse_types():
    raw = _arg("types", "box2d")
    req = [t.strip() for t in raw.split(",") if t.strip()]
    return [t for t in req if t in ALL_ART_TYPES] or ["box2d"]


def flag(name, default):
    v = _arg(name)
    if v is None:
        return default
    return v == "1"


def clean_title(filename):
    name = os.path.splitext(filename)[0]
    name = re.sub(r"[\(\[].*?[\)\]]", "", name)
    return name.strip()


def already_cached(art_dir, base):
    return any(os.path.exists(os.path.join(art_dir, base + e))
              for e in (".jpg", ".jpeg", ".png"))


class ScrapeError(Exception):
    """Fatal, reportable problem -- network down, quota, service error."""


class AuthError(ScrapeError):
    """Credentials were rejected (bad key / bad login)."""


TGDB_QUOTA_MSG = ("TheGamesDB quota reached. Set Scrape Source to ScreenScraper "
                  "in Settings, or wait for the monthly reset.")


def _transient_dns(err):
    s = str(err).lower()
    return ("temporary failure in name resolution" in s
            or "name or service not known" in s
            or "nodename nor servname" in s
            or "no address associated with hostname" in s
            or "[errno -2]" in s or "[errno -3]" in s or "[errno -5]" in s)


def wait_for_net(host, secs=18):
    """Right after a cold boot Wi-Fi + DNS can still be settling; a scrape
    kicked off in that window used to fail instantly. Give the network a
    chance to come up before we declare it dead. We resolve the source's own
    host; if only that one host is unresolvable but a neutral host is fine,
    that's the service's problem, not our connectivity -- proceed anyway."""
    import socket
    deadline = time.time() + secs
    first = True
    while time.time() < deadline:
        try:
            socket.getaddrinfo(host, 443, type=socket.SOCK_STREAM)
            return True
        except Exception:
            if first:
                write_status(0, 0, "Waiting for network...", "scraping")
                first = False
            # If a rock-solid host resolves, the network is up -- don't keep
            # waiting on the one service host.
            try:
                socket.getaddrinfo("one.one.one.one", 443, type=socket.SOCK_STREAM)
                return True
            except Exception:
                pass
            time.sleep(2)
    return False


def http_get(url, timeout=25, retries=4):
    req = Request(url, headers={"User-Agent": "SnapOS-Scraper/2.0"})
    last = None
    for attempt in range(retries + 1):
        try:
            with urlopen(req, timeout=timeout) as resp:
                return resp.read()
        except HTTPError as e:
            if e.code in (401, 403):
                raise AuthError("Credentials rejected by the server (HTTP %d)" % e.code)
            if e.code == 429:
                # TheGamesDB returns JSON in the 429 body with the remaining
                # monthly allowance. If we still have allowance it's just a
                # short burst limit -- back off and retry instead of bailing
                # with a scary "quota reached".
                allowance = None
                refresh = None
                try:
                    j = json.loads(e.read().decode("utf-8", "replace"))
                    allowance = j.get("remaining_monthly_allowance")
                    refresh = j.get("allowance_refresh_timer")
                except Exception:
                    pass
                if allowance == 0:
                    when = ""
                    try:
                        days = int(refresh) // 86400
                        when = f" (resets in ~{days} day%s)" % ("s" if days != 1 else "")
                    except Exception:
                        pass
                    raise ScrapeError("This TheGamesDB key is out of its monthly "
                                      "allowance%s. Switch Scrape Source to "
                                      "ScreenScraper, or enter a different key." % when)
                try:
                    ra = int(e.headers.get("Retry-After", "0"))
                except Exception:
                    ra = 0
                if attempt < retries:
                    delay = ra if 0 < ra <= 60 else min(15 * (attempt + 1), 45)
                    print(f"  rate limited (HTTP 429, allowance={allowance}); "
                          f"waiting {delay}s (try {attempt + 1}/{retries})...")
                    write_status(0, 0, "Rate limited -- waiting to retry...", "scraping")
                    time.sleep(delay)
                    continue
                raise ScrapeError("TheGamesDB is rate limiting the key -- "
                                  "wait a minute and try again")
            if e.code in (500, 502, 503, 504) and attempt < retries:
                time.sleep(4 * (attempt + 1))
                continue
            if e.code == 503:
                raise ScrapeError("TheGamesDB is temporarily unavailable (HTTP 503) -- try again later")
            raise ScrapeError("Server error (HTTP %d)" % e.code)
        except URLError as e:
            last = getattr(e, "reason", e)
            if attempt < retries:
                secs = 4 * (attempt + 1)
                if _transient_dns(last):
                    print(f"  network not ready ({last}); retrying in {secs}s...")
                    write_status(0, 0, "Waiting for network...", "scraping")
                else:
                    print(f"  network error ({last}); retrying in {secs}s...")
                time.sleep(secs)
                continue
            raise ScrapeError("No network connection (%s)" % last)
        except (TimeoutError, OSError) as e:
            last = e
            if attempt < retries:
                time.sleep(4 * (attempt + 1))
                continue
            raise ScrapeError("Network timeout / unreachable (%s)" % e)
    raise ScrapeError("Network unreachable (%s)" % (last or "unknown"))


def download_image(url, dest_path):
    data = http_get(url, timeout=30)
    if len(data) < 256:
        raise ValueError("image response too small")
    with open(dest_path, "wb") as f:
        f.write(data)


def iter_roms():
    """Yield (short_dir, fname, base, title) for every ROM found."""
    seen_dirs = list(PLATFORMS) + list(DIR_ALIASES)
    for d in seen_dirs:
        short = DIR_ALIASES.get(d, d)
        if SYSTEM_FILTER and short not in SYSTEM_FILTER:
            continue
        rom_dir = os.path.join(ROMS_BASE, d)
        if not os.path.isdir(rom_dir):
            continue
        for root, _dirs, files in os.walk(rom_dir):
            for fname in sorted(files):
                if not fname.lower().endswith(ROM_EXTENSIONS):
                    continue
                base = os.path.splitext(fname)[0]
                yield short, fname, base, clean_title(fname)


# --------------------------------------------------------------------------- #
class TheGamesDB:
    name = "TheGamesDB"
    API = "https://api.thegamesdb.net/v1"

    def __init__(self):
        if not os.path.exists(TGDB_KEY_PATH):
            fail(f"no TheGamesDB key at {TGDB_KEY_PATH}")
        with open(TGDB_KEY_PATH) as f:
            self.key = f.read().strip()
        if not self.key:
            fail("TheGamesDB key file is empty")
        self.platform_ids = {}
        self._img_cache = {}

    def _get(self, endpoint, params):
        params = dict(params, apikey=self.key)
        raw = http_get(f"{self.API}/{endpoint}?{urlencode(params)}", timeout=15)
        data = json.loads(raw.decode())
        code = data.get("code")
        if code in (401, 403) or str(data.get("status", "")).lower().startswith("invalid"):
            raise AuthError("TheGamesDB rejected the key -- check it in Settings > Account")
        return data

    def setup(self):
        # A bad key surfaces here first -- let AuthError/ScrapeError propagate so
        # main() can report it. Only an empty/odd platform list is non-fatal.
        try:
            data = self._get("Platforms", {})
        except AuthError:
            raise
        except ScrapeError:
            raise
        except Exception as e:
            print(f"  platform list failed: {e}")
            return
        plats = data.get("data", {}).get("platforms", {})
        for pid, info in plats.items():
            nm = info.get("name", "").lower()
            for short, (hint, _ss) in PLATFORMS.items():
                if hint.lower() == nm:
                    self.platform_ids[short] = int(pid)

    def find_game(self, short, title):
        params = {"name": title, "fields": "overview"}
        pid = self.platform_ids.get(short)
        if pid:
            params["filter[platform]"] = pid
        data = self._get("Games/ByGameName", params)
        games = data.get("data", {}).get("games", [])
        return games[0] if games else None

    def description(self, game):
        return (game.get("overview") or "").strip()

    def _images(self, game_id):
        if game_id not in self._img_cache:
            data = self._get("Games/Images", {"games_id": game_id})
            base = data.get("data", {}).get("base_url", {}).get("original", "")
            imgs = data.get("data", {}).get("images", {}).get(str(game_id), [])
            self._img_cache[game_id] = (base, imgs)
        return self._img_cache[game_id]

    def art_url(self, game, slug):
        ttype, side = TGDB_MAP.get(slug, (None, None))
        if not ttype:
            return None
        base, imgs = self._images(game.get("id"))
        for img in imgs:
            if img.get("type") != ttype:
                continue
            if side and img.get("side") != side:
                continue
            return base + img.get("filename", "")
        return None


class ScreenScraper:
    name = "ScreenScraper"
    API = "https://api.screenscraper.fr/api2"

    def __init__(self):
        cfg = {}
        if os.path.exists(SS_CFG_PATH):
            with open(SS_CFG_PATH) as f:
                for line in f:
                    line = line.strip()
                    if "=" in line and not line.startswith("#"):
                        k, v = line.split("=", 1)
                        cfg[k.strip()] = v.strip()
        self.ssid = cfg.get("ssid", "")
        self.sspassword = cfg.get("sspassword", "")
        # App-level dev credential. ScreenScraper's API needs one to identify the
        # scraping software (separate from your account). Default to the same one
        # Knulli's EmulationStation ships with, so -- exactly like ES -- the user
        # only has to enter their personal login. Override in screenscraper.cfg.
        self.devid = cfg.get("devid", "") or "knulli"
        self.devpassword = cfg.get("devpassword", "") or "74kuKW8xoqV"
        self.softname = cfg.get("softname", "") or "knulli"
        if not self.ssid:
            print("  NOTE: no ScreenScraper login in screenscraper.cfg -- "
                  "running anonymously (slow, low quota).")

    def _base_params(self):
        p = {"output": "json", "softname": self.softname}
        if self.devid:
            p["devid"] = self.devid
            p["devpassword"] = self.devpassword
        if self.ssid:
            p["ssid"] = self.ssid
            p["sspassword"] = self.sspassword
        return p

    def setup(self):
        pass

    def _raw(self, url, timeout=45):
        """GET returning (status, body-bytes). ScreenScraper signals login and
        quota errors in the *body* even on non-200, so we always read it.
        Returns (None, b"") on a timeout so the caller can retry."""
        req = Request(url, headers={"User-Agent": "SnapOS-Scraper/2.0"})
        try:
            with urlopen(req, timeout=timeout) as resp:
                return 200, resp.read()
        except HTTPError as e:
            try:
                return e.code, e.read()
            except Exception:
                return e.code, b""
        except URLError as e:
            r = getattr(e, "reason", e)
            if isinstance(r, (TimeoutError, OSError)) or "timed out" in str(r).lower():
                return None, b""          # retryable
            raise ScrapeError("No network connection (%s)" % r)
        except (TimeoutError, OSError):
            return None, b""              # retryable

    def find_game(self, short, title, fname=None):
        sysid = PLATFORMS[short][1]
        params = self._base_params()
        params["systemeid"] = sysid
        params["romtype"] = "rom"
        params["romnom"] = fname or (title + ".zip")
        url = f"{self.API}/jeuInfos.php?{urlencode(params)}"

        for attempt in range(5):
            status, body = self._raw(url)
            if status is None:            # timeout -- ScreenScraper is slow/throttling
                if attempt < 4:
                    time.sleep(6 * (attempt + 1))
                    continue
                raise ScrapeError("ScreenScraper is not responding (timeouts). "
                                  "It throttles by IP -- wait a while and retry.")
            txt = body.decode("utf-8", "replace").strip()
            low = txt.lower()

            if txt[:1] == "{":
                try:
                    data = json.loads(txt)
                except ValueError:
                    return None
                return data.get("response", {}).get("jeu")

            # --- non-JSON: ScreenScraper's plain-text error strings ---
            # Developer-credential error comes back looking like a login error,
            # so check it FIRST -- it means devid/devpassword, not your account.
            if any(s in low for s in ("développeur", "developpeur", "developer",
                                      "identifiants developpeur")):
                raise AuthError("ScreenScraper needs developer credentials. Add "
                                "devid= and devpassword= to screenscraper.cfg, "
                                "or switch Scrape Source to TheGamesDB.")
            if any(s in low for s in (
                    "erreur de login", "vérifiez vos identifiants",
                    "verifiez vos identifiants", "bad login", "wrong login",
                    "mot de passe")):
                raise AuthError("ScreenScraper rejected your account login -- "
                                "check the username/password in Settings.")
            if any(s in low for s in ("quota", "maximum", "dépass", "depass",
                                      "limite")):
                if attempt < 3:
                    time.sleep(10)
                    continue
                raise ScrapeError("ScreenScraper request quota reached -- "
                                  "try again later")
            if any(s in low for s in ("ferme", "fermé", "closed", "maintenance",
                                      "surcharg", "overload")):
                raise ScrapeError("ScreenScraper is busy or in maintenance -- "
                                  "try again later")
            if status in (401, 403):
                raise AuthError("ScreenScraper rejected the credentials (HTTP %d)"
                                % status)
            if status == 404:
                return None          # genuinely no match for this ROM
            # anything else unrecognised: treat as a miss, keep going
            return None
        return None

    def description(self, game):
        for s in game.get("synopsis", []) or []:
            if s.get("langue") in ("en", "en_us"):
                return (s.get("text") or "").strip()
        syn = game.get("synopsis", []) or []
        return (syn[0].get("text", "").strip() if syn else "")

    def _pick(self, medias, want_types):
        for wt in want_types:
            cands = [m for m in medias if m.get("type") == wt and m.get("url")]
            if not cands:
                continue
            for reg in SS_REGION_PREF:
                for m in cands:
                    if m.get("region") == reg:
                        return m["url"]
            return cands[0]["url"]
        return None

    def art_url(self, game, slug):
        return self._pick(game.get("medias", []) or [], SS_MAP.get(slug, []))


def fail(msg):
    """Report a fatal, user-actionable reason and stop. Snap OS reads the
    status file (state='error'), not the exit code, so exit 0."""
    msg = str(msg).strip() or "Scrape failed"
    print(f"ERROR: {msg}")
    write_status(0, 0, msg[:140], "error")
    sys.exit(0)


# --------------------------------------------------------------------------- #
def main():
    source = (_arg("source", "gamesdb") or "gamesdb").lower()
    types = parse_types()
    want_desc = flag("description", False)
    only_missing = flag("only-missing", True)

    print(f"Source: {source}   types: {', '.join(types)}   "
          f"desc: {want_desc}   only-missing: {only_missing}")
    print(f"data={DATA_ROOT}  roms={ROMS_BASE}")

    write_status(0, 0, "Starting...", "scraping")
    is_ss = source in ("screenscraper", "ss")
    if not wait_for_net("api.screenscraper.fr" if is_ss else "api.thegamesdb.net"):
        fail("No network -- connect Wi-Fi and try again")
    backend = ScreenScraper() if is_ss else TheGamesDB()
    try:
        backend.setup()
    except AuthError as e:
        fail(str(e))
    except ScrapeError as e:
        fail(str(e))
    except Exception as e:
        print(f"  setup warning: {e}")

    # Build the work list up front so Snap OS can show "n / total".
    work = []   # (short, fname, base, title, slug)  -- slug "description" is special
    for short, fname, base, title in iter_roms():
        for slug in types:
            art_dir = os.path.join(BOXART_BASE, short, slug)
            os.makedirs(art_dir, exist_ok=True)
            if only_missing and already_cached(art_dir, base):
                continue
            work.append((short, fname, base, title, slug))
        if want_desc:
            dd = os.path.join(BOXART_BASE, short, "description")
            os.makedirs(dd, exist_ok=True)
            if not (only_missing and os.path.exists(os.path.join(dd, base + ".txt"))):
                work.append((short, fname, base, title, "description"))

    total = len(work)
    if total == 0:
        write_status(0, 0, "Nothing to scrape", "done")
        print("Nothing to do.")
        return

    found = missed = 0
    consec_fail = 0
    game_cache = {}   # (short, base) -> game dict or None

    for idx, (short, fname, base, title, slug) in enumerate(work):
        label = f"{title} ({ART_TYPE_DISPLAY.get(slug, 'Description')})"
        write_status(idx, total, label, "scraping")
        print(f"[{idx + 1}/{total}] {label}")

        key = (short, base)
        if key not in game_cache:
            try:
                if isinstance(backend, ScreenScraper):
                    game_cache[key] = backend.find_game(short, title, fname)
                else:
                    game_cache[key] = backend.find_game(short, title)
                consec_fail = 0
            except AuthError as e:
                # Bad credentials will fail for every ROM -- stop now.
                fail(str(e))
            except ScrapeError as e:
                # Transient: a timeout / momentary rate-limit on ONE lookup.
                # Skip this game and carry on; only bail if it keeps happening.
                print(f"  temporary error: {e}")
                game_cache[key] = None
                consec_fail += 1
                if consec_fail >= 8:
                    fail(f"{e}  (gave up after {consec_fail} in a row)")
                write_status(idx, total, f"{title} -- retrying...", "scraping")
                time.sleep(3)
            except Exception as e:
                print(f"  search error: {e}")
                game_cache[key] = None
        game = game_cache[key]
        if not game:
            print("  no match")
            missed += 1
            continue

        if slug == "description":
            text = ""
            try:
                text = backend.description(game)
            except Exception as e:
                print(f"  description error: {e}")
            if not text:
                print("  no description")
                missed += 1
                continue
            dest = os.path.join(BOXART_BASE, short, "description", base + ".txt")
            try:
                with open(dest, "w", encoding="utf-8") as f:
                    f.write(text)
                found += 1
                print("  saved description")
            except Exception as e:
                print(f"  save error: {e}")
                missed += 1
            continue

        try:
            url = backend.art_url(game, slug)
        except Exception as e:
            print(f"  art lookup error: {e}")
            missed += 1
            continue
        if not url:
            print(f"  no {slug}")
            missed += 1
            continue

        ext = os.path.splitext(url.split("?", 1)[0])[1].lower()
        if ext not in (".jpg", ".jpeg", ".png"):
            ext = ".png"
        dest = os.path.join(BOXART_BASE, short, slug, base + ext)
        try:
            download_image(url, dest)
            found += 1
            print(f"  saved -> {dest}")
        except Exception as e:
            print(f"  download error: {e}")
            missed += 1

    write_status(total, total, f"Done -- {found} saved, {missed} missed", "done")
    print(f"\nDone. {found} saved, {missed} missed.")


if __name__ == "__main__":
    try:
        main()
    except SystemExit:
        raise
    except KeyboardInterrupt:
        pass
    except (AuthError, ScrapeError) as e:
        fail(str(e))
    except Exception as e:
        # Never let an uncaught traceback leave Snap OS spinning "Scraping...".
        import traceback
        traceback.print_exc()
        try:
            write_status(0, 0, f"Scraper crashed: {e}"[:140], "error")
        except Exception:
            pass
        sys.exit(0)
