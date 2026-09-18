# Supervisor

## Allgemein:

Je nach Anforderungen an die Marker Positionierungssoftware k�nnen entsprechende Supervisor angelegt werden. 
Dabei soll die ImageSouce, die Pipeline und die Ausgabe flexibel konfiguriert werden k�nnnen.
Hier zwei Beispiele:

Der LiveSupervisor nimmt in festen Intervallen mehrere Kamera-Streams entgegen, �bergibt diese der Detection-Pipeline und sendet die Ergebnisse per UDP raus und speichert sie parallel in eine SQLite Datei ab.
Dabei nutzt der Supervisor Worker, welche in separaten Threads einen Frame der Detektion �bergibt.

Der StaticSupervisor arbeitet nicht mit Live-Daten, sondern mit Frames, die in einem vorgegebenen Ordner liegen. Diese Bilder werden der Detektion-Pipeline �bergeben und die Ergebnisse in einer SQLite Datenbank gespeichert.

## Pipelinestruktur:

### A Konfiguration
- Input Daten statisch oder live?
   - statisch: Pfad zu den Daten, Daten k�nnen direkt nacheinander verarbeitet werden
   - live: Daten werden kontinuierlich empfangen. Diese m�ssen zwischengespeichert werden. In vorgegebenen Intervallen m�ssen die Daten verarbeitet werden.

### B Supervisor
- verschiedene Supervisor f�r jeweils ausgew�hlte Konfigurationen
	- Static: vorhandene Bilder werden durch die Pipeline geleitet
	- Live: kontinuierlich empfangene Bilder werden durch die Pipeline geleitet


## Ideen f�r weitere Supervisoren:

- Supervisor, der ohne festes Intervall, sequentiell Live Daten verarbeitet. Dabei k�nnte man mehrere Worker anlegen, um die Erkennung auf mehrere Threads aufzuteilen.

## Aktueller Stand der Supervisor-Auswahl

- `Settings.json` steuert �ber `supervisorMode` den zu startenden Supervisor.
- `LiveDetection` verarbeitet Live-Bilder �ber die DetectionPipeline und nutzt den bestehenden Sink.
- `LiveImagePassthrough` sendet `cv::Mat`-Frames unkomprimiert als segmentierte UDP-Chunks (`ImageUdpPublisher`) und enth�lt Capture-Timestamp, Receive-Timestamp und FrameId im Chunk-Header.
- `StaticDetection` ist als Modus vorgesehen, aber weiterhin nicht implementiert.

### UDP-Protokoll f�r `LiveImagePassthrough` (Version 2)

- Byte-Order ist explizit **Little-Endian** (`byteOrder = 1`).
- Header enth�lt zus�tzlich zu `cvType` die Felder `channels`, `elemSizeBytes` und `depthCode`.
- Jedes Paket enth�lt `chunkStrideBytes`, damit ein Empf�nger die Frame-Reassembly deterministisch aus `chunkIndex` berechnen kann.
- Referenz-Receiver (Python): `tools/udp_image_receiver.py`.

### UDP-Protokoll fuer `LiveDetection` (MarkerMessage)

Im `LiveDetection`-Modus sendet der UDP-Worker (`IO/Sink/Sink.cpp`, `udpWorkerLoop()`) fuer
jeden in einem Zyklus erkannten Marker genau ein UDP-Paket an `udpIp`:`udpPort` aus
`Settings.json`. Zustaendig fuer Aufbau/Versand ist `IO/Sink/UdpPublisher.cpp`
(`UdpPublisher::serialize()`).

- **Paketgroesse:** fest **176 Bytes** (`22 * sizeof(double)`), Byte-Order **Little-Endian**.
- **Werte:** Position in Metern (Kamera-Koordinatensystem), Rotation in **Radiant**.
- Nicht benutzte Bytes sind `0x00` (der Sendepuffer wird nullinitialisiert und nur teilweise
  beschrieben).

| Offset (Bytes) | Groesse | Typ | Feld | Bedeutung |
|---|---|---|---|---|
| 0 | 8 | `double` | `rotX` | Roll (`pose.roll`), Radiant |
| 8 | 8 | `double` | `rotZ` | Yaw (`pose.yaw`), Radiant -- Achtung: an dritter Stelle im Pose-Objekt, aber zweites Feld im Paket |
| 16 | 8 | `double` | `rotY` | Pitch (`pose.pitch`), Radiant |
| 24 | 8 | `double` | reserviert | immer `-1.0` |
| 32 | 8 | `double` | `posX` | X-Position in Metern |
| 40 | 8 | `double` | `posY` | Y-Position in Metern |
| 48 | 8 | `double` | `posZ` | Z-Position in Metern |
| 56 | 1 | `uint8` | `markerId` | erkannte Tag-ID (0-255) |
| 57 | 8 | `double` | `cameraId` | ID der Kamera, siehe `cameras[].id` in `Settings.json` |
| 65 | 88 | `double[11]` | Padding | reserviert fuer zukuenftige Erweiterungen, aktuell immer `0.0` |
| 153 | 23 | - | ungenutzt | Rest des 176-Byte-Puffers, immer `0x00` |

**Wichtiger Hinweis:** `MarkerMessage.h` definiert zusaetzliche Felder (`imageTimestamp`,
`markerType`, `errorCode`, `errorMessage`), die intern fuer das SQLite-Logging
(`IO/Sink/ResultLogger.cpp`) verwendet werden, aber **nicht** Teil des UDP-Pakets sind --
`UdpPublisher::serialize()` schreibt ausschliesslich die oben aufgefuehrten Felder.

Es wird **kein Paket gesendet**, wenn in einem Zyklus kein Marker erkannt wurde
(`pose.tagId == -1`); in diesem Fall ist `result.detectedMarkers` leer und die
`for`-Schleife im UDP-Worker laeuft ohne Iteration durch.

Referenz-Receiver (Python), inkl. Parsing-Beispiel: `tools/udp_message_receiver.py`.