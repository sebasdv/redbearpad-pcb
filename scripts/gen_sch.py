"""Genera redbearpad.kicad_sch (formato KiCad 8, legible por KiCad 9).
Extrae los simbolos de las librerias estandar instaladas y conecta los pines
colocando global labels exactamente en el punto de conexion de cada pin
(sin wires). Pines no usados llevan no_connect.
"""
import os, re, uuid

KICAD_SYMS = r"C:\Users\sebastian.duarte\AppData\Local\Programs\KiCad\9.0\share\kicad\symbols"
HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.normpath(os.path.join(HERE, "..", "redbearpad.kicad_sch"))
ROOT_UUID = "f0a11111-2222-3333-4444-555566667777"

NAMESPACE = uuid.uuid5(uuid.NAMESPACE_URL, "redbearpad-gamepad-pcb")
_uuid_counter = 0

def U():
    global _uuid_counter
    _uuid_counter += 1
    return str(uuid.uuid5(NAMESPACE, str(_uuid_counter)))

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

# 12 entradas (sin OLED). Cada SWn a su pin de header segun el contrato v2.
J1_NETS = ["GND", "GND", None, None, None, None, None, None,
           "SW4", "SW2", "SW3", "SW1", "SW11", "SW9"]
J2_NETS = ["GND", None, None, None, None, None, None, "SW8",
           "SW7", None, "SW6", "SW5", "SW12", "SW10"]

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

# Reset counter for deterministic UUID generation
_uuid_counter = 0

SW_FP_MX   = "redbearpad:SW_Redragon_LowProfile_PCB_1.00u"
SW_FP_TACT = "redbearpad:SW_Tact_CAX_4.5x4.5"
TACT_REFS = {"SW11", "SW12"}
for k in range(12):
    ref = "SW%d" % (k + 1)
    x, y = 63.5, 25.4 + 15.24 * k
    fp = SW_FP_TACT if ref in TACT_REFS else SW_FP_MX
    body.append(sym_instance("Switch:SW_Push", ref, "Tact_CAX" if ref in TACT_REFS else "Redragon_LP",
                             fp, x, y, ["1", "2"]))
    for num, net in (("1", ref), ("2", "GND")):
        px, py = sw_pins[num]
        body.append(glabel(net, x + px, y - py, left=px < 0))

place_conn("J1", "BlendMicro_fila_izq",
           "Connector_PinSocket_2.54mm:PinSocket_1x14_P2.54mm_Vertical",
           "Connector_Generic:Conn_01x14", c14_pins, J1_NETS, 152.4, 63.5)
place_conn("J2", "BlendMicro_fila_der",
           "Connector_PinSocket_2.54mm:PinSocket_1x14_P2.54mm_Vertical",
           "Connector_Generic:Conn_01x14", c14_pins, J2_NETS, 203.2, 63.5)

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
