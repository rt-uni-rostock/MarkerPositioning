# Test-Checkliste MarkerPositioning

Diese Checkliste dient dem umfassenden Regressionstest nach den Änderungen an `Supervisor`, `IO/Sink` (UDP-Passthrough) und `Settings.json`. Ziel: sicherstellen, dass bestehende Funktionalität (Live-Detection, statische Kameras, GigE/LUCID, RTSP/RTP) nicht durch die neuen Änderungen beeinträchtigt wurde.

**Bedienung:** Diese Datei kann in VS Code (oder jedem Markdown-Editor mit Checkbox-Support) geöffnet werden. Klicke auf `[ ]`, um es zu `[x]` zu ändern, oder editiere den Text direkt (`[ ]` → `[x]`).

---

## 0. Vorbereitung

- [x] Build ist aktuell (`MarkerPositioning.exe` frisch gebaut nach allen Änderungen)
- [x] `Settings.json` liegt im Build-Output-Ordner (`out/build/x64-debug/Debug/`)
- [x] Backup der aktuellen `Settings.json` gemacht (falls Testkonfigurationen überschrieben werden)
- [x] Log-Verzeichnis (`imageOutputPath`) ist leer/bekannt, um neue Dateien leicht zu erkennen
- [x] LUCID-Vision-Kamera ist angeschlossen und im Netzwerk erreichbar (Ping-Test)
- [x] Webcam ist angeschlossen und von Windows erkannt (Kamera-App testet kurz)

---

## 1. Grundfunktion: Programmstart & Settings

- [x] Programm startet ohne Absturz mit unveränderter `Settings.json`
- [x] Log-Ausgabe zeigt `Settings loaded successfully.`
- [x] Log-Ausgabe zeigt korrekt geladene Werte (`supervisorMode`, `sourceMode`, `udpIp`, `udpPort`, `frameRate` — **ohne** `imageUdpMaxPayloadBytes`, da entfernt)
- [x] Programm beendet sich sauber mit Ctrl+C (kein Hänger, kein Crash)
- [x] Fehlerhafte/fehlende `Settings.json` erzeugt sinnvolle Fehlermeldung (nicht getestet? optional)

---

## 2. LiveSupervisorMode (`supervisorMode: 0`) — Detection + Sink

### 2.1 Mit Webcam (`streamType: 4`, cameraId ggf. 2)
- [x] `active: true` nur bei Webcam-Kamera gesetzt, restliche Kameras `false`
- [x] Supervisor startet, Webcam öffnet erfolgreich (Log: "Webcam stream opened successfully")
- [x] AprilTag wird bei Sichtkontakt erkannt (Tag mit passender `tagID`/`tagType` verwenden)
- [x] Pose-Ergebnis wird per UDP gesendet (mit `../Tests/UdpReceiver.py` — ein eigenständiges Tool, das außerhalb des Repos im Ordner `../Tests/` liegt und UDP-MarkerMessage-Pakete empfängt und protokolliert)
- [x] SQLite-Datenbank wird geschrieben/aktualisiert (Pfad prüfen, z.B. via DB-Browser)
- [x] `imageLogOptions.saveDetectionResults: true` → Ergebnis-Bilder werden unter `imageOutputPath` gespeichert
- [x] `imageLogOptions.visualizeAllDetections: true` → Bounding-Box/Marker-ID sichtbar im gespeicherten Bild
- [x] `imageLogOptions.saveRawFrames: true` → unbearbeitete Frames werden zusätzlich gespeichert
- [x] `imageLogOptions.saveGrayFrames: true` → Graustufen-Frames werden zusätzlich gespeichert
- [ ] Kein Marker im Bild → keine falschen Positiv-Erkennungen, Programm läuft weiter stabil
- [x] Mehrere Minuten Laufzeit ohne Speicherleck/Absturz/wachsende Latenz

### 2.2 Mit LUCID-Vision-Kamera (`streamType: 1`)
- [x] Nur LUCID-Kamera `active: true`, andere `false`
- [x] Supervisor erkennt und öffnet GigE-Kamera (Log-Meldung prüfen)
- [x] Kamera liefert Frames mit erwarteter Auflösung (kein Schwarzbild/Timeout)
- [x] AprilTag-Erkennung funktioniert wie bei Webcam (Tag sichtbar → erkannt)
- [x] UDP-Ergebnisse werden gesendet
- [x] SQLite-Ergebnisse werden geschrieben
- [x] Image-Logging funktioniert identisch zu 2.1
- [ ] Kamera-Trennung (Kabel ziehen) wird sauber behandelt (kein Absturz, sinnvolle Fehlermeldung/Reconnect-Versuch)
- [x] Mehrere Minuten Laufzeit stabil

