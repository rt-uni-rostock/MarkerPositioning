# Supervisor

## Überblick

Der Supervisor entkoppelt die Anwendung von der konkreten Frage "Woher kommen die Bilder, wie
werden sie verarbeitet und wohin gehen die Ergebnisse?". Jeder Supervisor implementiert das
Interface `ISupervisorMode` (`start()` / `stop()`) und verdrahtet dafür eine `ImageSource`
(Kamera-Stream oder Datei), optional eine `DetectionPipeline` (AprilTag-/ArUco-Erkennung) und
einen `Sink` bzw. `ImageUdpPublisher` für die Ausgabe.

`SupervisorFactory` liest `supervisorMode` aus `Settings.json` und baut daraus genau einen der
folgenden Supervisor auf:

| `supervisorMode` (Wert) | Klasse | Status |
|---|---|---|
| `LiveDetection` (0) | `LiveSupervisorMode` | implementiert, produktiver Modus |
| `StaticDetection` (1) | `StaticSupervisorMode` | **nicht implementiert** (Platzhalter) |
| `LiveImagePassthrough` (2) | `LiveImagePassthroughSupervisorMode` | implementiert, Debug-/Kalibrier-Modus |

## LiveDetection

Der Standard-Modus für den produktiven Betrieb: Live-Kamerabilder werden erkannt und die
Ergebnisse per UDP verschickt sowie vollständig in SQLite protokolliert.

Ablauf (`LiveSupervisorMode`):

1. Für jede aktive Kamera (`cameras[].active == true` in `Settings.json`) werden **zwei Worker**
   angelegt, die sich jeweils eine eigene `DetectionPipeline`-Instanz teilen. Zwei Worker pro
   Kamera sorgen dafür, dass ein Zyklus, dessen Verarbeitung länger als das Intervall dauert,
   den nächsten Zyklus nicht blockiert - während ein Worker noch rechnet, kann der andere den
   nächsten Frame übernehmen.
2. Ein Supervisor-Thread wacht in festen Intervallen auf (Intervall = `1000 / frameRate`
   Millisekunden) und startet für jede aktive Kamera - sofern gerade ein Worker frei ist - einen
   neuen Erkennungszyklus (`handleCycle()`).
3. Der Worker holt sich das **zuletzt empfangene Bild** von der zugehörigen `ImageSource`
   (`getLatestFrame()`, das Bild selbst wird von einem separaten Capture-Thread kontinuierlich
   aktualisiert) und lässt es durch die `DetectionPipeline` laufen (Undistortion, Tag-Erkennung,
   Pose-Schätzung).
4. Das Ergebnis (`PipelineResult`, ggf. mit mehreren erkannten Markern) wird an den `Sink`
   übergeben. Bleibt ein Worker über das Intervall hinaus aktiv, wird das als "Deadline
   überschritten" geloggt, der Zyklus läuft aber weiter.

Der `Sink` verteilt jedes Ergebnis auf zwei unabhängige, asynchrone Worker-Threads:

- **UDP-Versand:** pro Kamera wird immer nur das *neueste* Ergebnis vorgehalten
  ("latest-only"). Kommt ein neues Ergebnis, bevor das vorherige verschickt wurde, wird das
  ältere verworfen (und als Drop-Event protokolliert). Für jeden erkannten Marker wird ein
  eigenes UDP-Paket verschickt.
- **SQLite-Logging:** jedes Ergebnis wird vollständig und ohne Drops in `results.db`
  protokolliert (gebündelt in Batches, siehe `IO/Sink/ResultLogger.cpp`).

### UDP-Paketformat für `LiveDetection` (MarkerMessage)

Für jeden in einem Zyklus erkannten Marker sendet der UDP-Worker (`IO/Sink/Sink.cpp`,
`udpWorkerLoop()`) genau ein UDP-Paket an `udpIp`:`udpPort` aus `Settings.json`. Aufbau und
Versand übernimmt `IO/Sink/UdpPublisher.cpp` (`UdpPublisher::serialize()`).

