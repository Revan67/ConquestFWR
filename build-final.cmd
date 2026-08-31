@echo off
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Build-Conquest.ps1" -Configuration Final %*
exit /b %ERRORLEVEL%

