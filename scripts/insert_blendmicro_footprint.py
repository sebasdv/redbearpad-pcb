"""Inserta el footprint visual BlendMicro_Module en redbearpad.kicad_pcb, SIN
regenerar la placa (igual que attach_3d_models.py, para no perder el ruteo).

Posicion: coincide con el rectangulo de serigrafia B.SilkS "BLEND MICRO" que
gen_pcb.py ya dibuja (X 38.8..57.2, Y 0.2..42.1) -- centro X=48.0 entre J1/J2,
Y centrado usando las medidas reales del DXF (42.087mm, ver
gen_blendmicro_step.py) para que el borde superior del modulo caiga en y=0,
igual que documenta el spec ("el extremo USB del modulo coincide con el
borde superior y=0").

Ejecutar con el Python de KiCad.
"""
import os
import pcbnew

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
PCB = os.path.join(ROOT, "redbearpad.kicad_pcb")
LOCAL_LIB = os.path.join(ROOT, "libs", "redbearpad.pretty")

REF = "BLENDMICRO1"
X, Y = 48.0, 21.044  # centro; ver docstring

board = pcbnew.LoadBoard(PCB)

# idempotente: si ya existe, no duplicar
existing = board.FindFootprintByReference(REF)
if existing is not None:
    print(f"{REF} ya existe en el board, no se toca. Borralo a mano si quieres reinsertar.")
else:
    fp = pcbnew.FootprintLoad(LOCAL_LIB, "BlendMicro_Module")
    assert fp is not None, "no se pudo cargar BlendMicro_Module desde la libreria"
    fp.SetReference(REF)
    fp.SetPosition(pcbnew.VECTOR2I(int(round(X * 1e6)), int(round(Y * 1e6))))
    # el footprint ya se autoria nativo en B.Cu (silk/fab/courtyard en capas
    # B.*), asi que NO se llama Flip() -- a diferencia de J1/J2, que son
    # footprints estandar nativos en F.Cu y si necesitan Flip() en gen_pcb.py.
    board.Add(fp)
    print(f"{REF} insertado en ({X}, {Y})")

board.Save(PCB)