- **Paketgröße:** fest **176 Bytes** (`22 * sizeof(double)`), Byte-Reihenfolge **Little-Endian**.
- **Einheiten:** Position in Metern (Kamera-Koordinatensystem), Rotation in **Radiant**.
- Nicht benutzte Bytes sind `0x00` (der Sendepuffer wird nullinitialisiert und nur teilweise
  beschrieben).

| Offset (Bytes) | Größe | Typ | Feld | Bedeutung |
|---|---|---|---|---|
| 0 | 8 | `double` | `rotX` | Roll (`pose.roll`), Radiant |
| 8 | 8 | `double` | `rotZ` | Yaw (`pose.yaw`), Radiant — Achtung: liegt an zweiter Stelle im Paket, nicht an dritter |
| 16 | 8 | `double` | `rotY` | Pitch (`pose.pitch`), Radiant |
| 24 | 8 | `double` | reserviert | immer `-1.0` |
| 32 | 8 | `double` | `posX` | X-Position in Metern |
| 40 | 8 | `double` | `posY` | Y-Position in Metern |
| 48 | 8 | `double` | `posZ` | Z-Position in Metern |
| 56 | 1 | `uint8` | `markerId` | erkannte Tag-ID (0–255) |
| 57 | 8 | `double` | `cameraId` | ID der Kamera, siehe `cameras[].id` in `Settings.json` |
| 65 | 88 | `double[11]` | Padding | reserviert für zukünftige Erweiterungen, aktuell immer `0.0` |
| 153 | 23 | – | ungenutzt | Rest des 176-Byte-Puffers, immer `0x00` |

**Wichtiger Hinweis:** `MarkerMessage.h` definiert zusätzliche Felder (`imageTimestamp`,
`markerType`, `errorCode`, `errorMessage`), die intern für das SQLite-Logging
(`IO/Sink/ResultLogger.cpp`) verwendet werden, aber **nicht** Teil des UDP-Pakets sind —
`UdpPublisher::serialize()` schreibt ausschließlich die oben aufgeführten Felder.

Es wird **kein Paket gesendet**, wenn in einem Zyklus kein Marker erkannt wurde
(`pose.tagId == -1`); in diesem Fall bleibt `result.detectedMarkers` leer und die
Marker-Schleife im UDP-Worker läuft nicht durch.

Referenz-Receiver (Python), inklusive Parsing-Beispiel: `tools/udp_message_receiver.py`.

## LiveImagePassthrough

Debug-/Kalibrier-Modus: Es findet **keine** Tag-Erkennung statt. Stattdessen wird das jeweils
aktuellste, unkomprimierte Kamerabild jeder aktiven Kamera in festen Intervallen (`frameRate`)
per UDP verschickt (`LiveImagePassthroughSupervisorMode`, `ImageUdpPublisher`). Nützlich, um z. B.
den rohen Kamerastream auf einem anderen Rechner anzusehen oder eine Kamera zu kalibrieren, ohne
dass die Detection-Pipeline oder SQLite-Logging beteiligt sind.

Da UDP-Pakete auf ca. 1400 Bytes begrenzt sind (Standard-MTU), wird ein Bild in mehrere Chunks
aufgeteilt und jedem Chunk ein Header vorangestellt, der die Reassemblierung beim Empfänger
ermöglicht.

### UDP-Paketformat für `LiveImagePassthrough` (Version 2)

- **Chunk-Header:** fest **64 Bytes**, danach folgen bis zu `chunkBytes` Bytes reiner
  Bilddaten (Zeilen-major, wie im `cv::Mat`-Speicher).
- Byte-Reihenfolge ist explizit **Little-Endian** (`byteOrder = 1`), Struct ist gepackt
  (`#pragma pack(push, 1)`), es gibt also keine Padding-Bytes im Header selbst.

