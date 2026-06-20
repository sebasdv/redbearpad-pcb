/*
 * RedBearPad - Keypad USB HID de 8 teclas con OLED 128x32
 * Placa: RedBearLab Blend Micro V1.0 (ATmega32U4, core redbearlab:avr)
 *
 * El PC lo ve como un teclado: cada switch envia una tecla mientras se mantiene
 * pulsado. La OLED muestra en vivo que tecla esta activa.
 *
 * Mapeo de teclas:
 *   B1->Esc  B2->W  B3->A  B4->S  B5->D  B6->Espacio  B7->Enter  B8->Shift
 *
 * Librerias necesarias (todas desde el Gestor de librerias de Arduino):
 *   - HID-Project      (NicoHood) -> provee el Keyboard USB HID
 *   - Adafruit SSD1306
 *   - Adafruit GFX Library
 *
 * Mapeo de pines (fijado por la PCB redbearpad):
 *   SW1->D5  SW2->D8  SW3->D9  SW4->D10  SW5->D11  SW6->D12  SW7->A0  SW8->A1
 *   OLED I2C: SDA->D2  SCL->D3   (3.3 V)
 */

#include <HID-Project.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ----- Switches: un pin por boton, a GND, con pull-up interno (activo en LOW) -----
const uint8_t NUM_BOTONES = 8;
const uint8_t PIN_BOTON[NUM_BOTONES] = { 5, 8, 9, 10, 11, 12, A0, A1 };

// ----- Tecla HID que envia cada boton (HID-Project KeyboardKeycode) -----
const KeyboardKeycode TECLA[NUM_BOTONES] = {
  KEY_ESC,        // B1
  KEY_W,          // B2
  KEY_A,          // B3
  KEY_S,          // B4
  KEY_D,          // B5
  KEY_SPACE,      // B6
  KEY_ENTER,      // B7
  KEY_LEFT_SHIFT  // B8
};
// Etiqueta corta para la OLED / monitor serie de cada tecla.
const char* const ETIQUETA[NUM_BOTONES] = {
  "Esc", "W", "A", "S", "D", "Spc", "Ent", "Sft"
};

// ----- OLED 128x32 por I2C -----
#define OLED_ANCHO   128
#define OLED_ALTO    32
#define OLED_RESET   -1      // comparte el reset de la placa
#define OLED_ADDR    0x3C    // direccion tipica de los modulos 128x32
Adafruit_SSD1306 oled(OLED_ANCHO, OLED_ALTO, &Wire, OLED_RESET);

// El teclado HID lo provee HID-Project mediante el objeto global `Keyboard`.

// Debounce por integracion: la lectura cruda debe mantenerse ESTABLE durante
// DEBOUNCE_MS antes de aceptarla. Asi se rechazan glitches breves de crosstalk
// entre pines vecinos (p.ej. pulsar SW1/D5 inducia un pico en SW2/D8, adyacentes
// en el header J1) que un debounce de tipo "lockout" dejaba pasar.
bool estadoPrev[NUM_BOTONES];     // estado aceptado (ya filtrado)
bool lecturaRaw[NUM_BOTONES];     // ultima lectura cruda del pin
uint32_t tLecturaRaw[NUM_BOTONES]; // instante en que cambio la lectura cruda
const uint16_t DEBOUNCE_MS = 25;  // ms que debe mantenerse estable para aceptar

bool oledOk = false;

void dibujarPantalla(const bool pulsado[]) {
  if (!oledOk) return;
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.println(F("RedBearPad  keypad"));
  // Dos filas de 4: etiqueta de la tecla, en video inverso si esta pulsada.
  for (uint8_t i = 0; i < NUM_BOTONES; i++) {
    uint8_t col = i % 4;
    uint8_t fila = i / 4;
    int16_t x = col * 32;
    int16_t y = 14 + fila * 10;
    if (pulsado[i]) {
      oled.fillRect(x, y - 1, 30, 10, SSD1306_WHITE);
      oled.setTextColor(SSD1306_BLACK);
    } else {
      oled.setTextColor(SSD1306_WHITE);
    }
    oled.setCursor(x + 2, y);
    oled.print(ETIQUETA[i]);
  }
  oled.display();
}

