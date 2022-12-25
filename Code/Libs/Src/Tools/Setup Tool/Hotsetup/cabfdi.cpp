/*
 * testfdi.c
 *
 * Demonstrates how to use the FDI library APIs
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <dos.h>
#include "setup.h"
#include "hotsetup.h"
#include "util.h"
#include "hotsetuprc.h"
#include "vercopy.h"

#include "fdi.h"

extern BOOL ReadTTFInfo(PSTR pszFile, LPSTR lpszVersion, LPSTR lpszFontName);

typedef struct CopyList
{
	LPFILECOPYSTATUS fs;
	EBUCALLBACK lpfn;
	char *cabname;
}
COPYLIST, *LPCOPYLIST;
static LPCOPYLIST lpCopy;
static LPINSTALLLIST lpCurrent;

#define CAB1      0xfffffff0
#define CAB2      0xfffffff2
typedef struct CabFile
{
   BOOL bCabOpen;
   DWORD dwCabSize;
   LPBYTE lpCabPointer;
   LPBYTE lpCabCurrPointer;
}CABFILE;

using namespace NGLOBALS;

static CABFILE Cab1;
static CABFILE Cab2;
static char tempName[_MAX_PATH];

/*
 * Function prototypes
 */
BOOL	test_fdi(char *cabinet_file);
EBURETCODE extract_files(char *cabinet_name);
static EBURETCODE retc = EBU_OK;

/*
 * Memory allocation function
 */
FNALLOC(mem_alloc)
{
	return malloc(cb);
}


/*
 * Memory free function
 */
FNFREE(mem_free)
{
	free(pv);
}


FNOPEN(file_open)
{
	HRSRC  hRsrc;
	HGLOBAL hGlobal;
	CABFILE *tCab;
	int cabcode;
	
	if(!lstrcmpi(pszFile+lstrlen(pszFile)-4,".CAB"))
	{
		if(Cab1.bCabOpen)
		{
			cabcode = CAB2;
			tCab = &Cab2;
		}
		else
		{
			tCab = &Cab1;
			cabcode = CAB1;
		}
		
		if(tCab->bCabOpen)
			return (int)tCab->lpCabPointer;
		if (hRsrc = FindResource(GetAppInst(), "IDR_CABFILE","CABFILE"))
		{
			hGlobal = LoadResource(GetAppInst(), hRsrc);
			tCab->lpCabPointer = (LPBYTE)LockResource(hGlobal);
			if(tCab->lpCabPointer == NULL)
				return -1;
			tCab->lpCabCurrPointer = tCab->lpCabPointer;
			tCab->dwCabSize =  *(DWORD *)(tCab->lpCabPointer + 8);
			tCab->bCabOpen = TRUE;
			return (int)cabcode;
		}
		return -1;
		
	}
	else
	{
		if (EnsureCDROMInserted())
		{
			return _open(pszFile, oflag, pmode);
		}
		else
		{
			return -1;
		}
	}
}


FNREAD(file_read)
{
	CABFILE *tCab=NULL;
	if(hf == CAB2)
		tCab = &Cab2;
	else if(hf == CAB1) 
		tCab = &Cab1;
	if(tCab)
	{
		memcpy(pv,tCab->lpCabCurrPointer,cb);
		tCab->lpCabCurrPointer+=cb;
		return cb;
	}
	else
	{
		if (EnsureCDROMInserted())
		{
			return _read(hf, pv, cb);
		}
		else
		{
			return -1;
		}
	}
}


FNWRITE(file_write)
{
	if(GetAppCallback())
	{
		retc = (*(GetAppCallback())) ((void *) lpCopy->fs);
		ASSERT(EBU_OK == retc || EBU_ABORT == retc || EBU_CANCEL == retc);
		
		switch (retc)
		{
		case EBU_ABORT:
		case EBU_CANCEL:
			//
			//User has already been prompted to quit if EBU_CANCEL was returned...
			//
			retc = EBU_ABORT;
			return -1;
		}
		lpCopy->fs->dwLastFile = cb;
		lpCopy->fs->dwTotalCopied += (DWORD)cb;
	}

	if (EnsureCDROMInserted())
	{
		return _write(hf, pv, cb);
	}
	else
	{
		return -1;
	}
}


FNCLOSE(file_close)
{
	CABFILE *tCab=NULL;
	if(hf == CAB2)
		tCab = &Cab2;
	else if(hf == CAB1)
		tCab = &Cab1;
	if(tCab)
	{
		tCab->lpCabPointer = tCab->lpCabCurrPointer = NULL;
		tCab->bCabOpen = FALSE;
		return 0;
	}
	else
		return _close(hf);
}


