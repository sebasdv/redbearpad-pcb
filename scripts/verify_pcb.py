#!/usr/bin/env python3
"""
Comprehensive PCB specification verifier for redbearpad.kicad_pcb
Checks 2-8 from the specification.
"""
import sys
import math

# Add KiCad python path
sys.path.insert(0, r"C:\Users\sebastian.duarte\AppData\Local\Programs\KiCad\9.0\bin\Lib\site-packages")

import pcbnew

PCB_PATH = r"C:\redbearpad\gamepad-pcb\redbearpad.kicad_pcb"
board = pcbnew.LoadBoard(PCB_PATH)

SCALE = 1e6  # KiCad internal units to mm
TOL = 0.01   # mm tolerance

issues = []
ok_items = []

def mm(iu):
    return iu / SCALE

def check(condition, ok_msg, fail_msg):
    if condition:
        ok_items.append("OK: " + ok_msg)
    else:
        issues.append("FAIL: " + fail_msg)

# ─────────────────────────────────────────────────────────────────────────────
# CHECK 2: Footprint positions and count
# ─────────────────────────────────────────────────────────────────────────────
EXPECTED_FP = {
    "SW1": (16, 16), "SW2": (32, 32), "SW3": (16, 48), "SW4": (32, 48),
    "SW5": (48, 48), "SW6": (64, 32), "SW7": (80, 32), "SW8": (80, 48),
    "J1":  (40.38, 1.667), "J2": (55.62, 1.667), "J3": (30, 9.7),
    "H1":  (4, 4), "H2": (92, 4), "H3": (4, 60), "H4": (92, 60),
}

footprints = {fp.GetReference(): fp for fp in board.GetFootprints()}
refs = set(footprints.keys())
expected_refs = set(EXPECTED_FP.keys())

check(len(footprints) == 15,
      f"Exactly 15 footprints found",
      f"Expected 15 footprints, found {len(footprints)}: {sorted(refs)}")

extra = refs - expected_refs
missing = expected_refs - refs
if extra:
    issues.append(f"FAIL: Extra footprints: {sorted(extra)}")
if missing:
    issues.append(f"FAIL: Missing footprints: {sorted(missing)}")

for ref, (ex, ey) in EXPECTED_FP.items():
    if ref in footprints:
        fp = footprints[ref]
        pos = fp.GetPosition()
        ax, ay = mm(pos.x), mm(pos.y)
        ok = abs(ax - ex) <= TOL and abs(ay - ey) <= TOL
        check(ok,
              f"{ref} at ({ax:.3f},{ay:.3f})",
              f"{ref} expected ({ex},{ey}) got ({ax:.3f},{ay:.3f})")

# ─────────────────────────────────────────────────────────────────────────────
# CHECK 3: Layer assignments
# ─────────────────────────────────────────────────────────────────────────────
B_CU_REFS = {"J1", "J2"}
F_CU_REFS = {"J3", "SW1", "SW2", "SW3", "SW4", "SW5", "SW6", "SW7", "SW8"}

for ref in B_CU_REFS:
    if ref in footprints:
        layer = footprints[ref].GetLayer()
        check(layer == pcbnew.B_Cu,
              f"{ref} on B_Cu (layer={layer})",
              f"{ref} should be B_Cu but is layer {layer}")

for ref in F_CU_REFS:
    if ref in footprints:
        layer = footprints[ref].GetLayer()
        check(layer == pcbnew.F_Cu,
              f"{ref} on F_Cu (layer={layer})",
              f"{ref} should be F_Cu but is layer {layer}")

# ─────────────────────────────────────────────────────────────────────────────
# CHECK 4: Pad nets
# ─────────────────────────────────────────────────────────────────────────────

def get_pad_nets(ref):
    """Return dict of {pad_number_str: netname}"""
    if ref not in footprints:
        return {}
    fp = footprints[ref]
    result = {}
    for pad in fp.Pads():
        result[pad.GetNumber()] = pad.GetNetname()
    return result

# J1 expected nets
J1_NETS = {"1": "GND", "2": "GND", "11": "SDA", "12": "SCL", "13": "SW1", "14": "SW2"}
J1_NO_NET = {"3","4","5","6","7","8","9","10"}

