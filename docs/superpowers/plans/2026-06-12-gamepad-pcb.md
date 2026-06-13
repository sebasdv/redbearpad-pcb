# RedBearPad Gamepad PCB — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Generar el proyecto KiCad 9 completo (esquemático + PCB ruteada + Gerbers JLCPCB) del gamepad de 8 botones con OLED para Blend Micro, según el spec `docs/superpowers/specs/2026-06-12-gamepad-pcb-design.md`.

**Architecture:** Todo se genera por script (Python) para que sea reproducible y verificable sin GUI: `gen_sch.py` produce el esquemático extrayendo símbolos de las librerías estándar de KiCad; el netlist exportado con `kicad-cli` alimenta a `gen_pcb.py` (API `pcbnew`) que coloca, rutea y rellena zonas. La verificación es ERC=0, DRC=0, 0 unconnected, más renders para revisión visual.

**Tech Stack:** KiCad 9.0.x (`kicad-cli.exe` + Python embebido con módulo `pcbnew`), PowerShell, git.

**Decisiones de plan (desviaciones conscientes del spec, mismo resultado):**
1. El "zócalo Blend Micro propio" se implementa como **dos footprints estándar `PinSocket_1x14_P2.54mm_Vertical`** (J1/J2) colocados en coordenadas exactas y volteados a la cara inferior, más serigrafía de orientación propia. Mismo resultado físico, menos riesgo que un footprint a mano.
2. La librería local se renombra `libs/footprints/` → `libs/redbearpad.pretty/` (convención KiCad para librerías de footprints).

---

## Datos maestros (de referencia para todas las tareas)

Coordenadas KiCad en mm, origen esquina superior izquierda de la placa, Y hacia abajo.

### Netlist objetivo

| Net | Nodos |
|---|---|
| GND | J1.1, J1.2, J2.1, J3.1, SW1.2 … SW8.2 |
| +3V3 | J2.2, J3.2 |
| SCL | J1.12, J3.3 |
| SDA | J1.11, J3.4 |
| SW1 | SW1.1, J1.13 |
| SW2 | SW2.1, J1.14 |
| SW3 | SW3.1, J2.14 |
| SW4 | SW4.1, J2.13 |
| SW5 | SW5.1, J2.12 |
| SW6 | SW6.1, J2.11 |
| SW7 | SW7.1, J2.9 |
| SW8 | SW8.1, J2.8 |

Pines sin conexión (no_connect en esquemático): J1.3–J1.10 (VIN, SS, CLK, MOSI, MISO, /RST, D0, D1), J2.3–J2.7 (AREF, A5–A2), J2.10 (D13).

El orden de pines de J1/J2 ya incorpora el **espejado por montaje en cara inferior**
(vista desde el frente de la placa, USB arriba):
J1 (columna x=40.38, pin1 arriba): GND, GND, VIN, SS, CLK, MOSI, MISO, /RST, D0, D1, D2/SDA, D3/SCL, D5, D8.
J2 (columna x=55.62, pin1 arriba): GND, V33, AREF, A5, A4, A3, A2, A1, A0, D13, D12, D11, D10, D9.

### Posiciones en placa

| Ref | Footprint | Posición (origen del footprint) | Cara | Notas |
|---|---|---|---|---|
| SW1 | `redbearpad:SW_Redragon_LowProfile_PCB_1.00u` | (16, 16) | F | pad1 en (+3.8,+3.8), pad2 en (0,+6.5) del origen |
| SW2 | ídem | (32, 32) | F | |
| SW3 | ídem | (16, 48) | F | |
| SW4 | ídem | (32, 48) | F | |
| SW5 | ídem | (48, 48) | F | |
| SW6 | ídem | (64, 32) | F | |
| SW7 | ídem | (80, 32) | F | |
| SW8 | ídem | (80, 48) | F | |
| J1 | `Connector_PinSocket_2.54mm:PinSocket_1x14_P2.54mm_Vertical` | (40.38, 1.667), pin n en y=1.667+2.54·(n−1) | **B** (flip) | fila izquierda Blend Micro |
| J2 | ídem | (55.62, 1.667) | **B** (flip) | fila derecha Blend Micro |
| J3 | `Connector_PinSocket_2.54mm:PinSocket_1x04_P2.54mm_Vertical` | (30, 9.7), pin n en y=9.7+2.54·(n−1) | F | OLED: GND, VCC, SCL, SDA |
| H1–H4 | `redbearpad:MountingHole_3.1mm` | (4,4), (92,4), (4,60), (92,60) | F | NPTH Ø3.1 |