FNSEEK(file_seek)
{
	
	CABFILE *tCab=NULL;
	if(hf == CAB2)
		tCab = &Cab2;
	else if(hf == CAB1)
		tCab = &Cab1;
	if(tCab)
	{
		switch(seektype)
		{
		case SEEK_CUR:
			{
				long check = tCab->lpCabCurrPointer - tCab->lpCabPointer;
				if((DWORD)(check + dist) > tCab->dwCabSize || check + dist < 0)
					return -1L;
				tCab->lpCabCurrPointer += dist;
				return tCab->lpCabCurrPointer - tCab->lpCabPointer;
			}
		case SEEK_SET:
			if((DWORD)dist > tCab->dwCabSize)
				return -1L;
			tCab->lpCabCurrPointer = tCab->lpCabPointer + dist;
			return dist;
		case SEEK_END:
			if(dist > 0 || (DWORD)(-dist) > tCab->dwCabSize)
				return -1L;
			tCab->lpCabCurrPointer  = tCab->lpCabPointer + dist;
			return tCab->lpCabCurrPointer - tCab->lpCabPointer;
		}
		return -1L;
	}
	else
	{
		if (EnsureCDROMInserted())
		{
			return _lseek(hf, dist, seektype);
		}
		else
		{
			return -1L;
		}
	}
}

LPINSTALLLIST FindFileInList(char *filename)
{
	LPINSTALLLIST lcurr,lprev;
	if(NULL == GetListHead())
		return NULL;
	lcurr = GetListHead();
	lprev = NULL;
	while(lcurr != NULL)
	{
		char *ptr;
		char szSource[_MAX_PATH];
#if 0
		if (IsDBCS())
		{
			ptr = lcurr->szSource+lstrlen(lcurr->szSource);
			do {
				ptr = CharPrev(lcurr->szSource, ptr);
			} while(*ptr != '\\' && ptr != lcurr->szSource);
		}
		else
		{
			ptr = lcurr->szSource+lstrlen(lcurr->szSource)-1;
			while(*ptr != '\\' && ptr != lcurr->szSource)
				ptr--;
		}

		if(*ptr == '\\')
			ptr++;
#else
			ptr = lcurr->szSource;
         char moddir[_MAX_PATH];
         GetModuleDirectory(moddir,sizeof(moddir));
		   int count = lstrlen(moddir);
			ptr = lcurr->szSource+count;
#endif

		lstrcpy(szSource,ptr);
		if(!lstrcmpi(filename,szSource))
		{
			if(lprev != NULL)
                lprev->nextElement = lcurr->nextElement;
			else
				SetListHead(lcurr->nextElement);
			if(lprev == NULL && lcurr->nextElement == NULL)
			{
				SetListHead(NULL);
				SetListEnd(NULL);
			}
			return lcurr;
		}
		else
		{
			lprev = lcurr;
		    lcurr = lcurr->nextElement;
		}
	}
	return NULL;
}


