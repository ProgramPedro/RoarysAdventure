#!/usr/bin/env python3
"""
check_platformer_reach.py - can Floor 1 actually be finished?

Reads the layout straight out of src/scenes/PlatformerLevel.h and the jump
tuning out of src/core/Config.h, works out how far a jump really goes, and
then checks three things:

  1. every jump the level design intends is physically possible
  2. the goal is reachable from the spawn, and BOTH branches of BOTH forks
     reach their meeting point
  3. every coin sits somewhere a player can actually touch it

Run it after editing PlatformerLevel.h. It needs nothing but Python 3.

    python3 tools/check_platformer_reach.py
"""

import math
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LEVEL = os.path.join(ROOT, "src", "scenes", "PlatformerLevel.h")
CONFIG = os.path.join(ROOT, "src", "core", "Config.h")

NUM = r"(-?[0-9]*\.?[0-9]+)f?"


# --------------------------------------------------------------------------
# reading the C++ tables
# --------------------------------------------------------------------------
def read(path):
    with open(path) as handle:
        return handle.read()


def block(text, name):
    """The text between `name[] = {` and the matching `};`."""
    start = text.index(name + "[] = {") + len(name + "[] = {")
    end = text.index("};", start)
    return text[start:end]


def constant(text, name):
    match = re.search(r"\b" + name + r"\s*=\s*" + NUM, text)
    if not match:
        raise SystemExit("could not find constant " + name)
    return float(match.group(1))


def vec3s(chunk):
    """Every `{a, b, c}` triple in order."""
    pattern = r"\{\s*" + NUM + r"\s*,\s*" + NUM + r"\s*,\s*" + NUM + r"\s*\}"
    return [tuple(float(v) for v in m) for m in re.findall(pattern, chunk)]


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


# --------------------------------------------------------------------------
# boxes
# --------------------------------------------------------------------------
class Box:
    def __init__(self, name, center, size):
        self.name = name
        self.center = center
        self.size = size

    @property
    def top(self):
        return self.center[1] + self.size[1] * 0.5

    def span(self, axis):
        return (self.center[axis] - self.size[axis] * 0.5,
                self.center[axis] + self.size[axis] * 0.5)

    def gap_to(self, other):
        """Edge-to-edge horizontal distance, 0 if the footprints overlap."""
        total = 0.0
        for axis in (0, 2):
            a0, a1 = self.span(axis)
            b0, b1 = other.span(axis)
            gap = max(b0 - a1, a0 - b1, 0.0)
            total += gap * gap
        return math.sqrt(total)

    def __repr__(self):
        return self.name


# --------------------------------------------------------------------------
# the jump envelope
# --------------------------------------------------------------------------
class Jump:
    """How far you travel from a launch of a given upward speed."""

    def __init__(self, rise, gravity_up, gravity_down, run_speed):
        self.rise = rise
        self.gravity_up = gravity_up
        self.gravity_down = gravity_down
        self.run_speed = run_speed
        self.time_to_apex = math.sqrt(2.0 * rise / gravity_up)

    def arc_height(self, distance):
        """
        How high above the launch point you are after travelling `distance`
        horizontally at full run speed. Rising under one gravity and falling
        under a heavier one, exactly as the game does it.
        """
        time = distance / self.run_speed
        if time <= self.time_to_apex:
            return (self.run_speed * 0.0
                    + math.sqrt(2.0 * self.rise * self.gravity_up) * time
                    - 0.5 * self.gravity_up * time * time)
        over = time - self.time_to_apex
        return self.rise - 0.5 * self.gravity_down * over * over

    def horizontal_reach(self, dy):
        """
        The furthest you can get while ending up `dy` above where you started.

        The answer is set by the LATEST moment you are at that height, which
        is on the way back down - not the first time you cross it. Measuring
        it at the first crossing is the mistake that made an earlier version
        of this script declare a perfectly good level unreachable.
        """
        if dy > self.rise:
            return None                     # cannot get that high at all
        drop = self.rise - dy               # from the apex down to the target
        time = self.time_to_apex + math.sqrt(2.0 * drop / self.gravity_down)
        return self.run_speed * time


