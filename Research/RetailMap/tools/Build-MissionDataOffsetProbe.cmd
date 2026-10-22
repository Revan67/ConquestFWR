@echo off
setlocal

call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars32.bat"
if errorlevel 1 exit /b 2

set "ROOT=%~dp0..\..\.."
cl /nologo /c /EHsc /MT /DWIN32 /DNDEBUG /D_WINDOWS ^
  /I"C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Include" ^
  /I"%ROOT%\Code\App\Src" ^
  /I"%ROOT%\Code\App\Src\include" ^
  /I"%ROOT%\Code\App\DInclude" ^
  /I"%ROOT%\Code\Libs\Include" ^
  "%~dp0VerifyMissionDataOffsets.cpp" ^
  /Fo"%TEMP%\CFW-MissionDataOffsetProbe.obj"

exit /b %errorlevel%
