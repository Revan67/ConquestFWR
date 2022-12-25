# Microsoft Developer Studio Generated NMAKE File, Based on audio.dsp
!IF "$(CFG)" == ""
CFG=audio - Win32 Debug
!MESSAGE No configuration specified. Defaulting to audio - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "audio - Win32 Release" && "$(CFG)" != "audio - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "audio.mak" CFG="audio - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "audio - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "audio - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "audio - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\audio.dll"

!ELSE 

ALL : "$(OUTDIR)\audio.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\audiomgr.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\Waudmgrsfil.obj"
	-@erase "$(OUTDIR)\audio.dll"
	-@erase "$(OUTDIR)\audio.exp"
	-@erase "$(OUTDIR)\audio.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)\audio.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Release/
CPP_SBRS=.

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o NUL /win32 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\audio.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\audio.pdb" /machine:I386 /out:"$(OUTDIR)\audio.dll"\
 /implib:"$(OUTDIR)\audio.lib" 
LINK32_OBJS= \
	"$(INTDIR)\audiomgr.obj" \
	"$(INTDIR)\Waudmgrsfil.obj"

"$(OUTDIR)\audio.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "audio - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug

!IF "$(RECURSE)" == "0" 

ALL : "..\..\explicitdll\audio.dll"

!ELSE 

ALL : "..\..\explicitdll\audio.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\audiomgr.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(INTDIR)\Waudmgrsfil.obj"
	-@erase "$(OUTDIR)\audio.exp"
	-@erase "$(OUTDIR)\audio.lib"
	-@erase "$(OUTDIR)\audio.pdb"
	-@erase "..\..\explicitdll\audio.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /G6 /ML /W3 /Gm /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)\audio.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o NUL /win32 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\audio.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=dacom.lib comheap.lib kernel32.lib user32.lib gdi32.lib\
 winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib\
 uuid.lib odbc32.lib odbccp32.lib shell32.lib uuid.lib mss32.lib /nologo\
 /entry:"DllMain" /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\audio.pdb" /debug /machine:I386\
 /out:"\libs\explicitdll\audio.dll" /implib:"$(OUTDIR)\audio.lib" /pdbtype:sept\
 /libpath:"c:\libs\static" 
LINK32_OBJS= \
	"$(INTDIR)\audiomgr.obj" \
	"$(INTDIR)\Waudmgrsfil.obj"

"..\..\explicitdll\audio.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "audio - Win32 Release" || "$(CFG)" == "audio - Win32 Debug"
SOURCE=.\audiomgr.CPP

!IF  "$(CFG)" == "audio - Win32 Release"

DEP_CPP_AUDIO=\
	".\audiomgr.h"\
	".\dacom.h"\
	".\davariant.h"\
	".\filesys.h"\
	".\heapobj.h"\
	".\mssw.h"\
	".\results.h"\
	".\stddat.h"\
	".\system.h"\
	".\tcomponent.h"\
	".\tsmartpointer.h"\
	".\typedefs.h"\
	".\Waudmgrsfil.h"\
	

"$(INTDIR)\audiomgr.obj" : $(SOURCE) $(DEP_CPP_AUDIO) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "audio - Win32 Debug"

DEP_CPP_AUDIO=\
	".\audiomgr.h"\
	".\dacom.h"\
	".\davariant.h"\
	".\filesys.h"\
	".\heapobj.h"\
	".\mssw.h"\
	".\results.h"\
	".\stddat.h"\
	".\system.h"\
	".\tcomponent.h"\
	".\tsmartpointer.h"\
	".\typedefs.h"\
	".\Waudmgrsfil.h"\
	

"$(INTDIR)\audiomgr.obj" : $(SOURCE) $(DEP_CPP_AUDIO) "$(INTDIR)"


!ENDIF 

SOURCE=.\Waudmgrsfil.cpp
DEP_CPP_WAUDM=\
	".\mssw.h"\
	".\Waudmgrsfil.h"\
	

"$(INTDIR)\Waudmgrsfil.obj" : $(SOURCE) $(DEP_CPP_WAUDM) "$(INTDIR)"



!ENDIF 

