#!/usr/bin/env python3
"""
Meridian 59 Client Command Injector - Python
Sendet DM-Befehle direkt an den laufenden Client über Named Pipe.
Ersetzt unzuverlässige sendkeys-Methoden.
"""

import win32pipe
import win32file
import pywintypes
import sys
import time
import psutil

class M59ClientCommandInjector:
    def __init__(self, pid=None):
        """
        Initialisiert den Command Injector.

        Args:
            pid: Process ID des Clients (optional, wird automatisch gefunden wenn None)
        """
        self.pid = pid
        self.pipe_handle = None
        self.pipe_name = None

    def find_client_pid(self):
        """Findet die Process ID des laufenden Meridian59 Clients"""
        for proc in psutil.process_iter(['pid', 'name']):
            try:
                # Suche nach meridian.exe oder clientd3d.exe
                if proc.info['name'].lower() in ['meridian.exe', 'clientd3d.exe', 'client.exe']:
                    print(f"✓ Client gefunden: PID {proc.info['pid']}")
                    return proc.info['pid']
            except (psutil.NoSuchProcess, psutil.AccessDenied):
                pass
        return None

    def connect(self):
        """Verbindet zum Client via Named Pipe"""
        if self.pid is None:
            self.pid = self.find_client_pid()

        if self.pid is None:
            print("✗ Kein laufender Meridian 59 Client gefunden!")
            return False

        self.pipe_name = f"\\\\.\\pipe\\Meridian59_Command_{self.pid}"

        try:
            print(f"→ Verbinde zu Pipe: {self.pipe_name}")

            # Versuche Verbindung zur Named Pipe
            self.pipe_handle = win32file.CreateFile(
                self.pipe_name,
                win32file.GENERIC_WRITE,
                0,  # No sharing
                None,  # Default security
                win32file.OPEN_EXISTING,
                0,  # Default attributes
                None  # No template
            )

            print("✓ Verbunden mit Client!")
            return True

        except pywintypes.error as e:
            print(f"✗ Verbindungsfehler: {e}")
            print("\nHinweis: Der Client muss die Command Pipe Unterstützung kompiliert haben!")
            return False

    def send_command(self, command):
        """
        Sendet einen Befehl an den Client.

        Args:
            command: Der zu sendende Befehl (z.B. "go rid_tos")

        Returns:
            True bei Erfolg, False bei Fehler
        """
        if self.pipe_handle is None:
            print("✗ Nicht verbunden!")
            return False

        try:
            # Füge Newline hinzu
            cmd_bytes = (command + "\n").encode('utf-8')

            # Sende Befehl
            win32file.WriteFile(self.pipe_handle, cmd_bytes)

            print(f"✓ Befehl gesendet: {command}")
            return True

        except pywintypes.error as e:
            print(f"✗ Fehler beim Senden: {e}")
            return False

    def disconnect(self):
        """Trennt die Verbindung"""
        if self.pipe_handle is not None:
            win32file.CloseHandle(self.pipe_handle)
            self.pipe_handle = None
            print("✓ Verbindung getrennt")

    def __enter__(self):
        self.connect()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.disconnect()


def main():
    """Beispiel-Verwendung"""

    print("=== Meridian 59 Client Command Injector ===\n")

    # Beispiel 1: Einzelner Befehl
    print("=== Beispiel 1: Einzelner Befehl ===")
    with M59ClientCommandInjector() as injector:
        if injector.pipe_handle:
            injector.send_command("go rid_tos")
            time.sleep(0.5)

    # Beispiel 2: Mehrere Befehle
    print("\n=== Beispiel 2: Mehrere Befehle ===")
    with M59ClientCommandInjector() as injector:
        if injector.pipe_handle:
            commands = [
                "go rid_tos",
                "dm boost stats",
                "dm get spells",
                "dm get skills"
            ]

            for cmd in commands:
                injector.send_command(cmd)
                time.sleep(0.2)  # Kurze Pause zwischen Befehlen

    # Beispiel 3: Interaktiver Modus
    print("\n=== Beispiel 3: Interaktiver Modus ===")
    with M59ClientCommandInjector() as injector:
        if injector.pipe_handle:
            print("\nGib Befehle ein (oder 'quit' zum Beenden):")

            while True:
                try:
                    cmd = input("> ")
                    if cmd.lower() in ['quit', 'exit', 'q']:
                        break

                    if cmd.strip():
                        injector.send_command(cmd)

                except KeyboardInterrupt:
                    print("\n\n✓ Abgebrochen")
                    break


if __name__ == "__main__":
    try:
        # Prüfe ob pywin32 und psutil installiert sind
        import win32pipe
        import psutil
        main()
    except ImportError as e:
        print(f"✗ Fehlende Abhängigkeit: {e}")
        print("\nInstalliere die erforderlichen Pakete mit:")
        print("  pip install pywin32 psutil")
        sys.exit(1)