Contorno: rect 96×64, esquinas r=1 (4 segmentos + 4 arcos en Edge.Cuts).
Zonas GND en F.Cu y B.Cu cubriendo toda la placa.
Pistas: señal 0.25 mm, +3V3 0.5 mm, clearance 0.2 mm (defaults KiCad).

### Rutas sugeridas (punto de partida; el criterio de éxito es DRC=0 y 0 unconnected, ajustar waypoints si hace falta)

Posiciones absolutas de pads señal de switches: pad1 = centro + (3.8, 3.8).

| Net | Capa | Waypoints (mm) |
|---|---|---|
| SW1 | F.Cu | (19.8,19.8) → (30,30) → (38.5,32.15) → (40.38,32.15) |
| SW2 | F.Cu | (35.8,35.8) → (39.2,34.69) → (40.38,34.687) |
| SW3 | F.Cu | (19.8,51.8) → (19.8,58) → (57.5,58) → (57.5,36.6) → (55.62,34.687) |
| SW4 | F.Cu | (35.8,51.8) → (43,46) → (43,38) → (53.5,33.5) → (55.62,32.147) |
| SW5 | F.Cu | (51.8,51.8) → (58.5,45) → (58.5,30.5) → (55.62,29.607) |
| SW6 | F.Cu | (67.8,35.8) → (62,29) → (57.5,27.5) → (55.62,27.067) |
| SW7 | F.Cu | (83.8,35.8) → (70,23) → (57.5,21.99) → (55.62,21.987) |
| SW8 | F.Cu | (83.8,51.8) → (75,40) → (62,19.45) → (55.62,19.447) |
| SDA | F.Cu | (30,17.32) → (36,22) → (38.5,27.07) → (40.38,27.067) |
| SCL | **B.Cu** | (30,14.78) → (34,20) → (38,29.61) → (40.38,29.607) |
| +3V3 (0.5mm) | F.Cu | (30,12.24) → (27.5,14) → (27.5,36.5) → (58.5,36.5) → (58.5,4.21) → (55.62,4.207) |

Notas de ruteo: nunca correr una pista vertical sobre una columna de pads (x=40.38 o x=55.62) — entrar a cada pad lateralmente desde x≈38.5 (J1, por la izquierda) o x≈57.5/58.5 (J2, por la derecha). El hueco entre pads adyacentes de una columna (pitch 2.54, pad Ø1.7) solo deja banda libre de ~0.84 mm: no cruzar columnas con pistas de 0.5 mm; la ruta de +3V3 rodea ambas columnas por el sur (y=36.5). GND no se rutea: lo conectan las zonas (si DRC reporta islas GND sin conectar, añadir vías de costura Ø0.6/0.3 donde haga falta).

---

### Task 1: Entorno y esqueleto del proyecto

**Files:**
- Create: `redbearpad.kicad_pro`
- Create: `fp-lib-table`
- Create: `.gitignore`
- Rename: `libs/footprints/` → `libs/redbearpad.pretty/`

- [ ] **Step 1: Verificar herramientas KiCad**

Run:
```powershell
& "C:\Program Files\KiCad\9.0\bin\kicad-cli.exe" version
& "C:\Program Files\KiCad\9.0\bin\python.exe" -c "import pcbnew; print(pcbnew.Version())"
```
Expected: ambas imprimen versión 9.0.x. Si la ruta no existe, localizar con
`Get-ChildItem 'C:\Program Files\KiCad' -Directory` y ajustar la ruta en TODOS los
comandos/scripts siguientes (definirla una vez como `$KICAD = "C:\Program Files\KiCad\<ver>\bin"`).

- [ ] **Step 2: Renombrar la librería local**

```powershell
git -C C:\redbearpad\gamepad-pcb mv libs/footprints libs/redbearpad.pretty
```

- [ ] **Step 3: Crear `redbearpad.kicad_pro`**

