# RedBearPad — Gamepad PCB de 8 botones con OLED — Diseño

**Fecha:** 2026-06-12
**Estado:** Aprobado
**Herramienta:** KiCad 9.0.x (instalado), verificación con `kicad-cli`

## Objetivo

PCB de un gamepad USB HID de 8 botones con pantalla OLED, controlado por una placa
RedBearLab Blend Micro V1.0 (ATmega32U4 + nRF8001, 3.3 V) montada como módulo
enchufable. Entrega lista para fabricar en JLCPCB (PCB desnuda, sin ensamblado SMD:
el diseño no lleva componentes SMD). El firmware HID queda fuera de alcance.

## Restricciones de partida (del usuario)

- Placa de **96 × 64 mm**, geometría según `C:\redbearpad\REFERENCE_PCB.dxf`.
- Switches **Redragon SMD RGB MX Low Profile** (3 pines, ya comprados). Sin LEDs
  en la PCB (decisión: se eliminaron los LEDs del diseño).
- Pantalla **OLED 128×32 px, 0.91"/0.94", I2C de 4 pines** (ya comprada).
- Blend Micro montado **en la cara inferior**, enchufable (headers hembra).

## Convención de coordenadas

El DXF de referencia usa origen en la esquina superior izquierda con Y negativa
hacia abajo. En KiCad se usa la misma geometría con **Y positiva hacia abajo**:
`(x_kicad, y_kicad) = (x_dxf, −y_dxf)`. Todas las coordenadas de este documento
están en formato KiCad (mm), origen en la esquina superior izquierda de la placa.

## Geometría (fiel al DXF de referencia)

- **Contorno:** rectángulo 96 × 64 mm, esquinas redondeadas r = 1 mm (capa Edge.Cuts).
- **4 perforaciones de montaje M3:** Ø3.1 mm NPTH en (4,4), (92,4), (4,60), (92,60).
- **8 switches** (centro del poste):

| Ref | Posición (x, y) | Clúster |
|---|---|---|
| SW1 | (16, 16) | izquierdo |
| SW2 | (32, 32) | izquierdo |
| SW3 | (16, 48) | izquierdo |
| SW4 | (32, 48) | izquierdo |
| SW5 | (48, 48) | central-inferior |
| SW6 | (64, 32) | derecho |
| SW7 | (80, 32) | derecho |
| SW8 | (80, 48) | derecho |

- **Zona OLED (cara superior):** x 29–66, y 8–19 (37 × 11 mm). Header hembra de
  4 pines paso 2.54 mm; el módulo OLED queda en voladizo sobre la zona.
- **Zona Blend Micro (cara inferior):** x 38.5–57.5, y 0–42, centrada en x = 48.
  El conector micro-USB del módulo queda en el borde superior de la placa (y = 0)
  para el acceso del cable.

## Arquitectura eléctrica

Sin matriz y sin diodos: cada switch conecta un GPIO a GND; el firmware usa
`INPUT_PULLUP`. No hay componentes SMD ni pasivos en la PCB — solo conectores,
switches y perforaciones.

### Asignación de pines

| Señal | Pin Blend Micro | Nota |
|---|---|---|
| SW1 | D5 | |
| SW2 | D8 | |
| SW3 | D9 | |
| SW4 | D10 | |
| SW5 | D11 | |
| SW6 | D12 | |
| SW7 | A0 | digital con pull-up |
| SW8 | A1 | digital con pull-up |
| OLED SDA | D2 (SDA) | I2C hardware |
| OLED SCL | D3 (SCL) | I2C hardware |
| OLED VCC | V33 (3.3 V) | regulador del módulo |
| OLED GND | GND | |

Se evitan deliberadamente: D0/D1 (serial libre para debug), D13 (LED onboard
interfiere con pull-up), D4/D6/D7 y SPI (reservados por el BLE nRF8001 interno).

## Footprints

### Switch Redragon Low Profile (3 pines)

Fuente: `rgoulter/keyboard-labs` (`pcb/ProjectLocal.pretty/`), usado en un teclado
real fabricado con estos switches (fork `Apiratchai/cheapis-redragon-lowprofile`).
Copia local ya guardada en `libs/footprints/SW_Redragon_LowProfile_PCB_1.00u.kicad_mod`.

- Poste central: NPTH Ø4.4 mm en (0, 0) del footprint.
- Pin 1: (3.8, 3.8), taladro Ø1.2 mm, pad Ø2.2 mm.
- Pin 2: (0, 6.5), taladro Ø1.2 mm, pad Ø2.2 mm.

### Zócalo Blend Micro (footprint propio, a crear)

Datos extraídos del DXF oficial (`upstream/Blend/PCB/Blend_Micro.dxf`):

