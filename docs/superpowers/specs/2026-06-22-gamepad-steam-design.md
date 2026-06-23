# RedBearPad como gamepad HID para Steam — Diseño

**Fecha:** 2026-06-22
**Estado:** Aprobado

## Problema

El RedBearPad (8 switches sobre una RedBearLab Blend Micro, ATmega32U4) funciona hoy
como **teclado USB HID** (`firmware/redbearpad_keypad/redbearpad_keypad.ino`, con
HID-Project + Keyboard). Se quiere una variante que el PC y **Steam** reconozcan como
**gamepad**, para jugar con cruceta + botones en vez de teclas.

## Solución

Sketch **nuevo e independiente** que reusa la misma placa y librería pero enumera como
**gamepad HID genérico** mediante la clase `Gamepad` de HID-Project (ya instalada). No
requiere XInput ni cambios de bootloader. Steam lo reconoce como control genérico y se
configura con **Steam Input**.

El keypad actual se conserva sin tocar; el usuario elige qué sketch flashear.

## Alcance

- **Incluye:** firmware gamepad (cruceta + 4 botones), OLED con estado del mando,
  diagnósticos heredados, verificación de compilación y detección en Windows/Steam.
- **No incluye:** XInput/emulación Xbox, sticks analógicos reales (no hay potenciómetros
  en el hardware), cambios en la PCB, resolver el montaje físico de la OLED (su conexión ya
  quedó funcional; aquí solo se mejora el refresco).

## Estructura de archivos

```
firmware/redbearpad_gamepad/redbearpad_gamepad.ino   (NUEVO; la carpeta ya existe vacía)
docs/superpowers/specs/2026-06-22-gamepad-steam-design.md  (este documento)
```

No se modifica `redbearpad_keypad/`.

## Componentes

### 1. Enumeración HID

- `#include <HID-Project.h>`, objeto global `Gamepad`.
- `Gamepad.begin()` en `setup()`. El dispositivo aparece como gamepad HID; en Windows se
  ve en `joy.cpl`, y en Steam como control genérico ("RedBearPad").

### 2. Lectura de switches (reutilizado del keypad)

Pines (fijados por la PCB), todos `INPUT_PULLUP`, activo en LOW:

| Switch | Pin | Rol gamepad |
|---|---|---|
| SW2 | D8  | Cruceta ARRIBA |
| SW3 | D9  | Cruceta IZQUIERDA |
| SW4 | D10 | Cruceta ABAJO |
| SW5 | D11 | Cruceta DERECHA |
| SW1 | D5  | Botón 1 |
| SW6 | D12 | Botón 2 |
| SW7 | A0  | Botón 3 |
| SW8 | A1  | Botón 4 |

Se reutiliza el **debounce por integrador con histéresis** y el **muestreo a intervalo
fijo (1 ms)** del keypad (es la lógica que hace el filtro independiente de la velocidad
del loop). Cada switch tiene su integrador; el estado filtrado (`estadoPrev[i]`)
alimenta tanto la cruceta como los botones.

### 3. Cruceta (hat) con diagonales

Los 4 estados filtrados de dirección se combinan **cada ciclo** en un único valor de hat
(8 direcciones + centro) y se envía con `Gamepad.dPad1(dir)`:

- Solo ARRIBA → `GAMEPAD_DPAD_UP`; ARRIBA+IZQUIERDA → `GAMEPAD_DPAD_UP_LEFT`; etc.
- Combinaciones opuestas simultáneas (ARRIBA+ABAJO) → se cancelan a centro.
- Sin dirección → `GAMEPAD_DPAD_CENTERED`.

La cruceta se recalcula a partir de los estados, no por eventos sueltos, para que las
diagonales sean correctas.

### 4. Botones de acción

Los 4 switches de acción mapean a `Gamepad.press(n)` / `Gamepad.release(n)` con
`n = 1..4`, disparados al cambiar el estado filtrado correspondiente.

### 5. Envío del reporte

Tras actualizar hat y botones en un ciclo, si hubo algún cambio se llama a
`Gamepad.write()` una vez (no en cada vuelta) para enviar el reporte HID.

### 6. OLED (opcional, con refresco corregido)

- Detección honesta por I2C (probe de ACK antes de `begin`, como en el keypad mejorado),
  prueba 0x3C/0x3D, con `Wire.setWireTimeout` para no congelar el loop.
- Dibuja el estado del mando: indicador de dirección de la cruceta (flechas/cruz) y los
  4 botones, resaltando los activos.
- **Refresco solo cuando cambia el estado** (se elimina el redibujo periódico de 250 ms
  que causaba el parpadeo/"movimiento" observado). Si no hay OLED, el mando funciona
  igual (`oledOk` protege todo).

### 7. Diagnósticos (heredados)

- Autodiagnóstico de pines en `setup()` (todos deben leer HIGH en reposo).
- `escanearI2C()` + estado real de la OLED por el monitor serie (115200).
- Eco por serie de dirección de cruceta y botones al cambiar, para depurar sin Steam.

## Lado Steam / Windows

- **Windows:** `joy.cpl` → "RedBearPad" → la pestaña de propiedades muestra los 4 botones
  y la cruceta; sirve para validar antes de Steam.
- **Steam:** *Configuración → Controlador* habilitar el soporte de controles genéricos;
  el mando aparece en Steam Input y se le asigna layout (cruceta → d-pad o stick, botones
  1-4 → A/B/X/Y o lo que el juego necesite).

## Manejo de errores

- OLED ausente o muda → `oledOk=false`, el mando sigue operativo.
- Bus I2C trabado → el timeout de `Wire` aborta y no congela el envío HID.
- Direcciones opuestas simultáneas en la cruceta → se resuelven a centro (sin estado
  inválido de hat).

## Verificación

1. Compilar para `redbearlab:avr:blendmicro8` (y `blendmicro16` como sanity) con
   arduino-cli; debe enlazar sin errores.
2. Subir a la placa (lo hace el usuario) y abrir el monitor serie: ver el eco de
   dirección/botones y el bloque de diagnóstico.
3. **Windows `joy.cpl`:** confirmar que los 8 switches mueven cruceta/botones correctos,
   incluidas diagonales.
4. **Steam:** confirmar que aparece como control y es configurable en Steam Input.

Recordatorio: usar siempre **8 MHz** (`blendmicro8`); 16 MHz es overclock fuera de spec
a 3.3 V (causa fallos erráticos; documentado en el paquete `blend-micro-boards`).

## Fuera de alcance / futuro

- Sticks analógicos (requeriría hardware nuevo).
- Modo combinado teclado+gamepad simultáneo.
- Perfil/configuración guardada en EEPROM.
