#!/bin/bash

# Skript für CMake + Ninja/Make unter Linux mit optionalem Clean und CMake-Preset

set -e

BUILD_DIR="build"
CLEAN_BUILD=false
PRESET=""

usage() {
    echo "Usage: $0 [--clean] [--preset <name>]"
    echo ""
    echo "  --clean            Löscht das Build-Verzeichnis vor dem Bauen"
    echo "  --preset <name>    Nutzt ein CMake-Preset aus CMakePresets.json"
    echo "                     (z. B. linux-debian mit LUCID-Unterstützung,"
    echo "                     linux-rpi5 ohne LUCID-Unterstützung)."
    echo "                     Ohne --preset wird der klassische Ablauf"
    echo "                     (cmake + make im Verzeichnis '$BUILD_DIR') verwendet."
}

while [ $# -gt 0 ]; do
    case "$1" in
        --clean)
            CLEAN_BUILD=true
            ;;
        --preset)
            shift
            PRESET="$1"
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unbekannte Option: $1"
            usage
            exit 1
            ;;
    esac
    shift
done

ARCH="$(uname -m)"
echo "Erkannte Architektur: $ARCH"

if [ -n "$PRESET" ]; then
    echo "Verwende CMake-Preset: $PRESET"

    if [ "$CLEAN_BUILD" = true ]; then
        echo "Lösche Build-Verzeichnis für Clean-Build..."
        rm -rf "out/build/$PRESET"
    fi

    echo "Starte CMake-Konfiguration mit Preset '$PRESET'..."
    cmake --preset "$PRESET"

    echo "Starte Build..."
    cmake --build --preset "$PRESET" -j"$(nproc)"

    BIN_DIR="out/build/$PRESET"
    if [ ! -f "$BIN_DIR/Settings.json" ] && [ -f "Settings.json" ]; then
        cp Settings.json "$BIN_DIR/Settings.json"
    fi

    exit 0
fi

# Klassischer Ablauf ohne Preset (Standard-Generator, i. d. R. Makefiles)

# Optional: Build-Verzeichnis löschen
if [ "$CLEAN_BUILD" = true ]; then
    echo "Lösche Build-Verzeichnis für Clean-Build..."
    rm -rf "$BUILD_DIR"
fi

# Build-Verzeichnis erstellen
if [ ! -d "$BUILD_DIR" ]; then
    mkdir "$BUILD_DIR"
    cp Settings.json "$BUILD_DIR/Settings.json"
fi

cd "$BUILD_DIR"

# CMake konfigurieren
echo "Starte CMake..."
cmake ..

# Kompilieren
echo "Starte Make..."
make -j"$(nproc)"
