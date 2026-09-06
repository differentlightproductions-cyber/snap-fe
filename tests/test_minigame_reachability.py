"""Deterministic reachability checks for SNAP FE's level-based mini-games.

The audit reads the level declarations from the production sources and mirrors
their movement/collision rules.  This keeps the test useful when level data is
edited: no second handwritten copy of either game's boards is involved.

Run directly for a compact solution report::

    python tests/test_minigame_reachability.py

or run it as a normal unittest module.
"""

from __future__ import annotations

from collections import deque
from dataclasses import dataclass
from pathlib import Path
import re
import struct
import unittest


ROOT = Path(__file__).resolve().parents[1]
RUNNER_SOURCE = ROOT / "mini_games_extra.h"
BLOCK_SOURCE = ROOT / "main.c"

# STATE_MINIGAME is always considered animating by main.c.  These are the
# frame targets selected by frame_target_ms at normal, warm, power-save, and
# thermal-throttle settings.  Runner itself caps a longer update to 50 ms.
RUNNER_FRAME_MS = (16, 22, 33, 50)


def runner_float_constant(name: str) -> float:
    source = RUNNER_SOURCE.read_text(encoding="utf-8")
    match = re.search(rf"#define\s+{name}\s+(-?(?:\d+(?:\.\d*)?|\.\d+))f?", source)
    if match is None:
        raise AssertionError(f"could not find {name} in mini_games_extra.h")
    return float(match.group(1))


def runner_int_constant(name: str) -> int:
    source = RUNNER_SOURCE.read_text(encoding="utf-8")
    match = re.search(rf"#define\s+{name}\s+(\d+)u?", source)
    if match is None:
        raise AssertionError(f"could not find {name} in mini_games_extra.h")
    return int(match.group(1))


RUNNER_JUMP_VY = runner_float_constant("MGX_RUN_JUMP_VY")
RUNNER_GRAVITY = runner_float_constant("MGX_RUN_GRAVITY")
RUNNER_COYOTE_MS = runner_int_constant("MGX_RUN_COYOTE_MS")
RUNNER_BUFFER_MS = runner_int_constant("MGX_RUN_BUFFER_MS")


def f32(value: float) -> float:
    """Round like a C float after an assignment/expression."""

    return struct.unpack("<f", struct.pack("<f", value))[0]


@dataclass(frozen=True)
class RunnerObject:
    x: int
    width: int
    height: int
    kind: int  # 0 spike, 1 block/platform, 2 ground gap


@dataclass(frozen=True)
class RunnerLevel:
    length: int
    speed: int
    objects: tuple[RunnerObject, ...]
    name: str


def load_runner_levels() -> tuple[RunnerLevel, ...]:
    source = RUNNER_SOURCE.read_text(encoding="utf-8")
    object_sets: dict[int, tuple[RunnerObject, ...]] = {}
    array_re = re.compile(
        r"static\s+const\s+MgxRunObj\s+mgx_rl(\d+)\[\]\s*=\s*\{(.*?)\};",
        re.DOTALL,
    )
    object_re = re.compile(r"R(SP|BL|GP)\(([^)]*)\)")
    for array_match in array_re.finditer(source):
        index = int(array_match.group(1))
        objects: list[RunnerObject] = []
        for object_match in object_re.finditer(array_match.group(2)):
            macro = object_match.group(1)
            values = [int(part.strip()) for part in object_match.group(2).split(",")]
            if macro == "SP":
                objects.append(RunnerObject(values[0], 30, 32, 0))
            elif macro == "BL":
                objects.append(RunnerObject(values[0], values[1], values[2], 1))
            else:
                objects.append(RunnerObject(values[0], values[1], 0, 2))
        object_sets[index] = tuple(objects)

    table_match = re.search(
        r"static\s+const\s+MgxRunLevel\s+mgx_run_levels\[10\]\s*=\s*\{(.*?)\};",
        source,
        re.DOTALL,
    )
    if table_match is None:
        raise AssertionError("could not find mgx_run_levels in mini_games_extra.h")
    entry_re = re.compile(
        r"\{\s*(\d+)\s*,\s*(\d+)\s*,\s*MGX_ARRN\(mgx_rl(\d+)\)\s*,"
        r"\s*mgx_rl\d+\s*,\s*\"([^\"]+)\"\s*\}"
    )
    levels = tuple(
        RunnerLevel(int(length)*3, int(speed), tuple(RunnerObject(o.x+section*int(length),o.width,o.height,o.kind) for section in range(3) for o in object_sets[int(index)]), name)
        for length, speed, index, name in entry_re.findall(table_match.group(1))
    )
    if len(levels) != 10:
        raise AssertionError(f"expected 10 runner levels, parsed {len(levels)}")
    return levels


