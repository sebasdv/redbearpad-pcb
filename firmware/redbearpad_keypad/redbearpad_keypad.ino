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

// Estado previo de cada boton para enviar al PC solo en los cambios (con debounce).
bool estadoPrev[NUM_BOTONES];
uint32_t ultimoCambio[NUM_BOTONES];
const uint16_t DEBOUNCE_MS = 15;

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
    ultimoCambio[i] = 0;
  }

  Keyboard.begin();                 // arranca el HID
  Serial.println(F("RedBearPad listo - keypad 8 teclas"));

  // La pantalla es opcional para el test: si falla, el teclado sigue funcionando.
  oledOk = oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  if (oledOk) {
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(0, 12);
    oled.println(F("RedBearPad listo"));
    oled.display();
  }
}

void loop() {
  bool pulsadoAhora[NUM_BOTONES];
  uint32_t ahora = millis();

  for (uint8_t i = 0; i < NUM_BOTONES; i++) {
    // INPUT_PULLUP: pin en LOW = boton pulsado.
    bool leido = (digitalRead(PIN_BOTON[i]) == LOW);
    pulsadoAhora[i] = estadoPrev[i];

    if (leido != estadoPrev[i] && (ahora - ultimoCambio[i]) > DEBOUNCE_MS) {
      estadoPrev[i] = leido;
      ultimoCambio[i] = ahora;
      pulsadoAhora[i] = leido;
      // press mantiene la tecla mientras el boton este pulsado; release al soltar.
      if (leido) Keyboard.press(TECLA[i]);
      else       Keyboard.release(TECLA[i]);
      // Eco por el monitor serie.
      Serial.print(ETIQUETA[i]);
      Serial.println(leido ? F(" pulsado") : F(" soltado"));
    }
  }

  dibujarPantalla(pulsadoAhora);
}
