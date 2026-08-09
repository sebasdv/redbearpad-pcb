"""Espeja BlendMicro_Module_V1.step en el eje X (mirror izquierda/derecha),
preservando nombres y colores de cada sub-parte.

Por que no un simple `scale (xyz -1 1 1)` en el bloque (model ...) del
footprint: esa es la forma "estandar" de KiCad para espejar un modelo, pero
en un solido B-rep una negacion de escala pura invierte la orientacion de
las caras (normales) sin re-triangular correctamente; el renderer de KiCad
hace back-face culling y el cuerpo del PCB desaparece casi por completo
(se probo, commit del bug documentado). El fix real es una reflexion
geometrica de verdad (OCC BRepBuilderAPI_Transform via Shape.mirror), que
si reconstruye la orientacion de las caras correctamente.

El STEP del usuario reutiliza el propio PinHeader_1x14_P2.54mm.step del
proyecto DOS VECES (una por fila de header) mas un conector micro-USB
detallado y la placa PCB en si -- ambas instancias del header comparten
nombre en el STEP, lo que hace que el loader de ensambles de cadquery
(que exige nombres unicos) falle; se parchea Assembly.add() para
renombrar automaticamente en colision, solo durante esta carga.

Requiere `pip install cadquery`.
"""
import os
import cadquery as cq
from cadquery.assembly import Assembly
from cadquery.occ_impl.exporters.assembly import exportAssembly

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
SRC = os.path.join(ROOT, "libs", "redbearpad.3dshapes", "BlendMicro_Module_V1.step")
OUT = os.path.join(ROOT, "libs", "redbearpad.3dshapes", "BlendMicro_Module_V1_mirrored.step")

# ---------- parche temporal: permitir nombres duplicados al cargar ----------
_orig_add = Assembly.add


def _patched_add(self, arg, **kwargs):
    name = kwargs.get("name")
    if name is not None:
        existing = {c.name for c in self.children}
        if name in existing or name == self.name:
            base, i = name, 2
            while name in existing or name == self.name:
                name = f"{base}_{i}"
                i += 1
            kwargs["name"] = name
    return _orig_add(self, arg, **kwargs)


Assembly.add = _patched_add
src = cq.Assembly.load(SRC)
# OJO: el parche se mantiene activo tambien durante la reconstruccion de abajo,
# porque los nombres de las hojas (p.ej. "Pins:1", "Body:1") se repiten entre
# las dos instancias del header aunque sus contenedores ya se hayan renombrado.

# ---------- recorrer el arbol, acumular transformaciones a coords de mundo,
# espejar cada hoja (shape propia) y volcar todo plano en un Assembly nuevo ----------
out = cq.Assembly(name=src.name)


def walk(node, accum_loc):
    loc = accum_loc * node.loc if node.loc is not None else accum_loc
    if node.obj is not None:
        world_shape = node.obj.moved(loc)          # hornea la transformacion acumulada
        mirrored = world_shape.mirror("YZ")          # reflexion real en el plano YZ (invierte X)
        out.add(mirrored, name=node.name, color=node.color)
    for child in node.children:
        walk(child, loc)


walk(src, cq.Location())
Assembly.add = _orig_add  # restaurar recien aca, ya terminamos de construir "out"

exportAssembly(out, OUT)
print("STEP espejado escrito:", OUT)