Contenido completo:
```json
{
  "board": { "design_settings": {}, "layer_presets": [], "viewports": [] },
  "boards": [],
  "cvpcb": { "equivalence_files": [] },
  "libraries": { "pinned_footprint_libs": [], "pinned_symbol_libs": [] },
  "meta": { "filename": "redbearpad.kicad_pro", "version": 3 },
  "net_settings": {
    "classes": [
      {
        "name": "Default",
        "clearance": 0.2, "track_width": 0.25,
        "via_diameter": 0.6, "via_drill": 0.3,
        "diff_pair_gap": 0.25, "diff_pair_via_gap": 0.25, "diff_pair_width": 0.2,
        "microvia_diameter": 0.3, "microvia_drill": 0.1,
        "bus_width": 12, "line_style": 0, "pcb_color": "rgba(0, 0, 0, 0.000)",
        "schematic_color": "rgba(0, 0, 0, 0.000)", "wire_width": 6
      }
    ],
    "meta": { "version": 4 }
  },
  "pcbnew": { "page_layout_descr_file": "" },
  "schematic": { "legacy_lib_dir": "", "legacy_lib_list": [] },
  "sheets": [],
  "text_variables": {}
}
```

- [ ] **Step 4: Crear `fp-lib-table`**

```
(fp_lib_table
  (version 7)
  (lib (name "redbearpad")(type "KiCad")(uri "${KIPRJMOD}/libs/redbearpad.pretty")(options "")(descr "Footprints locales RedBearPad"))
)
```

- [ ] **Step 5: Crear `.gitignore`**

```
*.kicad_prl
*-backups/
fp-info-cache
_autosave-*
~*.lck
```

- [ ] **Step 6: Commit**

```powershell
git -C C:\redbearpad\gamepad-pcb add -A
git -C C:\redbearpad\gamepad-pcb commit -m "Esqueleto del proyecto KiCad (pro, fp-lib-table, lib local)"
```

---

### Task 2: Footprint MountingHole_3.1mm

**Files:**
- Create: `libs/redbearpad.pretty/MountingHole_3.1mm.kicad_mod`

- [ ] **Step 1: Escribir el footprint**

Contenido completo:
```
(footprint "MountingHole_3.1mm"
	(version 20240108)
	(generator "manual")
	(layer "F.Cu")
	(descr "Perforacion de montaje M3, NPTH 3.1mm, segun REFERENCE_PCB.dxf")
	(attr exclude_from_pos_files exclude_from_bom)
	(property "Reference" "REF**"
		(at 0 -3 0)
		(layer "F.SilkS")
		(hide yes)
		(effects (font (size 1 1) (thickness 0.15)))
	)
	(property "Value" "MountingHole_3.1mm"
		(at 0 3 0)
		(layer "F.Fab")
		(hide yes)
		(effects (font (size 1 1) (thickness 0.15)))
	)
	(fp_circle (center 0 0) (end 1.8 0) (stroke (width 0.15) (type solid)) (fill none) (layer "F.CrtYd"))
	(pad "" np_thru_hole circle (at 0 0) (size 3.1 3.1) (drill 3.1) (layers "*.Cu" "*.Mask"))
)
```

- [ ] **Step 2: Test de carga (ambos footprints locales)**

```powershell
& "C:\Program Files\KiCad\9.0\bin\python.exe" -c @"
import pcbnew
lib = r'C:\redbearpad\gamepad-pcb\libs\redbearpad.pretty'
for name in ['MountingHole_3.1mm', 'SW_Redragon_LowProfile_PCB_1.00u']:
    fp = pcbnew.FootprintLoad(lib, name)
    assert fp is not None, name
    print(name, 'OK, pads:', fp.Pads().size())
"@
```
Expected: `MountingHole_3.1mm OK, pads: 1` y `SW_Redragon_LowProfile_PCB_1.00u OK, pads: 3`.

- [ ] **Step 3: Commit**

```powershell
git -C C:\redbearpad\gamepad-pcb add -A
git -C C:\redbearpad\gamepad-pcb commit -m "Footprint NPTH 3.1mm + verificacion de carga de libreria local"
```

---

### Task 3: Generador del esquemático + ERC

**Files:**
- Create: `scripts/gen_sch.py`
- Create (generado): `redbearpad.kicad_sch`

- [ ] **Step 1: Escribir `scripts/gen_sch.py`**

Código completo (si algún detalle del formato s-expression es rechazado por KiCad,
iterar hasta que el Step 2 y el ERC pasen — el formato de referencia se puede
inspeccionar guardando un esquemático mínimo desde la GUI de KiCad):

