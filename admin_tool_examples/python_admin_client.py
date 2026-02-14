#!/usr/bin/env python3
"""
Meridian 59 Admin Tool - Python Client
Verbindet sich zum Maintenance Port des Servers und sendet Admin-Befehle.
Ersetzt unzuverlässige sendkeys-Methoden.
"""

import socket
import sys
import time

class M59AdminClient:
    def __init__(self, host='127.0.0.1', port=9998):
        self.host = host
        self.port = port
        self.sock = None

    def connect(self):
        """Verbindet zum Maintenance Port"""
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.settimeout(10)  # 10 Sekunden Timeout
            self.sock.connect((self.host, self.port))
            print(f"✓ Verbunden mit {self.host}:{self.port}")

            # Optional: Begrüßungsnachricht empfangen
            response = self.receive()
            if response:
                print(f"Server: {response}")

            return True
        except Exception as e:
            print(f"✗ Verbindungsfehler: {e}")
            return False

    def send_command(self, command):
        """Sendet einen Admin-Befehl zum Server"""
        if not self.sock:
            print("✗ Nicht verbunden!")
            return None

        try:
            # Befehl mit Carriage Return abschließen
            cmd_bytes = (command + "\r\n").encode('utf-8')
            self.sock.sendall(cmd_bytes)
            print(f"→ Gesendet: {command}")

            # Warte auf Antwort
            time.sleep(0.1)  # Kurze Pause für Server-Verarbeitung
            response = self.receive()

            return response
        except Exception as e:
            print(f"✗ Fehler beim Senden: {e}")
            return None

    def receive(self, buffer_size=4096):
        """Empfängt Antwort vom Server"""
        try:
            self.sock.settimeout(2)  # 2 Sekunden Timeout für Antwort
            data = self.sock.recv(buffer_size)
            if data:
                response = data.decode('utf-8', errors='ignore')
                return response.strip()
        except socket.timeout:
            return None
        except Exception as e:
            print(f"✗ Fehler beim Empfangen: {e}")
            return None
        return None

    def disconnect(self):
        """Trennt die Verbindung"""
        if self.sock:
            try:
                self.send_command("QUIT")
                self.sock.close()
                print("✓ Verbindung getrennt")
            except:
                pass
            self.sock = None

    def __enter__(self):
        """Context manager support"""
        self.connect()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager support"""
        self.disconnect()


def main():
    """Beispiel-Verwendung"""

    # Beispiel 1: Einfacher Befehl
    print("=== Beispiel 1: Einzelner Befehl ===")
    with M59AdminClient() as client:
        response = client.send_command("show status")
        if response:
            print(f"← Antwort:\n{response}\n")

    # Beispiel 2: Mehrere Befehle
    print("\n=== Beispiel 2: Mehrere Befehle ===")
    with M59AdminClient() as client:
        commands = [
            "show status",
            "show sessions",
            "show memory"
        ]

        for cmd in commands:
            response = client.send_command(cmd)
            if response:
                print(f"← Antwort: {response}\n")
            time.sleep(0.5)

    # Beispiel 3: Account erstellen
    print("\n=== Beispiel 3: Account erstellen ===")
    with M59AdminClient() as client:
        # Account erstellen
        response = client.send_command("create automated testuser testpass123")
        if response:
            print(f"← Antwort: {response}")

        time.sleep(0.5)

        # Status prüfen
        response = client.send_command("show account testuser")
        if response:
            print(f"← Antwort: {response}")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n✓ Abgebrochen durch Benutzer")
        sys.exit(0)
