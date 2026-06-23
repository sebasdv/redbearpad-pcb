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
