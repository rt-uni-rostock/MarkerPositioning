# Settings.json - Konfigurationshandbuch

Dieses Dokument beschreibt alle verfügbaren Konfigurationsparameter in der `Settings.json`-Datei. Die Datei steuert das Verhalten der MarkerPositioning-Anwendung, einschließlich der Supervisor-Modus-Auswahl, Kamera-Konfiguration, Marker-Erkennung und UDP-Ausgabe.

## ⚠️ Wichtige Hinweise

- **JSON-Syntax**: JSON unterstützt keine Kommentare. Achte auf korrekte Syntax (Kommas zwischen Elementen, keine Kommas nach letzten Elementen).
- **Neustart erforderlich**: Änderungen in `Settings.json` erfordern einen Neustart der Anwendung.
- **Feste Parameter**: Die UDP-Performance-Parameter (`imageUdpMaxPayloadBytes`, `imageUdpPacingBurstChunks`, `imageUdpPacingSleepMs`) sind nicht konfigurierbar und fest für Echtzeit-Stabilität eingestellt.

---

## Globale Parameter

### `supervisorMode` (Integer, erforderlich)
Bestimmt, welcher Supervisor aktiv ist und damit die Pipeline-Verarbeitung steuert.

| Wert | Supervisor | Beschreibung |
|------|-----------|-------------|
| `0` | `LiveSupervisorMode` | Verarbeitet Live-Bilder durch die DetectionPipeline und speichert Ergebnisse in SQLite + UDP |
| `1` | `StaticSupervisorMode` | Liest statische Bilder aus Verzeichnis, verarbeitet sie durch DetectionPipeline, speichert Ergebnisse (noch nicht implementiert) |
| `2` | `LiveImagePassthroughMode` | Sendet Live-Bilder unverändert als UDP-Pakete (Echtzeitpassthrough ohne Detection) |

**Default:** `2`

**Beispiel:**
```json
"supervisorMode": 2
```

---

### `sourceMode` (Integer, erforderlich)
Bestimmt die Art der Bildquelle innerhalb des gewählten Supervisors.

| Wert | Quelle | Beschreibung |
|------|--------|-------------|
| `0` | Live-Stream | Kamera-Streams (RTSP, RTP, GigE, USB-Webcam) von aktiven Kameras |

**Default:** `0`

**Beispiel:**
```json
"sourceMode": 0
```

---

### `frameRate` (Float, erforderlich)
Bestimmt die Verarbeitungs-Frequenz in Frames pro Sekunde (FPS) für die Supervisor-Loop.

- **Min:** 0.1 FPS
- **Max:** Begrenzt durch Kamera + Netzwerk/Verarbeitung
- **Empfohlen für Tests:** 1-30 FPS

**Auswirkung:**
- Bei `supervisorMode=2` (LiveImagePassthrough): Steuert das Sendeintervall der UDP-Pakete.
- Bei anderen Modi: Steuert die DetectionPipeline-Zyklusrate.

**Default:** `1`

**Beispiel:**
```json
"frameRate": 1
```

---

## Marker-Erkennung (AprilTags)

### `tagType` (Integer, erforderlich)
Bestimmt die AprilTag-Familie für die Erkennung.

| Wert | Tag-Familie | Größe | Kosten |
|------|-----------|------|--------|
| `0` | tag36h11 | 36 Bits | ~3-5 ms pro Frame |
| `1` | tag25h9 | 25 Bits | ~2-3 ms pro Frame |
| `2` | tag16h5 | 16 Bits | ~1-2 ms pro Frame |

**Default:** `0`

**Beispiel:**
```json
"tagType": 0
```

---

### `tagSize` (Float, erforderlich)
Die physikalische Seitenlänge des AprilTag-Markers in Metern.

- Verwendet für 6-DoF Pose-Berechnung.
- Muss der echten physikalischen Markergröße entsprechen.

**Default:** `0.12` (12 cm)

**Beispiel:**
```json
"tagSize": 0.12
```

---

### `tagID` (Integer, erforderlich)
Die zu erkennende Marker-ID innerhalb der ausgewählten Tag-Familie.

- Muss innerhalb des Wertebereichs der Tag-Familie liegen (z.B. 0-586 für tag36h11).

