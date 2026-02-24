# Supervisor

## Allgemein:

- die Pipeline soll verwaltet werden
- einzelne Pipelineschritte sollen konfiguriert werden

## Pipelinestruktur:

### A Konfiguration
- Input Daten statisch oder live?
   - statisch: Pfad zu den Daten, Daten können direkt nacheinander verarbeitet werden
   - live: Daten werden kontinuierlich empfangen. Diese müssen zwischengespeichert werden. In vorgegebenen Intervallen müssen die Daten verarbeitet werden.

### B Supervisor
- verschiedene Supervisor für jeweils ausgewählte Konfigurationen
	- Static: vorhandene Bilder werden durch die Pipeline geleitet
	- Live: kontinuierlich empfangene Bilder werden durch die Pipeline geleitet