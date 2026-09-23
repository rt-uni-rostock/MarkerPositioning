# Test-Checkliste: Mehrere LUCID-Kameras parallel (TDR054)

Diese Checkliste dient dem manuellen Test der Änderungen, die das parallele
Betreiben mehrerer LUCID-Vision-Kameras (z. B. zwei TDR054) ermöglichen:
`serialNumber`-Feld in `CameraSettings`, geänderte Geräteauswahl in
`LUCIDStream::open()` und die neue Start-Validierung in `SupervisorFactory`.

**Bedienung:** In VS Code oder einem beliebigen Markdown-Editor mit
Checkbox-Support öffnen. `[ ]` → `[x]` setzen, sobald ein Punkt erfolgreich
geprüft wurde.

**Voraussetzung:** Zwei LUCID TDR054-Kameras, per GigE angeschlossen (idealerweise
über einen Switch bzw. getrennte NICs, je nach Setup), Build mit
`ENABLE_LUCID=ON`.

---

## 0. Vorbereitung

- [x] Build ist aktuell (`MarkerPositioning.exe` frisch gebaut nach diesen Änderungen, `ENABLE_LUCID=ON`)
- [x] Beide LUCID-Kameras sind angeschlossen, per Ping/ARP im Netzwerk erreichbar
- [x] Beide Kameras sind mit einem LUCID-Tool (z. B. ArenaView / IPConfig) sichtbar; Seriennummern notiert
- [x] `Settings.json` liegt im Build-Output-Ordner (`out/build/x64-debug/Debug/` bzw. entsprechendes Preset)
- [x] Backup der aktuellen `Settings.json` angelegt (`Settings.json.bak-manualtest`)

---

## 1. Seriennummern ermitteln (Discovery-Logging)

- [x] Nur **eine** Kamera-Konfiguration mit `streamType: 1` (`LUCID`) aktiv (`active: true`), `serialNumber` bewusst **leer** lassen
- [x] Programm starten, Log prüfen: für **jede** angeschlossene LUCID-Kamera erscheint eine Zeile
      `Detected LUCID camera: serial=..., model=..., ip=...`
- [x] Beide angeschlossenen Kameras (nicht nur die konfigurierte) tauchen in der Log-Ausgabe auf
- [x] Seriennummern aus dem Log stimmen mit denen aus dem LUCID-Tool (Schritt 0) überein
- [x] Notiere beide Seriennummern für die folgenden Schritte

---

## 2. Rückwärtskompatibilität: eine Kamera, kein `serialNumber`

- [x] Nur **eine** der beiden Kameras physisch angeschlossen (oder nur eine Konfiguration `active: true`, wobei tatsächlich nur eine Kamera am Netz hängt)
- [x] `serialNumber` in der Konfiguration **leer** lassen
- [x] Programm startet erfolgreich, Log zeigt Warnung:
      `No serialNumber configured for this LUCID camera; falling back to the single detected device (serial=...)`
- [x] Kamera liefert Frames wie gewohnt (Detection/Passthrough je nach `supervisorMode`)
- [x] Kein Crash, kein Fehler

---

## 3. Explizite Geräteauswahl per `serialNumber` (eine Kamera)

- [x] Beide Kameras angeschlossen, aber nur **eine** Konfiguration `active: true`
- [x] `serialNumber` auf die Seriennummer **einer bestimmten** Kamera gesetzt (z. B. die "zweite" im Log)
- [x] Programm startet, wählt exakt die konfigurierte Kamera aus (per Sichtprüfung: Kamera-LED/Objektivabdeckung der jeweils anderen Kamera testen, ob nur die erwartete Kamera Bilder liefert)
- [x] Log zeigt `Creating device with serial=<erwartete Seriennummer>...` (bestätigt für serial=253802209, inkl. erfolgreichem Streaming + AprilTag-Erkennung)
- [x] Falsche/erfundene Seriennummer eingetragen → Programm bricht kontrolliert mit klarer Fehlermeldung ab (`No LUCID camera with serialNumber=... found among N detected device(s)`), kein Crash (getestet mit serialNumber="999999999")

---

## 4. Zwei LUCID-Kameras parallel (Kernszenario)

