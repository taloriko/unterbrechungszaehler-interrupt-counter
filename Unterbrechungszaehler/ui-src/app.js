(() => {
  'use strict';

  const $ = (selector, root = document) => root.querySelector(selector);
  const el = (tag, className) => {
    const node = document.createElement(tag);
    if (className) node.className = className;
    return node;
  };
  const icon = (name, className = '') => {
    const svg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
    svg.classList.add('icon');
    if (className) svg.classList.add(className);
    svg.setAttribute('aria-hidden', 'true');
    const use = document.createElementNS('http://www.w3.org/2000/svg', 'use');
    use.setAttribute('href', `#i-${name}`);
    svg.append(use);
    return svg;
  };

  const I18N = {
    de: {
      'project.displayEnabled': 'Display',
      'analytics.metric': 'Metrik',
      'analytics.metric.count': 'Anzahl',
      'analytics.metric.averageInterval': 'Ø Abstand',
      'analytics.intervalSamples': '{n} Abstände',
      'analytics.coveragePartial': 'Ø-Abstand basiert auf den noch vorhandenen Rohereignissen.',
      'nav.home': 'Home', 'nav.analytics': 'Auswertung', 'nav.device': 'Gerät', 'nav.settings': 'Einstellungen',
      'view.analytics.title': 'Auswertung', 'view.analytics.desc': 'Heatmaps wahlweise als Anzahl oder durchschnittlicher abgeschlossener Zeitabstand zwischen Unterbrechungen.',
      'interruptions.title': 'Unterbrechungszähler', 'interruptions.desc': 'Jeder Tastendruck oder Klick wird sofort als Unterbrechung erfasst.', 'interruptions.today': 'Unterbrechungen heute', 'interruptions.last': 'Letzte Unterbrechung', 'interruptions.button': 'Unterbrechung', 'interruptions.sound': 'Ton bei Unterbrechung', 'interruptions.soundOn': 'Ein', 'interruptions.soundOff': 'Aus', 'interruptions.pending': 'Wird gespeichert …', 'interruptions.dropped': '{n} Unterbrechung(en) konnten in diesem Lauf nicht dauerhaft gespeichert werden.', 'interruptions.captureFailed': 'Unterbrechung konnte nicht erfasst werden.', 'interruptions.justNow': 'gerade eben', 'interruptions.never': 'noch keine', 'interruptions.ageUnknown': 'Zeitabstand nicht bestimmbar', 'interruptions.agoSeconds': 'vor {n} Sek.', 'interruptions.agoMinutes': 'vor {n} Min.', 'interruptions.agoHours': 'vor {n} Std.', 'interruptions.agoDays': 'vor {n} Tagen',
      'project.settings.title': 'Feedback & Display', 'project.settings.desc': 'Gerätebezogene Rückmeldung für Unterbrechungen und die lokale OLED-Anzeige. Änderungen gelten sofort und bleiben im ESP32 gespeichert.', 'project.soundMode': 'Unterbrechungston', 'project.soundMode.fixed': 'Fester Track', 'project.soundMode.rotate': 'Wechselnd – jedes Mal nächster Track', 'project.soundTrack': 'Track bei festem Ton', 'project.soundTrackHint': 'Track 1 bleibt ausschließlich für den Boot-Ton reserviert.', 'project.soundTracksAvailable': 'Verfügbare Tracks: {n}. Wechselnd verwendet Track 2 bis {n}.', 'project.soundTracksUnknown': 'Trackanzahl nicht bekannt; im Wechselmodus dient der feste Track als Fallback.', 'project.displayFlash': 'Display bei Unterbrechung aufblitzen', 'project.displayMode': 'Display-Anzeige', 'project.displayMode.standard': 'Standard: Heute + letzte Unterbrechung', 'project.displayMode.count': 'Nur Zahl – maximal groß', 'project.displayMode.last': 'Nur letzte Unterbrechung – maximal groß', 'project.displayBrightness': 'Display-Helligkeit', 'project.displayDimAfter': 'Dimmen nach', 'project.displayDimBrightness': 'Helligkeit gedimmt', 'project.minutes': 'Minuten', 'project.dimDisabled': '0 = Dimmer aus', 'project.preferenceError': 'Einstellung konnte nicht gespeichert werden.',
      'event.source.physical_button': 'Taster', 'event.source.web_button': 'Web', 'event.source.software': 'Software', 'event.source.api': 'API', 'event.source.hardware': 'Hardware', 'event.source.unknown': 'Unbekannt',
      'analytics.hourly.title': 'Wochentage / Stunden', 'analytics.hourly.desc': 'Wochentage und Stunden – wahlweise Anzahl oder Ø Abstand bis zur nächsten Unterbrechung am selben Tag.', 'analytics.monthWeek.title': 'Monate / Kalenderwochen', 'analytics.monthWeek.desc': 'Monate und ISO-Kalenderwochen – wahlweise Anzahl oder Ø Abstand.', 'analytics.yearMonth.title': 'Letzte 5 Jahre / Monate', 'analytics.yearMonth.desc': 'Letzte fünf Kalenderjahre nach Monat – wahlweise Anzahl oder Ø Abstand.', 'analytics.storage.title': 'Daten & Export', 'analytics.storage.desc': 'Rohereignisse liegen binär im Ringspeicher; CSV wird erst beim Download erzeugt.', 'analytics.mode': 'Auswahl', 'analytics.mode.week': 'Kalenderwoche', 'analytics.mode.range': 'Von / Bis', 'analytics.year': 'Jahr', 'analytics.week': 'Kalenderwoche', 'analytics.from': 'Von', 'analytics.to': 'Bis', 'analytics.load': 'Anzeigen', 'analytics.download': 'CSV herunterladen', 'analytics.rawEvents': 'Rohereignisse', 'analytics.dailyRecords': 'Tagesaggregate', 'analytics.storageUsed': 'Dateisystem verwendet', 'analytics.unassigned': 'Ohne Kalenderzeit', 'analytics.dropped': 'Nicht dauerhaft gespeichert (dieser Lauf)', 'analytics.recovering': 'Datenwiederherstellung läuft', 'analytics.noData': 'Keine Daten im gewählten Zeitraum.', 'analytics.loadError': 'Auswertung konnte nicht geladen werden.', 'analytics.ringHint': 'Bei vollem Ringspeicher werden die ältesten Rohereignisse überschrieben; Tagesaggregate bleiben separat erhalten.',
      'view.home.title': 'Home',
      'view.device.title': 'Gerät', 'view.device.desc': 'Allgemeine Informationen des ESP32 und seiner aktuellen Verbindung.',
      'view.settings.title': 'Einstellungen', 'view.settings.desc': 'Browserbezogene Darstellungseinstellungen ohne ESP32-Flash-Schreibzugriffe.',
      'card.device': 'Gerät', 'card.device.desc': 'Reale allgemeine Hardware- und Firmwareinformationen.',
      'card.wifi': 'WLAN', 'card.wifi.desc': 'Verbindungsstatus, Netzwerkadresse und Signalqualität.',
      'card.memory': 'Speicher', 'card.memory.desc': 'Interner Heap des ESP32. PSRAM wird separat nur angezeigt, wenn vorhanden.',
      'card.ota': 'Firmware-Update', 'card.ota.desc': 'Neue Firmware als .bin direkt über den Browser installieren. WLAN-Zugangsdaten bleiben erhalten.',
      'card.hardware': 'Hardware', 'card.hardware.desc': 'Optionale Hardwaremodule, ihr letzter bestätigter Zustand und die beim Check verfügbaren Informationen.',
      'card.time': 'Zeitverwaltung', 'card.time.desc': 'Zentrale Zeitquelle für System und spätere Ereignislogs. NTP führt, RTC und Browser dienen als Fallback.',
      'card.language': 'Sprache', 'card.language.desc': 'Die Änderung wird sofort angewendet und nur in diesem Browser gespeichert.',
      'card.theme': 'Darstellung', 'card.theme.desc': 'System, Hell oder Dunkel – ohne zusätzlichen Speichern-Button.',
      'label.firmware': 'Firmware', 'label.version': 'Version', 'label.uptime': 'Uptime', 'label.board': 'Board', 'label.chip': 'Chip', 'label.cores': 'CPU-Kerne', 'label.flash': 'Flash', 'label.psram': 'PSRAM',
      'label.network': 'Netzwerk', 'label.ip': 'IP-Adresse', 'label.signal': 'Signal', 'label.heapTotal': 'Heap gesamt', 'label.heapFree': 'Heap frei', 'label.heapMin': 'Minimum frei', 'label.heapUsed': 'Heap verwendet',
      'label.language': 'Sprache', 'label.theme': 'Theme', 'label.firmwareImage': 'Aktuelles Firmware-Image', 'label.otaCapacity': 'OTA-Speicher', 'label.projectReserve': 'Reserve für Projektcode',
      'status.wifi': 'WLAN', 'status.api': 'Geräte-API', 'status.gpio': 'GPIO', 'status.rtc': 'RTC', 'status.display': 'Display', 'status.audio': 'Audio', 'status.time': 'Zeit', 'status.data': 'Datenspeicher', 'status.connected': 'Verbunden', 'status.disconnected': 'Getrennt', 'status.ap': 'Access Point', 'status.ok': 'OK', 'status.warning': 'Warnung', 'status.error': 'Fehler', 'status.inactive': 'Inaktiv', 'status.disabled': 'Deaktiviert', 'status.checking': 'Wird geprüft', 'status.no_response': 'Keine Antwort', 'status.unknown': 'Unbekannt', 'status.busy': 'Wird geladen', 'status.stale': 'Veraltet',
      'quality.good': 'Gut', 'quality.normal': 'Normal', 'quality.notice': 'Hinweis', 'quality.warning': 'Warnung', 'quality.critical': 'Kritisch', 'quality.unknown': 'Keine Daten',
      'theme.system': 'Automatisch / System', 'theme.light': 'Hell', 'theme.dark': 'Dunkel',
      'lang.de': 'Deutsch', 'lang.en': 'Englisch', 'lang.swg': 'Schwäbisch', 'lang.auto': 'Automatisch aus Browsersprache erkannt', 'lang.manual': 'Manuell ausgewählt',
      'action.refresh': 'Aktualisieren', 'action.retry': 'Erneut versuchen', 'action.otaUpload': 'Firmware installieren', 'action.hardwareCheckAll': 'Alle prüfen', 'action.hardwareCheck': 'Prüfen', 'action.audioTest': 'Ton testen', 'action.displayTest': 'Display testen',
      'ota.file': 'Firmware-Datei (.bin)',
      'ota.idle': 'Noch keine Firmware ausgewählt.', 'ota.ready': 'Bereit zum Update.', 'ota.uploading': 'Firmware wird übertragen und in die inaktive OTA-Partition geschrieben.', 'ota.verifying': 'Übertragung abgeschlossen. Firmware wird geprüft und aktiviert.',
      'ota.success': 'Update erfolgreich. Das Gerät startet neu.', 'ota.reconnecting': 'Warte auf den Neustart …', 'ota.reconnected': 'Gerät ist wieder erreichbar. Seite wird neu geladen.', 'ota.reconnectFailed': 'Neustart wurde ausgelöst. Falls die Seite nicht zurückkommt, neue IP in der seriellen Ausgabe prüfen.',
      'ota.fileTooLarge': 'Die gewählte Firmware ist größer als der verfügbare OTA-Speicher.', 'ota.invalidFile': 'Bitte eine kompilierte ESP32-Firmwaredatei mit der Endung .bin auswählen.', 'ota.networkError': 'Die Verbindung ist während des Updates abgebrochen. Die bisherige Firmware bleibt aktiv, sofern der ESP32 das Update nicht erfolgreich abgeschlossen hat.', 'ota.failedPrefix': 'Update fehlgeschlagen',
      'ota.stage.start': 'Start', 'ota.stage.write': 'Flash schreiben', 'ota.stage.finish': 'Prüfen/Aktivieren', 'ota.stage.aborted': 'Übertragung', 'ota.stage.request': 'Anfrage', 'ota.stage.unknown': 'Unbekannte Phase',
      'ota.reason.write': 'Schreiben in den Flash ist fehlgeschlagen.', 'ota.reason.erase': 'Löschen des Zielbereichs im Flash ist fehlgeschlagen.', 'ota.reason.read': 'Die neue Firmware konnte nicht zuverlässig aus dem Flash gelesen werden.', 'ota.reason.space': 'Die Firmware ist zu groß für die OTA-Partition.', 'ota.reason.size': 'Die übertragene Dateigröße ist ungültig oder unvollständig.', 'ota.reason.stream': 'Die Datenübertragung ist abgebrochen oder in ein Timeout gelaufen.', 'ota.reason.checksum': 'Die Firmware-Prüfsumme stimmt nicht.', 'ota.reason.image': 'Die Datei ist kein gültiges ESP32-Firmware-Image.', 'ota.reason.activate': 'Die neue Firmware konnte nicht als startfähig aktiviert werden.', 'ota.reason.partition': 'Es wurde keine geeignete zweite OTA-App-Partition gefunden.', 'ota.reason.argument': 'Die OTA-Anfrage enthält ungültige Daten.', 'ota.reason.aborted': 'Der Upload wurde vom Browser oder Netzwerk abgebrochen.', 'ota.reason.verify': 'Die Firmwareprüfung ist fehlgeschlagen.', 'ota.reason.unknown': 'Unbekannter OTA-Fehler.',
      'api.unavailable': 'Die Geräte-API ist derzeit nicht erreichbar. Bereits geladene Werte bleiben sichtbar und werden nicht als bestätigt ausgegeben.',
      'hardware.gpio': 'Digitale Ein-/Ausgänge', 'hardware.rtc': 'DS3231 RTC', 'hardware.display': 'SH1106 OLED', 'hardware.audio': 'DY-SV17F Sound',
      'hardware.info.inputs': 'DI-Pins', 'hardware.info.outputs': 'DO-Pins', 'hardware.info.model': 'Modell', 'hardware.info.transport': 'Schnittstelle', 'hardware.info.address': 'Adresse', 'hardware.info.rtcTime': 'RTC-Zeit', 'hardware.info.temperature': 'Temperatur', 'hardware.info.osf': 'Oszillator-Stop-Flag', 'hardware.info.resolution': 'Auflösung', 'hardware.info.initialized': 'Initialisiert', 'hardware.info.pins': 'Pins', 'hardware.info.playState': 'Wiedergabe', 'hardware.info.onlineDevices': 'Datenträgerstatus', 'hardware.info.fileCount': 'Dateien', 'hardware.info.busy': 'BUSY aktiv', 'hardware.info.lastCheck': 'Prüfung bei Uptime', 'hardware.info.feedback': 'Rückmeldung', 'hardware.info.testTrack': 'Test-/Boot-Ton',
      'hardware.feedback.none': 'Keine', 'hardware.feedback.local_state': 'Lokaler ESP32-Zustand', 'hardware.feedback.transport_ack': 'Bus/Transport bestätigt', 'hardware.feedback.protocol_response': 'Protokollantwort', 'hardware.feedback.external_feedback': 'Externe Rückmeldung',
      'hardware.play.stopped': 'Gestoppt', 'hardware.play.playing': 'Spielt', 'hardware.play.paused': 'Pausiert', 'hardware.play.unknown': 'Unbekannt', 'hardware.check.pending': 'Prüfung läuft …', 'hardware.check.done': 'Hardwareprüfung abgeschlossen.', 'hardware.check.timeout': 'Die Hardwareprüfung läuft länger als erwartet. Der letzte bekannte Zustand bleibt sichtbar.', 'hardware.action.failed': 'Die Hardwareaktion konnte nicht ausgeführt werden.',
      'time.priority.ntp.title': '1. NTP', 'time.priority.ntp.desc': 'Referenz / höchste Priorität',
      'time.priority.rtc.title': '2. RTC', 'time.priority.rtc.desc': 'Startzeit / autark',
      'time.priority.browser.title': '3. Browser', 'time.priority.browser.desc': 'Fallback',
      'time.priority.relative.title': '4. Ohne Zeit', 'time.priority.relative.desc': 'nur relativ',
      'time.activeSource': 'Aktive Zeitquelle', 'time.systemTime': 'Systemzeit', 'time.timeStatus': 'Zeitzustand', 'time.lastCheck': 'Letzte Prüfung',
      'time.source.ntp': 'NTP', 'time.source.rtc': 'RTC', 'time.source.browser': 'Browser', 'time.source.relative': 'Nur relativ', 'time.source.none': 'Keine',
      'time.quality.reference': 'Referenz', 'time.quality.valid': 'Gültig', 'time.quality.fallback': 'Fallback', 'time.quality.relative': 'Keine absolute Zeit', 'time.quality.none': 'Unbekannt',
      'time.ntp.title': 'NTP', 'time.ntp.server': 'Primärer NTP-Server', 'time.ntp.checkSave': 'Prüfen & speichern', 'time.ntp.hint': 'Der eingetragene primäre NTP-Server wird geprüft und nur bei gültiger Antwort dauerhaft gespeichert.',
      'time.browser.title': 'Browser-Fallback', 'time.browser.hint': 'Nur wenn keine gültige NTP- oder RTC-Zeit vorhanden ist, darf der Browser einmalig seine Uhrzeit übertragen.',
      'time.differences.title': 'Zeitquellen & Differenzen', 'time.reference': 'Referenz', 'time.delta': 'Differenz', 'time.sample.ntp': 'NTP', 'time.sample.rtc': 'RTC', 'time.sample.rtcBefore': 'RTC vor NTP-Abgleich', 'time.sample.system': 'System', 'time.sample.browser': 'Browser',
      'time.rtcSync': 'RTC-Nachführung', 'time.rtcSync.ok': 'Erfolgreich aus NTP nachgeführt', 'time.rtcSync.failed': 'Nachführung fehlgeschlagen', 'time.rtcSync.none': 'Bei diesem Check nicht erforderlich',
      'time.check': 'Zeit prüfen', 'time.checking': 'Zeitquellen werden geprüft …', 'time.checked': 'Zeitprüfung abgeschlossen.', 'time.ntpChecking': 'NTP-Server wird geprüft …', 'time.ntpSaved': 'NTP-Server geprüft, gespeichert und als Referenz übernommen.',
      'time.browserAccepted': 'Browserzeit wurde als Fallback übernommen.', 'time.unavailable': 'Nicht verfügbar', 'time.invalid': 'Ungültig', 'time.valid': 'Gültig',
      'time.error.no_network': 'Kein verbundenes WLAN für NTP.', 'time.error.invalid_server': 'Der NTP-Server ist ungültig.', 'time.error.dns_error': 'Der NTP-Server konnte nicht aufgelöst werden.', 'time.error.socket_error': 'UDP für die NTP-Abfrage konnte nicht gestartet werden.', 'time.error.send_error': 'Die NTP-Anfrage konnte nicht gesendet werden.', 'time.error.timeout': 'Der NTP-Server hat nicht rechtzeitig geantwortet.', 'time.error.invalid_response': 'Die NTP-Antwort ist ungültig.', 'time.error.server_unsynchronized': 'Der NTP-Server meldet sich selbst als nicht synchronisiert.', 'time.error.implausible_time': 'Die erhaltene Zeit ist nicht plausibel.', 'time.error.persist_failed': 'Der Server antwortet, konnte aber nicht dauerhaft gespeichert werden.', 'time.error.browser_time_invalid': 'Die Browserzeit ist nicht plausibel.', 'time.error.browser_disabled': 'Browser-Fallback ist deaktiviert.', 'time.error.higher_priority_source': 'Eine höher priorisierte Zeitquelle ist bereits gültig.', 'time.error.busy': 'Eine Zeitprüfung läuft bereits.', 'time.error.unknown': 'Unbekannter Zeitfehler.',
      'common.none': '—', 'common.on': 'EIN', 'common.off': 'AUS', 'common.yes': 'Ja', 'common.no': 'Nein',
      'footer.github': 'GitHub', 'footer.project': 'Projekt',
      'aria.primaryNav': 'Hauptnavigation', 'aria.status': 'Systemstatus', 'time.week': 'KW'
    },
    en: {
      'project.displayEnabled': 'Display',
      'analytics.metric': 'Metric',
      'analytics.metric.count': 'Count',
      'analytics.metric.averageInterval': 'Average interval',
      'analytics.intervalSamples': '{n} intervals',
      'analytics.coveragePartial': 'Average interval is based on the raw events that are still retained.',
      'nav.home': 'Home', 'nav.analytics': 'Analytics', 'nav.device': 'Device', 'nav.settings': 'Settings',
      'view.analytics.title': 'Analytics', 'view.analytics.desc': 'Heatmaps can show either interruption counts or the average completed interval between interruptions.',
      'interruptions.title': 'Interruption counter', 'interruptions.desc': 'Every physical press or web click is captured immediately as one interruption.', 'interruptions.today': 'Interruptions today', 'interruptions.last': 'Last interruption', 'interruptions.button': 'Interruption', 'interruptions.sound': 'Sound on interruption', 'interruptions.soundOn': 'On', 'interruptions.soundOff': 'Off', 'interruptions.pending': 'Saving …', 'interruptions.dropped': '{n} interruption(s) could not be persisted during this run.', 'interruptions.captureFailed': 'The interruption could not be captured.', 'interruptions.justNow': 'just now', 'interruptions.never': 'none yet', 'interruptions.ageUnknown': 'age cannot be determined', 'interruptions.agoSeconds': '{n} sec ago', 'interruptions.agoMinutes': '{n} min ago', 'interruptions.agoHours': '{n} h ago', 'interruptions.agoDays': '{n} d ago',
      'project.settings.title': 'Feedback & display', 'project.settings.desc': 'Device-side interruption feedback and the local OLED view. Changes apply immediately and are stored on the ESP32.', 'project.soundMode': 'Interruption sound', 'project.soundMode.fixed': 'Fixed track', 'project.soundMode.rotate': 'Rotating – next track every time', 'project.soundTrack': 'Track for fixed sound', 'project.soundTrackHint': 'Track 1 is reserved exclusively for the boot sound.', 'project.soundTracksAvailable': 'Available tracks: {n}. Rotating mode uses tracks 2 through {n}.', 'project.soundTracksUnknown': 'Track count is unknown; rotating mode uses the fixed track as fallback.', 'project.displayFlash': 'Flash display on interruption', 'project.displayMode': 'Display view', 'project.displayMode.standard': 'Standard: today + last interruption', 'project.displayMode.count': 'Count only – maximum size', 'project.displayMode.last': 'Last interruption only – maximum size', 'project.displayBrightness': 'Display brightness', 'project.displayDimAfter': 'Dim after', 'project.displayDimBrightness': 'Dimmed brightness', 'project.minutes': 'minutes', 'project.dimDisabled': '0 = dimmer off', 'project.preferenceError': 'The setting could not be saved.',
      'event.source.physical_button': 'Button', 'event.source.web_button': 'Web', 'event.source.software': 'Software', 'event.source.api': 'API', 'event.source.hardware': 'Hardware', 'event.source.unknown': 'Unknown',
      'analytics.hourly.title': 'Weekdays / hours', 'analytics.hourly.desc': 'Weekdays and hours – either count or average interval to the next interruption on the same day.', 'analytics.monthWeek.title': 'Months / calendar weeks', 'analytics.monthWeek.desc': 'Months and ISO calendar weeks – either count or average interval.', 'analytics.yearMonth.title': 'Last 5 years / months', 'analytics.yearMonth.desc': 'The last five calendar years by month – either count or average interval.', 'analytics.storage.title': 'Data & export', 'analytics.storage.desc': 'Raw events are stored in a binary ring; CSV is generated only when downloaded.', 'analytics.mode': 'Selection', 'analytics.mode.week': 'Calendar week', 'analytics.mode.range': 'From / to', 'analytics.year': 'Year', 'analytics.week': 'Calendar week', 'analytics.from': 'From', 'analytics.to': 'To', 'analytics.load': 'Show', 'analytics.download': 'Download CSV', 'analytics.rawEvents': 'Raw events', 'analytics.dailyRecords': 'Daily aggregates', 'analytics.storageUsed': 'Filesystem used', 'analytics.unassigned': 'Without calendar time', 'analytics.dropped': 'Not persisted (this run)', 'analytics.recovering': 'Data recovery is running', 'analytics.noData': 'No data in the selected period.', 'analytics.loadError': 'Analytics could not be loaded.', 'analytics.ringHint': 'When the raw ring is full, the oldest raw events are overwritten; daily aggregates are retained separately.',
      'view.home.title': 'Home',
      'view.device.title': 'Device', 'view.device.desc': 'General ESP32 information and current connection data.',
      'view.settings.title': 'Settings', 'view.settings.desc': 'Browser-only display preferences without ESP32 flash writes.',
      'card.device': 'Device', 'card.device.desc': 'Real general hardware and firmware information.',
      'card.wifi': 'Wi-Fi', 'card.wifi.desc': 'Connection state, network address and signal quality.',
      'card.memory': 'Memory', 'card.memory.desc': 'ESP32 internal heap. PSRAM is shown separately only when available.',
      'card.ota': 'Firmware update', 'card.ota.desc': 'Install a new .bin firmware image directly from the browser. Wi-Fi credentials are retained.',
      'card.hardware': 'Hardware', 'card.hardware.desc': 'Optional hardware modules, their last confirmed health and information available from an intentional check.',
      'card.time': 'Time management', 'card.time.desc': 'Central clock source for the system and future event logs. NTP leads; RTC and browser are fallbacks.',
      'card.language': 'Language', 'card.language.desc': 'Changes apply immediately and are stored only in this browser.',
      'card.theme': 'Appearance', 'card.theme.desc': 'System, Light or Dark – no separate save button.',
      'label.firmware': 'Firmware', 'label.version': 'Version', 'label.uptime': 'Uptime', 'label.board': 'Board', 'label.chip': 'Chip', 'label.cores': 'CPU cores', 'label.flash': 'Flash', 'label.psram': 'PSRAM',
      'label.network': 'Network', 'label.ip': 'IP address', 'label.signal': 'Signal', 'label.heapTotal': 'Heap total', 'label.heapFree': 'Heap free', 'label.heapMin': 'Minimum free', 'label.heapUsed': 'Heap used',
      'label.language': 'Language', 'label.theme': 'Theme', 'label.firmwareImage': 'Current firmware image', 'label.otaCapacity': 'OTA capacity', 'label.projectReserve': 'Project code headroom',
      'status.wifi': 'Wi-Fi', 'status.api': 'Device API', 'status.gpio': 'GPIO', 'status.rtc': 'RTC', 'status.display': 'Display', 'status.audio': 'Audio', 'status.time': 'Time', 'status.data': 'Data storage', 'status.connected': 'Connected', 'status.disconnected': 'Disconnected', 'status.ap': 'Access Point', 'status.ok': 'OK', 'status.warning': 'Warning', 'status.error': 'Error', 'status.inactive': 'Inactive', 'status.disabled': 'Disabled', 'status.checking': 'Checking', 'status.no_response': 'No response', 'status.unknown': 'Unknown', 'status.busy': 'Loading', 'status.stale': 'Stale',
      'quality.good': 'Good', 'quality.normal': 'Normal', 'quality.notice': 'Notice', 'quality.warning': 'Warning', 'quality.critical': 'Critical', 'quality.unknown': 'No data',
      'theme.system': 'Automatic / System', 'theme.light': 'Light', 'theme.dark': 'Dark',
      'lang.de': 'German', 'lang.en': 'English', 'lang.swg': 'Swabian', 'lang.auto': 'Automatically detected from browser language', 'lang.manual': 'Selected manually',
      'action.refresh': 'Refresh', 'action.retry': 'Try again', 'action.otaUpload': 'Install firmware', 'action.hardwareCheckAll': 'Check all', 'action.hardwareCheck': 'Check', 'action.audioTest': 'Test sound', 'action.displayTest': 'Test display',
      'ota.file': 'Firmware file (.bin)',
      'ota.idle': 'No firmware selected yet.', 'ota.ready': 'Ready to update.', 'ota.uploading': 'Firmware is being uploaded and written to the inactive OTA partition.', 'ota.verifying': 'Upload complete. Firmware is being verified and activated.',
      'ota.success': 'Update successful. The device is restarting.', 'ota.reconnecting': 'Waiting for the restart …', 'ota.reconnected': 'Device is reachable again. Reloading the page.', 'ota.reconnectFailed': 'Restart was triggered. If this page does not return, check the new IP in the serial output.',
      'ota.fileTooLarge': 'The selected firmware is larger than the available OTA capacity.', 'ota.invalidFile': 'Select a compiled ESP32 firmware file with the .bin extension.', 'ota.networkError': 'The connection was interrupted during the update. The previous firmware remains active unless the ESP32 already completed the update successfully.', 'ota.failedPrefix': 'Update failed',
      'ota.stage.start': 'Start', 'ota.stage.write': 'Flash write', 'ota.stage.finish': 'Verify/activate', 'ota.stage.aborted': 'Transfer', 'ota.stage.request': 'Request', 'ota.stage.unknown': 'Unknown stage',
      'ota.reason.write': 'Writing to flash failed.', 'ota.reason.erase': 'Erasing the target flash area failed.', 'ota.reason.read': 'The new firmware could not be read back reliably.', 'ota.reason.space': 'The firmware is too large for the OTA partition.', 'ota.reason.size': 'The transferred file size is invalid or incomplete.', 'ota.reason.stream': 'The data transfer stopped or timed out.', 'ota.reason.checksum': 'The firmware checksum does not match.', 'ota.reason.image': 'The file is not a valid ESP32 firmware image.', 'ota.reason.activate': 'The new firmware could not be marked bootable.', 'ota.reason.partition': 'No suitable second OTA application partition was found.', 'ota.reason.argument': 'The OTA request contains invalid data.', 'ota.reason.aborted': 'The upload was aborted by the browser or network.', 'ota.reason.verify': 'Firmware verification failed.', 'ota.reason.unknown': 'Unknown OTA error.',
      'api.unavailable': 'The device API is currently unavailable. Previously loaded values remain visible and are not presented as newly confirmed.',
      'hardware.gpio': 'Digital I/O', 'hardware.rtc': 'DS3231 RTC', 'hardware.display': 'SH1106 OLED', 'hardware.audio': 'DY-SV17F audio',
      'hardware.info.inputs': 'DI pins', 'hardware.info.outputs': 'DO pins', 'hardware.info.model': 'Model', 'hardware.info.transport': 'Interface', 'hardware.info.address': 'Address', 'hardware.info.rtcTime': 'RTC time', 'hardware.info.temperature': 'Temperature', 'hardware.info.osf': 'Oscillator stop flag', 'hardware.info.resolution': 'Resolution', 'hardware.info.initialized': 'Initialized', 'hardware.info.pins': 'Pins', 'hardware.info.playState': 'Playback', 'hardware.info.onlineDevices': 'Device status', 'hardware.info.fileCount': 'Files', 'hardware.info.busy': 'BUSY active', 'hardware.info.lastCheck': 'Check at uptime', 'hardware.info.feedback': 'Feedback', 'hardware.info.testTrack': 'Test/boot track',
      'hardware.feedback.none': 'None', 'hardware.feedback.local_state': 'Local ESP32 state', 'hardware.feedback.transport_ack': 'Bus/transport acknowledged', 'hardware.feedback.protocol_response': 'Protocol response', 'hardware.feedback.external_feedback': 'External feedback',
      'hardware.play.stopped': 'Stopped', 'hardware.play.playing': 'Playing', 'hardware.play.paused': 'Paused', 'hardware.play.unknown': 'Unknown', 'hardware.check.pending': 'Hardware check running …', 'hardware.check.done': 'Hardware check completed.', 'hardware.check.timeout': 'The hardware check is taking longer than expected. The last known state remains visible.', 'hardware.action.failed': 'The hardware action could not be executed.',
      'time.priority.ntp.title': '1. NTP', 'time.priority.ntp.desc': 'Reference / highest priority',
      'time.priority.rtc.title': '2. RTC', 'time.priority.rtc.desc': 'Start time / autonomous',
      'time.priority.browser.title': '3. Browser', 'time.priority.browser.desc': 'Fallback',
      'time.priority.relative.title': '4. No clock', 'time.priority.relative.desc': 'relative only',
      'time.activeSource': 'Active time source', 'time.systemTime': 'System time', 'time.timeStatus': 'Time state', 'time.lastCheck': 'Last check',
      'time.source.ntp': 'NTP', 'time.source.rtc': 'RTC', 'time.source.browser': 'Browser', 'time.source.relative': 'Relative only', 'time.source.none': 'None',
      'time.quality.reference': 'Reference', 'time.quality.valid': 'Valid', 'time.quality.fallback': 'Fallback', 'time.quality.relative': 'No absolute time', 'time.quality.none': 'Unknown',
      'time.ntp.title': 'NTP', 'time.ntp.server': 'Primary NTP server', 'time.ntp.checkSave': 'Check & save', 'time.ntp.hint': 'The entered primary NTP server is tested and stored permanently only after a valid reply.',
      'time.browser.title': 'Browser fallback', 'time.browser.hint': 'Only when neither NTP nor RTC provides valid time may the browser transfer its clock once.',
      'time.differences.title': 'Time sources & differences', 'time.reference': 'Reference', 'time.delta': 'Difference', 'time.sample.ntp': 'NTP', 'time.sample.rtc': 'RTC', 'time.sample.rtcBefore': 'RTC before NTP sync', 'time.sample.system': 'System', 'time.sample.browser': 'Browser',
      'time.rtcSync': 'RTC synchronization', 'time.rtcSync.ok': 'Successfully synchronized from NTP', 'time.rtcSync.failed': 'Synchronization failed', 'time.rtcSync.none': 'Not required for this check',
      'time.check': 'Check time', 'time.checking': 'Checking time sources …', 'time.checked': 'Time check completed.', 'time.ntpChecking': 'Checking NTP server …', 'time.ntpSaved': 'NTP server validated, stored and adopted as reference.',
      'time.browserAccepted': 'Browser time was accepted as fallback.', 'time.unavailable': 'Unavailable', 'time.invalid': 'Invalid', 'time.valid': 'Valid',
      'time.error.no_network': 'No connected Wi-Fi network is available for NTP.', 'time.error.invalid_server': 'The NTP server is invalid.', 'time.error.dns_error': 'The NTP server could not be resolved.', 'time.error.socket_error': 'UDP could not be started for the NTP request.', 'time.error.send_error': 'The NTP request could not be sent.', 'time.error.timeout': 'The NTP server did not reply in time.', 'time.error.invalid_response': 'The NTP response is invalid.', 'time.error.server_unsynchronized': 'The NTP server reports itself as unsynchronized.', 'time.error.implausible_time': 'The received time is not plausible.', 'time.error.persist_failed': 'The server replied but could not be stored permanently.', 'time.error.browser_time_invalid': 'The browser time is not plausible.', 'time.error.browser_disabled': 'Browser fallback is disabled.', 'time.error.higher_priority_source': 'A higher-priority time source is already valid.', 'time.error.busy': 'A time check is already in progress.', 'time.error.unknown': 'Unknown time error.',
      'common.none': '—', 'common.on': 'ON', 'common.off': 'OFF', 'common.yes': 'Yes', 'common.no': 'No',
      'footer.github': 'GitHub', 'footer.project': 'Project',
      'aria.primaryNav': 'Primary navigation', 'aria.status': 'System status', 'time.week': 'Wk'
    },
    swg: {
      'project.displayEnabled': 'Display',
      'analytics.metric': 'Metrik',
      'analytics.metric.count': 'Anzahl',
      'analytics.metric.averageInterval': 'Ø Abstand',
      'analytics.intervalSamples': '{n} Abständ',
      'analytics.coveragePartial': 'Dr Ø-Abstand basiert auf de Rohereignisse, wo no im Speicher send.',
      'nav.home': 'Dahoim', 'nav.analytics': 'Auswertung', 'nav.device': 'Grät', 'nav.settings': 'Eistellonga',
      'view.analytics.title': 'Auswertung', 'view.analytics.desc': 'D Heatmaps zeiget entweder d Anzahl oder dr durchschnittlich abgeschlossene Abstand zwischa de Unterbrechunga.',
      'interruptions.title': 'Unterbrechungszähler', 'interruptions.desc': 'Jeder Druck am Knopf oder im Web zählt sofort als oine Unterbrechung.', 'interruptions.today': 'Unterbrechunga heit', 'interruptions.last': 'Letzte Unterbrechung', 'interruptions.button': 'Unterbrechung', 'interruptions.sound': 'Ton bei Unterbrechung', 'interruptions.soundOn': 'Ei', 'interruptions.soundOff': 'Aus', 'interruptions.pending': 'Wird gspeichert …', 'interruptions.dropped': '{n} Unterbrechung(en) send in dem Lauf net dauerhaft gspeichert worda.', 'interruptions.captureFailed': 'D Unterbrechung hot net erfasst werda kenna.', 'interruptions.justNow': 'grad eben', 'interruptions.never': 'no koine', 'interruptions.ageUnknown': 'Zeitabstand net bestimmbar', 'interruptions.agoSeconds': 'vor {n} Sek.', 'interruptions.agoMinutes': 'vor {n} Min.', 'interruptions.agoHours': 'vor {n} Std.', 'interruptions.agoDays': 'vor {n} Täg',
      'project.settings.title': 'Rückmeldung & Display', 'project.settings.desc': 'D Rückmeldung bei Unterbrechunga ond d OLED-Azeig. Änderungen geltet sofort ond bleibet im ESP32 gspeichert.', 'project.soundMode': 'Unterbrechungston', 'project.soundMode.fixed': 'Feschter Track', 'project.soundMode.rotate': 'Wechselnd – jedes Mol dr nächste Track', 'project.soundTrack': 'Track beim feschta Ton', 'project.soundTrackHint': 'Track 1 bleibt bloß für dr Boot-Ton reserviert.', 'project.soundTracksAvailable': 'Verfügbare Tracks: {n}. Wechselnd nimmt Track 2 bis {n}.', 'project.soundTracksUnknown': 'Trackanzahl isch net bekannt; beim Wechseln dient dr feschte Track als Fallback.', 'project.displayFlash': 'Display bei Unterbrechung aufblitza', 'project.displayMode': 'Display-Azeig', 'project.displayMode.standard': 'Standard: Heit + letzte Unterbrechung', 'project.displayMode.count': 'Bloß Zahl – so groß wie möglich', 'project.displayMode.last': 'Bloß letzte Unterbrechung – so groß wie möglich', 'project.displayBrightness': 'Display-Helligkeit', 'project.displayDimAfter': 'Dimma nach', 'project.displayDimBrightness': 'Helligkeit gedimmt', 'project.minutes': 'Minuta', 'project.dimDisabled': '0 = Dimmer aus', 'project.preferenceError': 'D Eistellung hot sich net speichera lassa.',
      'event.source.physical_button': 'Taster', 'event.source.web_button': 'Web', 'event.source.software': 'Software', 'event.source.api': 'API', 'event.source.hardware': 'Hardware', 'event.source.unknown': 'Net bekannt',
      'analytics.hourly.title': 'Wochentäg / Stunda', 'analytics.hourly.desc': 'Wochentäg ond Stunda – Anzahl oder Ø Abstand bis zur nächste Unterbrechung am selba Tag.', 'analytics.monthWeek.title': 'Monat / Kalenderwocha', 'analytics.monthWeek.desc': 'Monat ond ISO-Kalenderwocha – Anzahl oder Ø Abstand.', 'analytics.yearMonth.title': 'Letzte 5 Johr / Monat', 'analytics.yearMonth.desc': 'D letzte fünf Kalenderjohr nach Monat – Anzahl oder Ø Abstand.', 'analytics.storage.title': 'Dada & Export', 'analytics.storage.desc': 'D Rohereignisse lieget kompakt im Ringspeicher; CSV wird erscht beim Runterlada gmacht.', 'analytics.mode': 'Auswahl', 'analytics.mode.week': 'Kalenderwoch', 'analytics.mode.range': 'Von / Bis', 'analytics.year': 'Johr', 'analytics.week': 'Kalenderwoch', 'analytics.from': 'Von', 'analytics.to': 'Bis', 'analytics.load': 'Azeiga', 'analytics.download': 'CSV runterlada', 'analytics.rawEvents': 'Rohereignisse', 'analytics.dailyRecords': 'Tagesaggregate', 'analytics.storageUsed': 'Dateisystem belegt', 'analytics.unassigned': 'Ohne Kalenderzeit', 'analytics.dropped': 'Net dauerhaft gspeichert (der Lauf)', 'analytics.recovering': 'Dada-Wiederherstellig läuft', 'analytics.noData': 'In dem Zeitraum hot s koine Dada.', 'analytics.loadError': 'D Auswertung hot net glada werda kenna.', 'analytics.ringHint': 'Wenn dr Roh-Ring voll isch, werdet d älteste Rohereignisse überschrieba; d Tageswerte bleibet extra erhalta.',
      'view.home.title': 'Dahoim',
      'view.device.title': 'Grät', 'view.device.desc': 'Allgemeine ESP32-Info ond was grad mit em Netz los isch.',
      'view.settings.title': 'Eistellonga', 'view.settings.desc': 'Darstellung im Browser – ohne unnötig am ESP32-Flash rumzuschreiba.',
      'card.device': 'Grät', 'card.device.desc': 'Echte allgemeine Hardware- ond Firmware-Info.',
      'card.wifi': 'WLAN', 'card.wifi.desc': 'Verbindung, Netzwerkadresse ond Signalqualität.',
      'card.memory': 'Speicher', 'card.memory.desc': 'Dr interne Heap vom ESP32. PSRAM wird bloß zeigt, wenn au wirklich oiner do isch.',
      'card.ota': 'Firmware-Update', 'card.ota.desc': 'A neue .bin-Firmware direkt em Browser uffspiela. D WLAN-Zugangsdada bleibet erhalta.',
      'card.hardware': 'Hardware', 'card.hardware.desc': 'Optionale Hardwaremodule, dr letschte bestätigte Zustand ond d Info vom gezielte Check.',
      'card.time': 'Zeitverwaltung', 'card.time.desc': 'D zentrale Zeit für System ond spätere Ereignislogs. NTP führt, RTC ond Browser send dr Fallback.',
      'card.language': 'Sproch', 'card.language.desc': 'Wird sofort umgstellt ond bloß in dem Browser gspeichert.',
      'card.theme': 'Darstellung', 'card.theme.desc': 'System, Hell oder Dunkel – extra Speichern braucht\'s net.',
      'label.firmware': 'Firmware', 'label.version': 'Version', 'label.uptime': 'Laufzeit', 'label.board': 'Board', 'label.chip': 'Chip', 'label.cores': 'CPU-Kerne', 'label.flash': 'Flash', 'label.psram': 'PSRAM',
      'label.network': 'Netzwerk', 'label.ip': 'IP-Adresse', 'label.signal': 'Signal', 'label.heapTotal': 'Heap gesamt', 'label.heapFree': 'Heap frei', 'label.heapMin': 'Bisher am wenigschta frei', 'label.heapUsed': 'Heap belegt',
      'label.language': 'Sproch', 'label.theme': 'Theme', 'label.firmwareImage': 'Aktuelles Firmware-Image', 'label.otaCapacity': 'OTA-Speicher', 'label.projectReserve': 'Reserve für Projektcode',
      'status.wifi': 'WLAN', 'status.api': 'Gräte-API', 'status.gpio': 'GPIO', 'status.rtc': 'RTC', 'status.display': 'Display', 'status.audio': 'Audio', 'status.time': 'Zeit', 'status.data': 'Dataspeicher', 'status.connected': 'Verbunda', 'status.disconnected': 'Getrennt', 'status.ap': 'Access Point', 'status.ok': 'Passt', 'status.warning': 'Obacht', 'status.error': 'Fehler', 'status.inactive': 'Inaktiv', 'status.disabled': 'Ausgschaltet', 'status.checking': 'Wird prüft', 'status.no_response': 'Koi Antwort', 'status.unknown': 'Net bekannt', 'status.busy': 'Wird glada', 'status.stale': 'Nemme frisch',
      'quality.good': 'Gut', 'quality.normal': 'Normal', 'quality.notice': 'Hinweis', 'quality.warning': 'Obacht', 'quality.critical': 'Kritisch', 'quality.unknown': 'Koi Daten',
      'theme.system': 'Automatisch / System', 'theme.light': 'Hell', 'theme.dark': 'Dunkel',
      'lang.de': 'Deutsch', 'lang.en': 'Englisch', 'lang.swg': 'Schwäbisch', 'lang.auto': 'Vom Browser automatisch erkannt', 'lang.manual': 'Von dir ausgwählt',
      'action.refresh': 'Neu lada', 'action.retry': 'No mal probiera', 'action.otaUpload': 'Firmware uffspiela', 'action.hardwareCheckAll': 'Alle prüfa', 'action.hardwareCheck': 'Prüfa', 'action.audioTest': 'Ton testa', 'action.displayTest': 'Display testa',
      'ota.file': 'Firmware-Datei (.bin)',
      'ota.idle': 'No koi Firmware ausgwählt.', 'ota.ready': 'Bereit fürs Update.', 'ota.uploading': 'D Firmware wird übertraga ond in d inaktive OTA-Partition gschrieba.', 'ota.verifying': 'Übertragung fertig. D Firmware wird prüft ond aktiviert.',
      'ota.success': 'Update hat klappt. S Grät startet neu.', 'ota.reconnecting': 'I wart auf dr Neustart …', 'ota.reconnected': 'S Grät isch wieder erreichbar. D Seite wird neu glada.', 'ota.reconnectFailed': 'Dr Neustart isch ausglöst. Wenn d Seite net wiederkommt, guck nach dr neua IP in dr seriella Ausgabe.',
      'ota.fileTooLarge': 'D ausgwählte Firmware isch größer als dr freie OTA-Speicher.', 'ota.invalidFile': 'Bitte a kompilierte ESP32-Firmware mit dr Endung .bin auswähla.', 'ota.networkError': 'D Verbindung isch beim Update abbrocha. D bisherige Firmware bleibt aktiv, wenn dr ESP32 s Update net scho erfolgreich fertig gmacht hot.', 'ota.failedPrefix': 'Update fehlgschlaga',
      'ota.stage.start': 'Start', 'ota.stage.write': 'Flash schreiba', 'ota.stage.finish': 'Prüfa/Aktiviara', 'ota.stage.aborted': 'Übertragung', 'ota.stage.request': 'Anfrage', 'ota.stage.unknown': 'Unbekannte Phase',
      'ota.reason.write': 'S Schreiba in dr Flash hot net klappt.', 'ota.reason.erase': 'Dr Zielbereich im Flash hot sich net sauber löscha lassa.', 'ota.reason.read': 'D neue Firmware hot sich net zuverlässig aus em Flash lesa lassa.', 'ota.reason.space': 'D Firmware isch zu groß für d OTA-Partition.', 'ota.reason.size': 'D übertragene Dateigröße passt net oder isch unvollständig.', 'ota.reason.stream': 'D Datenübertragung isch abbrocha oder ins Timeout gloffa.', 'ota.reason.checksum': 'D Prüfsumme von dr Firmware stimmt net.', 'ota.reason.image': 'D Datei isch koi gültigs ESP32-Firmware-Image.', 'ota.reason.activate': 'D neue Firmware hot sich net als startfähig aktiviara lassa.', 'ota.reason.partition': 'Koi passende zweite OTA-App-Partition gfunda.', 'ota.reason.argument': 'Mit dr OTA-Anfrage stimmt ebbes net.', 'ota.reason.aborted': 'Dr Browser oder s Netzwerk hot dr Upload abbrocha.', 'ota.reason.verify': 'D Firmware-Prüfung isch fehlgschlaga.', 'ota.reason.unknown': 'Unbekannter OTA-Fehler.',
      'api.unavailable': 'D Gräte-API isch grad net erreichbar. Alte Werte bleibet sichtbar, werdet aber net als frisch bestätigt ausgaba.',
      'hardware.gpio': 'Digitale Ei-/Ausgäng', 'hardware.rtc': 'DS3231 RTC', 'hardware.display': 'SH1106 OLED', 'hardware.audio': 'DY-SV17F Sound',
      'hardware.info.inputs': 'DI-Pins', 'hardware.info.outputs': 'DO-Pins', 'hardware.info.model': 'Modell', 'hardware.info.transport': 'Schnittstell', 'hardware.info.address': 'Adress', 'hardware.info.rtcTime': 'RTC-Zeit', 'hardware.info.temperature': 'Temperatur', 'hardware.info.osf': 'Oszillator-Stop-Flag', 'hardware.info.resolution': 'Auflösung', 'hardware.info.initialized': 'Initialisiert', 'hardware.info.pins': 'Pins', 'hardware.info.playState': 'Wiedergab', 'hardware.info.onlineDevices': 'Datenträgerstatus', 'hardware.info.fileCount': 'Dateia', 'hardware.info.busy': 'BUSY aktiv', 'hardware.info.lastCheck': 'Prüfung bei Uptime', 'hardware.info.feedback': 'Rückmeldung', 'hardware.info.testTrack': 'Test-/Boot-Ton',
      'hardware.feedback.none': 'Koi', 'hardware.feedback.local_state': 'Lokaler ESP32-Zustand', 'hardware.feedback.transport_ack': 'Bus/Transport bestätigt', 'hardware.feedback.protocol_response': 'Protokollantwort', 'hardware.feedback.external_feedback': 'Externe Rückmeldung',
      'hardware.play.stopped': 'Gstoppt', 'hardware.play.playing': 'Läuft', 'hardware.play.paused': 'Pausiert', 'hardware.play.unknown': 'Net bekannt', 'hardware.check.pending': 'Hardware wird grad prüft …', 'hardware.check.done': 'Hardwareprüfung isch fertig.', 'hardware.check.timeout': 'D Hardwareprüfung braucht länger als erwartet. Dr letschte Zustand bleibt sichtbar.', 'hardware.action.failed': 'D Hardwareaktion hot net klappt.',
      'time.priority.ntp.title': '1. NTP', 'time.priority.ntp.desc': 'Referenz / höchste Priorität',
      'time.priority.rtc.title': '2. RTC', 'time.priority.rtc.desc': 'Startzeit / autark',
      'time.priority.browser.title': '3. Browser', 'time.priority.browser.desc': 'Fallback',
      'time.priority.relative.title': '4. Ohne Zeit', 'time.priority.relative.desc': 'bloß relativ',
      'time.activeSource': 'Aktive Zeitquell', 'time.systemTime': 'Systemzeit', 'time.timeStatus': 'Zeitzustand', 'time.lastCheck': 'Letschte Prüfung',
      'time.source.ntp': 'NTP', 'time.source.rtc': 'RTC', 'time.source.browser': 'Browser', 'time.source.relative': 'Bloß relativ', 'time.source.none': 'Koi',
      'time.quality.reference': 'Referenz', 'time.quality.valid': 'Gültig', 'time.quality.fallback': 'Fallback', 'time.quality.relative': 'Koi absolute Zeit', 'time.quality.none': 'Net bekannt',
      'time.ntp.title': 'NTP', 'time.ntp.server': 'Primärer NTP-Server', 'time.ntp.checkSave': 'Prüfa & speichera', 'time.ntp.hint': 'Dr eingetragene NTP-Server wird prüft ond bloß bei ere gültige Antwort dauerhaft gspeichert.',
      'time.browser.title': 'Browser-Fallback', 'time.browser.hint': 'Bloß wenn weder NTP no RTC a gültige Zeit hend, darf dr Browser oimal sei Uhrzeit rüberschicka.',
      'time.differences.title': 'Zeitquella & Unterschied', 'time.reference': 'Referenz', 'time.delta': 'Unterschied', 'time.sample.ntp': 'NTP', 'time.sample.rtc': 'RTC', 'time.sample.rtcBefore': 'RTC vor em NTP-Abgleich', 'time.sample.system': 'System', 'time.sample.browser': 'Browser',
      'time.rtcSync': 'RTC-Nachführung', 'time.rtcSync.ok': 'Erfolgreich vom NTP nachgführt', 'time.rtcSync.failed': 'Nachführung hot net klappt', 'time.rtcSync.none': 'Bei dem Check net nötig',
      'time.check': 'Zeit prüfa', 'time.checking': 'Zeitquella werdet grad prüft …', 'time.checked': 'Zeitprüfung isch fertig.', 'time.ntpChecking': 'NTP-Server wird grad prüft …', 'time.ntpSaved': 'NTP-Server isch prüft, gspeichert ond als Referenz übernomma.',
      'time.browserAccepted': 'Browserzeit isch als Fallback übernomma.', 'time.unavailable': 'Net verfügbar', 'time.invalid': 'Net gültig', 'time.valid': 'Gültig',
      'time.error.no_network': 'Für NTP isch grad koi verbundenes WLAN do.', 'time.error.invalid_server': 'Dr NTP-Server isch net gültig.', 'time.error.dns_error': 'Dr NTP-Server hot sich net auflösa lassa.', 'time.error.socket_error': 'UDP für NTP hot sich net starta lassa.', 'time.error.send_error': 'D NTP-Anfrag hot sich net senda lassa.', 'time.error.timeout': 'Dr NTP-Server hot net rechtzeitig geantwortet.', 'time.error.invalid_response': 'D NTP-Antwort isch net gültig.', 'time.error.server_unsynchronized': 'Dr NTP-Server sagt selber, dass er net synchron isch.', 'time.error.implausible_time': 'D bekommene Zeit isch net plausibel.', 'time.error.persist_failed': 'Dr Server antwortet, hot sich aber net dauerhaft speichera lassa.', 'time.error.browser_time_invalid': 'D Browserzeit isch net plausibel.', 'time.error.browser_disabled': 'Dr Browser-Fallback isch ausgschaltet.', 'time.error.higher_priority_source': 'A bessere Zeitquell isch scho gültig.', 'time.error.busy': 'A Zeitprüfung lauft grad scho.', 'time.error.unknown': 'Net bekannter Zeitfehler.',
      'common.none': '—', 'common.on': 'EIN', 'common.off': 'AUS', 'common.yes': 'Ja', 'common.no': 'Nein',
      'footer.github': 'GitHub', 'footer.project': 'Projekt',
      'aria.primaryNav': 'Hauptnavigation', 'aria.status': 'Systemstatus', 'time.week': 'KW'
    }
  };

  // Additional bundled language packs. Dialect variants inherit the general Swabian pack
  // so technical terminology and future fallbacks remain consistent.
  I18N.it = {
    'project.displayEnabled': 'Display',
    'analytics.metric': 'Metrica',
    'analytics.metric.count': 'Conteggio',
    'analytics.metric.averageInterval': 'Intervallo medio',
    'analytics.intervalSamples': '{n} intervalli',
    'analytics.coveragePartial': 'L\'intervallo medio si basa sugli eventi grezzi ancora presenti in memoria.',
    "nav.home": "Home",
    "nav.analytics": "Analisi",
    "nav.device": "Dispositivo",
    "nav.settings": "Impostazioni",
    "view.analytics.title": "Analisi",
    "view.analytics.desc": "Le mappe di calore mostrano il numero di interruzioni oppure l'intervallo medio completato tra le interruzioni.",
    "interruptions.title": "Contatore delle interruzioni",
    "interruptions.desc": "Ogni pressione del pulsante o clic sul web viene registrato immediatamente come un’interruzione.",
    "interruptions.today": "Interruzioni oggi",
    "interruptions.last": "Ultima interruzione",
    "interruptions.button": "Interruzione",
    "interruptions.sound": "Suono all’interruzione",
    "interruptions.soundOn": "Attivo",
    "interruptions.soundOff": "Disattivo",
    "interruptions.pending": "Salvataggio …",
    "interruptions.dropped": "{n} interruzione/i non sono state salvate in modo permanente durante questa sessione.",
    "interruptions.captureFailed": "Impossibile registrare l’interruzione.",
    "interruptions.justNow": "proprio ora",
    "interruptions.never": "ancora nessuna",
    "interruptions.ageUnknown": "impossibile determinare il tempo trascorso",
    "interruptions.agoSeconds": "{n} sec fa",
    "interruptions.agoMinutes": "{n} min fa",
    "interruptions.agoHours": "{n} h fa",
    "interruptions.agoDays": "{n} g fa",
    "project.settings.title": "Feedback e display",
    "project.settings.desc": "Feedback del dispositivo per le interruzioni e visualizzazione OLED locale. Le modifiche si applicano subito e vengono memorizzate sull’ESP32.",
    "project.soundMode": "Suono dell’interruzione",
    "project.soundMode.fixed": "Traccia fissa",
    "project.soundMode.rotate": "A rotazione – traccia successiva ogni volta",
    "project.soundTrack": "Traccia per il suono fisso",
    "project.soundTrackHint": "La traccia 1 è riservata esclusivamente al suono di avvio.",
    "project.soundTracksAvailable": "Tracce disponibili: {n}. La modalità a rotazione usa le tracce da 2 a {n}.",
    "project.soundTracksUnknown": "Numero di tracce sconosciuto; in modalità a rotazione viene usata la traccia fissa come fallback.",
    "project.displayFlash": "Lampeggio del display all’interruzione",
    "project.displayMode": "Vista display",
    "project.displayMode.standard": "Standard: oggi + ultima interruzione",
    "project.displayMode.count": "Solo numero – dimensione massima",
    "project.displayMode.last": "Solo ultima interruzione – dimensione massima",
    "project.displayBrightness": "Luminosità display",
    "project.displayDimAfter": "Attenua dopo",
    "project.displayDimBrightness": "Luminosità attenuata",
    "project.minutes": "minuti",
    "project.dimDisabled": "0 = attenuazione disattivata",
    "project.preferenceError": "Impossibile salvare l’impostazione.",
    "event.source.physical_button": "Pulsante",
    "event.source.web_button": "Web",
    "event.source.software": "Software",
    "event.source.api": "API",
    "event.source.hardware": "Hardware",
    "event.source.unknown": "Sconosciuto",
    "analytics.hourly.title": "Giorni della settimana / ore",
    "analytics.hourly.desc": "Giorni e ore: conteggio oppure intervallo medio fino all'interruzione successiva dello stesso giorno.",
    "analytics.monthWeek.title": "Mesi / settimane di calendario",
    "analytics.monthWeek.desc": "Mesi e settimane ISO: conteggio oppure intervallo medio.",
    "analytics.yearMonth.title": "Ultimi 5 anni / mesi",
    "analytics.yearMonth.desc": "Ultimi cinque anni per mese: conteggio oppure intervallo medio.",
    "analytics.storage.title": "Dati ed esportazione",
    "analytics.storage.desc": "Gli eventi grezzi sono memorizzati in un ring binario; il CSV viene generato solo al download.",
    "analytics.mode": "Selezione",
    "analytics.mode.week": "Settimana di calendario",
    "analytics.mode.range": "Da / a",
    "analytics.year": "Anno",
    "analytics.week": "Settimana di calendario",
    "analytics.from": "Da",
    "analytics.to": "A",
    "analytics.load": "Mostra",
    "analytics.download": "Scarica CSV",
    "analytics.rawEvents": "Eventi grezzi",
    "analytics.dailyRecords": "Aggregati giornalieri",
    "analytics.storageUsed": "File system utilizzato",
    "analytics.unassigned": "Senza ora di calendario",
    "analytics.dropped": "Non salvati in modo permanente (questa sessione)",
    "analytics.recovering": "Recupero dati in corso",
    "analytics.noData": "Nessun dato nel periodo selezionato.",
    "analytics.loadError": "Impossibile caricare l’analisi.",
    "analytics.ringHint": "Quando il ring degli eventi grezzi è pieno, gli eventi più vecchi vengono sovrascritti; gli aggregati giornalieri vengono conservati separatamente.",
    "view.home.title": "Home",
    "view.device.title": "Dispositivo",
    "view.device.desc": "Informazioni generali dell’ESP32 e dati della connessione corrente.",
    "view.settings.title": "Impostazioni",
    "view.settings.desc": "Preferenze di visualizzazione solo del browser, senza scritture nella flash dell’ESP32.",
    "card.device": "Dispositivo",
    "card.device.desc": "Informazioni reali generali su hardware e firmware.",
    "card.wifi": "Wi‑Fi",
    "card.wifi.desc": "Stato della connessione, indirizzo di rete e qualità del segnale.",
    "card.memory": "Memoria",
    "card.memory.desc": "Heap interno dell’ESP32. La PSRAM viene mostrata separatamente solo se disponibile.",
    "card.ota": "Aggiornamento firmware",
    "card.ota.desc": "Installa un nuovo firmware .bin direttamente dal browser. Le credenziali Wi‑Fi vengono mantenute.",
    "card.hardware": "Hardware",
    "card.hardware.desc": "Moduli hardware opzionali, ultimo stato confermato e informazioni disponibili da un controllo intenzionale.",
    "card.time": "Gestione del tempo",
    "card.time.desc": "Sorgente oraria centrale per il sistema e i futuri log degli eventi. NTP ha priorità; RTC e browser sono fallback.",
    "card.language": "Lingua",
    "card.language.desc": "Le modifiche si applicano subito e vengono memorizzate solo in questo browser.",
    "card.theme": "Aspetto",
    "card.theme.desc": "Sistema, Chiaro o Scuro – senza pulsante di salvataggio separato.",
    "label.firmware": "Firmware",
    "label.version": "Versione",
    "label.uptime": "Tempo di attività",
    "label.board": "Scheda",
    "label.chip": "Chip",
    "label.cores": "Core CPU",
    "label.flash": "Flash",
    "label.psram": "PSRAM",
    "label.network": "Rete",
    "label.ip": "Indirizzo IP",
    "label.signal": "Segnale",
    "label.heapTotal": "Heap totale",
    "label.heapFree": "Heap libero",
    "label.heapMin": "Minimo libero",
    "label.heapUsed": "Heap utilizzato",
    "label.language": "Lingua",
    "label.theme": "Tema",
    "label.firmwareImage": "Immagine firmware attuale",
    "label.otaCapacity": "Capacità OTA",
    "label.projectReserve": "Margine per il codice del progetto",
    "status.wifi": "Wi‑Fi",
    "status.api": "API dispositivo",
    "status.gpio": "GPIO",
    "status.rtc": "RTC",
    "status.display": "Display",
    "status.audio": "Audio",
    "status.time": "Ora",
    "status.data": "Archivio dati",
    "status.connected": "Connesso",
    "status.disconnected": "Disconnesso",
    "status.ap": "Access Point",
    "status.ok": "OK",
    "status.warning": "Avviso",
    "status.error": "Errore",
    "status.inactive": "Inattivo",
    "status.disabled": "Disattivato",
    "status.checking": "Controllo",
    "status.no_response": "Nessuna risposta",
    "status.unknown": "Sconosciuto",
    "status.busy": "Caricamento",
    "status.stale": "Obsoleto",
    "quality.good": "Buono",
    "quality.normal": "Normale",
    "quality.notice": "Nota",
    "quality.warning": "Avviso",
    "quality.critical": "Critico",
    "quality.unknown": "Nessun dato",
    "theme.system": "Automatico / Sistema",
    "theme.light": "Chiaro",
    "theme.dark": "Scuro",
    "lang.de": "Tedesco",
    "lang.en": "Inglese",
    "lang.swg": "Svevo",
    "lang.auto": "Rilevato automaticamente dalla lingua del browser",
    "lang.manual": "Selezionato manualmente",
    "action.refresh": "Aggiorna",
    "action.retry": "Riprova",
    "action.otaUpload": "Installa firmware",
    "action.hardwareCheckAll": "Controlla tutto",
    "action.hardwareCheck": "Controlla",
    "action.audioTest": "Test audio",
    "action.displayTest": "Test display",
    "ota.file": "File firmware (.bin)",
    "ota.idle": "Nessun firmware selezionato.",
    "ota.ready": "Pronto per l’aggiornamento.",
    "ota.uploading": "Il firmware viene caricato e scritto nella partizione OTA inattiva.",
    "ota.verifying": "Caricamento completato. Il firmware viene verificato e attivato.",
    "ota.success": "Aggiornamento riuscito. Il dispositivo si riavvia.",
    "ota.reconnecting": "In attesa del riavvio …",
    "ota.reconnected": "Il dispositivo è di nuovo raggiungibile. Ricarico la pagina.",
    "ota.reconnectFailed": "Il riavvio è stato avviato. Se la pagina non ritorna, controlla il nuovo IP nell’output seriale.",
    "ota.fileTooLarge": "Il firmware selezionato è più grande della capacità OTA disponibile.",
    "ota.invalidFile": "Seleziona un firmware ESP32 compilato con estensione .bin.",
    "ota.networkError": "La connessione si è interrotta durante l’aggiornamento. Il firmware precedente rimane attivo, salvo che l’ESP32 abbia già completato correttamente l’aggiornamento.",
    "ota.failedPrefix": "Aggiornamento fallito",
    "ota.stage.start": "Avvio",
    "ota.stage.write": "Scrittura flash",
    "ota.stage.finish": "Verifica/attivazione",
    "ota.stage.aborted": "Trasferimento",
    "ota.stage.request": "Richiesta",
    "ota.stage.unknown": "Fase sconosciuta",
    "ota.reason.write": "Scrittura nella flash non riuscita.",
    "ota.reason.erase": "Cancellazione dell’area flash di destinazione non riuscita.",
    "ota.reason.read": "Impossibile rileggere in modo affidabile il nuovo firmware.",
    "ota.reason.space": "Il firmware è troppo grande per la partizione OTA.",
    "ota.reason.size": "La dimensione del file trasferito non è valida o è incompleta.",
    "ota.reason.stream": "Il trasferimento dati si è interrotto o è scaduto.",
    "ota.reason.checksum": "Il checksum del firmware non corrisponde.",
    "ota.reason.image": "Il file non è un’immagine firmware ESP32 valida.",
    "ota.reason.activate": "Impossibile contrassegnare il nuovo firmware come avviabile.",
    "ota.reason.partition": "Nessuna seconda partizione applicativa OTA adatta trovata.",
    "ota.reason.argument": "La richiesta OTA contiene dati non validi.",
    "ota.reason.aborted": "Il caricamento è stato interrotto dal browser o dalla rete.",
    "ota.reason.verify": "Verifica del firmware non riuscita.",
    "ota.reason.unknown": "Errore OTA sconosciuto.",
    "api.unavailable": "L’API del dispositivo non è attualmente disponibile. I valori caricati in precedenza restano visibili ma non vengono presentati come appena confermati.",
    "hardware.gpio": "I/O digitali",
    "hardware.rtc": "RTC DS3231",
    "hardware.display": "OLED SH1106",
    "hardware.audio": "Audio DY-SV17F",
    "hardware.info.inputs": "Pin DI",
    "hardware.info.outputs": "Pin DO",
    "hardware.info.model": "Modello",
    "hardware.info.transport": "Interfaccia",
    "hardware.info.address": "Indirizzo",
    "hardware.info.rtcTime": "Ora RTC",
    "hardware.info.temperature": "Temperatura",
    "hardware.info.osf": "Flag arresto oscillatore",
    "hardware.info.resolution": "Risoluzione",
    "hardware.info.initialized": "Inizializzato",
    "hardware.info.pins": "Pin",
    "hardware.info.playState": "Riproduzione",
    "hardware.info.onlineDevices": "Stato dispositivo",
    "hardware.info.fileCount": "File",
    "hardware.info.busy": "BUSY attivo",
    "hardware.info.lastCheck": "Controllo a uptime",
    "hardware.info.feedback": "Feedback",
    "hardware.info.testTrack": "Traccia test/avvio",
    "hardware.feedback.none": "Nessuno",
    "hardware.feedback.local_state": "Stato locale ESP32",
    "hardware.feedback.transport_ack": "Bus/trasporto confermato",
    "hardware.feedback.protocol_response": "Risposta protocollo",
    "hardware.feedback.external_feedback": "Feedback esterno",
    "hardware.play.stopped": "Fermato",
    "hardware.play.playing": "In riproduzione",
    "hardware.play.paused": "In pausa",
    "hardware.play.unknown": "Sconosciuto",
    "hardware.check.pending": "Controllo hardware in corso …",
    "hardware.check.done": "Controllo hardware completato.",
    "hardware.check.timeout": "Il controllo hardware richiede più tempo del previsto. Rimane visibile l’ultimo stato noto.",
    "hardware.action.failed": "Impossibile eseguire l’azione hardware.",
    "time.priority.ntp.title": "1. NTP",
    "time.priority.ntp.desc": "Riferimento / priorità massima",
    "time.priority.rtc.title": "2. RTC",
    "time.priority.rtc.desc": "Ora iniziale / autonomo",
    "time.priority.browser.title": "3. Browser",
    "time.priority.browser.desc": "Fallback",
    "time.priority.relative.title": "4. Senza orologio",
    "time.priority.relative.desc": "solo relativo",
    "time.activeSource": "Sorgente oraria attiva",
    "time.systemTime": "Ora di sistema",
    "time.timeStatus": "Stato dell’ora",
    "time.lastCheck": "Ultimo controllo",
    "time.source.ntp": "NTP",
    "time.source.rtc": "RTC",
    "time.source.browser": "Browser",
    "time.source.relative": "Solo relativo",
    "time.source.none": "Nessuno",
    "time.quality.reference": "Riferimento",
    "time.quality.valid": "Valido",
    "time.quality.fallback": "Fallback",
    "time.quality.relative": "Nessuna ora assoluta",
    "time.quality.none": "Sconosciuto",
    "time.ntp.title": "NTP",
    "time.ntp.server": "Server NTP primario",
    "time.ntp.checkSave": "Controlla e salva",
    "time.ntp.hint": "Il server NTP primario inserito viene testato e memorizzato in modo permanente solo dopo una risposta valida.",
    "time.browser.title": "Fallback browser",
    "time.browser.hint": "Solo quando né NTP né RTC forniscono un’ora valida, il browser può trasferire una volta il proprio orologio.",
    "time.differences.title": "Sorgenti orarie e differenze",
    "time.reference": "Riferimento",
    "time.delta": "Differenza",
    "time.sample.ntp": "NTP",
    "time.sample.rtc": "RTC",
    "time.sample.rtcBefore": "RTC prima della sincronizzazione NTP",
    "time.sample.system": "Sistema",
    "time.sample.browser": "Browser",
    "time.rtcSync": "Sincronizzazione RTC",
    "time.rtcSync.ok": "Sincronizzato correttamente da NTP",
    "time.rtcSync.failed": "Sincronizzazione non riuscita",
    "time.rtcSync.none": "Non richiesta per questo controllo",
    "time.check": "Controlla ora",
    "time.checking": "Controllo delle sorgenti orarie …",
    "time.checked": "Controllo dell’ora completato.",
    "time.ntpChecking": "Controllo server NTP …",
    "time.ntpSaved": "Server NTP verificato, memorizzato e adottato come riferimento.",
    "time.browserAccepted": "L’ora del browser è stata accettata come fallback.",
    "time.unavailable": "Non disponibile",
    "time.invalid": "Non valido",
    "time.valid": "Valido",
    "time.error.no_network": "Nessuna rete Wi‑Fi connessa disponibile per NTP.",
    "time.error.invalid_server": "Il server NTP non è valido.",
    "time.error.dns_error": "Impossibile risolvere il server NTP.",
    "time.error.socket_error": "Impossibile avviare UDP per la richiesta NTP.",
    "time.error.send_error": "Impossibile inviare la richiesta NTP.",
    "time.error.timeout": "Il server NTP non ha risposto in tempo.",
    "time.error.invalid_response": "La risposta NTP non è valida.",
    "time.error.server_unsynchronized": "Il server NTP segnala di non essere sincronizzato.",
    "time.error.implausible_time": "L’ora ricevuta non è plausibile.",
    "time.error.persist_failed": "Il server ha risposto ma non è stato possibile salvarlo in modo permanente.",
    "time.error.browser_time_invalid": "L’ora del browser non è plausibile.",
    "time.error.browser_disabled": "Il fallback del browser è disattivato.",
    "time.error.higher_priority_source": "È già valida una sorgente oraria con priorità superiore.",
    "time.error.busy": "È già in corso un controllo dell’ora.",
    "time.error.unknown": "Errore orario sconosciuto.",
    "common.none": "—",
    "common.on": "ON",
    "common.off": "OFF",
    "common.yes": "Sì",
    "common.no": "No",
    "footer.github": "GitHub",
    "footer.project": "Progetto",
    "aria.primaryNav": "Navigazione principale",
    "aria.status": "Stato del sistema",
    "time.week": "Sett."
};
  I18N.fr = {
    'project.displayEnabled': 'Écran',
    'analytics.metric': 'Mesure',
    'analytics.metric.count': 'Nombre',
    'analytics.metric.averageInterval': 'Intervalle moyen',
    'analytics.intervalSamples': '{n} intervalles',
    'analytics.coveragePartial': 'L\'intervalle moyen repose sur les événements bruts encore conservés.',
    "nav.home": "Accueil",
    "nav.analytics": "Analyse",
    "nav.device": "Appareil",
    "nav.settings": "Paramètres",
    "view.analytics.title": "Analyse",
    "view.analytics.desc": "Les cartes thermiques affichent le nombre d'interruptions ou l'intervalle moyen terminé entre les interruptions.",
    "interruptions.title": "Compteur d’interruptions",
    "interruptions.desc": "Chaque pression sur le bouton ou clic Web est immédiatement enregistré comme une interruption.",
    "interruptions.today": "Interruptions aujourd’hui",
    "interruptions.last": "Dernière interruption",
    "interruptions.button": "Interruption",
    "interruptions.sound": "Son lors d’une interruption",
    "interruptions.soundOn": "Activé",
    "interruptions.soundOff": "Désactivé",
    "interruptions.pending": "Enregistrement …",
    "interruptions.dropped": "{n} interruption(s) n’ont pas pu être enregistrées durablement pendant cette session.",
    "interruptions.captureFailed": "L’interruption n’a pas pu être enregistrée.",
    "interruptions.justNow": "à l’instant",
    "interruptions.never": "aucune pour le moment",
    "interruptions.ageUnknown": "délai impossible à déterminer",
    "interruptions.agoSeconds": "il y a {n} s",
    "interruptions.agoMinutes": "il y a {n} min",
    "interruptions.agoHours": "il y a {n} h",
    "interruptions.agoDays": "il y a {n} j",
    "project.settings.title": "Retour & affichage",
    "project.settings.desc": "Retour local lors des interruptions et affichage OLED. Les modifications s’appliquent immédiatement et sont enregistrées sur l’ESP32.",
    "project.soundMode": "Son d’interruption",
    "project.soundMode.fixed": "Piste fixe",
    "project.soundMode.rotate": "Rotation – piste suivante à chaque fois",
    "project.soundTrack": "Piste pour le son fixe",
    "project.soundTrackHint": "La piste 1 est réservée exclusivement au son de démarrage.",
    "project.soundTracksAvailable": "Pistes disponibles : {n}. Le mode rotation utilise les pistes 2 à {n}.",
    "project.soundTracksUnknown": "Nombre de pistes inconnu ; le mode rotation utilise la piste fixe comme solution de repli.",
    "project.displayFlash": "Faire clignoter l’écran lors d’une interruption",
    "project.displayMode": "Mode d’affichage",
    "project.displayMode.standard": "Standard : aujourd’hui + dernière interruption",
    "project.displayMode.count": "Nombre seul – taille maximale",
    "project.displayMode.last": "Dernière interruption seule – taille maximale",
    "project.displayBrightness": "Luminosité de l’écran",
    "project.displayDimAfter": "Atténuer après",
    "project.displayDimBrightness": "Luminosité atténuée",
    "project.minutes": "minutes",
    "project.dimDisabled": "0 = atténuation désactivée",
    "project.preferenceError": "Le paramètre n’a pas pu être enregistré.",
    "event.source.physical_button": "Bouton",
    "event.source.web_button": "Web",
    "event.source.software": "Logiciel",
    "event.source.api": "API",
    "event.source.hardware": "Matériel",
    "event.source.unknown": "Inconnu",
    "analytics.hourly.title": "Jours de la semaine / heures",
    "analytics.hourly.desc": "Jours et heures : nombre ou intervalle moyen jusqu'à l'interruption suivante du même jour.",
    "analytics.monthWeek.title": "Mois / semaines calendaires",
    "analytics.monthWeek.desc": "Mois et semaines ISO : nombre ou intervalle moyen.",
    "analytics.yearMonth.title": "5 dernières années / mois",
    "analytics.yearMonth.desc": "Cinq dernières années par mois : nombre ou intervalle moyen.",
    "analytics.storage.title": "Données & export",
    "analytics.storage.desc": "Les événements bruts sont stockés dans un anneau binaire ; le CSV n’est généré qu’au téléchargement.",
    "analytics.mode": "Sélection",
    "analytics.mode.week": "Semaine calendaire",
    "analytics.mode.range": "Du / au",
    "analytics.year": "Année",
    "analytics.week": "Semaine calendaire",
    "analytics.from": "Du",
    "analytics.to": "Au",
    "analytics.load": "Afficher",
    "analytics.download": "Télécharger le CSV",
    "analytics.rawEvents": "Événements bruts",
    "analytics.dailyRecords": "Agrégats journaliers",
    "analytics.storageUsed": "Système de fichiers utilisé",
    "analytics.unassigned": "Sans heure calendaire",
    "analytics.dropped": "Non enregistrés durablement (cette session)",
    "analytics.recovering": "Récupération des données en cours",
    "analytics.noData": "Aucune donnée pour la période sélectionnée.",
    "analytics.loadError": "Impossible de charger l’analyse.",
    "analytics.ringHint": "Lorsque l’anneau brut est plein, les événements bruts les plus anciens sont écrasés ; les agrégats journaliers sont conservés séparément.",
    "view.home.title": "Accueil",
    "view.device.title": "Appareil",
    "view.device.desc": "Informations générales sur l’ESP32 et données de connexion actuelles.",
    "view.settings.title": "Paramètres",
    "view.settings.desc": "Préférences d’affichage propres au navigateur, sans écriture dans la flash de l’ESP32.",
    "card.device": "Appareil",
    "card.device.desc": "Informations générales réelles sur le matériel et le firmware.",
    "card.wifi": "Wi‑Fi",
    "card.wifi.desc": "État de la connexion, adresse réseau et qualité du signal.",
    "card.memory": "Mémoire",
    "card.memory.desc": "Tas interne de l’ESP32. La PSRAM n’est affichée séparément que si elle est disponible.",
    "card.ota": "Mise à jour du firmware",
    "card.ota.desc": "Installer directement depuis le navigateur un nouveau firmware .bin. Les identifiants Wi‑Fi sont conservés.",
    "card.hardware": "Matériel",
    "card.hardware.desc": "Modules matériels optionnels, dernier état confirmé et informations disponibles lors d’un contrôle volontaire.",
    "card.time": "Gestion du temps",
    "card.time.desc": "Source horaire centrale pour le système et les futurs journaux d’événements. NTP est prioritaire ; RTC et navigateur servent de repli.",
    "card.language": "Langue",
    "card.language.desc": "Les changements s’appliquent immédiatement et sont enregistrés uniquement dans ce navigateur.",
    "card.theme": "Apparence",
    "card.theme.desc": "Système, Clair ou Sombre – sans bouton d’enregistrement séparé.",
    "label.firmware": "Firmware",
    "label.version": "Version",
    "label.uptime": "Temps de fonctionnement",
    "label.board": "Carte",
    "label.chip": "Puce",
    "label.cores": "Cœurs CPU",
    "label.flash": "Flash",
    "label.psram": "PSRAM",
    "label.network": "Réseau",
    "label.ip": "Adresse IP",
    "label.signal": "Signal",
    "label.heapTotal": "Tas total",
    "label.heapFree": "Tas libre",
    "label.heapMin": "Minimum libre",
    "label.heapUsed": "Tas utilisé",
    "label.language": "Langue",
    "label.theme": "Thème",
    "label.firmwareImage": "Image firmware actuelle",
    "label.otaCapacity": "Capacité OTA",
    "label.projectReserve": "Marge pour le code du projet",
    "status.wifi": "Wi‑Fi",
    "status.api": "API appareil",
    "status.gpio": "GPIO",
    "status.rtc": "RTC",
    "status.display": "Affichage",
    "status.audio": "Audio",
    "status.time": "Heure",
    "status.data": "Stockage des données",
    "status.connected": "Connecté",
    "status.disconnected": "Déconnecté",
    "status.ap": "Point d’accès",
    "status.ok": "OK",
    "status.warning": "Avertissement",
    "status.error": "Erreur",
    "status.inactive": "Inactif",
    "status.disabled": "Désactivé",
    "status.checking": "Vérification",
    "status.no_response": "Aucune réponse",
    "status.unknown": "Inconnu",
    "status.busy": "Chargement",
    "status.stale": "Périmé",
    "quality.good": "Bon",
    "quality.normal": "Normal",
    "quality.notice": "Information",
    "quality.warning": "Avertissement",
    "quality.critical": "Critique",
    "quality.unknown": "Aucune donnée",
    "theme.system": "Automatique / Système",
    "theme.light": "Clair",
    "theme.dark": "Sombre",
    "lang.de": "Allemand",
    "lang.en": "Anglais",
    "lang.swg": "Souabe",
    "lang.auto": "Détecté automatiquement depuis la langue du navigateur",
    "lang.manual": "Sélectionné manuellement",
    "action.refresh": "Actualiser",
    "action.retry": "Réessayer",
    "action.otaUpload": "Installer le firmware",
    "action.hardwareCheckAll": "Tout vérifier",
    "action.hardwareCheck": "Vérifier",
    "action.audioTest": "Tester le son",
    "action.displayTest": "Tester l’affichage",
    "ota.file": "Fichier firmware (.bin)",
    "ota.idle": "Aucun firmware sélectionné.",
    "ota.ready": "Prêt pour la mise à jour.",
    "ota.uploading": "Le firmware est téléversé et écrit dans la partition OTA inactive.",
    "ota.verifying": "Téléversement terminé. Le firmware est vérifié et activé.",
    "ota.success": "Mise à jour réussie. L’appareil redémarre.",
    "ota.reconnecting": "En attente du redémarrage …",
    "ota.reconnected": "L’appareil est à nouveau joignable. Rechargement de la page.",
    "ota.reconnectFailed": "Le redémarrage a été déclenché. Si la page ne revient pas, vérifiez la nouvelle IP dans la sortie série.",
    "ota.fileTooLarge": "Le firmware sélectionné est plus volumineux que la capacité OTA disponible.",
    "ota.invalidFile": "Sélectionnez un fichier firmware ESP32 compilé avec l’extension .bin.",
    "ota.networkError": "La connexion a été interrompue pendant la mise à jour. Le firmware précédent reste actif, sauf si l’ESP32 avait déjà terminé la mise à jour avec succès.",
    "ota.failedPrefix": "Échec de la mise à jour",
    "ota.stage.start": "Démarrage",
    "ota.stage.write": "Écriture flash",
    "ota.stage.finish": "Vérifier/activer",
    "ota.stage.aborted": "Transfert",
    "ota.stage.request": "Requête",
    "ota.stage.unknown": "Étape inconnue",
    "ota.reason.write": "Échec de l’écriture dans la flash.",
    "ota.reason.erase": "Échec de l’effacement de la zone flash cible.",
    "ota.reason.read": "Le nouveau firmware n’a pas pu être relu de manière fiable.",
    "ota.reason.space": "Le firmware est trop volumineux pour la partition OTA.",
    "ota.reason.size": "La taille du fichier transféré est invalide ou incomplète.",
    "ota.reason.stream": "Le transfert de données s’est arrêté ou a expiré.",
    "ota.reason.checksum": "La somme de contrôle du firmware ne correspond pas.",
    "ota.reason.image": "Le fichier n’est pas une image firmware ESP32 valide.",
    "ota.reason.activate": "Le nouveau firmware n’a pas pu être marqué comme amorçable.",
    "ota.reason.partition": "Aucune seconde partition d’application OTA appropriée n’a été trouvée.",
    "ota.reason.argument": "La requête OTA contient des données invalides.",
    "ota.reason.aborted": "Le téléversement a été interrompu par le navigateur ou le réseau.",
    "ota.reason.verify": "Échec de la vérification du firmware.",
    "ota.reason.unknown": "Erreur OTA inconnue.",
    "api.unavailable": "L’API de l’appareil est actuellement indisponible. Les valeurs déjà chargées restent visibles mais ne sont pas présentées comme nouvellement confirmées.",
    "hardware.gpio": "E/S numériques",
    "hardware.rtc": "RTC DS3231",
    "hardware.display": "OLED SH1106",
    "hardware.audio": "Audio DY-SV17F",
    "hardware.info.inputs": "Broches DI",
    "hardware.info.outputs": "Broches DO",
    "hardware.info.model": "Modèle",
    "hardware.info.transport": "Interface",
    "hardware.info.address": "Adresse",
    "hardware.info.rtcTime": "Heure RTC",
    "hardware.info.temperature": "Température",
    "hardware.info.osf": "Indicateur arrêt oscillateur",
    "hardware.info.resolution": "Résolution",
    "hardware.info.initialized": "Initialisé",
    "hardware.info.pins": "Broches",
    "hardware.info.playState": "Lecture",
    "hardware.info.onlineDevices": "État du périphérique",
    "hardware.info.fileCount": "Fichiers",
    "hardware.info.busy": "BUSY actif",
    "hardware.info.lastCheck": "Contrôle à uptime",
    "hardware.info.feedback": "Retour",
    "hardware.info.testTrack": "Piste test/démarrage",
    "hardware.feedback.none": "Aucun",
    "hardware.feedback.local_state": "État local ESP32",
    "hardware.feedback.transport_ack": "Bus/transport acquitté",
    "hardware.feedback.protocol_response": "Réponse protocole",
    "hardware.feedback.external_feedback": "Retour externe",
    "hardware.play.stopped": "Arrêté",
    "hardware.play.playing": "Lecture",
    "hardware.play.paused": "En pause",
    "hardware.play.unknown": "Inconnu",
    "hardware.check.pending": "Contrôle du matériel en cours …",
    "hardware.check.done": "Contrôle du matériel terminé.",
    "hardware.check.timeout": "Le contrôle du matériel prend plus de temps que prévu. Le dernier état connu reste affiché.",
    "hardware.action.failed": "L’action matérielle n’a pas pu être exécutée.",
    "time.priority.ntp.title": "1. NTP",
    "time.priority.ntp.desc": "Référence / priorité maximale",
    "time.priority.rtc.title": "2. RTC",
    "time.priority.rtc.desc": "Heure de départ / autonome",
    "time.priority.browser.title": "3. Navigateur",
    "time.priority.browser.desc": "Repli",
    "time.priority.relative.title": "4. Sans horloge",
    "time.priority.relative.desc": "relatif uniquement",
    "time.activeSource": "Source horaire active",
    "time.systemTime": "Heure système",
    "time.timeStatus": "État de l’heure",
    "time.lastCheck": "Dernier contrôle",
    "time.source.ntp": "NTP",
    "time.source.rtc": "RTC",
    "time.source.browser": "Navigateur",
    "time.source.relative": "Relatif uniquement",
    "time.source.none": "Aucune",
    "time.quality.reference": "Référence",
    "time.quality.valid": "Valide",
    "time.quality.fallback": "Repli",
    "time.quality.relative": "Aucune heure absolue",
    "time.quality.none": "Inconnu",
    "time.ntp.title": "NTP",
    "time.ntp.server": "Serveur NTP principal",
    "time.ntp.checkSave": "Vérifier et enregistrer",
    "time.ntp.hint": "Le serveur NTP principal saisi est testé et n’est enregistré durablement qu’après une réponse valide.",
    "time.browser.title": "Repli navigateur",
    "time.browser.hint": "Uniquement lorsque ni NTP ni RTC ne fournissent une heure valide, le navigateur peut transmettre son horloge une seule fois.",
    "time.differences.title": "Sources horaires et écarts",
    "time.reference": "Référence",
    "time.delta": "Écart",
    "time.sample.ntp": "NTP",
    "time.sample.rtc": "RTC",
    "time.sample.rtcBefore": "RTC avant synchronisation NTP",
    "time.sample.system": "Système",
    "time.sample.browser": "Navigateur",
    "time.rtcSync": "Synchronisation RTC",
    "time.rtcSync.ok": "Synchronisé avec succès depuis NTP",
    "time.rtcSync.failed": "Échec de la synchronisation",
    "time.rtcSync.none": "Non nécessaire pour ce contrôle",
    "time.check": "Vérifier l’heure",
    "time.checking": "Vérification des sources horaires …",
    "time.checked": "Contrôle de l’heure terminé.",
    "time.ntpChecking": "Vérification du serveur NTP …",
    "time.ntpSaved": "Serveur NTP validé, enregistré et adopté comme référence.",
    "time.browserAccepted": "L’heure du navigateur a été acceptée comme solution de repli.",
    "time.unavailable": "Indisponible",
    "time.invalid": "Invalide",
    "time.valid": "Valide",
    "time.error.no_network": "Aucun réseau Wi‑Fi connecté n’est disponible pour NTP.",
    "time.error.invalid_server": "Le serveur NTP est invalide.",
    "time.error.dns_error": "Le serveur NTP n’a pas pu être résolu.",
    "time.error.socket_error": "UDP n’a pas pu être démarré pour la requête NTP.",
    "time.error.send_error": "La requête NTP n’a pas pu être envoyée.",
    "time.error.timeout": "Le serveur NTP n’a pas répondu à temps.",
    "time.error.invalid_response": "La réponse NTP est invalide.",
    "time.error.server_unsynchronized": "Le serveur NTP indique qu’il n’est pas synchronisé.",
    "time.error.implausible_time": "L’heure reçue n’est pas plausible.",
    "time.error.persist_failed": "Le serveur a répondu mais n’a pas pu être enregistré durablement.",
    "time.error.browser_time_invalid": "L’heure du navigateur n’est pas plausible.",
    "time.error.browser_disabled": "Le repli navigateur est désactivé.",
    "time.error.higher_priority_source": "Une source horaire de priorité supérieure est déjà valide.",
    "time.error.busy": "Un contrôle de l’heure est déjà en cours.",
    "time.error.unknown": "Erreur horaire inconnue.",
    "common.none": "—",
    "common.on": "ON",
    "common.off": "OFF",
    "common.yes": "Oui",
    "common.no": "Non",
    "footer.github": "GitHub",
    "footer.project": "Projet",
    "aria.primaryNav": "Navigation principale",
    "aria.status": "État du système",
    "time.week": "Sem."
};

  I18N['swg-alb'] = {
    'analytics.yearMonth.desc': 'D letzte fünf Johr nach Monat – Anzahl oder Ø Abstand.',
    'analytics.monthWeek.desc': 'Monat ond Kalenderwocha – Anzahl oder Ø Abstand.',
    'analytics.hourly.desc': 'Wochentäg ond Stunda – Anzahl oder Ø Abstand bis zom nächste Druck am selba Tag.',
    'view.analytics.desc': 'D Heatmaps zeiget Anzahl oder dr durchschnittlich fertige Abstand zwischa de Unterbrechunga.',
    'project.displayEnabled': 'Display',
    'analytics.metric': 'Metrik',
    'analytics.metric.count': 'Anzahl',
    'analytics.metric.averageInterval': 'Ø Abstand',
    'analytics.intervalSamples': '{n} Abständ',
    'analytics.coveragePartial': 'Dr Ø-Abstand kommt aus de Rohereignisse, wo no do send.',
    ...I18N.swg,
    'nav.analytics': 'Auswertig',
    'nav.device': 'Grät',
    'nav.settings': 'Eistellunga',
    'interruptions.title': 'Unterbrechungszähler',
    'interruptions.desc': 'Jeder Druck uff da Knopf oder Klick em Web wird glei als Unterbrechung erfasst.',
    'interruptions.today': 'Unterbrechunga heit',
    'interruptions.last': 'Letschte Unterbrechung',
    'interruptions.button': 'Unterbrechung',
    'interruptions.sound': 'Ton bei dr Unterbrechung',
    'interruptions.justNow': 'grad ebba',
    'interruptions.never': 'no koi',
    'project.settings.title': 'Rückmeldung & Display',
    'project.settings.desc': 'Rückmeldung am Grät ond d OLED-Anzeig. Ändrunga geltet glei ond bleibet em ESP32 gspeichert.',
    'project.soundMode.rotate': 'Wechselnd – jedes Mol dr nächschte Track',
    'project.displayMode.standard': 'Standard: heit + letschte Unterbrechung',
    'analytics.hourly.title': 'Wochadäg / Stonda',
    'analytics.monthWeek.title': 'Monat / Kalenderwocha',
    'analytics.yearMonth.title': 'Letschte 5 Johr / Monat',
    'view.device.title': 'Grät',
    'card.device': 'Grät',
    'card.time': 'Zeitverwaltig',
    'card.language': 'Sproch',
    'label.language': 'Sproch',
    'status.connected': 'Verbunda',
    'status.disconnected': 'Net verbunda',
    'status.warning': 'Obacht',
    'action.refresh': 'Neu lada',
    'action.retry': 'No mol probiera',
    'time.check': 'Zeit prüafa',
    'time.checking': 'Zeitquella werdet grad prüaft …',
    'common.yes': 'Jo',
    'common.no': 'Noi'
  };

  I18N['swg-ob'] = {
    'analytics.yearMonth.desc': 'D letzte fünf Johr nach Monat – Anzahl oder Ø Abstand.',
    'analytics.monthWeek.desc': 'Monat ond Kalenderwocha – Anzahl oder Ø Abstand.',
    'analytics.hourly.desc': 'Wochentäg ond Stunda – Anzahl oder Ø Abstand bis zur nächste Unterbrechung am selba Tag.',
    'view.analytics.desc': 'D Heatmaps zeiget Anzahl oder dr durchschnittlich abgeschlossene Abstand zwischa de Unterbrechunga.',
    'project.displayEnabled': 'Display',
    'analytics.metric': 'Metrik',
    'analytics.metric.count': 'Anzahl',
    'analytics.metric.averageInterval': 'Ø Abstand',
    'analytics.intervalSamples': '{n} Abständ',
    'analytics.coveragePartial': 'Dr Ø-Abstand basiert auf de Rohereignisse, wo no gspeichert send.',
    ...I18N.swg,
    'nav.analytics': 'Auswertung',
    'nav.device': 'Gerät',
    'nav.settings': 'Einstellungen',
    'interruptions.desc': 'Jeder Druck auf da Knopf oder Klick im Web wird glei als Unterbrechung gspeichert.',
    'interruptions.today': 'Unterbrechunga heut',
    'interruptions.last': 'Letzte Unterbrechung',
    'interruptions.sound': 'Ton bei dr Unterbrechung',
    'interruptions.justNow': 'grad jetzt',
    'interruptions.never': 'no koine',
    'project.settings.title': 'Rückmeldung & Display',
    'project.settings.desc': 'Rückmeldung am Gerät und d OLED-Anzeige. Änderungen geltet sofort und bleibet auf em ESP32 gspeichert.',
    'project.soundMode.rotate': 'Abwechselnd – jedes Mol dr nächste Track',
    'project.displayMode.standard': 'Standard: heut + letzte Unterbrechung',
    'analytics.hourly.title': 'Wochentäg / Stunda',
    'analytics.monthWeek.title': 'Monat / Kalenderwocha',
    'analytics.yearMonth.title': 'Letzte 5 Johr / Monat',
    'card.time': 'Zeitverwaltung',
    'card.language': 'Sproch',
    'label.language': 'Sproch',
    'status.connected': 'Verbunda',
    'status.disconnected': 'Net verbunda',
    'status.warning': 'Obacht',
    'action.refresh': 'Neu lada',
    'action.retry': 'Nomol probiera',
    'time.check': 'Zeit prüfa',
    'time.checking': 'Zeitquella werdet grad prüft …',
    'common.yes': 'Jo',
    'common.no': 'Nai'
  };


  const I18N_330 = {
    de: {
      'analytics.source': 'Herkunft', 'analytics.source.all': 'Beides', 'analytics.source.physical_button': 'Knopf / GPIO', 'analytics.source.web_button': 'Web',
      'analytics.sourceCoveragePartial': 'Der Herkunftsfilter basiert auf den noch vorhandenen Rohereignissen.',
      'analytics.storageState': 'Datenspeicher', 'analytics.rawProblem': 'Rohdaten-Problem', 'analytics.aggregateProblem': 'Statistik-Problem',
      'analytics.databaseDeleteTitle': 'Datenbank löschen', 'analytics.databaseDeletePassword': 'Passwort / Projektname',
      'analytics.databaseDeleteHint': 'Zum vollständigen Löschen aller Ereignisse und Statistiken den Projektname eingeben.',
      'analytics.databaseDeleteAction': 'Datenbank vollständig löschen', 'analytics.databaseDeleteConfirm': 'Alle gespeicherten Ereignisse und Statistiken wirklich löschen?',
      'analytics.databaseDeleteWrong': 'Passwort stimmt nicht. Erwartet wird der Projektname.', 'analytics.databaseDeleteFailed': 'Datenbank konnte nicht vollständig gelöscht werden.',
      'analytics.databaseDeleteSuccess': 'Datenbank gelöscht. Gerät startet neu.'
    },
    en: {
      'analytics.source': 'Source', 'analytics.source.all': 'Both', 'analytics.source.physical_button': 'Button / GPIO', 'analytics.source.web_button': 'Web',
      'analytics.sourceCoveragePartial': 'The source filter is based on the raw events still retained on the device.',
      'analytics.storageState': 'Data storage', 'analytics.rawProblem': 'Raw-data problem', 'analytics.aggregateProblem': 'Statistics problem',
      'analytics.databaseDeleteTitle': 'Delete database', 'analytics.databaseDeletePassword': 'Password / project name',
      'analytics.databaseDeleteHint': 'Enter the project name to permanently delete all events and statistics.',
      'analytics.databaseDeleteAction': 'Delete complete database', 'analytics.databaseDeleteConfirm': 'Really delete all stored events and statistics?',
      'analytics.databaseDeleteWrong': 'Wrong password. The project name is required.', 'analytics.databaseDeleteFailed': 'The database could not be deleted completely.',
      'analytics.databaseDeleteSuccess': 'Database deleted. Device is restarting.'
    },
    it: {
      'analytics.source': 'Origine', 'analytics.source.all': 'Entrambi', 'analytics.source.physical_button': 'Pulsante / GPIO', 'analytics.source.web_button': 'Web',
      'analytics.sourceCoveragePartial': 'Il filtro origine usa gli eventi grezzi ancora disponibili sul dispositivo.',
      'analytics.storageState': 'Memoria dati', 'analytics.rawProblem': 'Problema dati grezzi', 'analytics.aggregateProblem': 'Problema statistiche',
      'analytics.databaseDeleteTitle': 'Elimina database', 'analytics.databaseDeletePassword': 'Password / nome progetto',
      'analytics.databaseDeleteHint': 'Inserire il nome del progetto per eliminare definitivamente eventi e statistiche.',
      'analytics.databaseDeleteAction': 'Elimina tutto il database', 'analytics.databaseDeleteConfirm': 'Eliminare davvero tutti gli eventi e le statistiche?',
      'analytics.databaseDeleteWrong': 'Password errata. È richiesto il nome del progetto.', 'analytics.databaseDeleteFailed': 'Impossibile eliminare completamente il database.',
      'analytics.databaseDeleteSuccess': 'Database eliminato. Il dispositivo si riavvia.'
    },
    fr: {
      'analytics.source': 'Origine', 'analytics.source.all': 'Les deux', 'analytics.source.physical_button': 'Bouton / GPIO', 'analytics.source.web_button': 'Web',
      'analytics.sourceCoveragePartial': 'Le filtre d’origine utilise les événements bruts encore conservés sur l’appareil.',
      'analytics.storageState': 'Stockage des données', 'analytics.rawProblem': 'Problème données brutes', 'analytics.aggregateProblem': 'Problème statistiques',
      'analytics.databaseDeleteTitle': 'Supprimer la base', 'analytics.databaseDeletePassword': 'Mot de passe / nom du projet',
      'analytics.databaseDeleteHint': 'Saisir le nom du projet pour supprimer définitivement tous les événements et statistiques.',
      'analytics.databaseDeleteAction': 'Supprimer toute la base', 'analytics.databaseDeleteConfirm': 'Supprimer vraiment tous les événements et statistiques ?',
      'analytics.databaseDeleteWrong': 'Mot de passe incorrect. Le nom du projet est requis.', 'analytics.databaseDeleteFailed': 'La base n’a pas pu être entièrement supprimée.',
      'analytics.databaseDeleteSuccess': 'Base supprimée. L’appareil redémarre.'
    },
    swg: {
      'analytics.source': 'Herkunft', 'analytics.source.all': 'Beides', 'analytics.source.physical_button': 'Knopf / GPIO', 'analytics.source.web_button': 'Web',
      'analytics.sourceCoveragePartial': 'Dr Herkunftsfilter nimmt bloß no die Rohereignis, wo no em Speicher send.',
      'analytics.storageState': 'Datenspeicher', 'analytics.rawProblem': 'Rohdata-Problem', 'analytics.aggregateProblem': 'Statistik-Problem',
      'analytics.databaseDeleteTitle': 'Datenbank löscha', 'analytics.databaseDeletePassword': 'Passwort / Projektname',
      'analytics.databaseDeleteHint': 'Zum komplette Löscha vom ganze Zeug dr Projektname eigeba.',
      'analytics.databaseDeleteAction': 'Datenbank komplett löscha', 'analytics.databaseDeleteConfirm': 'Wirklich alle Ereignis ond Statistika löscha?',
      'analytics.databaseDeleteWrong': 'Passwort passt net. Dr Projektname wird braucht.', 'analytics.databaseDeleteFailed': 'D Datenbank hot sich net komplett löscha lassa.',
      'analytics.databaseDeleteSuccess': 'Datenbank isch weg. S Gerät startet neu.'
    },
    'swg-alb': {
      'analytics.source': 'Herkunft', 'analytics.source.all': 'Beides', 'analytics.source.physical_button': 'Knopf / GPIO', 'analytics.source.web_button': 'Web',
      'analytics.sourceCoveragePartial': 'Dr Herkunftsfilter nimmt dia Rohereignis, wo no em Speicher send.',
      'analytics.storageState': 'Datenspeicher', 'analytics.rawProblem': 'Rohdata-Problem', 'analytics.aggregateProblem': 'Statistik-Problem',
      'analytics.databaseDeleteTitle': 'Datenbank löscha', 'analytics.databaseDeletePassword': 'Passwort / Projektname',
      'analytics.databaseDeleteHint': 'Zum komplette Löscha dr Projektname eigeba.',
      'analytics.databaseDeleteAction': 'Datenbank komplett löscha', 'analytics.databaseDeleteConfirm': 'Wirklich alles löscha?',
      'analytics.databaseDeleteWrong': 'Passwort passt net. Dr Projektname wird braucht.', 'analytics.databaseDeleteFailed': 'D Datenbank hot sich net komplett löscha lassa.',
      'analytics.databaseDeleteSuccess': 'Datenbank isch weg. S Gerät startet neu.'
    },
    'swg-ob': {
      'analytics.source': 'Herkunft', 'analytics.source.all': 'Beides', 'analytics.source.physical_button': 'Knopf / GPIO', 'analytics.source.web_button': 'Web',
      'analytics.sourceCoveragePartial': 'Dr Herkunftsfilter nimmt bloß no dia Rohereignis, wo no gspeichert send.',
      'analytics.storageState': 'Datenspeicher', 'analytics.rawProblem': 'Rohdata-Problem', 'analytics.aggregateProblem': 'Statistik-Problem',
      'analytics.databaseDeleteTitle': 'Datenbank löscha', 'analytics.databaseDeletePassword': 'Passwort / Projektname',
      'analytics.databaseDeleteHint': 'Zum komplette Löscha dr Projektname eigeba.',
      'analytics.databaseDeleteAction': 'Datenbank komplett löscha', 'analytics.databaseDeleteConfirm': 'Wirklich alle Ereignis ond Statistika löscha?',
      'analytics.databaseDeleteWrong': 'Passwort passt it. Dr Projektname wird braucht.', 'analytics.databaseDeleteFailed': 'D Datenbank hot sich it komplett löscha lassa.',
      'analytics.databaseDeleteSuccess': 'Datenbank isch weg. S Gerät startet neu.'
    }
  };
  Object.entries(I18N_330).forEach(([code, labels]) => Object.assign(I18N[code], labels));

  const STORAGE_STATUS_LABELS = {
    de: { 'status.ready': 'Bereit', 'status.unavailable': 'Nicht verfügbar' },
    en: { 'status.ready': 'Ready', 'status.unavailable': 'Unavailable' },
    it: { 'status.ready': 'Pronto', 'status.unavailable': 'Non disponibile' },
    fr: { 'status.ready': 'Prêt', 'status.unavailable': 'Indisponible' },
    swg: { 'status.ready': 'Bereit', 'status.unavailable': 'Net verfügbar' },
    'swg-alb': { 'status.ready': 'Bereit', 'status.unavailable': 'Net verfügbar' },
    'swg-ob': { 'status.ready': 'Bereit', 'status.unavailable': 'It verfügbar' }
  };
  Object.entries(STORAGE_STATUS_LABELS).forEach(([code, labels]) => Object.assign(I18N[code], labels));

  const LANGUAGE_LABELS = {
    de: { 'lang.it': 'Italienisch', 'lang.fr': 'Französisch', 'lang.swgAlb': 'Alb-Schwäbisch', 'lang.swgOb': 'Oberschwäbisch' },
    en: { 'lang.it': 'Italian', 'lang.fr': 'French', 'lang.swgAlb': 'Swabian (Alb)', 'lang.swgOb': 'Upper Swabian' },
    swg: { 'lang.it': 'Italienisch', 'lang.fr': 'Französisch', 'lang.swgAlb': 'Alb-Schwäbisch', 'lang.swgOb': 'Oberschwäbisch' },
    it: { 'lang.it': 'Italiano', 'lang.fr': 'Francese', 'lang.swgAlb': 'Svevo dell’Alb', 'lang.swgOb': 'Svevo superiore' },
    fr: { 'lang.it': 'Italien', 'lang.fr': 'Français', 'lang.swgAlb': 'Souabe de l’Alb', 'lang.swgOb': 'Haut-souabe' },
    'swg-alb': { 'lang.it': 'Italienisch', 'lang.fr': 'Französisch', 'lang.swgAlb': 'Alb-Schwäbisch', 'lang.swgOb': 'Oberschwäbisch' },
    'swg-ob': { 'lang.it': 'Italienisch', 'lang.fr': 'Französisch', 'lang.swgAlb': 'Alb-Schwäbisch', 'lang.swgOb': 'Oberschwäbisch' }
  };
  Object.entries(LANGUAGE_LABELS).forEach(([code, labels]) => Object.assign(I18N[code], labels));


  const UI_CONFIG = {
    navigation: [
      { id: 'home', key: 'nav.home', icon: 'home', order: 10, visible: true },
      { id: 'analytics', key: 'nav.analytics', icon: 'analytics', order: 20, visible: true },
      { id: 'device', key: 'nav.device', icon: 'device', order: 30, visible: true },
      { id: 'settings', key: 'nav.settings', icon: 'settings', order: 40, visible: true }
    ],
    status: [
      { id: 'wifi', path: 'status.wifi', key: 'status.wifi', icon: 'wifi', visible: true },
      { id: 'api', path: 'status.api', key: 'status.api', icon: 'server', visible: true }
    ],
    meters: {
      wifiRssi: {
        min: -100, max: -50, segments: 10,
        quality(value) {
          if (value >= -60) return 'good';
          if (value >= -70) return 'normal';
          if (value >= -78) return 'notice';
          if (value >= -85) return 'warning';
          return 'critical';
        }
      },
      heapUsed: {
        min: 0, max: 100, segments: 20,
        quality(value) {
          if (value <= 60) return 'good';
          if (value <= 75) return 'normal';
          if (value <= 85) return 'notice';
          if (value <= 92) return 'warning';
          return 'critical';
        }
      }
    },
    views: {
      home: {
        titleKey: 'view.home.title', descriptionKey: 'interruptions.desc',
        cards: [
          { id: 'interruptions-home', titleKey: 'interruptions.title', descriptionKey: 'interruptions.desc', icon: 'interrupt', width: 'full', components: [
            { type: 'interruptionHome' }
          ] },
          { id: 'project-settings', titleKey: 'project.settings.title', descriptionKey: 'project.settings.desc', icon: 'settings', width: 'full', components: [
            { type: 'projectSettings' }
          ] }
        ]
      },
      analytics: {
        titleKey: 'view.analytics.title', descriptionKey: 'view.analytics.desc',
        cards: [
          { id: 'heatmap-hourly', titleKey: 'analytics.hourly.title', descriptionKey: 'analytics.hourly.desc', icon: 'analytics', width: 'full', components: [{ type: 'heatmapHourly' }] },
          { id: 'heatmap-month-week', titleKey: 'analytics.monthWeek.title', descriptionKey: 'analytics.monthWeek.desc', icon: 'calendar', width: 'full', components: [{ type: 'heatmapMonthWeek' }] },
          { id: 'heatmap-year-month', titleKey: 'analytics.yearMonth.title', descriptionKey: 'analytics.yearMonth.desc', icon: 'calendar', width: 'full', components: [{ type: 'heatmapYearMonth' }] },
          { id: 'analytics-storage', titleKey: 'analytics.storage.title', descriptionKey: 'analytics.storage.desc', icon: 'memory', width: 'full', components: [{ type: 'analyticsStorage' }] }
        ]
      },
      device: {
        titleKey: 'view.device.title', descriptionKey: 'view.device.desc',
        cards: [
          { id: 'api-error', titleKey: 'status.api', icon: 'server', width: 'full', visibleWhen: { path: 'status.api', equals: 'error' }, components: [
            { type: 'notice', kind: 'error', textKey: 'api.unavailable' }
          ] },
          { id: 'device', titleKey: 'card.device', descriptionKey: 'card.device.desc', icon: 'device', width: 'normal', components: [
            { type: 'kv', items: [
              { labelKey: 'label.firmware', path: 'device.firmware' },
              { labelKey: 'label.version', path: 'device.version' },
              { labelKey: 'label.uptime', path: 'device.uptimeMs', format: 'uptime' },
              { labelKey: 'label.board', path: 'device.board' },
              { labelKey: 'label.chip', path: 'device.chip' },
              { labelKey: 'label.cores', path: 'device.cores' },
              { labelKey: 'label.flash', path: 'device.flashBytes', format: 'bytes' },
              { labelKey: 'label.psram', path: 'device.psramBytes', format: 'bytes' }
            ] }
          ] },
          { id: 'wifi', titleKey: 'card.wifi', descriptionKey: 'card.wifi.desc', icon: 'wifi', width: 'normal', components: [
            { type: 'status', labelKey: 'status.wifi', path: 'wifi.state' },
            { type: 'kv', items: [
              { labelKey: 'label.network', path: 'wifi.ssid' },
              { labelKey: 'label.ip', path: 'wifi.ip' },
              { labelKey: 'label.signal', path: 'wifi.rssi', format: 'dbm' }
            ] },
            { type: 'meter', labelKey: 'label.signal', path: 'wifi.rssi', policy: 'wifiRssi', format: 'dbm', visibleWhen: { path: 'wifi.connected', equals: true } }
          ] },
          { id: 'memory', titleKey: 'card.memory', descriptionKey: 'card.memory.desc', icon: 'memory', width: 'normal', components: [
            { type: 'kv', items: [
              { labelKey: 'label.heapTotal', path: 'memory.heapTotal', format: 'bytes' },
              { labelKey: 'label.heapFree', path: 'memory.heapFree', format: 'bytes' },
              { labelKey: 'label.heapMin', path: 'memory.heapMin', format: 'bytes' }
            ] },
            { type: 'meter', labelKey: 'label.heapUsed', path: 'memory.heapUsedPercent', policy: 'heapUsed', format: 'percent' },
            { type: 'status', labelKey: 'analytics.storageState', path: 'storage.state' },
            { type: 'kv', items: [
              { labelKey: 'analytics.rawProblem', path: 'storage.rawError' },
              { labelKey: 'analytics.aggregateProblem', path: 'storage.aggregateError' }
            ] }
          ] },
          { id: 'time', titleKey: 'card.time', descriptionKey: 'card.time.desc', icon: 'clock', width: 'full', components: [
            { type: 'timeManagement' }
          ] },
          { id: 'hardware', titleKey: 'card.hardware', descriptionKey: 'card.hardware.desc', icon: 'hardware', width: 'full', components: [
            { type: 'hardware', path: 'hardware.modules' },
            { type: 'action', actions: [
              { id: 'hardware-check-all', labelKey: 'action.hardwareCheckAll', icon: 'refresh' }
            ] }
          ] },
          { id: 'ota', titleKey: 'card.ota', descriptionKey: 'card.ota.desc', icon: 'upload', width: 'full', components: [
            { type: 'kv', items: [
              { labelKey: 'label.firmwareImage', path: 'ota.currentBytes', format: 'bytes' },
              { labelKey: 'label.otaCapacity', path: 'ota.maxBytes', format: 'bytes' },
              { labelKey: 'label.projectReserve', path: 'ota.headroomBytes', format: 'bytes' }
            ] },
            { type: 'meter', labelKey: 'label.otaCapacity', path: 'ota.usedPercent', policy: 'heapUsed', format: 'percent' },
            { type: 'upload', id: 'firmware', endpoint: '/api/ota', fieldName: 'firmware', accept: '.bin,application/octet-stream', labelKey: 'ota.file', buttonKey: 'action.otaUpload', maxPath: 'ota.maxBytes', visibleWhen: { path: 'ota.supported', equals: true } },
          ] }
        ]
      },
      settings: {
        titleKey: 'view.settings.title', descriptionKey: 'view.settings.desc',
        cards: [
          { id: 'language', titleKey: 'card.language', descriptionKey: 'card.language.desc', icon: 'language', width: 'normal', components: [
            { type: 'select', id: 'language', labelKey: 'label.language', preference: 'language', options: [
              { value: 'de', key: 'lang.de' }, { value: 'en', key: 'lang.en' }, { value: 'it', key: 'lang.it' }, { value: 'fr', key: 'lang.fr' }, { value: 'swg', key: 'lang.swg' }, { value: 'swg-alb', key: 'lang.swgAlb' }, { value: 'swg-ob', key: 'lang.swgOb' }
            ], note: 'languageMode' }
          ] },
          { id: 'theme', titleKey: 'card.theme', descriptionKey: 'card.theme.desc', icon: 'theme', width: 'normal', components: [
            { type: 'select', id: 'theme', labelKey: 'label.theme', preference: 'theme', options: [
              { value: 'system', key: 'theme.system' }, { value: 'light', key: 'theme.light' }, { value: 'dark', key: 'theme.dark' }
            ] }
          ] }
        ]
      }
    }
  };


  Object.assign(I18N.de, {
    'project.section.display': 'Display',
    'project.section.displayFeedback': 'Display-Feedback',
    'project.section.sound': 'Ton / DY-SV17F',
    'project.soundVolume': 'Lautstärke',
    'project.displayRotation180': 'Display um 180° drehen',
    'project.displayMode.dayProgress': 'Tagesfortschritt – Heute + Ø Abstand',
    'project.displayMode.focus': 'Fokus – Zeit seit letzter Unterbrechung',
    'project.settings.title': 'Hardware-Feedback',
    'project.settings.desc': 'Display, Display-Feedback und Ton sind getrennt gruppiert. Änderungen gelten sofort und bleiben im ESP32 gespeichert.',
    'card.language.desc': 'Die Sprache gilt für Weboberfläche und OLED und wird auf dem ESP32 gespeichert.'
  });
  Object.assign(I18N.en, {
    'project.section.display': 'Display',
    'project.section.displayFeedback': 'Display feedback',
    'project.section.sound': 'Sound / DY-SV17F',
    'project.soundVolume': 'Volume',
    'project.displayRotation180': 'Rotate display 180°',
    'project.displayMode.dayProgress': 'Day progress – today + average interval',
    'project.displayMode.focus': 'Focus – time since last interruption',
    'project.settings.title': 'Hardware feedback',
    'project.settings.desc': 'Display, display feedback and sound are grouped separately. Changes apply immediately and remain stored on the ESP32.',
    'card.language.desc': 'The language applies to both the web interface and OLED and is stored on the ESP32.'
  });
  Object.assign(I18N.it, {
    'project.section.display': 'Display',
    'project.section.displayFeedback': 'Feedback display',
    'project.section.sound': 'Audio / DY-SV17F',
    'project.soundVolume': 'Volume',
    'project.displayRotation180': 'Ruota display di 180°',
    'project.displayMode.dayProgress': 'Progresso giornaliero – oggi + intervallo medio',
    'project.displayMode.focus': 'Focus – tempo dall’ultima interruzione',
    'project.settings.title': 'Feedback hardware',
    'project.settings.desc': 'Display, feedback del display e audio sono raggruppati separatamente. Le modifiche vengono applicate subito e salvate nell’ESP32.',
    'card.language.desc': 'La lingua vale per interfaccia web e OLED e viene salvata nell’ESP32.'
  });
  Object.assign(I18N.fr, {
    'project.section.display': 'Affichage',
    'project.section.displayFeedback': 'Retour d’affichage',
    'project.section.sound': 'Son / DY-SV17F',
    'project.soundVolume': 'Volume',
    'project.displayRotation180': 'Tourner l’affichage de 180°',
    'project.displayMode.dayProgress': 'Progression du jour – aujourd’hui + intervalle moyen',
    'project.displayMode.focus': 'Focus – temps depuis la dernière interruption',
    'project.settings.title': 'Retour matériel',
    'project.settings.desc': 'Affichage, retour visuel et son sont regroupés séparément. Les modifications sont immédiates et enregistrées sur l’ESP32.',
    'card.language.desc': 'La langue s’applique à l’interface web et à l’OLED et est enregistrée sur l’ESP32.'
  });
  Object.assign(I18N.swg, {
    'project.section.display': 'Display',
    'project.section.displayFeedback': 'Display-Rückmeldung',
    'project.section.sound': 'Ton / DY-SV17F',
    'project.soundVolume': 'Lautstärk',
    'project.displayRotation180': 'Display um 180° dreha',
    'project.displayMode.dayProgress': 'Tagesverlauf – heit + Ø Abstand',
    'project.displayMode.focus': 'Fokus – Zeit seit dr letschta Unterbrechung',
    'project.settings.title': 'Hardware-Rückmeldung',
    'project.settings.desc': 'Display, Display-Rückmeldung ond Ton send sauber trennt. Ändrunga geltet glei ond bleibet em ESP32 gspeichert.',
    'card.language.desc': 'D Sproch gilt fürs Web ond fürs OLED ond wird em ESP32 gspeichert.'
  });
  Object.assign(I18N['swg-alb'], {
    'project.section.display': 'Display',
    'project.section.displayFeedback': 'Display-Rückmeldung',
    'project.section.sound': 'Ton / DY-SV17F',
    'project.soundVolume': 'Lautstärk',
    'project.displayRotation180': 'Display um 180° dreha',
    'project.displayMode.dayProgress': 'Tagesverlauf – heit + Ø Abstand',
    'project.displayMode.focus': 'Fokus – Zeit seit dr letschta Unterbrechung',
    'project.settings.title': 'Hardware-Rückmeldung',
    'project.settings.desc': 'Display, Display-Rückmeldung ond Ton send extra gruppiert. Ändrunga geltet glei ond bleibet gspeichert.',
    'card.language.desc': 'D Sproch gilt fürs Web ond OLED ond bleibt em ESP32 gspeichert.'
  });
  Object.assign(I18N['swg-ob'], {
    'project.section.display': 'Display',
    'project.section.displayFeedback': 'Display-Rückmeldung',
    'project.section.sound': 'Ton / DY-SV17F',
    'project.soundVolume': 'Lautstärk',
    'project.displayRotation180': 'Display um 180° dreha',
    'project.displayMode.dayProgress': 'Tagesverlauf – heit + Ø Abstand',
    'project.displayMode.focus': 'Fokus – Zeit seit dr letschta Unterbrechung',
    'project.settings.title': 'Hardware-Rückmeldung',
    'project.settings.desc': 'Display, Display-Rückmeldung ond Ton send getrennt gruppiert. Ändrunga geltet sofort ond bleibet gspeichert.',
    'card.language.desc': 'D Sproch gilt fürs Web ond OLED ond bleibt em ESP32 gspeichert.'
  });

  const state = {
    project: { name: 'ESP32 UI', icon: 'chip', version: '', footerComment: '', githubUser: '', githubUserUrl: '', projectUrl: '', timeZone: 'Europe/Berlin' },
    firmware: {},
    preferences: { language: 'en', languageMode: 'auto', theme: 'system', fallbackLanguage: 'en', availableLanguages: Object.keys(I18N), languageStored: false, themeStored: false },
    status: { wifi: 'unknown', time: 'unknown', api: 'busy', gpio: 'disabled', rtc: 'disabled', display: 'disabled', audio: 'disabled', data: 'unknown' },
    connection: { deviceLoading: false, deviceRequested: false, deviceLoaded: false },
    device: {}, wifi: {}, memory: {}, hardware: { checking: false, modules: [] }, ota: { supported: null, currentBytes: 0, maxBytes: 0, headroomBytes: 0, usedPercent: 0 },
    interruptions: { todayCount: 0, unassignedCount: 0, sequence: 0, persistedSequence: 0, pendingCount: 0, droppedCount: 0, storageState: 'unavailable', soundEnabled: true, last: { available: false } },
    projectSettings: { soundEnabled: true, soundVolume: 100, soundMode: 'rotate', soundTrack: 2, soundTrackCount: 0, language: 'en', languageStored: false, displayEnabled: true, displayRotation180: false, displayFlashEnabled: true, displayMode: 'standard', displayBrightness: 65, displayDimAfterMinutes: 10, displayDimBrightness: 5 },
    analytics: { loaded: false, loading: false, dirty: false, error: '', storage: null, hourly: null, monthWeek: null, yearMonth: null, hourlyMode: 'week', metric: 'count', source: 'all' },
    time: { valid: false, source: 'relative', quality: 'relative', epochMs: 0, syncPerf: 0 },
    timeManagement: { activeSource: 'relative', quality: 'relative', valid: false, ntpServer: '', browserFallbackAllowed: false, ntp: {}, rtc: {}, browser: {}, system: {} },
    uptime: { baseMs: 0, syncPerf: 0 },
    activeView: 'home'
  };

  const PreferenceStore = {
    prefix: 'espui.',
    get(key) {
      try { return localStorage.getItem(this.prefix + key); } catch (_) { return null; }
    },
    set(key, value) {
      try { localStorage.setItem(this.prefix + key, value); } catch (_) { /* browser storage is optional */ }
    }
  };

  const t = (key) => (I18N[state.preferences.language] && I18N[state.preferences.language][key]) || I18N.en[key] || key;

  function detectLanguage() {
    const supported = state.preferences.availableLanguages;
    const candidates = Array.isArray(navigator.languages) && navigator.languages.length ? navigator.languages : [navigator.language || ''];
    for (const raw of candidates) {
      const code = String(raw).toLowerCase();
      if (supported.includes(code)) return code;
      if (code.startsWith('it')) return 'it';
      if (code.startsWith('fr')) return 'fr';
      if (code.startsWith('swg')) return 'swg';
      if (code.startsWith('de')) return 'de';
      if (code.startsWith('en')) return 'en';
    }
    return state.preferences.fallbackLanguage || 'en';
  }

  function initPreferences() {
    const savedLanguage = PreferenceStore.get('language');
    if (state.preferences.availableLanguages.includes(savedLanguage)) {
      state.preferences.language = savedLanguage;
      state.preferences.languageMode = 'manual';
      state.preferences.languageStored = true;
    } else {
      state.preferences.language = detectLanguage();
      state.preferences.languageMode = 'auto';
      state.preferences.languageStored = false;
    }

    const savedTheme = PreferenceStore.get('theme');
    state.preferences.themeStored = ['system', 'light', 'dark'].includes(savedTheme);
    state.preferences.theme = state.preferences.themeStored ? savedTheme : 'system';
    applyTheme();
    document.documentElement.lang = state.preferences.language;
  }

  const systemTheme = matchMedia('(prefers-color-scheme: dark)');
  function applyTheme() {
    const mode = state.preferences.theme;
    const resolved = mode === 'system' ? (systemTheme.matches ? 'dark' : 'light') : mode;
    document.documentElement.dataset.theme = resolved;
  }
  const handleSystemThemeChange = () => { if (state.preferences.theme === 'system') applyTheme(); };
  if (systemTheme.addEventListener) systemTheme.addEventListener('change', handleSystemThemeChange);
  else if (systemTheme.addListener) systemTheme.addListener(handleSystemThemeChange);

  function setLanguage(value) {
    if (!state.preferences.availableLanguages.includes(value)) return;
    state.preferences.language = value;
    state.preferences.languageMode = 'manual';
    state.preferences.languageStored = true;
    PreferenceStore.set('language', value);
    Transport.setProjectPreference('language', value);
    document.documentElement.lang = value;
    updateNavigationText();
    updateStatusBar();
    renderFooter();
    renderView(state.activeView);
  }

  function setTheme(value) {
    if (!['system', 'light', 'dark'].includes(value)) return;
    state.preferences.theme = value;
    state.preferences.themeStored = true;
    PreferenceStore.set('theme', value);
    applyTheme();
    Bindings.notify('preferences.theme');
  }

  function getPath(path) {
    return path.split('.').reduce((obj, part) => (obj == null ? undefined : obj[part]), state);
  }

  const Bindings = {
    items: new Map(),
    clearView() {
      for (const [path, list] of this.items) {
        const kept = list.filter(item => item.scope !== 'view');
        if (kept.length) this.items.set(path, kept); else this.items.delete(path);
      }
    },
    add(paths, update, scope = 'view') {
      for (const path of Array.isArray(paths) ? paths : [paths]) {
        if (!path) continue;
        const list = this.items.get(path) || [];
        list.push({ update, scope });
        this.items.set(path, list);
      }
      update();
    },
    notify(path) {
      const list = this.items.get(path);
      if (list) for (const item of list) item.update();
    },
    notifyMany(paths) { for (const path of paths) this.notify(path); }
  };

  function mergeState(target, source, prefix = '', changed = new Set()) {
    for (const [key, value] of Object.entries(source || {})) {
      const path = prefix ? `${prefix}.${key}` : key;
      if (value && typeof value === 'object' && !Array.isArray(value)) {
        if (!target[key] || typeof target[key] !== 'object') target[key] = {};
        mergeState(target[key], value, path, changed);
      } else if (target[key] !== value) {
        target[key] = value;
        changed.add(path);
      }
    }
    return changed;
  }

  function patchState(data) {
    const changed = mergeState(state, data);
    Bindings.notifyMany(changed);
    if ([...changed].some(path => path.startsWith('status.'))) updateStatusBar();
    return changed;
  }

  const Formats = {
    text(value) { return value == null || value === '' ? t('common.none') : String(value); },
    bytes(value) {
      if (!Number.isFinite(Number(value))) return t('common.none');
      const units = ['B', 'KiB', 'MiB', 'GiB'];
      let n = Number(value), i = 0;
      while (n >= 1024 && i < units.length - 1) { n /= 1024; i++; }
      const digits = n >= 100 || i === 0 ? 0 : n >= 10 ? 1 : 2;
      return `${n.toFixed(digits)} ${units[i]}`;
    },
    dbm(value) { return Number.isFinite(Number(value)) ? `${Number(value)} dBm` : t('common.none'); },
    percent(value) { return Number.isFinite(Number(value)) ? `${Math.round(Number(value))} %` : t('common.none'); },
    celsius(value) { return Number.isFinite(Number(value)) ? `${Number(value).toFixed(2)} °C` : t('common.none'); },
    dateTime(value) {
      const ms = Number(value);
      if (!Number.isFinite(ms) || ms <= 0) return t('common.none');
      const locale = state.preferences.language === 'en' ? 'en-GB' : 'de-DE';
      return new Date(ms).toLocaleString(locale, { day: '2-digit', month: '2-digit', year: 'numeric', hour: '2-digit', minute: '2-digit', second: '2-digit' });
    },
    deltaMs(value) {
      const ms = Number(value);
      if (!Number.isFinite(ms)) return t('common.none');
      const sign = ms > 0 ? '+' : ms < 0 ? '−' : '±';
      const abs = Math.abs(ms);
      if (abs < 1000) return `${sign}${Math.round(abs)} ms`;
      if (abs < 60000) return `${sign}${(abs / 1000).toFixed(abs < 10000 ? 3 : 1)} s`;
      return `${sign}${(abs / 60000).toFixed(2)} min`;
    },
    bool(value) { return value === true ? t('common.yes') : value === false ? t('common.no') : t('common.none'); },
    stateKey(value) { return t(`hardware.play.${value || 'unknown'}`); },
    checkTime(value) {
      const ms = Number(value);
      if (!Number.isFinite(ms) || ms <= 0) return t('common.none');
      let seconds = Math.floor(ms / 1000);
      const hours = Math.floor(seconds / 3600); seconds %= 3600;
      const minutes = Math.floor(seconds / 60); seconds %= 60;
      return `${String(hours).padStart(2, '0')}:${String(minutes).padStart(2, '0')}:${String(seconds).padStart(2, '0')} uptime`;
    },
    uptime() {
      if (!state.uptime.syncPerf) return t('common.none');
      const ms = state.uptime.baseMs + (performance.now() - state.uptime.syncPerf);
      let seconds = Math.max(0, Math.floor(ms / 1000));
      const days = Math.floor(seconds / 86400); seconds %= 86400;
      const hours = Math.floor(seconds / 3600); seconds %= 3600;
      const minutes = Math.floor(seconds / 60); seconds %= 60;
      if (days) return `${days} d ${String(hours).padStart(2, '0')}:${String(minutes).padStart(2, '0')}:${String(seconds).padStart(2, '0')}`;
      return `${String(hours).padStart(2, '0')}:${String(minutes).padStart(2, '0')}:${String(seconds).padStart(2, '0')}`;
    }
  };

  function formatValue(value, format) {
    if (format === 'uptime') return Formats.uptime();
    return (Formats[format] || Formats.text)(value);
  }

  function stateText(value) {
    const key = `status.${value || 'unknown'}`;
    return t(key);
  }

  function statusMark(value) {
    return ({ ok: '✓', ap: 'A', warning: '!', error: '×', inactive: '–', disabled: '–', checking: '…', no_response: '×', unknown: '?', busy: '…', disconnected: '×', stale: '!' })[value] || '?';
  }

  function updateFavicon(iconId) {
    const link = $('#favicon');
    const symbol = document.getElementById(`i-${iconId || 'chip'}`) || document.getElementById('i-chip');
    if (!link || !symbol) return;
    const svg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
    svg.setAttribute('xmlns', 'http://www.w3.org/2000/svg');
    svg.setAttribute('viewBox', symbol.getAttribute('viewBox') || '0 0 24 24');
    svg.setAttribute('fill', 'none');
    const accent = getComputedStyle(document.documentElement).getPropertyValue('--accent').trim() || '#2f81f7';
    svg.setAttribute('stroke', accent);
    svg.setAttribute('stroke-width', '1.8');
    svg.setAttribute('stroke-linecap', 'round');
    svg.setAttribute('stroke-linejoin', 'round');
    for (const child of symbol.children) svg.append(child.cloneNode(true));
    link.href = `data:image/svg+xml,${encodeURIComponent(new XMLSerializer().serializeToString(svg))}`;
  }

  function renderBrand() {
    const brand = $('#brand');
    const projectIcon = state.project.icon || 'chip';
    brand.replaceChildren(icon(projectIcon));
    const name = el('span');
    name.textContent = state.project.name || 'ESP32 UI';
    brand.append(name);
    document.title = state.project.name || 'ESP32 UI';
    updateFavicon(projectIcon);
  }

  function buildNavigation() {
    const nav = $('#navigation');
    nav.setAttribute('aria-label', t('aria.primaryNav'));
    nav.replaceChildren();
    const entries = UI_CONFIG.navigation.filter(item => item.visible !== false).sort((a, b) => a.order - b.order);
    for (const item of entries) {
      const button = el('button', 'nav-button');
      button.type = 'button';
      button.dataset.view = item.id;
      button.setAttribute('role', 'tab');
      button.setAttribute('aria-selected', item.id === state.activeView ? 'true' : 'false');
      button.append(icon(item.icon));
      const label = el('span', 'nav-label');
      label.dataset.navKey = item.key;
      label.textContent = t(item.key);
      button.append(label);
      button.setAttribute('aria-label', t(item.key));
      nav.append(button);
    }
  }

  function updateNavigationText() {
    $('#navigation').setAttribute('aria-label', t('aria.primaryNav'));
    for (const button of document.querySelectorAll('.nav-button')) {
      const def = UI_CONFIG.navigation.find(item => item.id === button.dataset.view);
      if (!def) continue;
      $('.nav-label', button).textContent = t(def.key);
      button.setAttribute('aria-label', t(def.key));
    }
  }

  function applyStatusProviders(providers) {
    const apiProvider = { id: 'api', path: 'status.api', key: 'status.api', icon: 'server', visible: true };
    const order = { wifi: 10, time: 15, gpio: 20, rtc: 30, display: 40, audio: 50, data: 55 };
    const mapped = Array.isArray(providers) ? providers.filter(item => item && item.visible !== false).map(item => ({
      id: item.id, path: `status.${item.id}`, key: item.key || `status.${item.id}`, icon: item.icon || 'hardware', visible: true
    })).sort((a, b) => (order[a.id] || 100) - (order[b.id] || 100)) : [];
    UI_CONFIG.status = [...mapped.filter(item => item.id !== 'api'), apiProvider];
    buildStatusBar();
  }

  function buildStatusBar() {
    const root = $('#headerStatus');
    root.setAttribute('aria-label', t('aria.status'));
    root.replaceChildren();
    for (const def of UI_CONFIG.status.filter(item => item.visible !== false)) {
      const node = el('span', 'status-icon');
      node.tabIndex = 0;
      node.dataset.statusId = def.id;
      node.append(icon(def.icon));
      root.append(node);
    }
    updateStatusBar();
  }

  function updateStatusBar() {
    const root = $('#headerStatus');
    root.setAttribute('aria-label', t('aria.status'));
    for (const def of UI_CONFIG.status.filter(item => item.visible !== false)) {
      const node = root.querySelector(`[data-status-id="${def.id}"]`);
      if (!node) continue;
      const value = getPath(def.path) || 'unknown';
      const detail = def.id === 'time' ? t(`time.source.${state.timeManagement?.activeSource || 'none'}`) : stateText(value);
      const description = `${t(def.key)}: ${detail}`;
      node.dataset.state = value;
      node.dataset.mark = statusMark(value);
      node.dataset.tooltip = description;
      node.setAttribute('aria-label', description);
    }
  }

  function renderFooter() {
    const footer = $('#footer');
    footer.replaceChildren();

    const left = el('div', 'footer-left');
    if (state.project.githubUser) {
      const target = state.project.githubUserUrl ? el('a', 'footer-link') : el('span', 'footer-link');
      if (state.project.githubUserUrl) {
        target.href = state.project.githubUserUrl;
        target.target = '_blank'; target.rel = 'noopener noreferrer';
      }
      target.append(icon('github'));
      const text = el('span'); text.textContent = `${t('footer.github')}: ${state.project.githubUser}`; target.append(text);
      left.append(target);
    }

    const center = el('div', 'footer-center');
    const centerParts = [];
    if (state.project.footerComment) centerParts.push(state.project.footerComment);
    if (state.project.version) centerParts.push(state.project.version);
    center.textContent = centerParts.join(' · ');

    const right = el('div', 'footer-right');
    if (state.project.projectUrl) {
      const a = el('a', 'footer-link');
      a.href = state.project.projectUrl; a.target = '_blank'; a.rel = 'noopener noreferrer';
      a.append(icon('link'));
      const text = el('span'); text.textContent = t('footer.project'); a.append(text);
      right.append(a);
    }

    if (left.childNodes.length) footer.append(left); else footer.append(el('div', 'footer-left'));
    if (center.textContent) footer.append(center); else footer.append(el('div', 'footer-center'));
    if (right.childNodes.length) footer.append(right); else footer.append(el('div', 'footer-right'));
  }

  function conditionPaths(condition) { return condition && condition.path ? [condition.path] : []; }
  function matchesCondition(condition) {
    if (!condition) return true;
    return getPath(condition.path) === condition.equals;
  }
  function bindVisibility(node, condition) {
    if (!condition) return;
    Bindings.add(conditionPaths(condition), () => { node.hidden = !matchesCondition(condition); });
  }

  function renderKv(def) {
    const dl = el('dl', 'kv-list');
    for (const item of def.items || []) {
      const row = el('div', 'kv-row');
      const dt = el('dt'); dt.textContent = t(item.labelKey);
      const dd = el('dd');
      row.append(dt, dd); dl.append(row);
      Bindings.add(item.path, () => {
        const value = getPath(item.path);
        const present = value !== undefined && value !== null && value !== '';
        row.hidden = !present;
        if (!present) return;
        dd.textContent = formatValue(value, item.format);
        dd.classList.toggle('empty-value', !present);
      });
    }
    return dl;
  }

  function renderStatus(def) {
    const root = el('div', 'inline-status');
    const label = el('span', 'inline-status-label'); label.textContent = t(def.labelKey);
    const value = el('span', 'status-value');
    const dot = el('span', 'status-dot'); dot.setAttribute('aria-hidden', 'true');
    const text = el('span');
    value.append(dot, text); root.append(label, value);
    Bindings.add(def.path, () => {
      const current = getPath(def.path) || 'unknown';
      value.dataset.state = current;
      text.textContent = stateText(current === 'ok' && def.path.startsWith('wifi') ? 'connected' : current);
    });
    return root;
  }

  function meterPercent(value, policy) {
    const n = Number(value);
    if (!Number.isFinite(n)) return 0;
    return Math.max(0, Math.min(100, ((n - policy.min) / (policy.max - policy.min)) * 100));
  }

  function renderMeter(def) {
    const policy = UI_CONFIG.meters[def.policy];
    const root = el('div', 'meter-block');
    const head = el('div', 'meter-head');
    const label = el('span', 'meter-label'); label.textContent = t(def.labelKey);
    const reading = el('span', 'meter-reading'); head.append(label, reading);
    const track = el('div', 'meter-track');
    track.setAttribute('role', 'progressbar');
    track.style.setProperty('--segments', policy.segments);
    track.style.setProperty('--seg-gap', policy.segments > 15 ? '1px' : policy.segments > 8 ? '2px' : '3px');
    const foot = el('div', 'meter-foot');
    const quality = el('span', 'meter-quality'); foot.append(quality);
    root.append(head, track, foot);

    const paths = [def.path, ...conditionPaths(def.visibleWhen)];
    Bindings.add(paths, () => {
      root.hidden = !matchesCondition(def.visibleWhen);
      const raw = getPath(def.path);
      const numeric = Number(raw);
      const valid = Number.isFinite(numeric);
      const percent = valid ? meterPercent(numeric, policy) : 0;
      const q = valid ? policy.quality(numeric) : 'unknown';
      const shown = formatValue(raw, def.format);
      reading.textContent = shown;
      quality.textContent = t(`quality.${q}`);
      root.dataset.quality = q;
      track.style.setProperty('--meter-value', `${percent.toFixed(1)}%`);
      track.setAttribute('aria-valuemin', String(policy.min));
      track.setAttribute('aria-valuemax', String(policy.max));
      if (valid) track.setAttribute('aria-valuenow', String(numeric)); else track.removeAttribute('aria-valuenow');
      track.setAttribute('aria-valuetext', valid ? `${shown}, ${t(`quality.${q}`)}` : t('quality.unknown'));
    });
    return root;
  }

  function renderSelect(def) {
    const root = el('div', 'form-control');
    const label = el('label'); label.htmlFor = `select-${def.id}`; label.textContent = t(def.labelKey);
    const wrap = el('div', 'select-wrap');
    const select = el('select'); select.id = `select-${def.id}`; select.dataset.preference = def.preference;
    for (const optionDef of def.options || []) {
      const option = el('option'); option.value = optionDef.value; option.textContent = t(optionDef.key); select.append(option);
    }
    wrap.append(select, icon('chevron'));
    root.append(label, wrap);
    if (def.note) {
      const note = el('div', 'form-note');
      root.append(note);
      Bindings.add(`preferences.${def.note}`, () => {
        note.textContent = def.note === 'languageMode' ? t(state.preferences.languageMode === 'manual' ? 'lang.manual' : 'lang.auto') : '';
      });
    }
    Bindings.add(`preferences.${def.preference}`, () => { select.value = state.preferences[def.preference]; });
    return root;
  }

  // Generic switch renderer is intentionally part of the toolkit even though
  // the base project has no binary setting yet.
  function renderSwitch(def) {
    const root = el('div', 'switch-row');
    const label = el('div', 'switch-label');
    const title = el('strong'); title.textContent = t(def.labelKey);
    label.append(title);
    if (def.descriptionKey) { const small = el('small'); small.textContent = t(def.descriptionKey); label.append(small); }
    const control = el('div', 'switch-control');
    const input = el('input'); input.type = 'checkbox'; input.id = `switch-${def.id}`; input.dataset.preference = def.preference;
    const visual = el('label', 'switch-visual'); visual.htmlFor = input.id;
    const stateLabel = el('div', 'switch-state');
    control.append(input, visual, stateLabel); root.append(label, control);
    Bindings.add(`preferences.${def.preference}`, () => {
      input.checked = Boolean(state.preferences[def.preference]);
      stateLabel.textContent = t(input.checked ? 'common.on' : 'common.off');
    });
    return root;
  }

  function renderAction(def) {
    const root = el('div', 'action-row');
    for (const action of def.actions || []) {
      const button = el('button', `button${action.kind === 'danger' ? ' danger' : ''}`);
      button.type = 'button'; button.dataset.action = action.id;
      if (action.confirmKey) button.dataset.confirmKey = action.confirmKey;
      if (action.icon) button.append(icon(action.icon));
      const text = el('span'); text.textContent = t(action.labelKey); button.append(text);
      root.append(button);
    }
    return root;
  }

  function renderUpload(def) {
    const root = el('div', 'upload-control');
    root.dataset.uploadId = def.id;
    root.dataset.endpoint = def.endpoint;
    root.dataset.fieldName = def.fieldName || 'file';
    root.dataset.maxPath = def.maxPath || '';

    const label = el('label', 'upload-label');
    const inputId = `upload-${def.id}`;
    label.htmlFor = inputId;
    label.textContent = t(def.labelKey);

    const input = el('input', 'upload-input');
    input.type = 'file';
    input.id = inputId;
    input.accept = def.accept || '.bin';
    input.dataset.uploadInput = def.id;

    const selected = el('div', 'upload-selected');
    selected.textContent = t('ota.idle');

    const actions = el('div', 'action-row');
    const button = el('button', 'button');
    button.type = 'button';
    button.dataset.uploadButton = def.id;
    button.disabled = true;
    button.append(icon('upload'));
    const buttonText = el('span');
    buttonText.textContent = t(def.buttonKey);
    button.append(buttonText);
    actions.append(button);

    const progressWrap = el('div', 'upload-progress');
    progressWrap.hidden = true;
    const progressTrack = el('div', 'upload-progress-track');
    progressTrack.setAttribute('role', 'progressbar');
    progressTrack.setAttribute('aria-valuemin', '0');
    progressTrack.setAttribute('aria-valuemax', '100');
    progressTrack.setAttribute('aria-valuenow', '0');
    const progressValue = el('div', 'upload-progress-value');
    progressTrack.append(progressValue);
    const progressText = el('div', 'upload-progress-text');
    progressText.textContent = '0 %';
    progressWrap.append(progressTrack, progressText);

    const status = el('div', 'upload-status');
    status.dataset.state = 'idle';
    status.setAttribute('aria-live', 'polite');
    status.textContent = t('ota.idle');

    root.append(label, input, selected, actions, progressWrap, status);
    bindVisibility(root, def.visibleWhen);
    return root;
  }

  function renderHardware(def) {
    const root = el('div', 'hardware-list');
    Bindings.add(def.path, () => {
      root.replaceChildren();
      const modules = getPath(def.path);
      if (!Array.isArray(modules) || !modules.length) return;
      for (const module of modules) {
        const item = el('article', 'hardware-item');
        item.dataset.hardwareId = module.id || '';

        const head = el('div', 'hardware-item-head');
        const titleWrap = el('div', 'hardware-item-title');
        const iconBox = el('span', 'hardware-item-icon'); iconBox.append(icon(module.icon || 'hardware'));
        const title = el('strong'); title.textContent = t(module.nameKey || `status.${module.id}`);
        titleWrap.append(iconBox, title);

        const health = el('span', 'status-value');
        const dot = el('span', 'status-dot'); dot.setAttribute('aria-hidden', 'true');
        const healthText = el('span'); healthText.textContent = stateText(module.health);
        health.dataset.state = module.health || 'unknown';
        health.append(dot, healthText);
        head.append(titleWrap, health);
        item.append(head);

        const info = el('dl', 'hardware-info');
        const infoItems = Array.isArray(module.info) ? module.info : [];
        for (const infoItem of infoItems) {
          const row = el('div', 'kv-row');
          const dt = el('dt'); dt.textContent = t(infoItem.labelKey);
          const dd = el('dd'); dd.textContent = formatValue(infoItem.value, infoItem.format);
          row.append(dt, dd); info.append(row);
        }
        const feedbackRow = el('div', 'kv-row');
        const feedbackLabel = el('dt'); feedbackLabel.textContent = t('hardware.info.feedback');
        const feedbackValue = el('dd'); feedbackValue.textContent = t(`hardware.feedback.${module.feedback || 'none'}`);
        feedbackRow.append(feedbackLabel, feedbackValue); info.append(feedbackRow);
        if (Number(module.lastCheckMs) > 0) {
          const checkedRow = el('div', 'kv-row');
          const checkedLabel = el('dt'); checkedLabel.textContent = t('hardware.info.lastCheck');
          const checkedValue = el('dd'); checkedValue.textContent = Formats.checkTime(module.lastCheckMs);
          checkedRow.append(checkedLabel, checkedValue); info.append(checkedRow);
        }
        item.append(info);

        if (module.error) {
          const error = el('div', 'hardware-error'); error.textContent = module.error; item.append(error);
        }

        const actions = el('div', 'hardware-item-actions');
        const button = el('button', 'button');
        button.type = 'button';
        button.dataset.hardwareCheck = module.id;
        button.disabled = module.health === 'checking';
        button.append(icon('refresh'));
        const buttonText = el('span'); buttonText.textContent = t('action.hardwareCheck'); button.append(buttonText);
        actions.append(button);
        for (const actionDef of module.actions || []) {
          const actionButton = el('button', 'button');
          actionButton.type = 'button';
          actionButton.dataset.hardwareAction = actionDef.id || '';
          actionButton.dataset.hardwareModule = module.id || '';
          actionButton.disabled = module.health === 'checking';
          actionButton.append(icon(actionDef.icon || module.icon || 'hardware'));
          const actionText = el('span'); actionText.textContent = t(actionDef.labelKey || actionDef.id); actionButton.append(actionText);
          actions.append(actionButton);
        }
        item.append(actions);
        root.append(item);
      }
    });
    return root;
  }

  function timeErrorText(code) {
    return t(`time.error.${code || 'unknown'}`);
  }

  function setTimeUiMessage(root, kind, text) {
    if (!root) return;
    const node = $('[data-time-message]', root);
    if (!node) return;
    node.dataset.state = kind || 'idle';
    node.textContent = text || '';
  }

  function renderTimeManagement() {
    const root = el('div', 'time-management');

    const priority = el('div', 'time-priority');
    for (const source of ['ntp', 'rtc', 'browser', 'relative']) {
      const item = el('div', 'time-priority-item');
      const title = el('strong'); title.textContent = t(`time.priority.${source}.title`);
      const desc = el('span'); desc.textContent = t(`time.priority.${source}.desc`);
      item.append(title, desc); priority.append(item);
    }

    const summary = el('dl', 'kv-list time-summary');
    const summaryRows = {};
    const addSummaryRow = (id, labelKey) => {
      const row = el('div', 'kv-row');
      const dt = el('dt'); dt.textContent = t(labelKey);
      const dd = el('dd'); row.append(dt, dd); summary.append(row); summaryRows[id] = dd;
    };
    addSummaryRow('source', 'time.activeSource');
    addSummaryRow('system', 'time.systemTime');
    addSummaryRow('quality', 'time.timeStatus');
    addSummaryRow('lastCheck', 'time.lastCheck');

    const ntpSection = el('section', 'time-section');
    const ntpTitle = el('h3'); ntpTitle.textContent = t('time.ntp.title');
    const ntpRow = el('div', 'time-ntp-row');
    const inputWrap = el('div', 'form-control');
    const inputLabel = el('label'); inputLabel.htmlFor = 'time-ntp-server'; inputLabel.textContent = t('time.ntp.server');
    const input = el('input', 'text-input'); input.type = 'text'; input.id = 'time-ntp-server'; input.maxLength = 96; input.autocomplete = 'off'; input.spellcheck = false; input.dataset.timeNtpInput = '1';
    input.addEventListener('input', () => { input.dataset.dirty = '1'; });
    inputWrap.append(inputLabel, input);
    const ntpButton = el('button', 'button primary-button'); ntpButton.type = 'button'; ntpButton.dataset.timeAction = 'ntp-save';
    ntpButton.append(icon('refresh')); const ntpButtonText = el('span'); ntpButtonText.textContent = t('time.ntp.checkSave'); ntpButton.append(ntpButtonText);
    ntpRow.append(inputWrap, ntpButton);
    const ntpHint = el('div', 'form-note'); ntpHint.textContent = t('time.ntp.hint');
    ntpSection.append(ntpTitle, ntpRow, ntpHint);

    const browserSection = el('section', 'time-section');
    const browserTitle = el('h3'); browserTitle.textContent = t('time.browser.title');
    const browserNotice = el('div', 'notice'); browserNotice.dataset.kind = 'info'; browserNotice.append(icon('info'));
    const browserText = el('p'); browserText.textContent = t('time.browser.hint'); browserNotice.append(browserText);
    browserSection.append(browserTitle, browserNotice);

    const differences = el('section', 'time-section');
    const diffTitle = el('h3'); diffTitle.textContent = t('time.differences.title');
    const sourceGrid = el('div', 'time-source-grid');
    const rtcSync = el('div', 'time-rtc-sync');
    differences.append(diffTitle, sourceGrid, rtcSync);

    const actions = el('div', 'action-row time-actions');
    const checkButton = el('button', 'button'); checkButton.type = 'button'; checkButton.dataset.timeAction = 'check';
    checkButton.append(icon('refresh')); const checkText = el('span'); checkText.textContent = t('time.check'); checkButton.append(checkText);
    actions.append(checkButton);
    const message = el('div', 'time-operation-status'); message.dataset.timeMessage = '1'; message.setAttribute('aria-live', 'polite');

    root.append(priority, summary, ntpSection, browserSection, differences, actions, message);

    const renderSample = (labelKey, sample, delta, isReference, note = '') => {
      const item = el('article', 'time-source-item');
      const head = el('div', 'time-source-head');
      const name = el('strong'); name.textContent = t(labelKey); head.append(name);
      if (isReference) { const badge = el('span', 'time-reference-badge'); badge.textContent = t('time.reference'); head.append(badge); }
      const value = el('div', 'time-source-value');
      if (sample?.available && Number(sample.epochMs) > 0) value.textContent = Formats.dateTime(sample.epochMs);
      else if (sample?.available) value.textContent = t('time.invalid');
      else value.textContent = t('time.unavailable');
      item.append(head, value);
      const meta = el('div', 'time-source-meta');
      if (sample?.available && !sample?.valid) {
        const invalidNode = el('span'); invalidNode.textContent = t('time.invalid'); meta.append(invalidNode);
      }
      if (Number.isFinite(Number(delta))) {
        const deltaNode = el('span'); deltaNode.textContent = `${t('time.delta')}: ${Formats.deltaMs(delta)}`; meta.append(deltaNode);
      }
      if (note) { const noteNode = el('span'); noteNode.textContent = note; meta.append(noteNode); }
      if (meta.childNodes.length) item.append(meta);
      return item;
    };

    const updateManagedClock = () => {
      const tm = state.timeManagement || {};
      summaryRows.system.textContent = tm.valid ? Formats.dateTime(currentDate().getTime()) : t('time.unavailable');
      if (Number(tm.lastCheckMonotonicMs) > 0 && tm.valid && Number(tm.epochMs) > 0 && Number(tm.monotonicMs) >= Number(tm.lastCheckMonotonicMs)) {
        const checkEpoch = Number(tm.epochMs) - (Number(tm.monotonicMs) - Number(tm.lastCheckMonotonicMs));
        summaryRows.lastCheck.textContent = Formats.dateTime(checkEpoch);
      } else if (Number(tm.lastCheckMonotonicMs) > 0) {
        summaryRows.lastCheck.textContent = Formats.checkTime(tm.lastCheckMonotonicMs);
      } else {
        summaryRows.lastCheck.textContent = t('common.none');
      }
    };

    const update = () => {
      const tm = state.timeManagement || {};
      const active = tm.activeSource || 'relative';
      summaryRows.source.textContent = t(`time.source.${active}`);
      summaryRows.quality.textContent = t(`time.quality.${tm.quality || 'none'}`);
      updateManagedClock();
      const checking = !!tm.checking;
      ntpButton.disabled = checking;
      checkButton.disabled = checking;
      if (checking && !message.textContent) setTimeUiMessage(root, 'busy', t('time.checking'));
      if (input.dataset.dirty !== '1') input.value = tm.ntpServer || tm.ntp?.server || '';

      sourceGrid.replaceChildren();
      const ntpNote = tm.ntp?.error && tm.ntp.error !== 'none' ? timeErrorText(tm.ntp.error) : (tm.ntp?.rttMs ? `RTT ${tm.ntp.rttMs} ms` : '');
      sourceGrid.append(renderSample('time.sample.ntp', tm.ntp?.sample, tm.ntp?.deltaMs, active === 'ntp', ntpNote));
      if (tm.rtc?.beforeSync?.available) {
        sourceGrid.append(renderSample('time.sample.rtcBefore', tm.rtc.beforeSync, tm.rtc?.beforeSyncDeltaMs, false));
      }
      const rtcNote = tm.rtc?.osf ? 'OSF' : '';
      sourceGrid.append(renderSample('time.sample.rtc', tm.rtc?.sample, tm.rtc?.deltaMs, active === 'rtc', rtcNote));
      sourceGrid.append(renderSample('time.sample.system', { available: tm.valid, valid: tm.valid, epochMs: tm.epochMs }, tm.system?.deltaMs, false));
      sourceGrid.append(renderSample('time.sample.browser', tm.browser?.sample, tm.browser?.deltaMs, active === 'browser'));

      rtcSync.replaceChildren();
      const syncLabel = el('strong'); syncLabel.textContent = `${t('time.rtcSync')}: `;
      const syncValue = el('span');
      if (tm.rtc?.syncAttempted) syncValue.textContent = t(tm.rtc?.syncOk ? 'time.rtcSync.ok' : 'time.rtcSync.failed');
      else syncValue.textContent = t('time.rtcSync.none');
      rtcSync.append(syncLabel, syncValue);
    };

    Bindings.add('timeManagement', update);
    Bindings.add('clock.tick', updateManagedClock);
    return root;
  }

  function renderNotice(def) {
    const root = el('div', 'notice'); root.dataset.kind = def.kind || 'info';
    root.append(icon('info'));
    const p = el('p'); p.textContent = t(def.textKey); root.append(p);
    bindVisibility(root, def.visibleWhen);
    return root;
  }

  function renderList(def) {
    const list = el('ul', 'simple-list');
    for (const item of def.items || []) { const li = el('li'); li.textContent = item.textKey ? t(item.textKey) : Formats.text(item.text); list.append(li); }
    return list;
  }

  function applyInterruptionSummary(summary) {
    if (!summary || typeof summary !== 'object') return;
    const previousSequence = Number(state.interruptions.sequence || 0);
    const nextSequence = Number(summary.sequence || 0);
    const receivedPerf = performance.now();
    let relativeBaseAge = 0;
    let relativeAgeKnown = false;
    if (summary.last?.available && !summary.last.absoluteValid && Number(summary.last.monotonicMs) > 0) {
      const deviceMono = Number(summary.monotonicMs || 0);
      if (deviceMono >= Number(summary.last.monotonicMs)) {
        relativeBaseAge = Math.floor((deviceMono - Number(summary.last.monotonicMs)) / 1000);
        relativeAgeKnown = true;
      }
    }
    patchState({ interruptions: summary, status: { data: summary.storageState === 'ready' ? 'ok' : summary.storageState === 'warning' ? 'warning' : summary.storageState === 'error' ? 'error' : 'unknown' } });
    if (state.interruptions.last?.available) {
      state.interruptions.last._receivedPerf = receivedPerf;
      state.interruptions.last._relativeBaseAge = relativeBaseAge;
      state.interruptions.last._relativeAgeKnown = relativeAgeKnown;
    }
    if (typeof summary.soundEnabled === 'boolean' && state.projectSettings.soundEnabled !== summary.soundEnabled) {
      state.projectSettings.soundEnabled = summary.soundEnabled;
      Bindings.notify('projectSettings');
    }
    if (nextSequence !== previousSequence) state.analytics.dirty = true;
  }

  function translatedCount(key, count) {
    return t(key).replace('{n}', String(count));
  }

  function interruptionAgeSeconds() {
    const last = state.interruptions.last || {};
    if (!last.available) return null;
    if (last.absoluteValid && state.time.valid && Number(last.timeValueSeconds) > 0) {
      return Math.max(0, Math.floor((currentDate().getTime() / 1000) - Number(last.timeValueSeconds)));
    }
    if (last._relativeAgeKnown && Number(last._receivedPerf) > 0) {
      return Math.max(0, Number(last._relativeBaseAge || 0) + Math.floor((performance.now() - Number(last._receivedPerf)) / 1000));
    }
    return null;
  }

  function interruptionAgeText() {
    const last = state.interruptions.last || {};
    if (!last.available) return t('interruptions.never');
    const seconds = interruptionAgeSeconds();
    if (seconds == null) return t('interruptions.ageUnknown');
    if (seconds < 5) return t('interruptions.justNow');
    if (seconds < 60) return translatedCount('interruptions.agoSeconds', seconds);
    const minutes = Math.floor(seconds / 60);
    if (minutes < 60) return translatedCount('interruptions.agoMinutes', minutes);
    const hours = Math.floor(minutes / 60);
    if (hours < 48) return translatedCount('interruptions.agoHours', hours);
    return translatedCount('interruptions.agoDays', Math.floor(hours / 24));
  }

  function renderInterruptionHome() {
    const root = el('div', 'interruption-home');
    const summary = el('div', 'interruption-summary');
    const countWrap = el('div', 'interruption-count-wrap');
    const count = el('div', 'interruption-count');
    count.setAttribute('aria-live', 'polite');
    const countLabel = el('div', 'interruption-count-label'); countLabel.textContent = t('interruptions.today');
    countWrap.append(count, countLabel);
    const lastWrap = el('div', 'interruption-last');
    const lastLabel = el('span', 'interruption-last-label'); lastLabel.textContent = t('interruptions.last');
    const lastValue = el('strong', 'interruption-last-value');
    const lastMeta = el('small', 'interruption-last-meta');
    lastWrap.append(lastLabel, lastValue, lastMeta);
    summary.append(countWrap, lastWrap);

    const button = el('button', 'button interruption-button primary-button');
    button.type = 'button'; button.dataset.interruptionTrigger = 'web';
    button.append(icon('interrupt'));
    const buttonText = el('span'); buttonText.textContent = t('interruptions.button'); button.append(buttonText);

    const status = el('div', 'interruption-persist-status'); status.setAttribute('aria-live', 'polite');
    root.append(summary, button, status);

    const updateSummary = () => {
      count.textContent = String(state.interruptions.todayCount || 0);
      lastValue.textContent = interruptionAgeText();
      const last = state.interruptions.last || {};
      const eventSourceKey = `event.source.${last.eventSource || 'unknown'}`;
      lastMeta.textContent = last.available ? `${t(`time.source.${last.timeSource || 'relative'}`)} · ${t(eventSourceKey)}` : '';
      const pending = Number(state.interruptions.pendingCount || 0);
      const dropped = Number(state.interruptions.droppedCount || 0);
      status.textContent = dropped > 0 ? translatedCount('interruptions.dropped', dropped) : pending > 0 ? `${t('interruptions.pending')} (${pending})` : '';
      status.dataset.state = state.interruptions.storageState || 'unavailable';
    };
    Bindings.add(['interruptions.todayCount','interruptions.pendingCount','interruptions.droppedCount','interruptions.storageState','interruptions.last','clock.tick'], updateSummary);
    return root;
  }

  function renderProjectSettings() {
    const root = el('div', 'project-settings');
    const controls = {};

    const addSection = (titleKey) => {
      const section = el('section', 'project-settings-section');
      const head = el('div', 'project-settings-section-head');
      head.textContent = t(titleKey);
      const grid = el('div', 'project-settings-grid');
      section.append(head, grid); root.append(section);
      return grid;
    };

    const addSwitch = (grid, field, labelKey) => {
      const row = el('label', 'project-setting-row project-setting-switch');
      const label = el('span', 'project-setting-label'); label.textContent = t(labelKey);
      const control = el('span', 'project-sound-control');
      const input = el('input'); input.type = 'checkbox'; input.dataset.projectSetting = field;
      const visual = el('span', 'project-sound-visual');
      const stateText = el('strong', 'project-sound-state');
      control.append(input, visual, stateText); row.append(label, control); grid.append(row);
      controls[field] = { input, stateText, row };
    };

    const addSelect = (grid, field, labelKey, options) => {
      const row = el('label', 'project-setting-row');
      const label = el('span', 'project-setting-label'); label.textContent = t(labelKey);
      const select = el('select', 'project-setting-input'); select.dataset.projectSetting = field;
      for (const [value, key] of options) { const option = el('option'); option.value = value; option.textContent = t(key); select.append(option); }
      row.append(label, select); grid.append(row); controls[field] = { input: select, row };
    };

    const addNumber = (grid, field, labelKey, min, max, suffixKey = '') => {
      const row = el('label', 'project-setting-row');
      const label = el('span', 'project-setting-label'); label.textContent = t(labelKey);
      const wrap = el('span', 'project-number-control');
      const input = el('input', 'project-setting-input'); input.type = 'number'; input.min = String(min); input.max = String(max); input.step = '1'; input.dataset.projectSetting = field;
      wrap.append(input);
      if (suffixKey) { const suffix = el('span', 'project-setting-suffix'); suffix.textContent = t(suffixKey); wrap.append(suffix); }
      row.append(label, wrap); grid.append(row); controls[field] = { input, row };
    };

    const addRange = (grid, field, labelKey, min, max) => {
      const row = el('label', 'project-setting-row');
      const label = el('span', 'project-setting-label'); label.textContent = t(labelKey);
      const wrap = el('span', 'project-range-control');
      const input = el('input', 'project-setting-range'); input.type = 'range'; input.min = String(min); input.max = String(max); input.step = '1'; input.dataset.projectSetting = field;
      const output = el('output', 'project-range-value'); output.textContent = '0 %';
      input.addEventListener('input', () => { output.textContent = `${input.value} %`; });
      wrap.append(input, output); row.append(label, wrap); grid.append(row); controls[field] = { input, output, row };
    };

    const addHardwareAction = (grid, moduleId, actionId, labelKey, iconName) => {
      const row = el('div', 'project-setting-row project-setting-action');
      const label = el('span', 'project-setting-label'); label.textContent = t(labelKey);
      const button = el('button', 'button'); button.type = 'button'; button.dataset.hardwareAction = actionId; button.dataset.hardwareModule = moduleId;
      button.append(icon(iconName)); const text = el('span'); text.textContent = t(labelKey); button.append(text);
      row.append(label, button); grid.append(row);
    };

    const displayGrid = addSection('project.section.display');
    addSwitch(displayGrid, 'displayEnabled', 'project.displayEnabled');
    addSelect(displayGrid, 'displayMode', 'project.displayMode', [
      ['standard','project.displayMode.standard'], ['count','project.displayMode.count'], ['last','project.displayMode.last'],
      ['day-progress','project.displayMode.dayProgress'], ['focus','project.displayMode.focus']
    ]);
    addSwitch(displayGrid, 'displayRotation180', 'project.displayRotation180');
    addRange(displayGrid, 'displayBrightness', 'project.displayBrightness', 1, 100);
    addNumber(displayGrid, 'displayDimAfterMinutes', 'project.displayDimAfter', 0, 1440, 'project.minutes');
    const dimHint = el('div', 'form-note project-setting-note'); dimHint.textContent = t('project.dimDisabled'); displayGrid.append(dimHint);
    addRange(displayGrid, 'displayDimBrightness', 'project.displayDimBrightness', 0, 100);
    addHardwareAction(displayGrid, 'display', 'test', 'action.displayTest', 'display');

    const feedbackGrid = addSection('project.section.displayFeedback');
    addSwitch(feedbackGrid, 'displayFlashEnabled', 'project.displayFlash');

    const soundGrid = addSection('project.section.sound');
    addSwitch(soundGrid, 'soundEnabled', 'interruptions.sound');
    addRange(soundGrid, 'soundVolume', 'project.soundVolume', 0, 100);
    addSelect(soundGrid, 'soundMode', 'project.soundMode', [['fixed','project.soundMode.fixed'],['rotate','project.soundMode.rotate']]);
    addNumber(soundGrid, 'soundTrack', 'project.soundTrack', 2, 65535);
    const soundHint = el('div', 'form-note project-setting-note'); soundGrid.append(soundHint);
    addHardwareAction(soundGrid, 'audio', 'test', 'action.audioTest', 'audio');

    const message = el('div', 'project-setting-message'); message.setAttribute('aria-live', 'polite'); message.dataset.projectSettingMessage = '1';
    root.append(message);

    const update = () => {
      const ps = state.projectSettings || {};
      for (const [field, entry] of Object.entries(controls)) {
        const input = entry.input;
        if (document.activeElement !== input) {
          if (input.type === 'checkbox') input.checked = !!ps[field];
          else if (ps[field] != null) input.value = String(ps[field]);
        }
        if (entry.stateText) entry.stateText.textContent = t(input.checked ? 'common.on' : 'common.off');
        if (entry.output) entry.output.textContent = `${input.value} %`;
      }
      const count = Number(ps.soundTrackCount || 0);
      controls.soundTrack.input.max = count >= 2 ? String(count) : '65535';
      const fixedMode = (ps.soundMode || 'rotate') === 'fixed';
      controls.soundTrack.row.hidden = !fixedMode;
      soundHint.textContent = count >= 2
        ? t('project.soundTracksAvailable').replaceAll('{n}', String(count))
        : `${t('project.soundTrackHint')} ${t('project.soundTracksUnknown')}`;
    };
    Bindings.add('projectSettings', update);
    return root;
  }

  function localeForLabels() { const lang = state.preferences.language; return lang === 'en' ? 'en-GB' : lang === 'fr' ? 'fr-FR' : lang === 'it' ? 'it-IT' : 'de-DE'; }
  function weekdayLabels() {
    const base = Date.UTC(2024, 0, 1); // Monday
    return Array.from({ length: 7 }, (_, i) => new Intl.DateTimeFormat(localeForLabels(), { weekday: 'short', timeZone: 'UTC' }).format(new Date(base + i * 86400000)));
  }
  function monthLabels() {
    return Array.from({ length: 12 }, (_, i) => new Intl.DateTimeFormat(localeForLabels(), { month: 'short', timeZone: 'UTC' }).format(new Date(Date.UTC(2024, i, 1))));
  }

  function heatmapLabels(kind, data, transpose) {
    if (kind === 'hourly') {
      const days = weekdayLabels(); const hours = Array.from({ length: 24 }, (_, i) => String(i).padStart(2, '0'));
      return transpose ? { rows: hours.map(h => `${h}:00`), cols: days } : { rows: days, cols: hours };
    }
    if (kind === 'monthWeek') {
      const months = monthLabels(); const weeks = Array.from({ length: 53 }, (_, i) => String(i + 1));
      return transpose ? { rows: weeks, cols: months } : { rows: months, cols: weeks };
    }
    const months = monthLabels();
    const startYear = Number(data?.startYear || currentDate().getFullYear() - 4);
    const years = Array.from({ length: 5 }, (_, i) => String(startYear + i));
    return transpose ? { rows: months, cols: years } : { rows: years, cols: months };
  }

  function formatIntervalSeconds(value, compact = false) {
    const total = Math.max(0, Math.round(Number(value) || 0));
    if (total < 60) return `${total}s`;
    const hours = Math.floor(total / 3600);
    const minutes = Math.floor((total % 3600) / 60);
    const seconds = total % 60;
    if (hours > 0) return `${hours}h${minutes ? ` ${minutes}m` : ''}${!compact && seconds ? ` ${seconds}s` : ''}`;
    if (compact && total >= 600) return `${Math.floor(total / 60)}m`;
    return `${Math.floor(total / 60)}m${seconds ? ` ${seconds}s` : ''}`;
  }

  function renderHeatmapGrid(root, kind, data) {
    const holder = $('.heatmap-holder', root);
    if (!holder) return;
    holder.replaceChildren();
    if (state.analytics.error) {
      const empty = el('div', 'heatmap-empty'); empty.textContent = t('analytics.loadError'); holder.append(empty); return;
    }
    if (!data || !Array.isArray(data.values)) {
      const empty = el('div', 'heatmap-empty'); empty.textContent = t('analytics.noData'); holder.append(empty); return;
    }
    const isAverage = data.metric === 'averageInterval';
    const samples = Array.isArray(data.samples) ? data.samples : [];
    const narrow = window.matchMedia('(max-width: 700px)').matches;
    const transpose = kind === 'monthWeek' ? window.matchMedia('(max-width: 1300px)').matches : narrow;
    const originalRows = Number(data.rows || 0), originalCols = Number(data.cols || 0);
    const rows = transpose ? originalCols : originalRows;
    const cols = transpose ? originalRows : originalCols;
    const labels = heatmapLabels(kind, data, transpose);
    const validAverages = isAverage
      ? data.values.map((v, i) => Number(samples[i] || 0) > 0 ? Number(v) : null).filter(v => Number.isFinite(v))
      : [];
    const minAverage = validAverages.length ? Math.min(...validAverages) : 0;
    const maxAverage = validAverages.length ? Math.max(...validAverages) : 0;
    const maxCount = isAverage ? 0 : Math.max(0, ...data.values.map(v => Number(v) || 0));
    const grid = el('div', 'heatmap-grid');
    grid.setAttribute('role', 'grid');
    grid.style.setProperty('--heat-cols', String(cols));

    const corner = el('div', 'heatmap-corner'); grid.append(corner);
    for (let col = 0; col < cols; col++) {
      const head = el('div', 'heatmap-col-head'); head.textContent = labels.cols[col] || String(col + 1);
      const originalCol = transpose ? -1 : col;
      const originalRow = transpose ? col : -1;
      if ((originalCol >= 0 && originalCol === Number(data.currentCol)) || (originalRow >= 0 && originalRow === Number(data.currentRow))) head.classList.add('is-current');
      grid.append(head);
    }
    for (let row = 0; row < rows; row++) {
      const rowHead = el('div', 'heatmap-row-head'); rowHead.textContent = labels.rows[row] || String(row + 1);
      const originalRowForHead = transpose ? -1 : row;
      const originalColForHead = transpose ? row : -1;
      if ((originalRowForHead >= 0 && originalRowForHead === Number(data.currentRow)) || (originalColForHead >= 0 && originalColForHead === Number(data.currentCol))) rowHead.classList.add('is-current');
      grid.append(rowHead);
      for (let col = 0; col < cols; col++) {
        const originalRow = transpose ? col : row;
        const originalCol = transpose ? row : col;
        const index = originalRow * originalCols + originalCol;
        const value = Number(data.values[index] || 0);
        const sampleCount = Number(samples[index] || 0);
        const hasValue = isAverage ? sampleCount > 0 : value > 0;
        const cell = el('div', 'heatmap-cell');
        const shown = isAverage ? (hasValue ? formatIntervalSeconds(value, true) : '—') : String(value);
        cell.textContent = shown;
        let heatPercent = 0;
        if (isAverage && hasValue) {
          heatPercent = maxAverage > minAverage
            ? 8 + ((maxAverage - value) / (maxAverage - minAverage)) * 88
            : 70;
        } else if (!isAverage && value > 0 && maxCount > 0) {
          heatPercent = Math.max(8, (value / maxCount) * 88);
        }
        cell.style.setProperty('--heat-pct', `${Math.round(heatPercent)}%`);
        if (!hasValue) cell.classList.add('is-zero');
        if (originalRow === Number(data.currentRow)) cell.classList.add('current-row');
        if (originalCol === Number(data.currentCol)) cell.classList.add('current-col');
        if (originalRow === Number(data.currentRow) && originalCol === Number(data.currentCol)) cell.classList.add('current-intersection');
        cell.setAttribute('role', 'gridcell');
        const rowLabel = transpose ? labels.cols[col] : labels.rows[row];
        const colLabel = transpose ? labels.rows[row] : labels.cols[col];
        const detail = isAverage
          ? (hasValue ? `${formatIntervalSeconds(value, false)} · ${t('analytics.intervalSamples').replace('{n}', String(sampleCount))}` : '—')
          : String(value);
        const accessible = `${rowLabel}, ${colLabel}: ${detail}`;
        cell.setAttribute('aria-label', accessible);
        cell.title = accessible;
        grid.append(cell);
      }
    }
    holder.append(grid);
    if (data.coverage?.complete === false) {
      const coverage = el('div', 'form-note heatmap-coverage');
      coverage.textContent = data.source && data.source !== 'all' ? t('analytics.sourceCoveragePartial') : t('analytics.coveragePartial');
      holder.append(coverage);
    }
  }

  function createFilterField(labelKey, type, value, datasetName) {
    const wrap = el('label', 'analytics-filter-field');
    const label = el('span'); label.textContent = t(labelKey);
    const input = el('input', 'text-input'); input.type = type; input.value = value; input.dataset[datasetName] = '1';
    wrap.append(label, input); return { wrap, input };
  }

  function createAnalyticsMetricField() {
    const wrap = el('label', 'analytics-filter-field');
    const label = el('span'); label.textContent = t('analytics.metric');
    const select = el('select'); select.dataset.analyticsMetric = '1';
    for (const [value, key] of [['count','analytics.metric.count'],['averageInterval','analytics.metric.averageInterval']]) {
      const option = el('option'); option.value = value; option.textContent = t(key); select.append(option);
    }
    select.value = state.analytics.metric || 'count';
    wrap.append(label, select);
    return { wrap, select };
  }

  function createAnalyticsSourceField() {
    const wrap = el('label', 'analytics-filter-field');
    const label = el('span'); label.textContent = t('analytics.source');
    const select = el('select'); select.dataset.analyticsSource = '1';
    for (const [value, key] of [['all','analytics.source.all'],['physical_button','analytics.source.physical_button'],['web_button','analytics.source.web_button']]) {
      const option = el('option'); option.value = value; option.textContent = t(key); select.append(option);
    }
    select.value = state.analytics.source || 'all';
    wrap.append(label, select);
    return { wrap, select };
  }

  function renderHeatmapHourly() {
    const root = el('div', 'analytics-block'); root.dataset.heatmapKind = 'hourly';
    const controls = el('div', 'analytics-filters');
    const metricField = createAnalyticsMetricField();
    const sourceField = createAnalyticsSourceField();
    const modeWrap = el('label', 'analytics-filter-field'); const modeLabel = el('span'); modeLabel.textContent = t('analytics.mode');
    const mode = el('select'); mode.dataset.analyticsHourlyMode = '1';
    for (const [value, key] of [['week','analytics.mode.week'],['range','analytics.mode.range']]) { const option=el('option'); option.value=value; option.textContent=t(key); mode.append(option); }
    mode.value = state.analytics.hourlyMode || 'week'; modeWrap.append(modeLabel, mode);
    const current = projectCurrentCalendar();
    const yearField = createFilterField('analytics.year','number',String(current.year),'analyticsHourlyYear'); yearField.input.min='2020'; yearField.input.max='2199';
    const weekField = createFilterField('analytics.week','number',String(current.week),'analyticsHourlyWeek'); weekField.input.min='1'; weekField.input.max='53';
    const dateText = `${current.year}-${String(current.month).padStart(2,'0')}-${String(current.day).padStart(2,'0')}`;
    const fromField = createFilterField('analytics.from','date',dateText,'analyticsHourlyFrom');
    const toField = createFilterField('analytics.to','date',dateText,'analyticsHourlyTo');
    const button = el('button','button'); button.type='button'; button.dataset.analyticsAction='hourly'; button.append(icon('refresh')); const bt=el('span'); bt.textContent=t('analytics.load'); button.append(bt);
    controls.append(metricField.wrap,sourceField.wrap,modeWrap,yearField.wrap,weekField.wrap,fromField.wrap,toField.wrap,button);
    const holder = el('div','heatmap-holder'); root.append(controls,holder);
    const updateMode = () => { const isWeek=mode.value==='week'; yearField.wrap.hidden=!isWeek; weekField.wrap.hidden=!isWeek; fromField.wrap.hidden=isWeek; toField.wrap.hidden=isWeek; state.analytics.hourlyMode=mode.value; };
    mode.addEventListener('change', updateMode); updateMode();
    Bindings.add(['analytics.hourly','analytics.error'],()=>renderHeatmapGrid(root,'hourly',state.analytics.hourly));
    return root;
  }

  function renderHeatmapMonthWeek() {
    const root=el('div','analytics-block'); root.dataset.heatmapKind='monthWeek';
    const controls=el('div','analytics-filters'); const metricField=createAnalyticsMetricField(); const sourceField=createAnalyticsSourceField(); const year=createFilterField('analytics.year','number',String(projectCurrentCalendar().year),'analyticsMonthWeekYear'); year.input.min='2020'; year.input.max='2199';
    const button=el('button','button'); button.type='button'; button.dataset.analyticsAction='month-week'; button.append(icon('refresh')); const bt=el('span');bt.textContent=t('analytics.load');button.append(bt); controls.append(metricField.wrap,sourceField.wrap,year.wrap,button);
    const holder=el('div','heatmap-holder'); root.append(controls,holder);
    Bindings.add(['analytics.monthWeek','analytics.error'],()=>renderHeatmapGrid(root,'monthWeek',state.analytics.monthWeek)); return root;
  }

  function renderHeatmapYearMonth() {
    const root=el('div','analytics-block'); root.dataset.heatmapKind='yearMonth';
    const controls=el('div','analytics-filters'); const metricField=createAnalyticsMetricField(); const sourceField=createAnalyticsSourceField(); controls.append(metricField.wrap,sourceField.wrap);
    const holder=el('div','heatmap-holder'); root.append(controls,holder);
    Bindings.add(['analytics.yearMonth','analytics.error'],()=>renderHeatmapGrid(root,'yearMonth',state.analytics.yearMonth)); return root;
  }

  function renderAnalyticsStorage() {
    const root=el('div','analytics-storage');
    const dl=el('dl','kv-list');
    const rows={};
    for(const [key,labelKey] of [['raw','analytics.rawEvents'],['daily','analytics.dailyRecords'],['used','analytics.storageUsed'],['unassigned','analytics.unassigned'],['dropped','analytics.dropped']]){
      const row=el('div','kv-row'); const dt=el('dt');dt.textContent=t(labelKey); const dd=el('dd'); row.append(dt,dd); dl.append(row); rows[key]=dd;
    }
    const problem=el('div','notice'); problem.dataset.kind='warning'; problem.hidden=true; problem.append(icon('warning')); const problemText=el('p'); problem.append(problemText);
    const hint=el('div','form-note');hint.textContent=t('analytics.ringHint');
    const actions=el('div','action-row'); const button=el('button','button primary-button');button.type='button';button.dataset.analyticsDownload='1';button.append(icon('download'));const bt=el('span');bt.textContent=t('analytics.download');button.append(bt);actions.append(button);

    const danger=el('section','time-section');
    const dangerTitle=el('h3'); dangerTitle.textContent=t('analytics.databaseDeleteTitle');
    const dangerNote=el('div','form-note'); dangerNote.textContent=t('analytics.databaseDeleteHint');
    const field=el('label','form-control'); const fieldLabel=el('span'); fieldLabel.textContent=t('analytics.databaseDeletePassword');
    const password=el('input','text-input'); password.type='password'; password.autocomplete='off'; password.spellcheck=false; password.dataset.databasePassword='1'; field.append(fieldLabel,password);
    const dangerActions=el('div','action-row'); const erase=el('button','button danger'); erase.type='button'; erase.append(icon('trash')); const eraseText=el('span'); eraseText.textContent=t('analytics.databaseDeleteAction'); erase.append(eraseText); dangerActions.append(erase);
    const eraseMessage=el('div','project-setting-message'); eraseMessage.setAttribute('aria-live','polite');
    erase.addEventListener('click', async()=>{
      if(!confirm(t('analytics.databaseDeleteConfirm'))) return;
      erase.disabled=true; password.disabled=true; eraseMessage.textContent='';
      try{
        await Transport.eraseDatabase(password.value);
        eraseMessage.dataset.state='ok'; eraseMessage.textContent=t('analytics.databaseDeleteSuccess');
      }catch(error){
        eraseMessage.dataset.state='error';
        eraseMessage.textContent=error?.code==='invalid_database_password'?t('analytics.databaseDeleteWrong'):t('analytics.databaseDeleteFailed');
        erase.disabled=false; password.disabled=false;
      }
    });
    danger.append(dangerTitle,dangerNote,field,dangerActions,eraseMessage);
    root.append(dl,problem,hint,actions,danger);
    Bindings.add('analytics.storage',()=>{
      const s=state.analytics.storage||{};
      rows.raw.textContent=`${Number(s.rawCount||0).toLocaleString()} / ${Number(s.rawCapacity||0).toLocaleString()}`;
      rows.daily.textContent=`${Number(s.dailyCount||0).toLocaleString()} / ${Number(s.dailyCapacity||0).toLocaleString()}`;
      rows.used.textContent=s.fsTotalBytes?`${Formats.bytes(s.fsUsedBytes)} / ${Formats.bytes(s.fsTotalBytes)}`:t('common.none');
      rows.unassigned.textContent=String(s.unassignedCount||0); rows.dropped.textContent=String(s.droppedCount||0);
      const detail=s.problem||s.rawError||s.aggregateError||'';
      problem.hidden=!detail; problemText.textContent=detail?`${t('analytics.storageState')}: ${detail}`:'';
    });
    return root;
  }

  function renderComponent(def) {
    const renderers = { kv: renderKv, status: renderStatus, meter: renderMeter, select: renderSelect, switch: renderSwitch, action: renderAction, upload: renderUpload, hardware: renderHardware, timeManagement: renderTimeManagement, notice: renderNotice, list: renderList, interruptionHome: renderInterruptionHome, projectSettings: renderProjectSettings, heatmapHourly: renderHeatmapHourly, heatmapMonthWeek: renderHeatmapMonthWeek, heatmapYearMonth: renderHeatmapYearMonth, analyticsStorage: renderAnalyticsStorage };
    const renderer = renderers[def.type];
    return renderer ? renderer(def) : null;
  }

  function renderCard(def) {
    const card = el('section', 'card');
    card.dataset.cardId = def.id;
    card.dataset.width = def.width || 'normal';
    const header = el('div', 'card-header');
    const iconBox = el('div', 'card-icon'); iconBox.append(icon(def.icon || 'info'));
    const titleWrap = el('div', 'card-title-wrap');
    const title = el('h2', 'card-title'); title.textContent = t(def.titleKey); titleWrap.append(title);
    if (def.descriptionKey) { const desc = el('p', 'card-description'); desc.textContent = t(def.descriptionKey); titleWrap.append(desc); }
    header.append(iconBox, titleWrap);
    const body = el('div', 'card-body');
    for (const component of def.components || []) {
      const node = renderComponent(component);
      if (node) body.append(node);
    }
    card.append(header, body);
    bindVisibility(card, def.visibleWhen);
    return card;
  }

  function renderView(viewId) {
    const def = UI_CONFIG.views[viewId] || UI_CONFIG.views.home;
    state.activeView = viewId;
    Bindings.clearView();
    const content = $('#content');
    content.replaceChildren();

    const heading = el('div', 'view-heading');
    const h1 = el('h1'); h1.textContent = t(def.titleKey);
    const p = el('p'); p.textContent = t(def.descriptionKey);
    heading.append(h1, p);
    const grid = el('div', 'card-grid');
    for (const cardDef of def.cards) if (cardDef.visible !== false) grid.append(renderCard(cardDef));
    content.append(heading, grid);

    for (const button of document.querySelectorAll('.nav-button')) button.setAttribute('aria-selected', button.dataset.view === viewId ? 'true' : 'false');
    if (viewId === 'device') Transport.loadDeviceOnce();
    if (viewId === 'analytics') Transport.loadAnalytics(true);
  }

  function syncClock(time) {
    const parts = time?.parts;
    if (time?.valid && Number(time.epochMs) > 0) {
      state.time = { valid: true, source: time.source || time.activeSource || 'system', quality: time.quality || 'valid', epochMs: Number(time.epochMs), syncPerf: performance.now() };
    } else if (time?.valid && parts && Number(parts.year) > 0) {
      const local = new Date(Number(parts.year), Number(parts.month) - 1, Number(parts.day), Number(parts.hour), Number(parts.minute), Number(parts.second));
      state.time = { valid: true, source: time.source || 'hardware', quality: time.quality || 'valid', epochMs: local.getTime(), syncPerf: performance.now() };
    } else if (time && time.valid && Number(time.epoch) > 0) {
      state.time = { valid: true, source: time.source || 'system', quality: time.quality || 'valid', epochMs: Number(time.epoch) * 1000, syncPerf: performance.now() };
    } else {
      state.time = { valid: false, source: 'relative', quality: 'relative', epochMs: 0, syncPerf: performance.now() };
    }
    updateClock();
  }

  function currentDate() {
    if (!state.time.valid) return new Date();
    return new Date(state.time.epochMs + (performance.now() - state.time.syncPerf));
  }

  function isoWeekYmd(year, month, dayOfMonth) {
    const utc = new Date(Date.UTC(year, month - 1, dayOfMonth));
    const day = utc.getUTCDay() || 7;
    utc.setUTCDate(utc.getUTCDate() + 4 - day);
    const yearStart = new Date(Date.UTC(utc.getUTCFullYear(), 0, 1));
    return { year: utc.getUTCFullYear(), week: Math.ceil((((utc - yearStart) / 86400000) + 1) / 7) };
  }

  function projectDateParts(date = currentDate()) {
    try {
      const parts = new Intl.DateTimeFormat('en-CA', {
        timeZone: state.project.timeZone || 'Europe/Berlin', year: 'numeric', month: '2-digit', day: '2-digit'
      }).formatToParts(date);
      const values = Object.fromEntries(parts.filter(part => part.type !== 'literal').map(part => [part.type, Number(part.value)]));
      if (values.year && values.month && values.day) return { year: values.year, month: values.month, day: values.day };
    } catch (_) {}
    return { year: date.getFullYear(), month: date.getMonth() + 1, day: date.getDate() };
  }

  function projectCurrentCalendar() {
    const parts = projectDateParts();
    return { ...parts, ...isoWeekYmd(parts.year, parts.month, parts.day) };
  }

  function updateClock() {
    const clock = $('#clock');
    const date = $('#date');
    const week = $('#week');
    if (!state.time.valid) {
      // Never present the browser's wall clock as device time while the ESP32
      // explicitly reports that only relative time is available.
      clock.textContent = '--:--:--';
      date.textContent = '--.--.----';
      week.textContent = '';
    } else {
      const now = currentDate();
      const locale = state.preferences.language === 'en' ? 'en-GB' : 'de-DE';
      clock.textContent = now.toLocaleTimeString(locale, { timeZone: state.project.timeZone || undefined, hour: '2-digit', minute: '2-digit', second: '2-digit' });
      date.textContent = now.toLocaleDateString(locale, { timeZone: state.project.timeZone || undefined, day: '2-digit', month: '2-digit', year: 'numeric' });
      week.textContent = `| ${t('time.week')} ${projectCurrentCalendar().week}`;
    }
    if (state.activeView === 'device') Bindings.notify('device.uptimeMs');
    Bindings.notify('clock.tick');
  }

  function syncUptime(ms) {
    if (!Number.isFinite(Number(ms))) return;
    state.uptime.baseMs = Number(ms);
    state.uptime.syncPerf = performance.now();
    Bindings.notify('device.uptimeMs');
  }

  function applyTimeManagementPayload(timeManagement, status = null) {
    if (!timeManagement) return;
    const patch = { timeManagement };
    if (status) patch.status = { ...status, api: 'ok' };
    patchState(patch);
    Bindings.notify('timeManagement');
    syncClock({ valid: timeManagement.valid, epochMs: timeManagement.epochMs, source: timeManagement.activeSource, quality: timeManagement.quality });
  }

  const Transport = {
    async request(path, options = {}) {
      const controller = new AbortController();
      const timeout = setTimeout(() => controller.abort(), 5000);
      try {
        const response = await fetch(path, { cache: 'no-store', headers: { Accept: 'application/json' }, signal: controller.signal, ...options });
        if (!response.ok) throw new Error(`HTTP ${response.status}`);
        return await response.json();
      } finally {
        clearTimeout(timeout);
      }
    },
    async requestResult(path, options = {}) {
      const controller = new AbortController();
      const timeout = setTimeout(() => controller.abort(), 7000);
      try {
        const response = await fetch(path, { cache: 'no-store', headers: { Accept: 'application/json' }, signal: controller.signal, ...options });
        let data = {};
        try { data = await response.json(); } catch (_) { data = {}; }
        if (!response.ok || data?.ok === false) {
          const error = new Error(data?.error || `HTTP ${response.status}`);
          error.code = data?.error || 'unknown';
          error.status = response.status;
          throw error;
        }
        return data;
      } finally {
        clearTimeout(timeout);
      }
    },
    async captureInterruption(button = null) {
      if (button) { button.disabled = true; button.dataset.state = 'busy'; }
      try {
        const data = await this.requestResult('/api/interruptions/event', { method: 'POST' });
        applyInterruptionSummary(data.summary || {});
        if (button) button.dataset.state = 'ok';
        return true;
      } catch (error) {
        console.warn('Interruption capture failed:', error);
        if (button) button.dataset.state = 'error';
        alert(t('interruptions.captureFailed'));
        return false;
      } finally {
        if (button) setTimeout(() => { button.disabled = false; button.dataset.state = ''; }, 140);
      }
    },
    async setInterruptionSound(enabled, input = null) {
      if (input) input.disabled = true;
      try {
        const data = await this.requestResult(`/api/interruptions/sound?enabled=${enabled ? '1' : '0'}`, { method: 'POST' });
        applyInterruptionSummary(data.summary || {});
      } catch (error) {
        console.warn('Sound preference failed:', error);
        if (input) input.checked = !!state.interruptions.soundEnabled;
      } finally { if (input) input.disabled = false; }
    },
    async setProjectPreference(field, value, input = null) {
      if (input) input.disabled = true;
      try {
        const data = await this.requestResult(`/api/interruptions/preferences?${encodeURIComponent(field)}=${encodeURIComponent(value)}`, { method: 'POST' });
        if (data.projectSettings) {
          patchState({ projectSettings: data.projectSettings });
          Bindings.notify('projectSettings');
        }
        if (data.summary) applyInterruptionSummary(data.summary);
        const message = document.querySelector('[data-project-setting-message]');
        if (message) { message.textContent = ''; message.dataset.state = 'ok'; }
        return true;
      } catch (error) {
        console.warn('Project preference failed:', field, error);
        Bindings.notify('projectSettings');
        const message = document.querySelector('[data-project-setting-message]');
        if (message) { message.textContent = t('project.preferenceError'); message.dataset.state = 'error'; }
        return false;
      } finally { if (input) input.disabled = false; }
    },
    async liveInterruptionTick() {
      if (this._liveBusy || !['home', 'analytics'].includes(state.activeView) || document.visibilityState !== 'visible') return;
      this._liveBusy = true;
      const controller = new AbortController();
      const timeout = setTimeout(() => controller.abort(), 2500);
      try {
        const response = await fetch(`/api/interruptions/live?since=${encodeURIComponent(state.interruptions.revision || state.interruptions.sequence || 0)}`, { cache: 'no-store', headers: { Accept: 'application/json' }, signal: controller.signal });
        if (response.status === 204) return;
        if (!response.ok) throw new Error(`HTTP ${response.status}`);
        const data = await response.json();
        applyInterruptionSummary(data.summary || {});
        if (state.activeView === 'analytics' && state.analytics.dirty && !this._analyticsRefreshTimer) {
          // A physical button can change statistics while the analytics view is
          // open. Refresh after a short one-shot debounce; the live summary is
          // still tiny and the event feedback/persistence path has priority.
          this._analyticsRefreshTimer = setTimeout(() => {
            this._analyticsRefreshTimer = null;
            if (state.activeView === 'analytics' && state.analytics.dirty && !state.analytics.loading) this.loadAnalytics(true);
          }, 350);
        }
      } catch (error) {
        if (error?.name !== 'AbortError') console.warn('Live interruption update failed:', error);
      } finally { clearTimeout(timeout); this._liveBusy = false; }
    },
    async loadAnalytics(force = false) {
      if (state.analytics.loading) return;
      if (!force && state.analytics.loaded && !state.analytics.dirty) return;
      state.analytics.loading = true;
      try {
        await this.loadAnalyticsBundle();
        patchState({ analytics: { error: '' } });
        state.analytics.loaded = true;
        state.analytics.dirty = false;
      } catch (error) {
        console.warn('Analytics load failed:', error);
        patchState({ analytics: { error: error?.message || 'load_failed' } });
        // Recovery is cooperative. Keep the visible analytics view dirty so the
        // single project tick can retry without a separate polling timer.
        state.analytics.dirty = true;
      } finally { state.analytics.loading = false; }
    },
    async loadAnalyticsBundle() {
      const hourlyRoot = document.querySelector('[data-heatmap-kind="hourly"]');
      const monthWeekRoot = document.querySelector('[data-heatmap-kind="monthWeek"]');
      const current = projectCurrentCalendar();
      const mode = hourlyRoot?.querySelector('[data-analytics-hourly-mode]')?.value || state.analytics.hourlyMode || 'week';
      state.analytics.hourlyMode = mode;

      const metric = state.analytics.metric || 'count';
      const source = state.analytics.source || 'all';
      const parts = [`metric=${encodeURIComponent(metric)}`, `source=${encodeURIComponent(source)}`, `hourlyMode=${encodeURIComponent(mode)}`];
      if (mode === 'range') {
        const from = hourlyRoot?.querySelector('[data-analytics-hourly-from]')?.value || '';
        const to = hourlyRoot?.querySelector('[data-analytics-hourly-to]')?.value || '';
        parts.push(`from=${encodeURIComponent(from)}`, `to=${encodeURIComponent(to)}`);
      } else {
        const year = hourlyRoot?.querySelector('[data-analytics-hourly-year]')?.value || current.year;
        const week = hourlyRoot?.querySelector('[data-analytics-hourly-week]')?.value || current.week;
        parts.push(`hourlyYear=${encodeURIComponent(year)}`, `hourlyWeek=${encodeURIComponent(week)}`);
      }
      const monthWeekYear = monthWeekRoot?.querySelector('[data-analytics-month-week-year]')?.value || current.year;
      parts.push(`monthWeekYear=${encodeURIComponent(monthWeekYear)}`);

      const data = await this.request(`/api/interruptions/analytics?${parts.join('&')}`);
      patchState({ analytics: {
        storage: data.storage || null,
        hourly: data.hourly || null,
        monthWeek: data.monthWeek || null,
        yearMonth: data.yearMonth || null
      } });
      for (const key of ['storage','hourly','monthWeek','yearMonth']) Bindings.notify(`analytics.${key}`);
    },
    async loadAnalyticsStorage() {
      const data = await this.request('/api/interruptions/storage');
      patchState({ analytics: { storage: data.storage || null } });
      Bindings.notify('analytics.storage');
    },
    async eraseDatabase(password) {
      const body = new URLSearchParams({ password: String(password || '') }).toString();
      const data = await this.requestResult('/api/interruptions/storage/reset', {
        method: 'POST',
        headers: { Accept: 'application/json', 'Content-Type': 'application/x-www-form-urlencoded;charset=UTF-8' },
        body
      });
      setTimeout(() => location.reload(), 3500);
      return data;
    },
    async loadHourlyHeatmap(root = null) {
      root = root || document.querySelector('[data-heatmap-kind="hourly"]');
      let query = '';
      if (root) {
        const mode = root.querySelector('[data-analytics-hourly-mode]')?.value || state.analytics.hourlyMode || 'week';
        state.analytics.hourlyMode = mode;
        if (mode === 'range') {
          const from = root.querySelector('[data-analytics-hourly-from]')?.value || '';
          const to = root.querySelector('[data-analytics-hourly-to]')?.value || '';
          query = `mode=range&from=${encodeURIComponent(from)}&to=${encodeURIComponent(to)}`;
        } else {
          const current = projectCurrentCalendar();
          const year = root.querySelector('[data-analytics-hourly-year]')?.value || current.year;
          const week = root.querySelector('[data-analytics-hourly-week]')?.value || current.week;
          query = `mode=week&year=${encodeURIComponent(year)}&week=${encodeURIComponent(week)}`;
        }
      } else {
        const current = projectCurrentCalendar();
        query = `mode=week&year=${current.year}&week=${current.week}`;
      }
      const data = await this.request(`/api/interruptions/heatmap/hourly?${query}`);
      patchState({ analytics: { hourly: data, error: '' } });
      Bindings.notify('analytics.hourly');
    },
    async loadMonthWeekHeatmap(root = null) {
      root = root || document.querySelector('[data-heatmap-kind="monthWeek"]');
      const year = root?.querySelector('[data-analytics-month-week-year]')?.value || projectCurrentCalendar().year;
      const data = await this.request(`/api/interruptions/heatmap/month-week?year=${encodeURIComponent(year)}`);
      patchState({ analytics: { monthWeek: data, error: '' } });
      Bindings.notify('analytics.monthWeek');
    },
    async loadYearMonthHeatmap() {
      const data = await this.request('/api/interruptions/heatmap/year-month');
      patchState({ analytics: { yearMonth: data, error: '' } });
      Bindings.notify('analytics.yearMonth');
    },
    downloadInterruptionCsv() {
      const link = document.createElement('a');
      link.href = '/api/interruptions/export.csv';
      link.download = '';
      document.body.append(link); link.click(); link.remove();
    },
    projectTick() {
      const now = performance.now();
      const livePollMs = Math.max(1000, Number(state.project.livePollMs || 1000));
      if (!this._lastLiveTick || now - this._lastLiveTick >= livePollMs) {
        this._lastLiveTick = now;
        this.liveInterruptionTick();
      }
      if (state.activeView === 'analytics' && state.analytics.dirty && !state.analytics.loading) this.loadAnalytics(true);
    },
    async loadBootstrap() {
      state.status.api = 'busy'; updateStatusBar();
      try {
        const data = await this.request('/api/bootstrap');
        if (data.preferences) {
          state.preferences.fallbackLanguage = data.preferences.fallbackLanguage || state.preferences.fallbackLanguage;
          if (Array.isArray(data.preferences.languages)) state.preferences.availableLanguages = [...new Set([...data.preferences.languages.filter(code => I18N[code]), ...Object.keys(I18N)])];
          if (!state.preferences.languageStored) {
            state.preferences.language = detectLanguage();
            state.preferences.languageMode = 'auto';
            document.documentElement.lang = state.preferences.language;
          }
          if (!state.preferences.themeStored && ['system', 'light', 'dark'].includes(data.preferences.defaultTheme)) {
            state.preferences.theme = data.preferences.defaultTheme;
            applyTheme();
          }
        }
        applyStatusProviders(data.statusProviders || []);
        patchState({ project: data.project || {}, firmware: data.firmware || {}, projectSettings: data.projectSettings || state.projectSettings, timeManagement: data.timeManagement || state.timeManagement, status: { ...(data.status || {}), api: 'ok' } });
        Bindings.notify('projectSettings');
        const deviceLanguage = data.projectSettings?.language || '';
        const deviceLanguageStored = data.projectSettings?.languageStored === true;
        if (!state.preferences.languageStored && deviceLanguageStored && state.preferences.availableLanguages.includes(deviceLanguage)) {
          state.preferences.language = deviceLanguage;
          state.preferences.languageMode = 'manual';
          state.preferences.languageStored = true;
          PreferenceStore.set('language', deviceLanguage);
          document.documentElement.lang = deviceLanguage;
        } else if (state.preferences.availableLanguages.includes(state.preferences.language) &&
                   (!deviceLanguageStored || deviceLanguage !== state.preferences.language)) {
          this.setProjectPreference('language', state.preferences.language);
        }
        if (data.interruptions) applyInterruptionSummary(data.interruptions);
        Bindings.notify('timeManagement');
        syncClock(data.time);
        renderBrand(); updateNavigationText(); renderFooter(); updateStatusBar(); renderView(state.activeView);
        if (data.timeManagement?.checking) {
          this.followTimeOperation(null, data.timeManagement?.operation?.id || 0, 'boot', true);
        } else if (data.timeManagement?.browserFallbackAllowed) {
          this.sendBrowserTime(null, true);
        }
      } catch (error) {
        console.warn('Bootstrap failed:', error);
        patchState({ status: { api: 'error' } });
        syncClock(null);
      }
    },
    async loadDeviceOnce() {
      if (state.connection.deviceRequested || state.connection.deviceLoading) return;
      state.connection.deviceRequested = true;
      state.connection.deviceLoading = true;
      patchState({ status: { api: 'busy' } });
      try {
        const data = await this.request('/api/device');
        const statusPatch = { api: 'ok', wifi: data.wifi?.state || 'unknown' };
        patchState({ device: data.device || {}, wifi: data.wifi || {}, memory: data.memory || {}, storage: data.storage || {}, hardware: data.hardware || { checking: false, modules: [] }, ota: data.ota || {}, status: statusPatch });
        syncUptime(data.device?.uptimeMs);
        state.connection.deviceLoaded = true;
      } catch (error) {
        console.warn('Initial device request failed:', error);
        patchState({ status: { api: 'error' } });
      } finally {
        state.connection.deviceLoading = false;
      }
    },
    async refreshHardwareState() {
      const data = await this.request('/api/hardware');
      patchState({ hardware: data.hardware || { checking: false, modules: [] }, status: { ...(data.status || {}), api: 'ok' } });
      return data;
    },
    async checkHardware(id = '') {
      const query = id ? `?id=${encodeURIComponent(id)}` : '';
      try {
        const data = await this.requestResult(`/api/hardware/check${query}`, { method: 'POST' });
        patchState({ hardware: data.hardware || { checking: false, modules: [] }, status: { ...(data.status || {}), api: 'ok' } });
        if (data.hardware?.checking) this.followHardwareCheck(0);
      } catch (error) {
        console.warn('Hardware check failed:', error);
        // A known module can reject a second check while a command is still
        // being verified. That is not an API outage; refresh cached health.
        try { await this.refreshHardwareState(); } catch (_) { patchState({ status: { api: 'error' } }); }
      }
    },
    async hardwareAction(moduleId, actionId) {
      const query = `?id=${encodeURIComponent(moduleId)}&action=${encodeURIComponent(actionId)}`;
      try {
        const data = await this.request(`/api/hardware/action${query}`, { method: 'POST' });
        patchState({ hardware: data.hardware || { checking: false, modules: [] }, status: { ...(data.status || {}), api: 'ok' } });
        this.followHardwareCheck(0);
      } catch (error) {
        console.warn('Hardware action failed:', error);
        try { await this.refreshHardwareState(); } catch (_) {}
        alert(t('hardware.action.failed'));
      }
    },
    async followHardwareCheck(attempt) {
      await new Promise(resolve => setTimeout(resolve, attempt === 0 ? 380 : 300));
      try {
        const data = await this.refreshHardwareState();
        if (data.hardware?.checking && attempt < 4) this.followHardwareCheck(attempt + 1);
      } catch (error) {
        console.warn('Hardware check follow-up failed:', error);
      }
    },
    async followTimeOperation(root = null, expectedId = 0, mode = 'check', silent = false) {
      // NTP response waiting is cooperative on the ESP32. Follow only this
      // explicit operation for a bounded time; this is not background polling.
      for (let attempt = 0; attempt < 12; attempt += 1) {
        await new Promise(resolve => setTimeout(resolve, attempt === 0 ? 220 : 250));
        try {
          const data = await this.requestResult('/api/time');
          applyTimeManagementPayload(data.time, data.status || null);
          const tm = data.time || {};
          if (tm.checking) continue;

          const operation = tm.operation || {};
          if (expectedId && Number(operation.id || 0) !== Number(expectedId)) continue;
          if (operation.ok === false && operation.error && operation.error !== 'none') {
            if (!silent) setTimeUiMessage(root, 'error', timeErrorText(operation.error));
            return null;
          }

          if (tm.browserFallbackAllowed) {
            await this.sendBrowserTime(root, silent);
            return data;
          }

          if (mode === 'ntp') {
            const input = root ? $('[data-time-ntp-input]', root) : null;
            if (input) { input.dataset.dirty = '0'; input.value = tm.ntpServer || input.value; }
            if (!silent) setTimeUiMessage(root, 'ok', t('time.ntpSaved'));
          } else if (!silent) {
            setTimeUiMessage(root, 'ok', t('time.checked'));
          }
          return data;
        } catch (error) {
          if (attempt === 11) {
            console.warn('Time operation follow-up failed:', error);
            if (!silent) setTimeUiMessage(root, 'error', timeErrorText(error.code || 'timeout'));
            return null;
          }
        }
      }
      if (!silent) setTimeUiMessage(root, 'error', timeErrorText('timeout'));
      return null;
    },
    async checkTime(root = null) {
      setTimeUiMessage(root, 'busy', t('time.checking'));
      try {
        const data = await this.requestResult('/api/time/check', { method: 'POST' });
        applyTimeManagementPayload(data.time, data.status || null);
        if (data.time?.checking) {
          return await this.followTimeOperation(root, data.time?.operation?.id || 0, 'check', false);
        }
        if (data.time?.browserFallbackAllowed) await this.sendBrowserTime(root, false);
        else setTimeUiMessage(root, 'ok', t('time.checked'));
        return data;
      } catch (error) {
        console.warn('Time check failed:', error);
        setTimeUiMessage(root, 'error', timeErrorText(error.code || 'unknown'));
        return null;
      }
    },
    async saveNtpServer(root, serverName) {
      const candidate = String(serverName || '').trim();
      if (!candidate) { setTimeUiMessage(root, 'error', timeErrorText('invalid_server')); return null; }
      setTimeUiMessage(root, 'busy', t('time.ntpChecking'));
      try {
        const path = `/api/time/ntp?server=${encodeURIComponent(candidate)}`;
        const data = await this.requestResult(path, { method: 'POST' });
        applyTimeManagementPayload(data.time, data.status || null);
        if (data.time?.checking) {
          return await this.followTimeOperation(root, data.time?.operation?.id || 0, 'ntp', false);
        }
        return data;
      } catch (error) {
        console.warn('NTP server validation failed:', error);
        setTimeUiMessage(root, 'error', timeErrorText(error.code || 'unknown'));
        return null;
      }
    },
    async sendBrowserTime(root = null, silent = false) {
      if (this.browserFallbackSubmitting) return null;
      this.browserFallbackSubmitting = true;
      try {
        const epochMs = Date.now();
        const tzOffset = new Date().getTimezoneOffset();
        const path = `/api/time/browser?epochMs=${encodeURIComponent(epochMs)}&tzOffset=${encodeURIComponent(tzOffset)}`;
        const data = await this.requestResult(path, { method: 'POST' });
        applyTimeManagementPayload(data.time, data.status || null);
        if (!silent) setTimeUiMessage(root, 'ok', t('time.browserAccepted'));
        return data;
      } catch (error) {
        console.warn('Browser time fallback failed:', error);
        if (!silent) setTimeUiMessage(root, 'error', timeErrorText(error.code || 'unknown'));
        return null;
      } finally {
        this.browserFallbackSubmitting = false;
      }
    }
  };

  function otaFailureText(result) {
    const stage = t(`ota.stage.${result?.stage || 'unknown'}`);
    const reason = t(`ota.reason.${result?.reason || 'unknown'}`);
    const technical = result?.error ? ` (${result.error}${Number.isFinite(Number(result.errorCode)) ? `, Code ${result.errorCode}` : ''})` : '';
    return `${t('ota.failedPrefix')} – ${stage}: ${reason}${technical}`;
  }

  const UploadTransport = {
    setStatus(root, stateName, text) {
      const status = $('.upload-status', root);
      if (!status) return;
      status.dataset.state = stateName;
      status.textContent = text;
    },
    setProgress(root, percent, visible = true) {
      const wrap = $('.upload-progress', root);
      const track = $('.upload-progress-track', root);
      const value = $('.upload-progress-value', root);
      const text = $('.upload-progress-text', root);
      if (!wrap || !track || !value || !text) return;
      const p = Math.max(0, Math.min(100, Number(percent) || 0));
      wrap.hidden = !visible;
      value.style.width = `${p.toFixed(1)}%`;
      text.textContent = `${Math.round(p)} %`;
      track.setAttribute('aria-valuenow', p.toFixed(1));
    },
    async waitForRestart(root) {
      this.setStatus(root, 'busy', `${t('ota.success')} ${t('ota.reconnecting')}`);
      await new Promise(resolve => setTimeout(resolve, 3000));
      for (let attempt = 0; attempt < 12; attempt++) {
        const controller = new AbortController();
        const timeout = setTimeout(() => controller.abort(), 1200);
        try {
          const response = await fetch('/api/bootstrap', { cache: 'no-store', signal: controller.signal });
          if (response.ok) {
            clearTimeout(timeout);
            this.setStatus(root, 'ok', t('ota.reconnected'));
            setTimeout(() => location.reload(), 500);
            return;
          }
        } catch (_) {
          // Expected while the ESP32 is rebooting.
        } finally {
          clearTimeout(timeout);
        }
        await new Promise(resolve => setTimeout(resolve, 1000));
      }
      this.setStatus(root, 'warning', t('ota.reconnectFailed'));
    },
    start(root) {
      const input = $('.upload-input', root);
      const button = $('[data-upload-button]', root);
      const file = input?.files?.[0];
      if (!file) { this.setStatus(root, 'error', t('ota.invalidFile')); return; }
      if (!String(file.name).toLowerCase().endsWith('.bin')) { this.setStatus(root, 'error', t('ota.invalidFile')); return; }

      const maxPath = root.dataset.maxPath;
      const maxBytes = maxPath ? Number(getPath(maxPath)) : 0;
      if (maxBytes > 0 && file.size > maxBytes) {
        this.setStatus(root, 'error', `${t('ota.fileTooLarge')} ${Formats.bytes(file.size)} > ${Formats.bytes(maxBytes)}`);
        return;
      }

      input.disabled = true;
      button.disabled = true;
      this.setProgress(root, 0, true);
      this.setStatus(root, 'busy', t('ota.uploading'));

      const form = new FormData();
      form.append(root.dataset.fieldName || 'file', file, file.name);
      const xhr = new XMLHttpRequest();
      xhr.open('POST', root.dataset.endpoint, true);
      xhr.timeout = 180000;
      xhr.setRequestHeader('Accept', 'application/json');
      xhr.setRequestHeader('X-Firmware-Size', String(file.size));

      xhr.upload.onprogress = event => {
        if (!event.lengthComputable) return;
        const percent = (event.loaded / event.total) * 100;
        this.setProgress(root, percent, true);
        if (percent >= 99.9) this.setStatus(root, 'busy', t('ota.verifying'));
      };

      const finish = () => { input.disabled = false; button.disabled = !input.files?.length; };
      xhr.onload = () => {
        finish();
        let result = null;
        try { result = JSON.parse(xhr.responseText || '{}'); } catch (_) { /* handled below */ }
        if (xhr.status >= 200 && xhr.status < 300 && result?.ok) {
          this.setProgress(root, 100, true);
          this.setStatus(root, 'ok', t('ota.success'));
          this.waitForRestart(root);
          return;
        }
        this.setStatus(root, 'error', result ? otaFailureText(result) : `${t('ota.failedPrefix')} – HTTP ${xhr.status}`);
      };
      xhr.onerror = () => { finish(); this.setStatus(root, 'error', t('ota.networkError')); };
      xhr.ontimeout = () => { finish(); this.setStatus(root, 'error', t('ota.networkError')); };
      xhr.onabort = () => { finish(); this.setStatus(root, 'error', t('ota.reason.aborted')); };
      xhr.send(form);
    }
  };

  const Actions = { 'hardware-check-all': () => Transport.checkHardware('') };

  document.addEventListener('click', async event => {
    const nav = event.target.closest('[data-view]');
    if (nav) { renderView(nav.dataset.view); $('#content').focus({ preventScroll: true }); return; }
    const interruptionButton = event.target.closest('[data-interruption-trigger]');
    if (interruptionButton) { await Transport.captureInterruption(interruptionButton); return; }
    const analyticsAction = event.target.closest('[data-analytics-action]');
    if (analyticsAction) {
      const root = analyticsAction.closest('[data-heatmap-kind]');
      if ((state.analytics.metric || 'count') === 'averageInterval' || (state.analytics.source || 'all') !== 'all') await Transport.loadAnalytics(true);
      else if (analyticsAction.dataset.analyticsAction === 'hourly') await Transport.loadHourlyHeatmap(root);
      else if (analyticsAction.dataset.analyticsAction === 'month-week') await Transport.loadMonthWeekHeatmap(root);
      return;
    }
    const csvButton = event.target.closest('[data-analytics-download]');
    if (csvButton) { Transport.downloadInterruptionCsv(); return; }
    const uploadButton = event.target.closest('[data-upload-button]');
    if (uploadButton) {
      const root = uploadButton.closest('.upload-control');
      if (root) UploadTransport.start(root);
      return;
    }
    const timeButton = event.target.closest('[data-time-action]');
    if (timeButton) {
      const root = timeButton.closest('.time-management');
      const actionId = timeButton.dataset.timeAction || '';
      timeButton.disabled = true;
      try {
        if (actionId === 'check') await Transport.checkTime(root);
        else if (actionId === 'ntp-save') {
          const input = root ? $('[data-time-ntp-input]', root) : null;
          await Transport.saveNtpServer(root, input?.value || '');
        }
      } finally {
        timeButton.disabled = false;
      }
      return;
    }
    const hardwareActionButton = event.target.closest('[data-hardware-action]');
    if (hardwareActionButton) {
      Transport.hardwareAction(hardwareActionButton.dataset.hardwareModule || '', hardwareActionButton.dataset.hardwareAction || '');
      return;
    }
    const hardwareButton = event.target.closest('[data-hardware-check]');
    if (hardwareButton) { Transport.checkHardware(hardwareButton.dataset.hardwareCheck || ''); return; }
    const button = event.target.closest('[data-action]');
    if (button) {
      const confirmKey = button.dataset.confirmKey;
      if (confirmKey && !confirm(t(confirmKey))) return;
      const action = Actions[button.dataset.action];
      if (action) action();
    }
  });

  document.addEventListener('change', event => {
    const metricControl = event.target.closest('[data-analytics-metric]');
    if (metricControl) {
      state.analytics.metric = metricControl.value === 'averageInterval' ? 'averageInterval' : 'count';
      for (const control of document.querySelectorAll('[data-analytics-metric]')) control.value = state.analytics.metric;
      state.analytics.dirty = true;
      Transport.loadAnalytics(true);
      return;
    }
    const sourceControl = event.target.closest('[data-analytics-source]');
    if (sourceControl) {
      const allowed = ['all','physical_button','web_button'];
      state.analytics.source = allowed.includes(sourceControl.value) ? sourceControl.value : 'all';
      for (const control of document.querySelectorAll('[data-analytics-source]')) control.value = state.analytics.source;
      state.analytics.dirty = true;
      Transport.loadAnalytics(true);
      return;
    }
    const projectSetting = event.target.closest('[data-project-setting]');
    if (projectSetting) {
      const field = projectSetting.dataset.projectSetting || '';
      const value = projectSetting.type === 'checkbox' ? (projectSetting.checked ? '1' : '0') : projectSetting.value;
      Transport.setProjectPreference(field, value, projectSetting);
      return;
    }
    const uploadInput = event.target.closest('[data-upload-input]');
    if (uploadInput) {
      const root = uploadInput.closest('.upload-control');
      const button = root ? $('[data-upload-button]', root) : null;
      const selected = root ? $('.upload-selected', root) : null;
      const file = uploadInput.files?.[0];
      if (button) button.disabled = !file;
      if (root) UploadTransport.setProgress(root, 0, false);
      if (!file) {
        if (selected) selected.textContent = t('ota.idle');
        if (root) UploadTransport.setStatus(root, 'idle', t('ota.idle'));
      } else if (!String(file.name).toLowerCase().endsWith('.bin')) {
        if (selected) selected.textContent = `${file.name} · ${Formats.bytes(file.size)}`;
        if (root) UploadTransport.setStatus(root, 'error', t('ota.invalidFile'));
        if (button) button.disabled = true;
      } else {
        if (selected) selected.textContent = `${file.name} · ${Formats.bytes(file.size)}`;
        const maxBytes = root?.dataset.maxPath ? Number(getPath(root.dataset.maxPath)) : 0;
        const tooLarge = maxBytes > 0 && file.size > maxBytes;
        if (root) UploadTransport.setStatus(root, tooLarge ? 'error' : 'ready', tooLarge ? t('ota.fileTooLarge') : t('ota.ready'));
        if (button) button.disabled = tooLarge;
      }
      return;
    }
    const control = event.target.closest('[data-preference]');
    if (!control) return;
    if (control.dataset.preference === 'language') setLanguage(control.value);
    else if (control.dataset.preference === 'theme') setTheme(control.value);
    else if (control.type === 'checkbox') {
      state.preferences[control.dataset.preference] = control.checked;
      PreferenceStore.set(control.dataset.preference, control.checked ? '1' : '0');
      Bindings.notify(`preferences.${control.dataset.preference}`);
    }
  });

  initPreferences();
  buildNavigation();
  buildStatusBar();
  renderBrand();
  renderFooter();
  renderView('home');
  syncClock(null);
  setInterval(() => { updateClock(); Transport.projectTick(); }, 1000); // Single UI timer: clock/uptime plus visibility-aware project live check.
  Transport.loadBootstrap();
})();
