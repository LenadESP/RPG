# Assets del menú, recortados

Recortados desde los originales de 1672x941 quitando el relleno transparente.
Umbral de alpha 4 (por debajo de eso solo había ruido del exportador repartido
por todo el canvas) y 2px de margen para no comerse el glow de los bordes.

`menu_completo.png` no está: es la preview.

VRAM: 42.0 MB -> 21.2 MB. El background es la mitad de eso y no se puede tocar,
porque ocupa el canvas entero de verdad.

## sheets/  — los sheets recortados, un archivo por asset

| archivo | tamaño | esquina que ocupaba en el canvas 1672x941 |
|---|---|---|
| background.png | 1672x941 | 0,0 (sin alpha, copiado tal cual) |
| logo.png | 1443x935 | 123,0 |
| boton_normal.png | 1340x258 | 167,340 |
| boton_hover.png | 1244x242 | 214,350 |
| textos_jugar_play_jolastu.png | 745x724 | 467,124 |
| textos_salir.png | 1138x690 | 269,151 |
| textos_configuracion.png | 1032x655 | 321,157 |

La columna de la derecha es dónde estaba el dibujo dentro del canvas original.
Sirve si quieres reproducir la composición de la preview: la posición en canvas
de cada pieza es esa esquina.

### srcrec de cada idioma dentro de su sheet

Orden de filas: castellano, inglés, euskera.

textos_jugar_play_jolastu.png (745x724)
  es  { 0,   2, 745, 223 }
  en  { 0, 268, 745, 218 }
  eu  { 0, 532, 745, 190 }

textos_salir.png (1138x690)
  es  { 0,   2, 1138, 192 }
  en  { 0, 252, 1138, 187 }
  eu  { 0, 499, 1138, 189 }

textos_configuracion.png (1032x655)
  es  { 0,   2, 1032, 179 }
  en  { 0, 233, 1032, 179 }
  eu  { 0, 475, 1032, 178 }

Ojo: cada idioma tiene ancho y alto distintos porque las palabras miden lo que
miden. El srcrec coge la fila entera del sheet, así que el ancho es el del sheet
y la palabra va centrada dentro con transparencia a los lados. Eso es bueno:
dibujas las tres igual y quedan centradas solas.

## split/  — lo mismo pero un PNG por idioma

Nueve archivos, cada uno recortado a su propia palabra. Si usas estos no hace
falta srcrec: dibujas la textura entera. A cambio son 9 texturas en vez de 3 y
cada palabra tiene su tamaño, así que el centrado lo haces tú.

Elige una de las dos carpetas, no las dos.

## Nota sobre las placas

boton_normal es 1340x258 y boton_hover es 1244x242, o sea el hover es MÁS
PEQUEÑO. En el canvas original los centros coinciden casi exactos (836,468 y
836,470), así que si dibujas las dos centradas en el mismo punto el hover se ve
como un encogimiento sutil. Queda como efecto de pulsado y no cuesta nada.
