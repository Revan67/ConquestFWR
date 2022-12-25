//
// setupdoc.cpp
//
//      A spiffy way to parse the input file.
//
// History:
//
//       2/02/95    KenSh       Created
//       8/10/95    a-DenSo     Added InstallFont command
//		03/15/97 update timestamp
//

#include "prepstub.h"
#include "textdoc.h"
#include "setupdoc.h"
#include "command.h"
#include "cmpstate.h"
#include "util.h"
#include "resource.h"
#include "stdio.h"
#include "diskinfo.h"
#include <stdlib.h>
#include <search.h>

#define ALLOC_COMMANDS          64      //how many commands at a time to allocate
extern g_bDiskPaths;
extern int g_nCurrentDiskID;
//Forward declare
BOOL DetermineDiskToken(ETOKEN eToken, CCommand* pCommand);

static EXPECTED SetupExpected[] =
{

//default error action is actPop with an error message.
//newline, whitespace, and commas are ignored if not specified.
	{ jSetupDoc,		nStart,			TOK_COMMENT,			actPush,		jComment,		nStart		},
	{ jSetupDoc,		nStart,			TOK_INSTALL,			actPush,		jInstall,		nStart		},
	{ jSetupDoc,		nStart,			TOK_INIVALUE,			actPush,		jIniValue,		nStart		},
	{ jSetupDoc,		nStart,			TOK_INSTFONT,			actPush,		jInstFont,		nStart		},
	{ jSetupDoc,		nStart,			TOK_MKDIR,				actPush,		jMkDir,			nStart		},
	{ jSetupDoc,		nStart,			TOK_MKROOT,				actPush,		jMkRoot,		nStart		},
	{ jSetupDoc,		nStart,			TOK_GETNAME,			actPush,		jGetName,		nStart		},
	{ jSetupDoc,		nStart,			TOK_GETPID,				actPush,		jGetPID,		nStart		},
	{ jSetupDoc,		nStart,			TOK_INSTDX,				actPush,		jInstDX,		nStart		},
	{ jSetupDoc,		nStart,			TOK_INSTDPLAY,			actPush,		jInstDPLAY,		nStart		},
	{ jSetupDoc,		nStart,			TOK_INSTICON,			actPush,		jInstIcon,		nStart		},
	{ jSetupDoc,		nStart,			TOK_CDSPEED,			actPush,		jCDSpeed,		nStart		},
	{ jSetupDoc,		nStart,			TOK_INSTALLLIST,		actPush,		jInstallList,	nStart		},
	{ jSetupDoc,		nStart,			TOK_INSTALLGO,			actPush,		jInstallGo,		nStart		},
	{ jSetupDoc,		nStart,			TOK_CABGO,				actPush,		jCabGo,			nStart		},
	{ jSetupDoc,		nStart,			TOK_REGWIZ,				actPush,		jRegWiz,		nStart		},
	{ jSetupDoc,		nStart,			TOK_SHELLEXECUTE,		actPush,		jShellExecute,	nStart		},
	{ jSetupDoc,		nStart,			TOK_DELETEFILE,			actPush,		jDeleteFile,	nStart		},
	{ jSetupDoc,		nStart,			TOK_GETGROUP,			actPush,		jGetGroup,		nStart		},
	{ jSetupDoc,		nStart,			TOK_READFILELIST,		actPush,		jReadFileList,	nStart		},
	{ jSetupDoc,		nStart,			TOK_COMMENT,			actPush,		jComment,		nStart		},
	{ jSetupDoc,		nStart,			TOK_BEGINFILELIST,		actPush,		jBeginFileList,	nStart		},
	{ jSetupDoc,		nStart,			TOK_ENDFILELIST,		actPush,		jEndFileList,	nStart		},
	{ jSetupDoc,		nStart,			TOK_PROPERTY,			actPush,		jProperty,		nStart		},
	{ jSetupDoc,		nStart,			TOK_RULE,				actPush,		jRule,			nStart		},
	{ jSetupDoc,		nStart,			TOK_STRINGVAR,			actPush,		jString,		nStart		},
	{ jSetupDoc,		nStart,			TOK_BEGINSTRINGLIST,	actPush,		jBeginStringList,	nStart		},
	{ jSetupDoc,		nStart,			TOK_ENDSTRINGLIST,		actPush,		jEndStringList,		nStart		},
	{ jSetupDoc,		nStart,			TOK_BEGINSTATICSTRINGLIST,	actPush,	jBeginStaticStringList,	nStart	},
	{ jSetupDoc,		nStart,			TOK_ENDSTATICSTRINGLIST,	actPush,	jEndStaticStringList,	nStart	},
	{ jSetupDoc,		nStart,			TOK_ACTION,				actPush,		jAction,		nStart,		},

	{ jAction,			nStart,			TOK_STRING,				actJump,		jAction,		nActionCommand	},
	{ jAction,			nActionCommand,	TOK_STRING,				actJump,		jAction,		nActionParam1	},
	{ jAction,			nActionCommand,	TOK_RECURSE,			actPush,		jAction,		nRecurse		},
	{ jAction,			nActionCommand, TOK_PRECOPY,			actPush,		jAction,		nPrecopy,		},
	{ jAction,			nActionCommand,	DEFAULT_ACTION,			actPop,			jDontCare,		nDontCare		},
	
	{ jAction,			nActionParam1,	TOK_STRING,				actJump,		jAction,		nActionParam2	},
	{ jAction,			nActionParam1,	DEFAULT_ACTION,			actPop,			jDontCare,		nDontCare		},
	{ jAction,			nActionParam1,	TOK_RECURSE,			actPush,		jAction,		nRecurse		},
	{ jAction,			nActionParam1,	TOK_PRECOPY,			actPush,		jAction,		nPrecopy		},

	{ jAction,			nActionParam2,	TOK_STRING,				actJump,		jAction,		nActionParam3	},
	{ jAction,			nActionParam2,	DEFAULT_ACTION,			actPop,			jDontCare,		nDontCare		},
	{ jAction,			nActionParam2,	TOK_RECURSE,			actPush,		jAction,		nRecurse		},
	{ jAction,			nActionParam2,	TOK_PRECOPY,			actPush,		jAction,		nPrecopy		},

	{ jAction,			nActionParam3,	TOK_STRING,				actJump,		jAction,		nActionParam4	},
	{ jAction,			nActionParam3,	DEFAULT_ACTION,			actPop,			jDontCare,		nDontCare		},
	{ jAction,			nActionParam3,	TOK_RECURSE,			actPush,		jAction,		nRecurse		},
	{ jAction,			nActionParam3,	TOK_PRECOPY,			actPush,		jAction,		nPrecopy		},

	{ jAction,			nActionParam4,	DEFAULT_ACTION,			actPop,			jDontCare,		nDontCare		},
	{ jAction,			nActionParam4,	TOK_RECURSE,			actPush,		jAction,		nRecurse		},
	{ jAction,			nActionParam4,	TOK_PRECOPY,			actPush,		jAction,		nPrecopy		},
	

	{ jAction,			nPrecopy,		DEFAULT_ACTION,			actPop,			jDontCare,		nDontCare	},
	{ jAction,			nRecurse,		DEFAULT_ACTION,			actPop,			jDontCare,		nDontCare	},
	
	{ jSetupDoc,		nOS,			DEFAULT_ACTION,			actPop,			jDontCare,		nDontCare	},
	{ jSetupDoc,		nInstall,		DEFAULT_ACTION,			actPop,			jDontCare,		nDontCare	},

	{ jComment,			nStart,			DEFAULT_ACTION,			actPop,			jDontCare,		nDontCare	},
	
	{ jBeginFileList,	nStart,			DEFAULT_ACTION,			actPop,			jDontCare,		nDontCare	},
	{ jEndFileList,		nStart,			DEFAULT_ACTION,			actPop,			jDontCare,		nDontCare	},

	{ jBeginStringList,	nStart,			DEFAULT_ACTION,			actPop,			jDontCare,		nDontCare	},
	{ jEndStringList,	nStart,			DEFAULT_ACTION,			actPop,			jDontCare,		nDontCare	},

	{ jBeginStaticStringList,	nStart,			DEFAULT_ACTION,			actPop,			jDontCare,		nDontCare	},
	{ jEndStaticStringList,		nStart,			DEFAULT_ACTION,			actPop,			jDontCare,		nDontCare	},

	{ jString,			nStart,			TOK_NUMBER,				actJump,		jString,		nStringID	},
	{ jString,			nStringID,		TOK_STRING,				actJump,		jString,		nStringValue},
	{ jString,			nStringValue,	DEFAULT_ACTION,			actPop,			jDontCare,		nDontCare	},

	{ jProperty,		nStart,			TOK_STRING,				actJump,		jProperty,		nProperty	},
	{ jProperty,		nProperty,		TOK_STRING,				actJump,		jProperty,		nPropertyValue},
	{ jProperty,		nPropertyValue,	DEFAULT_ACTION,			actPop,			jDontCare,		nDontCare	},

	{ jRule,			nStart,			TOK_STRING,				actJump,		jRule,			nAction		},
	{ jRule,			nAction,		TOK_STRING,				actJump,		jRule,			nPattern	},
	{ jRule,			nPattern,		TOK_STRING,				actJump,		jRule,			nGroup		},
	{ jRule,			nGroup,			TOK_WIN95,				actPush,		jSetupDoc,		nOS			},
	{ jRule,			nGroup,			TOK_WIN98,				actPush,		jSetupDoc,		nOS			},
	{ jRule,			nGroup,			TOK_NT40,				actPush,		jSetupDoc,		nOS			},
	{ jRule,			nGroup,			TOK_NT50,				actPush,		jSetupDoc,		nOS			},
	{ jRule,			nGroup,			TOK_ALLWIN,				actPush,		jSetupDoc,		nOS			},
	{ jRule,			nGroup,			TOK_ALLNT,				actPush,		jSetupDoc,		nOS			},
	{ jRule,			nGroup,			TOK_DBCS,				actPush,		jSetupDoc,		nOS			},
	{ jRule,			nGroup,			TOK_OEM,				actPush,		jSetupDoc,		nOS			},
	{ jRule,			nGroup,			TOK_RTL,				actPush,		jSetupDoc,		nOS			},
	{ jRule,			nGroup,			TOK_JPN,				actPush,		jSetupDoc,		nOS			},
	{ jRule,			nGroup,			TOK_GER,				actPush,		jSetupDoc,		nOS			},
	{ jRule,			nGroup,			TOK_FRA,				actPush,		jSetupDoc,		nOS			},
	{ jRule,			nGroup,			TOK_SPA,				actPush,		jSetupDoc,		nOS			},
	{ jRule,			nGroup,			TOK_USA,				actPush,		jSetupDoc,		nOS			},
	{ jRule,			nGroup,			TOK_APP1,				actPush,		jSetupDoc,		nOS			},
	{ jRule,			nGroup,			TOK_APP2,				actPush,		jSetupDoc,		nOS			},
	{ jRule,			nGroup,			TOK_APP3,				actPush,		jSetupDoc,		nOS			},
	{ jRule,			nGroup,			TOK_LOCALIZE,			actPush,		jSetupDoc,		nOS			},
	{ jRule,			nGroup,			TOK_HISTORY,			actPush,		jSetupDoc,		nOS			},
	{ jRule,			nGroup,			TOK_SYSTEM,				actPush,		jSetupDoc,		nInstall	},
	{ jRule,			nGroup,			TOK_CHECKVER,			actPush,		jSetupDoc,		nInstall	},
	{ jRule,			nGroup,			TOK_APP,				actPush,		jSetupDoc,		nInstall	},
	{ jRule,			nGroup,			TOK_WINDOWS,			actPush,		jSetupDoc,		nInstall	},
	{ jRule,			nGroup,			TOK_UNINSTALL,			actPush,		jSetupDoc,		nInstall	},
	{ jRule,			nGroup,			TOK_UNINSTALLALL,		actPush,		jSetupDoc,		nInstall	},
	{ jRule,			nGroup,			TOK_CAB,   				actPush,		jSetupDoc,		nInstall	},
	{ jRule,			nGroup,			TOK_SHARED,				actPush,		jSetupDoc,		nInstall	},
	{ jRule,			nGroup,			TOK_DLLREGISTER,		actPush,		jSetupDoc,		nInstall	},
	{ jRule,			nGroup,			DEFAULT_ACTION,			actPop,			jDontCare,		nDontCare	},

	{ jInstallList,		nStart,			TOK_SYSTEM,				actPush,		jInstallList,	nSysDir		},
	{ jInstallList,		nStart,			TOK_APP,				actPush,		jInstallList,	nAppDir		},
	{ jInstallList,		nStart,			TOK_WINDOWS,			actPush,		jInstallList,	nWinDir		},
	{ jInstallList,		nStart,			DEFAULT_ACTION,			actJump,		jInstallList,	nDestDir	},
	{ jInstallList,		nSysDir,		DEFAULT_ACTION,			actPop,			jDontCare,		nDontCare	},
	{ jInstallList,		nAppDir,		DEFAULT_ACTION,			actPop,			jDontCare,		nDontCare	},
	{ jInstallList,		nWinDir, 		DEFAULT_ACTION,			actPop,			jDontCare,		nDontCare	},

//	{ jInstallList,		nStart,			TOK_SHARED,				actJump,		jInstallList,	nDestDir	},
//	{ jInstallList,		nStart,			TOK_DLLREGISTER,		actJump,		jInstallList,	nDestDir	},
//	{ jInstallList,		nStart,			TOK_FONT,   			actJump,		jInstallList,	nDestDir	},

//	{ jInstallList,		nDestDir,		TOK_CHECKVER,			actPush,		jInstallList,	nSysFile	},
	{ jInstallList,		nDestDir,		TOK_UNINSTALL,			actPush,		jInstallList,	nUninstall	},
	{ jInstallList,		nDestDir,		TOK_CAB,   				actPush,		jInstallList,	nCab		},
	{ jInstallList,		nDestDir,		TOK_SHARED,				actPush,		jInstallList,	nSharedFile	},
	{ jInstallList,		nDestDir,		TOK_DLLREGISTER,		actPush,		jInstallList,	nDLLRegister},
	{ jInstallList,		nDestDir,		TOK_FONT,   			actPush,		jInstallList,	nFont		},
	{ jInstallList,		nDestDir,		TOK_UNINSTALLONLY,   	actPush,		jInstallList,	nUninstOnly	},
	{ jInstallList,		nDestDir,		TOK_STRING,				actJump,		jInstallList,	nDestName	},
//	{ jInstallList,		nSysFile,		DEFAULT_ACTION,			actPop,			jDontCare,		nDontCare	},
	{ jInstallList,		nUninstall,		DEFAULT_ACTION,			actPop,			jDontCare,		nDontCare	},
	{ jInstallList,		nCab, 			DEFAULT_ACTION,			actPop,			jDontCare,		nDontCare	},
	{ jInstallList,		nSharedFile,	DEFAULT_ACTION,			actPop,			jDontCare,		nDontCare	},
	{ jInstallList,		nDLLRegister,	DEFAULT_ACTION,			actPop,			jDontCare,		nDontCare	},
	{ jInstallList,		nFont, 			DEFAULT_ACTION,			actPop,			jDontCare,		nDontCare	},
	{ jInstallList,		nUninstOnly,	DEFAULT_ACTION,			actPop,			jDontCare,		nDontCare	},
	{ jInstallList,		nDestName,		TOK_STRING,				actJump,		jInstallList,	nSrcName	},
	{ jInstallList,		nSrcName,		TOK_DISK_01,			actJump,		jInstallList,	nDiskId		},
	{ jInstallList,		nSrcName,		TOK_DISK_02,			actJump,		jInstallList,	nDiskId		},
	{ jInstallList,		nSrcName,		TOK_DISK_03,			actJump,		jInstallList,	nDiskId		},
	{ jInstallList,		nSrcName,		TOK_DISK_04,			actJump,		jInstallList,	nDiskId		},
	{ jInstallList,		nSrcName,		TOK_DISK_05,			actJump,		jInstallList,	nDiskId		},
	{ jInstallList,		nSrcName,		TOK_DISK_06,			actJump,		jInstallList,	nDiskId		},
	{ jInstallList,		nSrcName,		TOK_DISK_07,			actJump,		jInstallList,	nDiskId		},
	{ jInstallList,		nSrcName,		TOK_DISK_08,			actJump,		jInstallList,	nDiskId		},
	{ jInstallList,		nSrcName,		TOK_STRING,				actJump,		jInstallList,	nGroup		},
	{ jInstallList,		nDiskId,		TOK_STRING,				actJump,		jInstallList,	nGroup		},
	{ jInstallList,		nGroup,			TOK_WIN95,				actPush,		jSetupDoc,		nOS			},
	{ jInstallList,		nGroup,			TOK_WIN98,				actPush,		jSetupDoc,		nOS			},
	{ jInstallList,		nGroup,			TOK_NT40,				actPush,		jSetupDoc,		nOS			},
	{ jInstallList,		nGroup,			TOK_NT50,				actPush,		jSetupDoc,		nOS			},
	{ jInstallList,		nGroup,			TOK_ALLWIN,				actPush,		jSetupDoc,		nOS			},
	{ jInstallList,		nGroup,			TOK_ALLNT,				actPush,		jSetupDoc,		nOS			},
	{ jInstallList,		nGroup,			TOK_DBCS,				actPush,		jSetupDoc,		nOS			},
	{ jInstallList,		nGroup,			TOK_OEM,				actPush,		jSetupDoc,		nOS			},
	{ jInstallList,		nGroup,			TOK_RTL,				actPush,		jSetupDoc,		nOS			},
	{ jInstallList,		nGroup,			TOK_JPN,				actPush,		jSetupDoc,		nOS			},
	{ jInstallList,		nGroup,			TOK_GER,				actPush,		jSetupDoc,		nOS			},
	{ jInstallList,		nGroup,			TOK_FRA,				actPush,		jSetupDoc,		nOS			},
	{ jInstallList,		nGroup,			TOK_SPA,				actPush,		jSetupDoc,		nOS			},
	{ jInstallList,		nGroup,			TOK_USA,				actPush,		jSetupDoc,		nOS			},
	{ jInstallList,		nGroup,			TOK_APP1,				actPush,		jSetupDoc,		nOS			},
	{ jInstallList,		nGroup,			TOK_APP2,				actPush,		jSetupDoc,		nOS			},
	{ jInstallList,		nGroup,			TOK_APP3,				actPush,		jSetupDoc,		nOS			},
	{ jInstallList,		nGroup,			TOK_PRECOPY,			actPush,		jSetupDoc,		nOS			},
	{ jInstallList,		nGroup,			DEFAULT_ACTION,			actPop,			jDontCare,		nDontCare	},

	{ jInstallGo,		nStart,			TOK_WIN95,				actJump,		jInstallGo,		nOS			},
	{ jInstallGo,		nStart,			TOK_WIN98,				actJump,		jInstallGo,		nOS			},
	{ jInstallGo,		nStart,			TOK_NT40,				actJump,		jInstallGo,		nOS			},
	{ jInstallGo,		nStart,			TOK_NT50,				actJump,		jInstallGo,		nOS			},
	{ jInstallGo,		nStart,			TOK_ALLWIN,				actJump,		jInstallGo,		nOS			},
	{ jInstallGo,		nStart,			TOK_ALLNT,				actJump,		jInstallGo,		nOS			},
	{ jInstallGo,		nStart,			TOK_DBCS,				actJump,		jInstallGo,		nOS			},
	{ jInstallGo,		nStart,			TOK_OEM,				actJump,		jInstallGo,		nOS			},
	{ jInstallGo,		nStart,			TOK_RTL,				actJump,		jInstallGo,		nOS			},
	{ jInstallGo,		nStart,			TOK_JPN,				actJump,		jInstallGo,		nOS			},
	{ jInstallGo,		nStart,			TOK_GER,				actJump,		jInstallGo,		nOS			},
	{ jInstallGo,		nStart,			TOK_FRA,				actJump,		jInstallGo,		nOS			},
	{ jInstallGo,		nStart,			TOK_SPA,				actJump,		jInstallGo,		nOS			},
	{ jInstallGo,		nStart,			TOK_USA,				actJump,		jInstallGo,		nOS			},
	{ jInstallGo,		nStart,			TOK_APP1,				actJump,		jInstallGo,		nOS			},
	{ jInstallGo,		nStart,			TOK_APP2,				actJump,		jInstallGo,		nOS			},
	{ jInstallGo,		nStart,			TOK_APP3,				actJump,		jInstallGo,		nOS			},
	{ jInstallGo,		nOS,			TOK_WIN95,				actPush,		jSetupDoc,		nOS			},
	{ jInstallGo,		nOS,			TOK_WIN98,				actPush,		jSetupDoc,		nOS			},
	{ jInstallGo,		nOS,			TOK_NT40,				actPush,		jSetupDoc,		nOS			},
	{ jInstallGo,		nOS,			TOK_NT50,				actPush,		jSetupDoc,		nOS			},
	{ jInstallGo,		nOS,			TOK_ALLWIN,				actPush,		jSetupDoc,		nOS			},
	{ jInstallGo,		nOS,			TOK_ALLNT,				actPush,		jSetupDoc,		nOS			},
	{ jInstallGo,		nOS,			TOK_DBCS,				actPush,		jSetupDoc,		nOS			},
	{ jInstallGo,		nOS,			TOK_OEM,				actPush,		jSetupDoc,		nOS			},
	{ jInstallGo,		nOS,			TOK_RTL,				actPush,		jSetupDoc,		nOS			},
	{ jInstallGo,		nOS,			TOK_JPN,				actPush,		jSetupDoc,		nOS			},
	{ jInstallGo,		nOS,			TOK_GER,				actPush,		jSetupDoc,		nOS			},
	{ jInstallGo,		nOS,			TOK_FRA,				actPush,		jSetupDoc,		nOS			},
	{ jInstallGo,		nOS,			TOK_SPA,				actPush,		jSetupDoc,		nOS			},
	{ jInstallGo,		nOS,			TOK_USA,				actPush,		jSetupDoc,		nOS			},
	{ jInstallGo,		nOS,			TOK_APP1,				actPush,		jSetupDoc,		nOS			},
	{ jInstallGo,		nOS,			TOK_APP2,				actPush,		jSetupDoc,		nOS			},
	{ jInstallGo,		nOS,			TOK_APP3,				actPush,		jSetupDoc,		nOS			},
	{ jInstallGo,		nOS,			DEFAULT_ACTION,			actPop,			jDontCare,		nDontCare	},

	{ jCabGo,			nStart,			TOK_STRING,				actJump,		jCabGo,			nCabGo		},
	{ jCabGo,			nCabGo,			TOK_WIN95,				actPush,		jSetupDoc,		nOS			},
	{ jCabGo,			nCabGo,			TOK_WIN98,				actPush,		jSetupDoc,		nOS			},
	{ jCabGo,			nCabGo,			TOK_NT40,				actPush,		jSetupDoc,		nOS			},
	{ jCabGo,			nCabGo,			TOK_NT50,				actPush,		jSetupDoc,		nOS			},
	{ jCabGo,			nCabGo,			TOK_ALLWIN,				actPush,		jSetupDoc,		nOS			},
	{ jCabGo,			nCabGo,			TOK_ALLNT,				actPush,		jSetupDoc,		nOS			},
	{ jCabGo,			nCabGo,			TOK_DBCS,				actPush,		jSetupDoc,		nOS			},
	{ jCabGo,			nCabGo,			TOK_OEM,				actPush,		jSetupDoc,		nOS			},
	{ jCabGo,			nCabGo,			TOK_RTL,				actPush,		jSetupDoc,		nOS			},
	{ jCabGo,			nCabGo,			TOK_JPN,				actPush,		jSetupDoc,		nOS			},
	{ jCabGo,			nCabGo,			TOK_GER,				actPush,		jSetupDoc,		nOS			},
	{ jCabGo,			nCabGo,			TOK_FRA,				actPush,		jSetupDoc,		nOS			},
	{ jCabGo,			nCabGo,			TOK_SPA,				actPush,		jSetupDoc,		nOS			},
	{ jCabGo,			nCabGo,			TOK_USA,				actPush,		jSetupDoc,		nOS			},
	{ jCabGo,			nCabGo,			TOK_APP1,				actPush,		jSetupDoc,		nOS			},
	{ jCabGo,			nCabGo,			TOK_APP2,				actPush,		jSetupDoc,		nOS			},
	{ jCabGo,			nCabGo,			TOK_APP3,				actPush,		jSetupDoc,		nOS			},
	{ jCabGo,			nCabGo,			DEFAULT_ACTION,			actPop,			jDontCare,		nDontCare	},

	{ jRegWiz,			nStart,			TOK_STRING,				actJump,		jRegWiz,		nRegWiz		},
	{ jRegWiz,			nRegWiz,		TOK_WIN95,				actPush,		jSetupDoc,		nOS			},
	{ jRegWiz,			nRegWiz,		TOK_WIN98,				actPush,		jSetupDoc,		nOS			},
	{ jRegWiz,			nRegWiz,		TOK_NT40,				actPush,		jSetupDoc,		nOS			},
	{ jRegWiz,			nRegWiz,		TOK_NT50,				actPush,		jSetupDoc,		nOS			},
	{ jRegWiz,			nRegWiz,		TOK_ALLWIN,				actPush,		jSetupDoc,		nOS			},
	{ jRegWiz,			nRegWiz,		TOK_ALLNT,				actPush,		jSetupDoc,		nOS			},
	{ jRegWiz,			nRegWiz,		TOK_DBCS,				actPush,		jSetupDoc,		nOS			},
	{ jRegWiz,			nRegWiz,		TOK_OEM,				actPush,		jSetupDoc,		nOS			},
	{ jRegWiz,			nRegWiz,		TOK_RTL,				actPush,		jSetupDoc,		nOS			},
	{ jRegWiz,			nRegWiz,		TOK_JPN,				actPush,		jSetupDoc,		nOS			},
	{ jRegWiz,			nRegWiz,		TOK_GER,				actPush,		jSetupDoc,		nOS			},
	{ jRegWiz,			nRegWiz,		TOK_FRA,				actPush,		jSetupDoc,		nOS			},
	{ jRegWiz,			nRegWiz,		TOK_SPA,				actPush,		jSetupDoc,		nOS			},
	{ jRegWiz,			nRegWiz,		TOK_USA,				actPush,		jSetupDoc,		nOS			},
	{ jRegWiz,			nRegWiz,		TOK_APP1,				actPush,		jSetupDoc,		nOS			},
	{ jRegWiz,			nRegWiz,		TOK_APP2,				actPush,		jSetupDoc,		nOS			},
	{ jRegWiz,			nRegWiz,		TOK_APP3,				actPush,		jSetupDoc,		nOS			},
	{ jRegWiz,			nRegWiz,		DEFAULT_ACTION,			actPop,			jDontCare,		nDontCare	},

	{ jShellExecute,	nStart,			TOK_WAIT,			actPush,		jShellExecute,		nWait		},
	{ jShellExecute,	nStart,			TOK_UNINSTALL,		actPush,		jShellExecute,		nUninstall	},
	{ jShellExecute,	nWait,			DEFAULT_ACTION,		actPop,			jDontCare,			nDontCare	},
	{ jShellExecute,	nStart,			TOK_STRING,			actJump,		jShellExecute,		nFilename	},
	{ jShellExecute,	nFilename,		TOK_STRING,			actJump,		jShellExecute,		nDirectory	},
	{ jShellExecute,	nDirectory,		TOK_STRING,			actJump,		jShellExecute,		nParameters	},
	{ jShellExecute,	nParameters,	TOK_STRING,			actJump,		jShellExecute,		nShow		},
	{ jShellExecute,	nShow,			TOK_STRING,			actJump,		jShellExecute,		nGroup		},
	{ jShellExecute,	nGroup,			TOK_WIN95,			actPush,		jSetupDoc,			nOS			},
	{ jShellExecute,	nGroup,			TOK_WIN98,			actPush,		jSetupDoc,			nOS			},
	{ jShellExecute,	nGroup,			TOK_NT40,			actPush,		jSetupDoc,			nOS			},
	{ jShellExecute,	nGroup,			TOK_NT50,			actPush,		jSetupDoc,			nOS			},
	{ jShellExecute,	nGroup,			TOK_ALLWIN,			actPush,		jSetupDoc,			nOS			},
	{ jShellExecute,	nGroup,			TOK_ALLNT,			actPush,		jSetupDoc,			nOS			},
	{ jShellExecute,	nGroup,			TOK_DBCS,			actPush,		jSetupDoc,			nOS			},
	{ jShellExecute,	nGroup,			TOK_OEM,			actPush,		jSetupDoc,			nOS			},
	{ jShellExecute,	nGroup,			TOK_RTL,			actPush,		jSetupDoc,			nOS			},
	{ jShellExecute,	nGroup,			TOK_JPN,			actPush,		jSetupDoc,			nOS			},
	{ jShellExecute,	nGroup,			TOK_GER,			actPush,		jSetupDoc,			nOS			},
	{ jShellExecute,	nGroup,			TOK_FRA,			actPush,		jSetupDoc,			nOS			},
	{ jShellExecute,	nGroup,			TOK_SPA,			actPush,		jSetupDoc,			nOS			},
	{ jShellExecute,	nGroup,			TOK_USA,			actPush,		jSetupDoc,			nOS			},
	{ jShellExecute,	nGroup,			TOK_APP1,			actPush,		jSetupDoc,			nOS			},
	{ jShellExecute,	nGroup,			TOK_APP2,			actPush,		jSetupDoc,			nOS			},
	{ jShellExecute,	nGroup,			TOK_APP3,			actPush,		jSetupDoc,			nOS			},
	{ jShellExecute,	nGroup,			DEFAULT_ACTION,		actPop,			jDontCare,			nDontCare	},
	{ jShellExecute,	nUninstall,		DEFAULT_ACTION,		actPop,			jDontCare,			nDontCare	},

	{ jMkDir,			nStart,			TOK_UNINSTALL,		actPush,		jMkDir,				nUninstall	},
	{ jMkDir,			nStart,			TOK_UNINSTALLALL,	actPush,		jMkDir,				nUninstall	},
	{ jMkDir,			nUninstall,		DEFAULT_ACTION,		actPop,			jDontCare,			nDontCare	},
	{ jMkDir,			nStart,			TOK_STRING,			actJump,		jMkDir,				nMkDir		},
	{ jMkDir,			nMkDir,			TOK_STRING,			actJump,		jMkDir,				nGroup		},
	{ jMkDir,			nGroup,			TOK_WIN95,			actPush,		jSetupDoc,			nOS			},
	{ jMkDir,			nGroup,			TOK_WIN98,			actPush,		jSetupDoc,			nOS			},
	{ jMkDir,			nGroup,			TOK_NT40,			actPush,		jSetupDoc,			nOS			},
	{ jMkDir,			nGroup,			TOK_NT50,			actPush,		jSetupDoc,			nOS			},
	{ jMkDir,			nGroup,			TOK_ALLWIN,			actPush,		jSetupDoc,			nOS			},
	{ jMkDir,			nGroup,			TOK_ALLNT,			actPush,		jSetupDoc,			nOS			},
	{ jMkDir,			nGroup,			TOK_DBCS,			actPush,		jSetupDoc,			nOS			},
	{ jMkDir,			nGroup,			TOK_OEM,			actPush,		jSetupDoc,			nOS			},
	{ jMkDir,			nGroup,			TOK_RTL,			actPush,		jSetupDoc,			nOS			},
	{ jMkDir,			nGroup,			TOK_JPN,			actPush,		jSetupDoc,			nOS			},
	{ jMkDir,			nGroup,			TOK_GER,			actPush,		jSetupDoc,			nOS			},
	{ jMkDir,			nGroup,			TOK_FRA,			actPush,		jSetupDoc,			nOS			},
	{ jMkDir,			nGroup,			TOK_SPA,			actPush,		jSetupDoc,			nOS			},
	{ jMkDir,			nGroup,			TOK_USA,			actPush,		jSetupDoc,			nOS			},
	{ jMkDir,			nGroup,			TOK_APP1,			actPush,		jSetupDoc,			nOS			},
	{ jMkDir,			nGroup,			TOK_APP2,			actPush,		jSetupDoc,			nOS			},
	{ jMkDir,			nGroup,			TOK_APP3,			actPush,		jSetupDoc,			nOS			},
	{ jMkDir,			nGroup,			DEFAULT_ACTION,		actPop,			jDontCare,			nDontCare	},

	{ jMkRoot,			nStart,			TOK_UNINSTALL,		actPush,		jMkRoot,			nUninstall	},
	{ jMkRoot,			nUninstall,		DEFAULT_ACTION,		actPop,			jDontCare,			nDontCare	},
	{ jMkRoot,			nStart,			TOK_WIN95,			actJump,		jMkRoot,			nOS			},
	{ jMkRoot,			nStart,			TOK_WIN98,			actJump,		jMkRoot,			nOS			},
	{ jMkRoot,			nStart,			TOK_NT40,			actJump,		jMkRoot,			nOS			},
	{ jMkRoot,			nStart,			TOK_NT50,			actJump,		jMkRoot,			nOS			},
	{ jMkRoot,			nStart,			TOK_ALLWIN,			actJump,		jMkRoot,			nOS			},
	{ jMkRoot,			nStart,			TOK_ALLNT,			actJump,		jMkRoot,			nOS			},
	{ jMkRoot,			nStart,			TOK_DBCS,			actJump,		jMkRoot,			nOS			},
	{ jMkRoot,			nStart,			TOK_OEM,			actJump,		jMkRoot,			nOS			},
	{ jMkRoot,			nStart,			TOK_RTL,			actJump,		jMkRoot,			nOS			},
	{ jMkRoot,			nStart,			TOK_JPN,			actJump,		jMkRoot,			nOS			},
	{ jMkRoot,			nStart,			TOK_GER,			actJump,		jMkRoot,			nOS			},
	{ jMkRoot,			nStart,			TOK_FRA,			actJump,		jMkRoot,			nOS			},
	{ jMkRoot,			nStart,			TOK_SPA,			actJump,		jMkRoot,			nOS			},
	{ jMkRoot,			nStart,			TOK_USA,			actJump,		jMkRoot,			nOS			},
	{ jMkRoot,			nStart,			TOK_APP1,			actJump,		jMkRoot,			nOS			},
	{ jMkRoot,			nStart,			TOK_APP2,			actJump,		jMkRoot,			nOS			},
	{ jMkRoot,			nStart,			TOK_APP3,			actJump,		jMkRoot,			nOS			},
	{ jMkRoot,			nStart,			TOK_IMEENABLE,		actJump,		jMkRoot,			nOS			},
	{ jMkRoot,			nStart,			TOK_IMEON,			actJump,		jMkRoot,			nOS			},
	{ jMkRoot,			nOS,			TOK_WIN95,			actPush,		jSetupDoc,			nOS			},
	{ jMkRoot,			nOS,			TOK_WIN98,			actPush,		jSetupDoc,			nOS			},
	{ jMkRoot,			nOS,			TOK_NT40,			actPush,		jSetupDoc,			nOS			},
	{ jMkRoot,			nOS,			TOK_NT50,			actPush,		jSetupDoc,			nOS			},
	{ jMkRoot,			nOS,			TOK_ALLWIN,			actPush,		jSetupDoc,			nOS			},
	{ jMkRoot,			nOS,			TOK_ALLNT,			actPush,		jSetupDoc,			nOS			},
	{ jMkRoot,			nOS,			TOK_DBCS,			actPush,		jSetupDoc,			nOS			},
	{ jMkRoot,			nOS,			TOK_OEM,			actPush,		jSetupDoc,			nOS			},
	{ jMkRoot,			nOS,			TOK_RTL,			actPush,		jSetupDoc,			nOS			},
	{ jMkRoot,			nOS,			TOK_JPN,			actPush,		jSetupDoc,			nOS			},
	{ jMkRoot,			nOS,			TOK_GER,			actPush,		jSetupDoc,			nOS			},
	{ jMkRoot,			nOS,			TOK_FRA,			actPush,		jSetupDoc,			nOS			},
	{ jMkRoot,			nOS,			TOK_SPA,			actPush,		jSetupDoc,			nOS			},
	{ jMkRoot,			nOS,			TOK_USA,			actPush,		jSetupDoc,			nOS			},
	{ jMkRoot,			nOS,			TOK_APP1,			actPush,		jSetupDoc,			nOS			},
	{ jMkRoot,			nOS,			TOK_APP2,			actPush,		jSetupDoc,			nOS			},
	{ jMkRoot,			nOS,			TOK_APP3,			actPush,		jSetupDoc,			nOS			},
	{ jMkRoot,			nOS,			TOK_IMEENABLE,		actPush,		jSetupDoc,			nOS			},
	{ jMkRoot,			nOS,			TOK_IMEON,			actPush,		jSetupDoc,			nOS			},
	{ jMkRoot,			nOS,			DEFAULT_ACTION,		actPop,			jDontCare,			nDontCare	},

	{ jGetName,			nStart,			TOK_WIN95,			actJump,		jGetName,			nOS			},
	{ jGetName,			nStart,			TOK_WIN98,			actJump,		jGetName,			nOS			},
	{ jGetName,			nStart,			TOK_NT40,			actJump,		jGetName,			nOS			},
	{ jGetName,			nStart,			TOK_NT50,			actJump,		jGetName,			nOS			},
	{ jGetName,			nStart,			TOK_ALLWIN,			actJump,		jGetName,			nOS			},
	{ jGetName,			nStart,			TOK_ALLNT,			actJump,		jGetName,			nOS			},
	{ jGetName,			nStart,			TOK_DBCS,			actJump,		jGetName,			nOS			},
	{ jGetName,			nStart,			TOK_OEM,			actJump,		jGetName,			nOS			},
	{ jGetName,			nStart,			TOK_RTL,			actJump,		jGetName,			nOS			},
	{ jGetName,			nStart,			TOK_JPN,			actJump,		jGetName,			nOS			},
	{ jGetName,			nStart,			TOK_GER,			actJump,		jGetName,			nOS			},
	{ jGetName,			nStart,			TOK_FRA,			actJump,		jGetName,			nOS			},
	{ jGetName,			nStart,			TOK_SPA,			actJump,		jGetName,			nOS			},
	{ jGetName,			nStart,			TOK_USA,			actJump,		jGetName,			nOS			},
	{ jGetName,			nStart,			TOK_APP1,			actJump,		jGetName,			nOS			},
	{ jGetName,			nStart,			TOK_APP2,			actJump,		jGetName,			nOS			},
	{ jGetName,			nStart,			TOK_APP3,			actJump,		jGetName,			nOS			},
	{ jGetName,			nStart,			TOK_IMEENABLE,		actJump,		jGetName,			nOS			},
	{ jGetName,			nStart,			TOK_IMEON,			actJump,		jGetName,			nOS			},
	{ jGetName,			nOS,			TOK_WIN95,			actPush,		jSetupDoc,			nOS			},
	{ jGetName,			nOS,			TOK_WIN98,			actPush,		jSetupDoc,			nOS			},
	{ jGetName,			nOS,			TOK_NT40,			actPush,		jSetupDoc,			nOS			},
	{ jGetName,			nOS,			TOK_NT50,			actPush,		jSetupDoc,			nOS			},
	{ jGetName,			nOS,			TOK_ALLWIN,			actPush,		jSetupDoc,			nOS			},
	{ jGetName,			nOS,			TOK_ALLNT,			actPush,		jSetupDoc,			nOS			},
	{ jGetName,			nOS,			TOK_DBCS,			actPush,		jSetupDoc,			nOS			},
	{ jGetName,			nOS,			TOK_OEM,			actPush,		jSetupDoc,			nOS			},
	{ jGetName,			nOS,			TOK_RTL,			actPush,		jSetupDoc,			nOS			},
	{ jGetName,			nOS,			TOK_JPN,			actPush,		jSetupDoc,			nOS			},
	{ jGetName,			nOS,			TOK_GER,			actPush,		jSetupDoc,			nOS			},
	{ jGetName,			nOS,			TOK_FRA,			actPush,		jSetupDoc,			nOS			},
	{ jGetName,			nOS,			TOK_SPA,			actPush,		jSetupDoc,			nOS			},
	{ jGetName,			nOS,			TOK_USA,			actPush,		jSetupDoc,			nOS			},
	{ jGetName,			nOS,			TOK_APP1,			actPush,		jSetupDoc,			nOS			},
	{ jGetName,			nOS,			TOK_APP2,			actPush,		jSetupDoc,			nOS			},
	{ jGetName,			nOS,			TOK_APP3,			actPush,		jSetupDoc,			nOS			},
	{ jGetName,			nOS,			TOK_IMEENABLE,		actPush,		jSetupDoc,			nOS			},
	{ jGetName,			nOS,			TOK_IMEON,			actPush,		jSetupDoc,			nOS			},
	{ jGetName,			nOS,			DEFAULT_ACTION,		actPop,			jDontCare,			nDontCare	},

	{ jGetGroup,		nStart,			TOK_WIN95,			actJump,		jGetGroup,			nOS			},
	{ jGetGroup,		nStart,			TOK_WIN98,			actJump,		jGetGroup,			nOS			},
	{ jGetGroup,		nStart,			TOK_NT40,			actJump,		jGetGroup,			nOS			},
	{ jGetGroup,		nStart,			TOK_NT50,			actJump,		jGetGroup,			nOS			},
	{ jGetGroup,		nStart,			TOK_ALLWIN,			actJump,		jGetGroup,			nOS			},
	{ jGetGroup,		nStart,			TOK_ALLNT,			actJump,		jGetGroup,			nOS			},
	{ jGetGroup,		nStart,			TOK_DBCS,			actJump,		jGetGroup,			nOS			},
	{ jGetGroup,		nStart,			TOK_OEM,			actJump,		jGetGroup,			nOS			},
	{ jGetGroup,		nStart,			TOK_RTL,			actJump,		jGetGroup,			nOS			},
	{ jGetGroup,		nStart,			TOK_JPN,			actJump,		jGetGroup,			nOS			},
	{ jGetGroup,		nStart,			TOK_GER,			actJump,		jGetGroup,			nOS			},
	{ jGetGroup,		nStart,			TOK_FRA,			actJump,		jGetGroup,			nOS			},
	{ jGetGroup,		nStart,			TOK_FRA,			actJump,		jGetGroup,			nOS			},
	{ jGetGroup,		nStart,			TOK_SPA,			actJump,		jGetGroup,			nOS			},
	{ jGetGroup,		nStart,			TOK_APP1,			actJump,		jGetGroup,			nOS			},
	{ jGetGroup,		nStart,			TOK_APP2,			actJump,		jGetGroup,			nOS			},
	{ jGetGroup,		nStart,			TOK_APP3,			actJump,		jGetGroup,			nOS			},
	{ jGetGroup,		nOS,			TOK_WIN95,			actPush,		jSetupDoc,			nOS			},
	{ jGetGroup,		nOS,			TOK_WIN98,			actPush,		jSetupDoc,			nOS			},
	{ jGetGroup,		nOS,			TOK_NT40,			actPush,		jSetupDoc,			nOS			},
	{ jGetGroup,		nOS,			TOK_NT50,			actPush,		jSetupDoc,			nOS			},
	{ jGetGroup,		nOS,			TOK_ALLWIN,			actPush,		jSetupDoc,			nOS			},
	{ jGetGroup,		nOS,			TOK_ALLNT,			actPush,		jSetupDoc,			nOS			},
	{ jGetGroup,		nOS,			TOK_DBCS,			actPush,		jSetupDoc,			nOS			},
	{ jGetGroup,		nOS,			TOK_OEM,			actPush,		jSetupDoc,			nOS			},
	{ jGetGroup,		nOS,			TOK_RTL,			actPush,		jSetupDoc,			nOS			},
	{ jGetGroup,		nOS,			TOK_JPN,			actPush,		jSetupDoc,			nOS			},
	{ jGetGroup,		nOS,			TOK_GER,			actPush,		jSetupDoc,			nOS			},
	{ jGetGroup,		nOS,			TOK_FRA,			actPush,		jSetupDoc,			nOS			},
	{ jGetGroup,		nOS,			TOK_SPA,			actPush,		jSetupDoc,			nOS			},
	{ jGetGroup,		nOS,			TOK_USA,			actPush,		jSetupDoc,			nOS			},
	{ jGetGroup,		nOS,			TOK_APP1,			actPush,		jSetupDoc,			nOS			},
	{ jGetGroup,		nOS,			TOK_APP2,			actPush,		jSetupDoc,			nOS			},
	{ jGetGroup,		nOS,			TOK_APP3,			actPush,		jSetupDoc,			nOS			},
	{ jGetGroup,		nOS,			DEFAULT_ACTION,		actPop,			jDontCare,			nDontCare	},
	
	{ jGetPID,			nStart,			TOK_WIN95,			actJump,		jGetPID,			nOS			},
	{ jGetPID,			nStart,			TOK_WIN98,			actJump,		jGetPID,			nOS			},
	{ jGetPID,			nStart,			TOK_NT40,			actJump,		jGetPID,			nOS			},
	{ jGetPID,			nStart,			TOK_NT50,			actJump,		jGetPID,			nOS			},
	{ jGetPID,			nStart,			TOK_ALLWIN,			actJump,		jGetPID,			nOS			},
	{ jGetPID,			nStart,			TOK_ALLNT,			actJump,		jGetPID,			nOS			},
	{ jGetPID,			nStart,			TOK_DBCS,			actJump,		jGetPID,			nOS			},
	{ jGetPID,			nStart,			TOK_OEM,			actJump,		jGetPID,			nOS			},
	{ jGetPID,			nStart,			TOK_RTL,			actJump,		jGetPID,			nOS			},
	{ jGetPID,			nStart,			TOK_JPN,			actJump,		jGetPID,			nOS			},
	{ jGetPID,			nStart,			TOK_GER,			actJump,		jGetPID,			nOS			},
	{ jGetPID,			nStart,			TOK_FRA,			actJump,		jGetPID,			nOS			},
	{ jGetPID,			nStart,			TOK_SPA,			actJump,		jGetPID,			nOS			},
	{ jGetPID,			nStart,			TOK_USA,			actJump,		jGetPID,			nOS			},
	{ jGetPID,			nStart,			TOK_APP1,			actJump,		jGetPID,			nOS			},
	{ jGetPID,			nStart,			TOK_APP2,			actJump,		jGetPID,			nOS			},
	{ jGetPID,			nStart,			TOK_APP3,			actJump,		jGetPID,			nOS			},
	{ jGetPID,			nStart,			TOK_IMEENABLE,		actJump,		jGetPID,			nOS			},
	{ jGetPID,			nStart,			TOK_IMEON,			actJump,		jGetPID,			nOS			},
	{ jGetPID,			nOS,			TOK_WIN95,			actPush,		jSetupDoc,			nOS			},
	{ jGetPID,			nOS,			TOK_WIN98,			actPush,		jSetupDoc,			nOS			},
	{ jGetPID,			nOS,			TOK_NT40,			actPush,		jSetupDoc,			nOS			},
	{ jGetPID,			nOS,			TOK_NT50,			actPush,		jSetupDoc,			nOS			},
	{ jGetPID,			nOS,			TOK_ALLWIN,			actPush,		jSetupDoc,			nOS			},
	{ jGetPID,			nOS,			TOK_ALLNT,			actPush,		jSetupDoc,			nOS			},
	{ jGetPID,			nOS,			TOK_DBCS,			actPush,		jSetupDoc,			nOS			},
	{ jGetPID,			nOS,			TOK_OEM,			actPush,		jSetupDoc,			nOS			},
	{ jGetPID,			nOS,			TOK_RTL,			actPush,		jSetupDoc,			nOS			},
	{ jGetPID,			nOS,			TOK_JPN,			actPush,		jSetupDoc,			nOS			},
	{ jGetPID,			nOS,			TOK_GER,			actPush,		jSetupDoc,			nOS			},
	{ jGetPID,			nOS,			TOK_FRA,			actPush,		jSetupDoc,			nOS			},
	{ jGetPID,			nOS,			TOK_SPA,			actPush,		jSetupDoc,			nOS			},
	{ jGetPID,			nOS,			TOK_USA,			actPush,		jSetupDoc,			nOS			},
	{ jGetPID,			nOS,			TOK_APP1,			actPush,		jSetupDoc,			nOS			},
	{ jGetPID,			nOS,			TOK_APP2,			actPush,		jSetupDoc,			nOS			},
	{ jGetPID,			nOS,			TOK_APP3,			actPush,		jSetupDoc,			nOS			},
	{ jGetPID,			nOS,			TOK_IMEENABLE,		actPush,		jSetupDoc,			nOS			},
	{ jGetPID,			nOS,			TOK_IMEON,			actPush,		jSetupDoc,			nOS			},
	{ jGetPID,			nOS,			DEFAULT_ACTION,		actPop,			jDontCare,			nDontCare	},

	{ jInstDX,			nStart,			TOK_STRING,			actJump,		jInstDX,			nInstDX			},
	{ jInstDX,			nInstDX,		TOK_STRING,			actJump,		jInstDX,			nInstDXName		},
	{ jInstDX,			nInstDXName,	TOK_STRING,			actJump,		jInstDX,			nInstDXMinVersion	},
	{ jInstDX,			nInstDXMinVersion,	TOK_STRING,		actJump,		jInstDX,			nInstDXFlags	},
	{ jInstDX,			nInstDXFlags,	TOK_WIN95,			actPush,		jSetupDoc,			nOS			},
	{ jInstDX,			nInstDXFlags,	TOK_WIN98,			actPush,		jSetupDoc,			nOS			},
	{ jInstDX,			nInstDXFlags,	TOK_NT40,			actPush,		jSetupDoc,			nOS			},
	{ jInstDX,			nInstDXFlags,	TOK_NT50,			actPush,		jSetupDoc,			nOS			},
	{ jInstDX,			nInstDXFlags,	TOK_ALLWIN,			actPush,		jSetupDoc,			nOS			},
	{ jInstDX,			nInstDXFlags,	TOK_ALLNT,			actPush,		jSetupDoc,			nOS			},
	{ jInstDX,			nInstDXFlags,	TOK_DBCS,			actPush,		jSetupDoc,			nOS			},
	{ jInstDX,			nInstDXFlags,	TOK_OEM,			actPush,		jSetupDoc,			nOS			},
	{ jInstDX,			nInstDXFlags,	TOK_RTL,			actPush,		jSetupDoc,			nOS			},
	{ jInstDX,			nInstDXFlags,	TOK_JPN,			actPush,		jSetupDoc,			nOS			},
	{ jInstDX,			nInstDXFlags,	TOK_GER,			actPush,		jSetupDoc,			nOS			},
	{ jInstDX,			nInstDXFlags,	TOK_FRA,			actPush,		jSetupDoc,			nOS			},
	{ jInstDX,			nInstDXFlags,	TOK_SPA,			actPush,		jSetupDoc,			nOS			},
	{ jInstDX,			nInstDXFlags,	TOK_USA,			actPush,		jSetupDoc,			nOS			},
	{ jInstDX,			nInstDXFlags,	TOK_APP1,			actPush,		jSetupDoc,			nOS			},
	{ jInstDX,			nInstDXFlags,	TOK_APP2,			actPush,		jSetupDoc,			nOS			},
	{ jInstDX,			nInstDXFlags,	TOK_APP3,			actPush,		jSetupDoc,			nOS			},
	{ jInstDX,			nInstDXFlags,	DEFAULT_ACTION,		actPop,			jDontCare,			nDontCare	},

	{ jInstDPLAY,			nStart,				TOK_STRING,			actJump,		jInstDPLAY,			nInstDPLAYName	},
	{ jInstDPLAY,			nInstDPLAYName,		TOK_STRING,			actJump,		jInstDPLAY,			nInstDPLAYMinVersion	},
	{ jInstDPLAY,			nInstDPLAYMinVersion,	TOK_WIN95,			actPush,		jSetupDoc,			nOS			},
	{ jInstDPLAY,			nInstDPLAYMinVersion,	TOK_WIN98,			actPush,		jSetupDoc,			nOS			},
	{ jInstDPLAY,			nInstDPLAYMinVersion,	TOK_NT40,			actPush,		jSetupDoc,			nOS			},
	{ jInstDPLAY,			nInstDPLAYMinVersion,	TOK_NT50,			actPush,		jSetupDoc,			nOS			},
	{ jInstDPLAY,			nInstDPLAYMinVersion,	TOK_ALLWIN,			actPush,		jSetupDoc,			nOS			},
	{ jInstDPLAY,			nInstDPLAYMinVersion,	TOK_ALLNT,			actPush,		jSetupDoc,			nOS			},
	{ jInstDPLAY,			nInstDPLAYMinVersion,	TOK_DBCS,			actPush,		jSetupDoc,			nOS			},
	{ jInstDPLAY,			nInstDPLAYMinVersion,	TOK_OEM,			actPush,		jSetupDoc,			nOS			},
	{ jInstDPLAY,			nInstDPLAYMinVersion,	TOK_RTL,			actPush,		jSetupDoc,			nOS			},
	{ jInstDPLAY,			nInstDPLAYMinVersion,	TOK_JPN,			actPush,		jSetupDoc,			nOS			},
	{ jInstDPLAY,			nInstDPLAYMinVersion,	TOK_GER,			actPush,		jSetupDoc,			nOS			},
	{ jInstDPLAY,			nInstDPLAYMinVersion,	TOK_FRA,			actPush,		jSetupDoc,			nOS			},
	{ jInstDPLAY,			nInstDPLAYMinVersion,	TOK_SPA,			actPush,		jSetupDoc,			nOS			},
	{ jInstDPLAY,			nInstDPLAYMinVersion,	TOK_USA,			actPush,		jSetupDoc,			nOS			},
	{ jInstDPLAY,			nInstDPLAYMinVersion,	TOK_APP1,			actPush,		jSetupDoc,			nOS			},
	{ jInstDPLAY,			nInstDPLAYMinVersion,	TOK_APP2,			actPush,		jSetupDoc,			nOS			},
	{ jInstDPLAY,			nInstDPLAYMinVersion,	TOK_APP3,			actPush,		jSetupDoc,			nOS			},
	{ jInstDPLAY,			nInstDPLAYMinVersion,	DEFAULT_ACTION,		actPop,			jDontCare,			nDontCare	},

	{ jInstIcon,		nStart,				TOK_UNINSTALLLINK,	actPush,	jInstIcon,			nUninstallLink	},
	{ jInstIcon,		nUninstallLink,		DEFAULT_ACTION,		actPop,		jDontCare,			nDontCare	},
	{ jInstIcon,		nStart,				TOK_STRING,			actJump,	jInstIcon,			nInstIconName		},
	{ jInstIcon,		nInstIconName,		TOK_STRING,			actJump,	jInstIcon,			nInstIconNameIcon	},
	{ jInstIcon,		nInstIconNameIcon,	TOK_STRING,			actJump,	jInstIcon,			nInstIconPath		},
	{ jInstIcon,		nInstIconPath,		TOK_STRING,			actJump,	jInstIcon,			nInstIconDest		},
	{ jInstIcon,		nInstIconDest,		TOK_STRING,			actJump,	jInstIcon,			nInstIconIndex		},
	{ jInstIcon,		nInstIconIndex,		TOK_STRING,			actJump,	jInstIcon,			nGroup		},
	{ jInstIcon,		nGroup,				TOK_WIN95,			actPush,	jSetupDoc,			nOS			},
	{ jInstIcon,		nGroup,				TOK_WIN98,			actPush,	jSetupDoc,			nOS			},
	{ jInstIcon,		nGroup,				TOK_NT40,			actPush,	jSetupDoc,			nOS			},
	{ jInstIcon,		nGroup,				TOK_NT50,			actPush,	jSetupDoc,			nOS			},
	{ jInstIcon,		nGroup,				TOK_ALLWIN,			actPush,	jSetupDoc,			nOS			},
	{ jInstIcon,		nGroup,				TOK_ALLNT,			actPush,	jSetupDoc,			nOS			},
	{ jInstIcon,		nGroup,				TOK_DBCS,			actPush,	jSetupDoc,			nOS			},
	{ jInstIcon,		nGroup,				TOK_OEM,			actPush,	jSetupDoc,			nOS			},
	{ jInstIcon,		nGroup,				TOK_RTL,			actPush,	jSetupDoc,			nOS			},
	{ jInstIcon,		nGroup,				TOK_JPN,			actPush,	jSetupDoc,			nOS			},
	{ jInstIcon,		nGroup,				TOK_GER,			actPush,	jSetupDoc,			nOS			},
	{ jInstIcon,		nGroup,				TOK_FRA,			actPush,	jSetupDoc,			nOS			},
	{ jInstIcon,		nGroup,				TOK_SPA,			actPush,	jSetupDoc,			nOS			},
	{ jInstIcon,		nGroup,				TOK_USA,			actPush,	jSetupDoc,			nOS			},
	{ jInstIcon,		nGroup,				TOK_APP1,			actPush,	jSetupDoc,			nOS			},
	{ jInstIcon,		nGroup,				TOK_APP2,			actPush,	jSetupDoc,			nOS			},
	{ jInstIcon,		nGroup,				TOK_APP3,			actPush,	jSetupDoc,			nOS			},
	{ jInstIcon,		nGroup,				DEFAULT_ACTION,		actPop,		jDontCare,			nDontCare	},

	{ jCDSpeed,			nStart,				TOK_STRING,			actJump,	jCDSpeed,		nCDSpeedCDMin	},
	{ jCDSpeed,			nCDSpeedCDMin,		TOK_STRING,			actJump,	jCDSpeed,		nCDSpeedCPUMax	},
	{ jCDSpeed,			nCDSpeedCPUMax,		TOK_STRING,			actJump,	jCDSpeed,		nCDSpeedName	},
	{ jCDSpeed,			nCDSpeedName,		TOK_WIN95,			actPush,	jSetupDoc,		nOS				},
	{ jCDSpeed,			nCDSpeedName,		TOK_WIN98,			actPush,	jSetupDoc,		nOS				},
	{ jCDSpeed,			nCDSpeedName,		TOK_NT40,			actPush,	jSetupDoc,		nOS				},
	{ jCDSpeed,			nCDSpeedName,		TOK_NT50,			actPush,	jSetupDoc,		nOS				},
	{ jCDSpeed,			nCDSpeedName,		TOK_ALLWIN,			actPush,	jSetupDoc,		nOS				},
	{ jCDSpeed,			nCDSpeedName,		TOK_ALLNT,			actPush,	jSetupDoc,		nOS				},
	{ jCDSpeed,			nCDSpeedName,		TOK_DBCS,			actPush,	jSetupDoc,		nOS				},
	{ jCDSpeed,			nCDSpeedName,		TOK_OEM,			actPush,	jSetupDoc,		nOS				},
	{ jCDSpeed,			nCDSpeedName,		TOK_RTL,			actPush,	jSetupDoc,		nOS				},
	{ jCDSpeed,			nCDSpeedName,		TOK_JPN,			actPush,	jSetupDoc,		nOS				},
	{ jCDSpeed,			nCDSpeedName,		TOK_GER,			actPush,	jSetupDoc,		nOS				},
	{ jCDSpeed,			nCDSpeedName,		TOK_FRA,			actPush,	jSetupDoc,		nOS				},
	{ jCDSpeed,			nCDSpeedName,		TOK_SPA,			actPush,	jSetupDoc,		nOS				},
	{ jCDSpeed,			nCDSpeedName,		TOK_USA,			actPush,	jSetupDoc,		nOS				},
	{ jCDSpeed,			nCDSpeedName,		TOK_APP1,			actPush,	jSetupDoc,		nOS				},
	{ jCDSpeed,			nCDSpeedName,		TOK_APP2,			actPush,	jSetupDoc,		nOS				},
	{ jCDSpeed,			nCDSpeedName,		TOK_APP3,			actPush,	jSetupDoc,		nOS				},
	{ jCDSpeed,			nCDSpeedName,		DEFAULT_ACTION,		actPop,		jDontCare,		nDontCare		},

	{ jDeleteFile,		nStart,				TOK_PERSIST,		actPush,	jDeleteFile,	nPersist		},
	{ jDeleteFile,		nPersist,			DEFAULT_ACTION,		actPop,		jDontCare,		nDontCare		},
	{ jDeleteFile,		nStart,				TOK_DELFILEINSTALL,	actPush,	jDeleteFile,	nInstall		},
	{ jDeleteFile,		nInstall,			DEFAULT_ACTION,		actPop,		jDontCare,		nDontCare		},
	{ jDeleteFile,		nStart,				TOK_RECURSE,		actPush,	jDeleteFile,	nRecurse		},
	{ jDeleteFile,		nRecurse,			DEFAULT_ACTION,		actPop,		jDontCare,		nDontCare		},
	{ jDeleteFile,		nStart,				TOK_SILENT,			actPush,	jDeleteFile,	nSilent			},
	{ jDeleteFile,		nSilent,			DEFAULT_ACTION,		actPop,		jDontCare,		nDontCare		},
	{ jDeleteFile,		nStart,				TOK_STRING,			actJump,	jDeleteFile,	nSrcName		},
	{ jDeleteFile,		nSrcName,			TOK_WIN95,			actPush,	jSetupDoc,		nOS				},
	{ jDeleteFile,		nSrcName,			TOK_WIN98,			actPush,	jSetupDoc,		nOS				},
	{ jDeleteFile,		nSrcName,			TOK_NT40,			actPush,	jSetupDoc,		nOS				},
	{ jDeleteFile,		nSrcName,			TOK_NT50,			actPush,	jSetupDoc,		nOS				},
	{ jDeleteFile,		nSrcName,			TOK_ALLWIN,			actPush,	jSetupDoc,		nOS				},
	{ jDeleteFile,		nSrcName,			TOK_ALLNT,			actPush,	jSetupDoc,		nOS				},
	{ jDeleteFile,		nSrcName,			TOK_DBCS,			actPush,	jSetupDoc,		nOS				},
	{ jDeleteFile,		nSrcName,			TOK_OEM,			actPush,	jSetupDoc,		nOS				},
	{ jDeleteFile,		nSrcName,			TOK_RTL,			actPush,	jSetupDoc,		nOS				},
	{ jDeleteFile,		nSrcName,			TOK_JPN,			actPush,	jSetupDoc,		nOS				},
	{ jDeleteFile,		nSrcName,			TOK_GER,			actPush,	jSetupDoc,		nOS				},
	{ jDeleteFile,		nSrcName,			TOK_FRA,			actPush,	jSetupDoc,		nOS				},
	{ jDeleteFile,		nSrcName,			TOK_SPA,			actPush,	jSetupDoc,		nOS				},
	{ jDeleteFile,		nSrcName,			TOK_USA,			actPush,	jSetupDoc,		nOS				},
	{ jDeleteFile,		nSrcName,			TOK_APP1,			actPush,	jSetupDoc,		nOS				},
	{ jDeleteFile,		nSrcName,			TOK_APP2,			actPush,	jSetupDoc,		nOS				},
	{ jDeleteFile,		nSrcName,			TOK_APP3,			actPush,	jSetupDoc,		nOS				},
	{ jDeleteFile,		nSrcName,			DEFAULT_ACTION,		actPop,		jDontCare,		nDontCare		},

	{ jIniValue,		nStart,				TOK_UNINSTALL,		actPush,	jIniValue,		nUninstall		},
	{ jIniValue,		nStart,				TOK_UNINSTALLALL,	actPush,	jIniValue,		nUninstall		},
	{ jIniValue,		nStart,				TOK_STRING,			actJump,	jIniValue,		nFilename		},
	{ jIniValue,		nStart,				TOK_MAP,			actJump,	jIniValue,		nMap			},
	{ jIniValue,		nFilename,			TOK_STRING,			actJump,	jIniValue,		nSection		},
	{ jIniValue,		nMap,				TOK_STRING,			actJump,	jIniValue,		nSection		},
	{ jIniValue,		nSection,			TOK_STRING,			actJump,	jIniValue,		nEntry			},
	{ jIniValue,		nEntry,				TOK_STRING,			actJump,	jIniValue,		nValue			},
	{ jIniValue,		nValue,				TOK_STRING,			actJump,	jIniValue,		nType			},
	{ jIniValue,		nType,				TOK_STRING,			actJump,	jIniValue,		nGroup			},
	{ jIniValue,		nGroup,				TOK_WIN95,			actPush,	jSetupDoc,		nOS				},
	{ jIniValue,		nGroup,				TOK_WIN98,			actPush,	jSetupDoc,		nOS				},
	{ jIniValue,		nGroup,				TOK_NT40,			actPush,	jSetupDoc,		nOS				},
	{ jIniValue,		nGroup,				TOK_NT50,			actPush,	jSetupDoc,		nOS				},
	{ jIniValue,		nGroup,				TOK_ALLWIN,			actPush,	jSetupDoc,		nOS				},
	{ jIniValue,		nGroup,				TOK_ALLNT,			actPush,	jSetupDoc,		nOS				},
	{ jIniValue,		nGroup,				TOK_DBCS,			actPush,	jSetupDoc,		nOS				},
	{ jIniValue,		nGroup,				TOK_OEM,			actPush,	jSetupDoc,		nOS				},
	{ jIniValue,		nGroup,				TOK_RTL,			actPush,	jSetupDoc,		nOS				},
	{ jIniValue,		nGroup,				TOK_JPN,			actPush,	jSetupDoc,		nOS				},
	{ jIniValue,		nGroup,				TOK_GER,			actPush,	jSetupDoc,		nOS				},
	{ jIniValue,		nGroup,				TOK_FRA,			actPush,	jSetupDoc,		nOS				},
	{ jIniValue,		nGroup,				TOK_SPA,			actPush,	jSetupDoc,		nOS				},
	{ jIniValue,		nGroup,				TOK_USA,			actPush,	jSetupDoc,		nOS				},
	{ jIniValue,		nGroup,				TOK_APP1,			actPush,	jSetupDoc,		nOS				},
	{ jIniValue,		nGroup,				TOK_APP2,			actPush,	jSetupDoc,		nOS				},
	{ jIniValue,		nGroup,				TOK_APP3,			actPush,	jSetupDoc,		nOS				},
	{ jIniValue,		nGroup,				DEFAULT_ACTION,		actPop,		jDontCare,		nDontCare		},
	{ jIniValue,		nUninstall,			DEFAULT_ACTION,		actPop,		jDontCare,		nDontCare		},

	{ jReadFileList,	nStart,				TOK_STRING,			actJump,	jReadFileList,	nReadFileListName	},
	{ jReadFileList,	nReadFileListName,	TOK_WIN95,			actPush,	jSetupDoc,		nOS				},
	{ jReadFileList,	nReadFileListName,	TOK_WIN98,			actPush,	jSetupDoc,		nOS				},
	{ jReadFileList,	nReadFileListName,	TOK_NT40,			actPush,	jSetupDoc,		nOS				},
	{ jReadFileList,	nReadFileListName,	TOK_NT50,			actPush,	jSetupDoc,		nOS				},
	{ jReadFileList,	nReadFileListName,	TOK_ALLWIN,			actPush,	jSetupDoc,		nOS				},
	{ jReadFileList,	nReadFileListName,	TOK_ALLNT,			actPush,	jSetupDoc,		nOS				},
	{ jReadFileList,	nReadFileListName,	TOK_DBCS,			actPush,	jSetupDoc,		nOS				},
	{ jReadFileList,	nReadFileListName,	TOK_OEM,			actPush,	jSetupDoc,		nOS				},
	{ jReadFileList,	nReadFileListName,	TOK_RTL,			actPush,	jSetupDoc,		nOS				},
	{ jReadFileList,	nReadFileListName,	TOK_JPN,			actPush,	jSetupDoc,		nOS				},
	{ jReadFileList,	nReadFileListName,	TOK_GER,			actPush,	jSetupDoc,		nOS				},
	{ jReadFileList,	nReadFileListName,	TOK_FRA,			actPush,	jSetupDoc,		nOS				},
	{ jReadFileList,	nReadFileListName,	TOK_SPA,			actPush,	jSetupDoc,		nOS				},
	{ jReadFileList,	nReadFileListName,	TOK_USA,			actPush,	jSetupDoc,		nOS				},
	{ jReadFileList,	nReadFileListName,	TOK_APP1,			actPush,	jSetupDoc,		nOS				},
	{ jReadFileList,	nReadFileListName,	TOK_APP2,			actPush,	jSetupDoc,		nOS				},
	{ jReadFileList,	nReadFileListName,	TOK_APP3,			actPush,	jSetupDoc,		nOS				},
	{ jReadFileList,	nReadFileListName,	DEFAULT_ACTION,		actPop,		jDontCare,		nDontCare		},

	{ jInstFont,		nStart,				TOK_SYSTEM,			actJump,	jInstFont,		nDestDir		},
	{ jInstFont,		nDestDir,			TOK_CHECKVER,		actPush,	jInstFont,		nSysFile		},
	{ jInstFont,		nDestDir,			TOK_SHARED,			actPush,	jInstFont,		nSharedFile		},
	{ jInstFont,		nDestDir,			TOK_STRING,			actJump,	jInstFont,		nDestName		},
	{ jInstFont,		nSysFile,			DEFAULT_ACTION,		actPop,		jDontCare,		nDontCare		},
	{ jInstFont,		nSharedFile,		DEFAULT_ACTION,		actPop,		jDontCare,		nDontCare		},
	{ jInstFont,		nDestName,			TOK_STRING,			actJump,	jInstFont,		nSrcName		},
	{ jInstFont,		nSrcName,			TOK_WIN95,			actPush,	jSetupDoc,		nOS				},
	{ jInstFont,		nSrcName,			TOK_WIN98,			actPush,	jSetupDoc,		nOS				},
	{ jInstFont,		nSrcName,			TOK_NT40,			actPush,	jSetupDoc,		nOS				},
	{ jInstFont,		nSrcName,			TOK_NT50,			actPush,	jSetupDoc,		nOS				},
	{ jInstFont,		nSrcName,			TOK_ALLWIN,			actPush,	jSetupDoc,		nOS				},
	{ jInstFont,		nSrcName,			TOK_ALLNT,			actPush,	jSetupDoc,		nOS				},
	{ jInstFont,		nSrcName,			TOK_DBCS,			actPush,	jSetupDoc,		nOS				},
	{ jInstFont,		nSrcName,			TOK_OEM,			actPush,	jSetupDoc,		nOS				},
	{ jInstFont,		nSrcName,			TOK_RTL,			actPush,	jSetupDoc,		nOS				},
	{ jInstFont,		nSrcName,			TOK_JPN,			actPush,	jSetupDoc,		nOS				},
	{ jInstFont,		nSrcName,			TOK_GER,			actPush,	jSetupDoc,		nOS				},
	{ jInstFont,		nSrcName,			TOK_FRA,			actPush,	jSetupDoc,		nOS				},
	{ jInstFont,		nSrcName,			TOK_SPA,			actPush,	jSetupDoc,		nOS				},
	{ jInstFont,		nSrcName,			TOK_USA,			actPush,	jSetupDoc,		nOS				},
	{ jInstFont,		nSrcName,			TOK_APP1,			actPush,	jSetupDoc,		nOS				},
	{ jInstFont,		nSrcName,			TOK_APP2,			actPush,	jSetupDoc,		nOS				},
	{ jInstFont,		nSrcName,			TOK_APP3,			actPush,	jSetupDoc,		nOS				},
	{ jInstFont,		nSrcName,			DEFAULT_ACTION,		actPop,		jDontCare,		nDontCare		},
    //marker for end of array
	{ jDontCare,		nDontCare,			DEFAULT_ACTION,		actError,	jDontCare,		nDontCare		}
};


