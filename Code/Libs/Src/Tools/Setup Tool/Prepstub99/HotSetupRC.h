
// These are the resource defines for the resources specific to the hotsetup library only.
// Application specific resources and defines should be contained within the applications rc files.

//#include "Script.h" // for the script command defines

#define IDR_SETUPINFO                   101

#define STR_LOCALIZE_DONTLOCALIZE           2
#define STR_LOCALIZE_LOCINSTR1              3
#define STR_LOCALIZE_LOCINSTR2              4
#define STR_LOCALIZE_CUSTOM					9

//
//String IDs below 100 should not be localized... These are all defined in hotsetup.rc
//
#define STR_REGKEY_MSGAMES_REG_ROOT         10	//"SOFTWARE\\Microsoft\\Microsoft Games"
#define STR_REGKEY_UNINSTALL        		11	//"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall" (should be using REGSTR_PATH_UNINSTALL)
#define STR_REGKEY_SHAREDDLL        		12	//"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SharedDlls"
#define STR_REGKEY_WIN40_FONTS              13	//"Software\\Microsoft\\Windows\\CurrentVersion\\Fonts"
#define STR_REGKEY_NT_FONTS                 14	//"Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts"
#define STR_REGKEY_VAL_COMMONFILESDIR       15	//"CommonFilesDir"		under the global REGSTR_PATH_SETUP key
#define STR_REGKEY_VAL_PROGRAMFILESDIR	    16	//"ProgramFilesDir"	under the global REGSTR_PATH_SETUP key

#define STR_REGKEY_VAL_LAUNCHED             17	//"Launched"
#define STR_REGKEY_VAL_PID                  18	//"Pid"
#define STR_REGKEY_VAL_TRIALVERSION         19	//"TrialVersion"
#define STR_REGKEY_VAL_VERSIONTYPE          20	//"VersionType"
#define STR_REGKEY_VAL_RETAILVERSION        21  //"RetailVersion"
#define STR_REGKEY_VAL_GROUP                22  //"InstalledGroup"
#define STR_REGKEY_VAL_PLAYERNAME			35	//"PlayerName"

#define STR_DIRECTORY_FONTS					23	// Windows "fonts" directory name
#define STR_DIRECTORY_MS_GAMES				24	// "Microsoft Games" base default directory for ms games

#define STR_PROGMAN_MS_GAMES				25	// "Microsoft Games" (under Start->Programs)

//TrueType font handling...
#define STR_TRUETYPE						26	// " (TrueType)"  used when fonts are installed
#define STR_TTF								27	// ".TTF" extension of TrueType font files...

#define STR_PID_SITECODE					28	// pid component, currently defined as "442"
#define STR_PID_OEM							29	// pid component, currently defined as "OEM"
#define STR_REGKEY_IGZGAMES_REG_ROOT   30	//"SOFTWARE\\Microsoft\\Internet Gaming Zone"
#define STR_DIRECTORY_IGZ_GAMES			31	// "Internet Gaming Zone" base default directory for ms games
#define STR_PROGMAN_IGZ_GAMES				32	// "Internet Gaming Zone" (under Start->Programs)
#define STR_REGKEY_IGZGAMES_REG_ROOTDEFAULT   33	//"SOFTWARE\\Microsoft\\Internet Gaming Zone\Setup"
#define STR_REGKEY_VAL_LANGID          34 // "LangID", language setup runs under

//
//Name of default setup DLL, this should be stored as a string resource in the setup
//application .EXE, not in the resource DLL...
//
#define STR_DEFAULTRESDLL			   99

//
//Strings in the 100-299 range are used and defined in the engine.  They can be localized
//and are all defined in hotsetup.rc
//

