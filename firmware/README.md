# Firmware de prueba — RedBearPad Keypad

Sketch `redbearpad_keypad/redbearpad_keypad.ino`: convierte la placa en un
**teclado USB HID de 8 teclas** y muestra en la OLED qué tecla está activa.
Sirve para validar PCB, switches y pantalla, y para usarlo como keypad de juego.

Mapeo de teclas:

| Botón | Tecla | Botón | Tecla |
|---|---|---|---|
| B1 | Esc | B5 | D |
| B2 | W | B6 | Espacio |
| B3 | A | B7 | Enter |
| B4 | S | B8 | Shift (izq.) |

Cada tecla se mantiene pulsada mientras el switch esté presionado (press/release).

## Requisitos

1. **Core del Blend Micro** `redbearlab:avr` (repo `blend-micro-boards`).
   Comprobar: `arduino-cli core list` debe listar `redbearlab:avr`.
2. **Librerías** (todas desde el Gestor de librerías de Arduino):
   - **HID-Project** — NicoHood. Provee el `Keyboard` USB HID. Se usa esta (y no
     la librería "Keyboard" del core ni "Joystick") porque su header
     `HID-Project.h` no colisiona con otras librerías y trae teclado, gamepad y
     más en un solo paquete.
   - **Adafruit SSD1306** y **Adafruit GFX Library**.

## Compilar

Verificado en esta máquina con arduino-cli (19500 B flash / 68 %, 664 B RAM),
usando solo librerías del Gestor (sin rutas extra):

```powershell
$cli = "C:\redbearpad\tools\arduino-cli\arduino-cli.exe"
& $cli lib install "HID-Project" "Adafruit SSD1306" "Adafruit GFX Library"
& $cli compile --fqbn redbearlab:avr:blendmicro16 firmware\redbearpad_keypad
```

FQBN: `redbearlab:avr:blendmicro16` (16 MHz, overclock) o
`redbearlab:avr:blendmicro8` (8 MHz). Ambas compilan igual.

## Cargar a la placa

El Blend Micro usa el bootloader Caterina (protocolo avr109, *1200 bps touch*).
Conectar por USB, identificar el puerto COM y subir:

```powershell
& $cli upload --fqbn redbearlab:avr:blendmicro16 -p COM<N> firmware\redbearpad_keypad
```

Desde el IDE de Arduino: seleccionar la placa *Blend Micro 3.3V/16MHz*, el puerto,
y pulsar *Subir*. Si la subida no arranca, pulsar *reset* en la placa justo cuando
el IDE indique "Subiendo" (el bootloader Caterina solo expone el puerto ~8 s).

> La carga la haces tú con la placa conectada; aquí solo se validó la compilación.

## Comprobar que funciona

- **PC:** abrir cualquier editor de texto y pulsar los switches: deben escribirse
  las teclas (W/A/S/D, Espacio, Enter) y actuar Esc y Shift.
- **Monitor serie** (115200 baudios): imprime `RedBearPad listo - keypad 8 teclas`
  y un eco `<tecla> pulsado` / `<tecla> soltado` en cada cambio.
- **OLED:** muestra `RedBearPad keypad` y una rejilla Esc/W/A/S/D/Spc/Ent/Sft;
  la tecla pulsada se resalta en vídeo inverso. Si la pantalla no responde,
  revisar que su dirección I2C sea `0x3C` (algunos módulos usan `0x3D`: cambiar
  `OLED_ADDR`).

## Mapeo de pines (fijado por la PCB)

| Botón | Pin | Botón | Pin |
|---|---|---|---|
| B1 | D5 | B5 | D11 |
| B2 | D8 | B6 | D12 |
| B3 | D9 | B7 | A0 |
| B4 | D10 | B8 | A1 |

OLED I2C: SDA→D2, SCL→D3, VCC→3.3 V, GND→GND.
