@echo off

if '%1' == '-d' goto dbg
if '%1' == '' goto usage
darthrtnobs.exe %1
goto done
:dbg
if '%2' == '' goto usage
if '%3' == '' goto nologfile
dbgtee darthrtnobs.exe %3 > %2
goto done
:nologfile
dbgtee darthrtnobs.exe %2
goto done


:usage
echo dwin9x [-d [logfile]] script_file
echo   Look in the .\Scripts\ directory for script files

:done