/////////////////////////////////////////////////////////////////////////////
// CSetupDoc

CSetupDoc::CSetupDoc()
{
    m_papCommands = NULL;
    m_cCommands = 0;
    m_maxCommands = 0;
	m_iCommandPtr = 0;
}

CSetupDoc::~CSetupDoc()
{
    int i;

    if( m_papCommands )
    {
        for( i = 0; i < m_cCommands; i++ )
        {
            delete m_papCommands[i];
        }

        free( m_papCommands );
    }
}

int	CSetupDoc::GetNumValidCommands ()
{
	int i, count = 0;
	
	for (i=0; i< m_cCommands; i++)
	{
		if (m_papCommands[i]->IsValidToken())
			count++;
	}
	return count;
}


// 
// Prepstub98 only function
//
void CSetupDoc::ReNumberCommands()
{
	CCommand *pCmd;
	int i, j = 1;

	for(i=0; i < GetNumCommands(); i++ )
    {
		pCmd = GetNthCommand(i);
		if (pCmd->IsValidToken())
			pCmd->SetCommandID (j++);
		else
			pCmd->SetCommandID (-1);
    }
}


int CSetupDoc::FindFirstToken (ETOKEN eToken)
{
	for(int i=0; i < GetNumCommands(); i++ )
    {
		if (GetNthCommand(i)->GetCommandType() == eToken)
			return i;
	}
	return -1;
}


