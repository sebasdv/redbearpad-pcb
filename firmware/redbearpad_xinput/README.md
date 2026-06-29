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

Motivo: la Blend Micro tiene cristal de **16 MHz**. El variant Micro configura el PLL USB para
16 MHz → enumera bien. El variant Pro Micro 8 MHz asume cristal de 8 MHz → no enumera.
Contrapartida: corre el CPU a 16 MHz @ 3.3 V (overclock fuera de spec). En la prueba funcionó;
si algún día glitchea, habría que hacer un variant XInput a medida a 8 MHz.

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

XInput no expone CDC → **no hay monitor serie**. Diagnóstico = el **LED 13 parpadea ~1 Hz**
("firmware vivo") y la observación en Windows (`joy.cpl`, hardwaretester.com/gamepad, Steam).

## Mapeo

| Switch | Función Xbox |
|---|---|
| SW2 / SW3 / SW4 / SW5 | Stick izquierdo + D-pad (arriba/izq/abajo/der, con diagonales) |
| SW1 | A |
| SW6 | B |
| SW7 | X |
| SW8 | Y |

## Probar

- **Windows:** `joy.cpl` → "Controller (XBOX 360 For Windows)" → botones, dpad y stick.
- **Steam:** Configuración → Controlador → aparece como mando Xbox; o directo en un juego.

## No incluye (a futuro)

- OLED (se podría reincorporar el visualizador de mando vía I2C, event-driven, convive con XInput).
- Gatillos analógicos, segundo stick, Start/Back/LB/RB (hay solo 8 switches).
