#!/usr/bin/env python3
import argparse
import json
import re
import sys
import time
import urllib.parse
import urllib.request
from datetime import datetime


def fetch_json(url, timeout=10, attempts=2):
    last_error = None
    for attempt in range(attempts):
        try:
            req = urllib.request.Request(url, headers={"User-Agent": "SnapFE/1.3.2 weather"})
            with urllib.request.urlopen(req, timeout=timeout) as response:
                return json.loads(response.read().decode("utf-8"))
        except Exception as exc:
            last_error = exc
            if attempt + 1 < attempts:
                time.sleep(0.25)
    raise last_error


def code_text(code, is_day=True):
    try:
        code = int(code)
    except Exception:
        return "Current conditions"
    table = {
        0: "Clear" if is_day else "Clear Night",
        1: "Mostly Clear",
        2: "Partly Cloudy",
        3: "Cloudy",
        45: "Fog",
        48: "Freezing Fog",
        51: "Light Drizzle",
        53: "Drizzle",
        55: "Heavy Drizzle",
        56: "Freezing Drizzle",
        57: "Freezing Drizzle",
        61: "Light Rain",
        63: "Rain",
        65: "Heavy Rain",
        66: "Freezing Rain",
        67: "Freezing Rain",
        71: "Light Snow",
        73: "Snow",
        75: "Heavy Snow",
        77: "Snow Grains",
        80: "Rain Showers",
        81: "Rain Showers",
        82: "Heavy Showers",
        85: "Snow Showers",
        86: "Heavy Snow Showers",
        95: "Thunderstorm",
        96: "Thunderstorm",
        99: "Severe Thunderstorm",
    }
    return table.get(code, "Current conditions")


def safe_text(value, limit=160):
    text = str(value or "").replace("\n", " ").replace("\r", " ").replace("|", " ")
    text = text.replace("<", "").replace(">", "").replace("{", "").replace("}", "")
    return text[:limit].strip()


def fmt_temp(value, unit):
    try:
        return f"{round(float(value))}{unit.upper()}"
    except Exception:
        return f"--{unit.upper()}"


def fmt_pct(value):
    try:
        return f"{round(float(value))}%"
    except Exception:
        return "--%"


def fmt_wind(value, unit):
    try:
        return f"{round(float(value))} {unit}"
    except Exception:
        return f"-- {unit}"


def parse_time_label(value, now_label=False):
    if now_label:
        return "Now"
    try:
        dt = datetime.fromisoformat(value)
        label = dt.strftime("%I %p")
        return label[1:] if label.startswith("0") else label
    except Exception:
        return safe_text(value, 8)


def parse_day_label(value):
    try:
        return datetime.fromisoformat(value).strftime("%a")
    except Exception:
        return safe_text(value, 8)


def parse_clock(value):
    try:
        return datetime.fromisoformat(value).strftime("%H:%M")
    except Exception:
        return ""


def locate_by_ip():
    data = fetch_json("http://ip-api.com/json/?fields=status,city,regionName,country,lat,lon,timezone", timeout=5)
    if data.get("status") != "success":
        raise RuntimeError("IP location unavailable")
    return {
        "lat": float(data["lat"]),
        "lon": float(data["lon"]),
        "name": safe_text(", ".join(x for x in [data.get("city"), data.get("regionName")] if x), 96) or "Local",
        "timezone": safe_text(data.get("timezone"), 64),
    }


def location_query_candidates(query):
    candidates = []

    def add(value):
        value = " ".join(value.strip(" ,").split())
        known = {item.casefold() for item in candidates}
        if value and value.casefold() not in known:
            candidates.append(value)

    add(query)
    without_notes = re.sub(r"\s*\([^)]*\)", "", query).strip(" ,")
    add(without_notes)
    # Older releases accepted free-form entries such as "Prescott Arizona".
    # Open-Meteo expects an administrative qualifier after a comma, so try the
    # final one to three words as that qualifier before using the city alone.
    if "," not in without_notes:
        words = without_notes.split()
        for tail_words in range(1, min(3, len(words) - 1) + 1):
            split = len(words) - tail_words
            add(f"{' '.join(words[:split])}, {' '.join(words[split:])}")
    add(query.split(",", 1)[0])
    return candidates


def locate_by_query(query):
    last_error = None
    for candidate in location_query_candidates(query):
        q = urllib.parse.urlencode({"name": candidate, "count": 1, "language": "en", "format": "json"})
        try:
            data = fetch_json(f"https://geocoding-api.open-meteo.com/v1/search?{q}")
        except Exception as exc:
            last_error = exc
            continue
        results = data.get("results") or []
        if not results:
            continue
        loc = results[0]
        admin = loc.get("admin1") or loc.get("country") or ""
        return {
            "lat": float(loc["latitude"]),
            "lon": float(loc["longitude"]),
            "name": safe_text(", ".join(x for x in [loc.get("name"), admin] if x), 96),
            "timezone": safe_text(loc.get("timezone"), 64),
        }
    if last_error:
        raise RuntimeError(f"Location lookup unavailable: {last_error}")
    raise RuntimeError("Location not found")