- Módulo: 18.4 × 42.09 mm. Pines: 2 filas × 14, paso 2.54 mm, separación entre
  filas 15.24 mm (0.6"), taladros para header hembra estándar (Ø1.0 mm, pad Ø1.7 mm).
- En coordenadas de placa: filas en x = 40.38 y x = 55.62; pines en
  y = 1.667 + n·2.54 (n = 0…13), es decir y 1.667 … 34.687. El extremo USB del
  módulo coincide con el borde superior (y = 0); solo el conector micro-USB
  sobresale (~1 mm).
- **El footprint se coloca en la cara inferior (B.Cu)**: al estar el módulo boca
  abajo, las columnas izquierda/derecha se ven espejadas desde la cara superior.
  El mapeo de señales del footprint debe construirse explícitamente espejado y
  verificarse contra `BlendMicro_Pins.png` (columna izquierda del módulo desde el
  USB hacia arriba: GND, GND, VIN, SS, CLK, MOSI, MISO, /RST, D0, D1, D2, D3, D5,
  D8 — columna derecha desde el USB: GND, V33, AREF, A5, A4, A3, A2, A1, A0, D13,
  D12, D11, D10, D9).
- Serigrafía en ambas caras indicando orientación del módulo y posición del USB.

### Header OLED

Header hembra 1×4, paso 2.54 mm, en la cara superior. El módulo comprado tiene
los pines en el **borde corto izquierdo** (columna vertical). El orden de header
en la placa, verificado contra el ajuste físico del módulo, es de arriba hacia
abajo **SDA, SCL, VCC, GND** (la conexión eléctrica se preserva — SDA→SDA, etc.;
este orden coloca cada pin del módulo en su pad correcto al enchufarlo).

El header se coloca como columna vertical 1×4 en el extremo izquierdo de la zona
SCREEN (x ≈ 30, pines centrados verticalmente en la zona: y = 9.7, 12.24, 14.78,
17.32, paso 2.54 mm), con **pin 1 = SDA arriba** y pin 4 = GND abajo. El cuerpo
del módulo queda en voladizo hacia la derecha cubriendo el resto de la zona.

## Reglas de diseño y fabricación

- 2 capas, FR-4 1.6 mm, acabado HASL — perfil estándar JLCPCB.
- Planos de tierra en ambas caras.
- Pistas: 0.25 mm señal, 0.5 mm alimentación (VIN/V33). Clearance 0.2 mm.
- Sin BOM/CPL: pedido de PCB desnuda, solo Gerbers + drill en ZIP formato JLCPCB.

## Estructura del proyecto

```
gamepad-pcb/
├── redbearpad.kicad_pro / .kicad_sch / .kicad_pcb
├── libs/footprints/            # Librería local del proyecto (Redragon SW, zócalo Blend Micro, etc.)
├── fabrication/                # Gerbers + drill (ZIP para JLCPCB), generado por kicad-cli
├── docs/superpowers/specs/     # este documento
└── README.md                   # cómo abrir, validar y pedir a JLCPCB
```

## Verificación

Automatizada en esta máquina con `kicad-cli`:

1. **ERC** sobre el esquemático: sin errores.
2. **DRC** sobre la PCB con reglas compatibles JLCPCB: sin errores.
3. Export de **Gerbers + drill** y empaquetado ZIP.
4. **Renders PNG** de ambas caras (`kicad-cli pcb render`) para revisión visual
   del usuario.

Checkpoints físicos del usuario **antes de enviar a fabricar**:

1. ~~Medir los 3 agujeros del switch Redragon~~ — **resuelto**: el usuario confirmó
   que el footprint coincide con el switch físico.
2. ~~Confirmar el orden de pines del módulo OLED~~ — **resuelto**: orden corregido
   a SDA, SCL, VCC, GND según el ajuste físico del módulo.
3. Revisar los renders (posiciones, serigrafía, orientación del Blend Micro).

## Riesgos

- **Footprint del switch:** es de la comunidad, no de un datasheet oficial.
  Mitigación: proviene de un diseño fabricado y funcional + checkpoint de medición.
- **Espejado del zócalo Blend Micro:** error clásico al montar módulos en la cara
  inferior. Mitigación: mapeo espejado explícito en el footprint, serigrafía de
  orientación y revisión del render inferior.
- ~~**Orden de pines OLED**~~ — riesgo eliminado: orden y posición verificados
  con fotografía del módulo físico.

## Fuera de alcance

- Firmware del gamepad (sketch HID) — proyecto posterior.
- Carcasa/case y keycaps.
- LEDs bajo los switches (eliminados por decisión del usuario).
- Uso del BLE (el diseño no lo bloquea: los pines BLE quedan libres en el módulo).
