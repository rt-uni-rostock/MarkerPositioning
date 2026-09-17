# ✅ MarkerPositioning Linux Portability - FERTIGSTELLUNG

## 🎯 Status: VOLLSTÄNDIG IMPLEMENTIERT & DOKUMENTIERT

Alle notwendigen Änderungen zur Lauffähigkeit des Projekts auf Debian 13 und Raspberry Pi 5 sind abgeschlossen.

---

## 📦 What You Get

### ✅ Code-Änderungen (6 Dateien)
- CMakeLists.txt: ENABLE_LUCID Feature Toggle + Linux Arena SDK Pfade
- ImageSourceFactory.cpp: Bedingte LUCID-Kompilierung
- WebcamStream.cpp: Platform-spezifische V4L2 Backends
- MarkerPositioning.cpp: Startup-Logging für Feature-Status
- CMakePresets.json: Debian & RPi5 Presets
- build.sh: Preset-basierte Build-Automatisierung

### ✅ Dokumentation (3 Dateien)
- `docs/QUICKSTART.md`: 3-Minuten Überblick
- `docs/linux-setup.md`: Komplette 8-Kapitel Installationsanleitung
- `docs/LINUX_IMPLEMENTATION_CHECKLIST.md`: Technische Details aller Änderungen

### ✅ Automatisierungs-Tools (4 Scripts + 1 Template)
- `tools/install-linux.sh`: Vollständig automatisiertes Setup
- `tools/diagnose-linux.sh`: Abhängigkeitsprüfung
- `tools/verify-build.sh`: Binary-Verifikation nach Build
- `tools/README.md`: Dokumentation aller Scripts
- `tools/marker-positioning.service`: Systemd Autostart-Unit (Template)

---

## 🚀 Schnelleinstieg

### Option 1: Automatische Installation (empfohlen)

```bash
cd ~/MarkerPositioning/MarkerPositioning
chmod +x tools/*.sh                          # Mache Scripts ausführbar
./tools/install-linux.sh                     # Installiere ALLES (ca. 2-3 Stunden)
./tools/verify-build.sh                      # Verifiziere Binary
./out/build/linux-debian/MarkerPositioning   # Starte Anwendung
```

**Was passiert automatisch:**
- System-Pakete aktualisieren
- OpenCV 4.12.0 aus Quellcode bauen (~45 min)
- AprilTag aus Quellcode bauen (~5 min)
- Arena SDK konfigurieren (falls vorhanden)
- Git Submodules initialisieren
- MarkerPositioning bauen mit appropriatem Preset
- Logs und Fehlerbehandlung auf jedem Schritt

### Option 2: Manuelle Installation (Schritt für Schritt)

```bash
# 1. Überprüfe Abhängigkeiten
./tools/diagnose-linux.sh

# 2. Folge docs/linux-setup.md Kapitel 1-5
# (installiere OpenCV, AprilTag, etc. manuell)

# 3. Baue Projekt
./build.sh --preset linux-debian

# 4. Verifiziere
./tools/verify-build.sh
```

---

## 🖥️ System-Anforderungen

### Debian 13 (x86_64)
- CMake 3.16+
- GCC/G++ 10+
- Ninja oder Make
- ~5GB freier Speicherplatz
- LUCID Arena SDK (optional, für GigE Kameras)

### Raspberry Pi 5 (aarch64)
- Debian 12+ für ARM64
- CMake 3.16+
- GCC/G++ 10+
- ~2-3GB freier Speicherplatz
- **KEIN Arena SDK erforderlich**

---

## 📋 Verifikations-Checkliste

Nach der Installation überprüfe:

- [ ] `./tools/diagnose-linux.sh` zeigt keine kritischen Fehler
- [ ] `./tools/verify-build.sh` zeigt alle grünen ✓
- [ ] Binary existiert: `out/build/linux-debian/MarkerPositioning`
- [ ] Settings.json wurde kopiert: `out/build/linux-debian/Settings.json`
- [ ] Binary startet und zeigt Feature-Status:
  ```
  LUCID GigE camera support: ENABLED (Debian)
  # oder
  LUCID GigE camera support: DISABLED (Raspberry Pi)
  ```

---

## 🎓 Architektur-Überblick

### Feature Toggle: ENABLE_LUCID

Das Projekt nutzt ein elegantes Feature-Toggle-System:

```cmake
# CMake Option (Default: ON für Backward-Compatibility)
option(ENABLE_LUCID "Enable LUCID GigE camera support" ON)

# Wenn ENABLE_LUCID=ON:
#   - LUCIDStream.cpp wird kompiliert
#   - Arena SDK Libraries werden gelinkt
#   - MP_ENABLE_LUCID Compile Definition gesetzt

# Wenn ENABLE_LUCID=OFF:
#   - LUCIDStream.cpp wird NICHT kompiliert
#   - Arena SDK wird NICHT benötigt
#   - Runtime Fallback: klare Fehlermeldung wenn streamType=1
```

### CMake Presets

```json
"linux-debian"  → Release + ENABLE_LUCID=ON  (für Debian x86_64)
"linux-rpi5"    → Release + ENABLE_LUCID=OFF (für Raspberry Pi 5)
```

### Platform-spezifische Anpassungen

| Feature | Windows | Linux |
|---------|---------|-------|
| Webcam Backend | CAP_DSHOW, CAP_MSMF | CAP_V4L2 |
| LUCID Support | Arena SDK (Win64_x64) | Arena SDK (Linux64_x64/ARM) |
| OpenCV | Hardcodiert | Find-Package, Standard-Pfade |

---

## 📚 Dokumentation

### Für Schnelle Übersicht
- `docs/QUICKSTART.md` - 5-Minuten TL;DR

### Für Detailierte Installation
- `docs/linux-setup.md` - Schritt-für-Schritt Anleitung
  1. System-Pakete
  2. OpenCV
  3. AprilTag
  4. Arena SDK (optional)
  5. MarkerPositioning Build
  6. Erste Schritte
  7. **Troubleshooting** (wichtig!)

### Für Technische Details
- `docs/LINUX_IMPLEMENTATION_CHECKLIST.md` - Alle Code-Änderungen dokumentiert
- `tools/README.md` - Jedes Script erklärt

### Für Projekt-Konfiguration
- `SETTINGS.md` - Runtime Konfiguration
- `README.md` - Projekt-Überblick

---

## 🐛 Häufige Probleme & Lösungen

### "CMake Error: spdlog not found"
```bash
cd ~/MarkerPositioning
git submodule update --init --recursive
cd MarkerPositioning
./build.sh --preset linux-debian
```

### "Could not find OpenCV"
Nutze `./tools/install-linux.sh` oder folge `docs/linux-setup.md` Kapitel 2.

### "Permission denied" für AprilTag Install
```bash
sudo cmake --install build  # Statt: cmake --build build --target install
sudo ldconfig
```

### "ENABLE_LUCID=ON but Arena SDK not found"
- Entweder: `export LUCID_DEV_ROOT=/opt/ArenaSDK_Linux_x64`
- Oder: `./build.sh --preset linux-rpi5` (ohne LUCID)

**Weitere Lösungen:** Siehe `docs/linux-setup.md` Kapitel 7

---

## 🧪 Testing & Verifikation

### Nach dem Build

```bash
# Automatische Verifikation
./tools/verify-build.sh

# Manual Check
file out/build/linux-debian/MarkerPositioning
ldd out/build/linux-debian/MarkerPositioning | grep opencv
```

### Runtime Test

```bash
# Starte mit Default Settings
./out/build/linux-debian/MarkerPositioning

# Du solltest sehen:
# [2026-09-16 15:08:47] LUCID GigE camera support: ENABLED (ENABLE_LUCID=ON)
# oder
# [2026-09-16 15:08:47] LUCID GigE camera support: DISABLED (ENABLE_LUCID=OFF)
```

---

## 📊 Performance Expectations

| Schritt | Zeit | Hardware |
|---------|------|----------|
| System-Pakete | ~10 min | Debian 13 x86_64 |
| OpenCV Build | 30-45 min | Debian 13 (4-Core CPU) |
| OpenCV Build | 2-4 Stunden | Raspberry Pi 5 |
| AprilTag Build | ~5 min | Beliebig |
| MarkerPositioning Build | ~1-2 min | Beliebig |
| **Gesamt** | **~1-2 Stunden** | **Debian 13** |
| **Gesamt** | **~3-5 Stunden** | **Raspberry Pi 5** |

---

## 🔄 Deployment auf Raspberry Pi 5

### Option 1: Lokal auf RPi5 bauen

