@echo off
REM ================================================================================
REM  Kompletter Build + Deploy Workflow fuer Server 113
REM ================================================================================

cd /d "C:\Users\Rod\Desktop\2\Server-104-main\qbscripts"

echo.
echo ================================================================================
echo   KOMPLETTER BUILD + DEPLOY WORKFLOW - SERVER 113
echo ================================================================================
echo.
echo Dieser Prozess wird:
echo   1. Server bauen (kompilieren)
echo   2. Server stoppen
echo   3. Dateien kopieren
echo   4. Server starten
echo.
pause

echo.
echo [1/4] Baue Server...
echo ================================================================================
powershell -ExecutionPolicy Bypass -File ".\build-113.ps1" -Action build -Server production

if errorlevel 1 (
    echo.
    echo FEHLER beim Build!
    pause
    exit /b 1
)

echo.
echo [2/4] Stoppe Server...
echo ================================================================================
powershell -ExecutionPolicy Bypass -File ".\build-113.ps1" -Action killserver -Server production

timeout /t 5 /nobreak >nul

echo.
echo [3/4] Kopiere Server-Dateien...
echo ================================================================================
powershell -ExecutionPolicy Bypass -File ".\build-113.ps1" -Action copyserver -Server production

echo.
echo [4/4] Starte Server...
echo ================================================================================
powershell -ExecutionPolicy Bypass -File ".\build-113.ps1" -Action startserver -Server production

echo.
echo ================================================================================
echo   FERTIG! Server 113 laeuft mit neuem Code!
echo ================================================================================
echo.
pause