int CSetupDoc::FindFirstToken (ETOKEN eToken, int iStart, int iEnd)
{
	for(int i=iStart; i < iEnd; i++ )
    {
		if (GetNthCommand(i)->GetCommandType() == eToken)
			return i;
	}
	return -1;
}


bool CSetupDoc::GetFileVersionInfo(const char *szFilePath)
{
	ETOKEN eToken;
	CCommand *pCmd;
	int i;
	char szPath[_MAX_PATH];
	char szDiskId[12];

	for(i=0; i < GetNumCommands(); i++ )
    {
		#ifdef EBUPREPSTUB
			UpdateDisplay();
		#else
			EbuYield();
		#endif

		pCmd = GetNthCommand(i);

		eToken = pCmd->GetCommandType();

		if (eToken == TOK_INSTALLLIST)
		{
			lstrcpy( szPath, szFilePath );
			//
			// Only look in sub-directories when told to do so by the prepstub user.
			// Otherwise process as before using the stubpath 

			//
			{
				if (g_bDiskPaths)
				{
					if (pCmd->GetDiskId() != DISK_NOT_SPECIFIED && pCmd->GetDiskId() != g_nCurrentDiskID)
					{
						g_nCurrentDiskID = pCmd->GetDiskId();
					}
					// Using alternate paths
					lstrcat( szPath, CharNext(&(DISKPATH)));
					lstrcat( szPath, itoa((g_nCurrentDiskID + 1), szDiskId, 10));
					lstrcat(szPath, "\\");
				}
				if( !pCmd->GetSourceVersionInfo( szPath ) )
				{
					return false;
				}
			}
		}
    }
	return true;
}