```python
"""Genera redbearpad.kicad_sch (formato KiCad 8, legible por KiCad 9).
Extrae los simbolos de las librerias estandar instaladas y conecta los pines
colocando global labels exactamente en el punto de conexion de cada pin
(sin wires). Pines no usados llevan no_connect.
"""
import os, re, uuid

KICAD_SYMS = r"C:\Program Files\KiCad\9.0\share\kicad\symbols"
HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.normpath(os.path.join(HERE, "..", "redbearpad.kicad_sch"))
ROOT_UUID = "f0a11111-2222-3333-4444-555566667777"

def U():
    return str(uuid.uuid4())

def balanced(text, start):
    depth = 0
    j = start
    while True:
        c = text[j]
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
            if depth == 0:
                return text[start:j + 1]
        j += 1

def extract_symbol(libfile, name):
    path = os.path.join(KICAD_SYMS, libfile)
    text = open(path, encoding="utf-8").read()
    i = text.index('(symbol "%s"' % name)
    block = balanced(text, i)
    nick = libfile.rsplit(".", 2)[0]
    return block.replace('(symbol "%s"' % name, '(symbol "%s:%s"' % (nick, name), 1)

def pin_map(sym_block):
    """num -> (x, y) punto de conexion en coords de simbolo (Y hacia arriba)."""
    pins = {}
    idx = 0
    while True:
        m = re.search(r"\(pin\s", sym_block[idx:])
        if not m:
            break
        blk = balanced(sym_block, idx + m.start())
        at = re.search(r"\(at\s+([-\d.]+)\s+([-\d.]+)", blk)
        num = re.search(r'\(number\s+"([^"]+)"', blk)
        pins[num.group(1)] = (float(at.group(1)), float(at.group(2)))
        idx = idx + m.start() + len(blk)
    return pins

def sym_instance(lib_id, ref, value, fp, x, y, pin_nums):
    pin_lines = "\n".join('    (pin "%s" (uuid "%s"))' % (n, U()) for n in pin_nums)
    return f"""  (symbol (lib_id "{lib_id}") (at {x} {y} 0) (unit 1)
    (exclude_from_sim no) (in_bom yes) (on_board yes) (dnp no)
    (uuid "{U()}")
    (property "Reference" "{ref}" (at {x} {y - 7.62} 0) (effects (font (size 1.27 1.27))))
    (property "Value" "{value}" (at {x} {y + 7.62} 0) (effects (font (size 1.27 1.27))))
    (property "Footprint" "{fp}" (at {x} {y} 0) (effects (font (size 1.27 1.27)) (hide yes)))
    (property "Datasheet" "" (at {x} {y} 0) (effects (font (size 1.27 1.27)) (hide yes)))
{pin_lines}
    (instances (project "redbearpad" (path "/{ROOT_UUID}" (reference "{ref}") (unit 1))))
  )"""

def glabel(net, x, y, left):
    ang = 180 if left else 0
    just = "(justify right)" if left else "(justify left)"
    return (f'  (global_label "{net}" (shape passive) (at {x} {y} {ang}) '
            f"(effects (font (size 1.27 1.27)) {just}) "
            f'(uuid "{U()}"))')

def no_connect(x, y):
    return f'  (no_connect (at {x} {y}) (uuid "{U()}"))'

sw_blk = extract_symbol("Switch.kicad_sym", "SW_Push")
c14_blk = extract_symbol("Connector_Generic.kicad_sym", "Conn_01x14")
c04_blk = extract_symbol("Connector_Generic.kicad_sym", "Conn_01x04")
sw_pins = pin_map(sw_blk)
c14_pins = pin_map(c14_blk)
c04_pins = pin_map(c04_blk)

J1_NETS = ["GND", "GND", None, None, None, None, None, None, None, None,
           "SDA", "SCL", "SW1", "SW2"]
J2_NETS = ["GND", "+3V3", None, None, None, None, None, "SW8", "SW7", None,
           "SW6", "SW5", "SW4", "SW3"]
J3_NETS = ["GND", "+3V3", "SCL", "SDA"]

body = []

def place_conn(ref, value, fp, lib_id, pins, nets, x, y):
    body.append(sym_instance(lib_id, ref, value, fp, x, y,
                             [str(n) for n in range(1, len(nets) + 1)]))
    for n, net in enumerate(nets, start=1):
        px, py = pins[str(n)]
        gx, gy = x + px, y - py
        if net is None:
            body.append(no_connect(gx, gy))
        else:
            body.append(glabel(net, gx, gy, left=px < 0))

for k in range(8):
    ref = "SW%d" % (k + 1)
    x, y = 63.5, 25.4 + 15.24 * k
    body.append(sym_instance("Switch:SW_Push", ref, "Redragon_LP",
                             "redbearpad:SW_Redragon_LowProfile_PCB_1.00u",
                             x, y, ["1", "2"]))
    for num, net in (("1", ref), ("2", "GND")):
        px, py = sw_pins[num]
        body.append(glabel(net, x + px, y - py, left=px < 0))

place_conn("J1", "BlendMicro_fila_izq",
           "Connector_PinSocket_2.54mm:PinSocket_1x14_P2.54mm_Vertical",
           "Connector_Generic:Conn_01x14", c14_pins, J1_NETS, 152.4, 63.5)
place_conn("J2", "BlendMicro_fila_der",
           "Connector_PinSocket_2.54mm:PinSocket_1x14_P2.54mm_Vertical",
           "Connector_Generic:Conn_01x14", c14_pins, J2_NETS, 203.2, 63.5)
place_conn("J3", "OLED_128x32_I2C",
           "Connector_PinSocket_2.54mm:PinSocket_1x04_P2.54mm_Vertical",
           "Connector_Generic:Conn_01x04", c04_pins, J3_NETS, 101.6, 152.4)

sch = f"""(kicad_sch (version 20231120) (generator "gen_sch")
  (uuid "{ROOT_UUID}")
  (paper "A3")
  (lib_symbols
{sw_blk}
{c14_blk}
{c04_blk}
  )
{chr(10).join(body)}
  (sheet_instances (path "/" (page "1")))
)
"""
with open(OUT, "w", encoding="utf-8") as f:
    f.write(sch)
print("Escrito", OUT)
```

