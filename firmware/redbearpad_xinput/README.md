# Firmware XInput (mando Xbox 360) — RedBearPad

Sketch `redbearpad_xinput/redbearpad_xinput.ino`: hace que la placa **enumere como un mando
Xbox 360 (XInput)**, que Steam y los juegos reconocen **nativamente**.

## Por qué XInput

El firmware gamepad HID genérico funciona en `joy.cpl` pero **muchos juegos solo aceptan
mandos XInput (Xbox)** y vía Steam Input no había caso. XInput resuelve esto de raíz: la placa
ES un mando Xbox para Windows. ✅ Verificado: Steam lo reconoce.

## Build — NO es el setup normal, leer esto

Este sketch **no** se compila con el paquete `redbearlab:avr`. Usa el core XInput.

### 1. Instalar el soporte XInput (una vez)

- Arduino IDE → **Archivo → Preferencias → URLs adicionales de gestor de placas**, añadir:
  ```
  https://raw.githubusercontent.com/dmadison/ArduinoXInput_Boards/master/package_dmadison_xinput_index.json
  ```
- **Herramientas → Placa → Gestor de placas** → instalar **"XInput AVR Boards"** (dmadison).
- **Herramientas → Gestor de librerías** → instalar **"XInput"** (David Madison).

### 2. Placa correcta: "Arduino Micro w/ XInput"

**Selecciona `Arduino Micro w/ XInput`** (FQBN `xinput:avr:micro`), NO el Pro Micro 8 MHz.

Motivo: la Blend Micro tiene cristal de **16 MHz**. `USBCore.cpp` fija el bit `PINDIV` del PLL
**en tiempo de compilación** según `F_CPU`: con `F_CPU=16000000` lo activa (divide el cristal
de 16 MHz por 2 → 8 MHz de entrada al PLL → 48 MHz de USB, correcto). Cualquier variant de
8 MHz —el Pro Micro, o el core `blendmicro8`— lo *limpia*, asumiendo un cristal de 8 MHz; con
el cristal real de 16 MHz el PLL sale a 96 MHz y **el USB no enumera** (ni siquiera aparece
como dispositivo desconocido).

⚠️ **Por eso este sketch no se compila con `blendmicro8`**, aunque sea el core correcto para el
keypad y el diagnóstico. Si lo haces, la placa queda muda en USB.

### Overclock: resuelto con prescaler en runtime

El variant Micro corre el CPU a 16 MHz @ 3.3 V, que está **fuera de spec** (el 32U4 a 3.3 V
sólo llega a ~10.7 MHz) y glitcheaba al calentar, acoplando botones vecinos tras ~5 min.

Solución: `setup()` baja el CPU a 8 MHz con el prescaler (`CLKPR`), **sin tocar el PLL**. Como
`PINDIV` ya quedó bien fijado al compilar, el USB sigue a 48 MHz y el mando enumera como Xbox
360, pero el CPU vuelve a estar en spec. Verificado: 30 min de Steam sin cuelgues ni fantasmas.

Contrapartida: `millis()` y todo el timing corren a la mitad → muestreo ~2 ms, debounce ~24 ms
y **heartbeat del LED 13 a ~0.5 Hz** (si lo ves a 1 Hz, el prescaler no se aplicó).

### 3. Subir

```powershell
$cli = "C:\redbearpad\tools\arduino-cli\arduino-cli.exe"
& $cli compile --fqbn xinput:avr:micro firmware\redbearpad_xinput
& $cli upload  --fqbn xinput:avr:micro -p COM<N> firmware\redbearpad_xinput
```

O desde el IDE: placa "Arduino Micro w/ XInput", puerto COM, Subir.

## ⚠️ Re-flasheo: doble-reset manual

Una vez corriendo XInput, la placa **ya no es un dispositivo serie (CDC)**, así que el
auto-reset por toque a 1200 bps **no funciona**. Para volver a subir cualquier firmware
(otra versión de XInput, o volver al gamepad/keypad):

1. Inicia la subida en el IDE.
2. Pulsa **reset dos veces rápido** en la placa → entra al bootloader Caterina (~8 s).
3. La subida arranca en esa ventana.

## Sin Serial Monitor

XInput no expone CDC → **no hay monitor serie**. Diagnóstico = el **LED 13 parpadea ~0.5 Hz**
("firmware vivo", a la mitad por el prescaler) y la observación en Windows (`joy.cpl`,
hardwaretester.com/gamepad, Steam). Para diagnóstico con serie, usar
`firmware/diagnostico_pines/` con el core `blendmicro8`.

## Mapeo (PCB v2, 12 botones)

| Switch | Posición en la placa | Pin | Función Xbox |
|---|---|---|---|
| SW1 / SW2 / SW3 / SW4 | cruceta (arriba/abajo/izq/der) | D3 / D1 / D2 / D0 | Stick izq. + D-pad |
| SW5 | cara, arriba-izq | D11 | X |
| SW6 | cara, arriba-der | D12 | Y |
| SW7 | cara, abajo-izq | A0 | A |
| SW8 | cara, abajo-der | A1 | B |
| SW9 / SW10 | hombros | D8 / D9 | LB / RB |
| SW11 / SW12 | tact centrales | D5 / D10 | BACK / START |

Los 4 botones de cara forman un cuadrado 2×2, así que se asignan como el diamante Xbox girado
45° (`X Y` arriba, `A B` abajo), respetando **Y arriba, A abajo, X izquierda, B derecha**.

## Probar

- **Windows:** `joy.cpl` → "Controller (XBOX 360 For Windows)" → botones, dpad y stick.
- **Steam:** Configuración → Controlador → aparece como mando Xbox; o directo en un juego.

## No incluye (a futuro)

- OLED (la v2 no la lleva; se podría reincorporar el visualizador de mando vía I2C,
  event-driven, que convive con XInput).
- Gatillos analógicos y segundo stick (la PCB v2 sólo tiene entradas digitales).
- Variant XInput a medida de 8 MHz: quedó **innecesario**, el prescaler en runtime resuelve
  el overclock sin salir del variant Micro de fábrica.
