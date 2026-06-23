# Firmware Gamepad HID (Steam) — Plan de Implementación

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Sketch `redbearpad_gamepad.ino` que hace que el RedBearPad enumere como gamepad HID (cruceta + 4 botones) reconocible por Steam.

**Architecture:** Sketch nuevo e independiente del keypad. Usa `Gamepad` de HID-Project. Reutiliza el debounce por integrador + muestreo fijo. Cada ciclo: filtra los 8 switches, combina 4 en la cruceta (hat, con diagonales) y mapea 4 a botones 1-4; envía el reporte solo al cambiar. OLED opcional muestra el estado.

**Tech Stack:** Arduino (ATmega32U4), HID-Project (Gamepad), Adafruit SSD1306/GFX, arduino-cli.

**Spec:** `docs/superpowers/specs/2026-06-22-gamepad-steam-design.md`

**Entorno (Windows / PowerShell):**
- arduino-cli: `C:\redbearpad\tools\arduino-cli\arduino-cli.exe`
- Repo: `C:\redbearpad\gamepad-pcb` (git, rama `implementacion`)
- FQBN: **`redbearlab:avr:blendmicro8`** (8 MHz — NUNCA 16 MHz; es overclock fuera de spec a 3.3 V).
- Librerías ya instaladas: HID-Project, Adafruit SSD1306, Adafruit GFX.

**Nota de verificación:** No hay framework de tests en el dispositivo. La verificación de cada tarea es: (a) **compilación limpia** con arduino-cli (lo hace el agente), y donde aplique (b) un **auto-test on-device** que imprime PASS/FAIL por serie, y (c) checks físicos en `joy.cpl`/Steam que ejecuta el usuario con la placa.

**Mapeo (índices dentro de `PIN_BOTON` / `estadoPrev`, 0-based):**

| idx | Switch | Pin | Rol |
|---|---|---|---|
| 0 | SW1 | 5 (D5)  | Botón 1 |
| 1 | SW2 | 8 (D8)  | Cruceta ARRIBA |
| 2 | SW3 | 9 (D9)  | Cruceta IZQUIERDA |
| 3 | SW4 | 10 (D10)| Cruceta ABAJO |
| 4 | SW5 | 11 (D11)| Cruceta DERECHA |
| 5 | SW6 | 12 (D12)| Botón 2 |
| 6 | SW7 | A0      | Botón 3 |
| 7 | SW8 | A1      | Botón 4 |

---

### Task 1: Baseline limpia — commitear cambios pendientes del keypad

El keypad tiene cambios sin commitear (scanner I2C + detección honesta de OLED) que se reutilizan aquí. Se guardan primero para partir de un árbol limpio.

**Files:**
- Commit: `firmware/redbearpad_keypad/redbearpad_keypad.ino` (ya modificado, sin commitear)

- [ ] **Step 1: Revisar el diff pendiente**

```powershell
git -C C:\redbearpad\gamepad-pcb diff --stat
```
Esperado: un archivo cambiado, `firmware/redbearpad_keypad/redbearpad_keypad.ino`.

- [ ] **Step 2: Commit**

```powershell
git -C C:\redbearpad\gamepad-pcb add firmware/redbearpad_keypad/redbearpad_keypad.ino
git -C C:\redbearpad\gamepad-pcb commit -m "feat(keypad): scanner I2C y deteccion honesta de OLED (probe ACK antes de begin)"
```
Esperado: commit creado. (No se hace `git push`: el repo no tiene remoto.)

---

### Task 2: Firmware del gamepad (cruceta + botones + diagnósticos)

Crea el sketch completo y funcional como mando (sin dibujo de OLED todavía). Incluye un auto-test de la lógica de cruceta que corre en `setup()`.

**Files:**
- Create: `firmware/redbearpad_gamepad/redbearpad_gamepad.ino` (la carpeta ya existe)

- [ ] **Step 1: Crear el sketch completo**

Contenido íntegro de `C:\redbearpad\gamepad-pcb\firmware\redbearpad_gamepad\redbearpad_gamepad.ino`:

```cpp
/*
 * RedBearPad - Gamepad USB HID (8 switches) para Steam
 * Placa: RedBearLab Blend Micro V1.0 (ATmega32U4, core redbearlab:avr).
 * USAR SIEMPRE 8 MHz (blendmicro8): 16 MHz es overclock fuera de spec a 3.3 V.
 *
 * Enumera como gamepad HID generico. En Windows aparece en joy.cpl; en Steam se
 * configura con Steam Input.
 *
 * Mapeo (fijado por la PCB):
 *   Cruceta: SW2=Arriba(D8)  SW3=Izq(D9)  SW4=Abajo(D10)  SW5=Der(D11)
 *   Botones: SW1=B1(D5)  SW6=B2(D12)  SW7=B3(A0)  SW8=B4(A1)
 *
 * Librerias: HID-Project (NicoHood), Adafruit SSD1306, Adafruit GFX.
 */
#include <HID-Project.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ----- Switches: un pin por boton, a GND, pull-up interno (activo en LOW) -----
const uint8_t NUM_BOTONES = 8;
const uint8_t PIN_BOTON[NUM_BOTONES] = { 5, 8, 9, 10, 11, 12, A0, A1 };  // SW1..SW8
// Indices dentro de PIN_BOTON / estadoPrev: 0=SW1 1=SW2 2=SW3 3=SW4 4=SW5 5=SW6 6=SW7 7=SW8
const uint8_t IDX_UP = 1, IDX_LEFT = 2, IDX_DOWN = 3, IDX_RIGHT = 4;  // cruceta
const uint8_t IDX_BTN[4] = { 0, 5, 6, 7 };  // SW1,SW6,SW7,SW8 -> botones gamepad 1..4
const char* const ETIQUETA[NUM_BOTONES] = { "B1", "Up", "Lt", "Dn", "Rt", "B2", "B3", "B4" };

// ----- Debounce por integrador con histeresis, muestreo a intervalo FIJO -----
bool estadoPrev[NUM_BOTONES];
uint8_t integrador[NUM_BOTONES];
uint32_t tUltimoSample = 0;
const uint8_t SAMPLE_MS = 1;
const uint8_t INTEGRADOR_MAX = 12;

// ----- OLED 128x32 por I2C (opcional) -----
#define OLED_ANCHO 128
#define OLED_ALTO  32
#define OLED_RESET -1
Adafruit_SSD1306 oled(OLED_ANCHO, OLED_ALTO, &Wire, OLED_RESET);
bool oledOk = false;

// Combina las 4 direcciones en un valor de hat (incluye diagonales).
int8_t calcularDpad(bool up, bool down, bool left, bool right) {
  if (up && down)    { up = false; down = false; }    // opuestos se cancelan
  if (left && right) { left = false; right = false; }
  if (up && right)   return GAMEPAD_DPAD_UP_RIGHT;
  if (up && left)    return GAMEPAD_DPAD_UP_LEFT;
  if (down && right) return GAMEPAD_DPAD_DOWN_RIGHT;
  if (down && left)  return GAMEPAD_DPAD_DOWN_LEFT;
  if (up)    return GAMEPAD_DPAD_UP;
  if (down)  return GAMEPAD_DPAD_DOWN;
  if (left)  return GAMEPAD_DPAD_LEFT;
  if (right) return GAMEPAD_DPAD_RIGHT;
  return GAMEPAD_DPAD_CENTERED;
}

// Auto-test de la logica de cruceta: corre una vez en setup, imprime PASS/FAIL.
void probarDpad() {
  struct Caso { bool u, d, l, r; int8_t esperado; };
  const Caso casos[] = {
    { 0,0,0,0, GAMEPAD_DPAD_CENTERED },
    { 1,0,0,0, GAMEPAD_DPAD_UP },
    { 0,1,0,0, GAMEPAD_DPAD_DOWN },
    { 0,0,1,0, GAMEPAD_DPAD_LEFT },
    { 0,0,0,1, GAMEPAD_DPAD_RIGHT },
    { 1,0,1,0, GAMEPAD_DPAD_UP_LEFT },
    { 1,0,0,1, GAMEPAD_DPAD_UP_RIGHT },
    { 0,1,1,0, GAMEPAD_DPAD_DOWN_LEFT },
    { 0,1,0,1, GAMEPAD_DPAD_DOWN_RIGHT },
    { 1,1,0,0, GAMEPAD_DPAD_CENTERED },   // arriba+abajo se cancelan
    { 0,0,1,1, GAMEPAD_DPAD_CENTERED },   // izq+der se cancelan
    { 1,1,1,1, GAMEPAD_DPAD_CENTERED },   // todo se cancela
  };
  uint8_t fallos = 0;
  for (uint8_t i = 0; i < sizeof(casos) / sizeof(casos[0]); i++) {
    int8_t got = calcularDpad(casos[i].u, casos[i].d, casos[i].l, casos[i].r);
    if (got != casos[i].esperado) {
      fallos++;
      Serial.print(F("  FAIL caso ")); Serial.print(i);
      Serial.print(F(" -> ")); Serial.print(got);
      Serial.print(F(" esperaba ")); Serial.println(casos[i].esperado);
    }
  }
  Serial.print(F("dpad self-test: "));
  Serial.println(fallos == 0 ? F("PASS") : F("FAIL"));
}

bool i2cResponde(uint8_t addr) {
  Wire.beginTransmission(addr);
  return (Wire.endTransmission() == 0);
}

void escanearI2C() {
  Serial.println(F("=== Escaneo I2C ==="));
  uint8_t n = 0;
  for (uint8_t a = 1; a < 127; a++) {
    if (i2cResponde(a)) {
      Serial.print(F("  dispositivo en 0x"));
      if (a < 16) Serial.print('0');
      Serial.println(a, HEX);
      n++;
    }
  }
  if (n == 0) Serial.println(F("  nada respondio -> revisar SDA/SCL/VCC/GND o el modulo OLED"));
}

void setup() {
  Serial.begin(115200);
  for (uint8_t i = 0; i < NUM_BOTONES; i++) {
    pinMode(PIN_BOTON[i], INPUT_PULLUP);
    estadoPrev[i] = false;
    integrador[i] = 0;
  }

  Wire.begin();
  Wire.setWireTimeout(25000 /*us*/, true /*resetea el bus al expirar*/);

  // Deteccion honesta de OLED: probar ACK por I2C antes de begin (Adafruit begin()
  // devuelve true aunque no haya pantalla). Probar 0x3C y, si falla, 0x3D.
  uint8_t oledAddr = 0;
  if (i2cResponde(0x3C))      oledAddr = 0x3C;
  else if (i2cResponde(0x3D)) oledAddr = 0x3D;
  oledOk = (oledAddr != 0) && oled.begin(SSD1306_SWITCHCAPVCC, oledAddr);
  if (oledOk) { oled.clearDisplay(); oled.display(); }  // sin dibujo aun

  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) { /* espera hasta 3 s al PC */ }

  Serial.println(F("=== RedBearPad gamepad - autodiagnostico de pines ==="));
  for (uint8_t i = 0; i < NUM_BOTONES; i++) {
    bool low = (digitalRead(PIN_BOTON[i]) == LOW);
    Serial.print(ETIQUETA[i]); Serial.print(F(" (pin "));
    Serial.print(PIN_BOTON[i]); Serial.print(F("): "));
    Serial.println(low ? F("LOW <-- CLAVADO sin pulsar!") : F("HIGH ok"));
  }
  escanearI2C();
  Serial.print(F("OLED: "));
  Serial.println(oledOk ? F("detectada") : F("NO detectada (ver escaneo I2C)"));
  probarDpad();

  Gamepad.begin();
  Serial.println(F("Gamepad HID iniciado."));
}

void loop() {
  uint32_t ahora = millis();
  bool huboCambio = false;

  if (ahora - tUltimoSample >= SAMPLE_MS) {
    tUltimoSample = ahora;
    for (uint8_t i = 0; i < NUM_BOTONES; i++) {
      bool leido = (digitalRead(PIN_BOTON[i]) == LOW);
      if (leido) { if (integrador[i] < INTEGRADOR_MAX) integrador[i]++; }
      else       { if (integrador[i] > 0)              integrador[i]--; }
      bool nuevo = estadoPrev[i];
      if (integrador[i] >= INTEGRADOR_MAX) nuevo = true;
      else if (integrador[i] == 0)         nuevo = false;
      if (nuevo != estadoPrev[i]) {
        estadoPrev[i] = nuevo;
        huboCambio = true;
      }
    }
  }

  if (huboCambio) {
    int8_t dpad = calcularDpad(estadoPrev[IDX_UP], estadoPrev[IDX_DOWN],
                               estadoPrev[IDX_LEFT], estadoPrev[IDX_RIGHT]);
    Gamepad.dPad1(dpad);
    for (uint8_t b = 0; b < 4; b++) {
      if (estadoPrev[IDX_BTN[b]]) Gamepad.press(b + 1);
      else                        Gamepad.release(b + 1);
    }
    Gamepad.write();

    Serial.print(F("dpad=")); Serial.print((int)dpad);
    Serial.print(F(" botones="));
    for (uint8_t b = 0; b < 4; b++) Serial.print(estadoPrev[IDX_BTN[b]] ? '1' : '0');
    Serial.println();
  }
}
```

