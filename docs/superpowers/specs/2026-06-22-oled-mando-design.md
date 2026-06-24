# Visualizador de mando en la OLED — Diseño

**Fecha:** 2026-06-22
**Estado:** Aprobado (maqueta validada por el usuario)

## Problema

Tras descartar el mini-juego del dino (se colgaba por glitches de I2C al hacer redibujo
continuo), se vuelve a un visualizador de botones en la OLED, pero **más atractivo** que la
rejilla de texto anterior (`dibujarPantalla` en `master`).

## Solución

Dibujar un **mini-mando** en la OLED 128x32: cruceta (cruz) a la izquierda y 4 botones de
acción (círculos en diamante) a la derecha, que se **encienden** (se rellenan) según el estado
de los switches. Redibujo **solo al cambiar** (event-driven, como la versión anterior que
nunca se colgó) → muy poca I2C, seguro. Reemplaza el cuerpo de `dibujarPantalla()`; el resto
del firmware gamepad (HID, debounce, diagnósticos) queda igual.

## Componentes

### 1. `dibujarPantalla()` (reescrita)

OLED 128x32, todo en blanco sobre negro. Sin título (más grande y limpio).

- **Cruceta (cruz)** a la izquierda, 5 segmentos:
  - arriba: rect (18,3) 8x9 — `estadoPrev[IDX_UP]`
  - abajo:  rect (18,20) 8x9 — `estadoPrev[IDX_DOWN]`
  - izquierda: rect (6,12) 9x8 — `estadoPrev[IDX_LEFT]`
  - derecha: rect (29,12) 9x8 — `estadoPrev[IDX_RIGHT]`
  - centro (hub): rect (18,12) 8x8 — siempre contorno
  Cada brazo: `fillRect` si pulsado, `drawRect` si no.
- **Botones de acción** (círculos r=6) en diamante a la derecha, numerados 1–4:
  - izquierda (B1): centro (86,16) — `estadoPrev[IDX_BTN[0]]`
  - abajo (B2): centro (100,25) — `estadoPrev[IDX_BTN[1]]`
  - derecha (B3): centro (114,16) — `estadoPrev[IDX_BTN[2]]`
  - arriba (B4): centro (100,6) — `estadoPrev[IDX_BTN[3]]`
  Cada botón: `fillCircle` + número en negro si pulsado; `drawCircle` + número en blanco si no.

Dos helpers para no repetir: `dibujarArma(x,y,w,h,on)` y `dibujarBotonCirc(cx,cy,num,on)`.

### 2. Robustez de I2C (carryover del debug del dino)

`oled.begin()` llama internamente a `Wire.begin()` y **pisa** el `Wire.setWireTimeout`. Se
**reaplica el timeout después de `oled.begin()`** para que un glitch de la OLED soldada a mano
aborte (25 ms) en vez de colgar el loop. Se mantiene el reloj I2C por defecto (100 kHz): como
el redibujo es solo al cambiar, la velocidad no importa y 100 kHz es lo más estable.

## Manejo de errores

- OLED ausente (`oledOk == false`): no se dibuja; el gamepad sigue funcionando.
- Bus I2C trabado: el timeout (ahora activo durante el runtime) aborta la transacción.

## Verificación

1. Compila para `redbearlab:avr:blendmicro8` sin errores.
2. En hardware (usuario): al pulsar direcciones se encienden los brazos de la cruz (incluidas
   diagonales) y al pulsar B1–B4 se rellenan los círculos correspondientes; el gamepad sigue
   respondiendo en `joy.cpl`.

## Fuera de alcance

- Animaciones continuas, juego, título, sonido. (El redibujo es solo al cambiar, a propósito.)
