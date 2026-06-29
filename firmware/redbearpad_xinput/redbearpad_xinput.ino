/*
 * RedBearPad - PRUEBA XInput (mando Xbox 360) en la Blend Micro
 * Placa de compilacion: "Arduino Micro w/ XInput" (FQBN xinput:avr:micro).
 *
 * Por que el variant de 16 MHz (Micro) y no el Pro Micro 8 MHz: el PLL USB de XInput
 * en el variant Micro esta configurado para un cristal de 16 MHz (el de la Blend Micro),
 * asi que es el que puede enumerar. Corre a 16 MHz @ 3.3 V (overclock) -> puede glitchear.
 *
 * SIN serial (XInput no es CDC): el LED 13 parpadea como senal de "vivo".
 * Para volver a subir firmware: doble-reset manual (ventana ~8 s del bootloader Caterina).
 *
 * Mapeo (fijado por la PCB):
 *   Direcciones: SW2=Arriba(D8) SW3=Izq(D9) SW4=Abajo(D10) SW5=Der(D11) -> stick izq + dpad
 *   Botones:     SW1(D5)->A  SW6(D12)->B  SW7(A0)->X  SW8(A1)->Y
 */
#include <XInput.h>

// ----- Switches: un pin por boton, a GND, pull-up interno (activo en LOW) -----
const uint8_t NUM_BOTONES = 8;
const uint8_t PIN_BOTON[NUM_BOTONES] = { 5, 8, 9, 10, 11, 12, A0, A1 };  // SW1..SW8
const uint8_t IDX_UP = 1, IDX_LEFT = 2, IDX_DOWN = 3, IDX_RIGHT = 4;     // direcciones
const uint8_t IDX_BTN[4] = { 0, 5, 6, 7 };                              // SW1,SW6,SW7,SW8
const uint8_t BTN_XBOX[4] = { BUTTON_A, BUTTON_B, BUTTON_X, BUTTON_Y };

// ----- Debounce por integrador con histeresis, muestreo a intervalo FIJO -----
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
  XInput.setAutoSend(false);   // un solo reporte por ciclo, con XInput.send()
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
    // Direcciones -> stick izquierdo (deflexion completa, diagonales por SOCD) y dpad.
    XInput.setJoystick(JOY_LEFT, up, down, left, right);
    XInput.setDpad(up, down, left, right);
    // Botones de accion -> A/B/X/Y.
    for (uint8_t b = 0; b < 4; b++) XInput.setButton(BTN_XBOX[b], estadoPrev[IDX_BTN[b]]);
    XInput.send();
  }

  digitalWrite(LED_BUILTIN, (millis() / 500) % 2);  // heartbeat (~1 Hz)
}
