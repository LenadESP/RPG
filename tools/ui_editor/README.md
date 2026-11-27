# ui_editor

Editor de los menús del juego (las tablas `CanvasElement` / `CanvasElementText` / `CanvasWidget`).
Independiente del juego: **lee** `include/` y `src/`, **nunca los escribe**. Lo único
que escribe es `tools/ui_editor/out/`.

```sh
cd tools/ui_editor
make          # build/ui_editor   (raylib en ~/.local; si no: make PREFIX=/usr/local)
make run      # abre el editor sobre este repo
make test     # sin ventana: lee cada pantalla, la exporta, relee el export y compara
```

## Qué lee del juego

| fichero | qué saca |
|---|---|
| `include/texture.h` | `enum TextureId` |
| `src/texture.c` | `texture_defs[]`: ruta, frame, smooth |
| `include/text.h` | `enum Language`, `enum TextId`, `enum TextStyle` |
| `src/text.c` | `text_defs[]` (las frases), `text_style_defs[]`, `font_path`, `font_characters` |
| `include/colors.h` | los `COLOR_*` de los estilos |
| `include/settings.h`, `include/canvas.h` | `enum SettingId`, `enum WidgetType` |
| `include/defines.h` | `CANVAS_WIDTH`, `CANVAS_HEIGHT`, `TEXT_OUTLINE_WIDTH`, `TEXT_SHADOW_OFFSET`, `FONT_LOAD_SIZE` |
| `src/*.c` | cada función con tablas `CanvasElement`/`CanvasElementText`/`CanvasWidget` = una pantalla |

Si una tabla tiene algo que no entiende (una expresión tipo `1672.0f / 2`, un
hueco en los índices...), avisa y no la carga, en vez de adivinar.

## Uso

- **Open...**: una pantalla del juego (`game_menu_draw`, `game_pause_draw`) o un borrador de `out/`.
  **New canvas...**: pantalla nueva (p. ej. `settings_menu`).
- **Izquierda**: `Textures` = las `TEX_*` del juego; `assets/` = todos los PNG; `Texts` = los `TXT_*`;
  `Widgets` = los `WIDGET_*` (slider, checkbox...). Un widget nuevo sale con el primer ajuste: cámbialo a la derecha.
  Arrastra al lienzo (o doble clic). Texturas y PNG entran como imagen, textos como label.
  Un PNG de `assets/` se convierte en textura nueva (verde); nombre, frame y smooth editables.
  Los textos nuevos no se crean aquí: añádelos a `include/text.h` y `src/text.c` y dale a Reload.
- **Label sobre un botón**: se encaja en su mismo rect (como en game.c). Mover el botón mueve sus labels.
  **Label sobre un label**: le cambia el texto. ES/EN/EU arriba cambia el idioma que se ve.
- **Derecha**: propiedades (rect, clickable + hover, texto + estilo, tipo + ajuste del widget, lock) y el orden
  de dibujado (`E` = UIElements, `T` = UITexts, `W` = UIWidgets; los índices son los del export).
  Los widgets se ven aproximados (sin valores reales): lo que cuenta es el rect.
- **Build** (Ctrl+B / Ctrl+S): dos bloques, cada uno con su botón de copiar:
  1. texturas nuevas: líneas para el enum de `texture.h` y para `texture_defs[]` de `texture.c`
  2. las tablas, con el formato de game.c y un comentario con los índices clicables

  Se guarda también en `out/<pantalla>.c` (es tu "guardar"; se puede reabrir) y se imprime en la terminal.

Atajos: rueda = zoom, botón central / Espacio+arrastrar = mover vista, F = encajar,
flechas = mover 1px (Shift: 10), Shift en esquina = mantener proporción, Alt = sin snap,
Ctrl+D duplicar, Supr borrar, RePág/AvPág reordenar, Ctrl+Z / Ctrl+Y.

"Clickable" solo pone `hover_effect = true` y la textura hover en la tabla; qué hace el clic
sigue siendo cosa tuya en el código (por índice, como ahora).

## Sin ventana

```sh
./build/ui_editor ../.. --list                 # texturas y pantallas encontradas
./build/ui_editor ../.. --build game_menu_draw # imprime los dos bloques
./build/ui_editor ../.. --selftest
```

Para tests: `UIED_OPEN=<pantalla> UIED_SHOT=x.png` abre, hace captura y sale.

## Límites

- `game_load_draw` usa `Rectangle` sueltos con `draw_glitched`, no tablas: no aparece.
- Una tabla de cada tipo por función.
