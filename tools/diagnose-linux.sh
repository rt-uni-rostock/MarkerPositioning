#!/bin/bash

# Diagnose-Script für MarkerPositioning Linux-Setup
# Überprüft, ob alle Abhängigkeiten und Konfigurationen vorhanden sind

set +e  # Fehler nicht abbrechen, sondern nur protokollieren

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

ERRORS=0
WARNINGS=0

check_ok() {
    echo -e "${GREEN}✓${NC} $1"
}

check_fail() {
    echo -e "${RED}✗${NC} $1"
    ERRORS=$((ERRORS+1))
}

check_warn() {
    echo -e "${YELLOW}⚠${NC} $1"
    WARNINGS=$((WARNINGS+1))
}

echo "========================================"
echo "MarkerPositioning Linux Diagnose"
echo "========================================"
echo ""

# 1. Architektur
echo "1. System-Architektur:"
ARCH=$(uname -m)
echo "   Architektur: $ARCH"
if [ "$ARCH" = "x86_64" ]; then
    check_ok "x86_64 (Debian) erkannt"
elif [ "$ARCH" = "aarch64" ]; then
    check_ok "aarch64 (Raspberry Pi 5) erkannt"
else
    check_warn "Unbekannte Architektur: $ARCH (erwartet x86_64 oder aarch64)"
fi
echo ""

# 2. Build-Tools
echo "2. Build-Tools:"
if command -v cmake &> /dev/null; then
    CMAKE_VER=$(cmake --version | head -n1)
    check_ok "CMake: $CMAKE_VER"
else
    check_fail "CMake nicht installiert"
fi

if command -v ninja &> /dev/null; then
    NINJA_VER=$(ninja --version)
    check_ok "Ninja: $NINJA_VER"
else
    check_warn "Ninja nicht installiert (baue mit Make, falls installiert)"
fi

if command -v gcc &> /dev/null; then
    GCC_VER=$(gcc --version | head -n1)
    check_ok "GCC: $GCC_VER"
else
    check_fail "GCC nicht installiert"
fi

if command -v g++ &> /dev/null; then
    GXX_VER=$(g++ --version | head -n1)
    check_ok "G++: $GXX_VER"
else
    check_fail "G++ nicht installiert"
fi
echo ""

# 3. OpenCV
echo "3. OpenCV:"
if pkg-config --exists opencv4; then
    OPENCV_VER=$(pkg-config --modversion opencv4)
    OPENCV_PREFIX=$(pkg-config --variable=prefix opencv4)
    check_ok "OpenCV $OPENCV_VER unter $OPENCV_PREFIX"
    
    # Prüfe GStreamer-Support
    if pkg-config --exists gstreamer-1.0; then
        check_ok "GStreamer-1.0 Entwickler-Header vorhanden"
    else
        check_warn "GStreamer-1.0 Entwickler-Header nicht gefunden (RTP-Stream könnte nicht funktionieren)"
    fi
    
    # Prüfe V4L2-Support
    if pkg-config --exists libv4l2; then
        check_ok "libv4l2 Entwickler-Header vorhanden"
    else
        check_warn "libv4l2 Entwickler-Header nicht gefunden (USB-Webcam könnte nicht funktionieren)"
    fi
else
    check_fail "OpenCV nicht gefunden (prüfe pkg-config oder CMakePresets)"
fi
echo ""

# 4. AprilTag
echo "4. AprilTag:"
if [ -f "/usr/local/include/apriltag/apriltag.h" ]; then
    check_ok "AprilTag Header (/usr/local/include/apriltag/apriltag.h) gefunden"
else
    check_fail "AprilTag Header nicht gefunden"
fi

if [ -f "/usr/local/lib/libapriltag.a" ]; then
    check_ok "AprilTag Bibliothek (/usr/local/lib/libapriltag.a) gefunden"
else
    check_fail "AprilTag Bibliothek nicht gefunden"
fi
echo ""

# 5. Arena SDK / LUCID
echo "5. LUCID Arena SDK (optional, nur für Debian x86_64 erforderlich):"
if [ "$ARCH" = "aarch64" ]; then
    check_ok "Raspberry Pi 5 erkannt - LUCID Arena SDK nicht erforderlich"
