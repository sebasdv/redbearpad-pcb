"""Genera redbearpad.kicad_pcb desde el netlist + tabla de placement del spec.
Ejecutar con el python de KiCad (tiene el modulo pcbnew).
"""
import os, re, sys
import pcbnew

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
PCB = os.path.join(ROOT, "redbearpad.kicad_pcb")
NETFILE = os.path.join(ROOT, "redbearpad.net")
LOCAL_LIB = os.path.join(ROOT, "libs", "redbearpad.pretty")
KICAD_FP = r"C:\Users\sebastian.duarte\AppData\Local\Programs\KiCad\9.0\share\kicad\footprints"

def MM(x, y):
    return pcbnew.VECTOR2I(int(round(x * 1e6)), int(round(y * 1e6)))

def FMM(v):
    return int(round(v * 1e6))

# ---------- parsear netlist ----------
def balanced(text, start):
    depth = 0
    j = start
    while True:
        if text[j] == "(":
            depth += 1
        elif text[j] == ")":
            depth -= 1
            if depth == 0:
                return text[start:j + 1]
        j += 1

text = open(NETFILE, encoding="utf-8").read()
NETS = {}          # nombre -> [(ref, pin)]
idx = 0
while True:
    m = re.search(r"\(net\s+\(code", text[idx:])
    if not m:
        break
    blk = balanced(text, idx + m.start())
    name = re.search(r'\(name\s+"([^"]+)"\)', blk).group(1)
    nodes = [(n.group(1), n.group(2)) for n in
             re.finditer(r'\(node\s+\(ref\s+"([^"]+)"\)\s+\(pin\s+"([^"]+)"\)', blk)]
    if len(nodes) > 1:
        NETS[name.lstrip("/")] = nodes
    idx = idx + m.start() + len(blk)

# ---------- placement ----------
SW_FP = (LOCAL_LIB, "SW_Redragon_LowProfile_PCB_1.00u")
S14 = (os.path.join(KICAD_FP, "Connector_PinSocket_2.54mm.pretty"),
       "PinSocket_1x14_P2.54mm_Vertical")
S04 = (os.path.join(KICAD_FP, "Connector_PinSocket_2.54mm.pretty"),
       "PinSocket_1x04_P2.54mm_Vertical")
HOLE = (LOCAL_LIB, "MountingHole_3.1mm")

PLACE = {
    "SW1": (SW_FP, 16, 16, False), "SW2": (SW_FP, 32, 32, False),
    "SW3": (SW_FP, 16, 48, False), "SW4": (SW_FP, 32, 48, False),
    "SW5": (SW_FP, 48, 48, False), "SW6": (SW_FP, 64, 32, False),
    "SW7": (SW_FP, 80, 32, False), "SW8": (SW_FP, 80, 48, False),
    "J1": (S14, 40.38, 1.667, True),
    "J2": (S14, 55.62, 1.667, True),
    "J3": (S04, 30.0, 9.7, False),
    "H1": (HOLE, 4, 4, False), "H2": (HOLE, 92, 4, False),
    "H3": (HOLE, 4, 60, False), "H4": (HOLE, 92, 60, False),
}

board = pcbnew.NewBoard(PCB)

net_objs = {}
for name in NETS:
    n = pcbnew.NETINFO_ITEM(board, name)
    board.Add(n)
    net_objs[name] = n

fps = {}
for ref, ((lib, fpname), x, y, flip) in PLACE.items():
    fp = pcbnew.FootprintLoad(lib, fpname)
    assert fp is not None, (lib, fpname)
    fp.SetReference(ref)
    board.Add(fp)
    if flip:
        # KiCad 9: el espejo izq-der deja la orientacion en 180 y las Y de los
        # pads correctas (pad 1 arriba, pad 14 abajo); no resetear orientacion
        # despues porque rotaria los pads hacia arriba.
        fp.Flip(MM(x, y), pcbnew.FLIP_DIRECTION_LEFT_RIGHT)
        fp.SetPosition(MM(x, y))
    else:
        fp.SetPosition(MM(x, y))
        fp.SetOrientationDegrees(0)
    fps[ref] = fp

