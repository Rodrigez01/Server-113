@echo off
REM Build script with Windows SDK 10.0.10240.0 for VS2015 compatibility
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x86 10.0.10240.0
cd /d "C:\Users\Rod\Desktop\2\Server-104-main.mitallenaenderungen.nicht.loeschen"
nmake release=1
pause
