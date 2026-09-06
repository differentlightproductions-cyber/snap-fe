#!/usr/bin/env python3
"""Read-only Knulli audit. Prints controller/BIOS facts, never account values.

Run on the handheld with Python, or pipe this file to `ssh DEVICE python3 -`.
Does not launch games, change files, install mappings, or fetch firmware.
"""
from pathlib import Path
import json
import re
import xml.etree.ElementTree as ET


def cfg(path):
    try:
        lines = Path(path).read_text(errors="replace").splitlines()
    except OSError:
        return {}
    result = {}
    for line in lines:
        if not line.strip() or line.lstrip().startswith(("#", ";")) or "=" not in line:
            continue
        key, value = line.split("=", 1)
        result[key.strip()] = value.strip().strip('"')
    return result


def tree(path):
    try:
        return ET.parse(path).getroot()
    except (OSError, ET.ParseError):
        return ET.Element("missing")


def installed_roms(path, extensions):
    """Presence only, no full ROM walk; subfolders mean possible content."""
    try:
        for entry in Path(path).iterdir():
            if entry.name.startswith(".") or entry.name in {"images", "media", "videos", "manuals"}:
                continue
            if entry.is_dir() or entry.suffix.lower() in extensions:
                return True
    except OSError:
        pass
    return False


def firmware(core, system):
    data = cfg(f"/usr/share/libretro/info/{core}_libretro.info")
    base = Path("/userdata/bios/amiga" if core in {"puae", "puae2021", "uae4arm"} else "/userdata/bios")
    result = []
    for key, path in data.items():
        match = re.fullmatch(r"firmware(\d+)_path", key)
        if not match:
            continue
        prefix = "firmware" + match[1]
        result.append({"file": str(base / path), "present": (base / path).is_file(),
                       "optional": data.get(prefix + "_opt", "").lower() in {"true", "1"},
                       "purpose": data.get(prefix + "_desc", "")})
    return result


def main():
    pads = []
    text = Path("/proc/bus/input/devices").read_text(errors="replace")
    for block in text.split("\n\n"):
        if not re.search(r"Handlers=.*\bjs\d+\b", block):
            continue
        name = re.search(r'N: Name="(.*?)"', block)
        event = re.search(r"\bevent\d+\b", block)
        if name:
            pads.append({"name": name[1], "event": event[0] if event else None})
    databases = ["/userdata/system/configs/emulationstation/es_input.cfg", "/usr/share/emulationstation/es_input.cfg"]
    for pad in pads:
        pad["profiles"] = []
        for path in databases:
            for node in tree(path).iter("inputConfig"):
                if node.get("deviceName") == pad["name"]:
                    pad["profiles"].append({"source": path, "guid": node.get("deviceGUID"),
                                            "controls": {x.get("name"): x.attrib for x in node.findall("input")}})

    settings = cfg("/userdata/system/knulli.conf")
    rows = []
    for node in tree("/usr/share/emulationstation/es_systems.cfg").iter("system"):
        name = node.findtext("name", "")
        romdir = node.findtext("path", "")
        extensions = set(node.findtext("extension", "").lower().split())
        if not romdir or not installed_roms(romdir, extensions):
            continue
        emulators = node.findall("./emulators/emulator")
        configured = settings.get(name + ".emulator", settings.get("global.emulator", "auto"))
        emu = next((e for e in emulators if e.get("name") == configured), None)
        if emu is None:
            emu = next((e for e in emulators if e.get("default") == "true"), emulators[0] if emulators else None)
        cores = list(emu.findall("./cores/core")) if emu is not None else []
        core = settings.get(name + ".core", "auto")
        if core == "auto":
            choice = next((c for c in cores if c.get("default") == "true"), cores[0] if cores else None)
            core = choice.text if choice is not None else None
        row = {"system": name, "emulator": None, "core": None}
        # XML lists available emulators, NOT board-specific defaults. Ask the
        # same read-only resolver as emulatorlauncher; never call generate().
        try:
            from configgen.Emulator import Emulator
            resolved = Emulator(name, str(Path(romdir) / "__snap_audit__.rom"))
            row["emulator"] = resolved.config.get("emulator")
            row["core"] = core = resolved.config.get("core")
        except Exception as error:
            row["resolver_status"] = "unavailable: " + type(error).__name__
            row["available_emulators"] = [e.get("name") for e in emulators]
        if row["emulator"] == "libretro" and core:
            row["core_installed"] = Path(f"/usr/lib/libretro/{core}_libretro.so").is_file()
            row["firmware"] = firmware(core, name)
        # Only report input-related override keys. Do not dump configs: even
        # Knulli's common.rmp can contain RetroAchievements credentials.
        row["input_overrides"] = {k: v for k, v in settings.items()
                                   if k.startswith((name + ".retroarch.input_", name + ".controller"))}
        rows.append(row)

    hotkeys = cfg("/userdata/system/snapos/config/retroarch-hotkeys.cfg")
    rules = Path("/etc/triggerhappy/triggers.d/multimedia_keys.conf")
    report = {"read_only": True, "pads": pads, "systems_with_content": rows,
              "global_input_overrides": {k: v for k, v in settings.items() if k.startswith("global.retroarch.input_")},
              "snap_menu_binding": {k: hotkeys.get(k) for k in ("input_menu_toggle_btn", "input_enable_hotkey_btn")},
              "stock_dpad_toggle_rules": [s for s in rules.read_text().splitlines() if not s.lstrip().startswith("#") and "dpad-toggle" in s] if rules.exists() else [],
              "notes": ["Missing optional BIOS is not proof that a game cannot run.",
                        "Core firmware metadata may list several machine models; not every BIOS applies to every ROM.",
                        "Profiles and files can be checked automatically; gameplay response still requires representative input tests."]}
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
