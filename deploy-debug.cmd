@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Deploy-Conquest.ps1" -Configuration Debug %*