- [ ] **Step 2: Compilar (verificación del agente)**

```powershell
$cli = "C:\redbearpad\tools\arduino-cli\arduino-cli.exe"
& $cli compile --clean -b redbearlab:avr:blendmicro8 C:\redbearpad\gamepad-pcb\firmware\redbearpad_gamepad
```
Esperado: termina con `Sketch uses ... bytes` y exit code 0. Si falla por una librería no encontrada, instalar: `& $cli lib install "HID-Project" "Adafruit SSD1306" "Adafruit GFX Library"`.

- [ ] **Step 3: Verificación en hardware (usuario)**

Subir desde el IDE de Arduino (placa **Blend Micro 3.3V/8MHz**), abrir Monitor Serie a 115200 y pulsar reset. Esperado al arranque:
- `dpad self-test: PASS`
- 8 líneas `... HIGH ok`
- Al pulsar switches: líneas `dpad=N botones=XXXX` con el valor correcto (p.ej. solo Up → `dpad=1`; Up+Lt → `dpad=8`; SW1 → `botones=1000`).

Si `dpad self-test: FAIL`, hay un error en `calcularDpad` — revisar contra la tabla de `probarDpad` antes de seguir.

- [ ] **Step 4: Commit**

```powershell
git -C C:\redbearpad\gamepad-pcb add firmware/redbearpad_gamepad/redbearpad_gamepad.ino
git -C C:\redbearpad\gamepad-pcb commit -m "feat(gamepad): firmware HID con cruceta (diagonales) y 4 botones"
```

