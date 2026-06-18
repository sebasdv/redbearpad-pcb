/*
 * RedBearPad - Gamepad USB HID de 8 botones con OLED 128x32
 * Placa: RedBearLab Blend Micro V1.0 (ATmega32U4, core redbearlab:avr)
 *
 * El PC lo ve como un control de juegos con 8 botones (Panel de control ->
 * "Configurar controladores de juego USB"). La OLED muestra en vivo que
 * boton esta pulsado, util para validar PCB, switches y pantalla.
 *
 * Librerias necesarias (todas desde el Gestor de librerias de Arduino):
 *   - HID-Project      (NicoHood) -> provee el Gamepad USB HID
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

// ----- OLED 128x32 por I2C -----
#define OLED_ANCHO   128
#define OLED_ALTO    32
#define OLED_RESET   -1      // comparte el reset de la placa
#define OLED_ADDR    0x3C    // direccion tipica de los modulos 128x32
Adafruit_SSD1306 oled(OLED_ANCHO, OLED_ALTO, &Wire, OLED_RESET);

// El gamepad HID lo provee HID-Project mediante el objeto global `Gamepad`.
// Sus botones se numeran 1..32, por eso usamos (i + 1) al pulsar/soltar.

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
  oled.println(F("RedBearPad  8-btn"));
  // Dos filas de 4: numero del boton, en video inverso si esta pulsado.
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
    oled.setCursor(x + 4, y);
    oled.print(F("B"));
    oled.print(i + 1);
  }
  oled.display();
}

void setup() {
  for (uint8_t i = 0; i < NUM_BOTONES; i++) {
    pinMode(PIN_BOTON[i], INPUT_PULLUP);
    estadoPrev[i] = false;          // sin pulsar
    ultimoCambio[i] = 0;
  }

  Gamepad.begin();                  // arranca el HID

  // La pantalla es opcional para el test: si falla, el gamepad sigue funcionando.
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
  bool huboCambio = false;

  for (uint8_t i = 0; i < NUM_BOTONES; i++) {
    // INPUT_PULLUP: pin en LOW = boton pulsado.
    bool leido = (digitalRead(PIN_BOTON[i]) == LOW);
    pulsadoAhora[i] = estadoPrev[i];

    if (leido != estadoPrev[i] && (ahora - ultimoCambio[i]) > DEBOUNCE_MS) {
      estadoPrev[i] = leido;
      ultimoCambio[i] = ahora;
      pulsadoAhora[i] = leido;
      // HID-Project numera los botones desde 1.
      if (leido) Gamepad.press(i + 1);
      else       Gamepad.release(i + 1);
      huboCambio = true;
    }
  }

  if (huboCambio) Gamepad.write();   // envia el reporte HID solo si cambio algo

  dibujarPantalla(pulsadoAhora);
}
