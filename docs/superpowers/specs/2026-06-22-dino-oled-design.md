# Dino de Chrome en la OLED del gamepad — Diseño

**Fecha:** 2026-06-22
**Estado:** Aprobado

## Problema

El firmware gamepad (`firmware/redbearpad_gamepad/redbearpad_gamepad.ino`) muestra en la
OLED 128x32 una rejilla con el estado de los botones. Se quiere reemplazar ese contenido
por una versión jugable del **juego del dinosaurio de Chrome (T-Rex runner)**: corre solo,
salta con la dirección arriba, se agacha con la dirección abajo, esquiva obstáculos, y al
chocar termina (game over) y reinicia. Sirve además como demo/previsualizador de que los
botones direccionales responden. El dispositivo debe **seguir funcionando como gamepad HID**
mientras el dino corre.

## Solución

Integrar el juego dentro del sketch gamepad existente: el game loop del dino pasa a ser el
contenido de la OLED, **sin alterar la lógica HID** (muestreo de switches + envío de gamepad
siguen igual). La OLED cambia de "redibujar solo al cambiar" a un **bucle a ~30 FPS** con
timestep fijo basado en `millis()`. Se sube el reloj I2C a 400 kHz para frames fluidos.

## Alcance

- **Incluye:** juego completo (dino corriendo, cactus y pájaros, salto/agachada, colisión
  AABB, game over + reinicio, puntaje), integrado al firmware gamepad, con el HID intacto.
- **No incluye:** sonido, tabla de récords persistente (EEPROM), animaciones extra (nubes,
  día/noche), ni cambios en la PCB o en el firmware keypad.

## Estructura de archivos

```
firmware/redbearpad_gamepad/redbearpad_gamepad.ino   (MODIFICAR)
docs/superpowers/specs/2026-06-22-dino-oled-design.md (este documento)
```

El juego se organiza en funciones propias dentro del .ino (estado global del juego +
`dinoReset()`, `dinoUpdate()`, `dinoRender()`), separadas de la lógica HID, para que cada
parte se entienda y se pruebe por separado. Se elimina/reemplaza `dibujarPantalla()`.

## Componentes

### 1. Integración con el loop existente

`loop()` mantiene su estructura: muestreo de switches a 1 ms con el debounce por integrador,
y envío HID (stick + botones + hat) cuando cambia un estado. Se **añade** un bloque de juego
con timestep fijo:

```
si (millis() - tUltimoFrame >= FRAME_MS):   // FRAME_MS = 33 (~30 FPS)
    tUltimoFrame += FRAME_MS
    dinoUpdate(arribaPulsado, abajoPulsado)
    dinoRender()
```

`arribaPulsado`/`abajoPulsado` salen de `estadoPrev[IDX_UP]` / `estadoPrev[IDX_DOWN]` (ya
filtrados). El juego **lee** esos estados; no interfiere con el envío HID, que ocurre aparte.
Se llama `Wire.setClock(400000)` en `setup()` tras `Wire.begin()`.

### 2. Estado del juego

Estructura con: posición/velocidad vertical del dino (para el salto), bandera de agachado,
estado (corriendo / game-over), lista de obstáculos (posición X, tipo cactus/pájaro),
distancia recorrida (puntaje), velocidad de scroll actual, y temporizador para generar el
próximo obstáculo.

### 3. Mecánica (`dinoUpdate`)

- **Correr:** los obstáculos avanzan de derecha a izquierda a `velocidad`, que sube de a poco
  con la distancia. Se generan a intervalos pseudoaleatorios con separación mínima jugable.
- **Saltar (arriba):** si el dino está en el suelo y se pulsa arriba, se le da velocidad
  vertical; gravedad constante por frame lo baja hasta el suelo. No se puede re-saltar en el aire.
- **Agacharse (abajo):** mientras abajo esté pulsado y el dino esté en el suelo, usa el sprite
  agachado (más bajo y ancho) para pasar bajo los pájaros.
- **Obstáculos:** **cactus** al ras del suelo (obligan a saltar) y **pájaro** a media altura
  (obliga a agacharse). Ambos tipos para que arriba y abajo importen.
- **Colisión:** cajas AABB entre el dino (según esté saltando/agachado/normal) y cada
  obstáculo. Si solapan → estado game-over.
- **Puntaje:** entero que crece con la distancia.

### 4. Game over y reinicio

En game-over el juego deja de avanzar y `dinoRender()` muestra el puntaje final y
"arriba = reiniciar". Al pulsar **arriba** (flanco de subida, no mantenido) se llama
`dinoReset()` y vuelve a correr.

### 5. Render (`dinoRender`) — OLED 128x32

- Línea de suelo cerca del borde inferior.
- Dino a la izquierda (~12×14 px), con 2 frames de animación de patas al correr; sprite
  agachado más bajo/ancho.
- Cactus (~6×12) y pájaro (~12×8) como bitmaps en PROGMEM, dibujados con `drawBitmap`.
- Puntaje como texto chico arriba a la derecha.
- Pantalla de game-over: "GAME OVER", puntaje y la indicación de reinicio.
- Un `clearDisplay()` + dibujo + `display()` por frame.

### 6. Convivencia con el gamepad

Solo arriba/abajo afectan al dino; los otros 6 switches **siguen enviando HID** y no tocan el
juego. El dino y el gamepad operan en paralelo: pulsar arriba salta el dino *y* manda la
dirección al PC.

## Manejo de errores / límites

- **OLED ausente** (`oledOk == false`): el juego no se dibuja (se omite el bloque de render),
  pero el gamepad sigue funcionando. La lógica `dinoUpdate` puede correr o saltarse; se omite
  todo el bloque de juego si no hay OLED para no gastar ciclos.
- **Latencia HID:** el redibujo bloquea el loop ~12 ms por frame (a 400 kHz), añadiendo ~12 ms
  de latencia al muestreo/HID mientras el dino corre. Trade-off aceptado para el demo.
- **Rollover de `millis()`:** se usa resta de enteros sin signo, segura ante el desbordamiento.

## Verificación

1. Compilar para `redbearlab:avr:blendmicro8` (y `blendmicro16` como sanity) sin errores.
2. En hardware (usuario): el dino corre y anima; arriba salta, abajo agacha; choca con cactus
   y pájaro → game over con puntaje; arriba reinicia. El gamepad sigue respondiendo en
   `joy.cpl` mientras el dino corre.
3. Auto-test on-device existente (`dpad self-test: PASS`) se mantiene.

Recordatorio: flashear siempre **blendmicro8** (8 MHz); 16 MHz es overclock fuera de spec.

## Fuera de alcance / futuro

- Récord en EEPROM, sonido (no hay buzzer), modo noche, dificultad configurable.
- Un selector de "modo" (gamepad normal vs dino) si más adelante se quiere alternar el
  contenido de la OLED sin reflashear.
