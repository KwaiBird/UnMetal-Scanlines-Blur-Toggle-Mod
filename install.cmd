@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0manage.ps1" install "%~dp0."
if errorlevel 1 (pause & exit /b 1)
echo Mod installed.
pause