---

### Task 3: Pantalla OLED con estado del mando (redibujo solo al cambiar)

Añade el dibujo del estado del gamepad, redibujando únicamente cuando cambia algo (elimina el parpadeo del refresco periódico).

**Files:**
- Modify: `firmware/redbearpad_gamepad/redbearpad_gamepad.ino`

- [ ] **Step 1: Añadir la función `dibujarPantalla()`**

Insertar esta función justo **antes** de `void setup() {`:

```cpp
// Dibuja el estado del mando: direccion de la cruceta + botones 1..4 (resaltados).
void dibujarPantalla() {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.println(F("RedBearPad gamepad"));

  int8_t d = calcularDpad(estadoPrev[IDX_UP], estadoPrev[IDX_DOWN],
                          estadoPrev[IDX_LEFT], estadoPrev[IDX_RIGHT]);
  const char* dir = "centro";
  switch (d) {
    case GAMEPAD_DPAD_UP:         dir = "arriba";  break;
    case GAMEPAD_DPAD_DOWN:       dir = "abajo";   break;
    case GAMEPAD_DPAD_LEFT:       dir = "izq";     break;
    case GAMEPAD_DPAD_RIGHT:      dir = "der";     break;
    case GAMEPAD_DPAD_UP_LEFT:    dir = "arr-izq"; break;
    case GAMEPAD_DPAD_UP_RIGHT:   dir = "arr-der"; break;
    case GAMEPAD_DPAD_DOWN_LEFT:  dir = "aba-izq"; break;
    case GAMEPAD_DPAD_DOWN_RIGHT: dir = "aba-der"; break;
  }
  oled.setCursor(0, 12);
  oled.print(F("Dir: ")); oled.print(dir);

  for (uint8_t b = 0; b < 4; b++) {
    bool on = estadoPrev[IDX_BTN[b]];
    int16_t x = b * 32;
    if (on) {
      oled.fillRect(x, 23, 30, 9, SSD1306_WHITE);
      oled.setTextColor(SSD1306_BLACK);
    } else {
      oled.setTextColor(SSD1306_WHITE);
    }
    oled.setCursor(x + 2, 24);
    oled.print('B'); oled.print(b + 1);
  }
  oled.display();
}
```