**Default:** `17`

**Beispiel:**
```json
"tagID": 17
```

---

### `quadDecimate` (Float, erforderlich)
Dezimierungsfaktor für die Quad-Erkennung während der Bildvorverarbeitung.

| Wert | Effekt | Vorteile | Nachteile |
|------|--------|----------|-----------|
| `1.0` | Keine Dezimierung | Höchste Genauigkeit | Höchste CPU-Last |
| `2.0` | 50% Pixelzahl | Gutes Gleichgewicht | Leicht reduzierte Genauigkeit |
| `4.0` | 75% Pixelzahl | Schnell | Reduzierte Genauigkeit bei kleinen Tags |

**Default:** `4.0`

**Beispiel:**
```json
"quadDecimate": 4.0
```

---

## Netzwerk (UDP)

### `udpIp` (String, erforderlich)
IP-Adresse des Zielrechners für UDP-Pakete (nur für Passthrough/Output-Modi).

- **localhost:** `"127.0.0.1"` (nur lokal auf gleicher Maschine)
- **Netzwerk:** `"192.168.x.x"` (andere Maschine im Netzwerk)
- **Broadcast:** `"255.255.255.255"` (alle im lokalen Netzwerk, nicht empfohlen)

**Default:** `"127.0.0.1"`

**Beispiel:**
```json
"udpIp": "192.168.3.50"
```

---

### `udpPort` (Integer, erforderlich)
Der UDP-Port des Zielrechners.

- **Standard-Ports:** 5000-5999 (meist frei)
- **Achtung:** Ports <1024 erfordern Admin-Rechte auf Unix-Systemen.

**Default:** `5001`

**Beispiel:**
```json
"udpPort": 5001
```

---

## Image Logging & Output

### `enableImageLogging` (Boolean, erforderlich)
Aktiviert oder deaktiviert das Speichern von Bildern auf der Festplatte.

- `true`: Bilder/Ergebnisse werden unter `imageOutputPath` gespeichert.
- `false`: Keine lokalen Dateien geschrieben.

**Default:** `true`

**Beispiel:**
```json
"enableImageLogging": true
```

---

### `imageOutputPath` (String, erforderlich wenn `enableImageLogging=true`)
Verzeichnis, in dem Bilder und Ergebnisse gespeichert werden.

- Kann relativ (`./ logs/frames/`) oder absolut (`C:/temp/markers/`) sein.
- Das Verzeichnis wird automatisch erstellt, falls es nicht existiert.

**Default:** `"./logs/frames/"`

**Beispiel:**
```json
"imageOutputPath": "./logs/frames/"
```

---

### `imageLogOptions` (Object)
Feinkontrolle, welche Bilder gespeichert werden.

#### `saveRawFrames` (Boolean)
Speichert unbearbeitete Originalbilder von jeder Kamera.

- **true:** Unkomprimierte RAW-Frames (großer Speicherplatz)
- **false:** Nicht speichern

**Default:** `false`

---

#### `saveGrayFrames` (Boolean)
Speichert Graustufenversionen (nach Vorverarbeitung).

- **true:** Gray-Frames aus der Pipeline
- **false:** Nicht speichern

**Default:** `false`

---

#### `saveDetectionResults` (Boolean)
Speichert Ergebnisse der Marker-Erkennung (Bboxes, Pose-Vektoren).

- **true:** Speichert erkannte Marker als Text/Bilder
- **false:** Nicht speichern

**Default:** `true`

---

#### `visualizeAllDetections` (Boolean)
Zeichnet erkannte Marker in Bildern ein (für visuelle Überprüfung).

- **true:** Markiert gefundene Tags mit Bounding Boxes und IDs
- **false:** Keine Visualisierung

**Default:** `true`

---

## Kamera-Konfiguration

Die `cameras` Array enthält alle verfügbaren Kameras. Für jede Kamera wird folgende Struktur verwendet:

```json
{
  "id": 2,
  "streamType": 4,
  "name": "Laptop Webcam",
  "fx": 1155.539961700352,
  "fy": 1156.270850547310,
  "cx": 968.3789685322045,
  "cy": 546.6712405711692,
  "d1": 0.102755637208742,
  "d2": -0.113412261446679,
  "d3": 0.0,
  "d4": 0.0,
  "d5": 0.0,
  "active": true
}
```

