@echo off
REM Batch file for publishing the libraries.
REM The basic steps for publishing are:
REM 1) Lock out the file
REM 2) Copy the new file
REM 3) Check in the new file
REM These steps need to be repeated for each file that is being published.
REM This program parses a file which contains the names of the files to
REM publish and where they should go.

if '%1'=='' goto SHOW_USAGE
if '%2'=='' goto SHOW_USAGE
if '%3'=='' goto SHOW_USAGE

rem set implicitDest=ImplicitDLL
rem set explicitDest=ExplicitDLL
rem set staticDest=Static

set explicitDest=%1
set implicitDest=%2
set staticDest=%3

REM For each directory under this one, if it ends in "Release", check for
REM .DLL and .LIB files. If only a .DLL file is found, copy it to Explicit.
REM All .LIB files are copied to Static. DLLs with the same name as a .LIB
REM file are copied into Implicit.
echo `================= START PUBLISH =================`
global /q gosub PER_DIR
echo `=================  END PUBLISH  =================`
QUIT

REM Usage
:SHOW_USAGE
echo `Usage: publish <explicit dir> <implicit dir> <static dir>`
QUIT

REM Per directory routine
:PER_DIR
if %@WORD["\",-0,%_CWD]==Release gosub ON_REL_DIR
RETURN

REM Release dir processing
:ON_REL_DIR
set dlldest=%implicitDest
for %libfile in (*.lib) do gosub LIB_CHECK
set dlldest=%explicitDest
for %dllfile in (*.dll) do gosub DLL_CHECK
RETURN

REM Lib checking routine
:LIB_CHECK
set dllfile=%@WORD[".",0,%libfile].dll
if NOT exist %dllfile set %dllfile=
gosub DO_PUBLISH
RETURN

REM DLL checking routine
REM WARNING!!! Must be called after LIB_CHECK
:DLL_CHECK
set libfile=%@WORD[".",0,%dllfile].lib
if NOT exist %libfile (
    set %libfile=
    GOSUB DO_PUBLISH
)
RETURN

REM Publishing routine
:DO_PUBLISH
REM If %libfile exists, copy it to staticDest
rem if exist %libfile echo copy %_CWD\%libfile to %staticDest
rem if exist %dllfile echo copy %_CWD\%dllfile to %dllDest
if exist %libfile (
    echo %_CWD\%libfile `=>` %staticDest
    copy %_CWD\%libfile %staticDest > NUL
)
if exist %dllfile (
    copy %_CWD\%dllfile %dllDest > NUL
    echo %_CWD\%dllfile `=>` %dllDest
)
RETURN