| Offset (Bytes) | Größe | Typ | Feld | Bedeutung |
|---|---|---|---|---|
| 0 | 4 | `uint32` | `magic` | konstant `0x4D50494D` ("MPIM") zur Paket-Erkennung |
| 4 | 2 | `uint16` | `version` | aktuell `2` |
| 6 | 1 | `uint8` | `byteOrder` | `1` = Little-Endian |
| 7 | 1 | `uint8` | `cameraId` | ID der Kamera, siehe `cameras[].id` in `Settings.json` |
| 8 | 8 | `uint64` | `frameId` | fortlaufende Frame-Nummer der Quelle |
| 16 | 8 | `int64` | `captureTimestampNs` | Aufnahmezeitpunkt, Unix-Epoche in Nanosekunden |
| 24 | 8 | `int64` | `receiveTimestampNs` | Zeitpunkt, zu dem der Supervisor den Frame abgeholt hat, Unix-Epoche in Nanosekunden |
| 32 | 4 | `int32` | `width` | Bildbreite in Pixeln |
| 36 | 4 | `int32` | `height` | Bildhöhe in Pixeln |
| 40 | 4 | `int32` | `cvType` | OpenCV-Typ des Bildes (`cv::Mat::type()`, z. B. `CV_8UC3`) |
| 44 | 1 | `uint8` | `channels` | Anzahl Farbkanäle |
| 45 | 1 | `uint8` | `elemSizeBytes` | Bytes pro Kanal (`elemSize1()`) |
| 46 | 1 | `uint8` | `depthCode` | OpenCV-Tiefencode (`cv::Mat::depth()`) |
| 47 | 1 | `uint8` | reserviert | immer `0` |
| 48 | 4 | `uint32` | `totalImageBytes` | Gesamtgröße des unkomprimierten Bildes in Bytes |
| 52 | 4 | `uint32` | `totalChunks` | Anzahl Chunks, in die dieses Bild aufgeteilt wurde |
| 56 | 4 | `uint32` | `chunkIndex` | Index dieses Chunks (`0`-basiert) |
| 60 | 2 | `uint16` | `chunkBytes` | Anzahl Nutzdaten-Bytes in diesem Chunk (kann beim letzten Chunk kleiner sein) |
| 62 | 2 | `uint16` | `chunkStrideBytes` | Nutzdaten-Bytes pro *vollem* Chunk, damit ein Empfänger die Byte-Position `chunkIndex * chunkStrideBytes` ohne Zwischenspeichern aller Chunks berechnen kann |

Ein Empfänger sammelt so lange Chunks mit gleicher `frameId`/`cameraId`, bis alle `totalChunks`
Chunks vorliegen, und setzt das Bild anhand von `chunkIndex * chunkStrideBytes` wieder
zusammen.

Referenz-Receiver (Python), inklusive Reassemblierung: `tools/udp_image_receiver.py`.

## StaticDetection

Als Modus vorgesehen für die Verarbeitung bereits vorhandener Bilddateien aus einem Ordner
(statt Live-Kamerabildern), aber aktuell **nicht implementiert**: `StaticSupervisorMode::start()`
und `::stop()` geben unconditional `false` zurück, und `SupervisorFactory::create()` wirft eine
`std::runtime_error`, wenn `supervisorMode` auf `StaticDetection` gesetzt wird.

## Konfiguration

`Settings.json` steuert über `supervisorMode` den zu startenden Supervisor sowie über
`sourceMode`, `cameras[]`, `frameRate`, `tagSize`/`tagID`/`tagType`, `quadDecimate`,
`enableImageLogging`/`imageLogOptions`, `udpIp`/`udpPort` die jeweiligen Details. Siehe
`SETTINGS.md` für die vollständige Beschreibung aller Felder.

## Ideen für weitere Supervisoren

- Supervisor, der ohne festes Intervall sequentiell Live-Daten verarbeitet. Dabei könnte man
  mehrere Worker anlegen, um die Erkennung auf mehrere Threads aufzuteilen.