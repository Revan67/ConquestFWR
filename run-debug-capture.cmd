@echo off
setlocal

set "game_dir=D:\Games\GOG\Conquest Frontier Wars"
set "dump_dir=D:\Dev\CFW\CrashDumps"
set "procdump=D:\Dev\CFW\Tools\ProcDump\procdump.exe"

if not exist "%game_dir%\Conquest.exe" (
    echo Conquest.exe was not found in "%game_dir%".
    exit /b 1
)

if not exist "%procdump%" (
    echo ProcDump was not found in "D:\Dev\CFW\Tools\ProcDump".
    exit /b 1
)

if not exist "%dump_dir%" mkdir "%dump_dir%"

pushd "%game_dir%" || exit /b 1
"%procdump%" -accepteula -ma -e -x "%dump_dir%" "%game_dir%\Conquest.exe"
set "exit_code=%errorlevel%"
popd

echo.
echo Crash capture ended. Dumps, if any, are in "%dump_dir%".
pause
exit /b %exit_code%