CCommand* CSetupDoc::AddCommand()
{
    //see if we need to expand the array
    if( m_cCommands == m_maxCommands )
    {
        m_maxCommands += ALLOC_COMMANDS;

        if( m_papCommands )
        {
            m_papCommands = (CCommand**)realloc( m_papCommands, sizeof(CCommand*) * m_maxCommands );
        }
        else
        {
            m_papCommands = (CCommand**)malloc( sizeof(CCommand*) * m_maxCommands );
        }
    }

    m_papCommands[m_cCommands] = new CCommand;

    return m_papCommands[m_cCommands++];
}

void CSetupDoc::AttachCommand(CCommand *pCmd)
{
	// Same as AddCommand, but adds a provided CCommand to the list
	// instead of creating a new.

    //see if we need to expand the array
    if( m_cCommands == m_maxCommands )
    {
        m_maxCommands += ALLOC_COMMANDS;

        if( m_papCommands )
        {
            m_papCommands = (CCommand**)realloc( m_papCommands, sizeof(CCommand*) * m_maxCommands );
        }
        else
        {
            m_papCommands = (CCommand**)malloc( sizeof(CCommand*) * m_maxCommands );
        }
    }

    m_papCommands[m_cCommands++] = pCmd;
}

void CSetupDoc::InsertCommand(CCommand *pCmd)
{
    //see if we need to expand the array
    if( m_cCommands == m_maxCommands )
    {
        m_maxCommands += ALLOC_COMMANDS;

        if( m_papCommands )
        {
            m_papCommands = (CCommand**)realloc( m_papCommands, sizeof(CCommand*) * m_maxCommands );
        }
        else
        {
            m_papCommands = (CCommand**)malloc( sizeof(CCommand*) * m_maxCommands );
        }
    }

	// shift all commands after insertion point forward one
	for (int i=m_cCommands-1;i>=m_iCommandPtr;i--)
	{
		m_papCommands[i+1] = m_papCommands[i];
	}

	// insert the new command
	m_papCommands[m_iCommandPtr] = pCmd;

	m_iCommandPtr++;
	m_cCommands++;
}

