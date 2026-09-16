# Linux-Setup: Debian x86_64 und Raspberry Pi 5 (aarch64)

Dieses Dokument beschreibt, wie die Build-Abhängigkeiten von MarkerPositioning auf
einem Debian-basierten Linux-System eingerichtet werden. Es deckt zwei
Zielsysteme mit unterschiedlichem Funktionsumfang ab:

| Zielsystem | Architektur | Kameras | LUCID/Arena SDK nötig? |
|---|---|---|---|
| Debian-Server/PC | x86_64 | RTSP, RTP, USB-Webcam, LUCID GigE | Ja |
| Raspberry Pi 5 | aarch64 (ARM64) | RTSP, RTP, USB-Webcam | Nein |

Die Auswahl erfolgt über die CMake-Option `ENABLE_LUCID` (siehe
`CMakeLists.txt` und `CMakePresets.json`, Presets `linux-debian` /
`linux-rpi5`).

## 1. Grundpakete (beide Zielsysteme)

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build git pkg-config \
    libgtk-3-dev \
    libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
    gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly \
    gstreamer1.0-libav gstreamer1.0-tools \
    v4l-utils libv4l-dev \
    libsqlite3-dev
```

Hinweise:
- `libgstreamer*` + `gstreamer1.0-plugins-*` werden für den RTP-Stream
  (`RTPStream`, GStreamer-Pipelines über OpenCV) benötigt und müssen auch beim
  OpenCV-Build aktiviert sein (`WITH_GSTREAMER=ON`, siehe unten).
- `libv4l-dev`/`v4l-utils` werden für USB-Webcams (`cv::CAP_V4L2`) benötigt.
- `libsqlite3-dev` ist eigentlich nicht zwingend nötig, da `external/sqlite`
  bereits als Quellcode im Repository vorliegt und mitgebaut wird — schadet
  aber nicht als Absicherung, falls Systemwerkzeuge sqlite3 nutzen wollen.

## 2. OpenCV aus dem Quellcode bauen

Die Debian-Repository-Pakete (`libopencv-dev`) sind i. d. R. zu alt und/oder
ohne GStreamer-/V4L2-Unterstützung gebaut. Empfehlung: OpenCV selbst bauen.

```bash
sudo apt install -y libjpeg-dev libpng-dev libtiff-dev

git clone --branch 4.12.0 --depth 1 https://github.com/opencv/opencv.git
cd opencv
mkdir build && cd build

cmake -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DWITH_GSTREAMER=ON \
    -DWITH_V4L=ON \
    -DBUILD_EXAMPLES=OFF \
    -DBUILD_TESTS=OFF \
    -DBUILD_PERF_TESTS=OFF \
    ..

ninja
sudo ninja install
sudo ldconfig
```

Nach der Installation kann `find_package(OpenCV REQUIRED)` in
`CMakeLists.txt` die Installation automatisch unter
`/usr/local/lib/cmake/opencv4` finden. Falls ein anderer Installationspfad
verwendet wird, kann er beim Konfigurieren explizit übergeben werden:

```bash
cmake -DOpenCV_DIR=/pfad/zu/opencv/lib/cmake/opencv4 ..
```

Auf dem Raspberry Pi 5 kann der native Build von OpenCV mehrere Stunden
dauern; ausreichend Swap-Speicher (z. B. 2 GB) einplanen, um Out-of-Memory-
Abbrüche beim Parallel-Build zu vermeiden (`ninja -j2` statt `-j$(nproc)`,
falls RAM knapp ist).

## 3. AprilTag-Bibliothek bauen

```bash
git clone https://github.com/AprilRobotics/apriltag.git
cd apriltag
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
sudo cmake --install build
sudo ldconfig
cd ..
```

Das installiert `apriltag.h` nach `/usr/local/include/apriltag/` und die
Bibliothek nach `/usr/local/lib/`, passend zum bestehenden
`find_path`/`find_library`-Zweig in `CMakeLists.txt`. Der `sudo`-Aufruf ist
nötig, weil das Schreiben nach `/usr/local` Administratorrechte erfordert.

## 4. LUCID Arena SDK (nur für Debian-Zielsystem mit `ENABLE_LUCID=ON`)

Auf dem Raspberry Pi 5 kann dieser Schritt übersprungen werden
(`ENABLE_LUCID=OFF`, siehe Preset `linux-rpi5`).

1. Arena SDK für Linux x86_64 von LUCID Vision Labs herunterladen
   (Registrierung bei https://thinklucid.com erforderlich).
2. Installationsskript gemäß LUCID-Dokumentation ausführen (installiert i. d. R.
   nach `/opt/ArenaSDK_Linux_x64` oder ähnlichem Pfad und trägt den
   Bibliothekspfad in `/etc/ld.so.conf.d/Arena_SDK.conf` ein).
3. Umgebungsvariablen setzen, bevor `cmake`/`build.sh` aufgerufen wird
   (z. B. in `~/.bashrc` oder einem Setup-Skript):

   ```bash
   export LUCID_DEV_ROOT=/opt/ArenaSDK_Linux_x64
   export LUCID_GENICAM_PATH=/opt/ArenaSDK_Linux_x64/GenICam
   ```

   Die genauen Pfade hängen von der tatsächlichen SDK-Installation ab —
   `LUCID_DEV_ROOT` muss auf das Wurzelverzeichnis der Arena SDK zeigen
   (enthält u. a. `lib64/libarena.so`), `LUCID_GENICAM_PATH` auf das
   `GenICam`-Unterverzeichnis darin (enthält
   `library/lib/Linux64_x64/libGCBase_*.so`).
4. `sudo ldconfig` erneut ausführen, damit die Arena-SDK-Bibliotheken zur
   Laufzeit gefunden werden.

`CMakeLists.txt` bricht mit einer klaren Fehlermeldung ab
(`FATAL_ERROR`), falls `ENABLE_LUCID=ON` gesetzt ist, aber Arena-SDK-
Bibliotheken nicht gefunden werden.

## 5. Projekt bauen

```bash
# Repository clonen
git clone https://github.com/rt-uni-rostock/MarkerPositioning.git
cd MarkerPositioning