- [ ] **Step 2: Generar y verificar que KiCad lo lee**

```powershell
& "C:\Program Files\KiCad\9.0\bin\python.exe" C:\redbearpad\gamepad-pcb\scripts\gen_sch.py
& "C:\Program Files\KiCad\9.0\bin\kicad-cli.exe" sch export netlist --output C:\redbearpad\gamepad-pcb\redbearpad.net C:\redbearpad\gamepad-pcb\redbearpad.kicad_sch
```
Expected: ambos sin error (el netlist se valida en la Task 4; aquí solo confirma
que el archivo es parseable).

- [ ] **Step 3: ERC limpio**

```powershell
& "C:\Program Files\KiCad\9.0\bin\kicad-cli.exe" sch erc --severity-error --exit-code-violations C:\redbearpad\gamepad-pcb\redbearpad.kicad_sch
```
Expected: exit code 0, "0 errors". Si reporta errores, leer el reporte, corregir
`gen_sch.py`, regenerar y repetir.

- [ ] **Step 4: Commit**

```powershell
git -C C:\redbearpad\gamepad-pcb add -A
git -C C:\redbearpad\gamepad-pcb commit -m "Esquematico generado por script, ERC limpio"
```

---

### Task 4: Verificación del netlist

**Files:**
- Create: `scripts/verify_netlist.py`
- Create (generado): `redbearpad.net`

- [ ] **Step 1: Escribir `scripts/verify_netlist.py`**