### Erforderliche Felder

#### `id` (Integer)
Eindeutige Kamera-ID innerhalb der Konfiguration.

- Wird in UDP-Paket-Headern und Logs verwendet.
- Muss eindeutig sein.

---

#### `streamType` (Integer)
Der Typ der Bildquelle:

| Wert | Typ | Beschreibung |
|------|-----|-------------|
| `1` | GigE | LUCID Vision GigE Kameras |
| `2` | RTP | RTP Video Stream |
| `3` | RTSP | RTSP Video Stream (z.B. IP-Kameras) |
| `4` | USB/Webcam | USB-Webcams oder lokale Kameras |

---

#### `name` (String)
Beschreibender Name der Kamera (für Logs und UI).

**Beispiel:**
```json
"name": "Laptop Webcam"
```

---

#### `serialNumber` (String, erforderlich bei mehreren LUCID-Kameras)
Seriennummer der physischen LUCID-Kamera (nur relevant für `streamType=1`).

- Wird beim Start jeder LUCID-Kamera verwendet, um das passende `Arena::DeviceInfo`
  aus der Liste aller erkannten GigE-Geräte auszuwählen.
- **Ist genau eine LUCID-Kamera erkannt und `serialNumber` leer:** die erkannte
  Kamera wird automatisch verwendet (Rückwärtskompatibilität), es wird jedoch
  eine Warnung geloggt.
- **Sind mehrere LUCID-Kameras aktiv konfiguriert:** `serialNumber` ist für
  jede davon **Pflicht** und muss eindeutig sein. Fehlt sie oder ist sie
  doppelt vergeben, bricht die Anwendung beim Start mit einer klaren
  Fehlermeldung ab, statt dass zwei Kamera-Threads sich um dasselbe Gerät
  streiten.
- Die tatsächlich erkannten Seriennummern werden beim Start jeder LUCID-Kamera
  geloggt (`Detected LUCID camera: serial=..., model=..., ip=...`), sodass sie
  einfach in die Konfiguration übernommen werden können.

**Beispiel:**
```json
"serialNumber": "223900123"
```

---

#### `url` (String, optional)
URL zum Video-Stream (nur für `streamType` 2, 3).

**Beispiele:**
```json
"url": "rtsp://192.168.3.60:554/ch01"
"url": "rtp://224.1.1.1:5000"
```

---

#### `pipeline` (String, optional)
GStreamer-Pipeline für spezielle Stream-Typen (nur für `streamType` 2).

**Standard:** `"gstreamer"`

---

### Kamera-Intrinsiken (Kalibrierung)

Diese Parameter beschreiben die optischen Eigenschaften der Kamera und müssen exakt kalibriert sein für präzise Pose-Schätzung.

#### `fx`, `fy` (Float)
Brennweite in Pixeln:
- `fx`: Brennweite in x-Richtung
- `fy`: Brennweite in y-Richtung
- Bei idealen quadratischen Pixeln: `fx ≈ fy`

**Typische Werte:** 500–3000 (abhängig von Kameraauflösung und Linse)

---

#### `cx`, `cy` (Float)
Hauptpunkt (optical center) in Pixeln:
- `cx`: x-Koordinate des Bildzentrums
- `cy`: y-Koordinate des Bildzentrums
- Üblicherweise etwa Bildbreite/2 und Bildheöhe/2

**Typische Werte:** 500–2000

---

#### `d1`–`d5` (Float)
Verzeichnungskoeffizienten (Distortion):
- `d1`, `d2`: Radiale Verzeichnung (wichtigste)
- `d3`, `d4`: Tangentiale Verzeichnung
- `d5`: Höhere radiale Verzeichnung (meist 0)

**Typische Werte:**
- Kleine Verzeichnung: `-0.5` bis `+0.5`
- Hohe Verzeichnung: `-2.0` bis `+2.0`

**Sonderfall:** Nicht verzerrte Kamera: alle auf `0.0` setzen.

---

#### `active` (Boolean)
Bestimmt, ob die Kamera aktiv genutzt wird.

- `true`: Kamera wird verarbeitet
- `false`: Kamera wird ignoriert

