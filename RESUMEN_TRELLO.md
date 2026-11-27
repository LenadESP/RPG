# Resumen del Trello — RPG (Izar Hautsia)

Sacado del tablero el 26/11/2026 y actualizado el 27/11/2026, después de la fuente, la config y las subcarpetas. Sirve para ponerse al día; el detalle sigue en cada tarjeta.

---

## 🏗️ Arquitectura (decidido, no se rediscute)

- **Game loop**: fixed timestep a 60 ticks/s con accumulator y clamp de 0.25 s; el render va libre. Reset de `lastTime` + accumulator al entrar en PLAYING.
- **Entidades**: un `struct Entity` plano en un memory pool fijo `entities[MAX_ENTITIES]` con flag `active`. Nada de malloc en el loop.
- **INVARIANTE**: el player es `entities[0]`. El array nunca se reordena ni se compacta, y el orden de dibujado irá por un array aparte de índices.
- **Ancla**: `x,y` es el centro de los pies. La hitbox (`hb_w`/`hb_h`) va en Entity y el offset del sprite en la tabla de texturas. `collision.c` no sabe que existen las texturas y `texture.c` no sabe que existen las entidades.
- **Colisiones**: resolución por ejes (X y luego Y) con snapping al borde del otro. Se mira el `solid` del OTRO. Contra tiles: `map_is_solid`, y fuera del mapa cuenta como sólido. `floorf` antes del cast a int.
- **Tilemap**: Tiled exporta a CSV, una layer por tileset y UNA de colisión ("pintado = sólido"). El parser recibe un buffer, no una ruta. `layer_cut` separa las capas de debajo y de encima del player.
- **Texturas**: tabla `texture_defs` con designated initializers y el array dentro de `texture.c`. `TEX_NONE` (el damero) va el primero del enum: si falla él, `LOG_FATAL`; si falla cualquier otra, warning y damero.
- **Estado**: es de `main`, y los módulos reciben punteros. Las transiciones van por callbacks (`start_game`, `open_settings`…) y la salida está en la condición del `while`. `GAME_SETTINGS` recuerda en `state_before_settings` de dónde viene (menú o pausa) para volver ahí.
- **Shutdown**: `game_shutdown` registrado en `atexit`. Nada de `assert` para invariantes, porque release lleva `-DNDEBUG`.
- **UI**:
  - Canvas de 1672x941 con letterbox (se queda la escala menor). El ratón se convierte a canvas, no al revés.
  - Tres tablas por pantalla: `CanvasElement` (imágenes), `CanvasElementText` (texto + estilo) y `CanvasWidget` (controles).
  - Las acciones de los botones van a pelo, por índice: **no se reabre**. Los widgets no usan índices: cada uno dice qué ajuste toca.
  - Helpers en `canvas.c`: `canvas_get_transform` (escala + offset en un `CanvasTransform`), `canvas_mouse`, `canvas_draw_texture`, `canvas_draw_elements` y `canvas_draw_texts`. Todos los menús los usan.
- **Textos**:
  - `text_defs[TXT_X][LANG_X]` en `text.c`; si falta una traducción se usa `LANG_ES`. Fuente Alagard (`AlagardES.ttf`, con los acentos añadidos por `tools/font/add_accents.py`).
  - Estilos en `text_style_defs`: tamaño, centrado o a la izquierda, contorno (botones) o sombra (el resto), y color.
  - Los colores van con nombre en `colors.h`, **nunca RGB a pelo**.
- **Widgets** (`widget.c`): slider, checkbox, desplegable, radio y tecla. El desplegable abierto se dibuja encima de todo y se come el clic.
- **Settings** (`settings.c`):
  - Cada ajuste es un `int` de `SettingId`: sliders de 0 a 100, checkbox 0/1, teclas como `KEY_*` de raylib, y el resto como su enum.
  - El menú edita la copia `pending`, que solo pasa a `applied` con Aplicar (Restablecer aplica directo y Cancelar o Esc descartan).
  - Las teclas son ajustes, y `input.c` las lee con `settings_get`.
