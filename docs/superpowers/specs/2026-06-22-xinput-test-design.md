# Prueba XInput (mando Xbox) en la Blend Micro — Diseño

**Fecha:** 2026-06-22
**Estado:** Aprobado (experimento de viabilidad)

## Problema

El gamepad HID genérico funciona en Windows/joy.cpl pero los juegos vía Steam Input "no lo
ven" (problema DirectInput↔XInput). Se quiere que la placa **enumere como mando Xbox 360
(XInput)** para que cualquier juego lo reconozca. Antes de invertir en un board-variant a
medida, se **prueba el variant de fábrica** a ver si enumera en la Blend Micro.

## Solución (prueba)

Sketch mínimo de XInput con la librería `XInput` (dmadison) compilado para el variant
**`xinput:avr:micro`** ("Arduino Micro w/ XInput"). Se elige el de 16 MHz (no el Pro Micro
8 MHz) porque su PLL USB está configurado para un cristal de **16 MHz** — el de la Blend
Micro — así que es el que tiene chance de **enumerar**. El precio: corre a 16 MHz @ 3.3 V
(overclock), que puede glitchear; si enumera pero es inestable, el siguiente paso es un
variant a medida de 8 MHz.

## Alcance

- **Incluye:** sketch XInput mínimo (8 switches → stick + dpad + A/B/X/Y), debounce por
  integrador reutilizado, LED de heartbeat, compilación para `xinput:avr:micro`.
- **No incluye:** OLED, serial (XInput no tiene CDC), variant a medida, mapeos avanzados
  (gatillos, segundo stick). Es una prueba de enumeración.

## Estructura de archivos

```
firmware/redbearpad_xinput/redbearpad_xinput.ino   (NUEVO; sketch independiente)
```

No se tocan los sketches keypad ni gamepad.

## Componentes

### Sketch `redbearpad_xinput.ino`

- `#include <XInput.h>`; `XInput.begin()` en `setup()`, `XInput.setAutoSend(false)` (un
  reporte por ciclo con `XInput.send()`).
- Lectura de los 8 switches con el **debounce por integrador + muestreo fijo 1 ms** (igual
  que el gamepad).
- Mapeo Xbox (al cambiar algún estado):
  - Direcciones SW2/3/4/5 → **stick izquierdo** con `setJoystick(JOY_LEFT, up,down,left,
    right)` (deflexión completa, diagonales por SOCD) **y** `setDpad(up,down,left,right)`.
  - SW1/SW6/SW7/SW8 → `setButton(BUTTON_A/B/X/Y, ...)`.
- **LED 13 parpadea** (~1 Hz) como señal de "firmware vivo", ya que no hay serial.

## Restricciones operativas (importantes)

- **Sin Serial Monitor:** XInput no expone CDC; no hay diagnóstico por serie.
- **Re-flasheo manual:** tras correr XInput, la placa ya no entra sola al bootloader; hay que
  **doble-reset manual** (ventana ~8 s del Caterina) para volver a subir firmware.
- **Subida de ESTA prueba:** se hace sobre el firmware actual (que es CDC), así que el toque
  1200 bps aún funciona para este primer flasheo.

## Verificación

1. Compila para `xinput:avr:micro` sin errores (lo hace el agente).
2. En hardware (usuario): subir, y comprobar en Windows que aparece un **mando Xbox 360**
   (en `joy.cpl`, hardwaretester.com/gamepad o Steam → Controlador). Pulsar botones y
   direcciones para ver A/B/X/Y, dpad y stick.

## Resultado esperado y bifurcación

- **Enumera y responde estable** → ¡listo! Se documenta y se puede pulir (mapeo, etc.).
- **Enumera pero glitchea** (overclock 16 MHz @ 3.3 V) → siguiente: variant XInput a medida
  a 8 MHz con la corrección de PLL de la Blend Micro.
- **No enumera** → el variant de fábrica no sirve; ir directo al variant a medida.
