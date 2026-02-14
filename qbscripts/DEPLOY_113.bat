@echo off
REM ================================================================================
REM  Deploy Server 113 (Production)
REM ================================================================================

cd /d "C:\Users\Rod\Desktop\2\Server-104-main\qbscripts"

echo.
echo ================================================================================
echo   Server 113 Deployment
echo ================================================================================
echo.
echo 1. Stoppe Server...
powershell -ExecutionPolicy Bypass -File ".\build-113.ps1" -Action killserver -Server production

timeout /t 3 /nobreak >nul

echo.
echo 2. Kopiere Server-Dateien...
powershell -ExecutionPolicy Bypass -File ".\build-113.ps1" -Action copyserver -Server production

echo.
echo 3. Starte Server...
powershell -ExecutionPolicy Bypass -File ".\build-113.ps1" -Action startserver -Server production

echo.
echo Deployment abgeschlossen!
echo.
pause