def build_alert(current_code, wind, precip):
    try:
        code = int(current_code)
    except Exception:
        code = 0
    try:
        wind = float(wind)
    except Exception:
        wind = 0
    try:
        precip = float(precip)
    except Exception:
        precip = 0
    if code in (95, 96, 99) or wind >= 40:
        return 2, "Emergency weather: storm or damaging wind risk nearby"
    if code in (65, 67, 75, 82, 85, 86) or wind >= 25 or precip >= 70:
        return 1, "Weather alert: conditions may affect travel or outdoor plans"
    return 0, ""


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--query", default="")
    parser.add_argument("--unit", choices=("f", "c"), default="f")
    parser.add_argument("--auto-ip", type=int, default=1)
    args = parser.parse_args()

    query = args.query.strip()
    if query:
        loc = locate_by_query(query)
    elif args.auto_ip:
        loc = locate_by_ip()
    else:
        raise RuntimeError("Choose a location")

    temp_unit = "fahrenheit" if args.unit == "f" else "celsius"
    wind_unit = "mph" if args.unit == "f" else "kmh"
    params = {
        "latitude": loc["lat"],
        "longitude": loc["lon"],
        "current": "temperature_2m,apparent_temperature,is_day,precipitation,weather_code,wind_speed_10m,relative_humidity_2m",
        "hourly": "temperature_2m,precipitation_probability,weather_code",
        "daily": "weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max,sunrise,sunset",
        "temperature_unit": temp_unit,
        "wind_speed_unit": wind_unit,
        "forecast_days": 16,
        "timezone": "auto",
    }
    url = "https://api.open-meteo.com/v1/forecast?" + urllib.parse.urlencode(params)
    data = fetch_json(url, timeout=12)

    current = data.get("current") or {}
    daily = data.get("daily") or {}
    hourly = data.get("hourly") or {}
    unit = "f" if args.unit == "f" else "c"
    temp = fmt_temp(current.get("temperature_2m"), unit)
    feels = fmt_temp(current.get("apparent_temperature"), unit)
    is_day = bool(current.get("is_day", 1))
    condition = code_text(current.get("weather_code"), is_day)
    high = fmt_temp((daily.get("temperature_2m_max") or [None])[0], unit)
    low = fmt_temp((daily.get("temperature_2m_min") or [None])[0], unit)
    precip = fmt_pct((daily.get("precipitation_probability_max") or [None])[0])
    wind = fmt_wind(current.get("wind_speed_10m"), wind_unit)
    humidity = fmt_pct(current.get("relative_humidity_2m"))

    times = hourly.get("time") or []
    htemps = hourly.get("temperature_2m") or []
    hpops = hourly.get("precipitation_probability") or []
    hcodes = hourly.get("weather_code") or []
    start = 0
    now_iso = current.get("time")
    if now_iso:
        try:
            current_time = datetime.fromisoformat(now_iso)
            for i, value in enumerate(times):
                if datetime.fromisoformat(value) <= current_time:
                    start = i
                else:
                    break
        except (TypeError, ValueError):
            if now_iso in times:
                start = times.index(now_iso)
    hourly_rows = []
    # Through the end of the day where the provider has it: the frontend shows
    # the first few in its compact strip and the whole run in its hourly screen.
    for i in range(start, min(start + 18, len(times))):
        label = parse_time_label(times[i], i == start)
        row = f"{label} {fmt_temp(htemps[i] if i < len(htemps) else None, unit)} {code_text(hcodes[i] if i < len(hcodes) else 0)} {fmt_pct(hpops[i] if i < len(hpops) else None)}"
        hourly_rows.append(safe_text(row, 63))

    days = daily.get("time") or []
    dmax = daily.get("temperature_2m_max") or []
    dmin = daily.get("temperature_2m_min") or []
    dpops = daily.get("precipitation_probability_max") or []
    dcodes = daily.get("weather_code") or []
    daily_rows = []
    daily_dates = []
    for i in range(min(16, len(days))):
        row = f"{parse_day_label(days[i])} {fmt_temp(dmax[i] if i < len(dmax) else None, unit)}/{fmt_temp(dmin[i] if i < len(dmin) else None, unit)} {code_text(dcodes[i] if i < len(dcodes) else 0)} {fmt_pct(dpops[i] if i < len(dpops) else None)}"
        daily_rows.append(safe_text(row, 63))
        # The date behind each row, so the frontend can label a week average
        # with the days it actually covers rather than "Wk1".
        daily_dates.append(safe_text(str(days[i])[:10], 10))

    alert_level, alert_text = build_alert(
        current.get("weather_code"),
        current.get("wind_speed_10m"),
        (daily.get("precipitation_probability_max") or [0])[0],
    )
    sunrise = parse_clock((daily.get("sunrise") or [""])[0])
    sunset = parse_clock((daily.get("sunset") or [""])[0])
    updated = safe_text((current.get("time") or "").replace("T", " "), 64)

    lines = [
        "SNAPWEATHER 1",
        f"summary={temp} {condition}",
        f"current={temp}",
        f"condition={condition}",
        f"feels={feels}",
        f"high={high}",
        f"low={low}",
        f"precip={precip}",
        f"wind={wind}",
        f"humidity={humidity}",
        f"place={loc['name']}",
        f"updated={updated}",
        f"alert_level={alert_level}",
        f"alert={safe_text(alert_text, 170)}",
        f"hourly={';'.join(hourly_rows)}",
        f"daily={';'.join(daily_rows)}",
        f"daily_dates={';'.join(daily_dates)}",
        f"sunrise={sunrise}",
        f"sunset={sunset}",
    ]
    sys.stdout.write("\n".join(lines) + "\n")


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        sys.stderr.write(f"weather_service: {exc}\n")
        sys.exit(1)
