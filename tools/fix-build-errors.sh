#!/bin/bash

# Fix für MarkerPositioning Build-Fehler auf Debian 13
# Dieser Script behebt die zwei Fehler automatisch

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

step() {
    echo -e "${BLUE}==>${NC} $1"
}

success() {
    echo -e "${GREEN}✓${NC} $1"
}

echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║  Fix: MarkerPositioning Build Errors on Debian 13         ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# SCHRITT 1: Git Submodules initialisieren
step "Initializing Git Submodules..."

if [ ! -f "external/spdlog/CMakeLists.txt" ]; then
    if [ -d ".git" ]; then
        # Befinde mich im Repository
        git submodule update --init --recursive
        success "Git Submodules initialized"
    else
        # Befinde mich vielleicht im übergeordneten Verzeichnis
        cd MarkerPositioning
        git submodule update --init --recursive
        cd ..
        success "Git Submodules initialized (from parent dir)"
    fi
else
    success "Git Submodules already initialized"
fi

echo ""

# SCHRITT 2: Build-Optionen
step "Choosing build variant..."

echo ""
echo "Hat dein Testrechner LUCID Arena SDK installiert?"
echo "  1) Nein, ohne LUCID bauen (EMPFOHLEN)" 
echo "  2) Ja, mit LUCID bauen (LUCID_DEV_ROOT muss gesetzt sein)"
echo ""
read -p "Wähle (1 oder 2): " choice

case $choice in
    1)
        echo ""
        step "Building WITHOUT LUCID (linux-rpi5 preset)..."
        ./build.sh --preset linux-rpi5
        BUILD_DIR="out/build/linux-rpi5"
        ;;
    2)
        echo ""
        read -p "LUCID_DEV_ROOT eingeben (z.B. /opt/ArenaSDK_Linux_x64): " lucid_root
        
        if [ ! -d "$lucid_root" ]; then
            echo -e "${RED}✗${NC} Verzeichnis nicht gefunden: $lucid_root"
            exit 1
        fi
        
        export LUCID_DEV_ROOT="$lucid_root"
        export LUCID_GENICAM_PATH="$lucid_root/GenICam"
        
        echo ""
        step "Building WITH LUCID (linux-debian preset)..."
        ./build.sh --preset linux-debian
        BUILD_DIR="out/build/linux-debian"
        ;;
    *)
        echo -e "${RED}Ungültige Wahl${NC}"
        exit 1
        ;;
esac

echo ""

# SCHRITT 3: Verifizierung
step "Verifying build..."
if [ -f "$BUILD_DIR/MarkerPositioning" ]; then
    success "Binary erfolgreich erstellt: $BUILD_DIR/MarkerPositioning"
    echo ""
    
    step "Running verify script..."
    ./tools/verify-build.sh
    
    echo ""
    echo "╔════════════════════════════════════════════════════════════╗"
    echo "║                   ✅ BUILD ERFOLGREICH                    ║"
    echo "╚════════════════════════════════════════════════════════════╝"
    echo ""
    echo "Starte die Anwendung mit:"
    echo "  ./$BUILD_DIR/MarkerPositioning"
    echo ""
else
    echo -e "${RED}✗ Binary nicht gefunden!${NC}"
    exit 1
fi
