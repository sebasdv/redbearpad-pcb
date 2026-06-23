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

// ===================== Juego del dino (OLED) =====================
const uint8_t GROUND_Y  = 30;    // linea de suelo (px desde arriba)
const uint8_t DINO_X    = 6;     // x fija del dino
const uint8_t DINO_W    = 16;
const uint8_t DINO_H    = 16;
const uint8_t DUCK_H    = 8;     // alto del dino agachado (mismo ancho 16)
const uint8_t FRAME_MS  = 33;    // ~30 FPS
const int16_t GRAVEDAD  = 2;     // subpixeles (1/8 px) por frame^2  [ajustable]
const int16_t IMPULSO   = 20;    // velocidad inicial de salto (subpx/frame) [ajustable]
const uint8_t MAX_OBST  = 3;
const int16_t DINO_SUELO = (int16_t)(GROUND_Y - DINO_H) << 3;  // posY en el suelo (subpx)

// Sprites del dino 16x16 (PROGMEM, bit7 = pixel izquierdo). Arte de 1a pasada, ajustable.
const uint8_t PROGMEM DINO_A[] = {
  0x00,0x3C, 0x00,0x7E, 0x00,0x7E, 0x00,0x5E,
  0x00,0x7E, 0x01,0xFE, 0x03,0xFE, 0x1F,0xFE,
  0x3F,0xFC, 0x3F,0xF8, 0x0F,0xF8, 0x03,0x68,
  0x03,0x08, 0x06,0x18, 0x04,0x10, 0x00,0x00
};
const uint8_t PROGMEM DINO_B[] = {
  0x00,0x3C, 0x00,0x7E, 0x00,0x7E, 0x00,0x5E,
  0x00,0x7E, 0x01,0xFE, 0x03,0xFE, 0x1F,0xFE,
  0x3F,0xFC, 0x3F,0xF8, 0x0F,0xF8, 0x03,0x68,
  0x03,0x08, 0x03,0x18, 0x02,0x08, 0x00,0x00
};
// Dino agachado 16x8.
const uint8_t PROGMEM DINO_DUCK[] = {
  0x00,0x00, 0x00,0x06, 0x1F,0xFE, 0x3F,0xFE,
  0x3F,0xFC, 0x0F,0xF0, 0x03,0x60, 0x06,0x60
};

struct Obstaculo { int16_t x; uint8_t tipo; bool activo; };  // tipo 0=cactus, 1=pajaro

struct Juego {
  int16_t  posY;       // top del dino en subpixeles (1/8 px)
  int16_t  velY;       // subpx/frame
  bool     enSuelo;
  bool     agachado;
  bool     gameOver;
  bool     arribaPrev;   // flanco de subida para saltar
  bool     reinicioPrev; // flanco de subida del boton de reinicio (B1)
  uint16_t puntaje;
  uint8_t  vel;        // px/frame de scroll
  int16_t  tProx;      // frames hasta el proximo obstaculo
  uint8_t  anim;       // 0/1 animacion de patas/alas
  uint8_t  animCnt;
  Obstaculo obst[MAX_OBST];
};
Juego juego;

