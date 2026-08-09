"""Adjunta modelos 3D a los footprints ya colocados en redbearpad.kicad_pcb, SIN
regenerar la placa (a diferencia de gen_pcb.py, esto no toca el ruteo).

Los footprints en el .kicad_pcb son copias instanciadas en el momento en que
gen_pcb.py los cargo desde la libreria; anadir un (model ...) al .kicad_mod de la
libreria no los actualiza retroactivamente. Este script recorre los footprints del
board por referencia y les agrega el modelo si todavia no lo tienen.

Ejecutar con el Python de KiCad: `...\\KiCad\\9.0\\bin\\python.exe`.
"""
import os
import pcbnew

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
PCB = os.path.join(ROOT, "redbearpad.kicad_pcb")

# ref-prefix -> (ruta del modelo relativa a KIPRJMOD, offset xyz, escala xyz, rotacion xyz)
MODELS = {
    "SW": (
        "${KIPRJMOD}/libs/redbearpad.3dshapes/SW_Redragon_LowProfile_PCB_1.00u.step",
        (0, 0, 0), (1, 1, 1), (0, 0, 0),
    ),
}
# Los tact (SW11, SW12) usan un footprint distinto sin modelo disponible todavia;
# se excluyen explicitamente para no adjuntarles por error el modelo del Redragon.
TACT_REFS = {"SW11", "SW12"}

board = pcbnew.LoadBoard(PCB)
changed = []
skipped = []

for fp in board.GetFootprints():
    ref = fp.GetReference()
    if not ref.startswith("SW") or ref in TACT_REFS:
        continue
    if fp.Models().size() > 0:
        skipped.append(ref)
        continue
    path, offset, scale, rot = MODELS["SW"]
    m = pcbnew.FP_3DMODEL()
    m.m_Filename = path
    m.m_Offset = pcbnew.VECTOR3D(*offset)
    m.m_Scale = pcbnew.VECTOR3D(*scale)
    m.m_Rotation = pcbnew.VECTOR3D(*rot)
    fp.Models().push_back(m)
    changed.append(ref)

board.Save(PCB)
print("Modelo adjuntado a:", sorted(changed))
print("Ya tenian modelo (sin tocar):", sorted(skipped))
