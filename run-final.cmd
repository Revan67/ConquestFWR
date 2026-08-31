@echo off
setlocal

set "game_dir=D:\Games\GOG-Final\Conquest Frontier Wars"
if not exist "%game_dir%\Conquest.exe" (
    echo Conquest.exe was not found in "%game_dir%".
    exit /b 1
)

pushd "%game_dir%" || exit /b 1
Conquest.exe
set "exit_code=%errorlevel%"
popd
exit /b %exit_code%
