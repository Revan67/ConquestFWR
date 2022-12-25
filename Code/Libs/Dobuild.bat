set MSDEV_VC98_PATH=c:\PROGRA~1\MICROS~3\VC98
set MSDEV_BIN_PATH=c:\PROGRA~1\MICROS~3\Common\MSDev98\Bin
set VSS_BIN_PATH=c:\PROGRA~1\MICROS~3\Common\vss\win32
set SSUSER=tbratton
set SSDIR=\\nt\libs\tools\libvss
set ROOT_PATH=\libs\dev
set ROOT_DRIVE=c:
set BUILD_ROOT=%ROOT_DRIVE%%ROOT_PATH%
set BUILD_PUBLISH_PATH=\\nt\dev\projects\libs\latest
set LOG_PATH=c:\
%ROOT_DRIVE%
md %ROOT_PATH%
cd %ROOT_PATH%
set INCLUDE=c:\libs\dev\include;c:\mssdk\include;c:\dxmedia\include;%MSDEV_VC98_PATH%\Include
set LIB=c:\libs\dev\static;c:\mssdk\lib;c:\dxmedia\lib;%MSDEV_VC98_PATH%\Lib
set path=%MSDEV_BIN_PATH%;%path%
REM "c:\Program Files\Microsoft Visual Studio\Common\vss\win32\ss.exe" Get -GF $/libs/dev/build.pl
@echo Retrieving perl build file > %LOG_PATH%\BUILD.LOG
%VSS_BIN_PATH%\ss.exe Get -GF $/libs/dev/build.pl >> %LOG_PATH%\BUILD.LOG
@echo Executing perl build file
REM perl build.pl %1 %2 %3 %4 %5 %6 >> %LOG_PATH%\BUILD.LOG
perl build.pl %1 %2 %3 %4 %5 %6 >> %LOG_PATH%\BUILD.LOG
REM "c:\Program Files\Microsoft Visual Studio\Common\vss\win32\ss.exe" Get -GF $/libs/dev/sendmail.pl
@echo Retrieving sendmail script. >> %LOG_PATH%\BUILD.LOG
%VSS_BIN_PATH%\ss.exe Get -GF $/libs/dev/sendmail.pl >> %LOG_PATH%\BUILD.LOG
@echo Retrieving distribution scripts. >> %LOG_PATH%\BUILD.LOG
%VSS_BIN_PATH%\ss.exe Get -GF $/libs/dev/distrib.pl >> %LOG_PATH%\BUILD.LOG
%VSS_BIN_PATH%\ss.exe Get -GF $/libs/dev/distrib.lst >> %LOG_PATH%\BUILD.LOG
@echo Distributing the build
perl distrib.pl distrib.lst >> %LOG_PATH%\BUILD.LOG
@echo Mailing results
perl sendmail.pl tbratton@digitalanvil.com tbratton@digitalanvil.com "Build results" < %LOG_PATH%\BUILD.LOG