def runner_in_gap(level: RunnerLevel, world_x: float) -> bool:
    return any(
        obj.kind == 2 and world_x >= obj.x and world_x <= obj.x + obj.width
        for obj in level.objects
    )


def hit(
    ax: float,
    ay: float,
    aw: float,
    ah: float,
    bx: float,
    by: float,
    bw: float,
    bh: float,
) -> bool:
    return ax < bx + bw and ax + aw > bx and ay < by + bh and ay + ah > by


@dataclass(frozen=True)
class RunnerState:
    y: float
    vy: float
    on_floor: bool
    air_ms: int
    jumps: tuple[int, ...]


def runner_frame(
    level: RunnerLevel,
    state: RunnerState,
    scroll: float,
    frame_ms: int,
    jump: bool,
) -> tuple[RunnerState | None, float, bool]:
    """Mirror one mgx_runner_step call; return (state, scroll, won)."""

    y, vy, on_floor, air_ms = state.y, state.vy, state.on_floor, state.air_ms
    jumps = state.jumps
    if jump and (on_floor or air_ms <= RUNNER_COYOTE_MS):
        vy = f32(RUNNER_JUMP_VY)
        on_floor = False
        # Production invalidates the current coyote window when a buffered
        # press becomes a jump, preventing a second jump in mid-air.
        air_ms = RUNNER_COYOTE_MS + 1
        jumps += (round(f32(scroll + 120.0)),)

    t = f32(f32(float(frame_ms)) / f32(16.6667))
    old_y = y
    scroll = f32(scroll + f32(f32(float(level.speed)) * t))
    vy = f32(vy + f32(f32(RUNNER_GRAVITY) * t))
    y = f32(y + f32(vy * t))
    world_x = f32(scroll + 120.0)
    player_bottom = f32(y + 24.0)
    on_floor = False
    floor_y = f32(397.0)

    # The slightly unusual old-position threshold is copied intentionally.
    landing_threshold = f32(floor_y + f32(vy * t) + 4.0)
    if (
        not runner_in_gap(level, world_x)
        and player_bottom >= floor_y
        and f32(old_y + 24.0) <= landing_threshold
        and vy >= 0.0
    ):
        y = f32(floor_y - 24.0)
        vy = 0.0
        on_floor = True

    for obj in level.objects:
        if obj.kind == 2:
            continue
        obstacle_x = f32(float(obj.x) - scroll)
        obstacle_y = f32(floor_y - float(obj.height))
        if (
            obj.kind == 1
            and vy >= 0.0
            and 142.0 > obstacle_x
            and 120.0 < obstacle_x + obj.width
            and old_y + 24.0 <= obstacle_y + 4.0
            and y + 24.0 >= obstacle_y
        ):
            y = f32(obstacle_y - 24.0)
            vy = 0.0
            on_floor = True
            continue
        inset_x = 5.0 if obj.kind == 0 else 0.0
        inset_y = 8.0 if obj.kind == 0 else 0.0
        shrink_w = 10.0 if obj.kind == 0 else 0.0
        shrink_h = 8.0 if obj.kind == 0 else 0.0
        if hit(
            123.0,
            f32(y + 3.0),
            18.0,
            18.0,
            f32(obstacle_x + inset_x),
            f32(obstacle_y + inset_y),
            f32(float(obj.width) - shrink_w),
            f32(float(obj.height) - shrink_h),
        ):
            return None, scroll, False

    if y > 510.0:  # WIN_H (480) + 30 in the shipped handheld layouts
        return None, scroll, False
    air_ms = 0 if on_floor else air_ms + frame_ms
    won = world_x >= level.length
    return RunnerState(y, vy, on_floor, air_ms, jumps), scroll, won


