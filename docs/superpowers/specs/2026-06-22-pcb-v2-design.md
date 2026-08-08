# RedBearPad v2 — Rediseño del PCB (mando XInput de 12 botones) — Diseño

**Fecha:** 2026-06-22
**Estado:** Aprobado
**Rama:** `pcb-rediseno`

## Problema

El RedBearPad v1 tiene 8 switches + OLED. Se quiere un mando orientado a juegos de
plataforma con **12 entradas** (estilo SNES) y **sin OLED**, manteniendo el mismo tamaño de
board (96×64 mm) y la Blend Micro como cerebro. El firmware destino es **XInput** (mando
Xbox 360), que ya funciona en la v1.

El layout y el mapeo de funciones fueron definidos por el usuario sobre un DXF modificado
(`redbearpad_actual_MOD.dxf`, analizado y conservado en el repo como referencia).

## Alcance

- **Incluye:** regenerar el PCB (esquemático + ruteo) a 12 entradas con el layout/mapeo
  aprobado; agregar el footprint del tact CAX; actualizar `verify_netlist.py`; extender el
  firmware XInput a 12 botones.
- **No incluye:** OLED, BLE, cambios de tamaño de board, segundo stick o gatillos analógicos.

## Hardware

### Board
- Contorno **96 × 64 mm**, esquinas r=1 (igual que v1).
- **4 agujeros de montaje** Ø3.1 en las esquinas (conservados).
- **Blend Micro** enchufada por debajo: headers hembra **2×14** en x≈40.4 y x≈55.6
  (conservados, mismo cableado de alimentación/USB que v1).
- **OLED eliminada** (se quita el conector J3 y sus nets SDA/SCL/VCC).

### Entradas (12) — posiciones (x, y desde arriba, mm) y mapeo

| Función | Posición | Componente |
|---|---|---|
| L | (16, 16) | switch Redragon low-profile 1u |
| R | (80, 16) | switch Redragon |
| Select | (32, 16) | **tact CAX C39832249** (4.5×4.5, DIP 4P) |
| Start | (64, 16) | **tact CAX C39832249** |
| Arriba | (32, 32) | switch Redragon |
| A | (64, 32) | switch Redragon |
| B | (80, 32) | switch Redragon |
| Izquierda | (16, 48) | switch Redragon |
| Abajo | (32, 48) | switch Redragon |
| Derecha | (48, 48) | switch Redragon |
| X | (64, 48) | switch Redragon |
| Y | (80, 48) | switch Redragon |

**Total: 10 switches Redragon + 2 tact CAX.** Espaciado en grilla de **16 mm** (igual que v1;
keycaps 14.2 mm → 1.8 mm de aire, courtyards 1u se solapan levemente, aceptado por el usuario).

Cada entrada es un pin a GND con pull-up interno (activo en LOW), igual que v1.

### Asignación de pines (GPIO)

12 GPIO del set limpio del ATmega32U4, **evitando** D6/D7 (nRF8001), D13 (LED) y los de SPI
(MISO/MOSI/SCK/SS). Candidatos: D0, D1, D2, D3, D5, D8, D9, D10, D11, D12, A0–A5.

La asignación exacta switch→pin la **fija la generación del PCB** buscando ruteo limpio y
**evitando adyacencias problemáticas** (en v1 un puente D9↔D10 cruzaba izquierda/abajo). El
netlist resultante es el contrato; el firmware se ajusta a esa asignación.

## Firmware (XInput, extender el existente)

Base: `firmware/redbearpad_xinput/redbearpad_xinput.ino`. Cambios:
- 12 entradas con el debounce por integrador (igual patrón).
- Cruceta (Arriba/Abajo/Izq/Der) → **stick izquierdo** (`setJoystick`, diagonales por SOCD)
  **y** D-pad (`setDpad`).
- A/B/X/Y → `setButton(BUTTON_A/B/X/Y, ...)`.
- L/R → `setButton(BUTTON_LB/RB, ...)`.
- Select → `BUTTON_BACK`; Start → `BUTTON_START`.
- LED 13 heartbeat (sin serial). Compila para `xinput:avr:micro`.

## Componentes / librería

- **Footprint nuevo:** tact CAX `C39832249` (4.5×4.5 mm, DIP, 4 patas) en
  `libs/redbearpad.pretty/`. Dimensiones del datasheet LCSC (cuerpo 4.5×4.5, paso de patas a
  confirmar del datasheet).
- Footprint del switch Redragon: el existente (`SW_Redragon_LowProfile_PCB_1.00u`).

## Generación y verificación

- Actualizar `scripts/gen_sch.py` y `scripts/gen_pcb.py` para producir el esquemático y el PCB
  v2 (12 entradas, footprints y posiciones nuevas, OLED fuera, mapeo de pines).
- Actualizar `scripts/verify_netlist.py` con el contrato de 12 entradas (cada switch a su pin +
  GND; sin nets de OLED).
- Verificación: ERC 0, DRC 0, `verify_netlist.py` OK; gerbers JLCPCB regenerados.
- Firmware: compila para `xinput:avr:micro`; el usuario prueba en Steam (mando Xbox de 12).

## Notas de diseño

- Cruceta: Arriba (32,32) encima de Abajo (32,48); Izq/Der en la fila y48. Decisión ergonómica
  del usuario (confirmada), no un error.
- El DXF `redbearpad_actual_MOD.dxf` queda como referencia del layout físico aprobado.

## Fuera de alcance / futuro

- Carcasa 3D, segundo stick, gatillos analógicos, RGB.
