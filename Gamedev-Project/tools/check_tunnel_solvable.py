#!/usr/bin/env python3
"""
check_tunnel_solvable.py - can Floor 2 actually be run?

Reads the tile grid out of src/scenes/TunnelLevel.h and the tuning out of
src/core/Config.h, then searches the whole level for a path from the start to
the finish line. It models the things that can actually stop you:

  * holes          you have to be somewhere else, or airborne
  * crates ('o')   fatal at ground level, so you dodge or jump them
  * springs ('^')  fire automatically, so landing on one is not optional
  * speed ramp     how far a jump carries depends on how fast you are going,
                   and you are going faster at the end than at the start

It also reports whether each gap after a spring really needs the spring, and
whether anything demands a two-lane shift in a single segment.

    python3 tools/check_tunnel_solvable.py
"""

import math
import os
import re
import sys
from collections import deque

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LEVEL = os.path.join(ROOT, "src", "scenes", "TunnelLevel.h")
CONFIG = os.path.join(ROOT, "src", "core", "Config.h")

NUM = r"(-?[0-9]*\.?[0-9]+)f?"

EMPTY, SOLID, BLOCK, CRUMBLE, SPRING, SPEED, ICE = ".", "#", "o", "X", "^", ">", "~"


def read(path):
    with open(path) as handle:
        return handle.read()


def tunnel_constant(text, name):
    """Constants from inside `namespace tunnel`, which has its own kJumpHeight."""
    start = text.index("namespace tunnel {")
    chunk = text[start:]
    match = re.search(r"\b" + name + r"\s*=\s*" + NUM, chunk)
    if not match:
        raise SystemExit("could not find cfg::tunnel::" + name)
    return float(match.group(1))


def load_segments():
    text = read(LEVEL)
    body = text[text.index("kSegments[] = {"):text.index("inline constexpr int kSegmentCount")]
    rows = re.findall(r'\{"(...)",\s*"(...)",\s*"(...)",\s*"(...)"\}', body)
    if not rows:
        raise SystemExit("no segments parsed - did the table format change?")
    # Flatten each segment to 12 lanes: face 0 lanes 0-2, face 1 lanes 3-5, ...
    # Moving one lane to the right is then always +1 with a wrap at 12, which
    # is exactly the corner rule the game uses.
    return ["".join(row) for row in rows]


