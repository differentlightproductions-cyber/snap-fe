#!/usr/bin/env python3
"""Offline regression checks for the RetroAchievements book cache helper."""

import os
import tempfile

import ra_achievements as ra


def main():
    rows = [
        {
            "Date": "2025-01-01 10:00:00",
            "HardcoreMode": 0,
            "AchievementID": 7,
            "Title": "First\tWin",
            "Description": "Clear\nlevel one",
            "Points": 5,
            "GameTitle": "Demo Game",
            "ConsoleName": "Game Boy Advance",
        },
        {
            "Date": "2025-01-02 10:00:00",
            "HardcoreMode": 1,
            "AchievementID": 7,
            "Title": "First Win",
            "Description": "Clear level one",
            "Points": 5,
            "GameTitle": "Demo Game",
            "ConsoleName": "Game Boy Advance",
        },
        {
            "Date": "2024-12-01 08:00:00",
            "HardcoreMode": 0,
            "AchievementID": 8,
            "Title": "Second Win",
            "Description": "Clear level two",
            "Points": 10,
            "GameTitle": "Demo Game",
            "ConsoleName": "Game Boy Advance",
        },
    ]
    result = ra.normalize(rows)
    assert len(result) == 2
    assert result[0][2] == "7" and result[0][1] == "1"
    assert all("\t" not in value and "\n" not in value for row in result for value in row)

    with tempfile.TemporaryDirectory() as temp:
        target = os.path.join(temp, "cache", "status.txt")
        ra.status(target, "done", "Loaded 2 achievements")
        with open(target, encoding="utf-8") as handle:
            assert handle.read() == "done|Loaded 2 achievements\n"
        assert not os.path.exists(target + ".part")

    print("RetroAchievements helper tests: PASS")


if __name__ == "__main__":
    main()
