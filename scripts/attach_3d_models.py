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
        (0, 0, 0), (1, 1, 1), (0, 0, 180),
    ),
}
# Los tact (SW11, SW12) usan un footprint distinto sin modelo disponible todavia;
# se excluyen explicitamente para no adjuntarles por error el modelo del Redragon.
TACT_REFS = {"SW11", "SW12"}

board = pcbnew.LoadBoard(PCB)
added = []
updated = []

for fp in board.GetFootprints():
    ref = fp.GetReference()
    if not ref.startswith("SW") or ref in TACT_REFS:
        continue
    path, offset, scale, rot = MODELS["SW"]
    models = fp.Models()
    # OJO: iterar `models` (VECTOR_FP_3DMODEL, wrapper SWIG de std::vector) da copias
    # por valor -- mutar el objeto obtenido en el for-loop NO se propaga al vector real.
    # Hay que indexar con models[i] para escribir sobre el elemento de verdad.
    idx = next((i for i in range(models.size()) if models[i].m_Filename == path), None)
    if idx is not None:
        models[idx].m_Offset = pcbnew.VECTOR3D(*offset)
        models[idx].m_Scale = pcbnew.VECTOR3D(*scale)
        models[idx].m_Rotation = pcbnew.VECTOR3D(*rot)
        updated.append(ref)
    else:
        m = pcbnew.FP_3DMODEL()
        m.m_Filename = path
        m.m_Offset = pcbnew.VECTOR3D(*offset)
        m.m_Scale = pcbnew.VECTOR3D(*scale)
        m.m_Rotation = pcbnew.VECTOR3D(*rot)
        fp.Models().push_back(m)
        added.append(ref)

board.Save(PCB)
print("Modelo agregado nuevo en:", sorted(added))
print("Modelo actualizado (offset/escala/rotacion) en:", sorted(updated))