bool CSetupDoc::DeleteCommand ()
{
	if (m_cCommands == 0 || m_iCommandPtr < 0 || m_iCommandPtr > m_maxCommands-1)
	{
		return false;
	}

	if (!m_papCommands[m_iCommandPtr])
	{
		return false;
	}
	else
	{
		delete (m_papCommands[m_iCommandPtr]);
	}
	m_papCommands[m_iCommandPtr] = NULL;


	// shift all commands back one
	for (int i=m_iCommandPtr;i<m_cCommands-1;i++)
	{
		m_papCommands[i] = m_papCommands[i+1];
	}

	m_cCommands--;
	return true;
}


BOOL CSetupDoc::PrepareStub( const char* pszInput, const char* pszOutput, LPCSTR lpszStubPath )
{
    LPSTR pch;
    LPSTR pBackslash = NULL;

    // Enable the excel workaround-hack which converts triple quotes into single quotes
    // and ignores single quotes.  -ks 2/2/95
    EnableExcelWorkaround();

    //Ensure that the stub name is terminated with a backslash.
    for( pch = (LPSTR)lpszStubPath; *pch; pch = AnsiNext(pch) )
    {
        if( *pch == '\\' )
        {
            pBackslash = pch;
        }
    }
    if( pBackslash+1 != pch )
    {
        *pch = '\\';
        pch = AnsiNext(pch);
        *pch = '\0';
    }

    m_lpszStubPath = lpszStubPath;

    if( OpenFile( pszInput ) )
    {
		// renumber the commands to filter out non essential tokens
		ReNumberCommands();

		// get file version info for commands that install files / fonts
		if (!GetFileVersionInfo(GetStubPath()))
		{
			return false;
		}

        return Write( pszOutput );
    }
    else
    {
        return false;
    }
}

BOOL CSetupDoc::Write( const char* pszOutput )
{
    HFILE hFile;
    int i;
    BOOL fResult = TRUE;
    char szBuf[255];

    hFile = _lcreat( pszOutput, 0 );

    if( HFILE_ERROR == hFile )
    {
        Alert( g_hwnd, MB_ICONSTOP | MB_OK, STR_CANTCREATEFILE, (LPCSTR)pszOutput );
        return FALSE;
    }
		 // number of commands
		 *(WORD *)szBuf = (WORD)GetNumValidCommands();
		 _lwrite( hFile, (LPCSTR)szBuf, sizeof(WORD) );
		 // first command res id (for compatibility with object layout, not used in BLOB's
		 *(WORD *)szBuf = 0;
		 _lwrite( hFile, (LPCSTR)szBuf, sizeof(WORD) );

    for( i = 0; i < GetNumCommands(); i++ )
    {
		#ifdef EBUPREPSTUB
			UpdateDisplay();
		#else
			EbuYield();
		#endif

		// Write any command with a valid Command Token.
//		if (GetNthCommand(i)->GetCommandID() != -1)
		if (GetNthCommand(i)->IsValidToken() )
		{
	        fResult = GetNthCommand(i)->BinaryWrite( hFile);

			if( !fResult )
				break;
		}
    }

    _lclose( hFile );

    return fResult;
}

//
// WriteTextFile is a Prepstub98 only function.
// although not called by Prepstub, it should remain here because this is
// a shared file.
//
BOOL CSetupDoc::WriteTextFile( const char* pszOutput )
{
    HFILE hFile;
    int i;
    BOOL fResult = TRUE;

    hFile = _lcreat( pszOutput, 0 );

    if( HFILE_ERROR == hFile )
    {
        Alert( g_hwnd, MB_ICONSTOP | MB_OK, STR_CANTCREATEFILE, (LPCSTR)pszOutput );
        return FALSE;
    }

    for( i = 0; i < GetNumCommands(); i++ )
    {
		#ifdef EBUPREPSTUB
			UpdateDisplay();
		#else
			EbuYield();
		#endif

        fResult = GetNthCommand(i)->TextWrite (hFile);

		if( !fResult )
			break;
    }

    _lclose( hFile );

    return fResult;
}


//****************************************************************************
// Class        CSetupDoc
//
// Procedure    OnAddToken
//
// Purpose      This is the callback from CTextDocument for every token in
//              the file which is being read in.  This routine handles the
//              ignoring of comments and then calls down to OnStateChanging
//              so that we can store some information while we walk through
//              the parse tree.
//
// Parameters   token       The current token from the input file
//
// Returns      zero to abort, nonzero to continue
//
// History       2/02/95    KenSh       Adapted from CBuildDoc in Author
//
BOOL CSetupDoc::OnAddToken( const CToken& Token )
{
    static BOOL         fSkippingComment;
    static CStateStack  StateStack;
    static EMAJOR       stateMajor;
    static EMINOR       stateMinor;
    EMAJOR              stateNextMajor; //what major state to jump to
    EMINOR              stateNextMinor; //what minor state to jump to
    EACTION             actionNext;     //what kind of transition to make
    int i;

    //unwrap the wrapped token
    ETOKEN eToken = Token.eToken;
    LPCSTR pszValue = Token.pszValue;

    //Initialization
    if( eToken == TOK_BEGIN )
    {
        fSkippingComment = FALSE;
        m_nLine = 1;
        StateStack.Empty();
        stateMajor = jSetupDoc;
        stateMinor = nStart;
        return TRUE;
    }
    else if( eToken == TOK_EOF )
    {
        return TRUE;
    }
    else if( eToken == TOK_NEWLINE )
    {
        m_nLine++;

		// add the token to the command list so we can persist cr/lf
        CCommand* pCommand = AddCommand();
        pCommand->SetCommandID( -2 );
		pCommand->SetCommandType (TOK_NEWLINE);
    }

//we jump back up here when a pop action happens.
BeginSearch:

    //Default action is error+pop.
    actionNext = actError;
    stateNextMajor = jDontCare;
    stateNextMinor = nDontCare;

    //Find combination of state and keyword in the "expected" table
    for( i = 0; SetupExpected[i].majFrom != jDontCare; i++ )
    {
        if( SetupExpected[i].majFrom == stateMajor &&
            SetupExpected[i].minFrom == stateMinor )
        {
            if( SetupExpected[i].eToken == eToken )
                //Found what we're looking for; mark it down and exit the loop.
            {
                //Mark down the action and the next state
                actionNext = SetupExpected[i].eAction;
                stateNextMajor = SetupExpected[i].majTo;
                stateNextMinor = SetupExpected[i].minTo;

                //break out of the loop
                break;
            }
            else if( SetupExpected[i].eToken == DEFAULT_ACTION )
                //An exact match hasn't yet been found; mark down the default action.
            {
                actionNext = SetupExpected[i].eAction;
                stateNextMajor = SetupExpected[i].majTo;
                stateNextMinor = SetupExpected[i].minTo;
            }
        }
    }

    // We ignore whitespace and commas unless the state transition was explicitly
    // specified (i.e. we aren't using the DEFAULT_ACTION).
    if( SetupExpected[i].majFrom != jDontCare ||        //not at end of list
        (eToken != TOK_WHITESPACE && eToken != TOK_NEWLINE && eToken != TOK_COMMA) )
    {
        //perform the action
        if( actionNext == actPush )
        {
            StateStack.Push( stateMajor, stateMinor );
            stateMajor = stateNextMajor;
            stateMinor = stateNextMinor;
        }
        else if( actionNext == actPop )
        {
            //Switch to the previously pushed state
            if( !StateStack.Pop( &stateMajor, &stateMinor ) )
            {
                //couldn't pop the stack; abort.
                Alert( g_hwnd, MB_ICONSTOP | MB_OK, STR_ILLEGALTOKEN,
                                (LPCSTR)pszValue, m_nLine );
                return FALSE;
            }
            else
            {
                //look through list again in new state.  If we didn't want to do
                //a goto we could recursively call this function, but why?
                goto BeginSearch;
            }
        }
        else if( actionNext == actJump )
        {
            stateMajor = stateNextMajor;
            stateMinor = stateNextMinor;
        }
        else if( actionNext == actError )
        {
            Alert( g_hwnd, MB_ICONSTOP | MB_OK, STR_ILLEGALTOKEN,
                    (LPCSTR)pszValue, m_nLine );
            return FALSE;
        }
        else
        {
            ASSERT( actionNext == actNOP );
            //nothing to do.
        }

        //Call the compiler callback.  Notice that we don't get here
        //on a Pop action, which is fine.
        if( !OnStateChanging( stateMajor, stateMinor, eToken, pszValue ) )
        {
            return FALSE;
        }
    }

    return TRUE;
}