else
    if [ -z "$LUCID_DEV_ROOT" ]; then
        check_warn "LUCID_DEV_ROOT nicht gesetzt (benötigt für ENABLE_LUCID=ON)"
        echo "   Setze vor dem Build:"
        echo "   export LUCID_DEV_ROOT=/opt/ArenaSDK_Linux_x64"
        echo "   export LUCID_GENICAM_PATH=/opt/ArenaSDK_Linux_x64/GenICam"
    else
        check_ok "LUCID_DEV_ROOT = $LUCID_DEV_ROOT"
    fi
    
    if [ -z "$LUCID_GENICAM_PATH" ]; then
        check_warn "LUCID_GENICAM_PATH nicht gesetzt"
    else
        check_ok "LUCID_GENICAM_PATH = $LUCID_GENICAM_PATH"
    fi
    
    if [ -n "$LUCID_DEV_ROOT" ] && [ -d "$LUCID_DEV_ROOT" ]; then
        if [ -f "$LUCID_DEV_ROOT/lib64/libarena.so" ]; then
            check_ok "Arena SDK Bibliothek vorhanden"
        else
            check_fail "Arena SDK Bibliothek ($LUCID_DEV_ROOT/lib64/libarena.so) nicht gefunden"
        fi
    fi
fi
echo ""

# 6. Git Submodules
echo "6. Git Submodules:"
if [ -f "external/spdlog/CMakeLists.txt" ]; then
    check_ok "spdlog Submodule initialisiert"
else
    check_fail "spdlog Submodule NICHT initialisiert"
    echo "   Behebung:"
    echo "   git submodule update --init --recursive"
fi

if [ -f "external/sqlite/CMakeLists.txt" ]; then
    check_ok "sqlite Submodule initialisiert"
else
    check_fail "sqlite Submodule NICHT initialisiert"
    echo "   Behebung:"
    echo "   git submodule update --init --recursive"
fi
echo ""

# 7. Projekt-Dateien
echo "7. Projekt-Konfiguration:"
if [ -f "CMakeLists.txt" ]; then
    check_ok "CMakeLists.txt vorhanden"
else
    check_fail "CMakeLists.txt nicht gefunden"
fi

if [ -f "CMakePresets.json" ]; then
    check_ok "CMakePresets.json vorhanden"
    
    if grep -q "linux-debian" CMakePresets.json; then
        check_ok "  - linux-debian Preset definiert"
    else
        check_fail "  - linux-debian Preset nicht definiert"
    fi
    
    if grep -q "linux-rpi5" CMakePresets.json; then
        check_ok "  - linux-rpi5 Preset definiert"
    else
        check_fail "  - linux-rpi5 Preset nicht definiert"
    fi
else
    check_fail "CMakePresets.json nicht gefunden"
fi

if [ -f "build.sh" ]; then
    check_ok "build.sh vorhanden"
    if [ -x "build.sh" ]; then
        check_ok "  - build.sh ist ausführbar"
    else
        check_warn "  - build.sh ist nicht ausführbar (chmod +x build.sh)"
    fi
else
    check_fail "build.sh nicht gefunden"
fi

if [ -f "settings/Settings.json" ]; then
    check_ok "Settings.json vorhanden"
else
    check_warn "Settings.json nicht vorhanden (wird beim Build kopiert)"
fi
echo ""

# 8. Zusammenfassung
echo "========================================"
echo "Diagnose-Zusammenfassung:"
echo "========================================"
echo "Fehler: $ERRORS"
echo "Warnungen: $WARNINGS"
echo ""

if [ $ERRORS -eq 0 ]; then
    echo -e "${GREEN}✓ Alle kritischen Abhängigkeiten vorhanden!${NC}"
    echo ""
    echo "Nächste Schritte:"
    if [ "$ARCH" = "aarch64" ]; then
        echo "  ./build.sh --preset linux-rpi5"
    else
        echo "  ./build.sh --preset linux-debian"
    fi
    echo ""
    exit 0
else
    echo -e "${RED}✗ Es gibt kritische Fehler - bitte beheben vor dem Build.${NC}"
    echo ""
    exit 1
fi