def solve_runner(level: RunnerLevel, frame_ms: int) -> RunnerState | None:
    """Breadth-first search over every legal jump/no-jump decision."""

    states = [RunnerState(f32(373.0), 0.0, True, 0, ())]
    scroll = f32(0.0)
    frame = 0
    # Starting the stage is separate from the first takeoff, so the player can
    # choose the correct rhythm for each opening obstacle.
    force_first_jump = False
    max_frames = int((level.length + 600) / max(level.speed * frame_ms / 16.6667, 0.1)) + 20
    while states and frame < max_frames:
        next_by_motion: dict[tuple[float, float, bool], RunnerState] = {}
        next_scroll: float | None = None
        for state in states:
            jump_ready = state.on_floor or state.air_ms <= RUNNER_COYOTE_MS
            decisions = (True,) if force_first_jump else ((False, True) if jump_ready else (False,))
            for jump in decisions:
                result, advanced_scroll, won = runner_frame(
                    level, state, scroll, frame_ms, jump
                )
                next_scroll = advanced_scroll
                if result is None:
                    continue
                if won:
                    return result
                # Values originate in binary32; exact values are safe keys.  The
                # rounded form only avoids signed-zero distinctions in reports.
                key = (round(result.y, 6), round(result.vy, 6), result.on_floor, result.air_ms)
                prior = next_by_motion.get(key)
                if prior is None or len(result.jumps) < len(prior.jumps):
                    next_by_motion[key] = result
        if next_scroll is None:
            return None
        states = list(next_by_motion.values())
        scroll = next_scroll
        force_first_jump = False
        frame += 1
    if __import__('os').environ.get('SNAP_TEST_TRACE'):
        print(level.name, frame_ms, 'last reachable x', round(scroll+120))
    return None


@dataclass(frozen=True)
class BlockLevel:
    rows: tuple[str, ...]
    start: tuple[int, int]
    goal: tuple[int, int]


def load_block_levels() -> tuple[BlockLevel, ...]:
    source = BLOCK_SOURCE.read_text(encoding="utf-8")
    table_match = re.search(
        r"static\s+const\s+BlockLevel\s+blk_levels\[BLK_LEVEL_COUNT\]\s*=\s*\{"
        r"(.*?)\n\};",
        source,
        re.DOTALL,
    )
    if table_match is None:
        raise AssertionError("could not find blk_levels in main.c")
    entry_re = re.compile(
        r"\{\{(.*?)\}\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*\}",
        re.DOTALL,
    )
    levels: list[BlockLevel] = []
    for rows_text, sx, sy, gx, gy in entry_re.findall(table_match.group(1)):
        rows = tuple(re.findall(r'\"([^\"]*)\"', rows_text))
        levels.append(BlockLevel(rows, (int(sx), int(sy)), (int(gx), int(gy))))
    expected = int(re.search(r"#define BLK_LEVEL_COUNT (\d+)", source)[1])
    if len(levels) != expected:
        raise AssertionError(f"expected {expected} Block Roll levels, parsed {len(levels)}")
    return tuple(levels)


def block_pose_ok(level: BlockLevel, x: int, y: int, orientation: int) -> bool:
    def cell_ok(cx: int, cy: int) -> bool:
        return (
            0 <= cy < len(level.rows)
            and 0 <= cx < len(level.rows[cy])
            and level.rows[cy][cx] == "#"
        )

    if not cell_ok(x, y):
        return False
    if orientation == 1:
        return cell_ok(x + 1, y)
    if orientation == 2:
        return cell_ok(x, y + 1)
    return True


