# RedBearPad — Gamepad de 8 botones con OLED para RedBearLab Blend Micro

PCB de 96 × 64 mm. Gamepad USB HID: 8 switches Redragon SMD RGB MX Low Profile
cableados directo a GPIO (pull-up interno), pantalla OLED 128×32 I2C, y la
Blend Micro V1.0 enchufada en la cara inferior (headers hembra 2×14, USB hacia
el borde superior).

## Archivos

- `redbearpad.kicad_pro/.kicad_sch/.kicad_pcb` — proyecto KiCad 9.
- `scripts/gen_sch.py`, `scripts/gen_pcb.py` — regeneran esquemático y PCB
  (ejecutar con el Python de KiCad: `...\KiCad\9.0\bin\python.exe`).
- `scripts/verify_netlist.py` — valida el netlist contra el contrato de diseño.
- `fabrication/redbearpad_jlcpcb.zip` — Gerbers + drill listos para JLCPCB.
- `docs/superpowers/specs/` — diseño aprobado; `docs/render_*.png` — vistas previas.

## Pedido en JLCPCB

1. jlcpcb.com → "Order now" → subir `fabrication/redbearpad_jlcpcb.zip`.
2. Opciones: 2 capas, 1.6 mm, HASL (todo por defecto sirve). Sin ensamblado.

## ⚠️ Antes de pagar

Medir con pie de metro un switch real contra el footprint: poste central Ø4.4 mm
en el centro; pines a (3.8, 3.8) y (0, 6.5) mm del centro, Ø1.2 mm. Si no
coincide, ajustar `libs/redbearpad.pretty/SW_Redragon_LowProfile_PCB_1.00u.kicad_mod`
y regenerar (`gen_pcb.py`).

## Conexiones (firmware)

| Función | Pin Arduino |
|---|---|
| SW1–SW6 | D5, D8, D9, D10, D11, D12 (`INPUT_PULLUP`, activo bajo) |
| SW7, SW8 | A0, A1 (`INPUT_PULLUP`, activo bajo) |
| OLED SDA/SCL | D2 / D3 (I2C, 3.3 V) |

Placa: usar el paquete `redbearlab:avr` (ver repo blend-micro-boards).

## Créditos

Footprint del switch: [rgoulter/keyboard-labs](https://github.com/rgoulter/keyboard-labs).
