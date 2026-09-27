@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0manage.ps1" uninstall "%~dp0."
if errorlevel 1 (pause & exit /b 1)
echo Original SDL2 DLL restored.
pause