Código completo:
```python
"""Verifica que redbearpad.net contiene EXACTAMENTE las nets del spec."""
import os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
NET = os.path.normpath(os.path.join(HERE, "..", "redbearpad.net"))

EXPECTED = {
    "GND": {("J1", "1"), ("J1", "2"), ("J2", "1"), ("J3", "1"),
            ("SW1", "2"), ("SW2", "2"), ("SW3", "2"), ("SW4", "2"),
            ("SW5", "2"), ("SW6", "2"), ("SW7", "2"), ("SW8", "2")},
    "+3V3": {("J2", "2"), ("J3", "2")},
    "SCL": {("J1", "12"), ("J3", "3")},
    "SDA": {("J1", "11"), ("J3", "4")},
    "SW1": {("SW1", "1"), ("J1", "13")},
    "SW2": {("SW2", "1"), ("J1", "14")},
    "SW3": {("SW3", "1"), ("J2", "14")},
    "SW4": {("SW4", "1"), ("J2", "13")},
    "SW5": {("SW5", "1"), ("J2", "12")},
    "SW6": {("SW6", "1"), ("J2", "11")},
    "SW7": {("SW7", "1"), ("J2", "9")},
    "SW8": {("SW8", "1"), ("J2", "8")},
}

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

text = open(NET, encoding="utf-8").read()
found = {}
idx = 0
while True:
    m = re.search(r"\(net\s+\(code", text[idx:])
    if not m:
        break
    blk = balanced(text, idx + m.start())
    name = re.search(r'\(name\s+"([^"]+)"\)', blk).group(1)
    nodes = set()
    for nm in re.finditer(r'\(node\s+\(ref\s+"([^"]+)"\)\s+\(pin\s+"([^"]+)"\)', blk):
        nodes.add((nm.group(1), nm.group(2)))
    if nodes:
        found[name.lstrip("/")] = nodes
    idx = idx + m.start() + len(blk)

ok = True
for net, nodes in EXPECTED.items():
    if found.get(net) != nodes:
        print("MISMATCH", net, "\n  esperado:", sorted(nodes),
              "\n  obtenido:", sorted(found.get(net, set())))
        ok = False
extra = set(found) - set(EXPECTED)
if extra:
    print("Nets extra con >1 nodo:", extra)
    # nets de un solo pin (pines NC) no aparecen aqui porque exigimos nodes
for net in extra:
    if len(found[net]) > 1:
        ok = False
print("NETLIST OK" if ok else "NETLIST CON ERRORES")
sys.exit(0 if ok else 1)
```

- [ ] **Step 2: Ejecutar**

```powershell
& "C:\Program Files\KiCad\9.0\bin\python.exe" C:\redbearpad\gamepad-pcb\scripts\verify_netlist.py
```
Expected: `NETLIST OK`, exit 0. Si falla, el error está en `gen_sch.py`
(orden de nets de J1/J2/J3) — corregir, regenerar, re-exportar netlist y repetir.

- [ ] **Step 3: Commit**

```powershell
git -C C:\redbearpad\gamepad-pcb add -A
git -C C:\redbearpad\gamepad-pcb commit -m "Netlist exportado y verificado contra el spec"
```

---

### Task 5: Generador de PCB — placement, contorno y nets

**Files:**
- Create: `scripts/gen_pcb.py`
- Create (generado): `redbearpad.kicad_pcb`

- [ ] **Step 1: Escribir `scripts/gen_pcb.py` (parte placement)**

Código completo (las TRACKS y zonas se añaden en la Task 6; este paso ya las deja
definidas pero la lista TRACKS puede ajustarse después):

```python
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
KICAD_FP = r"C:\Program Files\KiCad\9.0\share\kicad\footprints"

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
        NETS[name] = nodes
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
        fp.Flip(MM(x, y), True)
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

assert close(pad_at("SW1", "1"), (19.8, 19.8))
assert close(pad_at("SW1", "2"), (16.0, 22.5))
assert close(pad_at("SW8", "1"), (83.8, 51.8))
assert close(pad_at("J1", "1"), (40.38, 1.667))
assert close(pad_at("J1", "14"), (40.38, 34.687))
assert close(pad_at("J2", "2"), (55.62, 4.207))
assert close(pad_at("J2", "8"), (55.62, 19.447))
assert close(pad_at("J3", "1"), (30.0, 9.7))
assert close(pad_at("J3", "4"), (30.0, 17.32))
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
K = R * (1 - 0.70710678)   # sagita para el punto medio del arco de 90 grados
edge_seg(R, 0, 96 - R, 0)
edge_seg(96, R, 96, 64 - R)
edge_seg(96 - R, 64, R, 64)
edge_seg(0, 64 - R, 0, R)
edge_arc(96 - R, 0, 96 - K, K, 96, R)        # esquina sup. der.
edge_arc(96, 64 - R, 96 - K, 64 - K, 96 - R, 64)  # inf. der.
edge_arc(R, 64, K, 64 - K, 0, 64 - R)        # inf. izq.
edge_arc(0, R, K, K, R, 0)                   # sup. izq.

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

# ---------- pistas (TRACKS se completa/ajusta en Task 6) ----------
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
# marco y orientacion del Blend Micro en la cara inferior
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
```

