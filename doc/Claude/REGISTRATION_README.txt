MERIDIAN 59 - ACCOUNT REGISTRIERUNG
====================================

INSTALLATION:

1. Voraussetzungen:
   - PHP 7.0 oder höher
   - MySQL 5.6 oder höher
   - Webserver (Apache, Nginx, etc.)

2. Datenbank einrichten:
   - Öffnen Sie phpMyAdmin oder MySQL Workbench
   - Importieren Sie die Datei: create_accounts_table.sql
   - Oder führen Sie die SQL-Befehle manuell aus

3. Datenbank-Verbindung konfigurieren:
   - Öffnen Sie register.php
   - Passen Sie die Zeilen 4-7 an:
     $db_host = 'localhost';      // Ihr Datenbank-Host
     $db_name = 'meridian59';     // Name Ihrer Datenbank
     $db_user = 'root';           // Datenbank-Benutzername
     $db_pass = '';               // Datenbank-Passwort

4. Datei hochladen:
   - Kopieren Sie register.php in Ihr Webserver-Verzeichnis
   - Normalerweise: C:\xampp\htdocs\ oder /var/www/html/

5. Zugriff:
   - Öffnen Sie im Browser: http://localhost/register.php

SICHERHEITSHINWEISE:

- Ändern Sie das Standard-Admin-Passwort (admin123)!
- Verwenden Sie HTTPS in Produktionsumgebungen
- Setzen Sie sichere Datenbank-Passwörter
- Aktivieren Sie Firewall-Regeln für MySQL

VERWENDUNG:

Benutzer können jetzt Accounts erstellen mit:
- Benutzername (3-20 Zeichen, nur a-z, A-Z, 0-9, _)
- E-Mail-Adresse
- Passwort (mindestens 6 Zeichen)

Die Passwörter werden sicher mit BCrypt gehasht gespeichert.

INTEGRATION MIT MERIDIAN 59:

Sie müssen noch die Account-Daten mit Ihrem Meridian 59 Server
verbinden. Dies hängt von Ihrer Server-Konfiguration ab.

ANPASSUNGEN:

- Design ändern: Bearbeiten Sie den <style> Bereich in register.php
- Validierung anpassen: Ändern Sie die Bedingungen in Zeilen 20-34
- Weitere Felder: Fügen Sie neue Felder zur Tabelle und zum Formular hinzu

TROUBLESHOOTING:

- "Datenbankfehler": Prüfen Sie die Datenbank-Zugangsdaten
- "Table doesn't exist": Importieren Sie create_accounts_table.sql
- Felder werden nicht angezeigt: Prüfen Sie, ob PHP aktiviert ist

Bei Fragen oder Problemen überprüfen Sie die PHP-Fehlerprotokolle.