//Error messages
#define STR_ERROR_NEEDDISKSPACE				100	// message when the user has requested install on drive with not enough space
#define STR_ERROR_BADDIR					101	// user entered an invalid directory name
#define STR_ERROR_BADPATH					102 // user entered an invalid/incomplete/too long/writeprot/floppy path
#define STR_ERROR_FILENOTFOUND				106
#define STR_ERROR_CANTOPENSOURCE			107
#define STR_ERROR_RESOURCEFAILURE			108
#define STR_ERROR_BOGUSTOKEN				109
#define STR_ERROR_READFILEERROR				110
#define STR_ERROR_NEEDSYSDISKSPACE			112
#define STR_ERROR_CANTLOCKRESOURCE			113
#define STR_ERROR_INVALIDCOMMAND			114
#define STR_ERROR_OLEINITFAILED				115
#define STR_ERROR_DIRECTXSETUPFAILED		116
#define STR_ERROR_DXINST					117
#define STR_DXERROR_RETURN                  118
#define STR_DXERROR_RETURN1                 119
#define STR_DXERROR_RETURN2                 120
#define STR_DXERROR_RETURN3                 121
#define STR_DXERROR_RETURN4                 122
#define STR_DXERROR_RETURN5                 123
#define STR_DXERROR_RETURN6                 124
#define STR_DXERROR_RETURN7                 125
#define STR_DXERROR_RETURN8                 126
#define STR_DXERROR_RETURN9                 127
#define STR_DXERROR_RETURN10                128
#define STR_DXERROR_RETURN11                129
#define STR_DXERROR_RETURN12                130
#define STR_ERROR_NEED256COLORS             131
#define STR_ERROR_NOSOUNDCARD               132
#define STR_ERROR_CANTWRITETEMPFILE         133
#define STR_ERROR_CANTREADDEST              135
#define STR_ERROR_MISSINGCD                 136
#define STR_ERROR_MISSINGCDHALFWAY          137
#define STR_ERROR_NODISKSPACE               138
#define STR_ERROR_UNKNOWNWRITEPROBLEM       139
#define STR_ERROR_INITFAILURE               140
#define STR_ERROR_NOMEMORY                  141
#define STR_ERROR_PLEASECLOSEFILE           142
#define STR_ERROR_CANTCREATEDIRECTORY       143
#define STR_ERROR_MUSTGETDIR                144	// user must enter a directory (getroot processing)
#define STR_ERROR_BADCDKEY                  145
#define STR_ERROR_CANTSTARTREGWIZ			146
#define STR_ERROR_NOTENOUGHMEMORY           147
#define STR_ERROR_CANTFINDCD				148
#define STR_ERROR_NODESTDIR					149
#define	STR_ERROR_DIRECTPLAYSETUPFAILED		150

//
//Miscellaneous Engine strings
//
#define STR_MANUALREBOOT					200	// notify the user that they will have to reboot manualy
#define STR_REGWIZPROMPT					201	// Ask the user if they want to tray automatic registration
#define	STR_USERSTOPPEDCOPY					202	// prompt to verify the user wants to stop file copy
#define STR_QUIT_MAINTENANCE                203	// ask the user if they really want to quit Maintenance mode
#define STR_ABORT_MAINTENANCE				204	// ask the user if they want to abort maintenance mode (when dirty)
#define STR_QUIT_SETUP                      205	// ask the user if they really want to quit setup (not dirty)
#define	STR_ABORT_SETUP						206	// ask the user if they want to abort setup (when dirty)
#define STR_QUERY_END_SESSION				207	// windows asked us to shutdown and we can't

//
//Strings in the 300-499 range are used by the engine, but should be defined in the application's
//resource file(s)...
//

//
//Fonts - these are actually used in the AnimLib...
//
#define STR_FONT_BUTTON						300
#define STR_FONT_BOLD						301
#define STR_FONT_DEFAULT					302
#define STR_FONT_LARGE						303
#define STR_FONT_SMALL						304

//
//Regkeys
//
#define STR_REGKEY_UNINSTALL_DISPLAY_NAME   306 // "Microsoft Anarchy" (used in the add/remove programs setup)

#define STR_ERROR_NOWEBBROWSER				307 // if no browser is detected for online registration...

