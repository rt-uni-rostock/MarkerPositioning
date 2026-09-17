# Tools für MarkerPositioning

Dieses Verzeichnis enthält Hilfsskripte für Installation, Diagnose und Verifikation auf Linux-Systemen.

## Scripts

### 1. `install-linux.sh` - Vollständige automatisierte Installation

**Zweck:** Installiert alle Abhängigkeiten und baut das Projekt in einem Durchgang.

**Verwendung:**
```bash
cd ~/MarkerPositioning/MarkerPositioning
./tools/install-linux.sh
```

**Was es macht:**
1. Aktualisiert System-Pakete
2. Baut OpenCV 4.12.0 aus Quellcode
3. Baut AprilTag aus Quellcode
4. Konfiguriert Arena SDK (falls vorhanden)
5. Initialisiert Git Submodules
6. Baut MarkerPositioning mit appropriatem Preset

**Ausgabe:**
- Binary: `out/build/linux-debian/MarkerPositioning` (oder linux-rpi5)
- Settings.json wird automatisch kopiert

**Fehlerbehandlung:**
- Wenn ein Schritt fehlschlägt, wird das Script abgebrochen
- Überprüfe die Fehlermeldung und konsultiere `../docs/linux-setup.md` Kapitel 7

---

### 2. `diagnose-linux.sh` - Abhängigkeitsprüfung

**Zweck:** Überprüft, ob alle erforderlichen Abhängigkeiten vorhanden sind.

**Verwendung:**
```bash
./tools/diagnose-linux.sh
```

**Was es prüft:**
- System-Architektur (x86_64 vs aarch64)
- Build-Tools (cmake, ninja, gcc, g++)
- OpenCV Installation und GStreamer/V4L2 Support
- AprilTag Header und Bibliotheken
- LUCID Arena SDK (nur bei Debian x86_64)
- Git Submodules (spdlog, sqlite)
- Projekt-Konfiguration (CMakeLists.txt, CMakePresets.json, build.sh)

**Ausgabe:**
- ✅ Grün: Abhängigkeit vorhanden und korrekt
- ✗ Rot: Kritischer Fehler
- ⚠ Gelb: Warnung (optional, aber empfohlen)

**Exit-Code:**
- 0: Alle kritischen Abhängigkeiten vorhanden
- 1: Es gibt kritische Fehler

---

### 3. `verify-build.sh` - Build-Verifikation

**Zweck:** Überprüft, ob der Build erfolgreich war und das Binary funktioniert.

**Verwendung (nach erfolgreichem Build):**
```bash
./tools/verify-build.sh
```

**Was es prüft:**
1. Binärdatei existiert
2. Binärdatei ist ausführbar
3. Dateigröße und Abhängigkeiten
4. Settings.json Integrität
5. Anwendung startet ohne Crash
6. LUCID Feature-Status beim Start

**Ausgabe:**
- Detaillierter Report mit allen Überprüfungen
- Fehlerhafte Punkte mit roten ✗
- Erfolgreiche Punkte mit grünen ✓

---

### 4. `marker-positioning.service` - Systemd Unit (Template)

**Zweck:** Ermöglicht Autostart der Anwendung via systemd.

**Installation:**
```bash
# Bearbeite zunächst die Pfade in der Datei
nano tools/marker-positioning.service

# Dann installieren:
sudo cp tools/marker-positioning.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now marker-positioning.service

# Prüfe Status:
sudo systemctl status marker-positioning.service

# Logs anschauen:
sudo journalctl -u marker-positioning.service -f
```

**Wichtig:** Bevor die Unit installiert wird, müssen folgende Pfade angepasst werden:
- `WorkingDirectory`: Absoluter Pfad zum Build-Verzeichnis
- `ExecStart`: Absoluter Pfad zur Binärdatei
- `ExecStartPre`: Pfad zu Settings.json (optional)

**Beispiel-Konfiguration für Debian:**
```ini
[Service]
WorkingDirectory=/home/iat/MarkerPositioning/MarkerPositioning/out/build/linux-debian
ExecStart=/home/iat/MarkerPositioning/MarkerPositioning/out/build/linux-debian/MarkerPositioning
```

---

### 5. `udp_image_receiver.py` - Referenz-Bildempfaenger