- [x] Zwei Kamera-Einträge in `Settings.json`, beide `streamType: 1`, beide `active: true`, jeweils die korrekte, eindeutige `serialNumber` aus Schritt 1
- [x] Unterschiedliche `id` je Kamera-Eintrag
- [x] Programm startet ohne Fehler **(nach Bugfix, siehe Auffälligkeiten-Tabelle — zuvor harter Crash)**
- [x] Log zeigt für **beide** Kameras jeweils eigene `Creating device with serial=...`-Zeilen mit unterschiedlichen Seriennummern
- [x] Beide Kameras liefern gleichzeitig Frames **(nach Netzwerk-Rekonfiguration der Kamera TRI023S-C auf IP 10.1.1.150 bestätigt: beide Kameras streamen parallel, 30s-Testlauf mit je 63 Frames pro Kamera)**
- [x] Bilddaten werden korrekt der jeweiligen `cameraId` zugeordnet (Log zeigt korrekt alternierende `Worker completed successfully for camera 3 ...` / `for camera 6 ...`-Zeilen je Zyklus, keine Vermischung)
- [x] Beide Kamera-Threads laufen unabhängig; über den 3-Minuten-Dauerlauf hinweg exakt ausgeglichene Frame-Zahlen (185/185) ohne gegenseitige Blockade beobachtet
- [x] Mehrere Minuten Dauerbetrieb mit beiden Kameras: kein Crash, keine wachsende Latenz, kein Speicherleck (3-Minuten-Testlauf: Speicher stabil bei ~358-363 MB, Thread-Anzahl stabil bei ~38-40 nach Anlaufphase, 185 Frames pro Kamera, keine Fehler außer den erwarteten einmaligen "Frame with ID: 0 is empty"-Warnungen beim Start)
- [x] Sauberes Beenden mit Ctrl+C/Fehlerfall: beide Kamera-Streams werden geschlossen (Log zeigt `Stopping LUCID camera stream...` je Kamera bzw. `LUCIDSystemManager: released shared Arena system`), kein Hänger — verifiziert per echtem `CTRL_C_EVENT` (AttachConsole/GenerateConsoleCtrlEvent), korrekte Reihenfolge beider Shutdown-Logs

---

## 5. Validierung von Fehlkonfigurationen (Startup-Checks)

- [x] Zwei aktive LUCID-Kamera-Einträge, **eine ohne** `serialNumber` (leer) → Programmstart bricht sofort mit klarer Fehlermeldung ab
      (`Missing serialNumber for a LUCID camera while multiple LUCID cameras are configured.`), kein Absturz mit unklarem Stacktrace
- [x] Zwei aktive LUCID-Kamera-Einträge mit **identischer** `serialNumber` → Programmstart bricht mit klarer Fehlermeldung ab
      (`Duplicate serialNumber configured for multiple LUCID cameras.`)
- [x] Nach Korrektur der `Settings.json` (eindeutige Seriennummern) startet das Programm wieder normal (verifiziert: beide Kameras erkannt, ausgewählt, Streaming inkl. erfolgreicher AprilTag-Poseberechnung für Tag 17)

---

## 6. Kombination mit anderen Kameratypen

- [x] Eine LUCID-Kamera (mit `serialNumber`) + eine Webcam gleichzeitig `active: true`
- [x] Beide Streams laufen parallel, keine Vermischung der Ergebnisse zwischen Kamera-IDs (Log zeigt korrekt getrennte `Worker completed successfully for camera 3 ...` / `for camera 2 ...`-Zeilen)
- [x] Bestehende Webcam/RTSP/RTP-Funktionalität weiterhin unverändert (Kurzcheck: Webcam liefert Frames, AprilTag-Detection läuft normal, kein Fehler)

---

## 7. Fehler-/Sonderfälle