### 2.3 Mehrere aktive Kameras gleichzeitig
- [x] Webcam UND LUCID-Kamera gleichzeitig `active: true`
- [x] Beide Streams werden parallel verarbeitet (Log zeigt beide Kamera-IDs)
- [x] Keine Zuordnungsfehler zwischen Kamera-ID und Ergebnis (Pose/Bild passt zur richtigen Kamera)
- [x] Performance bleibt akzeptabel (keine deutlich höhere Latenz als bei Einzelkamera)

---

## 3. LiveImagePassthroughSupervisorMode (`supervisorMode: 2`) — Neue Funktion

### 3.1 Mit Webcam
- [x] Nur Webcam `active: true`
- [x] Supervisor startet, sendet Frames per UDP (Log: "Image frame sent via UDP")
- [x] Empfänger (`../Tests/UdpReceiver.py` — Python-Tool im Ordner `../Tests/` — oder MATLAB-äquivalent) empfängt Frames vollständig
- [x] Bilder sind farblich korrekt (kein Blau-/Lila-Stich)
- [x] Latenz liegt im erwarteten Bereich (~700-1100ms bei 1080p, abhängig von `frameRate`)
- [x] Kein bzw. minimaler Paketverlust (<5% nicht gespeicherter Frames über mehrere Minuten)
- [x] Frame-Reihenfolge stimmt (FrameId aufsteigend, keine Duplikate)

### 3.2 Mit LUCID-Vision-Kamera
- [x] Nur LUCID-Kamera `active: true`
- [x] Passthrough funktioniert analog zu 3.1 (Frames kommen an, Farben korrekt, Latenz akzeptabel)
- [x] Funktioniert bei höherer/anderer Auflösung als Webcam (LUCID-Sensorauflösung prüfen)

### 3.3 Langzeit-/Stresstest
- [x] 5+ Minuten Dauerbetrieb ohne wachsenden Backlog (Latenz bleibt stabil, wächst nicht kontinuierlich)
- [x] Programm per Ctrl+C sauber beendbar, auch während aktiver UDP-Übertragung

---

## 4. StaticSupervisorMode (`supervisorMode: 1`)

- [x] **Hinweis:** Laut Projektstand nicht implementiert — Test nur relevant, falls versehentlich Code-Pfad ausgelöst wird
- [x] Programm crasht nicht, wenn `supervisorMode: 1` gesetzt wird (ggf. erwartete "not implemented"-Meldung/Exception, aber kontrolliert statt hartem Absturz)

---

## 5. Settings.json Regressionstests

- [x] `supervisorMode` 0, 1, 2 jeweils einzeln durchprobiert (Verhalten wie erwartet, siehe oben)
- [x] `frameRate` geändert (z.B. 1 → 10 → 30) — Verarbeitungsrate ändert sich sichtbar in Logs/Ergebnissen
- [ ] `tagType`, `tagSize`, `tagID` geändert — nur passender Marker wird erkannt, andere ignoriert
- [ ] `quadDecimate` geändert (z.B. 1.0 vs. 4.0) — Erkennungsgenauigkeit/Performance ändert sich plausibel
- [ ] `udpIp`/`udpPort` geändert — Pakete kommen an neuer Adresse/Port an, alte Adresse erhält nichts mehr
- [ ] `enableImageLogging: false` — keine neuen Dateien im `imageOutputPath` trotz laufender Erkennung
- [ ] `cameras[].active` Kombinationen (keine aktiv, eine aktiv, alle aktiv) verhalten sich wie erwartet (keine aktive Kamera → sinnvolle Meldung statt Crash)
- [ ] Fehlender/falscher Wert in `Settings.json` (z.B. `tagSize` fehlt) → Default wird genutzt, kein Crash
- [ ] `Settings_Webcam.json` und `Settings_MUM.json` (alte Referenz-Configs) — prüfen ob diese noch mit aktueller `GeneralSettings`-Struktur kompatibel sind oder Migration brauchen

---

## 6. Cross-Check: Alte vs. neue Funktionalität

- [ ] Vor den Änderungen funktionierende Szenarien (falls dokumentiert/erinnerlich) erneut testen und Ergebnis vergleichen
- [ ] `README.md` und `SETTINGS.md` Anweisungen befolgt → Ergebnis entspricht der Dokumentation
- [x] Kein Absturz beim Wechsel zwischen den drei Supervisor-Modi (Settings ändern, Neustart, wiederholen)
- [ ] Speicherverbrauch bleibt über die Zeit stabil (Task-Manager, kein kontinuierliches Wachstum)
- [ ] CPU-Auslastung ist plausibel (keine Vollauslastung eines Kerns im Leerlauf)

---

## 7. Abschluss

- [ ] Alle relevanten Abschnitte oben abgehakt
- [ ] Auffälligkeiten/Bugs dokumentiert (siehe Abschnitt unten)
- [ ] `Settings.json` nach Tests wieder auf gewünschten Produktivstand zurückgesetzt

---

## Gefundene Auffälligkeiten (hier eintragen)

| # | Bereich | Beschreibung | Schweregrad | Status |
|---|---------|-------------|-------------|--------|
|   |         |             |             |        |
|   |         |             |             |        |