**Zweck:** Empfaengt die von `IO/Sink/ImageUdpPublisher` gesendeten, in Chunks aufgeteilten Bilder per UDP, setzt sie wieder zusammen und zeigt sie an bzw. speichert sie ab.

**Python-Setup (fuer dieses und das folgende Script):**
```bash
cd tools
python -m venv venv
# Windows: venv\Scripts\activate    |    Linux/macOS: source venv/bin/activate
pip install -r requirements.txt
```

**Verwendung:**
```bash
python tools/udp_image_receiver.py                          # nur Anzeige
python tools/udp_image_receiver.py --save-dir tools/received # Speichern, keine Anzeige
python tools/udp_image_receiver.py --save-dir tools/received --display  # Speichern UND Anzeige
```

Siehe auch `udp_image_receiver_matlab.m` fuer eine MATLAB-Variante.

---

### 6. `udp_message_receiver.py` - Referenz-Nachrichtenempfaenger

**Zweck:** Gegenstelle zu `IO/Sink/UdpPublisher`. Empfaengt die MarkerMessage-Pakete (Position/Rotation je Marker), protokolliert sie lesbar (Konsole + Logdatei) und speichert alle Rohdaten zusaetzlich als JSON-Lines-Datei.

**Verwendung:**
```bash
python tools/udp_message_receiver.py [--host 0.0.0.0] [--port 5001] [--logfile udp_receiver.log] [--jsonfile udp_messages.jsonl]
```

---

## Workflow-Beispiele

### Neue Installation

```bash
cd ~/MarkerPositioning/MarkerPositioning

# 1. Überprüfe Abhängigkeiten
./tools/diagnose-linux.sh

# 2. Installiere ALLES automatisch
./tools/install-linux.sh

# 3. Verifiziere erfolgreiches Build
./tools/verify-build.sh

# 4. Starte Anwendung
./out/build/linux-debian/MarkerPositioning
```

### Nach Quellcode-Änderungen

```bash
# 1. Nur rebuild (ohne Dependencies)
./build.sh --preset linux-debian

# 2. Verifiziere
./tools/verify-build.sh
```

### Clean Rebuild (bei Problemen)

```bash
# 1. Überprüfe System
./tools/diagnose-linux.sh

# 2. Clean Build
./build.sh --clean --preset linux-debian

# 3. Verifiziere
./tools/verify-build.sh
```

### Raspberry Pi 5 Spezifika

```bash
# Nutze immer das linux-rpi5 Preset (ohne LUCID):
./build.sh --preset linux-rpi5

# Verifiziere
./tools/verify-build.sh
```

---

## Häufige Fehler & Lösungen

### "diagnose-linux.sh: Permission denied"
```bash
chmod +x ./tools/*.sh
```

### "CMake Error: spdlog not found"
```bash
cd ~/MarkerPositioning
git submodule update --init --recursive
cd MarkerPositioning
./build.sh --preset linux-debian
```

### "Could not find OpenCV"
```bash
# Überprüfe Installation
pkg-config --modversion opencv4

# Falls nicht installiert, nutze install-linux.sh
./tools/install-linux.sh
```

### "ENABLE_LUCID=ON but the LUCID Arena SDK could not be found"
```bash
# Entweder Arena SDK installieren und Umgebungsvariablen setzen
export LUCID_DEV_ROOT=/opt/ArenaSDK_Linux_x64
export LUCID_GENICAM_PATH=/opt/ArenaSDK_Linux_x64/GenICam
./build.sh --preset linux-debian

# Oder ohne LUCID bauen (z.B. für Raspberry Pi)
./build.sh --preset linux-rpi5
```

---

## Weitere Dokumentation

- **Quick Start:** `docs/QUICKSTART.md`
- **Detaillierte Setup:** `docs/linux-setup.md`
- **Implementierungs-Details:** `docs/LINUX_IMPLEMENTATION_CHECKLIST.md`
- **Projekt-Readme:** `README.md`
- **Konfiguration:** `SETTINGS.md`

---

## Support

Bei Fragen oder Problemen:

1. Führe `diagnose-linux.sh` aus und überprüfe die Ausgabe
2. Lies `docs/linux-setup.md` Kapitel 7 (Troubleshooting)
3. Überprüfe die Logs der letzten Installation in `/tmp/`

---

Letzte Aktualisierung: 2026-09-16
