#!/bin/bash

# Verifikations-Script nach erfolgreichem Build
# Überprüft, ob die Binärdatei funktionsfähig ist

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

check_ok() {
    echo -e "${GREEN}✓${NC} $1"
}

check_fail() {
    echo -e "${RED}✗${NC} $1"
}

check_info() {
    echo -e "${BLUE}ℹ${NC} $1"
}

ARCH=$(uname -m)
if [ "$ARCH" = "aarch64" ]; then
    PRESET="linux-rpi5"
else
    PRESET="linux-debian"
fi

BINARY="out/build/$PRESET/MarkerPositioning"
SETTINGS="out/build/$PRESET/Settings.json"

echo "========================================"
echo "MarkerPositioning Verifikation"
echo "========================================"
echo "Preset: $PRESET"
echo "Binary: $BINARY"
echo "Settings: $SETTINGS"
echo ""

# 1. Überprüfe Binärdatei
echo "1. Binärdatei-Überprüfung:"
if [ -f "$BINARY" ]; then
    check_ok "Binärdatei existiert: $BINARY"
    
    # Überprüfe ob ausführbar
    if [ -x "$BINARY" ]; then
        check_ok "Binärdatei ist ausführbar"
    else
        check_fail "Binärdatei ist nicht ausführbar"
        chmod +x "$BINARY"
        check_info "Ausführbar-Flag gesetzt"
    fi
    
    # Überprüfe Dateigröße
    SIZE=$(stat -f%z "$BINARY" 2>/dev/null || stat -c%s "$BINARY" 2>/dev/null || echo "unknown")
    check_info "Dateigröße: $SIZE bytes"
else
    check_fail "Binärdatei nicht gefunden!"
    exit 1
fi
echo ""

# 2. Überprüfe Abhängigkeiten
echo "2. Abhängigkeits-Überprüfung:"

if command -v ldd &> /dev/null; then
    echo "Abhängigkeiten:"
    ldd "$BINARY" | grep -E "libopencv|libc|libstdc" || true
    echo ""
fi

# 3. Überprüfe Settings.json
echo "3. Konfigurationsdatei:"
if [ -f "$SETTINGS" ]; then
    check_ok "Settings.json existiert"
    
    # Validiere JSON
    if command -v jq &> /dev/null; then
        if jq empty "$SETTINGS" 2>/dev/null; then
            check_ok "Settings.json ist gültiges JSON"
        else
            check_fail "Settings.json ist kein gültiges JSON"
        fi
    else
        check_info "jq nicht installiert - JSON-Validierung übersprungen"
    fi
else
    check_fail "Settings.json nicht gefunden"
fi
echo ""

# 4. Test der Anwendung
echo "4. Anwendungs-Test:"

# Versuche kurzzeitig zu starten und zu prüfen, ob keine Crashes auftreten
TIMEOUT=2
TEMP_LOG=$(mktemp)

timeout $TIMEOUT "$BINARY" > "$TEMP_LOG" 2>&1 || true

# Prüfe auf Fehler in der Ausgabe
if grep -q "FATAL\|ERROR\|Segmentation" "$TEMP_LOG"; then
    check_fail "Anwendung startete mit Fehlern:"
    grep -E "FATAL|ERROR|Segmentation" "$TEMP_LOG"
else
    check_ok "Anwendung startet ohne Crash (Timeout nach ${TIMEOUT}s)"
    
    # Überprüfe auf LUCID-Status
    if grep -q "LUCID GigE camera support" "$TEMP_LOG"; then
        if grep -q "ENABLED" "$TEMP_LOG"; then
            check_ok "LUCID GigE-Unterstützung: ENABLED"
        elif grep -q "DISABLED" "$TEMP_LOG"; then
            check_ok "LUCID GigE-Unterstützung: DISABLED"
        fi
    fi
fi

rm -f "$TEMP_LOG"
echo ""

# 5. Zusammenfassung
echo "========================================"
echo "Verifikation abgeschlossen!"
echo "========================================"
echo ""
echo "Starte die Anwendung mit:"
echo "  ./$BINARY"
echo ""
echo "Konfiguriere die Kamera in:"
echo "  $SETTINGS"
echo ""
echo "Weitere Informationen:"
echo "  - docs/QUICKSTART.md"
echo "  - docs/linux-setup.md"
echo ""
