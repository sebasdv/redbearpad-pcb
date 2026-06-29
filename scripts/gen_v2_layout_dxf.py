"""Genera un DXF del layout v2 (12 botones estilo SNES) sobre el contorno del
gamepad actual (96 x 64 mm, esquinas r=1). Sin dependencias externas: escribe
DXF R12 (ASCII) directo.

- 10 switches Redragon low-profile: keycap 14.2 mm (footprint 1u = 19.05 mm).
- 2 tact CAX C39832249: 4.5 x 4.5 mm (Start / Select).
Coordenadas en mm. Origen abajo-izquierda, Y hacia arriba (estilo CAD).
Las posiciones se dan "desde arriba" (y_top) y se voltean a Y-up.
"""
import os

BOARD_W, BOARD_H, R = 96.0, 64.0, 1.0
MX_CAP = 14.2      # lado del keycap visible
MX_COURT = 19.05   # footprint 1u (referencia de cuerpo)
TACT = 4.5         # lado del tact CAX

# (etiqueta, x, y_desde_arriba, tipo)  tipo: 'mx' o 'tact'
BOTONES = [
    ("L",   20, 9,  "mx"),
    ("R",   76, 9,  "mx"),
    ("Sel", 42, 12, "tact"),
    ("Sta", 54, 12, "tact"),
    ("UP",  23, 26, "mx"),
    ("Y",   72, 26, "mx"),
    ("LT",   9, 40, "mx"),   # izquierda de la cruceta
    ("RT",  37, 40, "mx"),   # derecha de la cruceta
    ("X",   58, 40, "mx"),
    ("A",   86, 40, "mx"),
    ("DN",  23, 54, "mx"),
    ("B",   72, 54, "mx"),
]

def flip(y_top):
    return BOARD_H - y_top

ent = []

def line(x1, y1, x2, y2, layer):
    ent.append("0\nLINE\n8\n%s\n10\n%.4f\n20\n%.4f\n11\n%.4f\n21\n%.4f\n" % (layer, x1, y1, x2, y2))

def arc(cx, cy, r, a0, a1, layer):
    ent.append("0\nARC\n8\n%s\n10\n%.4f\n20\n%.4f\n40\n%.4f\n50\n%.4f\n51\n%.4f\n" % (layer, cx, cy, r, a0, a1))

def circle(cx, cy, r, layer):
    ent.append("0\nCIRCLE\n8\n%s\n10\n%.4f\n20\n%.4f\n40\n%.4f\n" % (layer, cx, cy, r))

def square(cx, cy, side, layer):
    h = side / 2.0
    line(cx - h, cy - h, cx + h, cy - h, layer)
    line(cx + h, cy - h, cx + h, cy + h, layer)
    line(cx + h, cy + h, cx - h, cy + h, layer)
    line(cx - h, cy + h, cx - h, cy - h, layer)

def text(cx, cy, h, s, layer):
    # TEXT con justificacion centrada (codigo 72=1 centro, 73=2 medio; alineacion en 11/21)
    ent.append("0\nTEXT\n8\n%s\n10\n%.4f\n20\n%.4f\n40\n%.4f\n1\n%s\n72\n1\n73\n2\n11\n%.4f\n21\n%.4f\n"
                % (layer, cx, cy, h, s, cx, cy))

# --- Contorno del board (rect redondeado, igual que las Edge.Cuts actuales) ---
line(R, 0, BOARD_W - R, 0, "OUTLINE")
line(BOARD_W, R, BOARD_W, BOARD_H - R, "OUTLINE")
line(BOARD_W - R, BOARD_H, R, BOARD_H, "OUTLINE")
line(0, BOARD_H - R, 0, R, "OUTLINE")
arc(R, R, R, 180, 270, "OUTLINE")
arc(BOARD_W - R, R, R, 270, 360, "OUTLINE")
arc(BOARD_W - R, BOARD_H - R, R, 0, 90, "OUTLINE")
arc(R, BOARD_H - R, R, 90, 180, "OUTLINE")

# --- Botones ---
for nombre, x, ytop, tipo in BOTONES:
    y = flip(ytop)
    if tipo == "mx":
        square(x, y, MX_CAP, "SWITCHES")     # keycap visible
        square(x, y, MX_COURT, "COURTYARD")  # cuerpo/footprint 1u (referencia)
        circle(x, y, 0.8, "CENTERS")
        text(x, y - 0.7, 2.2, nombre, "LABELS")
    else:
        square(x, y, TACT, "TACT")           # tact CAX 4.5 mm
        circle(x, y, 0.5, "CENTERS")
        text(x, y - 5.2, 1.8, nombre, "LABELS")

dxf = ("0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1009\n9\n$INSUNITS\n70\n4\n0\nENDSEC\n"
       "0\nSECTION\n2\nENTITIES\n" + "".join(ent) + "0\nENDSEC\n0\nEOF\n")

out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "redbearpad_v2_layout.dxf")
out = os.path.normpath(out)
with open(out, "w", encoding="ascii") as f:
    f.write(dxf)
print("DXF escrito en:", out)
print("Botones:", len(BOTONES), "(10 MX + 2 tact CAX)")
print("Board:", BOARD_W, "x", BOARD_H, "mm")