for name, nodes in NETS.items():
    for ref, pin in nodes:
        pad = fps[ref].FindPadByNumber(pin)
        assert pad is not None, (ref, pin)
        pad.SetNet(net_objs[name])

# ---------- asserts de posiciones de pads (el "test" de placement) ----------
def pad_at(ref, pin):
    p = fps[ref].FindPadByNumber(pin).GetPosition()
    return (p.x / 1e6, p.y / 1e6)

def close(a, b, tol=0.01):
    return abs(a[0] - b[0]) < tol and abs(a[1] - b[1]) < tol

assert close(pad_at("SW1", "1"), (19.8, 19.8)), pad_at("SW1", "1")
assert close(pad_at("SW1", "2"), (16.0, 22.5)), pad_at("SW1", "2")
assert close(pad_at("SW8", "1"), (83.8, 51.8)), pad_at("SW8", "1")
assert close(pad_at("J1", "1"), (40.38, 1.667)), pad_at("J1", "1")
assert close(pad_at("J1", "14"), (40.38, 34.687)), pad_at("J1", "14")
assert close(pad_at("J2", "2"), (55.62, 4.207)), pad_at("J2", "2")
assert close(pad_at("J2", "8"), (55.62, 19.447)), pad_at("J2", "8")
assert close(pad_at("J3", "1"), (30.0, 9.7)), pad_at("J3", "1")
assert close(pad_at("J3", "4"), (30.0, 17.32)), pad_at("J3", "4")
assert fps["J1"].GetLayer() == pcbnew.B_Cu, "J1 debe estar en la cara inferior"
assert fps["J2"].GetLayer() == pcbnew.B_Cu, "J2 debe estar en la cara inferior"

# ---------- contorno: rect 96x64, esquinas r=1 ----------
def edge_seg(x1, y1, x2, y2):
    s = pcbnew.PCB_SHAPE(board)
    s.SetShape(pcbnew.SHAPE_T_SEGMENT)
    s.SetStart(MM(x1, y1)); s.SetEnd(MM(x2, y2))
    s.SetLayer(pcbnew.Edge_Cuts); s.SetWidth(FMM(0.1))
    board.Add(s)

def edge_arc(sx, sy, mx, my, ex, ey):
    a = pcbnew.PCB_SHAPE(board)
    a.SetShape(pcbnew.SHAPE_T_ARC)
    a.SetArcGeometry(MM(sx, sy), MM(mx, my), MM(ex, ey))
    a.SetLayer(pcbnew.Edge_Cuts); a.SetWidth(FMM(0.1))
    board.Add(a)

R = 1.0
K = R * (1 - 0.70710678)   # sagita del punto medio del arco de 90 grados
edge_seg(R, 0, 96 - R, 0)
edge_seg(96, R, 96, 64 - R)
edge_seg(96 - R, 64, R, 64)
edge_seg(0, 64 - R, 0, R)
edge_arc(96 - R, 0, 96 - K, K, 96, R)
edge_arc(96, 64 - R, 96 - K, 64 - K, 96 - R, 64)
edge_arc(R, 64, K, 64 - K, 0, 64 - R)
edge_arc(0, R, K, K, R, 0)

# ---------- zonas GND ----------
for layer in (pcbnew.F_Cu, pcbnew.B_Cu):
    z = pcbnew.ZONE(board)
    z.SetLayer(layer)
    z.SetNet(net_objs["GND"])
    z.SetLocalClearance(FMM(0.3))
    z.SetMinThickness(FMM(0.25))
    outline = z.Outline()
    outline.NewOutline()
    for px, py in ((0, 0), (96, 0), (96, 64), (0, 64)):
        outline.Append(FMM(px), FMM(py))
    z.SetPadConnection(pcbnew.ZONE_CONNECTION_THERMAL)
    board.Add(z)

