# RedBearPad v2 — Mando XInput de 12 botones para RedBearLab Blend Micro

PCB de 96 × 64 mm. Mando estilo SNES orientado a juegos de plataforma que se
enumera como **control de Xbox 360 (XInput)** y Steam reconoce de forma nativa.
12 entradas a GND con pull-up interno (activo en bajo): **10 switches Redragon
SMD RGB MX Low Profile** + **2 tact CAX C39832249** (Start/Select). La Blend
Micro V1.0 va enchufada en la cara inferior (headers hembra 2×14, USB hacia el
borde superior). **Sin OLED** (eliminada respecto de la v1).

## Layout (vista desde arriba, grilla de 16 mm)

```
 L  Sel  ·  Sta  R       (fila y=16)   Sel/Sta = tact CAX
 ·  Up   ·   X   Y       (fila y=32)
 Iz Dn  Der  A   B       (fila y=48)   cruceta: Up arriba de Dn; Izq/Der en y=48
```

## Archivos

- `redbearpad.kicad_pro/.kicad_sch/.kicad_pcb` — proyecto KiCad 9 (ruteado, DRC limpio en cobre).
- `firmware/redbearpad_xinput/` — firmware XInput de 12 botones (el de producción).
- `scripts/gen_sch.py`, `scripts/gen_pcb.py` — regeneran esquemático y placement
  (ejecutar con el Python de KiCad: `...\KiCad\9.0\bin\python.exe`).
  ⚠️ Regenerar el PCB **borra el ruteo**; el ruteo actual está en el `.kicad_pcb` versionado.
- `scripts/verify_netlist.py` — valida el netlist contra el contrato de diseño (12 entradas).
- `scripts/attach_3d_models.py` — adjunta modelos 3D a los footprints ya colocados en el
  `.kicad_pcb` **sin regenerar la placa** (a diferencia de `gen_pcb.py`, no toca el ruteo).
  Solo hace falta si se rutea a mano sobre un board viejo sin modelos; una regeneración
  completa con `gen_pcb.py` ya los trae, porque viven en el `.kicad_mod` de la librería.
- `fabrication/gerbers/` — Gerbers + drill (F.Cu, B.Cu, Edge_Cuts, NPTH/PTH) regenerados.
- `docs/superpowers/specs/` — diseño aprobado de la v2.
- `docs/render_top.png`, `docs/render_bottom.png` — vistas previas 3D (`kicad-cli pcb render`).

## Ruteo

11 de las 12 nets ruteadas con **Freerouting** (autorouter, vía DSN/SES); la net
restante (SW7→A0) se ruteó a mano con una vía para cruzar la pared de pistas de
SW6/SW8. **DRC de cobre: 0 sin conectar, 0 cortos, 0 clearance.** Quedan 8 avisos
de serigrafía sobre los pads de los tact (cosméticos: el fabricante recorta la
silk sobre pads automáticamente).

## Pedido en JLCPCB

1. jlcpcb.com → "Order now" → subir un ZIP con el contenido de `fabrication/gerbers/`.
2. Opciones: 2 capas, 1.6 mm, HASL (todo por defecto sirve). Sin ensamblado.

## ⚠️ Antes de pagar

Medir con pie de metro un switch real contra el footprint: poste central Ø4.4 mm
en el centro; pines a (3.8, 3.8) y (0, 6.5) mm del centro, Ø1.2 mm. Si no
coincide, ajustar `libs/redbearpad.pretty/SW_Redragon_LowProfile_PCB_1.00u.kicad_mod`.
Verificar también el tact `SW_Tact_CAX_4.5x4.5` (cuerpo 4.5×4.5 mm, 4 patas).

## Conexiones (firmware XInput)

| Función | Pin Arduino | Botón XInput |
|---|---|---|
| Arriba / Abajo / Izquierda / Derecha | D3 / D1 / D2 / D0 | stick izq. + D-pad |
| X / Y (cara, fila superior) | D11 / D12 | X / Y |
| A / B (cara, fila inferior) | A0 / A1 | A / B |
| L / R | D8 / D9 | LB / RB |
| Select / Start | D5 / D10 | BACK / START |

Todas en `INPUT_PULLUP`, activas en bajo. La cruceta se manda como stick izquierdo
**y** D-pad para máxima compatibilidad.

Los 4 botones de cara forman un cuadrado 2×2, así que se asignan como el diamante
Xbox girado 45°, respetando las cuatro relaciones de la convención (**Y arriba, A
abajo, X izquierda, B derecha**):

```
 X  Y
 A  B
```

## Compilar y flashear (XInput)

- Placa: **"Arduino Micro w/ XInput"** (FQBN `xinput:avr:micro`, 16 MHz) del paquete
  [ArduinoXInput_Boards](https://raw.githubusercontent.com/dmadison/ArduinoXInput_Boards/master/package_dmadison_xinput_index.json)
  + librería **XInput** (David Madison). El variant 16 MHz calza con el cristal de
  la Blend Micro.
- XInput **no es CDC**: no hay Serial Monitor (LED 13 hace heartbeat). Para
  reflashear, hacer **doble-reset manual** para entrar al bootloader.

## Modelos 3D

Los 10 switches Redragon MX Low Profile tienen modelo 3D
(`libs/redbearpad.3dshapes/SW_Redragon_LowProfile_PCB_1.00u.step`), visible en
`docs/render_*.png` y en el visor 3D de KiCad. **Pendiente:** modelo del tact CAX
C39832249 (Start/Select) y de la Blend Micro (el módulo enchufable no tiene
representación 3D, solo los zócalos J1/J2 vacíos).

## Créditos

Footprint del switch: [rgoulter/keyboard-labs](https://github.com/rgoulter/keyboard-labs).
Modelo 3D del switch: librería "Switches/REDragon_MX_LowProfile_ZT04" (autor "saper").
Core/librería XInput: [dmadison/ArduinoXInput](https://github.com/dmadison/ArduinoXInput).