//Default directory (under ProgramFiles\Microsoft Games)
#define STR_DIRECTORY_APP_ROOT				308	// "Anarchy"

//Engine error messages
#define STR_OVERWRITELANGUAGE               309 // tell user file languages are different, ask to overwrite

//
//Product PID RPC code...
//
#define STR_PRODUCTRPC                     313	// products specific pid component

//
//Language and script specific ids
//
#define STR_ISDBCS						 315 // define == "1" if running under DBCS
#define STR_CUSTOMPLATFORM				 316 // custom value for setup script "platform" field
#define STR_ISOEM						 317 // define == "1" if this is an OEM build
#define STR_LANGUAGE					 318 // USA, GER, FRA, JPN currently supported.

//
//Command line switches...
//
#define STR_UNSETUPSWITCH				319	// "/uninstall" flag to notify setup it is being run as an uninstal
#define STR_AUTORUNSWITCH				320	// "/autorun" flag to notify setup it has been run by the autorun

//
//Used in pop up dialogs and engine messages "%APPTITLE" and "%SETUPTITLE"...
//
#define STR_SETUP_APPTITLE               321	// game display name EG.  "Microsoft XXXX"
#define STR_SETUP_SETUPTITLE			 322    // setup windows title EG. "Microsoft XXXX Setup"

//
//Must be defined so localization team can change numbers for file size differences...
//
#define STR_EXTRASYSTEMSPACE			 364	// bytes extra space needed on system drive (for DirectX, etc.)
#define STR_EXTRAAPPSPACE				 365	// bytes extra space needed on game drive, if any

#define STR_PRINTERFONTTYPE				 361

//
//Uninstall.Exe name - must be defined
//
#define STR_UNSETUPEXENAME				367		// usually "uninstal.exe", can be localized...

//
//Must be defined for OEM builds...
//
#define STR_PRODUCTOEMRPC				 355	// OEM build RPC 

//Not used directly in the setup engine (either app only or not used at all)
#define STR_UNINSTALLED                      337
#define STR_LAUNCHEXEWINNAME                 335

#define STR_MUSTRESTART                     310 // Used to user that they must reboot before starting this game

//hardware check errors
#define STR_ERROR_BADPROCESSOR              325
#define STR_ERROR_CDTOOSLOW                 326
#define STR_ERROR_CPUTOOHIGH                327
#define STR_ERROR_NEEDHIGHERRES				328

// Used by Prepstub9x to store build number information about itself, the script and 
// the injected binary blob.  The range 480-495 is reserved for this purpose
#define PREPSTUB_BUILD_NUMBERS				480

// The symbol table is a string resource lookup for substituting local specific strings
// into the setup script at runtime, mostly for the start menu and desktop icon descriptions
// and names.  Do not use 500 - 699 as defined values, these 199 values are reserved for the
// string table
#define SYMBOL_TABLE						500

//
//application defined strings should start at 1000 or above - this is not enforced by
//the engine, but it's the guideline we should follow...
//
#define	APPLICATION_DEFINES_START		 1000

#define APPLICATION_NONLOC_DEFINES_START 2000

//
//Above 2000 should not be localized...
//
#define STR_REGKEY_APP_REG_ROOT             2000 // "Anarchy"
#define STR_REGKEY_VAL_APPPATH              2001 // "InstalledPath" Engine stores the game destination path

//Used by LaunchApplication...
#define STR_LAUNCHEXE                       2002 // EXE to be launched by LaunchApplication()
#define STR_COMMANDLINE						2003 // optional command line for LaunchApplication()
#define STR_REGKEY_VAL_DEFAULTPATH              2004 // Default path for Zone install
#define STR_APP_GUID						2005 // Application GUID for DPLAY lobby registration
#define STR_DPLAYCOMMANDLINE				2006 // Command line for DPLAY lobby invokation

#ifndef IDC_STATIC
#define IDC_STATIC                      -1

#endif