- [ ] **Step 2: Ejecutar y verificar asserts**

```powershell
& "C:\Program Files\KiCad\9.0\bin\python.exe" C:\redbearpad\gamepad-pcb\scripts\gen_pcb.py
```
Expected: `PCB escrita: ...` sin AssertionError. Si un assert de posición falla,
revisar el flip de J1/J2 (el orden Flip→SetPosition importa: flip primero, luego
posición) o el origen del footprint PinSocket (pin 1 debe quedar en la posición dada).

- [ ] **Step 3: Commit**

```powershell
git -C C:\redbearpad\gamepad-pcb add -A
git -C C:\redbearpad\gamepad-pcb commit -m "PCB generada: placement verificado, contorno, zonas, rutas iniciales"
```

---

### Task 6: DRC limpio

**Files:**
- Modify: `scripts/gen_pcb.py` (solo la lista `TRACKS` y, si hace falta, vías de costura)
- Regenerate: `redbearpad.kicad_pcb`

- [ ] **Step 1: Ejecutar DRC**

```powershell
& "C:\Program Files\KiCad\9.0\bin\kicad-cli.exe" pcb drc --severity-error --exit-code-violations --format json --output C:\redbearpad\gamepad-pcb\drc.json C:\redbearpad\gamepad-pcb\redbearpad.kicad_pcb
Get-Content C:\redbearpad\gamepad-pcb\drc.json
```
Expected (criterio de éxito de la tarea): exit 0, `violations: []` y
`unconnected_items: []`.

- [ ] **Step 2: Iterar rutas hasta DRC limpio**

Si hay violaciones de clearance: mover waypoints en `TRACKS` (ver notas de ruteo en
los Datos maestros: entrar a los pads de columna lateralmente, no cruzar columnas).
Si hay `unconnected_items` de GND (islas de zona): añadir vías de costura cerca de
la isla con este helper (añadir a `gen_pcb.py` antes del fill):

```python
def via(x, y):
    v = pcbnew.PCB_VIA(board)
    v.SetPosition(MM(x, y))
    v.SetDrill(FMM(0.3))
    v.SetWidth(FMM(0.6))
    v.SetLayerPair(pcbnew.F_Cu, pcbnew.B_Cu)
    v.SetNet(net_objs["GND"])
    board.Add(v)
# ejemplo: via(10, 10)
```
Regenerar (`gen_pcb.py`) y repetir el Step 1 hasta exit 0. `drc.json` no se
commitea (añadirlo a `.gitignore` si molesta).

- [ ] **Step 3: Commit**

```powershell
git -C C:\redbearpad\gamepad-pcb add -A
git -C C:\redbearpad\gamepad-pcb commit -m "Ruteo final: DRC sin errores ni unconnected"
```

---

### Task 7: Renders para revisión visual

**Files:**
- Create (generados): `docs/render_top.png`, `docs/render_bottom.png`

- [ ] **Step 1: Generar renders**

```powershell
& "C:\Program Files\KiCad\9.0\bin\kicad-cli.exe" pcb render --side top --output C:\redbearpad\gamepad-pcb\docs\render_top.png C:\redbearpad\gamepad-pcb\redbearpad.kicad_pcb
& "C:\Program Files\KiCad\9.0\bin\kicad-cli.exe" pcb render --side bottom --output C:\redbearpad\gamepad-pcb\docs\render_bottom.png C:\redbearpad\gamepad-pcb\redbearpad.kicad_pcb
```
Expected: dos PNG. Revisarlos (con la herramienta Read) contra el spec:
8 switches en sus posiciones, OLED arriba-izquierda, columnas J1/J2 en cara
inferior con marco "BLEND MICRO"/"USB", serigrafía legible y no superpuesta a pads.
Corregir posiciones de texto en `gen_pcb.py` si algo se solapa, regenerar PCB
(repetir DRC de Task 6 Step 1 tras cualquier regeneración) y re-renderizar.

- [ ] **Step 2: CHECKPOINT — mostrar renders al usuario**

Presentar ambos renders al usuario y esperar su visto bueno antes de exportar
Gerbers. Recordarle el checkpoint físico pendiente: medir los 3 agujeros del
switch real (poste Ø4.4 central; pines en (3.8, 3.8) y (0, 6.5) desde el centro,
Ø1.2) antes de pagar la fabricación.

