# 2D rol-jokoa — programazio proiektua

> Rol-joko (RPG) bat, bi dimentsiotan, C lengoaian eta zerotik garatua.

![C](https://img.shields.io/badge/lengoaia-C99-blue)
![raylib](https://img.shields.io/badge/liburutegia-raylib-lightgrey)
![platform](https://img.shields.io/badge/plataforma-Windows%20%7C%20Linux-green)
![status](https://img.shields.io/badge/egoera-garapenean-orange)

---

## Proiektuari buruz

Programazio ikasgairako garatutako proiektua da hau. Helburua ez da joko komertzial bat
argitaratzea, baizik eta C lengoaia ikastea eta joko batek barrutik nola funtzionatzen
duen ulertzea. Horregatik dago ia dena eskuz idatzita.

Ez da joko-motorrik (*game engine*) erabili. `raylib` liburutegiak hiru gauza baino ez
ditu egiten: leihoa sortu, *input*-a irakurri eta pixelak pantailan marraztu. *Game
loop*-a, entitate-sistema, mugimendua, kolisioak eta joko-logika osoa proiektu honetako
kodea dira.

Kodea eta komentarioak **ingelesez** daude, konbentzioz; dokumentazioa, euskaraz.

---

## Egoera

Garapenean dago. Oinarria —*game loop*-a, *input*-a eta entitateak— martxan dago;
edukia eta sistema grafikoak falta dira.

| Atala | Egoera |
|---|---|
| *Game loop* (*fixed timestep*) | Eginda |
| *Input* sistema | Eginda |
| Entitate-sistema | Eginda |
| Pantaila-egoerak (menua / jokoa / pausa / ezarpenak) | Eginda |
| Testuak hiru hizkuntzatan (ES / EN / EU) | Eginda |
| Ezarpenen menua (`settings.conf`-en gordeta) | Eginda |
| UI editorea (`tools/ui_editor`) | Eginda |
| Leiho birdimentsionagarria eta pantaila osoa | Eginda |
| Kolisioak (AABB) | Eginda |
| Texturak | Eginda |
| Kamera | Eginda |
| Animazioak | Egiteke |
| *Tile* bidezko mapak | Egiten |
| Elkarrizketa-sistema | Egiteke |

---

## Ezaugarri teknikoak

**Denbora eta *game loop*.** Joko-logika *fixed timestep* batekin exekutatzen da,
segundoko 60 *tick*, *accumulator* baten bidez; errendatzea, berriz, askea da. Horrela
jokoak berdin funtzionatzen du edozein ordenagailutan, *frame rate*-a edozein izanda
ere. *Frame* luzeegi bat gertatzen bada, denbora mozten da (*clamp*), *tick* pilaketa
saihesteko.

**Entitateak.** Tamaina finkoko *array* bakar batean bizi dira, `active` marka batekin
(*memory pool*). Ez dago `malloc`/`free` deirik *game loop*-aren barruan: memoria guztia
hasieran erreserbatzen da.

**Egitura.** Joko-egoera `main`-ean dago eta moduluek erakusleak (*pointers*) jasotzen
dituzte; ez dago aldagai global ez *singleton*-ik. Modulu bakoitzak gauza bat egiten du:
`input` teklatua irakurtzen du, `entity` entitateak mugitu eta marrazten ditu, `screen`
marrazketaren ordena erabakitzen du.

**Ezarpenak.** Menuak kopia bat editatzen du, eta *Aplikatu* sakatzean bakarrik pasatzen
da jokora eta `settings.conf`-era (`gakoa: balioa`, lerro bakoitzeko bat). Lerro hutsak
eta `#`-z hasten direnak (iruzkinak) saltatu egiten dira, eta ulertzen ez den lerro batek
abisua ematen du, lerro-zenbakiarekin. Fitxategirik ez badago, balio lehenetsiak
erabiltzen dira.

**Kolisioak.** AABB (*axis-aligned bounding box*) bidez, ardatz bakoitza bestearen
ondoren ebatzita. Horri esker pertsonaia hormetan zehar irristatzen da, trabatu gabe.

---

## Beharrezko tresnak

### Linux (Ubuntu / Debian)

```sh
sudo apt install build-essential gcc-mingw-w64-x86-64
```

raylib iturburutik konpilatzen da, liburutegi estatiko gisa:

```sh
sudo apt install cmake git libasound2-dev libx11-dev libxrandr-dev libxi-dev \
                 libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev
git clone --depth 1 https://github.com/raysan5/raylib.git
cmake -S raylib -B raylib/build -DCMAKE_BUILD_TYPE=Release
sudo cmake --build raylib/build --target install
```

> **Oharra — *root* baimenik gabe.** raylib `$HOME`-en instala daiteke,
> `-DCMAKE_INSTALL_PREFIX=$HOME/.local` erabiliz. Kasu horretan `make`-i bide hori
> adierazi behar zaio: `make PREFIX=$HOME/.local`.

### Windows-erako exekutagarria

Windows-erako `.exe`-a Linuxetik sortzen da, `mingw-w64` *cross-compiler*-arekin.
Horretarako raylib-en **`win64_mingw-w64`** bertsioa deskargatu behar da
([raylib releases](https://github.com/raysan5/raylib/releases)) eta `lib/raylib-win/`
karpetan atera, egitura hau geratzeko:

```
lib/raylib-win/include/raylib.h
lib/raylib-win/lib/libraylib.a
```

> **Oharra — *root* baimenik gabe.** `mingw-w64` instalatu ezin bada,
> [llvm-mingw](https://github.com/mstorsjo/llvm-mingw/releases) `$HOME`-en atera daiteke
> eta haren `bin/` karpeta `PATH`-era gehitu, `make win` egin aurretik:
> `export PATH=~/.local/opt/llvm-mingw-.../bin:$PATH`.

---

## Konpilazioa

| Komandoa | Emaitza |
|---|---|
| `make` | Linuxerako *debug* bertsioa → `build/game` |
| `make run` | Konpilatu eta exekutatu |
| `make win` | Windows 64 bit-erako `.exe` → `build/game.exe`, eta zip-a → `build/game.zip` |
| `make RELEASE=1 win` | Bertsio optimizatua, kontsola-leihorik gabe |
| `make clean` | `build/` karpeta ezabatu |

**Entregatzeko bertsioa:**

```sh
make RELEASE=1 win
```

Sortzen den `build/game.exe` fitxategi bakarra da eta estatikoki lotuta dago
(*statically linked*): ez du DLLrik ez beste menpekotasunik behar. `assets/` karpeta
eta `settings.conf` exekutagarriaren ondoan egon behar dira. `build/game.zip`-ek hiruak
dakartza, entregatzeko prest.

---

## Proiektuaren egitura

```
.
├── Makefile
├── README.md
├── src/              # iturburu-kodea (.c): main.c eta game.c, gainerakoa karpetetan
│   ├── world/        #   mapa, entitateak, jokalaria, kamera, talkak
│   ├── ui/           #   canvas, testua, widget-ak
│   ├── resources/    #   texturak, soinuak, bolumena
│   └── settings/     #   ezarpenak, .conf fitxategia, teklak
├── include/          # goiburuak (.h), src/-ren karpeta berdinekin
├── settings.conf     # ezarpen lehenetsiak
├── assets/           # sprite-ak, tileset-ak, soinua
├── lib/raylib-win/   # raylib, Windows-erako cross-compilaziorako
└── build/            # sortutako fitxategiak (ez dago bertsio-kontrolpean)
```

---

## Kontrolak

| Tekla | Ekintza |
|---|---|
| `W` `A` `S` `D` | Mugitu |
| `Enter` | Jokoa hasi |
| `Esc` | Pausatu / jarraitu |
| `F11` | Pantaila osoa |
| `F3` | *Debug* modua (ezin da aldatu) |

Teklak ezarpenen menuan alda daitezke (*Kontrolak*).

---

## Kredituak

Grafikoak eta soinua: **Ninja Adventure Asset Pack**, [Pixel-Boy](https://pixel-boy.itch.io/ninja-adventure-asset-pack)
(CC0 lizentzia).

Liburutegia: [raylib](https://www.raylib.com/), Ramon Santamaria (zlib/libpng lizentzia).

---

## Egilea

**Danel (LenadESP)** — Programazio ikasgaia, 2026/2027 ikasturtea.