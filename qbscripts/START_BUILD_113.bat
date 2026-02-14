@echo off
REM ================================================================================
REM  Schnellstart-Script fuer Server 113 Build
REM ================================================================================

echo.
echo ================================================================================
echo   Meridian 59 Server 113 Build Script
echo ================================================================================
echo.

REM Wechsle zum korrekten Verzeichnis
cd /d "C:\Users\Rod\Desktop\2\Server-104-main\qbscripts"

echo Aktuelles Verzeichnis: %CD%
echo.

REM Entsperre das PowerShell Script
powershell -Command "Unblock-File '.\build-113.ps1'"

echo Script entsperrt!
echo.
echo Starte Build fuer Server 113 (Production)...
echo.

REM Fuehre das PowerShell Script aus
powershell -ExecutionPolicy Bypass -File ".\build-113.ps1" -Action build -Server production

echo.
echo Build abgeschlossen!
echo.
pause