- [ ] **Step 3: Commit**

```powershell
git -C C:\redbearpad\gamepad-pcb add -A
git -C C:\redbearpad\gamepad-pcb commit -m "Renders top/bottom para revision"
```

---

### Task 8: Gerbers + README

**Files:**
- Create (generados): `fabrication/gerbers/*`, `fabrication/redbearpad_jlcpcb.zip`
- Create: `README.md`

- [ ] **Step 1: Exportar Gerbers y drill**

```powershell
New-Item -ItemType Directory -Force C:\redbearpad\gamepad-pcb\fabrication\gerbers | Out-Null
& "C:\Program Files\KiCad\9.0\bin\kicad-cli.exe" pcb export gerbers --output C:\redbearpad\gamepad-pcb\fabrication\gerbers\ --layers "F.Cu,B.Cu,F.Mask,B.Mask,F.Silkscreen,B.Silkscreen,Edge.Cuts" --subtract-soldermask C:\redbearpad\gamepad-pcb\redbearpad.kicad_pcb
& "C:\Program Files\KiCad\9.0\bin\kicad-cli.exe" pcb export drill --output C:\redbearpad\gamepad-pcb\fabrication\gerbers\ --format excellon --excellon-units mm --drill-origin absolute C:\redbearpad\gamepad-pcb\redbearpad.kicad_pcb
```
Expected: archivos `.gbr` (7 capas) + `.drl` (PTH y NPTH) en `fabrication/gerbers/`.

- [ ] **Step 2: Empaquetar ZIP**

```powershell
Compress-Archive -Force -Path C:\redbearpad\gamepad-pcb\fabrication\gerbers\* -DestinationPath C:\redbearpad\gamepad-pcb\fabrication\redbearpad_jlcpcb.zip
```

- [ ] **Step 3: Escribir `README.md`**

Contenido completo:
```markdown
# RedBearPad — Gamepad de 8 botones con OLED para RedBearLab Blend Micro

PCB de 96 × 64 mm. Gamepad USB HID: 8 switches Redragon SMD RGB MX Low Profile
cableados directo a GPIO (pull-up interno), pantalla OLED 128×32 I2C, y la
Blend Micro V1.0 enchufada en la cara inferior (headers hembra 2×14, USB hacia
el borde superior).

## Archivos

- `redbearpad.kicad_pro/.kicad_sch/.kicad_pcb` — proyecto KiCad 9.
- `scripts/gen_sch.py`, `scripts/gen_pcb.py` — regeneran esquemático y PCB
  (ejecutar con el Python de KiCad: `C:\Program Files\KiCad\9.0\bin\python.exe`).
- `fabrication/redbearpad_jlcpcb.zip` — Gerbers + drill listos para JLCPCB.
- `docs/superpowers/specs/` — diseño aprobado; `docs/render_*.png` — vistas previas.

## Pedido en JLCPCB

1. jlcpcb.com → "Order now" → subir `fabrication/redbearpad_jlcpcb.zip`.
2. Opciones: 2 capas, 1.6 mm, HASL (todo por defecto sirve). Sin ensamblado.

## ⚠️ Antes de pagar

Medir con calibrador un switch real contra el footprint: poste central Ø4.4 mm
en el centro; pines a (3.8, 3.8) y (0, 6.5) mm del centro, Ø1.2 mm. Si no
coincide, ajustar `libs/redbearpad.pretty/SW_Redragon_LowProfile_PCB_1.00u.kicad_mod`
y regenerar.

## Conexiones (firmware)

| Función | Pin Arduino |
|---|---|
| SW1–SW6 | D5, D8, D9, D10, D11, D12 (`INPUT_PULLUP`, activo bajo) |
| SW7, SW8 | A0, A1 (`INPUT_PULLUP`, activo bajo) |
| OLED SDA/SCL | D2 / D3 (I2C, 3.3 V) |

Placa: usar el paquete `redbearlab:avr` (ver repo blend-micro-boards).

## Créditos

Footprint del switch: [rgoulter/keyboard-labs](https://github.com/rgoulter/keyboard-labs).
```

- [ ] **Step 4: Commit final**

```powershell
git -C C:\redbearpad\gamepad-pcb add -A
git -C C:\redbearpad\gamepad-pcb commit -m "Gerbers JLCPCB + README"
```
