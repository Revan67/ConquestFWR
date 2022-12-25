========================================================================
       Application - DbgRemote
========================================================================
This is the local portion of the HAP Remote Debugger - this program provides
an editor and source display (courtesy of the excellent scintilla control)




DbgRemote.dsp
    This file (the project file) contains information at the project level and
    is used to build a single project or subproject. Other users can share the
    project (.dsp) file, but they should export the makefiles locally.

DbgRemote.h
    This is the main header file for the application.  It includes other
    project specific headers (including Resource.h) and declares the
    CDbgRemoteApp application class.

DbgRemote.cpp
    This is the main application source file that contains the application
    class CDbgRemoteApp.

DbgRemote.rc
    This is a listing of all of the Microsoft Windows resources that the
    program uses.  It includes the icons, bitmaps, and cursors that are stored
    in the RES subdirectory.  This file can be directly edited in Microsoft
	Visual C++.

DbgRemote.clw
    This file contains information used by ClassWizard to edit existing
    classes or add new classes.  ClassWizard also uses this file to store
    information needed to create and edit message maps and dialog data
    maps and to create prototype member functions.

res\DbgRemote.ico
    This is an icon file, which is used as the application's icon.  This
    icon is included by the main resource file DbgRemote.rc.

res\DbgRemote.rc2
    This file contains resources that are not edited by Microsoft 
	Visual C++.  You should place all resources not editable by
	the resource editor in this file.



/////////////////////////////////////////////////////////////////////////////

For the main frame window:

MainFrm.h, MainFrm.cpp
    These files contain the frame class CMainFrame, which is derived from
    CMDIFrameWnd and controls all MDI frame features.

res\Toolbar.bmp
    This bitmap file is used to create tiled images for the toolbar.
    The initial toolbar and status bar are constructed in the CMainFrame
    class. Edit this toolbar bitmap using the resource editor, and
    update the IDR_MAINFRAME TOOLBAR array in DbgRemote.rc to add
    toolbar buttons.
/////////////////////////////////////////////////////////////////////////////

For the child frame window:

ChildFrm.h, ChildFrm.cpp
    These files define and implement the CChildFrame class, which
    supports the child windows in an MDI application.

/////////////////////////////////////////////////////////////////////////////

AppWizard creates one document type and one view:

DbgRemoteDoc.h, DbgRemoteDoc.cpp - the document
    These files contain your CDbgRemoteDoc class.  Edit these files to
    add your special document data and to implement file saving and loading
    (via CDbgRemoteDoc::Serialize).

DbgRemoteView.h, DbgRemoteView.cpp - the view of the document
    These files contain your CDbgRemoteView class.
    CDbgRemoteView objects are used to view CDbgRemoteDoc objects.

res\DbgRemoteDoc.ico
    This is an icon file, which is used as the icon for MDI child windows
    for the CDbgRemoteDoc class.  This icon is included by the main
    resource file DbgRemote.rc.


/////////////////////////////////////////////////////////////////////////////
Other standard files:

StdAfx.h, StdAfx.cpp
    These files are used to build a precompiled header (PCH) file
    named DbgRemote.pch and a precompiled types file named StdAfx.obj.

Resource.h
    This is the standard header file, which defines new resource IDs.
    Microsoft Visual C++ reads and updates this file.

/////////////////////////////////////////////////////////////////////////////
Other notes:



/////////////////////////////////////////////////////////////////////////////
