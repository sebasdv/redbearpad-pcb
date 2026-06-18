# Firmware de prueba — RedBearPad Gamepad

Sketch `redbearpad_gamepad/redbearpad_gamepad.ino`: convierte la placa en un
**gamepad USB HID de 8 botones** y muestra el estado de cada botón en la OLED.
Sirve para validar PCB, switches y pantalla, y para usar el control en juegos.

## Requisitos

1. **Core del Blend Micro** `redbearlab:avr` (repo `blend-micro-boards`).
   Comprobar: `arduino-cli core list` debe listar `redbearlab:avr`.
2. **Librerías:**
   - **Joystick** — Matthew Heironimus, *ArduinoJoystickLibrary*
     (https://github.com/MHeironimus/ArduinoJoystickLibrary). No está en el
     Gestor de librerías: instalar desde el ZIP de GitHub
     (IDE: *Programa → Incluir librería → Añadir biblioteca .ZIP*).
   - **Adafruit SSD1306** y **Adafruit GFX Library** (Gestor de librerías).

## Compilar

Verificado en esta máquina con arduino-cli (20586 B flash / 71 %, 718 B RAM):

```powershell
$cli = "C:\redbearpad\tools\arduino-cli\arduino-cli.exe"
# --libraries apunta a la carpeta donde extrajiste ArduinoJoystickLibrary
& $cli compile --fqbn redbearlab:avr:blendmicro16 --libraries <carpeta_libs> firmware\redbearpad_gamepad
```

FQBN: `redbearlab:avr:blendmicro16` (16 MHz, overclock) o
`redbearlab:avr:blendmicro8` (8 MHz). Ambas compilan igual.

## Cargar a la placa

El Blend Micro usa el bootloader Caterina (protocolo avr109, *1200 bps touch*).
Conectar por USB, identificar el puerto COM y subir:

```powershell
& $cli upload --fqbn redbearlab:avr:blendmicro16 --libraries <carpeta_libs> -p COM<N> firmware\redbearpad_gamepad
```

Desde el IDE de Arduino: seleccionar la placa *Blend Micro 3.3V/16MHz*, el puerto,
y pulsar *Subir*. Si la subida no arranca, pulsar *reset* en la placa justo cuando
el IDE indique "Subiendo" (el bootloader Caterina solo expone el puerto ~8 s).

> La carga la haces tú con la placa conectada; aquí solo se validó la compilación.

## Comprobar que funciona

- **PC:** Panel de control → *Configurar controladores de juego USB* → debe
  aparecer "RedBearPad" (o "Blend Micro") con 8 botones que se encienden al pulsar.
- **OLED:** muestra `RedBearPad 8-btn` y una rejilla B1…B8; el botón pulsado
  se resalta en vídeo inverso. Si la pantalla no responde, revisar que su
  dirección I2C sea `0x3C` (algunos módulos usan `0x3D`: cambiar `OLED_ADDR`).

## Mapeo de pines (fijado por la PCB)

| Botón | Pin | Botón | Pin |
|---|---|---|---|
| B1 | D5 | B5 | D11 |
| B2 | D8 | B6 | D12 |
| B3 | D9 | B7 | A0 |
| B4 | D10 | B8 | A1 |

OLED I2C: SDA→D2, SCL→D3, VCC→3.3 V, GND→GND.