- **Config en disco**: `conf.c` es genérico y trabaja con líneas `clave: valor`. `settings.conf` va en el repo con los defaults y se guarda al aplicar, al restablecer y con F11. Si falta el fichero, se usan los defaults y sale un warning.
- **Cámara**: Camera2D en main, pegada al player sin lerp. Zoom entero calculado desde la ventana (resolución virtual 240x136, suelo de 1).
- **Audio**: `sound.c` y `music.c` son hermanos, no capas. `main` hace el `InitAudioDevice`. Seis canales en `volume.c`, puestos por los sliders de la config. Un audio que falta es warning, nunca fatal.
- **Parámetros**: los internos en `defines.h` (macros con prefijo de familia). La config de usuario va aparte, en runtime (`settings.conf`).
- **Carpetas**:
  - `src/` e `include/` tienen las mismas subcarpetas: `world/`, `ui/`, `resources/` y `settings/`. `main.c` y `game.c` se quedan arriba.
  - El Makefile compila cualquier `.c` y mete todas las carpetas de `include/` en `-I`, así que los `#include` van sin carpeta.
- **Release**: **embeber assets DESCARTADO**. `make win` saca `build/game.zip` con el exe, `settings.conf`, README, LICENSE y **solo los assets cuya ruta `"assets/..."` aparece en `src/`** (~7 MB). Si falta uno, `make` se para. Una ruta montada en runtime hay que meterla en `EXTRA_ASSETS`.
- **Criterio general**: merece arquitectura lo que se va a volver a tocar; lo que se escribe una vez y se olvida, no.

---

## 🔨 On it

- **`music.c`**: sin empezar (`sound.c` ya está hecho, con 3 sonidos de menú). El diseño está cerrado:
  - `play_music(id, segundos_de_fade)`, con el fade en tiempo de frame, no en ticks.
  - Una sola ranura saliente: si llega otra transición, se corta en seco.
  - INVARIANTE: parar antes de reproducir (fatal en dev, warning en release).
  - Falta decidir dónde va el `UpdateMusicStream` de cada frame.
  - Recuerda: `assets/Audio/` está en `.gitignore`, así que un clon limpio arranca mudo y **no es un bug**. Ojo: **el zip de `make win` tampoco llevará audio** si se compila desde un clon limpio.

---

## 📋 TODO

1. **`level.c`**: desbloqueada, ya hay dos mapas (`camino_sur`, `valdoria_plaza`). Tiene que llevar la tabla de capas y el `layer_cut` de cada nivel, que hoy están a pelo en main. El cambio de nivel se marca con un flag y se ejecuta en el frame siguiente, para evitar el use-after-free. Volver al menú tiene que descargar el nivel.
2. **Animaciones**: a 10 fps. Falta decidir si avanzan por ticks o por segundos, y si el offset es por textura o por frame. Solo hacen falta Walk e Idle.
3. **Debug overlay**: FPS, frame time y % de CPU sobre 16,67 ms, en espacio de pantalla y detrás de F3.
4. **Rewrite del movimiento**: drag e inercia, con `vx`/`vy` como estado y el input como aceleración. Candidato a Navidades, y después del overlay.
5. **Entidades interactuables**: sin decidir si van con enum, bitflags o puntero a función, ni cómo se detecta la interacción.
6. **Historia**: está escrita en `Story/`. Falta trocearla en `text_defs` (ya existe la tabla), decidir cuánta entra y **decidir el combate**, que está completamente abierto y es la pieza más grande que queda.
7. **Usar las ayudas y la velocidad del texto**: los ajustes ya existen y se guardan, pero nadie los lee todavía. Se leerán con `settings_get(SETTING_SHOW_HINTS)` y `settings_get(SETTING_TEXT_SPEED)` cuando haya diálogos.
8. **Release**:
   - `ChangeDirectory(GetApplicationDirectory())` al arrancar, que ahora importa también para encontrar `settings.conf`.
   - Probar el zip desde una carpeta cualquiera.
9. **Semana de release**: preguntar a la profe qué máquina usa. **El riesgo gordo es OpenGL 3.3 contra 2.1/3.0**: si no llega, `InitWindow` falla y hay que recompilar raylib con `GRAPHICS_API_OPENGL_21`.

---

## 🐛 Bugs

- **F3 no dibuja la hitbox del player**: el `DrawRectangleLinesEx` se quedó dentro del bucle que empieza en `i = 1`. Tampoco se pinta la capa de colisión (`TEX_TILESET_COLLISIONS` se carga y no la usa nadie).
- **El texto de coordenadas X/Y sale en todos los estados**: está fuera del `switch` y del `if (debug)`, así que se ve hasta en el menú, en la config y en la release. O se borra, o se convierte en el principio del debug overlay.
- **Doble `UnloadTexture` en el fallback**: al copiar la struct del damero, varias entradas comparten un id de GPU. Además, la entrada caída hereda los metadatos del damero. Arreglo sin decidir: flag `is_fallback`, descargar por id único, o resolver el fallback en `texture_get`.
- **`settings.conf` no valida rangos**: un `language: 7` o un `display_mode: 9` metido a mano se sale de los arrays (`text_defs`, `dropdown_options`). Hay que hacer un clamp al leer.
- **F12 hace capturas**: raylib saca un `screenshotNNN.png` con F12 por defecto. Si se asigna F12 a una acción, se hacen capturas sin querer. Se arregla con `SetConfigFlags`/`SUPPORT_SCREEN_CAPTURE` o filtrando F12 en el rebind.

