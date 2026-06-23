# Firmware Gamepad — RedBearPad

Sketch `redbearpad_gamepad/redbearpad_gamepad.ino`: convierte la placa en un
**gamepad USB HID** con cruceta + 4 botones, para usar en Steam.

## Mapeo

| Switch | Rol gamepad |
|---|---|
| SW2 / SW3 / SW4 / SW5 | Cruceta Arriba / Izq / Abajo / Der (con diagonales) |
| SW1 | Botón 1 |
| SW6 | Botón 2 |
| SW7 | Botón 3 |
| SW8 | Botón 4 |

## Requisitos

- Core `redbearlab:avr` (repo `blend-micro-boards`).
- Librerías (Gestor de librerías): **HID-Project** (NicoHood), **Adafruit SSD1306**,
  **Adafruit GFX Library**.

## Compilar y subir

```powershell
$cli = "C:\redbearpad\tools\arduino-cli\arduino-cli.exe"
& $cli compile --fqbn redbearlab:avr:blendmicro8 firmware\redbearpad_gamepad
& $cli upload  --fqbn redbearlab:avr:blendmicro8 -p COM<N> firmware\redbearpad_gamepad
```

Usar **blendmicro8** (8 MHz). El 16 MHz es overclock fuera de spec a 3.3 V y causa
fallos erráticos.

## Probar

- **Windows:** `joy.cpl` → "RedBearPad" → mover cruceta y pulsar los 4 botones.
- **Monitor serie** (115200): al arrancar imprime `dpad self-test: PASS`, el
  autodiagnóstico de pines y el escaneo I2C; luego un eco `dpad=N botones=XXXX`.
- **Steam:** Configuración → Controlador → habilitar soporte de controles genéricos;
  configurar en Steam Input (la cruceta se puede asignar a d-pad o stick).