j1_pads = get_pad_nets("J1")
print(f"\nDEBUG J1 pads: {j1_pads}")
for pnum, expected_net in J1_NETS.items():
    actual = j1_pads.get(pnum, "PAD_MISSING")
    check(actual == expected_net,
          f"J1 pad {pnum} = {expected_net}",
          f"J1 pad {pnum}: expected '{expected_net}', got '{actual}'")
for pnum in J1_NO_NET:
    actual = j1_pads.get(pnum, "")
    no_net = (actual == "" or actual == "/")
    check(no_net,
          f"J1 pad {pnum} has no net",
          f"J1 pad {pnum} should have no net but got '{actual}'")

# J2 expected nets
J2_NETS = {"1": "GND", "2": "+3V3", "8": "SW8", "9": "SW7", "11": "SW6",
           "12": "SW5", "13": "SW4", "14": "SW3"}
J2_NO_NET = {"3","4","5","6","7","10"}

j2_pads = get_pad_nets("J2")
print(f"DEBUG J2 pads: {j2_pads}")
for pnum, expected_net in J2_NETS.items():
    actual = j2_pads.get(pnum, "PAD_MISSING")
    check(actual == expected_net,
          f"J2 pad {pnum} = {expected_net}",
          f"J2 pad {pnum}: expected '{expected_net}', got '{actual}'")
for pnum in J2_NO_NET:
    actual = j2_pads.get(pnum, "")
    no_net = (actual == "" or actual == "/")
    check(no_net,
          f"J2 pad {pnum} has no net",
          f"J2 pad {pnum} should have no net but got '{actual}'")

# J3 expected nets
J3_NETS = {"1": "GND", "2": "+3V3", "3": "SCL", "4": "SDA"}
j3_pads = get_pad_nets("J3")
print(f"DEBUG J3 pads: {j3_pads}")
for pnum, expected_net in J3_NETS.items():
    actual = j3_pads.get(pnum, "PAD_MISSING")
    check(actual == expected_net,
          f"J3 pad {pnum} = {expected_net}",
          f"J3 pad {pnum}: expected '{expected_net}', got '{actual}'")

# SW1..SW8 pad nets
for i in range(1, 9):
    ref = f"SW{i}"
    pads = get_pad_nets(ref)
    print(f"DEBUG {ref} pads: {pads}")
    p1 = pads.get("1", "PAD_MISSING")
    p2 = pads.get("2", "PAD_MISSING")
    check(p1 == ref,
          f"{ref} pad1 = {ref}",
          f"{ref} pad1: expected '{ref}', got '{p1}'")
    check(p2 == "GND",
          f"{ref} pad2 = GND",
          f"{ref} pad2: expected 'GND', got '{p2}'")

# ─────────────────────────────────────────────────────────────────────────────
# CHECK 5: Board outline
# ─────────────────────────────────────────────────────────────────────────────
edge_cuts_layer = pcbnew.Edge_Cuts
drawings = board.GetDrawings()

edge_items = [d for d in drawings if d.GetLayer() == edge_cuts_layer]
segments = [d for d in edge_items if d.GetClass() == "PCB_SHAPE" and
            d.GetShape() == pcbnew.SHAPE_T_SEGMENT]
arcs = [d for d in edge_items if d.GetClass() == "PCB_SHAPE" and
        d.GetShape() == pcbnew.SHAPE_T_ARC]

print(f"\nDEBUG Edge.Cuts items: {len(edge_items)} total, {len(segments)} segments, {len(arcs)} arcs")
for item in edge_items:
    print(f"  Class={item.GetClass()} Shape={item.GetShape()}")

check(len(segments) == 4,
      f"4 Edge.Cuts segments found",
      f"Expected 4 Edge.Cuts segments, found {len(segments)}")
check(len(arcs) == 4,
      f"4 Edge.Cuts arcs found",
      f"Expected 4 Edge.Cuts arcs, found {len(arcs)}")

# Compute bounding box of all edge items
all_xs = []
all_ys = []
for item in edge_items:
    bb = item.GetBoundingBox()
    all_xs += [mm(bb.GetLeft()), mm(bb.GetRight())]
    all_ys += [mm(bb.GetTop()), mm(bb.GetBottom())]