// eMajor, eMinor are the state we're changing *to*.
BOOL CSetupDoc::OnStateChanging( EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue )
{
    switch( eMajor )
    {
        case jSetupDoc:
            return OnSetupDocToken( eMajor, eMinor, eToken, pszValue );

//        case jInstall:
//            return OnSetupInstallToken( eMajor, eMinor, eToken, pszValue );

        case jMkDir:
            return OnSetupMkDirToken( eMajor, eMinor, eToken, pszValue );

        case jMkRoot:
            return OnSetupMkRootToken( eMajor, eMinor, eToken, pszValue );

        case jGetName:
            return OnSetupGetNameToken( eMajor, eMinor, eToken, pszValue );

        case jGetPID:
            return OnSetupGetPIDToken( eMajor, eMinor, eToken, pszValue );

        case jInstDX:
            return OnSetupInstDXToken( eMajor, eMinor, eToken, pszValue );

        case jInstDPLAY:
            return OnSetupInstDPLAYToken( eMajor, eMinor, eToken, pszValue );

        case jInstIcon:
            return OnSetupInstIcon( eMajor, eMinor, eToken, pszValue );

        case jCDSpeed:
            return OnSetupCDSpeed( eMajor, eMinor, eToken, pszValue );

        case jIniValue:
            return OnSetupIniValueToken( eMajor, eMinor, eToken, pszValue );

        case jInstFont:
            return OnSetupInstFontToken( eMajor, eMinor, eToken, pszValue );

        case jInstallList:
            return OnSetupInstallList( eMajor, eMinor, eToken, pszValue );

        case jRegWiz:
            return OnSetupRegWiz( eMajor, eMinor, eToken, pszValue );

		case jShellExecute:
			return OnSetupShellExecute( eMajor, eMinor, eToken, pszValue );

		case jCabGo:
        case jInstallGo:
            return OnSetupInstallGo( eMajor, eMinor, eToken, pszValue );

        case jDeleteFile:
            return OnSetupDeleteFile( eMajor, eMinor, eToken, pszValue );

        case jGetGroup:
            return OnSetupGetGroupToken( eMajor, eMinor, eToken, pszValue );

        case jReadFileList:
            return OnSetupReadFileList( eMajor, eMinor, eToken, pszValue );

		case jBeginFileList:
			return OnSetupBeginFileList( eMajor, eMinor, eToken, pszValue ); 

		case jEndFileList:
			return OnSetupEndFileList( eMajor, eMinor, eToken, pszValue ); 

		case jBeginStaticStringList:
			return OnSetupBeginStaticStrings ( eMajor, eMinor, eToken, pszValue ); 

		case jEndStaticStringList:
			return OnSetupEndStaticStrings ( eMajor, eMinor, eToken, pszValue ); 

		case jProperty:
			return OnSetupPropertyToken (eMajor, eMinor, eToken, pszValue);

		case jComment:
			return OnSetupCommentToken (eMajor, eMinor, eToken, pszValue );

		case jRule:
			return OnSetupRuleToken	(eMajor, eMinor, eToken, pszValue);

		case jString:
			return OnSetupStringToken (eMajor, eMinor, eToken, pszValue);

		case jBeginStringList:
			return OnSetupBeginStringList (eMajor, eMinor, eToken, pszValue);

		case jEndStringList:
			return OnSetupEndStringList (eMajor, eMinor, eToken, pszValue);

		case jAction:
			return OnSetupActionToken (eMajor, eMinor, eToken, pszValue);

        default:
            ASSERT( FALSE );
            return FALSE;
    }
}


//****************************************************************************
// Class        CSetupDoc
//
// Procedure    OnSetupDocToken
//
// Purpose      The "top-level" token handler - handles states whose major
//              component is jSetupDoc.
//
// Parameters   eMajor      the major half of the compiler state (jSetupDoc)
//              eMinor      the minor half of the compiler state
//              eToken      the token that caused us to be here.
//              pszValue    string represented by the token
//
// Returns      zero to abort, nonzero to continue
//
// History       2/02/95    KenSh       Created
//
BOOL CSetupDoc::OnSetupDocToken( EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue )
{
    ASSERT( eMajor == jSetupDoc );

    switch( eMinor )
    {
        case nStart:
            break;

		case nInstall:
		{
            CCommand* pCommand = GetLastCommand();
            switch( eToken )
            {
				case TOK_SYSTEM:
					pCommand->SetSystemDirFlag();
					break;
				case TOK_CHECKVER:
					pCommand->SetSysFileFlag();
					break;
				case TOK_APP:
					pCommand->SetAppDirFlag();
					break;
				case TOK_WINDOWS:
					pCommand->SetWindowsDirFlag();
					break;
				case TOK_UNINSTALL:
					pCommand->SetUninstallFileFlag();
					break;
				case TOK_UNINSTALLALL:
					pCommand->SetUninstallAllFlag();
					break;
				case TOK_CAB:
					pCommand->SetCabFlag();
					break;
				case TOK_SHARED:
					pCommand->SetSharedFileFlag();
					break;
				case TOK_DLLREGISTER:
					pCommand->SetDLLRegisterFlag();
					break;
                default:
                    ASSERT(FALSE);
			}
			break;
		}

        case nOS:
        {
            CCommand* pCommand = GetLastCommand();
            switch( eToken )
            {
                case TOK_WIN95:
                    pCommand->SetWin95Flag();
                    break;

                case TOK_WIN98:
                    pCommand->SetWin98Flag();
                    break;

                case TOK_ALLWIN:
                    pCommand->SetWin95Flag();
                    pCommand->SetWin98Flag();
                    break;

                case TOK_NT40:
                    pCommand->SetNT40Flag();
                    break;

                case TOK_NT50:
                    pCommand->SetNT50Flag();
                    break;

				case TOK_ALLNT:
					pCommand->SetNT40Flag();
                    pCommand->SetNT50Flag();
                    break;

				case TOK_DBCS:
					pCommand->SetDBCSFlag();
					break;

				case TOK_OEM:
					pCommand->SetOEMFlag();
					break;

				case TOK_RTL:
					pCommand->SetRTLFlag();
					break;

				case TOK_JPN:
					pCommand->SetJPNFlag();
					break;

				case TOK_GER:
					pCommand->SetGERFlag();
					break;

				case TOK_FRA:
					pCommand->SetFRAFlag();
					break;

				case TOK_SPA:
					pCommand->SetSPAFlag();
					break;

				case TOK_USA:
					pCommand->SetUSAFlag();
					break;

				case TOK_APP1:
					pCommand->SetAPP1Flag();
					break;

				case TOK_APP2:
					pCommand->SetAPP2Flag();
					break;

				case TOK_APP3:
					pCommand->SetAPP3Flag();
					break;

                case TOK_IMEENABLE:
                    pCommand->SetIMEENABLEFlag();
                    break;

                case TOK_IMEON:
                    pCommand->SetIMEONFlag();
                    break;

				case TOK_PRECOPY:
					pCommand->SetCabPreCopy (true);
					break;

                default:
                    ASSERT(FALSE);
            }
            break;
        }

        default:
            ASSERT(FALSE);
            return FALSE;
    }

    return TRUE;
}


BOOL CSetupDoc::OnSetupInstallList( EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue )
{
    ASSERT( eMajor == jInstallList );

	CCommand* pCommand;
	if ( GetNumCommands() == 0 )
	{
        pCommand = AddCommand();
        pCommand->SetCommandID( m_nLine );
	}
	else
	{
		pCommand = GetLastCommand();
		if ( pCommand->GetCommandID() != m_nLine )
		{
	        pCommand = AddCommand();
		    pCommand->SetCommandID( m_nLine );
		}
	}

    switch( eMinor )
    {
        case nStart:
            pCommand->SetCommandType( TOK_INSTALLLIST );
            break;

        case nUninstall:
            pCommand->SetUninstallFileFlag();
            break;

		case nCab:
            pCommand->SetCabFlag();
            break;

		case nFont:
			pCommand->SetFontFlag();
			break;

		case nUninstOnly:
			pCommand->SetUninstOnlyFlag();
			break;

		case nSharedFile:
			pCommand->SetSharedFlag();
			break;

		case nDLLRegister:
			pCommand->SetDLLRegisterFlag();
			break;

		case nDestName:
            pCommand->SetDestName( pszValue );
			if ( pCommand->GetFontFlag() )
			{
				// check for .ttf and fail if not found
				int	nLen = lstrlen ( pszValue ) - 4;
				if ( lstrcmpi( &pszValue[nLen], ".ttf" ) || pCommand->GetSharedFlag() ||
					pCommand->GetDLLRegisterFlag())
					return false;
			}
            break;

        case nSrcName:
            pCommand->SetSourceName( pszValue );
			if ( pCommand->GetFontFlag() )
			{
				// check for .ttf and fail if not found
				int	nLen = lstrlen ( pszValue ) - 4;
				if ( lstrcmpi( &pszValue[nLen], ".ttf" ) || pCommand->GetSharedFlag() ||
					pCommand->GetDLLRegisterFlag())
					return false;
			}

			#ifdef EBUPREPSTUB
				UpdateDisplay();
			#else
				EbuYield();
			#endif

            break;

        case nSysDir:
            pCommand->SetSystemDirFlag();
            break;

        case nWinDir:
            pCommand->SetWindowsDirFlag();
            break;

        case nAppDir:
            pCommand->SetAppDirFlag();
            break;

		case nDestDir:	// we get here for the first flag token - wierd parser behaviour
			if ( eToken == TOK_FONT )
				pCommand->SetFontFlag();
			else if ( eToken == TOK_SHARED )
				pCommand->SetSharedFlag();
			else if ( eToken == TOK_DLLREGISTER )
				pCommand->SetDLLRegisterFlag();
			else if ( eToken == TOK_UNINSTALL )
				pCommand->SetUninstallFileFlag();
			else if ( eToken == TOK_UNINSTALLONLY )
				pCommand->SetUninstOnlyFlag();
			else if ( eToken == TOK_CAB )
				pCommand->SetCabFlag();
			else
				return FALSE;
			break;
// not sure how we would get here.

		case nGroup:
			{
			  __int64 group=0;
			  if(*pszValue == '0' && (pszValue[1] == 'x' || pszValue[1] == 'X'))
			     sscanf(&pszValue[2],"%I64X",&group);
			  else
				  sscanf(pszValue,"%I64d",&group);

			  pCommand->SetGroup(group);
			}
			break;
		case nDiskId:
			{
				return (DetermineDiskToken(eToken, pCommand));
			}
			break;
		case nOS:
			return OnSetupDocToken(jSetupDoc, eMinor, eToken, pszValue);

        default:
            ASSERT(FALSE);
            return FALSE;
    }

    return TRUE;
}

BOOL CSetupDoc::OnSetupInstallGo( EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue )
{
    ASSERT( eMajor == jInstallGo || eMajor == jCabGo);

	CCommand* pCommand;
	if ( GetNumCommands() == 0 )
	{
        pCommand = AddCommand();
        pCommand->SetCommandID( m_nLine );
	}
	else
	{
		pCommand = GetLastCommand();
		if ( pCommand->GetCommandID() != m_nLine )
		{
	        pCommand = AddCommand();
		    pCommand->SetCommandID( m_nLine );
		}
	}

    switch( eMinor )
    {
		case nStart:
			if(eMajor == jInstallGo)
                pCommand->SetCommandType( TOK_INSTALLGO );
			else
                pCommand->SetCommandType( TOK_CABGO );

            break;
        case nOS:
        {
            switch( eToken )
            {
                case TOK_WIN95:
                    pCommand->SetWin95Flag();
                    break;

                case TOK_WIN98:
                    pCommand->SetWin98Flag();
                    break;

                case TOK_ALLWIN:
                    pCommand->SetWin95Flag();
                    pCommand->SetWin98Flag();
                    break;

                case TOK_NT40:
                    pCommand->SetNT40Flag();
                    break;

                case TOK_NT50:
                    pCommand->SetNT50Flag();
                    break;

				case TOK_ALLNT:
					pCommand->SetNT40Flag();
                    pCommand->SetNT50Flag();
                    break;

				case TOK_DBCS:
					pCommand->SetDBCSFlag();
					break;

				case TOK_OEM:
					pCommand->SetOEMFlag();
					break;

				case TOK_RTL:
					pCommand->SetRTLFlag();
					break;

				case TOK_JPN:
					pCommand->SetJPNFlag();
					break;

				case TOK_GER:
					pCommand->SetGERFlag();
					break;

				case TOK_FRA:
					pCommand->SetFRAFlag();
					break;

				case TOK_SPA:
					pCommand->SetSPAFlag();
					break;

				case TOK_USA:
					pCommand->SetUSAFlag();
					break;

				case TOK_APP1:
					pCommand->SetAPP1Flag();
					break;

				case TOK_APP2:
					pCommand->SetAPP2Flag();
					break;

				case TOK_APP3:
					pCommand->SetAPP3Flag();
					break;

                case TOK_IMEENABLE:
                    pCommand->SetIMEENABLEFlag();
                    break;

                case TOK_IMEON:
                    pCommand->SetIMEONFlag();
                    break;

                default:
                    ASSERT(FALSE);
            }
            break;
        }
		break;
//		case nCabName:
//			pCommand->SetCabName(pszValue);
//			break;

		case nCabGo:
			pCommand->SetCabName(pszValue);
			break;

        default:
            ASSERT(FALSE);
            return FALSE;
    }

    return TRUE;
}


BOOL CSetupDoc::OnSetupRegWiz( EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue )
{
    ASSERT( eMajor == jRegWiz );

	CCommand* pCommand;
	if ( GetNumCommands() == 0 )
	{
        pCommand = AddCommand();
        pCommand->SetCommandID( m_nLine );
	}
	else
	{
		pCommand = GetLastCommand();
		if ( pCommand->GetCommandID() != m_nLine )
		{
	        pCommand = AddCommand();
		    pCommand->SetCommandID( m_nLine );
		}
	}

    switch( eMinor )
    {
        case nStart:
            pCommand->SetCommandType( TOK_REGWIZ );
            break;
        case nOS:
        {
            switch( eToken )
            {
                case TOK_WIN95:
                    pCommand->SetWin95Flag();
                    break;

                case TOK_WIN98:
                    pCommand->SetWin98Flag();
                    break;

                case TOK_ALLWIN:
                    pCommand->SetWin95Flag();
                    pCommand->SetWin98Flag();
                    break;

                case TOK_NT40:
                    pCommand->SetNT40Flag();
                    break;

                case TOK_NT50:
                    pCommand->SetNT50Flag();
                    break;

				case TOK_ALLNT:
					pCommand->SetNT40Flag();
                    pCommand->SetNT50Flag();
                    break;

				case TOK_DBCS:
					pCommand->SetDBCSFlag();
					break;

				case TOK_OEM:
					pCommand->SetOEMFlag();
					break;

				case TOK_RTL:
					pCommand->SetRTLFlag();
					break;

				case TOK_JPN:
					pCommand->SetJPNFlag();
					break;

				case TOK_GER:
					pCommand->SetGERFlag();
					break;

				case TOK_FRA:
					pCommand->SetFRAFlag();
					break;

				case TOK_SPA:
					pCommand->SetSPAFlag();
					break;

				case TOK_USA:
					pCommand->SetUSAFlag();
					break;

				case TOK_APP1:
					pCommand->SetAPP1Flag();
					break;

				case TOK_APP2:
					pCommand->SetAPP2Flag();
					break;

				case TOK_APP3:
					pCommand->SetAPP3Flag();
					break;

                case TOK_IMEENABLE:
                    pCommand->SetIMEENABLEFlag();
                    break;

                case TOK_IMEON:
                    pCommand->SetIMEONFlag();
                    break;

                default:
                    ASSERT(FALSE);
            }
            break;
        }
		break;
		case nRegWiz:
			pCommand->SetRegWizRegName(pszValue);
			break;

        default:
            ASSERT(FALSE);
            return FALSE;
    }

    return TRUE;
}

BOOL CSetupDoc::OnSetupShellExecute( EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue )
{
    ASSERT( eMajor == jShellExecute );

	CCommand* pCommand;
	if ( GetNumCommands() == 0 )
	{
        pCommand = AddCommand();
        pCommand->SetCommandID( m_nLine );
	}
	else
	{
		pCommand = GetLastCommand();
		if ( pCommand->GetCommandID() != m_nLine )
		{
	        pCommand = AddCommand();
		    pCommand->SetCommandID( m_nLine );
		}
	}

    switch( eMinor )
    {
        case nStart:
            pCommand->SetCommandType( TOK_SHELLEXECUTE );
            break;

        case nOS:
        {
            switch( eToken )
            {
                case TOK_WIN95:
                    pCommand->SetWin95Flag();
                    break;

                case TOK_WIN98:
                    pCommand->SetWin98Flag();
                    break;

                case TOK_ALLWIN:
                    pCommand->SetWin95Flag();
                    pCommand->SetWin98Flag();
                    break;

                case TOK_NT40:
                    pCommand->SetNT40Flag();
                    break;

                case TOK_NT50:
                    pCommand->SetNT50Flag();
                    break;

				case TOK_ALLNT:
					pCommand->SetNT40Flag();
                    pCommand->SetNT50Flag();
                    break;

				case TOK_DBCS:
					pCommand->SetDBCSFlag();
					break;

				case TOK_OEM:
					pCommand->SetOEMFlag();
					break;

				case TOK_RTL:
					pCommand->SetRTLFlag();
					break;

				case TOK_JPN:
					pCommand->SetJPNFlag();
					break;

				case TOK_GER:
					pCommand->SetGERFlag();
					break;

				case TOK_FRA:
					pCommand->SetFRAFlag();
					break;

				case TOK_SPA:
					pCommand->SetSPAFlag();
					break;

				case TOK_USA:
					pCommand->SetUSAFlag();
					break;

				case TOK_APP1:
					pCommand->SetAPP1Flag();
					break;

				case TOK_APP2:
					pCommand->SetAPP2Flag();
					break;

				case TOK_APP3:
					pCommand->SetAPP3Flag();
					break;

                case TOK_IMEENABLE:
                    pCommand->SetIMEENABLEFlag();
                    break;

                case TOK_IMEON:
                    pCommand->SetIMEONFlag();
                    break;

                default:
                    ASSERT(FALSE);
            }
            break;
        }

		break;

		case nFilename:
			pCommand->SetShellExecuteFileName(pszValue);
			break;

		case nDirectory:
			pCommand->SetShellExecuteDirectory(pszValue);
			break;

		case nParameters:
			pCommand->SetShellExecuteParameters(pszValue);
			break;

		case nShow:
			pCommand->SetShellExecuteShowFlag(atoi(pszValue));
			break;

		case nWait:
			pCommand->SetShellExecuteWaitFlag();
			break;

		case nUninstall:
            pCommand->SetUninstallFileFlag();
			break;

		case nGroup:
			{
			  __int64 group;
			  if(*pszValue == '0' && (pszValue[1] == 'x' || pszValue[1] == 'X'))
			     sscanf(&pszValue[2],"%I64X",&group);
			  else
				  sscanf(pszValue,"%I64d",&group);

			  pCommand->SetGroup(group);
			}
			break;

        default:
            ASSERT(FALSE);
            return FALSE;
    }

    return TRUE;
}

