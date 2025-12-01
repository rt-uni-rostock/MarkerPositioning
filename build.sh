#!/bin/bash

# Skript für CMake + Make unter Linux mit optionalem Clean

BUILD_DIR="build"
CLEAN_BUILD=false

# Prüfen, ob "--clean" als Parameter übergeben wurde
for arg in "$@"
do
    if [ "$arg" == "--clean" ]; then
        CLEAN_BUILD=true
    fi
done

# Optional: Build-Verzeichnis löschen
if [ "$CLEAN_BUILD" = true ]; then
    echo "Lösche Build-Verzeichnis für Clean-Build..."
    rm -rf "$BUILD_DIR"
fi

# Build-Verzeichnis erstellen
if [ ! -d "$BUILD_DIR" ]; then
    mkdir "$BUILD_DIR"
    cp Settings.json build/Settings.json
fi

cd "$BUILD_DIR" || exit

# CMake konfigurieren
echo "Starte CMake..."
cmake ..

# Kompilieren
echo "Starte Make..."
make -j$(nproc)