def main():
    level = strip_comments(read(LEVEL))
    config = strip_comments(read(CONFIG))

    jump_height = constant(config, "kJumpHeight")
    time_to_apex = constant(config, "kTimeToApex")
    fall_mult = constant(config, "kFallGravityMultiplier")
    run_speed = constant(config, "kMaxRunSpeed")
    pad_speed = constant(config, "kBouncePadSpeed")
    coin_radius = constant(config, "kCoinRadius")
    player = vec3s(re.search(r"kPlayerSize\s*\{[^}]*\}", config).group(0))[0]

    gravity_up = 2.0 * jump_height / (time_to_apex * time_to_apex)
    gravity_down = gravity_up * fall_mult
    pad_rise = (pad_speed * pad_speed) / (2.0 * gravity_up)

    jump = Jump(jump_height, gravity_up, gravity_down, run_speed)
    pad = Jump(pad_rise, gravity_up, gravity_down, run_speed)

    print("Floor 1 reachability")
    print("=" * 66)
    print("  jump rise          %.2f units  (apex in %.2fs)" % (jump_height, time_to_apex))
    print("  bounce pad rise    %.2f units" % pad_rise)
    print("  run speed          %.2f units/s" % run_speed)
    print("  flat jump reach    %.2f units edge-to-edge" % jump.horizontal_reach(0.0))
    print("  flat pad reach     %.2f units edge-to-edge" % pad.horizontal_reach(0.0))
    print()

    # --- the tables ------------------------------------------------------
    names = ["G0", "G1", "G2",
             "A1", "A2", "A3", "A4", "A5", "A6",
             "B1", "B2", "B3",
             "M",
             "C1", "C2", "C3",
             "D1", "D2", "D3", "D4",
             "S1", "S2", "S3"]

    raw = vec3s(block(level, "kPlatforms"))
    pairs = list(zip(raw[0::2], raw[1::2]))
    if len(pairs) != len(names):
        raise SystemExit("expected %d platforms, found %d - update `names`"
                         % (len(names), len(pairs)))

    boxes = {}
    for name, (center, size) in zip(names, pairs):
        boxes[name] = Box(name, center, size)

    moving = vec3s(block(level, "kMovingPlatforms"))
    for index in range(0, len(moving), 3):
        start, finish, size = moving[index], moving[index + 1], moving[index + 2]
        tag = "MP%d" % (index // 3)
        boxes[tag + "@from"] = Box(tag + "@from", start, size)
        boxes[tag + "@to"] = Box(tag + "@to", finish, size)

    pads_raw = vec3s(block(level, "kBouncePads"))
    pads = {}
    for index in range(0, len(pads_raw), 2):
        tag = "P%d" % (index // 2)
        pads[tag] = Box(tag, pads_raw[index], pads_raw[index + 1])
        boxes[tag] = pads[tag]

    coins = vec3s(block(level, "kCoins"))
    spawn = vec3s(re.search(r"kSpawnPoint\s*\{[^}]*\}", level).group(0))[0]
    goal = vec3s(re.search(r"kGoalPoint\s*\{[^}]*\}", level).group(0))[0]

    failures = []

    # --- 1. the intended route graph -------------------------------------
    # (from, to, mechanism). This mirrors the diagram at the top of
    # PlatformerLevel.h, so if the level is redesigned this list changes too.
    edges = [
        ("G0", "G1", "jump", "pit 1"),
        ("G1", "G2", "jump", "pit 2"),
        ("G0", "P0", "jump", "step onto the tutorial pad"),

        ("G2", "A1", "jump", "FORK 1 -> safe"),
        ("A1", "A2", "jump", "safe stair"),
        ("A2", "A3", "jump", "safe stair"),
        ("A3", "A4", "jump", "safe stair"),
        ("A4", "A5", "jump", "safe stair"),
        ("A5", "A6", "jump", "safe stair"),
        ("A6", "M", "jump", "safe stair -> midway"),

        ("G2", "P1", "jump", "FORK 1 -> risky, step onto pad"),
        ("P1", "B1", "pad", "pad launch"),
        ("B1", "B2", "jump", "risky ledges"),
        ("B2", "B3", "jump", "risky ledges"),
        ("B3", "M", "jump", "risky ledges -> midway"),

        ("M", "C1", "jump", "FORK 2 -> safe"),
        ("C1", "C2", "jump", "wide path"),
        ("C2", "C3", "jump", "wide path"),
        ("C3", "MP0@from", "jump", "wide path -> moving platform"),
        ("MP0@to", "S1", "jump", "moving platform -> summit"),

        ("M", "D1", "jump", "FORK 2 -> risky"),
        ("D1", "D2", "jump", "narrow leap"),
        ("D2", "D3", "jump", "narrow leap"),
        ("D3", "D4", "jump", "narrow leap"),
        ("D4", "S1", "jump", "narrow leap -> summit"),

        ("S1", "S2", "jump", "summit"),
        ("S2", "S3", "jump", "summit, goal is here"),
    ]

    print("Intended jumps")
    print("-" * 66)
    print("%-16s %6s %6s %7s  %s" % ("jump", "rise", "gap", "reach", "note"))
    for source, target, how, note in edges:
        a, b = boxes[source], boxes[target]
        dy = b.top - a.top
        gap = a.gap_to(b)
        envelope = (pad if how == "pad" else jump).horizontal_reach(dy)

        if envelope is None:
            mark = "TOO HIGH"
            failures.append("%s -> %s needs %.2f of rise, a %s gives %.2f"
                            % (source, target, dy, how,
                               pad.rise if how == "pad" else jump.rise))
        elif gap > envelope:
            mark = "TOO FAR"
            failures.append("%s -> %s is a %.2f gap, reach at that height is %.2f"
                            % (source, target, gap, envelope))
        else:
            headroom = envelope - gap
            mark = "ok" if headroom > 0.35 else "ok (tight)"

        print("%-16s %6.2f %6.2f %7s  %s%s"
              % ("%s->%s" % (source, target), dy, gap,
                 "-" if envelope is None else "%.2f" % envelope,
                 mark.ljust(11), note))
    print()

    # --- 2. reachability from the spawn ----------------------------------
    # A move is legal if a jump (or a pad, from a pad) can make it, or if it
    # is simply a drop onto something lower that is close enough horizontally.
    def legal(a, b):
        dy = b.top - a.top
        gap = a.gap_to(b)
        source = pad if a.name.startswith("P") else jump
        envelope = source.horizontal_reach(dy)
        if envelope is not None and gap <= envelope:
            return True
        # falling onto something below: you keep your run speed on the way down
        if dy < 0.0:
            fall_time = math.sqrt(2.0 * -dy / gravity_down)
            return gap <= run_speed * fall_time
        return False

    # the moving platform carries you between its own endpoints
    carries = {"MP0@from": "MP0@to", "MP0@to": "MP0@from"}

    start = None
    for box in boxes.values():
        if (box.span(0)[0] - 0.5 <= spawn[0] <= box.span(0)[1] + 0.5
                and abs(box.top - (spawn[1] - player[1] * 0.5)) < 0.6):
            start = box.name
            break
    if start is None:
        raise SystemExit("could not work out which platform the spawn is on")

    seen = {start}
    queue = [start]
    while queue:
        here = boxes[queue.pop()]
        for other in boxes.values():
            if other.name in seen:
                continue
            if legal(here, other) or carries.get(here.name) == other.name:
                seen.add(other.name)
                queue.append(other.name)

    print("Reachability from the spawn (on %s)" % start)
    print("-" * 66)
    unreachable = [n for n in boxes if n not in seen]
    for name in sorted(unreachable):
        failures.append("%s cannot be reached from the spawn" % name)
    print("  reached %d of %d surfaces" % (len(seen), len(boxes)))
    if unreachable:
        print("  NEVER REACHED: " + ", ".join(sorted(unreachable)))

    # both branches must independently get to their meeting point
    for fork, safe, risky, meet in (("fork 1", "A6", "B3", "M"),
                                    ("fork 2", "MP0@to", "D4", "S1")):
        for branch, label in ((safe, "safe"), (risky, "risky")):
            ok = legal(boxes[branch], boxes[meet])
            print("  %s %-6s branch %-9s -> %-3s   %s"
                  % (fork, label, branch, meet, "ok" if ok else "BROKEN"))
            if not ok:
                failures.append("%s %s branch cannot reach %s" % (fork, label, meet))

    # the goal has to sit on a surface you can stand on
    on_goal = [b.name for b in boxes.values()
               if abs(b.top - (goal[1] - 0.35)) < 0.45
               and b.span(0)[0] <= goal[0] <= b.span(0)[1]]
    print("  goal stands on: %s" % (", ".join(on_goal) if on_goal else "NOTHING"))
    if not on_goal:
        failures.append("the goal is not sitting on any platform")
    elif not any(name in seen for name in on_goal):
        failures.append("the goal's platform is unreachable")
    print()

    # --- 2b. the branches must stay separate -----------------------------
    # The whole point of a fork is that picking one branch costs you what the
    # other one pays. If a safe platform is within jumping range of a risky
    # platform that carries a coin, the choice is free and therefore fake.
    # What counts as a leak is specifically an UPHILL or level crossing: a
    # player must not be able to step sideways out of the safe branch into the
    # reward without giving anything up. Crossing DOWNHILL is always possible
    # somewhere - you can also just walk back to the fork - and it is not a
    # leak, because it throws away the altitude you climbed to get there. Time
    # is the currency the forks trade in, so paying time is allowed.
    print("Branch isolation (no uphill crossing safe -> coin-carrying risky)")
    print("-" * 66)
    isolation = [
        ("fork 1", ["A1", "A2", "A3", "A4", "A5", "A6"], ["B1", "B2", "B3"]),
        ("fork 2", ["C1", "C2", "C3", "MP0@from", "MP0@to"], ["D1", "D2", "D3"]),
    ]
    for fork, safe_side, risky_side in isolation:
        leaks = []
        for safe_name in safe_side:
            for risky_name in risky_side:
                a, b = boxes[safe_name], boxes[risky_name]
                uphill = b.top >= a.top - 0.3
                if uphill and legal(a, b):
                    leaks.append("%s -> %s" % (safe_name, risky_name))
        if leaks:
            print("  %s   LEAKS: %s" % (fork, ", ".join(leaks)))
            failures.append("%s: the safe branch can jump to %s" % (fork, ", ".join(leaks)))
        else:
            print("  %s   sealed (%d x %d pairs checked)"
                  % (fork, len(safe_side), len(risky_side)))

    # The bail-out in the other direction is meant to work.
    print("  bail-out risky -> safe:")
    for risky_name, safe_name in (("B2", "G1"), ("D2", "C2")):
        ok = legal(boxes[risky_name], boxes[safe_name]) or boxes[risky_name].top > boxes[safe_name].top
        print("     %-3s -> %-3s   %s" % (risky_name, safe_name, "ok" if ok else "no"))
    print()

    # --- 3. coins --------------------------------------------------------
    # A coin is gettable if SOME jump arc from SOME reachable surface passes
    # within kCoinRadius of it. The arcs that matter are not only the ones
    # that end on another platform: four of the coins here hang in mid-air
    # over a pit and are caught in flight, which is the whole point of them.
    #
    # The player can take off anywhere on a platform, so the horizontal
    # distance from launch to coin can be anything between "right above the
    # near edge" and "the far corner". We sweep that interval.
    print("Coins")
    print("-" * 66)
    half_body = player[1] * 0.5

    def horizontal_span(box, point):
        """Nearest and furthest horizontal distance from `box` to an (x,z)."""
        near = 0.0
        for axis in (0, 2):
            low, high = box.span(axis)
            outside = max(low - point[axis], point[axis] - high, 0.0)
            near += outside * outside
        far = 0.0
        for axis in (0, 2):
            low, high = box.span(axis)
            reach = max(abs(point[axis] - low), abs(point[axis] - high))
            far += reach * reach
        return math.sqrt(near), math.sqrt(far)

    for index, coin in enumerate(coins):
        best = None
        for box in boxes.values():
            if box.name not in seen:
                continue

            launcher = pad if box.name.startswith("P") else jump
            launch_height = box.top + half_body
            dy = coin[1] - launch_height

            # Standing on it, or walking along it, already touches the coin.
            near, far = horizontal_span(box, coin)
            if near <= coin_radius and abs(dy) <= coin_radius:
                best = box.name + " (standing)"
                break

            if dy > launcher.rise + coin_radius:
                continue

            limit = launcher.horizontal_reach(min(dy, 0.0) - 1.0) or 0.0
            step = 0.04
            distance = near
            while distance <= min(far, limit):
                if abs(launcher.arc_height(distance) - dy) <= coin_radius:
                    best = box.name
                    break
                distance += step
            if best:
                break

        status = "ok  from %s" % best if best else "UNREACHABLE"
        print("  coin %2d (%6.2f,%6.2f,%6.2f)  %s" % (index, coin[0], coin[1], coin[2], status))
        if best is None:
            failures.append("coin %d at %s cannot be collected" % (index, coin))
    print()

    # --- verdict ---------------------------------------------------------
    print("=" * 66)
    if failures:
        print("FAILED - %d problem(s):" % len(failures))
        for problem in failures:
            print("  * " + problem)
        return 1
    print("PASSED - every intended jump works, both forks converge, the goal")
    print("         is reachable, and all %d coins can be collected." % len(coins))
    return 0


if __name__ == "__main__":
    sys.exit(main())