BOOL CSetupDoc::OnSetupReadFileList( EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue )
{
    ASSERT( eMajor == jReadFileList );

	CCommand* pCommand;
	if ( GetNumCommands() == 0 )
	{
        pCommand = AddCommand();
        pCommand->SetCommandID( m_nLine );
	}
	else
	{
		pCommand = GetLastCommand();
		if ( pCommand->GetCommandID() != m_nLine )
		{
	        pCommand = AddCommand();
		    pCommand->SetCommandID( m_nLine );
		}
	}

    switch( eMinor )
    {
        case nStart:
            pCommand->SetCommandType( TOK_READFILELIST );
            break;
        case nOS:
        {
            switch( eToken )
            {
                case TOK_WIN95:
                    pCommand->SetWin95Flag();
                    break;

                case TOK_WIN98:
                    pCommand->SetWin98Flag();
                    break;

                case TOK_ALLWIN:
                    pCommand->SetWin95Flag();
                    pCommand->SetWin98Flag();
                    break;

                case TOK_NT40:
                    pCommand->SetNT40Flag();
                    break;

                case TOK_NT50:
                    pCommand->SetNT50Flag();
                    break;

				case TOK_ALLNT:
					pCommand->SetNT40Flag();
                    pCommand->SetNT50Flag();
                    break;

				case TOK_DBCS:
					pCommand->SetDBCSFlag();
					break;

				case TOK_OEM:
					pCommand->SetOEMFlag();
					break;

				case TOK_RTL:
					pCommand->SetRTLFlag();
					break;

				case TOK_JPN:
					pCommand->SetJPNFlag();
					break;

				case TOK_GER:
					pCommand->SetGERFlag();
					break;

				case TOK_FRA:
					pCommand->SetFRAFlag();
					break;

				case TOK_SPA:
					pCommand->SetSPAFlag();
					break;

				case TOK_USA:
					pCommand->SetUSAFlag();
					break;

				case TOK_APP1:
					pCommand->SetAPP1Flag();
					break;

				case TOK_APP2:
					pCommand->SetAPP2Flag();
					break;

				case TOK_APP3:
					pCommand->SetAPP3Flag();
					break;

                case TOK_IMEENABLE:
                    pCommand->SetIMEENABLEFlag();
                    break;

                case TOK_IMEON:
                    pCommand->SetIMEONFlag();
                    break;

                default:
                    ASSERT(FALSE);
            }
            break;
        }
		break;
		case nReadFileListName:
			pCommand->SetReadFileListName(pszValue);
			break;

        default:
            ASSERT(FALSE);
            return FALSE;
    }

    return TRUE;
}




//****************************************************************************
// Class        CSetupDoc
//
// Procedure    OnSetupMkDirToken
//
// Purpose      Builds an MKDIR command based on the tokens we receive.
//
// Parameters   eMajor      the major half of the compiler state (jInstall)
//              eMinor      the minor half of the compiler state
//              eToken      the token that caused us to be here.
//              pszValue    string represented by the token
//
// Returns      zero to abort, nonzero to continue
//
// History       2/02/95    KenSh       Created
//               6/21/95    KenSh       Added UININSTALL file flag
//
BOOL CSetupDoc::OnSetupMkDirToken( EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue )
{
    ASSERT( eMajor == jMkDir );

	CCommand* pCommand;
	if ( GetNumCommands() == 0 )
	{
        pCommand = AddCommand();
        pCommand->SetCommandID( m_nLine );
	}
	else
	{
		pCommand = GetLastCommand();
		if ( pCommand->GetCommandID() != m_nLine )
		{
	        pCommand = AddCommand();
		    pCommand->SetCommandID( m_nLine );
		}
	}

    switch( eMinor )
    {
        case nStart:
            pCommand->SetCommandType( TOK_MKDIR );
            break;

        case nSysFile:
            pCommand->SetSysFileFlag();
            break;

        case nSharedFile:
            pCommand->SetSharedFileFlag();
            break;

        case nUninstall:
			if ( eToken == TOK_UNINSTALL )
				pCommand->SetUninstallFileFlag();
			else
			{
				pCommand->SetUninstallAllFlag();
			}
            break;

        case nMkDir:
            pCommand->SetMkDirValue( pszValue );
            break;

        case nGroup:
			{
			  __int64 group;
			  if(*pszValue == '0' && (pszValue[1] == 'x' || pszValue[1] == 'X'))
			     sscanf(&pszValue[2],"%I64X",&group);
			  else
				  sscanf(pszValue,"%I64d",&group);
              pCommand->SetDirGroup( group );
			}
            break;

        default:
            ASSERT(FALSE);
            return FALSE;
    }

    return TRUE;
}
//****************************************************************************
// Class        CSetupDoc
//
// Procedure    OnSetupMkRootToken
//
// Purpose      Builds an MKROOT command based on the tokens we receive.
//
// Parameters   eMajor      the major half of the compiler state (jInstall)
//              eMinor      the minor half of the compiler state
//              eToken      the token that caused us to be here.
//              pszValue    string represented by the token
//
// Returns      zero to abort, nonzero to continue
//
// History       2/02/95    KenSh       Created
//               6/21/95    KenSh       Added UININSTALL file flag
//
BOOL CSetupDoc::OnSetupMkRootToken( EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue )
{
    ASSERT( eMajor == jMkRoot );

	CCommand* pCommand;
	if ( GetNumCommands() == 0 )
	{
        pCommand = AddCommand();
        pCommand->SetCommandID( m_nLine );
	}
	else
	{
		pCommand = GetLastCommand();
		if ( pCommand->GetCommandID() != m_nLine )
		{
	        pCommand = AddCommand();
		    pCommand->SetCommandID( m_nLine );
		}
	}

    switch( eMinor )
    {
        case nStart:
            pCommand->SetCommandType( TOK_MKROOT );
            break;

        case nUninstall:
            pCommand->SetUninstallFileFlag();
            break;

		case nOS:
			return OnSetupDocToken(jSetupDoc, eMinor, eToken, pszValue);

        default:
            ASSERT(FALSE);
            return FALSE;
    }

    return TRUE;
}

BOOL CSetupDoc::OnSetupGetNameToken( EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue )
{
    ASSERT( eMajor == jGetName );

	CCommand* pCommand;
	if ( GetNumCommands() == 0 )
	{
        pCommand = AddCommand();
        pCommand->SetCommandID( m_nLine );
	}
	else
	{
		pCommand = GetLastCommand();
		if ( pCommand->GetCommandID() != m_nLine )
		{
	        pCommand = AddCommand();
		    pCommand->SetCommandID( m_nLine );
		}
	}

    switch( eMinor )
    {
        case nStart:
            pCommand->SetCommandType( TOK_GETNAME );
            break;

		case nOS:
			return OnSetupDocToken(jSetupDoc, eMinor, eToken, pszValue);

        default:
            ASSERT(FALSE);
            return FALSE;
    }

    return TRUE;
}
BOOL CSetupDoc::OnSetupGetGroupToken( EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue )
{
    ASSERT( eMajor == jGetGroup );

	CCommand* pCommand;
	if ( GetNumCommands() == 0 )
	{
        pCommand = AddCommand();
        pCommand->SetCommandID( m_nLine );
	}
	else
	{
		pCommand = GetLastCommand();
		if ( pCommand->GetCommandID() != m_nLine )
		{
	        pCommand = AddCommand();
		    pCommand->SetCommandID( m_nLine );
		}
	}

    switch( eMinor )
    {
        case nStart:
            pCommand->SetCommandType( TOK_GETGROUP );
            break;

        case nOS:
        {
            switch( eToken )
            {
                case TOK_WIN95:
                    pCommand->SetWin95Flag();
                    break;

                case TOK_WIN98:
                    pCommand->SetWin98Flag();
                    break;

                case TOK_ALLWIN:
                    pCommand->SetWin95Flag();
                    pCommand->SetWin98Flag();
                    break;

                case TOK_NT40:
                    pCommand->SetNT40Flag();
                    break;

                case TOK_NT50:
                    pCommand->SetNT50Flag();
                    break;

				case TOK_ALLNT:
					pCommand->SetNT40Flag();
                    pCommand->SetNT50Flag();
                    break;

				case TOK_DBCS:
					pCommand->SetDBCSFlag();
					break;

				case TOK_OEM:
					pCommand->SetOEMFlag();
					break;

				case TOK_RTL:
					pCommand->SetRTLFlag();
					break;

				case TOK_JPN:
					pCommand->SetJPNFlag();
					break;

				case TOK_GER:
					pCommand->SetGERFlag();
					break;

				case TOK_FRA:
					pCommand->SetFRAFlag();
					break;

				case TOK_SPA:
					pCommand->SetSPAFlag();
					break;

				case TOK_USA:
					pCommand->SetUSAFlag();
					break;

				case TOK_APP1:
					pCommand->SetAPP1Flag();
					break;

				case TOK_APP2:
					pCommand->SetAPP2Flag();
					break;

				case TOK_APP3:
					pCommand->SetAPP3Flag();
					break;

                case TOK_IMEENABLE:
                    pCommand->SetIMEENABLEFlag();
                    break;

                case TOK_IMEON:
                    pCommand->SetIMEONFlag();
                    break;

                default:
                    ASSERT(FALSE);
            }
            break;
        }
        default:
            ASSERT(FALSE);
            return FALSE;
    }

    return TRUE;
}
BOOL CSetupDoc::OnSetupDeleteFile( EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue )
{
    ASSERT( eMajor == jDeleteFile );

	CCommand* pCommand;
	if ( GetNumCommands() == 0 )
	{
        pCommand = AddCommand();
        pCommand->SetCommandID( m_nLine );
	}
	else
	{
		pCommand = GetLastCommand();
		if ( pCommand->GetCommandID() != m_nLine )
		{
	        pCommand = AddCommand();
		    pCommand->SetCommandID( m_nLine );
		}
	}

    switch( eMinor )
    {
        case nStart:
            pCommand->SetCommandType( TOK_DELETEFILE );
            break;

        case nSrcName:
            pCommand->SetDeleteFileValue( pszValue );
            break;

        case nPersist:
            pCommand->SetDeleteFilePersistFlag();
            break;

        case nRecurse:
            pCommand->SetDeleteFileRecurseFlag();
            break;

        case nInstall:
            pCommand->SetDeleteFileInstallFlag();
            break;

		case nSilent:
			pCommand->SetDeleteFileSilentFlag();
			break;

        case nOS:
        {
            switch( eToken )
            {
                case TOK_WIN95:
                    pCommand->SetWin95Flag();
                    break;

                case TOK_WIN98:
                    pCommand->SetWin98Flag();
                    break;

                case TOK_ALLWIN:
                    pCommand->SetWin95Flag();
                    pCommand->SetWin98Flag();
                    break;

                case TOK_NT40:
                    pCommand->SetNT40Flag();
                    break;

                case TOK_NT50:
                    pCommand->SetNT50Flag();
                    break;

				case TOK_ALLNT:
					pCommand->SetNT40Flag();
                    pCommand->SetNT50Flag();
                    break;

				case TOK_DBCS:
					pCommand->SetDBCSFlag();
					break;

				case TOK_OEM:
					pCommand->SetOEMFlag();
					break;

				case TOK_RTL:
					pCommand->SetRTLFlag();
					break;

				case TOK_JPN:
					pCommand->SetJPNFlag();
					break;

				case TOK_GER:
					pCommand->SetGERFlag();
					break;

				case TOK_FRA:
					pCommand->SetFRAFlag();
					break;

				case TOK_SPA:
					pCommand->SetSPAFlag();
					break;

				case TOK_USA:
					pCommand->SetUSAFlag();
					break;

				case TOK_APP1:
					pCommand->SetAPP1Flag();
					break;

				case TOK_APP2:
					pCommand->SetAPP2Flag();
					break;

				case TOK_APP3:
					pCommand->SetAPP3Flag();
					break;

                case TOK_IMEENABLE:
                    pCommand->SetIMEENABLEFlag();
                    break;

                case TOK_IMEON:
                    pCommand->SetIMEONFlag();
                    break;

                default:
                    ASSERT(FALSE);
            }
            break;
        }
        default:
            ASSERT(FALSE);
            return FALSE;
    }

    return TRUE;
}

BOOL CSetupDoc::OnSetupGetPIDToken( EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue )
{
    ASSERT( eMajor == jGetPID );

	CCommand* pCommand;
	if ( GetNumCommands() == 0 )
	{
        pCommand = AddCommand();
        pCommand->SetCommandID( m_nLine );
	}
	else
	{
		pCommand = GetLastCommand();
		if ( pCommand->GetCommandID() != m_nLine )
		{
	        pCommand = AddCommand();
		    pCommand->SetCommandID( m_nLine );
		}
	}

    switch( eMinor )
    {
        case nStart:
            pCommand->SetCommandType( TOK_GETPID );
            break;

		case nOS:
			return OnSetupDocToken(jSetupDoc, eMinor, eToken, pszValue);

		default:
            ASSERT(FALSE);
            return FALSE;
    }

    return TRUE;
}
BOOL CSetupDoc::OnSetupInstDXToken( EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue )
{
    ASSERT( eMajor == jInstDX );

	CCommand* pCommand;
	if ( GetNumCommands() == 0 )
	{
        pCommand = AddCommand();
        pCommand->SetCommandID( m_nLine );
	}
	else
	{
		pCommand = GetLastCommand();
		if ( pCommand->GetCommandID() != m_nLine )
		{
	        pCommand = AddCommand();
		    pCommand->SetCommandID( m_nLine );
		}
	}

    switch( eMinor )
    {
        case nStart:
            pCommand->SetCommandType( TOK_INSTDX );
            break;

        case nInstDX:
			if (lstrcmpi("NULL", pszValue) ? FALSE : TRUE)
			{
                //Stop supporting the DPLAY syntax.
                Alert( g_hwnd, MB_ICONSTOP | MB_OK, STR_ILLEGALTOKEN,
                                (LPCSTR)pszValue, m_nLine );
				return FALSE;
			}
			else
			{
	            pCommand->SetInstDXValue( pszValue );
			}
            break;

        case nInstDXName:
            pCommand->SetInstDXNameValue( pszValue );
            break;

        case nInstDXFlags:
            pCommand->SetInstDXFlagsValue( pszValue );
            break;

        case nInstDXMinVersion:
            pCommand->SetInstDXMinVersionValue( pszValue );
            break;

		case nOS:
			return OnSetupDocToken(jSetupDoc, eMinor, eToken, pszValue);

        default:
            ASSERT(FALSE);
            return FALSE;
    }

    return TRUE;
}
BOOL CSetupDoc::OnSetupInstDPLAYToken( EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue )
{
    ASSERT( eMajor == jInstDPLAY );

	CCommand* pCommand;
	if ( GetNumCommands() == 0 )
	{
        pCommand = AddCommand();
        pCommand->SetCommandID( m_nLine );
	}
	else
	{
		pCommand = GetLastCommand();
		if ( pCommand->GetCommandID() != m_nLine )
		{
	        pCommand = AddCommand();
		    pCommand->SetCommandID( m_nLine );
		}
	}

    switch( eMinor )
    {
        case nStart:
            pCommand->SetCommandType( TOK_INSTDPLAY );
            break;

        case nInstDPLAYName:
            pCommand->SetInstDPLAYNameValue( pszValue );
            break;

        case nInstDPLAYMinVersion:
            pCommand->SetInstDPLAYMinVersionValue( pszValue );
            break;

		case nOS:
			return OnSetupDocToken(jSetupDoc, eMinor, eToken, pszValue);

        default:
            ASSERT(FALSE);
            return FALSE;
    }

    return TRUE;
}

BOOL CSetupDoc::OnSetupInstIcon( EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue )
{
    ASSERT( eMajor == jInstIcon );

	CCommand* pCommand;
	if ( GetNumCommands() == 0 )
	{
        pCommand = AddCommand();
        pCommand->SetCommandID( m_nLine );
	}
	else
	{
		pCommand = GetLastCommand();
		if ( pCommand->GetCommandID() != m_nLine )
		{
	        pCommand = AddCommand();
		    pCommand->SetCommandID( m_nLine );
		}
	}

    switch( eMinor )
    {
        case nStart:
            pCommand->SetCommandType( TOK_INSTICON );
            break;

		case nUninstallLink:
			pCommand->SetUninstallLinkFlag();
			break;

        case nInstIconName:
            pCommand->SetInstIconNameValue( pszValue );
            break;

        case nInstIconNameIcon:
            pCommand->SetInstIconNameIconValue( pszValue );
            break;

        case nInstIconIndex:
            pCommand->SetInstIconIndexValue( pszValue );
            break;

        case nInstIconPath:
            pCommand->SetInstIconDescriptionValue(pszValue);
            break;

        case nInstIconDest:
            pCommand->SetInstIconDestinationValue( pszValue );
            break;

        case nGroup:
			{
			  __int64 group;

			  if (*pszValue == '0' && (pszValue[1] == 'x' || pszValue[1] == 'X'))
			     sscanf(&pszValue[2],"%I64X",&group);
			  else
				  sscanf(pszValue,"%I64d",&group);

              pCommand->SetGroup(group);
			}
            break;

		default:
            ASSERT(FALSE);
            return FALSE;
    }

    return TRUE;
}