# ---------- pistas ----------
TRACKS = [
    # (net, capa, ancho_mm, [(x, y), ...])
    ("SW1", pcbnew.F_Cu, 0.25, [(19.8, 19.8), (30, 30), (38.5, 32.15), (40.38, 32.147)]),
    ("SW2", pcbnew.F_Cu, 0.25, [(35.8, 35.8), (39.2, 34.69), (40.38, 34.687)]),
    ("SW3", pcbnew.F_Cu, 0.25, [(19.8, 51.8), (19.8, 58), (57.5, 58), (57.5, 36.6), (55.62, 34.687)]),
    ("SW4", pcbnew.F_Cu, 0.25, [(35.8, 51.8), (43, 46), (43, 38), (53.5, 33.5), (55.62, 32.147)]),
    ("SW5", pcbnew.F_Cu, 0.25, [(51.8, 51.8), (58.5, 45), (58.5, 30.5), (55.62, 29.607)]),
    ("SW6", pcbnew.F_Cu, 0.25, [(67.8, 35.8), (62, 29), (57.5, 27.5), (55.62, 27.067)]),
    ("SW7", pcbnew.F_Cu, 0.25, [(83.8, 35.8), (70, 23), (57.5, 21.99), (55.62, 21.987)]),
    ("SW8", pcbnew.F_Cu, 0.25, [(83.8, 51.8), (75, 40), (62, 19.45), (55.62, 19.447)]),
    ("SDA", pcbnew.F_Cu, 0.25, [(30, 17.32), (36, 22), (38.5, 27.07), (40.38, 27.067)]),
    ("SCL", pcbnew.B_Cu, 0.25, [(30, 14.78), (34, 20), (38, 29.61), (40.38, 29.607)]),
    ("+3V3", pcbnew.F_Cu, 0.5, [(30, 12.24), (27.5, 14), (27.5, 36.5), (58.5, 36.5), (58.5, 4.21), (55.62, 4.207)]),
]
for net, layer, width, pts in TRACKS:
    for a, b in zip(pts, pts[1:]):
        t = pcbnew.PCB_TRACK(board)
        t.SetStart(MM(*a)); t.SetEnd(MM(*b))
        t.SetWidth(FMM(width)); t.SetLayer(layer)
        t.SetNet(net_objs[net])
        board.Add(t)

# ---------- serigrafia ----------
def silk(textstr, x, y, layer=pcbnew.F_SilkS, size=1.2, mirror=False):
    t = pcbnew.PCB_TEXT(board)
    t.SetText(textstr)
    t.SetPosition(MM(x, y))
    t.SetLayer(layer)
    t.SetTextSize(pcbnew.VECTOR2I(FMM(size), FMM(size)))
    t.SetTextThickness(FMM(0.2))
    t.SetMirrored(mirror)
    board.Add(t)

silk("REDBEARPAD", 75, 8, size=2.0)
for ref, (_, x, y, _) in PLACE.items():
    if ref.startswith("SW"):
        silk(ref, x - 6.5, y - 6.5, size=1.0)
for lbl, py in (("GND", 9.7), ("VCC", 12.24), ("SCL", 14.78), ("SDA", 17.32)):
    silk(lbl, 26.0, py, size=0.9)

def silk_line(x1, y1, x2, y2, layer):
    s = pcbnew.PCB_SHAPE(board)
    s.SetShape(pcbnew.SHAPE_T_SEGMENT)
    s.SetStart(MM(x1, y1)); s.SetEnd(MM(x2, y2))
    s.SetLayer(layer); s.SetWidth(FMM(0.15))
    board.Add(s)

for (x1, y1, x2, y2) in ((38.8, 0.2, 38.8, 42.1), (57.2, 0.2, 57.2, 42.1),
                          (38.8, 42.1, 57.2, 42.1)):
    silk_line(x1, y1, x2, y2, pcbnew.B_SilkS)
silk("USB", 48, 3.0, layer=pcbnew.B_SilkS, mirror=True, size=1.2)
silk("BLEND MICRO", 48, 39.5, layer=pcbnew.B_SilkS, mirror=True, size=1.2)

# ---------- rellenar zonas y guardar ----------
filler = pcbnew.ZONE_FILLER(board)
filler.Fill(board.Zones())
board.Save(PCB)
print("PCB escrita:", PCB)