---

## 🧹 Deuda técnica / Sanity

- **Ordenado por Y** de las entidades: array aparte de índices con insertion sort, y el player como una más. Urge en cuanto haya NPCs.
- **Consumir los flancos dentro del loop de ticks**: urge en cuanto se meta atacar.
- **Teclas guardadas como número** en `settings.conf` (`key_up: 87`). Si se quieren legibles (`key_up: W`), son unas líneas más.
- **Cámara**: `camera_update` está dentro de `BeginDrawing` y también corre en el MENU. Hay includes redundantes en `camera.c` y hay que revisar el `-Wno-unused-parameter`.
- **`game_pause_draw` y `game_settings_draw`, con 8 y 7 parámetros**: aparcado a propósito. Si se retoma, agrupar solo el mundo y dejar las transiciones sueltas.
- **Invariantes apuntados**:
  - `entities[0]` es el player.
  - Un texto sin traducir va siempre en `LANG_ES` (índice 0).
  - Los volúmenes de `SettingId` van en el mismo orden que `ChannelId`.
  - Las pestañas de la config son `UIElements` 6 a 10, en el orden de `SettingsTab`.

### Cosas sueltas a limpiar
- `FADE_DURATION` está en `defines.h` y no lo usa nadie: o se borra o se le da uso.
- Comentario obsoleto en `game_pause_draw`: `//TEX_NONE: resume and exit-to-menu art doesn't exist yet`.
- `assets/Maps/map.csv` suelto: mirar si se puede borrar antes de que acabe en el zip.
- `GameState` y su switch estarían mejor en `game.c` que en `main.c` (no urge).
- Falta probar el letterbox con una ventana que no sea 16:9 (que no se deforme y que el hover acierte).
- `screenshot000-004.png` en la raíz del repo, de probar el F12.

---

## ✅ Hecho

- **Base**: toolchain (Linux y mingw estático), Makefile con `-Wall -Wextra`, input, colisiones + F3, cámara con zoom entero y clamp, hitbox separada del sprite, ancla en los pies, sistema de texturas, sprite del player y tilemap con colisión contra tiles.
- **Código**: `defines.h`, rename de screen → game (archivos y include guard).
- **Menús**: pantalla de carga con artefactos, menú principal y de pausa con letterbox, y `sound.c` + `volume.c` con sonidos de hover, aceptar y cancelar.
- **Player encima de las entidades**: arreglado.
- **Sistema de textos (ES/EN/EU)**: tablas en C en vez del fichero `indice|texto` que se planeó, con la fuente Alagard + acentos, estilos de texto y `colors.h`. Los 15 sprites de texto se borraron.
- **Menú de configuración completo**, con cinco pestañas:
  - General: ayudas y velocidad del texto.
  - Vídeo: modo de pantalla (ventana, completa o sin bordes) y brillo, con un mínimo real del 20%.
  - Audio: los 6 canales.
  - Controles: 7 teclas reasignables, con las repetidas en rojo y Esc para cancelar.
  - Idioma: euskera por defecto.
  - Abajo, Aplicar, Cancelar y Restablecer. Se abre desde el menú y desde la pausa.
  - Se hizo con sliders de verdad en vez de los botones +/− que se habían planeado.
- **Config persistente** en `settings.conf` (`clave: valor`). Quedan pendientes la validación de rangos (ver Bugs) y el framerate.
- **Editor de UI** (`tools/ui_editor`): lee y exporta las tres tablas y abre cada pestaña de la config como una pantalla. El selftest da 8/8.
- **Subcarpetas en `src/` e `include/`**, con el Makefile adaptado.
- **`make win` saca el zip**, solo con los assets que se usan.
- **Limpieza de la UI**: la escala va en `CanvasTransform`, y el menú y la pausa usan los helpers. Mientras se espera una tecla, el resto de teclas no hacen nada.

---

## ⏰ Plazo

- Unos 3 meses en total. **Navidades es tiempo de trabajo, no la entrega.**
- **Fecha exacta sin confirmar: preguntarla.**
- Nuevo en el encargo: explicarle a la clase cómo se hace el juego (sin planificar).