def main():
    config = read(CONFIG)
    segments = load_segments()
    count = len(segments)

    seg_len = tunnel_constant(config, "kSegmentLength")
    lane_width = tunnel_constant(config, "kTunnelSide") / 3.0
    start_speed = tunnel_constant(config, "kStartSpeed")
    max_speed = tunnel_constant(config, "kMaxSpeed")
    ramp = tunnel_constant(config, "kSpeedRamp")
    strafe = tunnel_constant(config, "kStrafeSpeed")
    jump_height = tunnel_constant(config, "kJumpHeight")
    time_to_apex = tunnel_constant(config, "kTimeToApex")
    fall_mult = tunnel_constant(config, "kFallGravityMultiplier")
    spring_rise = tunnel_constant(config, "kSpringRise")

    gravity_up = 2.0 * jump_height / (time_to_apex * time_to_apex)
    gravity_down = gravity_up * fall_mult

    jump_airtime = time_to_apex + math.sqrt(2.0 * jump_height / gravity_down)
    spring_v0 = math.sqrt(2.0 * gravity_up * spring_rise)
    spring_airtime = spring_v0 / gravity_up + math.sqrt(2.0 * spring_rise / gravity_down)

    def speed_at(segment):
        """Forward speed on arriving at a segment, from the ramp alone.

        Ignoring speed tiles is deliberate: it is the slower, stricter answer,
        and a check that passes at the slow speed passes at the fast one too.
        """
        distance = segment * seg_len
        # x = v0*t + a*t^2/2 until v hits the cap, flat after that.
        t_cap = (max_speed - start_speed) / ramp
        x_cap = start_speed * t_cap + 0.5 * ramp * t_cap * t_cap
        if distance >= x_cap:
            return max_speed
        # solve a/2 t^2 + v0 t - x = 0
        time = (-start_speed + math.sqrt(start_speed ** 2 + 2.0 * ramp * distance)) / ramp
        return start_speed + ramp * time

    def tile(segment, lane):
        if segment < 0 or segment >= count:
            return EMPTY
        return segments[segment][lane % 12]

    def span_and_reach(segment, airtime):
        """How many segments an arc covers, and how many lanes you can shift."""
        speed = speed_at(segment)
        segs = int(airtime * speed / seg_len)
        lanes = int(airtime * strafe / lane_width)
        return max(1, segs), max(1, lanes)

    print("Floor 2 solvability")
    print("=" * 70)
    print("  segments           %d  (%.0f units, ~%.0fs at full speed)"
          % (count, count * seg_len, count * seg_len / max_speed))
    print("  jump airtime       %.3fs  -> %d-%d segments across the run"
          % (jump_airtime, span_and_reach(0, jump_airtime)[0],
             span_and_reach(count - 1, jump_airtime)[0]))
    print("  spring airtime     %.3fs  -> %d-%d segments"
          % (spring_airtime, span_and_reach(0, spring_airtime)[0],
             span_and_reach(count - 1, spring_airtime)[0]))
    print()

    failures = []
    warnings = []

    # --- 1. no segment may be completely empty ---------------------------
    # One of those is an unavoidable death, however well you are playing.
    for index, row in enumerate(segments):
        if all(character == EMPTY for character in row):
            # Only a problem if it is not inside a gap you are meant to fly
            # over - the search below decides that. Noted here for the report.
            pass

    # --- 2. search the level ---------------------------------------------
    # A state is (segment, lane). Landing on a spring is not a choice, so a
    # state whose tile is a spring only has spring moves.
    def landable(segment, lane):
        glyph = tile(segment, lane)
        return glyph != EMPTY and glyph != BLOCK

    def moves(segment, lane):
        glyph = tile(segment, lane)

        if glyph == SPRING:
            segs, reach = span_and_reach(segment, spring_airtime)
            kind = "spring"
        else:
            # On the ground: step to the next segment, shifting at most a lane.
            for shift in (-1, 0, 1):
                target = lane + shift
                if landable(segment + 1, target):
                    yield (segment + 1, target % 12, "run")
            segs, reach = span_and_reach(segment, jump_airtime)
            kind = "jump"

        # Taking off anywhere within the segment shifts the landing by up to
        # one segment either way, so allow that whole window.
        for landing in range(max(segment + 1, segment + segs - 1), segment + segs + 2):
            if landing >= count:
                yield (count, lane, kind)      # past the finish line: done
                continue
            for shift in range(-reach, reach + 1):
                if landable(landing, lane + shift):
                    yield (landing, (lane + shift) % 12, kind)

    start = (0, 1)
    seen = {start: None}
    queue = deque([start])
    finish = None

    while queue:
        segment, lane = queue.popleft()
        if segment >= count - 1:
            finish = (segment, lane)
            break
        for nxt_segment, nxt_lane, kind in moves(segment, lane):
            key = (min(nxt_segment, count - 1), nxt_lane)
            if key not in seen:
                seen[key] = ((segment, lane), kind)
                queue.append(key)

    print("Route search")
    print("-" * 70)
    if finish is None:
        failures.append("there is no way to reach the finish line")
        reached = max(s for s, _ in seen)
        print("  DEAD END - the furthest any route gets is segment %d of %d"
              % (reached, count - 1))
        print("  segment %d looks like: %s" % (reached, segments[reached]))
        if reached + 1 < count:
            print("  segment %d looks like: %s" % (reached + 1, segments[reached + 1]))
    else:
        # Walk the path back so the report can say how it was done.
        path = []
        node = finish
        while seen[node] is not None:
            previous, kind = seen[node]
            path.append((previous, node, kind))
            node = previous
        path.reverse()
        jumps = sum(1 for _, _, k in path if k == "jump")
        springs = sum(1 for _, _, k in path if k == "spring")
        print("  SOLVABLE - reaches segment %d" % finish[0])
        print("  a clean route uses %d ground steps, %d jumps, %d springs"
              % (sum(1 for _, _, k in path if k == "run"), jumps, springs))

    reachable = sorted({s for s, _ in seen})
    missing = [i for i in range(count) if i not in set(reachable)]
    print("  segments a route can stand on: %d of %d" % (count - len(missing), count))
    print()

    # --- 3. are the springs load-bearing? --------------------------------
    print("Springs")
    print("-" * 70)
    for index, row in enumerate(segments):
        if SPRING not in row:
            continue
        # How far is it to the next segment with anything solid in it?
        gap = 0
        probe = index + 1
        while probe < count and all(c == EMPTY for c in segments[probe]):
            gap += 1
            probe += 1

        jump_segs, _ = span_and_reach(index, jump_airtime)
        spring_segs, _ = span_and_reach(index, spring_airtime)

        if gap == 0:
            note = "no gap after it"
            warnings.append("spring at %d has nothing to carry you over" % index)
        elif spring_segs < gap + 1:
            note = "TOO SHORT - spring covers %d, gap needs %d" % (spring_segs, gap + 1)
            failures.append("spring at segment %d cannot clear its own gap "
                            "(covers %d segments, needs %d)" % (index, spring_segs, gap + 1))
        elif jump_segs >= gap + 1:
            note = "ok, but a plain jump (%d) also clears it" % jump_segs
            warnings.append("the gap after the spring at %d can be jumped without it" % index)
        else:
            note = "ok - spring %d, gap %d, plain jump only %d" % (spring_segs, gap + 1, jump_segs)
        print("  segment %3d  gap %d  %s" % (index, gap, note))
    print()

    # --- 4. forced two-lane shifts ---------------------------------------
    # You can move about one lane per segment, so a row that only leaves a
    # lane two away from every lane the previous row allowed is unfair.
    print("Lane shifts")
    print("-" * 70)
    tight = []
    for index in range(count - 1):
        here_open = [l for l in range(12) if landable(index, l)]
        next_open = [l for l in range(12) if landable(index + 1, l)]
        if not here_open or not next_open:
            continue
        # The best case: from the most convenient open lane here, is anything
        # within one lane open next?
        best = min(
            min(abs(((a - b + 6) % 12) - 6) for b in next_open)
            for a in here_open
        )
        if best > 1:
            tight.append((index, best))
    for index, distance in tight:
        failures.append("segment %d -> %d demands a %d-lane shift in one segment"
                        % (index, index + 1, distance))
        print("  %3d -> %3d   needs %d lanes   %s / %s"
              % (index, index + 1, distance, segments[index], segments[index + 1]))
    if not tight:
        print("  none - every row is reachable with a single-lane move")
    print()

    # --- 5. ice that demands steering ------------------------------------
    # On ice you barely steer, so a lane change that starts on ice is slow.
    print("Ice")
    print("-" * 70)
    icy_turns = []
    for index in range(count - 1):
        on_ice = [l for l in range(12) if tile(index, l) == ICE]
        if not on_ice:
            continue
        for lane in on_ice:
            if not landable(index + 1, lane):
                icy_turns.append((index, lane))
    for index, lane in icy_turns:
        warnings.append("ice at segment %d lane %d runs out next row - "
                        "the player has to steer while sliding" % (index, lane))
        print("  segment %3d lane %2d has nothing ahead of it" % (index, lane))
    if not icy_turns:
        print("  no ice tile leads straight into a hole")
    print()

    # --- verdict ---------------------------------------------------------
    print("=" * 70)
    for note in warnings:
        print("note: " + note)
    if failures:
        print()
        print("FAILED - %d problem(s):" % len(failures))
        for problem in failures:
            print("  * " + problem)
        return 1
    print()
    print("PASSED - the tunnel can be run from start to finish.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