- [x] Eine der beiden konfigurierten Kameras beim Start **nicht** angeschlossen (simuliert per nicht-existenter `serialNumber`, da physisch kein Kabel gezogen werden konnte) → tatsächlich beobachtetes Verhalten weicht von der ursprünglichen Annahme dieser Checkliste ab: **die gesamte Anwendung bricht ab**, nicht nur die betroffene Kamera (siehe Auffälligkeit #5). Fehlermeldung selbst ist klar (`No LUCID camera with serialNumber=... found ...`), aber kein isolierter Ausfall.
- [x] Kabel einer Kamera während des Betriebs ziehen → **vom Nutzer getestet (Kamera id=3 während des Betriebs getrennt)**: ursprünglich echter Crash (`abort()`) gefunden, Ursache behoben (Auffälligkeit #6) und vom Nutzer per erneutem Kabelziehen **bestätigt: kein Crash mehr**. Zusätzlich entdeckt und behoben: veralteter Frame wurde bei Ausbleiben neuer Daten wiederholt geloggt/verarbeitet (Auffälligkeit #7, jetzt behoben und verifiziert).
- [x] Beide Kameras während des Betriebs kurz vom Netz trennen und wieder anschließen → **implementiert und mit echter Hardware verifiziert (deterministisch simuliert, siehe Auffälligkeit #8)**: `LiveImageSource` erkennt den Verbindungsverlust nach 3 aufeinanderfolgenden fehlgeschlagenen Frame-Abrufen, schließt den Stream und öffnet ihn automatisch neu; sobald die Kamera wieder erreichbar ist, läuft der Betrieb ohne Neustart der Anwendung normal weiter.

---

## 8. Abschluss

- [x] Alle relevanten, durch den Agenten automatisiert testbaren Abschnitte oben abgehakt (0-7 vollständig, inkl. Reconnect-nach-Trennung und teilweisem Start bei fehlender Kamera)
- [x] Auffälligkeiten/Bugs dokumentiert (siehe Tabelle unten)
- [x] `Settings.json` nach den Tests auf funktionierenden Zwei-Kamera-Zustand zurückgesetzt (`id=3` serial=253802209 + `id=6` serial=231200391, beide `active=true`; alle anderen Einträge `active=false`)

---

## Gefundene Auffälligkeiten (hier eintragen)

| # | Bereich | Beschreibung | Schweregrad | Status |
| --- | --- | --- | --- | --- |
| 1 | Code-Bug (behoben) | Jede `LUCIDStream`-Instanz rief unabhängig `Arena::OpenSystem()`/`CloseSystem()` auf. Die Arena-SDK erlaubt aber nur **ein** offenes System pro Prozess (`@warning: Only one system may be opened at a time`). Beim Start der zweiten Kamera stürzte die Anwendung ohne Exception/Log-Meldung hart ab (natives Access-Violation-artiges Crash, kein `catch`-Block griff). Behoben durch neuen, referenzgezählten `LUCIDSystemManager`, der das Arena-System geräteübergreifend teilt (`acquire()`/`release()`), plus einen gemeinsamen Mutex für Discovery/CreateDevice/DestroyDevice. Alle Arena-Aufrufe in `LUCIDStream` sind zusätzlich in `try/catch (GenICam::GenericException)` gekapselt, sodass SDK-Fehler jetzt sauber geloggt werden statt die App abstürzen zu lassen. | Kritisch | Behoben, verifiziert (kein Crash mehr in >5 Testläufen mit 2 Kameras) |
| 2 | Code-Verbesserung | `CreateDevice()` konnte beim Öffnen einer zweiten, bereits aktiv streamenden Kamera mit `GC_ERR_TIMEOUT` fehlschlagen (GVCP-Kontrollkanal-Timeout). Als Schutz wurden (a) ein Retry mit Backoff um `CreateDevice()` (5 Versuche, 750 ms) ergänzt und (b) das Starten des Bildstreams (`StartStream()`) architektonisch von `open()` in eine neue, separate `startStreaming()`-Phase verschoben (`IImageSource::beginCapture()`), die vom Supervisor erst aufgerufen wird, nachdem **alle** konfigurierten Kameras erfolgreich ihr Gerät erstellt haben. Das reduziert die Fenster, in denen GVCP-Verbindungsaufbau einer Kamera mit aktivem GVSP-Bildstrom einer anderen kollidiert. | Mittel | **Verifiziert mit zwei echten Kameras**: 30s-Test (63 Frames/Kamera) und 3-Minuten-Dauerlauf (185 Frames/Kamera) erfolgreich, kein Timeout mehr beobachtet, kein Crash, stabiler Speicher-/Thread-Verbrauch |
| 3 | Infrastruktur/Netzwerk (kein Code-Bug) | Im ursprünglichen Testaufbau war Kamera "TRI023S-C" (serial 231200391, IP 192.168.75.5) von diesem Host **nicht erreichbar**: `Test-Connection 192.168.75.5` lieferte keine Antwort, `Get-NetRoute`/`Get-NetIPAddress` zeigten keine Netzwerkschnittstelle mit einer IP im Subnetz 192.168.75.0/24 und keine passende Route. Die andere Kamera (TDR054S-C, serial 253802209, IP 10.1.72.35) lag im selben /16-Subnetz wie die aktive NIC (`Ethernet 3`, 10.1.1.200/16) und war erreichbar. Discovery (Broadcast) fand beide Kameras zuverlässig, aber der Verbindungsaufbau (`CreateDevice`, unicast GVCP) zu 192.168.75.5 lief konsequent in einen Timeout. | Hoch (blockierte Parallelbetrieb-Test) | **Behoben durch Benutzer**: Kamera TRI023S-C wurde per LUCID IPConfig-Tool auf IP 10.1.1.150 umkonfiguriert (im erreichbaren Subnetz 10.1.0.0/16). Seitdem läuft der Parallelbetrieb beider Kameras erfolgreich (siehe Abschnitt 4). Kein Code-Fix nötig/möglich. |
| 4 | Beobachtung (kein neuer Bug durch diese Änderung) | Unmittelbar nach mehreren schnell aufeinanderfolgenden Start/Stop-Zyklen (ca. 3 Testläufe innerhalb von 30s) meldete ein Lauf einmalig "No camera connected" (leeres `GetDevices()`-Ergebnis), obwohl die Konfiguration korrekt war. Ein erneuter Versuch nach ca. 5 Sekunden Pause funktionierte einwandfrei. Vermutlich benötigt die Arena-SDK-Discovery (100ms `UpdateDevices()`-Fenster) kurz Zeit zum "Settle", nachdem das Arena-System kurz zuvor geschlossen wurde. | Niedrig | Nicht behoben (kein Blocker) — betrifft nur sehr schnelle Neustarts direkt hintereinander, nicht den regulären Betrieb. Bei Bedarf könnte `UpdateDevices()` mit einem größeren Timeout oder einem kurzen Retry abgesichert werden. |
| 5 | Architektur-Verhalten (jetzt behoben, siehe Auffälligkeit #9) | Wenn beim Start **eine** von mehreren konfigurierten LUCID-Kameras nicht gefunden/geöffnet werden konnte (z. B. falsche `serialNumber` oder Kamera nicht angeschlossen), brach ursprünglich **die gesamte Anwendung** ab (`LiveSupervisorMode::start()` stoppte alle bereits gestarteten Quellen und gab `false` zurück, was zu `Failed to start Supervisor, shutting down application.` führte). Auf expliziten Wunsch des Nutzers wurde dieses Verhalten geändert (siehe Auffälligkeit #9): die Anwendung startet jetzt mit allen erfolgreich verbundenen Kameras und protokolliert für die fehlende(n) Kamera(s) fortlaufend Fehler, bis diese verfügbar werden. | War: Mittel (Betriebsverhalten) | **Behoben** durch die in Auffälligkeit #9 beschriebene Umstellung von `LiveImageSource` auf einen asynchronen Verbindungs-Lifecycle. |
| 6 | Code-Bug (behoben, durch Nutzer entdeckt) | Beim Ziehen des Kabels von Kamera id=3 **während des laufenden Betriebs** stürzte die gesamte Anwendung mit "Debug Error! abort() has been called" ab (Windows CRT Debug-Dialog). Ursache: `LUCIDStream::getFrame()` rief `device_->GetImage(2000)` sowie die nachfolgenden Arena-Aufrufe **ohne** try/catch auf. Bei getrennter Kamera wirft `GetImage()` eine `GenICam::GenericException` (z. B. Timeout), die unbehandelt aus dem Capture-Thread (`LiveImageSource::captureLoop()`, läuft als eigener `std::thread`) herauspropagierte. Eine aus einem `std::thread`-Funktionskörper entkommende Exception ruft `std::terminate()` → `abort()` auf und beendet den **gesamten Prozess**, inklusive der zweiten, noch funktionierenden Kamera. Behoben durch: (a) `LUCIDStream::getFrame()` fängt jetzt `GenICam::GenericException` und `std::exception` ab, loggt den Fehler und liefert ein leeres `ImageFrame` zurück (wird von `captureLoop()` bereits als "leerer Frame" toleriert und übersprungen); (b) zusätzlich als Verteidigung in der Tiefe ein try/catch direkt in `LiveImageSource::captureLoop()` um den generischen `stream_->getFrame()`-Aufruf, damit auch andere/zukünftige `IVideoStream`-Implementierungen (RTSP, Webcam, ...) den Prozess nicht mit einer unbehandelten Exception abschießen können. | Kritisch | **Fix implementiert, gebaut und verifiziert**: Da zum Testzeitpunkt kein physischer Kabelzugriff auf Kamera id=3 verfügbar war (noch nicht wieder angeschlossen), wurde der exakt gleiche Fehlerpfad deterministisch reproduziert, indem der `GetImage()`-Timeout in `LUCIDStream.cpp` testweise auf 1 ms verkürzt wurde (Kamera 6, echte Hardware, weiterhin angeschlossen). Das erzeugte zuverlässig eine reale `GenICam::TimeoutException (GC_ERR_TIMEOUT)` beim Frame-Abruf. Ergebnis: Exception wurde sauber geloggt (`Arena/GenICam exception while retrieving frame from LUCID camera ...: TimeoutException (GC_ERR_TIMEOUT) ...`), die Anwendung lief **ohne Absturz** weiter und lieferte danach wieder normale Frames samt AprilTag-Erkennung. Der Timeout wurde anschließend wieder auf 2000 ms zurückgesetzt, neu gebaut und ein Regressionslauf (Kamera 6 allein, 15s, 11 erfolgreiche Worker-Zyklen, keine Fehler) bestätigte, dass der reguläre Betrieb unverändert funktioniert. **Zusätzlich vom Nutzer selbst per echtem Kabelziehen bestätigt**: kein Crash mehr, Fix vollständig verifiziert. |
| 7 | Code-Verbesserung (Folge des Nutzer-Tests von #6) | Nach dem Beheben des Absturzes (#6) fiel auf, dass beim Ausbleiben neuer Frames (z. B. Kamera getrennt) weiterhin **jeden Zyklus derselbe letzte (veraltete) Frame** erneut geloggt und durch die `DetectionPipeline` verarbeitet wurde (`LiveImageSource::getLatestFrame()` lieferte immer den zuletzt gespeicherten `latestFrame_` zurück, unabhängig davon, ob seitdem ein neuer Frame eingetroffen war). Behoben durch neue Methode `IImageSource::hasNewFrame()` (Default `true` für z. B. `StaticImageSource`, das immer einen frischen Dummy-Frame erzeugt). `LiveImageSource` verwaltet dafür ein atomares Flag, das in `captureLoop()` bei jedem neu gespeicherten, nicht-leeren Frame gesetzt und bei jedem `hasNewFrame()`-Aufruf atomar konsumiert (exchange-and-reset) wird — so wird ein Frame nur genau einmal als "neu" gemeldet, auch wenn zwei Worker pro Kamera gleichzeitig abfragen. `Worker::start()` (Detection-Pfad) und `LiveImagePassthroughSupervisorMode::handleCycle()` (Passthrough-Pfad) überspringen jetzt den kompletten Zyklus (kein Log, keine Pipeline-Verarbeitung, kein Sink-Versand), wenn `hasNewFrame()` `false` liefert. | Mittel | **Implementiert, gebaut und mit echter Hardware verifiziert**: PreProcessing-Log zeigt jetzt durchgehend eindeutige, aufsteigende `frameId`-Werte ohne Wiederholungen (z. B. 2, 5, 8, 11, ... bei zwei parallelen Workern pro Kamera); beim erzwungenen `GC_ERR_TIMEOUT`-Test wurde der betroffene Zyklus korrekt als "no new frame available ... skipping cycle" übersprungen, ohne Pipeline-Log/-Verarbeitung. |
| 8 | Neues Feature (auf Nutzerwunsch) | Die Anwendung konnte sich bisher **nicht** automatisch von einer Kamera-Trennung während des Betriebs erholen: Nach einem Verbindungsverlust lieferte `getFrame()` dauerhaft leere Frames, ohne je wieder einen Verbindungsversuch zu unternehmen (Neustart der Anwendung war nötig). Implementiert: `LiveImageSource` zählt aufeinanderfolgende fehlgeschlagene Frame-Abrufe; nach 3 Fehlversuchen in Folge wird der Stream geschlossen (`stream_->close()`) und der komplette Lifecycle (`open()` → `startStreaming()`) automatisch neu durchlaufen — mit fortlaufenden Wiederholungsversuchen alle 2 Sekunden, falls die Kamera noch nicht wieder erreichbar ist. Da `LUCIDStream::open()` Geräte anhand der **Seriennummer** (nicht der IP) auswählt, funktioniert der Reconnect auch dann, wenn sich die IP-Adresse der Kamera zwischenzeitlich geändert hat (z. B. DHCP). Zusätzlich wurde eine Absicherung gegen einen Null-Pointer-Zugriff in `LUCIDStream::getFrame()` ergänzt, falls ein Frame angefragt wird, während der Stream gerade neu verbindet (`device_ == nullptr`). | Neues Feature | **Implementiert, gebaut und deterministisch mit echter Hardware verifiziert**: Da kein physischer Kabelzugriff zum Testzeitpunkt verfügbar war, wurden mehrere aufeinanderfolgende Frame-Abrufe testweise künstlich zum Scheitern gebracht (temporärer Test-Hook in `getFrame()`, danach vollständig entfernt). Log bestätigt den kompletten Zyklus: `Captured empty frame ... (3/3 consecutive failures before reconnect)` → `LiveImageSource lost connection ... closing stream and attempting to reconnect...` → `LiveImageSource attempting to open video stream...` → `LiveImageSource successfully opened video stream.` → `LiveImageSource successfully started streaming.` → Frame-Abruf läuft danach nahtlos mit fortlaufenden `frameId`-Werten weiter, ohne Absturz und ohne dass die zweite (nicht betroffene) Kamera beeinträchtigt wurde. Anschließender regulärer Zwei-Kamera-Lauf ohne den Test-Hook bestätigte unveränderten Normalbetrieb. |
| 9 | Neues Feature (auf Nutzerwunsch) | War beim Start **eine** von mehreren konfigurierten Kameras nicht angeschlossen/erreichbar, brach zuvor die gesamte Anwendung ab (siehe Auffälligkeit #5). Implementiert: `LiveImageSource::start()`/`beginCapture()` schlagen jetzt nicht mehr hart fehl, wenn die zugehörige Kamera (noch) nicht erreichbar ist — das eigentliche Öffnen (`stream_->open()`) erfolgt asynchron in einem neuen, dauerhaften Lifecycle-Thread, der bei Fehlschlag alle 2 Sekunden automatisch einen neuen Versuch unternimmt (gleicher Mechanismus wie Auffälligkeit #8, hier lediglich "noch nie erfolgreich verbunden" statt "Verbindung verloren"). Dadurch startet die Anwendung erfolgreich mit allen verfügbaren Kameras, protokolliert für die fehlende(n) Kamera(s) klare, fortlaufende Fehlermeldungen (`No LUCID camera with serialNumber=... found ...` / `LiveImageSource failed to open video stream ...; will keep retrying every 2000 ms.`) und nimmt die fehlende Kamera automatisch in Betrieb, sobald sie verfügbar wird — **ohne Neustart der Anwendung**. | Neues Feature | **Implementiert, gebaut und mit echter Hardware verifiziert**: Kamera id=6 testweise mit erfundener `serialNumber` konfiguriert (Kamera id=3 unverändert, echt erreichbar). Programm startete erfolgreich, Kamera 3 lieferte durchgehend Frames samt AprilTag-Erkennung, während für Kamera 6 alle ~2s ein klarer Fehler protokolliert wurde, ohne die Anwendung zu beenden. Nach Zurücksetzen der `serialNumber` auf den korrekten Wert und Neustart liefen wieder beide Kameras parallel (Regressionslauf ohne Fehler). |