// AABB: true si los rectangulos (x,y,w,h) se solapan.
bool solapan(int16_t ax, int16_t ay, int16_t aw, int16_t ah,
             int16_t bx, int16_t by, int16_t bw, int16_t bh) {
  return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

// Auto-test del AABB (corre en setup, imprime PASS/FAIL).
void probarColision() {
  uint8_t fallos = 0;
  if (!solapan(0,0,10,10, 5,5,10,10)) fallos++;   // se solapan
  if ( solapan(0,0,10,10, 20,0,5,5))  fallos++;   // separados en x
  if ( solapan(0,0,10,10, 0,20,5,5))  fallos++;   // separados en y
  if (!solapan(0,0,10,10, 9,9,2,2))   fallos++;   // tocan esquina
  Serial.print(F("colision self-test: "));
  Serial.println(fallos == 0 ? F("PASS") : F("FAIL"));
}

void dinoReset() {
  juego.posY = DINO_SUELO;
  juego.velY = 0;
  juego.enSuelo = true;
  juego.agachado = false;
  juego.gameOver = false;
  juego.arribaPrev = false;
  juego.reinicioPrev = false;
  juego.puntaje = 0;
  juego.vel = 2;
  juego.tProx = 30;
  juego.anim = 0;
  juego.animCnt = 0;
  for (uint8_t i = 0; i < MAX_OBST; i++) juego.obst[i].activo = false;
}

// Avanza un frame del juego. Estados ya filtrados. 'reiniciar' = boton B1 (SW1).
void dinoUpdate(bool arriba, bool abajo, bool reiniciar) {
  bool reinicioFlanco = (reiniciar && !juego.reinicioPrev);
  juego.reinicioPrev = reiniciar;

  if (juego.gameOver) {
    if (reinicioFlanco) { Serial.println(F("dino: reinicio (B1)")); dinoReset(); }   // flanco de B1
    juego.arribaPrev = arriba;
    return;
  }

  // Salto: flanco de subida y en el suelo.
  if (arriba && !juego.arribaPrev && juego.enSuelo) {
    juego.velY = -IMPULSO;
    juego.enSuelo = false;
  }
  juego.arribaPrev = arriba;

  // Agachado: solo en el suelo y con abajo mantenido.
  juego.agachado = (abajo && juego.enSuelo);

  // Fisica vertical (subpixeles).
  if (!juego.enSuelo) {
    juego.velY += GRAVEDAD;
    juego.posY += juego.velY;
    if (juego.posY >= DINO_SUELO) {
      juego.posY = DINO_SUELO;
      juego.velY = 0;
      juego.enSuelo = true;
    }
  }

  // Mover obstaculos y reciclar los que salen por la izquierda.
  for (uint8_t i = 0; i < MAX_OBST; i++) {
    if (!juego.obst[i].activo) continue;
    juego.obst[i].x -= juego.vel;
    if (juego.obst[i].x < -16) juego.obst[i].activo = false;
  }

  // Generar nuevo obstaculo cuando toca.
  if (--juego.tProx <= 0) {
    for (uint8_t i = 0; i < MAX_OBST; i++) {
      if (!juego.obst[i].activo) {
        juego.obst[i].activo = true;
        juego.obst[i].x = 128;
        juego.obst[i].tipo = (random(0, 10) < 3) ? 1 : 0;  // ~30% pajaro
        break;
      }
    }
    juego.tProx = random(45, 90);  // frames hasta el proximo
  }

  // Caja del dino segun estado (encogida 2 px para una colision justa).
  int16_t dy = juego.agachado ? (GROUND_Y - DUCK_H) : (juego.posY >> 3);
  int16_t dh = juego.agachado ? DUCK_H : DINO_H;

  // Colision contra cada obstaculo.
  for (uint8_t i = 0; i < MAX_OBST; i++) {
    if (!juego.obst[i].activo) continue;
    int16_t ox = juego.obst[i].x, oy, ow, oh;
    if (juego.obst[i].tipo == 0) { ow = 8;  oh = 12; oy = GROUND_Y - oh;      } // cactus
    else                         { ow = 14; oh = 8;  oy = GROUND_Y - 20;      } // pajaro
    if (solapan(DINO_X + 2, dy + 2, DINO_W - 4, dh - 4,
                ox + 1, oy + 1, ow - 2, oh - 2)) {
      juego.gameOver = true;
    }
  }
  if (juego.gameOver) Serial.println(F("dino: game over"));

  // Puntaje y dificultad creciente.
  juego.puntaje++;
  if ((juego.puntaje % 200) == 0 && juego.vel < 6) juego.vel++;

  // Animacion (cada 4 frames alterna).
  if (++juego.animCnt >= 4) { juego.animCnt = 0; juego.anim ^= 1; }
}

// Dibuja un frame del juego en la OLED.
void dinoRender() {
  oled.clearDisplay();
  oled.drawFastHLine(0, GROUND_Y, 128, SSD1306_WHITE);   // suelo

  // Dino.
  if (juego.agachado) {
    oled.drawBitmap(DINO_X, GROUND_Y - DUCK_H, DINO_DUCK, 16, 8, SSD1306_WHITE);
  } else {
    const uint8_t* spr = juego.anim ? DINO_B : DINO_A;
    oled.drawBitmap(DINO_X, juego.posY >> 3, spr, 16, 16, SSD1306_WHITE);
  }

  // Obstaculos.
  for (uint8_t i = 0; i < MAX_OBST; i++) {
    if (!juego.obst[i].activo) continue;
    int16_t ox = juego.obst[i].x;
    if (juego.obst[i].tipo == 0) {                          // cactus
      oled.fillRect(ox + 2, GROUND_Y - 12, 4, 12, SSD1306_WHITE);  // tronco
      oled.fillRect(ox,     GROUND_Y - 8,  3, 2,  SSD1306_WHITE);  // brazo izq
      oled.fillRect(ox + 5, GROUND_Y - 10, 3, 2,  SSD1306_WHITE);  // brazo der
    } else {                                                // pajaro
      int16_t by = GROUND_Y - 20;
      oled.fillRect(ox + 4, by + 3, 8, 2, SSD1306_WHITE);          // cuerpo
      if (juego.anim) {                                            // alas arriba
        oled.drawLine(ox + 4,  by + 3, ox,      by,     SSD1306_WHITE);
        oled.drawLine(ox + 11, by + 3, ox + 15, by,     SSD1306_WHITE);
      } else {                                                     // alas abajo
        oled.drawLine(ox + 4,  by + 4, ox,      by + 7, SSD1306_WHITE);
        oled.drawLine(ox + 11, by + 4, ox + 15, by + 7, SSD1306_WHITE);
      }
    }
  }

  // Puntaje.
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(98, 0);
  oled.print(juego.puntaje);

  // Game over.
  if (juego.gameOver) {
    oled.setCursor(34, 4);
    oled.print(F("GAME OVER"));
    oled.setCursor(16, 14);
    oled.print(F("B1 = reiniciar"));
  }

  oled.display();
}
// ================================================================

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
  Wire.setClock(400000);  // SSD1306 soporta 400 kHz -> frames mas rapidos

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
  probarColision();

  Gamepad.begin();
  Serial.println(F("Gamepad HID iniciado."));
  randomSeed(micros());
  dinoReset();
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
    // Tambien como stick izquierdo: la Gamepad API del navegador y casi todos los
    // juegos leen el stick para moverse, pero NO interpretan el hat (dPad) como
    // direccion. Se mapean las 4 direcciones a xAxis/yAxis con deflexion completa.
    int16_t ejeX = (int16_t)(estadoPrev[IDX_RIGHT] ? 32767 : 0) - (int16_t)(estadoPrev[IDX_LEFT] ? 32767 : 0);
    int16_t ejeY = (int16_t)(estadoPrev[IDX_DOWN]  ? 32767 : 0) - (int16_t)(estadoPrev[IDX_UP]   ? 32767 : 0);
    Gamepad.xAxis(ejeX);
    Gamepad.yAxis(ejeY);
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

  // Game loop del dino en la OLED, a FRAME_MS fijo. No toca el envio HID.
  static uint32_t tUltimoFrame = 0;
  if (oledOk && (ahora - tUltimoFrame >= FRAME_MS)) {
    tUltimoFrame = ahora;
    dinoUpdate(estadoPrev[IDX_UP], estadoPrev[IDX_DOWN], estadoPrev[IDX_BTN[0]]);
    dinoRender();
  }
}