FNFDINOTIFY(notification_function)
{
    static BOOL fAlreadyExists;
	
	switch (fdint)
	{
	case fdintCABINET_INFO: // general information about the cabinet
		return 0;
		
	case fdintPARTIAL_FILE: // first file in cabinet is continuation
		return 0;
		
	case fdintCOPY_FILE:	// file to be copied
		{
			fAlreadyExists=FALSE;
			lpCopy = (LPCOPYLIST)pfdin->pv;
			if((lpCurrent = FindFileInList(pfdin->psz1)) == NULL)
			{   
				return 0;
			}
			else
			{
				lpCopy->fs->dwLastFile = 0;
				lpCopy->fs->dwTotalCopied = 0;
				//				lpCopy->fs->dwTotalSize = (DWORD)pfdin->cb;
				lstrcpy(lpCopy->fs->szSource,lpCopy->cabname);
				lstrcpy(lpCopy->fs->szDest,lpCurrent->szDest);
				if (n_fMaintMode && !GetForceReinstall()) // Maintainence mode, only copy file in list if it's not there
				{				 // you won't recopy files in multiple groups
					if(GetFileAttributes(lpCopy->fs->szDest) != 0xFFFFFFFF)
						return 0;
				}
				
				lpCopy->lpfn = GetAppCallback();
				lpCopy->fs->nID = SC_CABGO;
				
				retc = EBU_OK;
				
				if ((retc = (*lpCopy->lpfn) ((void *) lpCopy->fs)) == EBU_ABORT)
				{              
					if (Alert(GetWndParent(), MB_ICONEXCLAMATION | MB_OKCANCEL, STR_ABORT_SETUP)==IDOK)
					{
						//
						//Set this to get out of the file copy loop and then break out of the
						//inner loop
						//
						retc = EBU_ABORT;
						FreeMemory(lpCurrent->szSource);
						FreeMemory(lpCurrent->szDest);
						FreeMemory(lpCurrent);
						lpCurrent = NULL;
						return -1;
					}
				}
				
				//
				//For a user confirmed abort or for a cancel return from copy proc
				//
				if (retc == EBU_CANCEL)
				{
					FreeMemory(lpCurrent->szSource);
					FreeMemory(lpCurrent->szDest);
					FreeMemory(lpCurrent);
					lpCurrent = NULL;
					return -1;
				}
			}
			
			ZeroMemory(tempName, _MAX_PATH);
			
			char tempPath[_MAX_PATH];
			if(!access(lpCopy->fs->szDest,0))
			{
				lstrcpy(tempName,lpCopy->fs->szDest);
				char *ptr = pszGetLast5C(tempName);
				if(ptr != NULL)
				{
					ptr = CharNext(ptr);
					*ptr = '\0';
				}
				
				GetTempFileName(tempName,"cjh",0,tempPath);
				lstrcpy(tempName,tempPath);
				fAlreadyExists = TRUE;

				if ( lpCurrent->wFlags & IF_FONTFILE )
					// free the font so that the file copy can complete successfully
					RemoveFontResource (lpCopy->fs->szDest);

			}
			
			if (EnsureCDROMInserted())
			{
				int	handle;
				handle = file_open(
					((fAlreadyExists) ? tempName : lpCopy->fs->szDest),
					_O_BINARY | _O_CREAT | _O_WRONLY | _O_SEQUENTIAL,
					_S_IREAD | _S_IWRITE
					);
				return handle;
			}
			else
			{
				retc = EBU_ABORT;
				return -1;
			}
		}
		
	case fdintCLOSE_FILE_INFO:	// close the file, set relevant info
        {
            HANDLE  EBUhandle;
            DWORD   attrs;
			
			file_close(pfdin->hf);

			
			
            /*
			* Set date/time
			*
			* Need Win32 type handle for to set date/time
			*/
            EBUhandle = EBUCreateFile(
				((fAlreadyExists) ? tempName : lpCopy->fs->szDest),
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
				);
			
            if (EBUhandle != INVALID_HANDLE_VALUE)
            {
                FILETIME datetime;
				
                if (TRUE == DosDateTimeToFileTime(
                    pfdin->date,
                    pfdin->time,
                    &datetime))
                {
                    FILETIME    local_filetime;
					
                    if (TRUE == LocalFileTimeToFileTime(
                        &datetime,
                        &local_filetime))
                    {
                        (void) SetFileTime(
                            EBUhandle,
                            &local_filetime,
                            NULL,
                            &local_filetime
							);
					}
                }
				
                CloseHandle(EBUhandle);
            }
			
            /*
			* Mask out attribute bits other than readonly,
			* hidden, system, and archive, since the other
			* attribute bits are reserved for use by
			* the cabinet format.
			*/
            attrs = pfdin->attribs;
			
            attrs &= (_A_RDONLY | _A_HIDDEN | _A_SYSTEM | _A_ARCH);
			
            (void) SetFileAttributes(
                ((fAlreadyExists) ? tempName : lpCopy->fs->szDest),
                attrs
				);
			if(fAlreadyExists && retc != EBU_ABORT)
			{
				UINT
					uVersionCompare;
				
				FILEINFO
					DestFileInfo,
					SourceFileInfo;
				
				uVersionCompare = VersionCompare( tempName, &SourceFileInfo, lpCurrent->szDest, &DestFileInfo, FALSE); // hit the file for version info
				retc = EBU_OK;
				
				if( !(uVersionCompare & VC_VER_GREATER) )
				{
					DeleteFile(lpCopy->fs->szDest);
					rename(tempName,lpCopy->fs->szDest);
				}
				else
					DeleteFile(tempName);
			}
			
			if (lpCurrent->wFlags & IF_FONTFILE)
			{
				char	lpszVersion[256];
				char	lpszFontname[256] = " ";
				char	szTemp[16];
				LPSTR	lpStr = pszGetLast5C(lpCopy->fs->szDest);

				SetDirtyBits(DIRTY_INSTALLFONT);
	
				//
				//convert filename to lowercase so can match against lowercase .ttf.  BUGBUG:REVIEW:
				//will hardcoded string .ttf be the same in other languages?
				//
				CharLower(lpStr);
				if (NULL != strstr(lpStr, ".ttf"))
				{
					// get fontName so we can store it in the registry later
					if (!ReadTTFInfo(lpCopy->fs->szDest, lpszVersion, lpszFontname))
					{
#ifdef _DEBUG
						TRACE(STR_HARDCODE_NOSOURCEFONT,lpCopy->fs->szDest);
#endif
						retc = EBU_ERROR;
						goto DoneHere;
					}

					// If we didn't retrieve our fontName from the file, then let's use the fontFile's name
					if (' ' == lpszFontname[0])   
					{
						lstrcpy(lpszFontname, &lpStr[1]);
					}
	
					// find the registry information for fonts
					char szRegistryKey [_MAX_PATH];
					if (GetOS() & OS_NTMASK)
					{
						//NT gets font information from different key than WIN40
						EBULoadString(GetResourceInst(), STR_REGKEY_NT_FONTS, szRegistryKey,
							sizeof (szRegistryKey));
					}
					else
					{
						EBULoadString(GetResourceInst(), STR_REGKEY_WIN40_FONTS, szRegistryKey,
							sizeof (szRegistryKey));
					}
		
					HKEY hkFonts;
					if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, szRegistryKey, 0,
						KEY_SET_VALUE, &hkFonts) != ERROR_SUCCESS)
					{
						retc = EBU_ERROR;
						goto DoneHere;
					}
		
					EBULoadString(GetResourceInst(), STR_TRUETYPE, szTemp, 16);
					lstrcat(lpszFontname, szTemp);
		
					// add this font to the registry
					RegSetValueEx (hkFonts, lpszFontname, 0, REG_SZ,
						(BYTE FAR *) lpStr, lstrlen (lpStr) + 1);
					RegCloseKey (hkFonts);
		
					// also install the font for immediate use
					AddFontResource (lpCopy->fs->szDest);
					SendMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);
				}
				else	// not .ttf file
					ASSERT (false);
			}
