#ifndef DBSTUFF_H
#define DBSTUFF_H
//--------------------------------------------------------------------------//
//                                                                          //
//                             DbStuff.h                                    //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Ajackson $
*/			    
//--------------------------------------------------------------------------//

#ifndef DOCUMENT_H
#include <Document.h>
#endif

//globals
extern IDocument *mainDoc;
extern BOOL32 saved;
extern BOOL32 RO;
extern char currentFileName[MAX_PATH];

//functions
void DoPrefsDialog();
void InitDB();
void Clone(char *name,char *defname);
void NewArchetype(char *baseName,char *defname);
void OpenArchetype(char *atName);
void DeleteArchetype(char *atName);
void RefreshList();//HWND hwnd);
void RenameArchetype(char *atName, char *atNewName);

BOOL32 SaveAs();
BOOL32 SaveDB();
void NewDB();
BOOL32 KillDB();
void OpenDB(char *name);
BOOL32 OpenDatabase(char *oldDB,char *oldH);
BOOL32 ConvertDB();
BOOL PlayResource(LPSTR lpName);
void EnumTypes();
void UpdateTitle();
BOOL32 SetupParser();
BOOL32 OpenDatabaseViaDir(char * dirname);

#endif