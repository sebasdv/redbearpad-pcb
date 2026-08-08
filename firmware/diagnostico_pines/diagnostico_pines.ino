/*
 * RedBearPad - DIAGNOSTICO de pines (SIN teclado HID, SIN I2C)
 * Placa: Blend Micro @ redbearlab:avr:blendmicro8 (8 MHz, en spec)
 *
 * Proposito: cazar las "senales fantasma que aparecen a los ~5 minutos".
 * - Muestrea los 8 pines de boton en crudo (sin debounce) y reporta CADA
 *   transicion con timestamp (ms). Un fantasma se ve como transiciones en un
 *   pin que nadie esta tocando, o en un pin vecino al que se pulsa.
 * - Cada 5 s imprime resumen: uptime, Vcc medido por el propio chip (bandgap
 *   interno; detecta caidas del riel 3.3V al calentar), nivel actual de cada
 *   pin y transiciones acumuladas por pin.
 * - NO envia teclas: se puede dejar corriendo 10+ min sin que escriba nada.
 *
 * Uso: cargar, abrir Monitor Serie a 115200, dejar la placa como en un uso
 * normal (misma posicion, mismo cable) SIN tocar botones 5-10 min; despues
 * pulsar de a UN boton y observar si un vecino transiciona.
 */

const uint8_t NUM_BOTONES = 8;
const uint8_t PIN_BOTON[NUM_BOTONES] = { 5, 8, 9, 10, 11, 12, A0, A1 };
const char* const ETIQUETA[NUM_BOTONES] = {
  "B1/Esc(D5)", "B2/W(D8)", "B3/A(D9)", "B4/S(D10)",
  "B5/D(D11)", "B6/Spc(D12)", "B7/Ent(A0)", "B8/Sft(A1)"
};

bool nivelPrev[NUM_BOTONES];            // ultimo nivel crudo leido (true = LOW/pulsado)
uint16_t transiciones[NUM_BOTONES];     // transiciones desde el ultimo resumen
uint32_t transTotal[NUM_BOTONES];       // transiciones desde el arranque
uint32_t tUltimoResumen = 0;
const uint32_t RESUMEN_MS = 5000;

// Vcc real en mV medido contra el bandgap interno de 1.1 V (sin hardware extra).
// Si el riel 3.3V se cae al calentar el regulador, se ve aca.
uint16_t leerVccMv() {
  ADMUX = (1 << REFS0) | 0b011110;      // ref = AVcc, canal = bandgap 1.1V
  delay(2);                             // asentamiento del mux
  ADCSRA |= (1 << ADSC);                // conversion
  while (ADCSRA & (1 << ADSC)) {}
  uint16_t adc = ADC;
  if (adc == 0) return 0;
  return (uint16_t)((1100UL * 1024UL) / adc);
}

void setup() {
  Serial.begin(115200);
  for (uint8_t i = 0; i < NUM_BOTONES; i++) {
    pinMode(PIN_BOTON[i], INPUT_PULLUP);
    transiciones[i] = 0;
    transTotal[i] = 0;
  }
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) {}
  // Nivel inicial despues de estabilizar los pull-ups.
  delay(10);
  for (uint8_t i = 0; i < NUM_BOTONES; i++)
    nivelPrev[i] = (digitalRead(PIN_BOTON[i]) == LOW);
  Serial.println(F("=== DIAGNOSTICO de pines RedBearPad (crudo, sin HID) ==="));
  Serial.println(F("t_ms;pin;evento   <- cada transicion cruda"));
  Serial.print(F("Vcc inicial: "));
  Serial.print(leerVccMv());
  Serial.println(F(" mV"));
}

void loop() {
  uint32_t ahora = millis();

  // --- Deteccion cruda de transiciones (sin filtro: queremos VER el ruido) ---
  for (uint8_t i = 0; i < NUM_BOTONES; i++) {
    bool nivel = (digitalRead(PIN_BOTON[i]) == LOW);
    if (nivel != nivelPrev[i]) {
      nivelPrev[i] = nivel;
      transiciones[i]++;
      transTotal[i]++;
      // Evento con timestamp. Rate-limit: si un pin acumulo >50 transiciones en
      // esta ventana (rebote en rafaga), deja de listar y lo cuenta el resumen.
      if (transiciones[i] <= 50) {
        Serial.print(ahora);
        Serial.print(F(";"));
        Serial.print(ETIQUETA[i]);
        Serial.println(nivel ? F(";LOW (pulsado)") : F(";HIGH (soltado)"));
      }
    }
  }

  // --- Resumen periodico ---
  if (ahora - tUltimoResumen >= RESUMEN_MS) {
    tUltimoResumen = ahora;
    Serial.print(F("--- t="));
    Serial.print(ahora / 1000);
    Serial.print(F("s Vcc="));
    Serial.print(leerVccMv());
    Serial.print(F("mV niveles:"));
    for (uint8_t i = 0; i < NUM_BOTONES; i++)
      Serial.print(nivelPrev[i] ? 'L' : 'H');
    Serial.print(F(" trans(5s):"));
    for (uint8_t i = 0; i < NUM_BOTONES; i++) {
      Serial.print(transiciones[i]);
      Serial.print(i < NUM_BOTONES - 1 ? ',' : ' ');
      transiciones[i] = 0;
    }
    Serial.print(F("total:"));
    for (uint8_t i = 0; i < NUM_BOTONES; i++) {
      Serial.print(transTotal[i]);
      if (i < NUM_BOTONES - 1) Serial.print(',');
    }
    Serial.println();
  }
}