```bash
git clone https://github.com/rt-uni-rostock/MarkerPositioning.git
cd MarkerPositioning/MarkerPositioning
./tools/install-linux.sh              # ~4-5 Stunden OpenCV auf ARM
```

### Option 2: Cross-Compile auf Debian, Deploy auf RPi5

(Wird in separatem Guide beschrieben)

---

## ✨ Was ist Neu?

### Für Benutzer
- ✅ Ein Befehl Setup: `./tools/install-linux.sh`
- ✅ Automatische Abhängigkeitsprüfung: `./tools/diagnose-linux.sh`
- ✅ Automatische Verifikation: `./tools/verify-build.sh`
- ✅ Vollständige Dokumentation
- ✅ Systemd Autostart Template

### Für Entwickler
- ✅ CMake Presets statt manueller Config
- ✅ Feature Toggle für optionale Dependencies
- ✅ Platform-spezifische Code-Pfade
- ✅ Erweiterte Fehlerbehandlung
- ✅ Strukturierte Dokumentation

### Für Maintainer
- ✅ Windows-Build unverändert (Regression-Test ✅)
- ✅ Modular erweiterbar (weitere Linux-Varianten)
- ✅ Automatisierbare CI/CD Integration
- ✅ Vollständige Audit-Trail aller Änderungen

---

## 🎯 Nächste Schritte

### Jetzt sofort
1. Führe aus: `./tools/diagnose-linux.sh`
2. Überprüfe den Output auf kritische Fehler
3. Führe aus: `./tools/install-linux.sh`
4. Warte auf Completion (~2-3 Stunden auf Debian)

### Nach erfolgreichem Build
1. Führe aus: `./tools/verify-build.sh`
2. Bearbeite `out/build/linux-debian/Settings.json`
3. Starte: `./out/build/linux-debian/MarkerPositioning`
4. Teste mit Deinem Kamera-Setup

### Optional
- Installiere Systemd Unit für Autostart
- Erstelle Wrapper-Script für Produktivumgebung
- Integriere in Deine CI/CD Pipeline

---

## 📞 Support & Troubleshooting

### Diagnose durchführen
```bash
./tools/diagnose-linux.sh              # Zeigt Systemstatus
```

### Logs überprüfen
```bash
journalctl -u marker-positioning.service -n 50    # Letzte 50 Zeilen
tail -50 /tmp/install-linux-*.log                  # Install-Log
```

### Manuelle Überprüfungen
```bash
pkg-config --modversion opencv4        # OpenCV Version
ls -la /usr/local/include/apriltag/    # AprilTag Header
ls -la /usr/local/lib/libapriltag.a    # AprilTag Library
echo $LUCID_DEV_ROOT                   # Arena SDK Path
```

### Dokumentation
- Quick Start: `docs/QUICKSTART.md`
- Vollständig: `docs/linux-setup.md`
- Details: `docs/LINUX_IMPLEMENTATION_CHECKLIST.md`
- Tool-Übersicht: `tools/README.md`

---

## 🏆 Zusammenfassung

Das MarkerPositioning-Projekt ist nun **vollständig produktionsreif für Linux**:

✅ Code-Änderungen implementiert und getestet  
✅ Dokumentation komplett und benutzerfreundlich  
✅ Automatische Install/Diagnose/Verify Tools bereit  
✅ Feature Toggle für optionale Dependencies  
✅ Windows-Unterstützung unverändert  
✅ Raspberry Pi 5 Ready  

**Implementierungs-Status:** 🎉 **100% FERTIG**

---

## 📝 Version & Changelog

**Status-Stand:** 2026-09-16  
**Letzte Änderung:** Komplette Dokumentation & Tools  

**Implementierte Features:**
- [x] CMakeLists.txt: ENABLE_LUCID Option
- [x] Linux Arena SDK Pfad-Handling
- [x] CMake Presets für Debian & RPi5
- [x] build.sh Preset-Support
- [x] ImageSourceFactory.cpp Fallback
- [x] WebcamStream.cpp V4L2 Support
- [x] docs/linux-setup.md
- [x] docs/QUICKSTART.md
- [x] tools/install-linux.sh
- [x] tools/diagnose-linux.sh
- [x] tools/verify-build.sh
- [x] tools/marker-positioning.service
- [x] Comprehensive Checklists & READMEs

---

**Viel Erfolg! 🚀**

Bei Fragen: Siehe `docs/linux-setup.md` oder führe `./tools/diagnose-linux.sh` aus.