- [ ] **Step 2: Dibujar la rejilla inicial en `setup()`**

Reemplazar en `setup()` la línea:
```cpp
  Serial.println(F("Gamepad HID iniciado."));
```
por:
```cpp
  Serial.println(F("Gamepad HID iniciado."));
  if (oledOk) dibujarPantalla();
```

- [ ] **Step 3: Redibujar al cambiar en `loop()`**

Reemplazar en `loop()` el bloque final:
```cpp
    Serial.print(F("dpad=")); Serial.print((int)dpad);
    Serial.print(F(" botones="));
    for (uint8_t b = 0; b < 4; b++) Serial.print(estadoPrev[IDX_BTN[b]] ? '1' : '0');
    Serial.println();
  }
}
```
por:
```cpp
    Serial.print(F("dpad=")); Serial.print((int)dpad);
    Serial.print(F(" botones="));
    for (uint8_t b = 0; b < 4; b++) Serial.print(estadoPrev[IDX_BTN[b]] ? '1' : '0');
    Serial.println();

    if (oledOk) dibujarPantalla();   // redibujo SOLO al cambiar -> sin parpadeo
  }
}
```

- [ ] **Step 4: Compilar**

```powershell
$cli = "C:\redbearpad\tools\arduino-cli\arduino-cli.exe"
& $cli compile --clean -b redbearlab:avr:blendmicro8 C:\redbearpad\gamepad-pcb\firmware\redbearpad_gamepad
```
Esperado: exit code 0, `Sketch uses ... bytes`.

- [ ] **Step 5: Commit**

```powershell
git -C C:\redbearpad\gamepad-pcb add firmware/redbearpad_gamepad/redbearpad_gamepad.ino
git -C C:\redbearpad\gamepad-pcb commit -m "feat(gamepad): OLED muestra cruceta y botones, redibujo solo al cambiar"
```

---

### Task 4: Verificación final y documentación

**Files:**
- Create: `firmware/redbearpad_gamepad/README.md`

- [ ] **Step 1: Compilar también para 16 MHz (sanity)**

```powershell
$cli = "C:\redbearpad\tools\arduino-cli\arduino-cli.exe"
& $cli compile --clean -b redbearlab:avr:blendmicro8  C:\redbearpad\gamepad-pcb\firmware\redbearpad_gamepad
& $cli compile --clean -b redbearlab:avr:blendmicro16 C:\redbearpad\gamepad-pcb\firmware\redbearpad_gamepad
```
Esperado: ambas compilan (exit 0). Recordatorio: para usar la placa, flashear **blendmicro8**.

- [ ] **Step 2: Escribir `firmware/redbearpad_gamepad/README.md`**

````markdown
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
````

- [ ] **Step 3: Commit**

```powershell
git -C C:\redbearpad\gamepad-pcb add firmware/redbearpad_gamepad/README.md
git -C C:\redbearpad\gamepad-pcb commit -m "docs(gamepad): README de uso y configuracion en Steam"
```

---

## Verificación contra la spec

- Enumeración HID gamepad → Task 2 (`Gamepad.begin`).
- Lectura con integrador + muestreo fijo → Task 2 (reutilizado).
- Cruceta con diagonales → Task 2 (`calcularDpad` + `dPad1`), auto-test incluido.
- Botones 1-4 → Task 2 (`press`/`release` + `write`).
- OLED estado + redibujo solo al cambiar → Task 3.
- Diagnósticos (pines, I2C, eco serie) → Task 2.
- Verificación joy.cpl/Steam + 8 MHz → Tasks 3/4 + README.