DoneHere:			if (lpCurrent->wFlags & IF_DLLREGISTER)
			{
				// Register the DLL if the copy was successful and if flagged to do so...
				DLLRegister(lpCopy->fs->szDest, DO_INSTALL);
			}

			FreeMemory(lpCurrent->szSource);
			FreeMemory(lpCurrent->szDest);
			FreeMemory(lpCurrent);
			lpCurrent = NULL;
			ZeroMemory(tempName,_MAX_PATH);
			
			if(retc != EBU_OK)
				return FALSE;
			else
				return TRUE;
        }
		
	case fdintNEXT_CABINET:	// file continued to next cabinet
		return 0;
	}
	
	return 0;
}


EBURETCODE extract_files(char *cabinet_name)
{
	HFDI			hfdi;
	ERF				erf;
	FDICABINETINFO	fdici;
	int				hf;
	FILECOPYSTATUS fs;
	COPYLIST Copy;
	Copy.fs = &fs;
	Copy.lpfn = GetAppCallback();
	
	char cabinet_fullpath[_MAX_PATH];
	char cabname[_MAX_PATH];
	char cabpath[_MAX_PATH];
	Copy.cabname = &cabname[0];
	
	char *ptr;
	
	if (IsDBCS())
	{
		ptr = cabinet_name + lstrlen(cabinet_name);
		do {
			ptr = CharPrev(cabinet_name, ptr);
		} while(*ptr != '\\' && ptr != cabinet_name);
	}
	else
	{
		ptr = cabinet_name + lstrlen(cabinet_name)-1;
		while(*ptr != '\\' && ptr != cabinet_name)
			ptr--;
	}
	
	if(*ptr == '\\' || ptr != cabinet_name)
		ptr++;
    lstrcpy(cabname,ptr);
    GetCurrentDirectory(_MAX_PATH,cabinet_fullpath);
	lstrcpy(cabpath,cabinet_fullpath);
	
	if (IsDBCS())
	{
		if(*(CharPrev(cabpath, cabpath + lstrlen(cabpath))) != '\\')
		{
			lstrcat(cabpath, "\\");
		}
	}
	else
	{
		if(*(cabpath + lstrlen(cabpath)-1) != '\\')
		{
			lstrcat(cabpath,"\\");
		}
	}
	
	if (IsDBCS())
	{
		if(*(CharPrev(cabinet_fullpath, cabinet_fullpath + lstrlen(cabinet_fullpath))) != '\\')
		{
			lstrcat(cabinet_fullpath, "\\");
		}
	}
	else
	{
		if(*(cabinet_fullpath + lstrlen(cabinet_fullpath)-1) != '\\')
		{
			lstrcat(cabinet_fullpath,"\\");
		}
	}
	
	lstrcat(cabinet_fullpath,cabname);
	if(*ptr == '\\')
	{
		*ptr = '\0';
		lstrcat(cabpath,cabinet_name);
		lstrcat(cabpath,"\\");
	}
	LPINSTALLLIST traverse = GetListHead();
	do
	{
		fs.dwTotalSize+=traverse->dwFileSize;
		traverse=traverse->nextElement;
		
	} while(traverse != NULL);
	
	
	hfdi = FDICreate(
		mem_alloc,
		mem_free,
		file_open,
		file_read,
		file_write,
		file_close,
		file_seek,
		cpu80386,
		&erf
		);
	
	if (hfdi == NULL)
	{
		return EBU_ERROR;
	}
	
	
	if (!EnsureCDROMInserted())
	{
		return EBU_ABORT;
	}
	
	/*
	* Is this file really a cabinet?
	*/
	hf = file_open(
		cabinet_fullpath,
		_O_BINARY | _O_RDONLY | _O_SEQUENTIAL,
		0
		);
	
	if (EBU_ABORT == GetResultCode())
	{
		return EBU_ABORT;
	}
	else
	{
		if (hf == -1)
		{
			(void) FDIDestroy(hfdi);
			
			return EBU_ERROR;
		}
	}
	
	if (FALSE == FDIIsCabinet(
		hfdi,
		hf,
		&fdici))
	{
	/*
	* No, it's not a cabinet!
		*/
		file_close(hf);
		
		(void) FDIDestroy(hfdi);
		return EBU_ERROR;
	}
	else
	{
		file_close(hf);
		
	}
	
	if (TRUE != FDICopy(
		hfdi,
		cabname,
		cabpath,
		0,
		notification_function,
		NULL,
		(void *)&Copy))
	{
		if(NULL != GetListHead())
		{
			
			LPINSTALLLIST	traverse=GetListHead();
			LPINSTALLLIST	lpLast;
			while (traverse)
			{
				lpLast = traverse;
				FreeMemory(traverse->szSource);
				FreeMemory(traverse->szDest);
				traverse = traverse->nextElement;
				FreeMemory(lpLast);
				lpLast = NULL;
			}
			SetListHead(NULL);
			SetListEnd(NULL);
		}
		
		if(lpCurrent)
		{
			FreeMemory(lpCurrent->szSource);
			FreeMemory(lpCurrent->szDest);
			FreeMemory(lpCurrent);
			lpCurrent = NULL;
		}
		
		if(erf.erfOper == FDIERROR_TARGET_FILE && erf.fError == TRUE && retc != EBU_ABORT)
		{
			
			
			Alert(GetWndParent(), MB_OK | MB_ICONEXCLAMATION, STR_ERROR_NODISKSPACE);
			retc = EBU_ABORT;
		}
		
		(void) FDIDestroy(hfdi);
		if(*tempName)
		{
			DeleteFile(tempName);
		}
		return retc;
	}
	
	if (FDIDestroy(hfdi) != TRUE)
	{
		return EBU_ERROR;
	}
	
	fs.dwTotalCopied = fs.dwTotalSize;
	fs.fDone = TRUE;
	
    retc = (*(GetAppCallback())) ((void *) &fs);
	
	if(NULL != GetListHead()) // if optional install files, such as DirectX tree are not present say
	{                    // as in a trial version, then list isn't clean, clear it out.
		LPINSTALLLIST	lpLast=NULL;
		LPINSTALLLIST	traverse=GetListHead();
		while (traverse)
		{
			lpLast = traverse;
			FreeMemory(traverse->szSource);
			FreeMemory(traverse->szDest);
			traverse = traverse->nextElement;
			FreeMemory(lpLast);
			lpLast = NULL;
		}
		// Nullify the Global copy of the list pointer
		SetListHead(NULL);
	}
	
	return retc;
}


