"""Genera una regleta de pines macho 1x14, paso 2.54mm (0.1"), THT, y la
exporta en STEP. Medidas estandar de un header macho single-row generico
(ej. Wurth 61300211121, Amphenol/Harwin equivalentes):

- Paso:              2.54 mm
- Cuerpo (aislante):  2.5 x 2.5 mm de seccion por pin, negro
- Pin (metal):        0.64 x 0.64 mm de seccion cuadrada
    - cola de soldadura (bajo el cuerpo, THT): 3.0 mm
    - embebido en el cuerpo:                    2.5 mm
    - punta de acople (sobre el cuerpo):         8.5 mm

Origen local: Z=0 en la base del cuerpo (la cara que apoya en la PCB, misma
convencion que el resto de los modelos del proyecto RedBearPad); X=0 en el
centro de la fila (pin 1 en X negativo, pin 14 en X positivo); Y=0 en el eje
de los pines.

Requiere `pip install cadquery`.
"""
import os
import cadquery as cq

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))

N = 14
PITCH = 2.54
BODY_W = 2.5        # seccion del cuerpo (Y)
BODY_H = 2.5         # alto del cuerpo (Z), tambien largo embebido del pin
PIN_SQ = 0.64        # lado del pin cuadrado
TAIL_LEN = 3.0        # cola de soldadura bajo el cuerpo
TIP_LEN = 8.5          # punta de acople sobre el cuerpo

BODY_L = (N - 1) * PITCH + BODY_W   # cuerpo continuo (una sola pieza moldeada)
PIN_TOTAL = TAIL_LEN + BODY_H + TIP_LEN

# ---------- cuerpo aislante (negro) ----------
body = (
    cq.Workplane("XY")
    .rect(BODY_L, BODY_W)
    .extrude(BODY_H)
)

# ---------- pines (metal), uno por posicion, union en un solo solido ----------
pins = None
for i in range(N):
    x = (i - (N - 1) / 2) * PITCH
    pin = (
        cq.Workplane("XY")
        .center(x, 0)
        .rect(PIN_SQ, PIN_SQ)
        .extrude(PIN_TOTAL)
        .translate((0, 0, -TAIL_LEN))
    )
    pins = pin if pins is None else pins.union(pin)

# ---------- ensamble con color por pieza (igual que el switch Redragon: ----------
# Body/Pins como sub-partes nombradas, para que el STEP conserve la separacion) ---
assy = cq.Assembly(name="PinHeader_1x14_P2.54mm")
assy.add(body, name="Body", color=cq.Color(0.08, 0.08, 0.08))       # plastico negro
assy.add(pins, name="Pins", color=cq.Color(0.78, 0.68, 0.35))       # metal dorado

from cadquery.occ_impl.exporters.assembly import exportAssembly

OUT = os.path.join(ROOT, "libs", "redbearpad.3dshapes", "PinHeader_1x14_P2.54mm.step")
# assy.save(...) es un wrapper delgado sobre exportAssembly(...) marcado
# deprecado en cadquery 2.8; se llama a la funcion real directamente.
exportAssembly(assy, OUT)
print("STEP escrito:", OUT)
print(f"Cuerpo: {BODY_L:.3f} x {BODY_W} x {BODY_H} mm")
print(f"Pin: {PIN_SQ}x{PIN_SQ} mm, total {PIN_TOTAL} mm (cola {TAIL_LEN} + cuerpo {BODY_H} + punta {TIP_LEN})")
