===========================================================
MERIDIAN 59 - WEB-BASIERTER ACCOUNT MANAGER
===========================================================

INSTALLATION:
-------------

1. Voraussetzungen:
   - PHP 7.0 oder höher
   - Webserver (Apache, Nginx, IIS)
   - Laufender Blakserv Server

2. Datei kopieren:
   Kopieren Sie account_manager.php in Ihr Webserver-Verzeichnis:
   - XAMPP: C:\xampp\htdocs\
   - WAMP: C:\wamp\www\
   - IIS: C:\inetpub\wwwroot\

3. Blakserv konfigurieren:
   Öffnen Sie blakserv.cfg und stellen Sie sicher, dass diese Zeile vorhanden ist:

   SOCKET_PORT_MAINTENANCE 9998

4. Zugriff:
   Öffnen Sie im Browser:
   http://localhost/account_manager.php


VERWENDUNG:
-----------

Die Web-Oberfläche hat 3 Tabs:

1. SCHNELLERSTELLUNG:
   - Einfachste Methode für normale Spieler
   - Erstellt Account + User-Slots automatisch
   - Eingabe: Benutzername, Passwort, E-Mail

2. ERWEITERT:
   - Für spezielle Account-Typen
   - Wählbar: USER, ADMIN, DM, GUEST
   - Eingabe: Typ, Benutzername, Passwort, E-Mail

3. VERWALTEN:
   - Alle Accounts anzeigen
   - Bestimmten Account per ID anzeigen


FUNKTIONEN:
-----------

✓ Modernes, responsives Design
✓ Echtzeit-Verbindungsstatus zum Server
✓ Direkte Kommunikation mit Blakserv Maintenance Port
✓ Eingabevalidierung (Client + Server)
✓ Fehlerbehandlung und hilfreiche Meldungen
✓ Keine Datenbank erforderlich


SICHERHEIT:
-----------

WICHTIG - Produktionsumgebung:

1. Schützen Sie die Seite mit Passwort (.htaccess):

   AuthType Basic
   AuthName "Account Manager"
   AuthUserFile /pfad/.htpasswd
   Require valid-user

2. Verwenden Sie HTTPS statt HTTP

3. Beschränken Sie den Zugriff auf bestimmte IP-Adressen

4. Der Maintenance Port sollte nur von localhost erreichbar sein


KONFIGURATION:
--------------

In account_manager.php können Sie anpassen (Zeilen 8-10):

$MAINTENANCE_HOST = 'localhost';  // Server-Adresse
$MAINTENANCE_PORT = 9998;         // Maintenance Port
$TIMEOUT = 5;                     // Verbindungs-Timeout in Sekunden


TROUBLESHOOTING:
----------------

Problem: "Server Offline" Anzeige
Lösung:
  - Starten Sie den Blakserv Server
  - Prüfen Sie, ob Port 9998 offen ist
  - Testen Sie: telnet localhost 9998

Problem: "Verbindung zum Server fehlgeschlagen"
Lösung:
  - Prüfen Sie die Firewall-Einstellungen
  - Stellen Sie sicher, dass PHP socket-Funktionen aktiviert sind
  - Prüfen Sie die Port-Konfiguration in blakserv.cfg

Problem: Keine Antwort vom Server
Lösung:
  - Erhöhen Sie den TIMEOUT Wert
  - Prüfen Sie die Blakserv Logs

Problem: "Account already exists"
Lösung:
  - Wählen Sie einen anderen Benutzernamen
  - Prüfen Sie mit "Alle Accounts anzeigen"


BEISPIEL-ABLAUF:
----------------

1. Öffnen Sie http://localhost/account_manager.php
2. Server-Status sollte "● Server Online" zeigen (grün)
3. Wählen Sie Tab "Schnellerstellung"
4. Geben Sie ein:
   - Benutzername: TestSpieler
   - Passwort: geheim123
   - E-Mail: test@example.com
5. Klicken Sie "Account erstellen"
6. Erfolg: "✅ Account 'TestSpieler' erfolgreich erstellt!"


ACCOUNT-REGELN:
--------------

- Benutzername: 3-20 Zeichen
- Erlaubte Zeichen: a-z, A-Z, 0-9, _
- NICHT erlaubt: Doppelpunkt (:)
- Passwort: Mindestens 6 Zeichen
- E-Mail: Muss gültiges Format haben


PHP-ANFORDERUNGEN:
------------------

Benötigte PHP-Extensions:
- sockets (oder fsockopen aktiviert)
- Standard PHP-Funktionen

Prüfen Sie in php.ini:
allow_url_fopen = On


WEITERFÜHRENDE INFORMATIONEN:
-----------------------------

Die erstellten Accounts werden direkt in:
  E:\Meridian59_source\04.08.2016\Meridian59-develop\run\server\accounts.txt
gespeichert.

Für erweiterte Verwaltung können Sie weiterhin:
- maintenance.bat verwenden
- Telnet zum Port 9998 verbinden
- Direkt die accounts.txt Datei bearbeiten (Vorsicht!)


Bei Fragen oder Problemen:
- Prüfen Sie die Blakserv Logs
- Testen Sie die Maintenance-Verbindung mit Telnet
- Überprüfen Sie PHP error_log
