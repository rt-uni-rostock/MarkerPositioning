# Quick Start: MarkerPositioning auf Debian 13 / Raspberry Pi 5

## ⚡ Ultra-Schnell (mit Automation)

```bash
cd ~/MarkerPositioning/MarkerPositioning

# 1. Überprüfe, ob alle Abhängigkeiten vorhanden sind:
tools/diagnose-linux.sh

# 2. Installiere ALLES automatisch (OpenCV, AprilTag, Build):
tools/install-linux.sh

# 3. Starte die Anwendung:
./out/build/linux-debian/MarkerPositioning        # für Debian x86_64
# oder
./out/build/linux-rpi5/MarkerPositioning          # für Raspberry Pi 5
```

## 📋 Manueller TL;DR für Debian 13 (x86_64, mit LUCID)

```bash
# 1. Abhängigkeiten
sudo apt update && sudo apt install -y build-essential cmake ninja-build git \
    pkg-config libgtk-3-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
    gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly \
    gstreamer1.0-libav v4l-utils libv4l-dev libsqlite3-dev \
    libjpeg-dev libpng-dev libtiff-dev

# 2. OpenCV (ca. 30-45 min)
git clone --branch 4.12.0 --depth 1 https://github.com/opencv/opencv.git
cd opencv && mkdir build && cd build
cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DWITH_GSTREAMER=ON -DWITH_V4L=ON -DBUILD_EXAMPLES=OFF -DBUILD_TESTS=OFF -DBUILD_PERF_TESTS=OFF ..
ninja && sudo ninja install && sudo ldconfig
cd ../..

# 3. AprilTag
git clone https://github.com/AprilRobotics/apriltag.git
cd apriltag && cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)" && sudo cmake --install build && sudo ldconfig
cd ..

# 4. Arena SDK (optional, für LUCID GigE)
export LUCID_DEV_ROOT=/opt/ArenaSDK_Linux_x64
export LUCID_GENICAM_PATH=/opt/ArenaSDK_Linux_x64/GenICam

# 5. MarkerPositioning
git clone https://github.com/rt-uni-rostock/MarkerPositioning.git
cd MarkerPositioning/MarkerPositioning
git submodule update --init --recursive
./build.sh --preset linux-debian

# 6. Laufen lassen
./out/build/linux-debian/MarkerPositioning
```

## 📋 Manueller TL;DR für Raspberry Pi 5 (aarch64, ohne LUCID)

Schritte 1-3 wie Debian oben, dann:

```bash
# 5. MarkerPositioning (ohne LUCID)
git clone https://github.com/rt-uni-rostock/MarkerPositioning.git
cd MarkerPositioning/MarkerPositioning
git submodule update --init --recursive
./build.sh --preset linux-rpi5

# 6. Laufen lassen
./out/build/linux-rpi5/MarkerPositioning
```

## 🐛 Erste Fehler?

Siehe `docs/linux-setup.md` Kapitel 7 **Troubleshooting**.

Oder führe aus:
```bash
tools/diagnose-linux.sh
```

## ⚙️ Kamera konfigurieren

Bearbeite `out/build/<preset>/Settings.json`:
- `supervisorMode`: `0` (Erkennung), `2` (Bild-Passthrough)
- `streamType`: `2` (RTP), `3` (RTSP), `4` (USB-Webcam), `1` (LUCID nur auf Debian!)
- `udpIp`/`udpPort`: Ziel-Empfänger

Details: siehe `SETTINGS.md` im Repository.