if all_xs and all_ys:
    min_x, max_x = min(all_xs), max(all_xs)
    min_y, max_y = min(all_ys), max(all_ys)
    width = max_x - min_x
    height = max_y - min_y
    print(f"\nDEBUG Board outline bbox: ({min_x:.3f},{min_y:.3f}) to ({max_x:.3f},{max_y:.3f}), size={width:.3f}x{height:.3f}")
    check(abs(min_x) <= 0.1 and abs(min_y) <= 0.1,
          f"Board starts at origin ({min_x:.3f},{min_y:.3f})",
          f"Board origin expected ~(0,0), got ({min_x:.3f},{min_y:.3f})")
    check(abs(width - 96) <= 0.5,
          f"Board width ~96mm ({width:.3f}mm)",
          f"Board width expected 96mm, got {width:.3f}mm")
    check(abs(height - 64) <= 0.5,
          f"Board height ~64mm ({height:.3f}mm)",
          f"Board height expected 64mm, got {height:.3f}mm")

# ─────────────────────────────────────────────────────────────────────────────
# CHECK 6: Copper zones
# ─────────────────────────────────────────────────────────────────────────────
zones = list(board.Zones())
print(f"\nDEBUG Zones: {len(zones)}")
copper_gnd_zones = []
rule_area_zones = []
for z in zones:
    net = z.GetNetname()
    layer = z.GetLayer()
    is_rule = z.GetIsRuleArea() if hasattr(z, 'GetIsRuleArea') else False
    print(f"  Zone: net='{net}' layer={layer} is_rule_area={is_rule}")
    if not is_rule:
        if net == "GND":
            copper_gnd_zones.append(z)
    else:
        rule_area_zones.append(z)

gnd_f = [z for z in copper_gnd_zones if z.GetLayer() == pcbnew.F_Cu]
gnd_b = [z for z in copper_gnd_zones if z.GetLayer() == pcbnew.B_Cu]

check(len(gnd_f) >= 1,
      f"GND zone on F_Cu found ({len(gnd_f)})",
      f"Expected GND zone on F_Cu, found {len(gnd_f)}")
check(len(gnd_b) >= 1,
      f"GND zone on B_Cu found ({len(gnd_b)})",
      f"Expected GND zone on B_Cu, found {len(gnd_b)}")
check(len(copper_gnd_zones) == 2,
      f"Exactly 2 GND copper zones",
      f"Expected 2 GND copper zones, found {len(copper_gnd_zones)}")

# ─────────────────────────────────────────────────────────────────────────────
# CHECK 7: Track widths
# ─────────────────────────────────────────────────────────────────────────────
tracks = board.GetTracks()
print(f"\nDEBUG Tracks: {len(list(tracks))} total")

# Get board bounding box for edge proximity check
board_bbox = board.GetBoardEdgesBoundingBox()
edge_min_x = mm(board_bbox.GetLeft())
edge_max_x = mm(board_bbox.GetRight())
edge_min_y = mm(board_bbox.GetTop())
edge_max_y = mm(board_bbox.GetBottom())
print(f"DEBUG Board bbox for edge check: ({edge_min_x:.3f},{edge_min_y:.3f}) to ({edge_max_x:.3f},{edge_max_y:.3f})")

track_width_issues = []
edge_proximity_issues = []

for track in board.GetTracks():
    if track.GetClass() not in ("PCB_TRACK", "PCB_ARC"):
        continue
    net = track.GetNetname()
    w = mm(track.GetWidth())

    # Width check
    if net == "+3V3":
        if abs(w - 0.5) > 0.01:
            track_width_issues.append(f"+3V3 track width={w:.3f}mm (expected 0.5mm)")
    else:
        # Signal tracks should be 0.25mm
        if abs(w - 0.25) > 0.01:
            track_width_issues.append(f"Net '{net}' track width={w:.3f}mm (expected 0.25mm)")

    # Edge proximity check (0.3mm from board edge)
    start = track.GetStart()
    end = track.GetEnd()
    for pt in [start, end]:
        px, py = mm(pt.x), mm(pt.y)
        dist_left = px - edge_min_x
        dist_right = edge_max_x - px
        dist_top = py - edge_min_y
        dist_bot = edge_max_y - py
        min_dist = min(dist_left, dist_right, dist_top, dist_bot)
        if min_dist < 0.3:
            edge_proximity_issues.append(
                f"Track net='{net}' endpoint ({px:.3f},{py:.3f}) is {min_dist:.3f}mm from edge (min 0.3mm)")