**Wichtig:** Für `LiveImagePassthrough` muss **mindestens eine** Kamera `"active": true` sein.

---

## Vollständiges Beispiel

```json
{
  "supervisorMode": 2,
  "sourceMode": 0,
  "frameRate": 1,
  "quadDecimate": 4.0,
  
  "udpIp": "192.168.3.50",
  "udpPort": 5001,
  
  "tagType": 0,
  "tagSize": 0.12,
  "tagID": 17,
  
  "enableImageLogging": true,
  "imageOutputPath": "./logs/frames/",
  "imageLogOptions": {
    "saveRawFrames": false,
    "saveGrayFrames": false,
    "saveDetectionResults": true,
    "visualizeAllDetections": true
  },
  
  "cameras": [
    {
      "id": 2,
      "streamType": 4,
      "name": "Laptop Webcam",
      "fx": 1155.539961700352,
      "fy": 1156.270850547310,
      "cx": 968.3789685322045,
      "cy": 546.6712405711692,
      "d1": 0.102755637208742,
      "d2": -0.113412261446679,
      "d3": 0.0,
      "d4": 0.0,
      "d5": 0.0,
      "active": true
    }
  ]
}
```

---

## Häufige Szenarien

### Szenario 1: Live-Marker-Erkennung mit UDP-Output
```json
{
  "supervisorMode": 0,
  "frameRate": 30,
  "udpIp": "192.168.1.100",
  "udpPort": 5001,
  "tagSize": 0.1,
  "tagID": 5
}
```

### Szenario 2: Live-Image-Passthrough (Bildstrom)
```json
{
  "supervisorMode": 2,
  "frameRate": 1,
  "udpIp": "192.168.1.100",
  "udpPort": 5001
}
```

### Szenario 3: Lokal mit Bildlogging
```json
{
  "supervisorMode": 0,
  "frameRate": 10,
  "enableImageLogging": true,
  "imageOutputPath": "./detections/",
  "imageLogOptions": {
    "saveDetectionResults": true,
    "visualizeAllDetections": true
  }
}
```

### Szenario 4: Mehrere LUCID-Kameras parallel
```json
{
  "cameras": [
    {
      "id": 1,
      "streamType": 1,
      "name": "LUCID Kamera links",
      "serialNumber": "223900123",
      "active": true
    },
    {
      "id": 2,
      "streamType": 1,
      "name": "LUCID Kamera rechts",
      "serialNumber": "223900456",
      "active": true
    }
  ]
}
```
Jede LUCID-Kamera läuft in ihrem eigenen Capture-Thread; `serialNumber` sorgt
dafür, dass jede Konfiguration an das richtige physische Gerät gebunden wird.
Die verfügbaren Seriennummern werden beim Start im Log ausgegeben.

---

## Troubleshooting

### Problem: "Settings.json not found"
- Stelle sicher, dass `Settings.json` im selben Verzeichnis wie die EXE liegt.
- Die Master-Kopie liegt im Repo unter `settings/Settings.json` und wird von `build.sh` in den Build-Output-Ordner kopiert.

### Problem: UDP-Pakete kommen nicht an
- Prüfe `udpIp` und `udpPort`.
- Firewall-Regeln prüfen (UDP-Port kann blockiert sein).
- Testiere mit `udpIp: "127.0.0.1"` (localhost).

### Problem: Marker werden nicht erkannt
- Kontrolliere `tagID` und `tagType`.
- Verifiziere Kamera-Intrinsiken (`fx`, `fy`, `cx`, `cy`).
- Erhöhe `frameRate` oder reduziere `quadDecimate`.

### Problem: Hohe Latenz beim Image-Passthrough
- Reduziere `frameRate` (mehr Zeit zwischen Frames).
- Prüfe Netzwerkverbindung und UDP-Buffer-Einstellungen des Empfängers.

---

## Weitere Ressourcen

- **Supervisor-Dokumentation:** Siehe `Supervisor/README.md` für Details zu den verschiedenen Verarbeitungsmodi.
- **UDP-Protokoll:** Siehe `Supervisor/README.md` für das LiveImagePassthrough UDP-Paketformat.
- **Kamera-Kalibrierung:** Nutze OpenCV-Kalibriertools zur Bestimmung von Intrinsiken.

