# Microsoft Developer Studio Generated NMAKE File, Based on EventMgr.dsp
!IF "$(CFG)" == ""
CFG=EventMgr - Win32 Debug
!MESSAGE No configuration specified. Defaulting to EventMgr - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "EventMgr - Win32 Release" && "$(CFG)" !=\
 "EventMgr - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "EventMgr.mak" CFG="EventMgr - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "EventMgr - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "EventMgr - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "EventMgr - Win32 Release"

OUTDIR=.
INTDIR=.
# Begin Custom Macros
OutDir=.\.\ 
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\EventMgr.dll"

!ELSE 

ALL : "$(OUTDIR)\EventMgr.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\Eventmgr.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\EventMgr.dll"
	-@erase "$(OUTDIR)\EventMgr.exp"
	-@erase "$(OUTDIR)\EventMgr.lib"
	-@erase "$(OUTDIR)\EventMgr.map"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /G5 /ML /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)\EventMgr.pch" /YX"windows.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"\
 /FD /c 
CPP_OBJS=.\ 
CPP_SBRS=.
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o NUL /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\EventMgr.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=DACOM.lib COMHeap.lib kernel32.lib user32.lib gdi32.lib\
 winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib\
 uuid.lib /nologo /entry:"DllMain" /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\EventMgr.pdb" /map:"$(INTDIR)\EventMgr.map" /machine:I386\
 /out:"$(OUTDIR)\EventMgr.dll" /implib:"$(OUTDIR)\EventMgr.lib" 
LINK32_OBJS= \
	"$(INTDIR)\Eventmgr.obj"

"$(OUTDIR)\EventMgr.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "EventMgr - Win32 Debug"

OUTDIR=.
INTDIR=.
# Begin Custom Macros
OutDir=.\.\ 
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\EventMgr.dll"

!ELSE 

ALL : "$(OUTDIR)\EventMgr.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\Eventmgr.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\EventMgr.dll"
	-@erase "$(OUTDIR)\EventMgr.exp"
	-@erase "$(OUTDIR)\EventMgr.lib"
	-@erase "$(OUTDIR)\EventMgr.map"
	-@erase "$(OUTDIR)\EventMgr.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /G5 /ML /W3 /Zi /Od /Gf /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)\EventMgr.pch" /YX"windows.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"\
 /FD /c 
CPP_OBJS=.\ 
CPP_SBRS=.
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o NUL /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\EventMgr.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=DACOM.lib COMHeap.lib kernel32.lib user32.lib gdi32.lib\
 winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib\
 uuid.lib /nologo /entry:"DllMain" /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\EventMgr.pdb" /map:"$(INTDIR)\EventMgr.map" /debug\
 /machine:I386 /out:"$(OUTDIR)\EventMgr.dll" /implib:"$(OUTDIR)\EventMgr.lib"\
 /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\Eventmgr.obj"

"$(OUTDIR)\EventMgr.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

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


!IF "$(CFG)" == "EventMgr - Win32 Release" || "$(CFG)" ==\
 "EventMgr - Win32 Debug"
SOURCE=.\Eventmgr.cpp
DEP_CPP_EVENT=\
	".\dacom.h"\
	".\eventmgr.h"\
	".\HeapObj.h"\
	".\results.h"\
	".\stddat.h"\
	".\System.h"\
	".\TComponent.h"\
	".\typedefs.h"\
	

"$(INTDIR)\Eventmgr.obj" : $(SOURCE) $(DEP_CPP_EVENT) "$(INTDIR)"



!ENDIF 

