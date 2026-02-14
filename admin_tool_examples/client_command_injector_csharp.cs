using System;
using System.IO;
using System.IO.Pipes;
using System.Diagnostics;
using System.Linq;
using System.Text;

/// <summary>
/// Meridian 59 Client Command Injector - C#
/// Sendet DM-Befehle direkt an den laufenden Client über Named Pipe.
/// Ersetzt unzuverlässige sendkeys-Methoden.
/// </summary>
public class M59ClientCommandInjector : IDisposable
{
    private int? _pid;
    private NamedPipeClientStream _pipeClient;
    private string _pipeName;

    public M59ClientCommandInjector(int? pid = null)
    {
        _pid = pid;
    }

    /// <summary>
    /// Findet die Process ID des laufenden Meridian59 Clients
    /// </summary>
    public int? FindClientPid()
    {
        var processes = Process.GetProcesses();
        var clientNames = new[] { "meridian", "clientd3d", "client" };

        foreach (var proc in processes)
        {
            try
            {
                var processName = proc.ProcessName.ToLower();
                if (clientNames.Any(name => processName.Contains(name)))
                {
                    Console.WriteLine($"✓ Client gefunden: PID {proc.Id}");
                    return proc.Id;
                }
            }
            catch
            {
                // Ignoriere Access Denied Fehler
            }
        }

        return null;
    }

    /// <summary>
    /// Verbindet zum Client via Named Pipe
    /// </summary>
    public bool Connect()
    {
        if (_pid == null)
        {
            _pid = FindClientPid();
        }

        if (_pid == null)
        {
            Console.WriteLine("✗ Kein laufender Meridian 59 Client gefunden!");
            return false;
        }

        _pipeName = $"Meridian59_Command_{_pid}";

        try
        {
            Console.WriteLine($"→ Verbinde zu Pipe: \\\\.\\pipe\\{_pipeName}");

            _pipeClient = new NamedPipeClientStream(
                ".",                    // Local server
                _pipeName,              // Pipe name
                PipeDirection.Out       // Write-only
            );

            // Versuche Verbindung (5 Sekunden Timeout)
            _pipeClient.Connect(5000);

            Console.WriteLine("✓ Verbunden mit Client!");
            return true;
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ Verbindungsfehler: {ex.Message}");
            Console.WriteLine("\nHinweis: Der Client muss die Command Pipe Unterstützung kompiliert haben!");
            return false;
        }
    }

    /// <summary>
    /// Sendet einen Befehl an den Client
    /// </summary>
    public bool SendCommand(string command)
    {
        if (_pipeClient == null || !_pipeClient.IsConnected)
        {
            Console.WriteLine("✗ Nicht verbunden!");
            return false;
        }

        try
        {
            // Füge Newline hinzu
            byte[] buffer = Encoding.UTF8.GetBytes(command + "\n");

            // Sende Befehl
            _pipeClient.Write(buffer, 0, buffer.Length);
            _pipeClient.Flush();

            Console.WriteLine($"✓ Befehl gesendet: {command}");
            return true;
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ Fehler beim Senden: {ex.Message}");
            return false;
        }
    }

    /// <summary>
    /// Trennt die Verbindung
    /// </summary>
    public void Disconnect()
    {
        if (_pipeClient != null)
        {
            _pipeClient.Close();
            _pipeClient.Dispose();
            _pipeClient = null;
            Console.WriteLine("✓ Verbindung getrennt");
        }
    }

    public void Dispose()
    {
        Disconnect();
    }
}

/// <summary>
/// Beispiel-Verwendung
/// </summary>
class Program
{
    static void Main(string[] args)
    {
        Console.WriteLine("=== Meridian 59 Client Command Injector ===\n");

        // Beispiel 1: Einzelner Befehl
        Console.WriteLine("=== Beispiel 1: Einzelner Befehl ===");
        using (var injector = new M59ClientCommandInjector())
        {
            if (injector.Connect())
            {
                injector.SendCommand("go rid_tos");
                System.Threading.Thread.Sleep(500);
            }
        }

        // Beispiel 2: Mehrere Befehle
        Console.WriteLine("\n=== Beispiel 2: Mehrere Befehle ===");
        using (var injector = new M59ClientCommandInjector())
        {
            if (injector.Connect())
            {
                string[] commands = {
                    "go rid_tos",
                    "dm boost stats",
                    "dm get spells",
                    "dm get skills"
                };

                foreach (var cmd in commands)
                {
                    injector.SendCommand(cmd);
                    System.Threading.Thread.Sleep(200);
                }
            }
        }

        // Beispiel 3: Interaktiver Modus
        Console.WriteLine("\n=== Beispiel 3: Interaktiver Modus ===");
        using (var injector = new M59ClientCommandInjector())
        {
            if (injector.Connect())
            {
                Console.WriteLine("\nGib Befehle ein (oder 'quit' zum Beenden):");

                while (true)
                {
                    Console.Write("> ");
                    string cmd = Console.ReadLine();

                    if (string.IsNullOrWhiteSpace(cmd))
                        continue;

                    if (cmd.ToLower() == "quit" || cmd.ToLower() == "exit" || cmd.ToLower() == "q")
                        break;

                    injector.SendCommand(cmd);
                }
            }
        }

        Console.WriteLine("\nDrücke eine Taste zum Beenden...");
        Console.ReadKey();
    }
}
