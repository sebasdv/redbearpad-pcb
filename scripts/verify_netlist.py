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
for net in set(found) - set(EXPECTED):
    if len(found[net]) > 1:
        print("Net extra con >1 nodo:", net, sorted(found[net]))
        ok = False
print("NETLIST OK" if ok else "NETLIST CON ERRORES")
sys.exit(0 if ok else 1)