void setup() {
  Serial.begin(115200);             // monitor serie (CDC sobre USB); no bloquea si no hay PC
  for (uint8_t i = 0; i < NUM_BOTONES; i++) {
    pinMode(PIN_BOTON[i], INPUT_PULLUP);
    estadoPrev[i] = false;          // sin pulsar
    lecturaRaw[i] = false;
    tLecturaRaw[i] = 0;
  }

  // CLAVE: timeout del I2C antes de tocar el bus. En AVR, Wire se cuelga para
  // siempre si el bus se traba (SDA/SCL retenida, ruido, pull-ups debiles) y eso
  // congelaria TODO el loop, teclado incluido. Con timeout, una transaccion
  // trabada aborta a los 25 ms, se resetea el bus y el teclado sigue vivo.
  Wire.begin();
  Wire.setWireTimeout(25000 /*us*/, true /*resetea el bus al expirar*/);

  // La pantalla es opcional: probar 0x3C y, si falla, 0x3D (modulos varian).
  oledOk = oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  if (!oledOk) oledOk = oled.begin(SSD1306_SWITCHCAPVCC, 0x3D);

  // DIAGNOSTICO: con nada pulsado todos los pines deben leer HIGH. Si alguno
  // arranca en LOW, ese pin esta clavado (mal cableado, puente, o uso on-board)
  // y generaria una tecla fantasma. Esperar a que el monitor serie se conecte.
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) { /* espera hasta 3 s al PC */ }
  Serial.println(F("=== RedBearPad keypad - autodiagnostico de pines ==="));
  bool algunoClavado = false;
  for (uint8_t i = 0; i < NUM_BOTONES; i++) {
    bool low = (digitalRead(PIN_BOTON[i]) == LOW);
    Serial.print(ETIQUETA[i]);
    Serial.print(F(" (pin "));
    Serial.print(PIN_BOTON[i]);
    Serial.print(F("): "));
    Serial.println(low ? F("LOW  <-- CLAVADO sin pulsar!") : F("HIGH ok"));
    if (low) algunoClavado = true;
  }
  Serial.println(algunoClavado
    ? F("AVISO: hay pines en LOW al arranque -> revisar cableado/HW")
    : F("Pines OK. Arrancando teclado HID."));

  Keyboard.begin();                 // arranca el HID despues del diagnostico

  if (oledOk) dibujarPantalla(estadoPrev);   // rejilla inicial (todo sin pulsar)
}

void loop() {
  bool pulsadoAhora[NUM_BOTONES];
  uint32_t ahora = millis();
  bool huboCambio = false;

  for (uint8_t i = 0; i < NUM_BOTONES; i++) {
    // INPUT_PULLUP: pin en LOW = boton pulsado.
    bool leido = (digitalRead(PIN_BOTON[i]) == LOW);

    // Si la lectura cruda cambia, reinicia el cronometro de estabilidad.
    if (leido != lecturaRaw[i]) {
      lecturaRaw[i] = leido;
      tLecturaRaw[i] = ahora;
    }

    pulsadoAhora[i] = estadoPrev[i];

    // Solo se acepta el cambio si la lectura lleva DEBOUNCE_MS estable.
    // Un glitch de crosstalk (mas corto que DEBOUNCE_MS) nunca llega a aceptarse.
    if (leido != estadoPrev[i] && (ahora - tLecturaRaw[i]) >= DEBOUNCE_MS) {
      estadoPrev[i] = leido;
      pulsadoAhora[i] = leido;
      huboCambio = true;
      // press mantiene la tecla mientras el boton este pulsado; release al soltar.
      if (leido) Keyboard.press(TECLA[i]);
      else       Keyboard.release(TECLA[i]);
      // Eco por el monitor serie.
      Serial.print(ETIQUETA[i]);
      Serial.println(leido ? F(" pulsado") : F(" soltado"));
    }
  }

  // Refrescar la OLED al cambiar, y ademas un refresco lento de respaldo (~4 Hz)
  // por si la pantalla pierde el contenido. Volcar el framebuffer en CADA vuelta
  // saturaba el I2C y dejaba sin tiempo al USB (HID + Serial) -> el "colapso".
  static uint32_t ultimoRefresco = 0;
  if (oledOk && (huboCambio || ahora - ultimoRefresco > 250)) {
    dibujarPantalla(pulsadoAhora);
    ultimoRefresco = ahora;
  }
}
