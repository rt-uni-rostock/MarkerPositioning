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