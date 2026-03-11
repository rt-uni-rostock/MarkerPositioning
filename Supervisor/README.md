# Supervisor

## Allgemein:

Je nach Anforderungen an die Marker Positionierungssoftware können entsprechende Supervisor angelegt werden. 
Dabei soll die ImageSouce, die Pipeline und die Ausgabe flexibel konfiguriert werden könnnen.
Hier zwei Beispiele:

Der LiveSupervisor nimmt in festen Intervallen mehrere Kamera-Streams entgegen, übergibt diese der Detection-Pipeline und sendet die Ergebnisse per UDP raus und speichert sie parallel in eine SQLite Datei ab.
Dabei nutzt der Supervisor Worker, welche in separaten Threads einen Frame der Detektion übergibt.

Der StaticSupervisor arbeitet nicht mit Live-Daten, sondern mit Frames, die in einem vorgegebenen Ordner liegen. Diese Bilder werden der Detektion-Pipeline übergeben und die Ergebnisse in einer SQLite Datenbank gespeichert.

## Pipelinestruktur:

### A Konfiguration
- Input Daten statisch oder live?
   - statisch: Pfad zu den Daten, Daten können direkt nacheinander verarbeitet werden
   - live: Daten werden kontinuierlich empfangen. Diese müssen zwischengespeichert werden. In vorgegebenen Intervallen müssen die Daten verarbeitet werden.

### B Supervisor
- verschiedene Supervisor für jeweils ausgewählte Konfigurationen
	- Static: vorhandene Bilder werden durch die Pipeline geleitet
	- Live: kontinuierlich empfangene Bilder werden durch die Pipeline geleitet


## Ideen für weitere Supervisoren:

- Supervisor, der ohne festes Intervall, sequentiell Live Daten verarbeitet. Dabei könnte man mehrere Worker anlegen, um die Erkennung auf mehrere Threads aufzuteilen.