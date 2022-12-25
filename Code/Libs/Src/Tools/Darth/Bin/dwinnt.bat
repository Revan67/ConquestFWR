@echo off

if '%1' == '-d' goto dbg
if '%1' == '' goto usage
darthrt.exe %1
goto done
:dbg
if '%2' == '' goto usage
if '%3' == '' goto nologfile
dbgtee darthrt.exe %3 > %2
goto done
:nologfile
dbgtee darthrt.exe %2
goto done


:usage
echo dwinnt [-d [logfile]] script_file
echo   Look in the .\Scripts\ directory for script files

:done