if track_width_issues:
    # De-dup
    seen = set()
    for msg in track_width_issues:
        if msg not in seen:
            seen.add(msg)
            issues.append("FAIL: " + msg)
else:
    ok_items.append("OK: All track widths correct (signal=0.25mm, +3V3=0.5mm)")

if edge_proximity_issues:
    seen = set()
    for msg in edge_proximity_issues[:10]:
        if msg not in seen:
            seen.add(msg)
            issues.append("FAIL: " + msg)
    if len(edge_proximity_issues) > 10:
        issues.append(f"FAIL: ...and {len(edge_proximity_issues)-10} more edge-proximity violations")
else:
    ok_items.append("OK: All tracks >= 0.3mm from board edges")

# ─────────────────────────────────────────────────────────────────────────────
# CHECK 8: Silkscreen texts
# ─────────────────────────────────────────────────────────────────────────────
F_SILKS = pcbnew.F_SilkS
B_SILKS = pcbnew.B_SilkS

f_silk_texts = set()
b_silk_texts = set()

for item in board.GetDrawings():
    layer = item.GetLayer()
    cls = item.GetClass()
    if cls in ("PCB_TEXT", "PCB_TEXTBOX"):
        text = item.GetText().strip()
        if layer == F_SILKS:
            f_silk_texts.add(text)
        elif layer == B_SILKS:
            b_silk_texts.add(text)

# Also check reference/value texts in footprints (these appear on silkscreen)
for fp in board.GetFootprints():
    ref_item = fp.Reference()
    val_item = fp.Value()
    for item in [ref_item, val_item]:
        layer = item.GetLayer()
        text = item.GetText().strip()
        if layer == F_SILKS:
            f_silk_texts.add(text)
        elif layer == B_SILKS:
            b_silk_texts.add(text)

print(f"\nDEBUG F.SilkS texts: {sorted(f_silk_texts)}")
print(f"DEBUG B.SilkS texts: {sorted(b_silk_texts)}")

# Also look at all drawings for B_SilkS including lines
b_silk_lines = [d for d in board.GetDrawings() if d.GetLayer() == B_SILKS]
print(f"DEBUG B.SilkS items (all): {len(b_silk_lines)}")

F_REQUIRED = ["REDBEARPAD", "SW1", "SW2", "SW3", "SW4", "SW5", "SW6", "SW7", "SW8",
              "GND", "VCC", "SCL", "SDA"]
B_REQUIRED_TEXTS = ["USB", "BLEND MICRO"]

for text in F_REQUIRED:
    found = any(text in t for t in f_silk_texts)
    check(found,
          f"F.SilkS contains '{text}'",
          f"F.SilkS missing '{text}' (found: {sorted(f_silk_texts)})")

for text in B_REQUIRED_TEXTS:
    found = any(text in t for t in b_silk_texts)
    check(found,
          f"B.SilkS contains '{text}'",
          f"B.SilkS missing '{text}' (found: {sorted(b_silk_texts)})")

# B.SilkS frame lines (3 lines)
b_silk_line_items = [d for d in board.GetDrawings()
                     if d.GetLayer() == B_SILKS and
                     d.GetClass() == "PCB_SHAPE" and
                     d.GetShape() == pcbnew.SHAPE_T_SEGMENT]
print(f"DEBUG B.SilkS line segments: {len(b_silk_line_items)}")
check(len(b_silk_line_items) >= 3,
      f"B.SilkS has >= 3 frame lines ({len(b_silk_line_items)})",
      f"B.SilkS expected >= 3 frame lines, found {len(b_silk_line_items)}")

# ─────────────────────────────────────────────────────────────────────────────
# SUMMARY
# ─────────────────────────────────────────────────────────────────────────────
print("\n" + "="*70)
print("VERIFICATION SUMMARY")
print("="*70)
print(f"\nOK items ({len(ok_items)}):")
for item in ok_items:
    print(f"  {item}")

if issues:
    print(f"\nFAILED checks ({len(issues)}):")
    for issue in issues:
        print(f"  {issue}")
    print(f"\n*** RESULT: FAIL — {len(issues)} issue(s) found ***")
else:
    print(f"\n*** RESULT: PASS — All checks passed ***")
