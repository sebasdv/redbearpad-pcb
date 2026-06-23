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

// Debounce por INTEGRADOR con histeresis, muestreado a intervalo FIJO.
// Por que: con pull-up interno (alta impedancia) la linea sube "sucia" al soltar.
// El debounce anterior ("estable N ms, se reinicia con CADA glitch") se atascaba:
// con el loop a ~16 kHz (Serial cerrado) cazaba cada glitch, reiniciaba el conteo
// y el release NUNCA se aceptaba -> la tecla quedaba pegada -> autorepeat del SO.
// El integrador sube con LOW y baja con HIGH; un glitch disperso solo mueve el
// contador 1 y no alcanza a voltear el estado. Muestrear a SAMPLE_MS fijo hace
// que el filtro NO dependa de la velocidad del loop (ni del Serial).
bool estadoPrev[NUM_BOTONES];        // estado aceptado (filtrado): true = pulsado
uint8_t integrador[NUM_BOTONES];     // 0..INTEGRADOR_MAX por tecla
uint32_t tUltimoSample = 0;          // ultimo instante de muestreo
const uint8_t SAMPLE_MS = 1;         // periodo de muestreo fijo (ms)
const uint8_t INTEGRADOR_MAX = 12;   // ~12 ms de senal predominante para cambiar

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

// true si un dispositivo I2C ACKea en esa direccion. Usa el timeout de Wire, asi que
// no se cuelga si el bus esta trabado.
bool i2cResponde(uint8_t addr) {
  Wire.beginTransmission(addr);
  return (Wire.endTransmission() == 0);
}

// Escaneo del bus I2C: lista las direcciones que responden. Sirve para diagnosticar
// la OLED (suele estar en 0x3C o 0x3D). Si no responde nada, el problema esta en el
// cableado (SDA/SCL/VCC/GND) o el modulo, no en el sketch.
void escanearI2C() {
  Serial.println(F("=== Escaneo I2C ==="));
  uint8_t encontrados = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    if (i2cResponde(addr)) {
      Serial.print(F("  dispositivo en 0x"));
      if (addr < 16) Serial.print('0');
      Serial.println(addr, HEX);
      encontrados++;
    }
  }
  if (encontrados == 0)
    Serial.println(F("  nada respondio -> revisar SDA/SCL/VCC/GND o el modulo OLED"));
  else {
    Serial.print(F("  total dispositivos: "));
    Serial.println(encontrados);
  }
  Serial.println(F("  (la OLED 128x32 suele estar en 0x3C o 0x3D)"));
}

void setup() {
  Serial.begin(115200);             // monitor serie (CDC sobre USB); no bloquea si no hay PC
  for (uint8_t i = 0; i < NUM_BOTONES; i++) {
    pinMode(PIN_BOTON[i], INPUT_PULLUP);
    estadoPrev[i] = false;          // sin pulsar
    integrador[i] = 0;              // integrador en reposo (soltado)
  }

  // CLAVE: timeout del I2C antes de tocar el bus. En AVR, Wire se cuelga para
  // siempre si el bus se traba (SDA/SCL retenida, ruido, pull-ups debiles) y eso
  // congelaria TODO el loop, teclado incluido. Con timeout, una transaccion
  // trabada aborta a los 25 ms, se resetea el bus y el teclado sigue vivo.
  Wire.begin();
  Wire.setWireTimeout(25000 /*us*/, true /*resetea el bus al expirar*/);

  // La pantalla es opcional. PRIMERO se verifica por I2C si el chip realmente responde:
  // Adafruit_SSD1306::begin() devuelve true aunque NO haya pantalla (solo reserva el
  // buffer, no comprueba el ACK). Probando el ACK antes, oledOk refleja la presencia
  // real y se inicializa en la direccion que de verdad contesta (0x3C o 0x3D).
  uint8_t oledAddr = 0;
  if (i2cResponde(0x3C))      oledAddr = 0x3C;
  else if (i2cResponde(0x3D)) oledAddr = 0x3D;
  oledOk = (oledAddr != 0) && oled.begin(SSD1306_SWITCHCAPVCC, oledAddr);

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

  // Diagnostico de pantalla: escaneo del bus y estado de la OLED.
  escanearI2C();
  Serial.print(F("OLED: "));
  Serial.println(oledOk ? F("detectada e inicializada")
                        : F("NO detectada (ver escaneo I2C de arriba)"));

  Keyboard.begin();                 // arranca el HID despues del diagnostico

  if (oledOk) dibujarPantalla(estadoPrev);   // rejilla inicial (todo sin pulsar)
}

void loop() {
  uint32_t ahora = millis();
  bool huboCambio = false;
  bool pulsadoAhora[NUM_BOTONES];
  for (uint8_t i = 0; i < NUM_BOTONES; i++) pulsadoAhora[i] = estadoPrev[i];

  // Muestreo a intervalo FIJO (SAMPLE_MS): el filtro no depende de cuan rapido
  // corra el loop, asi que se comporta igual con el Serial abierto o cerrado.
  if (ahora - tUltimoSample >= SAMPLE_MS) {
    tUltimoSample = ahora;

    for (uint8_t i = 0; i < NUM_BOTONES; i++) {
      // INPUT_PULLUP: pin en LOW = boton pulsado.
      bool leido = (digitalRead(PIN_BOTON[i]) == LOW);

      // Integrador: sube hacia INTEGRADOR_MAX con LOW, baja hacia 0 con HIGH.
      if (leido) { if (integrador[i] < INTEGRADOR_MAX) integrador[i]++; }
      else       { if (integrador[i] > 0)              integrador[i]--; }

      // Histeresis: solo se voltea en los extremos; el ruido disperso (que no
      // llega a un extremo) mantiene el estado anterior y no genera eventos.
      bool nuevo = estadoPrev[i];
      if (integrador[i] >= INTEGRADOR_MAX) nuevo = true;   // pulsado confirmado
      else if (integrador[i] == 0)         nuevo = false;  // soltado confirmado

      if (nuevo != estadoPrev[i]) {
        estadoPrev[i] = nuevo;
        pulsadoAhora[i] = nuevo;
        huboCambio = true;
        // press mantiene la tecla mientras este pulsado; release al soltar.
        if (nuevo) Keyboard.press(TECLA[i]);
        else       Keyboard.release(TECLA[i]);
        // Eco por el monitor serie.
        Serial.print(ETIQUETA[i]);
        Serial.println(nuevo ? F(" pulsado") : F(" soltado"));
      }
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
