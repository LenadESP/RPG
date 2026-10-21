# Izar Hautsia

> 2D rol-jokoa, istorioan oinarritua, C lengoaian zerotik garatua.

![C](https://img.shields.io/badge/lengoaia-C99-blue)
![raylib](https://img.shields.io/badge/liburutegia-raylib-lightgrey)
![platform](https://img.shields.io/badge/plataforma-Windows%20%7C%20Linux-green)

---

## Proiektuari buruz

Programazio ikasgairako garatutako proiektua da hau: rol-joko (RPG) sinple bat, bi
dimentsiotan eta istorio lineal batean oinarritua. Jokalariak mapan zehar mugitu,
pertsonaiekin hitz egin eta etsaiei aurre egin diezaieke, istorioa aurrera doan heinean.

Jokoaren muina — grafiko-motorra, fisika eta joko-logika osoa — **C lengoaian idatzita
dago, zerotik**. Ez da joko-motorrik (*game engine*) erabili: `raylib` liburutegiak
leihoa sortu, sarrera (*input*) irakurri eta pixelak pantailan marraztea besterik ez du
egiten; gainerako guztia proiektu honetako kodea da.

Kodea eta komentarioak **ingelesez** daude, konbentzioz. Dokumentazioa, aldiz, euskaraz.

---

## Ezaugarri teknikoak

| Atala | Inplementazioa |
|---|---|
| Joko-begizta | *Fixed timestep* (60 Hz), errendatzea bereizita |
| Fisika | Integrazio sinplea + ardatzez ardatzeko ebazpena |
| Kolisioak | AABB (*axis-aligned bounding box*) propioa |
| Mapak | *Tile* bidezko sistema, bi dimentsioko arrayetan |
| Entitateak | Tamaina finkoko array bat, `active` markarekin (*memory pool*) |
| Istorioa | Elkarrizketa-sistema eta egoera-makina sinple bat |

Memoria kudeaketa estatikoa da ia osorik: entitateak aurrez erreserbatutako array batean
bizi dira, beraz ez dago `malloc`/`free` deirik joko-begiztaren barruan.

---

## Beharrezko tresnak

### Linux (Ubuntu / Debian)

```sh
sudo apt install build-essential gcc-mingw-w64-x86-64 libraylib-dev
```

`libraylib-dev` paketea zaharkituta badago edo eskuragarri ez badago, raylib
iturburutik konpila daiteke:

```sh
sudo apt install cmake git libasound2-dev libx11-dev libxrandr-dev libxi-dev \
                 libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev
git clone --depth 1 https://github.com/raysan5/raylib.git
cmake -S raylib -B raylib/build -DCMAKE_BUILD_TYPE=Release
sudo cmake --build raylib/build --target install
```

### Windows-erako exekutagarria

Windows-erako `.exe`-a Linuxetik sortzen da, `mingw-w64` *cross-compiler*-arekin.
Horretarako, raylib-en **`win64_mingw-w64`** bertsioa deskargatu
([raylib releases](https://github.com/raysan5/raylib/releases)) eta `lib/raylib-win/`
karpetan atera behar da, egitura hau geratzeko:

```
lib/raylib-win/include/raylib.h
lib/raylib-win/lib/libraylib.a
```

---

## Konpilazioa

| Komandoa | Emaitza |
|---|---|
| `make` | Linuxerako *debug* bertsioa → `build/game` |
| `make run` | Konpilatu eta exekutatu |
| `make win` | Windows 64 bit-erako `.exe` → `build/game.exe` |
| `make RELEASE=1 win` | Bertsio optimizatua, kontsola-leihorik gabe |
| `make clean` | `build/` karpeta ezabatu |

**Entregatzeko bertsioa:**

```sh
make RELEASE=1 win
```

Sortzen den `build/game.exe` fitxategi bakarra da eta estatikoki lotuta dago
(*statically linked*): ez du DLLrik ez beste menpekotasunik behar, nahikoa da bi klik
egitea edozein Windows 64 bit-eko ordenagailutan.

---

## Proiektuaren egitura

```
.
├── Makefile
├── README.md
├── src/              # iturburu-kodea (.c)
├── include/          # goiburuak (.h)
├── assets/           # testurak, mapak, testuak
├── lib/raylib-win/   # raylib, Windows-erako cross-compilaziorako
└── build/            # sortutako fitxategiak (ez dago bertsio-kontrolpean)
```

---

## Kontrolak

| Tekla | Ekintza |
|---|---|
| `W` `A` `S` `D` / geziak | Mugitu |
| `E` | Elkarrizketa / interakzioa |
| `Espazioa` | Erasoa |
| `I` | Inbentarioa |
| `Esc` | Menua / irten |

---

## Egilea

**Danel (LenadESP)** — Programazio ikasgaia, 2026/2027 ikasturtea.