# Git-Submodule initialisieren (wichtig für spdlog, etc.)
git submodule update --init --recursive

# In das Projektverzeichnis wechseln
cd MarkerPositioning

# Build mit dem gewünschten Preset
# Debian, mit LUCID-Unterstützung
./build.sh --preset linux-debian

# ODER für Raspberry Pi 5, ohne LUCID-Unterstützung
# ./build.sh --preset linux-rpi5
```

Das Binary liegt danach unter `out/build/<preset>/MarkerPositioning`,
`Settings.json` wird automatisch dorthin kopiert (sofern nicht bereits
vorhanden).

## 7. Troubleshooting: Häufige Build-Fehler

### CMake Error: "does not contain a CMakeLists.txt file" (spdlog)
**Ursache:** Git-Submodule nicht initialisiert.
**Lösung:**
```bash
cd ~/MarkerPositioning
git submodule update --init --recursive
cd MarkerPositioning
./build.sh --preset linux-debian
```

### CMake Error: "Could not find AprilTag library"
**Ursache:** AprilTag nicht installiert oder nicht unter `/usr/local/lib` gefunden.
**Lösung:**
```bash
# Prüfe ob apriltag.h existiert:
ls -la /usr/local/include/apriltag/apriltag.h

# Prüfe ob libapriltag.a existiert:
ls -la /usr/local/lib/libapriltag.a

# Falls nicht: baue und installiere AprilTag erneut:
git clone https://github.com/AprilRobotics/apriltag.git
cd apriltag
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
sudo cmake --install build
sudo ldconfig
```

### CMake Error: "Could not find OpenCV"
**Ursache:** OpenCV nicht installiert oder nicht unter `/usr/local` gefunden.
**Lösung:**
```bash
# Prüfe ob opencv4 Config existiert:
ls -la /usr/local/lib/cmake/opencv4/OpenCVConfig.cmake

# Falls nicht: baue OpenCV erneut:
git clone --branch 4.12.0 --depth 1 https://github.com/opencv/opencv.git
cd opencv && mkdir build && cd build
cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DWITH_GSTREAMER=ON -DWITH_V4L=ON -DBUILD_EXAMPLES=OFF -DBUILD_TESTS=OFF \
    -DBUILD_PERF_TESTS=OFF ..
ninja
sudo ninja install
sudo ldconfig
```

### CMake Error: "ENABLE_LUCID=ON but the LUCID Arena SDK could not be found"
**Ursache:** Arena SDK nicht installiert oder Umgebungsvariablen nicht gesetzt.
**Lösungen:**
1. **Mit LUCID bauen:** Umgebungsvariablen vor `./build.sh` setzen:
   ```bash
   export LUCID_DEV_ROOT=/opt/ArenaSDK_Linux_x64
   export LUCID_GENICAM_PATH=/opt/ArenaSDK_Linux_x64/GenICam
   ./build.sh --preset linux-debian
   ```
2. **Ohne LUCID bauen:** (z. B. für Raspberry Pi 5)
   ```bash
   ./build.sh --preset linux-rpi5
   ```

## 8. Optional: Autostart auf dem Raspberry Pi 5 via systemd

Eine Beispiel-Unit liegt unter `tools/marker-positioning.service`. Nach
Anpassung von `WorkingDirectory`/`ExecStart` auf das tatsächliche
Build-Ausgabeverzeichnis (z. B. `out/build/linux-rpi5`) installieren mit:

```bash
sudo cp tools/marker-positioning.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now marker-positioning.service
journalctl -u marker-positioning.service -f
```