def block_move(pose: tuple[int, int, int], key: str) -> tuple[int, int, int]:
    x, y, orientation = pose
    nx, ny, new_orientation = x, y, orientation
    if orientation == 0:
        if key == "L":
            nx -= 2
            new_orientation = 1
        elif key == "R":
            nx += 1
            new_orientation = 1
        elif key == "U":
            ny -= 2
            new_orientation = 2
        else:
            ny += 1
            new_orientation = 2
    elif orientation == 1:
        if key == "L":
            nx -= 1
            new_orientation = 0
        elif key == "R":
            nx += 2
            new_orientation = 0
        elif key == "U":
            ny -= 1
        else:
            ny += 1
    else:
        if key == "U":
            ny -= 1
            new_orientation = 0
        elif key == "D":
            ny += 2
            new_orientation = 0
        elif key == "L":
            nx -= 1
        else:
            nx += 1
    return nx, ny, new_orientation


def solve_block(level: BlockLevel) -> str | None:
    start = (level.start[0], level.start[1], 0)
    queue = deque([(start, "")])
    visited = {start}
    while queue:
        pose, path = queue.popleft()
        for key in "UDLR":
            nxt = block_move(pose, key)
            if not block_pose_ok(level, *nxt) or nxt in visited:
                continue
            next_path = path + key
            if nxt[2] == 0 and nxt[:2] == level.goal:
                return next_path
            visited.add(nxt)
            queue.append((nxt, next_path))
    return None


class MiniGameReachabilityTests(unittest.TestCase):
    def test_runner_accessibility_windows_cover_slow_frames(self) -> None:
        self.assertGreaterEqual(RUNNER_COYOTE_MS, 2 * max(RUNNER_FRAME_MS))
        self.assertGreaterEqual(RUNNER_BUFFER_MS, 2 * max(RUNNER_FRAME_MS))

    def test_stage_four_is_the_forgiving_staircase(self) -> None:
        stage = load_runner_levels()[3]
        self.assertLessEqual(stage.speed, 4)
        self.assertLessEqual(max(obj.height for obj in stage.objects), 66)
        self.assertLessEqual(max(obj.width for obj in stage.objects if obj.kind == 2), 105)

    def test_all_runner_levels_are_reachable_at_supported_frame_caps(self) -> None:
        levels = load_runner_levels()
        failures: list[str] = []
        for frame_ms in RUNNER_FRAME_MS:
            for number, level in enumerate(levels, 1):
                result = solve_runner(level, frame_ms)
                if result is None:
                    failures.append(f"stage {number} {level.name!r} at {frame_ms} ms")
        self.assertFalse(failures, "unreachable Runner configurations: " + ", ".join(failures))

    def test_all_block_roll_levels_are_solvable(self) -> None:
        levels = load_block_levels()
        failures = [
            f"level {number}"
            for number, level in enumerate(levels, 1)
            if solve_block(level) is None
        ]
        self.assertFalse(failures, "unsolvable Block Roll boards: " + ", ".join(failures))


def print_report() -> bool:
    all_ok = True
    print("Pulse Runner")
    for frame_ms in RUNNER_FRAME_MS:
        print(f"  frame interval {frame_ms} ms")
        for number, level in enumerate(load_runner_levels(), 1):
            solution = solve_runner(level, frame_ms)
            if solution is None:
                all_ok = False
                print(f"    {number:2d}. {level.name:<14} UNREACHABLE")
            else:
                jump_list = ",".join(map(str, solution.jumps))
                print(
                    f"    {number:2d}. {level.name:<14} reachable; "
                    f"{len(solution.jumps):2d} jumps at world x [{jump_list}]"
                )
    print("Block Roll")
    for number, level in enumerate(load_block_levels(), 1):
        solution = solve_block(level)
        if solution is None:
            all_ok = False
            print(f"    {number:2d}. UNREACHABLE")
        else:
            print(f"    {number:2d}. reachable in {len(solution):2d} moves: {solution}")
    return all_ok


if __name__ == "__main__":
    raise SystemExit(0 if print_report() else 1)