BOOL CSetupDoc::OnSetupCDSpeed( EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue )
{
    ASSERT( eMajor == jCDSpeed );

	CCommand* pCommand;
	if ( GetNumCommands() == 0 )
	{
        pCommand = AddCommand();
        pCommand->SetCommandID( m_nLine );
	}
	else
	{
		pCommand = GetLastCommand();
		if ( pCommand->GetCommandID() != m_nLine )
		{
	        pCommand = AddCommand();
		    pCommand->SetCommandID( m_nLine );
		}
	}

    switch( eMinor )
    {
        case nStart:
            pCommand->SetCommandType( TOK_CDSPEED );
            break;

        case nCDSpeedName:
            pCommand->SetCDSpeedFileNameValue( pszValue );
            break;

        case nCDSpeedCDMin:
            pCommand->SetCDSpeedMinCDValue( pszValue );
            break;

        case nCDSpeedCPUMax:
            pCommand->SetCDSpeedMaxCPUValue( pszValue );
            break;

        default:
            ASSERT(FALSE);
            return FALSE;
    }

    return TRUE;
}


//****************************************************************************
// Class        CSetupDoc
//
// Procedure    OnSetupIniValueToken
//
// Purpose      Builds an ADDINIVALUE command based on the tokens we receive.
//
// Parameters   eMajor      the major half of the compiler state (jIniValue)
//              eMinor      the minor half of the compiler state
//              eToken      the token that caused us to be here.
//              pszValue    string represented by the token
//
// Returns      zero to abort, nonzero to continue
//
// History       2/02/95    KenSh       Created
//
BOOL CSetupDoc::OnSetupIniValueToken( EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue )
{
    ASSERT( eMajor == jIniValue );

	CCommand* pCommand;
	if ( GetNumCommands() == 0 )
	{
        pCommand = AddCommand();
        pCommand->SetCommandID( m_nLine );
	}
	else
	{
		pCommand = GetLastCommand();
		if ( pCommand->GetCommandID() != m_nLine )
		{
	        pCommand = AddCommand();
		    pCommand->SetCommandID( m_nLine );
		}
	}

    switch( eMinor )
    {
        case nStart:
            pCommand->SetCommandType( TOK_INIVALUE );
            break;

        case nFilename:
            pCommand->SetIniFilename( pszValue );
            break;

        case nMap:
            pCommand->SetMapFlag();
            break;

        case nSection:
            pCommand->SetIniSection( pszValue );
            break;

        case nEntry:
            pCommand->SetIniEntry( pszValue );
            break;

        case nValue:
            pCommand->SetIniValue( pszValue );
            break;

		case nType:
			pCommand->SetIniType(pszValue);
			break; 

		case nGroup:
			__int64 group;

			if(*pszValue == '0' && (pszValue[1] == 'x' || pszValue[1] == 'X'))
			{
				sscanf(&pszValue[2],"%I64X",&group);
			}
			else
			{
				sscanf(pszValue,"%I64d",&group);
			}

			pCommand->SetGroup(group);

			break;

		case nUninstall:
			if ( eToken == TOK_UNINSTALL )
				pCommand->SetUninstallFileFlag();
			else
			{
				pCommand->SetUninstallAllFlag();
			}
			break;

        default:
            ASSERT(FALSE);
            return FALSE;
    }

    return TRUE;
}

//****************************************************************************
// Class        CSetupDoc
//
// Procedure    OnSetupInstFontToken
//
// Purpose      Builds an INSTALLFONT command based on the tokens we receive.
//
// Parameters   eMajor      the major half of the compiler state (jInstFont)
//              eMinor      the minor half of the compiler state
//              eToken      the token that caused us to be here.
//              pszValue    string represented by the token
//
// Returns      zero to abort, nonzero to continue
//
// History       8/10/95    a-DenSo     Created based on OnSetupInstallToken
//
BOOL CSetupDoc::OnSetupInstFontToken( EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue )
{
    ASSERT( eMajor == jInstFont );

	CCommand* pCommand;
	if ( GetNumCommands() == 0 )
	{
        pCommand = AddCommand();
        pCommand->SetCommandID( m_nLine );
	}
	else
	{
		pCommand = GetLastCommand();
		if ( pCommand->GetCommandID() != m_nLine )
		{
	        pCommand = AddCommand();
		    pCommand->SetCommandID( m_nLine );
		}
	}

    switch( eMinor )
    {
        case nStart:
            pCommand->SetCommandType( TOK_INSTFONT );
            break;

        case nDestDir:
            if( eToken == TOK_SYSTEM )
            {
                pCommand->SetSystemDirFlag();
            }
//            else
//            {
//                pCommand->SetSystem32DirFlag();
//            }
            break;

        case nSysFile:
            pCommand->SetSysFileFlag();
            break;

        case nSharedFile:
            pCommand->SetSharedFileFlag();
            break;

        case nDestName:
            pCommand->SetDestName( pszValue );
            break;

        case nSrcName:
            pCommand->SetSourceName( pszValue );

			#ifdef EBUPREPSTUB
				UpdateDisplay();
			#else
				EbuYield();
			#endif

            break;

		case nOS:
			return OnSetupDocToken(jSetupDoc, eMinor, eToken, pszValue);

        default:
            ASSERT(FALSE);
            return FALSE;
    }

    return TRUE;
}


BOOL CSetupDoc::OnSetupBeginFileList ( EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue )
{
    ASSERT( eMajor == jBeginFileList);

	CCommand* pCommand;
	if ( GetNumCommands() == 0 )
	{
        pCommand = AddCommand();
        pCommand->SetCommandID( m_nLine );
	}
	else
	{
		pCommand = GetLastCommand();
		if ( pCommand->GetCommandID() != m_nLine )
		{
	        pCommand = AddCommand();
		    pCommand->SetCommandID( m_nLine );
		}
	}

    switch( eMinor )
    {
        case nStart:
            pCommand->SetCommandType( TOK_BEGINFILELIST);
            break;

        default:
            ASSERT(FALSE);
            return FALSE;
    }

    return TRUE;
}

BOOL CSetupDoc::OnSetupEndFileList ( EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue )
{
    ASSERT( eMajor == jEndFileList);

	CCommand* pCommand;
	if ( GetNumCommands() == 0 )
	{
        pCommand = AddCommand();
        pCommand->SetCommandID( m_nLine );
	}
	else
	{
		pCommand = GetLastCommand();
		if ( pCommand->GetCommandID() != m_nLine )
		{
	        pCommand = AddCommand();
		    pCommand->SetCommandID( m_nLine );
		}
	}

    switch( eMinor )
    {
        case nStart:
            pCommand->SetCommandType( TOK_ENDFILELIST);
            break;

        default:
            ASSERT(FALSE);
            return FALSE;
    }

    return TRUE;
}

BOOL CSetupDoc::OnSetupPropertyToken ( EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue )
{
    ASSERT( eMajor == jProperty);

	CCommand* pCommand;
	if ( GetNumCommands() == 0 )
	{
        pCommand = AddCommand();
        pCommand->SetCommandID( m_nLine );
	}
	else
	{
		pCommand = GetLastCommand();
		if ( pCommand->GetCommandID() != m_nLine )
		{
	        pCommand = AddCommand();
		    pCommand->SetCommandID( m_nLine );
		}
	}

    switch( eMinor )
    {
        case nStart:
            pCommand->SetCommandType( TOK_PROPERTY );
            break;

		case nProperty:
			pCommand->SetProperty (pszValue);
			break;

		case nPropertyValue:
			pCommand->SetPropertyValue (pszValue);
			break;

        default:
            ASSERT(FALSE);
            return FALSE;
    }

    return TRUE;
}

BOOL CSetupDoc::OnSetupCommentToken( EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue )
{
    ASSERT( eMajor == jComment );
  
	CCommand* pCommand;
	if ( GetNumCommands() == 0 )
	{
        pCommand = AddCommand();
        pCommand->SetCommandID( m_nLine );
	}
	else
	{
		pCommand = GetLastCommand();
		if ( pCommand->GetCommandID() != m_nLine )
		{
	        pCommand = AddCommand();
		    pCommand->SetCommandID( m_nLine );
		}
	}

    switch( eMinor )
    {
        case nStart:
            pCommand->SetCommandType( TOK_COMMENT );
			pCommand->SetComment ( pszValue );
            break;
       default:
            ASSERT(FALSE);
            return FALSE;
    }

    return TRUE;
}

BOOL CSetupDoc::OnSetupRuleToken( EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue )
{
    ASSERT( eMajor == jRule);
  
	CCommand* pCommand;
	if ( GetNumCommands() == 0 )
	{
        pCommand = AddCommand();
        pCommand->SetCommandID( m_nLine );
	}
	else
	{
		pCommand = GetLastCommand();
		if ( pCommand->GetCommandID() != m_nLine )
		{
	        pCommand = AddCommand();
		    pCommand->SetCommandID( m_nLine );
		}
	}

    switch( eMinor )
    {
        case nStart:
            pCommand->SetCommandType( TOK_RULE );
            break;

		case nAction:
			pCommand->SetRuleAction ( pszValue );
			break;

		case nPattern:
			pCommand->SetRulePattern( pszValue );
			break;

		case nGroup:
			{
			  __int64 group=0;
			  if(*pszValue == '0' && (pszValue[1] == 'x' || pszValue[1] == 'X'))
			     sscanf(&pszValue[2],"%I64X",&group);
			  else
				  sscanf(pszValue,"%I64d",&group);

			  pCommand->SetGroup(group);
			}
			break;
        default:
            ASSERT(FALSE);
            return FALSE;
    }

    return TRUE;
}


BOOL CSetupDoc::OnSetupBeginStringList ( EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue )
{
    ASSERT( eMajor == jBeginStringList);

	CCommand* pCommand;
	if ( GetNumCommands() == 0 )
	{
        pCommand = AddCommand();
        pCommand->SetCommandID( m_nLine );
	}
	else
	{
		pCommand = GetLastCommand();
		if ( pCommand->GetCommandID() != m_nLine )
		{
	        pCommand = AddCommand();
		    pCommand->SetCommandID( m_nLine );
		}
	}

    switch( eMinor )
    {
        case nStart:
            pCommand->SetCommandType( TOK_BEGINSTRINGLIST);
            break;

        default:
            ASSERT(FALSE);
            return FALSE;
    }

    return TRUE;
}


BOOL CSetupDoc::OnSetupEndStringList ( EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue )
{
    ASSERT( eMajor == jEndStringList);

	CCommand* pCommand;
	if ( GetNumCommands() == 0 )
	{
        pCommand = AddCommand();
        pCommand->SetCommandID( m_nLine );
	}
	else
	{
		pCommand = GetLastCommand();
		if ( pCommand->GetCommandID() != m_nLine )
		{
	        pCommand = AddCommand();
		    pCommand->SetCommandID( m_nLine );
		}
	}

    switch( eMinor )
    {
        case nStart:
            pCommand->SetCommandType( TOK_ENDSTRINGLIST);
            break;

        default:
            ASSERT(FALSE);
            return FALSE;
    }

    return TRUE;
}


BOOL CSetupDoc::OnSetupBeginStaticStrings ( EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue )
{
    ASSERT( eMajor == jBeginStaticStringList);

	CCommand* pCommand;
	if ( GetNumCommands() == 0 )
	{
        pCommand = AddCommand();
        pCommand->SetCommandID( m_nLine );
	}
	else
	{
		pCommand = GetLastCommand();
		if ( pCommand->GetCommandID() != m_nLine )
		{
	        pCommand = AddCommand();
		    pCommand->SetCommandID( m_nLine );
		}
	}

    switch( eMinor )
    {
        case nStart:
            pCommand->SetCommandType( TOK_BEGINSTATICSTRINGLIST);
            break;

        default:
            ASSERT(FALSE);
            return FALSE;
    }

    return TRUE;
}


BOOL CSetupDoc::OnSetupEndStaticStrings ( EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue )
{
    ASSERT( eMajor == jEndStaticStringList);

	CCommand* pCommand;
	if ( GetNumCommands() == 0 )
	{
        pCommand = AddCommand();
        pCommand->SetCommandID( m_nLine );
	}
	else
	{
		pCommand = GetLastCommand();
		if ( pCommand->GetCommandID() != m_nLine )
		{
	        pCommand = AddCommand();
		    pCommand->SetCommandID( m_nLine );
		}
	}

    switch( eMinor )
    {
        case nStart:
            pCommand->SetCommandType( TOK_ENDSTATICSTRINGLIST);
            break;

        default:
            ASSERT(FALSE);
            return FALSE;
    }

    return TRUE;
}


BOOL CSetupDoc::OnSetupStringToken( EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue )
{
    ASSERT( eMajor == jString);
  
	CCommand* pCommand;
	if ( GetNumCommands() == 0 )
	{
        pCommand = AddCommand();
        pCommand->SetCommandID( m_nLine );
	}
	else
	{
		pCommand = GetLastCommand();
		if ( pCommand->GetCommandID() != m_nLine )
		{
	        pCommand = AddCommand();
		    pCommand->SetCommandID( m_nLine );
		}
	}

    switch( eMinor )
    {
        case nStart:
            pCommand->SetCommandType( TOK_STRINGVAR );
            break;

		case nStringID:
			{
			int nStringID = -1;
			sscanf (pszValue, "%d", &nStringID);
			pCommand->SetStringID (nStringID);
			}
			break;

		case nStringValue:
			pCommand->SetStringValue (pszValue);
			break;

        default:
            ASSERT(FALSE);
            return FALSE;
    }

    return TRUE;
}


BOOL CSetupDoc::OnSetupActionToken( EMAJOR eMajor, EMINOR eMinor, ETOKEN eToken, LPCSTR pszValue )
{
    ASSERT( eMajor == jAction);
  
	CCommand* pCommand;
	if ( GetNumCommands() == 0 )
	{
        pCommand = AddCommand();
        pCommand->SetCommandID( m_nLine );
	}
	else
	{
		pCommand = GetLastCommand();
		if ( pCommand->GetCommandID() != m_nLine )
		{
	        pCommand = AddCommand();
		    pCommand->SetCommandID( m_nLine );
		}
	}

    switch( eMinor )
    {
        case nStart:
            pCommand->SetCommandType( TOK_ACTION );
            break;
		case nActionCommand:
			pCommand->SetActionCommand (pszValue);
			break;
		case nActionParam1:
			pCommand->SetActionParam1 (pszValue);
			break;
		case nActionParam2:
			pCommand->SetActionParam2 (pszValue);
			break;
		case nActionParam3:
			pCommand->SetActionParam3 (pszValue);
			break;
		case nActionParam4:
			pCommand->SetActionParam4 (pszValue);
			break;
		case nRecurse:
			pCommand->SetActionRecurseFlag (true);
			break;
		case nPrecopy:
			pCommand->SetCabPreCopy (true);
			break;
        default:
            ASSERT(FALSE);
            return FALSE;
    }

    return TRUE;
}


BOOL DetermineDiskToken(ETOKEN eToken, CCommand* pCommand)
{
	switch (eToken)
	{
		case TOK_DISK_01:
			pCommand->SetDiskId(DISK_01);
			break;
		case TOK_DISK_02:
			pCommand->SetDiskId(DISK_02);
			break;
		case TOK_DISK_03:
			pCommand->SetDiskId(DISK_03);
			break;
		case TOK_DISK_04:
			pCommand->SetDiskId(DISK_04);
			break;
		case TOK_DISK_05:
			pCommand->SetDiskId(DISK_05);
			break;
		case TOK_DISK_06:
			pCommand->SetDiskId(DISK_06);
			break;
		case TOK_DISK_07:
			pCommand->SetDiskId(DISK_07);
			break;
		case TOK_DISK_08:
			pCommand->SetDiskId(DISK_08);
			break;
	}
	return true;
}


void CSetupDoc::SortCommandsInRange (int nStart, int nEnd, SORTCOMPAREPROC SortCompareProc)
{
	int i;
	int nNumItems = nEnd - nStart + 1;
	
	CCommand **papCommands = (CCommand **) malloc (nNumItems * sizeof (CCommand *));
	

	for (i=0;i<nNumItems;i++)
	{
		papCommands[i] = m_papCommands[nStart+i];
	}

	qsort (papCommands, (size_t)nNumItems, sizeof (CCommand *), SortCompareProc);

	for (i=0;i<nNumItems;i++)
	{
		 m_papCommands[nStart+i] = papCommands[i];
	}

	free (papCommands);


	// clean up the dynamic file area, making sure each installlist command is followed by a newline token,
	// and there are no more than one newline token in a row.
	i = nStart;

	while (i <= nEnd)
	{
		if (m_papCommands[i]->GetCommandType() == TOK_NEWLINE)
		{
			SetCommandPtr (i);
			DeleteCommand ();
			nEnd--;
		}
		else
		if (m_papCommands[i]->GetCommandType() == TOK_INSTALLLIST)
		{
			if (m_papCommands[i]->GetCommandType() != TOK_NEWLINE)
			{
				i++;
				SetCommandPtr (i);

				CCommand *pCmd = new CCommand;

				pCmd->SetCommandType (TOK_NEWLINE);
				pCmd->SetCommandID (-1);
				
				InsertCommand (pCmd);
				i++;
				nEnd++;
			}
		}
		else
		{
			i++;
		}
	}
}
