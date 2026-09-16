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

# Auto-navigate to correct directory (only if we're in parent)
step "Verifying project directory..."

if [ -f "CMakeLists.txt" ]; then
    # Wir sind schon im richtigen directory
    success "Project found in: $(pwd)"
elif [ -d "MarkerPositioning" ] && [ -f "MarkerPositioning/CMakeLists.txt" ]; then
    # Wir sind im parent directory, wechsle zum project directory
    cd MarkerPositioning
    success "Navigated to project directory: $(pwd)"
else
    echo -e "${RED}✗${NC} Project directory not found!"
    echo "Expected to find CMakeLists.txt in current or MarkerPositioning/ subdirectory"
    exit 1
fi

echo ""

# SCHRITT 1: Git Submodules initialisieren
step "Initializing Git Submodules..."

if [ ! -f "external/spdlog/CMakeLists.txt" ]; then
    git submodule update --init --recursive
    success "Git Submodules initialized"
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
        step "Searching for Arena SDK installation..."
        
        # Versuche automatisch LUCID_DEV_ROOT zu finden
        if [ -d "/opt/ArenaSDK_Linux_x64" ]; then
            lucid_root="/opt/ArenaSDK_Linux_x64"
            success "Found Arena SDK at: $lucid_root"
        elif [ -d "/opt/ArenaSDK" ]; then
            lucid_root="/opt/ArenaSDK"
            success "Found Arena SDK at: $lucid_root"
        else
            # Suche nach libarena.so
            echo "Searching for libarena.so..."
            lucid_lib=$(find ~ -name 'libarena.so' 2>/dev/null | head -1)
            
            if [ -n "$lucid_lib" ]; then
                lucid_root=$(dirname $(dirname "$lucid_lib"))
                success "Found Arena SDK at: $lucid_root"
            else
                echo -e "${YELLOW}Could not auto-detect Arena SDK location${NC}"
                read -p "Enter LUCID_DEV_ROOT manually (e.g. /opt/ArenaSDK_Linux_x64): " lucid_root
            fi
        fi
        
        if [ ! -d "$lucid_root" ]; then
            echo -e "${RED}✗${NC} Directory not found: $lucid_root"
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
