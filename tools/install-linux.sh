#!/bin/bash

# Installation Helper für MarkerPositioning auf Debian 13 / Raspberry Pi 5
# Dieses Script installiert alle Abhängigkeiten automatisch

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

step() {
    echo -e "${BLUE}==>${NC} $1"
}

success() {
    echo -e "${GREEN}✓${NC} $1"
}

error() {
    echo -e "${RED}✗${NC} $1" >&2
}

warn() {
    echo -e "${YELLOW}⚠${NC} $1"
}

# Architektur erkennen
ARCH=$(uname -m)
if [ "$ARCH" != "x86_64" ] && [ "$ARCH" != "aarch64" ]; then
    error "Nicht unterstützte Architektur: $ARCH (erwartet x86_64 oder aarch64)"
    exit 1
fi

echo ""
echo "========================================"
echo "MarkerPositioning Linux Install"
echo "========================================"
echo "Architektur: $ARCH"
echo ""

# Schritt 1: System-Pakete
step "System-Pakete aktualisieren und installieren..."
sudo apt update
sudo apt install -y \
    build-essential cmake ninja-build git pkg-config \
    libgtk-3-dev \
    libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
    gstreamer1.0-plugins-good gstreamer1.0-plugins-bad \
    gstreamer1.0-plugins-ugly gstreamer1.0-libav \
    v4l-utils libv4l-dev libsqlite3-dev \
    libjpeg-dev libpng-dev libtiff-dev

success "System-Pakete installiert"
echo ""

# Schritt 2: OpenCV
step "OpenCV 4.12.0 installieren (ca. 30-45 min)..."

if [ -d "opencv" ]; then
    warn "opencv-Verzeichnis existiert bereits, überspringe Klonen"
else
    git clone --branch 4.12.0 --depth 1 https://github.com/opencv/opencv.git
fi

cd opencv

if [ -d "build" ]; then
    warn "OpenCV build-Verzeichnis existiert bereits, starte Rebuild..."
    rm -rf build
fi

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

ninja -j"$(nproc)"
sudo ninja install
sudo ldconfig

cd ../..
success "OpenCV 4.12.0 installiert"
echo ""

# Schritt 3: AprilTag
step "AprilTag installieren..."

if [ -d "apriltag" ]; then
    warn "apriltag-Verzeichnis existiert bereits, überspringe Klonen"
else
    git clone https://github.com/AprilRobotics/apriltag.git
fi

cd apriltag

if [ -d "build" ]; then
    warn "AprilTag build-Verzeichnis existiert bereits, starte Rebuild..."
    rm -rf build
fi

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
sudo cmake --install build
sudo ldconfig

cd ..
success "AprilTag installiert"
echo ""

# Schritt 4: Arena SDK (nur für x86_64 / Debian)
if [ "$ARCH" = "x86_64" ]; then
    step "Arena SDK konfigurieren (LUCID GigE)..."
    
    # Suche nach ArenaSDK_Linux_x64
    LUCID_FOUND=false
    if [ -d "/opt/ArenaSDK_Linux_x64" ]; then
        export LUCID_DEV_ROOT="/opt/ArenaSDK_Linux_x64"
        export LUCID_GENICAM_PATH="/opt/ArenaSDK_Linux_x64/GenICam"
        LUCID_FOUND=true
    elif [ -d "$HOME/ArenaSDK_Linux_x64" ]; then
        export LUCID_DEV_ROOT="$HOME/ArenaSDK_Linux_x64"
        export LUCID_GENICAM_PATH="$HOME/ArenaSDK_Linux_x64/GenICam"
        LUCID_FOUND=true
    fi
    
    if [ "$LUCID_FOUND" = true ]; then
        success "Arena SDK unter $LUCID_DEV_ROOT gefunden"
        # Speichere in Profil
        echo "export LUCID_DEV_ROOT=$LUCID_DEV_ROOT" >> ~/.bashrc
        echo "export LUCID_GENICAM_PATH=$LUCID_GENICAM_PATH" >> ~/.bashrc
        success "LUCID-Umgebungsvariablen zu ~/.bashrc hinzugefügt"
    else
        warn "Arena SDK nicht gefunden - Build wird ohne LUCID-Unterstützung durchgeführt"
        warn "Zum Installieren: Herunterladen von https://thinklucid.com/downloads/"
        warn "Dann: export LUCID_DEV_ROOT=/path/to/ArenaSDK_Linux_x64"
    fi
else
    success "Raspberry Pi 5 erkannt - LUCID Arena SDK nicht erforderlich"
fi
echo ""

# Schritt 5: MarkerPositioning Repository
step "MarkerPositioning vorbereiten..."

if [ ! -d "MarkerPositioning/.git" ]; then
    error "Befinde mich nicht im Repository-Verzeichnis"
    echo "Bitte führe dieses Script aus der Projekt-Root aus:"
    echo "  cd ~/MarkerPositioning/MarkerPositioning"
    echo "  ../tools/install-linux.sh"
    exit 1
fi

# Git Submodules initialisieren
if [ ! -f "external/spdlog/CMakeLists.txt" ]; then
    step "Initialisiere Git Submodules..."
    git submodule update --init --recursive
    success "Git Submodules initialisiert"
fi
echo ""

# Schritt 6: Build
step "MarkerPositioning bauen..."

if [ "$ARCH" = "aarch64" ]; then
    PRESET="linux-rpi5"
    echo "Verwende Preset: $PRESET (Raspberry Pi 5, ohne LUCID)"
else
    PRESET="linux-debian"
    echo "Verwende Preset: $PRESET (Debian x86_64, mit LUCID)"
fi

# Quelle die aktualisierten Umgebungsvariablen
if [ -f ~/.bashrc ]; then
    source ~/.bashrc
fi

./build.sh --preset "$PRESET"

if [ -f "out/build/$PRESET/MarkerPositioning" ]; then
    success "Binary erfolgreich erstellt: out/build/$PRESET/MarkerPositioning"
else
    error "Binary nicht gefunden nach Build!"
    exit 1
fi
echo ""

# Schritt 7: Verifikation
step "Teste Anwendung..."
if ./out/build/$PRESET/MarkerPositioning --version 2>/dev/null || ./out/build/$PRESET/MarkerPositioning --help 2>/dev/null; then
    success "Anwendung startet erfolgreich!"
else
    warn "Konnte Anwendung nicht testen (--version/--help nicht unterstützt)"
fi
echo ""

echo "========================================"
echo -e "${GREEN}✓ Installation abgeschlossen!${NC}"
echo "========================================"
echo ""
echo "Starte die Anwendung mit:"
echo "  ./out/build/$PRESET/MarkerPositioning"
echo ""
echo "Konfiguriere die Kamera in:"
echo "  out/build/$PRESET/Settings.json"
echo ""
echo "Weitere Informationen:"
echo "  - Konfiguration: siehe SETTINGS.md"
echo "  - Troubleshooting: siehe docs/linux-setup.md"
echo "  - Quick Start: siehe docs/QUICKSTART.md"
echo ""
