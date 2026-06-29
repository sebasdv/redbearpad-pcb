/*
 * RedBearPad v2 - XInput (mando Xbox 360) de 12 botones, sin OLED.
 * Placa de compilacion: "Arduino Micro w/ XInput" (FQBN xinput:avr:micro).
 * Sin serial (XInput no es CDC): LED 13 parpadea como "vivo".
 * Reflasheo: doble-reset manual para entrar al bootloader.
 *
 * Mapeo (contrato PCB v2). PIN_BOTON en orden de funcion:
 *   0:Arriba(D3) 1:Abajo(D1) 2:Izq(D2) 3:Der(D0) 4:A(D11) 5:B(D12)
 *   6:X(A0) 7:Y(A1) 8:L(D8) 9:R(D9) 10:Select(D5) 11:Start(D10)
 */
#include <XInput.h>

const uint8_t NUM_BOTONES = 12;
const uint8_t PIN_BOTON[NUM_BOTONES] = { 3, 1, 2, 0, 11, 12, A0, A1, 8, 9, 5, 10 };
const uint8_t IDX_UP = 0, IDX_DOWN = 1, IDX_LEFT = 2, IDX_RIGHT = 3;

bool estadoPrev[NUM_BOTONES];
uint8_t integrador[NUM_BOTONES];
uint32_t tUltimoSample = 0;
const uint8_t SAMPLE_MS = 1;
const uint8_t INTEGRADOR_MAX = 12;

void setup() {
  for (uint8_t i = 0; i < NUM_BOTONES; i++) {
    pinMode(PIN_BOTON[i], INPUT_PULLUP);
    estadoPrev[i] = false;
    integrador[i] = 0;
  }
  pinMode(LED_BUILTIN, OUTPUT);
  XInput.setAutoSend(false);
  XInput.begin();
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
      if (nuevo != estadoPrev[i]) { estadoPrev[i] = nuevo; huboCambio = true; }
    }
  }

  if (huboCambio) {
    bool up = estadoPrev[IDX_UP], down = estadoPrev[IDX_DOWN];
    bool left = estadoPrev[IDX_LEFT], right = estadoPrev[IDX_RIGHT];
    XInput.setJoystick(JOY_LEFT, up, down, left, right);
    XInput.setDpad(up, down, left, right);
    XInput.setButton(BUTTON_A,     estadoPrev[4]);
    XInput.setButton(BUTTON_B,     estadoPrev[5]);
    XInput.setButton(BUTTON_X,     estadoPrev[6]);
    XInput.setButton(BUTTON_Y,     estadoPrev[7]);
    XInput.setButton(BUTTON_LB,    estadoPrev[8]);
    XInput.setButton(BUTTON_RB,    estadoPrev[9]);
    XInput.setButton(BUTTON_BACK,  estadoPrev[10]);
    XInput.setButton(BUTTON_START, estadoPrev[11]);
    XInput.send();
  }

  digitalWrite(LED_BUILTIN, (millis() / 500) % 2);
}
