using System;
using System.Net.Sockets;
using System.Text;
using System.Threading;

/// <summary>
/// Meridian 59 Admin Tool - C# Client
/// Verbindet sich zum Maintenance Port des Servers und sendet Admin-Befehle.
/// Ersetzt unzuverlässige sendkeys-Methoden.
/// </summary>
public class M59AdminClient : IDisposable
{
    private readonly string _host;
    private readonly int _port;
    private TcpClient _client;
    private NetworkStream _stream;

    public M59AdminClient(string host = "127.0.0.1", int port = 9998)
    {
        _host = host;
        _port = port;
    }

    /// <summary>
    /// Verbindet zum Maintenance Port
    /// </summary>
    public bool Connect()
    {
        try
        {
            _client = new TcpClient();
            _client.Connect(_host, _port);
            _stream = _client.GetStream();
            _stream.ReadTimeout = 10000; // 10 Sekunden
            _stream.WriteTimeout = 10000;

            Console.WriteLine($"✓ Verbunden mit {_host}:{_port}");

            // Optional: Begrüßungsnachricht empfangen
            Thread.Sleep(100);
            string response = Receive();
            if (!string.IsNullOrEmpty(response))
            {
                Console.WriteLine($"Server: {response}");
            }

            return true;
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ Verbindungsfehler: {ex.Message}");
            return false;
        }
    }

    /// <summary>
    /// Sendet einen Admin-Befehl zum Server
    /// </summary>
    public string SendCommand(string command)
    {
        if (_stream == null || !_stream.CanWrite)
        {
            Console.WriteLine("✗ Nicht verbunden!");
            return null;
        }

        try
        {
            // Befehl mit Carriage Return abschließen
            byte[] data = Encoding.UTF8.GetBytes(command + "\r\n");
            _stream.Write(data, 0, data.Length);
            _stream.Flush();

            Console.WriteLine($"→ Gesendet: {command}");

            // Warte auf Antwort
            Thread.Sleep(100);
            string response = Receive();

            return response;
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ Fehler beim Senden: {ex.Message}");
            return null;
        }
    }

    /// <summary>
    /// Empfängt Antwort vom Server
    /// </summary>
    private string Receive(int bufferSize = 4096)
    {
        if (_stream == null || !_stream.CanRead)
            return null;

        try
        {
            if (_stream.DataAvailable)
            {
                byte[] buffer = new byte[bufferSize];
                int bytesRead = _stream.Read(buffer, 0, buffer.Length);

                if (bytesRead > 0)
                {
                    string response = Encoding.UTF8.GetString(buffer, 0, bytesRead);
                    return response.Trim();
                }
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"✗ Fehler beim Empfangen: {ex.Message}");
        }

        return null;
    }

    /// <summary>
    /// Trennt die Verbindung
    /// </summary>
    public void Disconnect()
    {
        try
        {
            if (_stream != null)
            {
                SendCommand("QUIT");
                _stream.Close();
                _stream = null;
            }

            if (_client != null)
            {
                _client.Close();
                _client = null;
            }

            Console.WriteLine("✓ Verbindung getrennt");
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Fehler beim Trennen: {ex.Message}");
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
        // Beispiel 1: Einfacher Befehl
        Console.WriteLine("=== Beispiel 1: Einzelner Befehl ===");
        using (var client = new M59AdminClient())
        {
            if (client.Connect())
            {
                string response = client.SendCommand("show status");
                if (!string.IsNullOrEmpty(response))
                {
                    Console.WriteLine($"← Antwort:\n{response}\n");
                }
            }
        }

        // Beispiel 2: Mehrere Befehle
        Console.WriteLine("\n=== Beispiel 2: Mehrere Befehle ===");
        using (var client = new M59AdminClient())
        {
            if (client.Connect())
            {
                string[] commands = {
                    "show status",
                    "show sessions",
                    "show memory"
                };

                foreach (var cmd in commands)
                {
                    string response = client.SendCommand(cmd);
                    if (!string.IsNullOrEmpty(response))
                    {
                        Console.WriteLine($"← Antwort: {response}\n");
                    }
                    Thread.Sleep(500);
                }
            }
        }

        // Beispiel 3: Account erstellen
        Console.WriteLine("\n=== Beispiel 3: Account erstellen ===");
        using (var client = new M59AdminClient())
        {
            if (client.Connect())
            {
                // Account erstellen
                string response = client.SendCommand("create automated testuser testpass123");
                if (!string.IsNullOrEmpty(response))
                {
                    Console.WriteLine($"← Antwort: {response}");
                }

                Thread.Sleep(500);

                // Status prüfen
                response = client.SendCommand("show account testuser");
                if (!string.IsNullOrEmpty(response))
                {
                    Console.WriteLine($"← Antwort: {response}");
                }
            }
        }

        Console.WriteLine("\nDrücke eine Taste zum Beenden...");
        Console.ReadKey();
    }
}
