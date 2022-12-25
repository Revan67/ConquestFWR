//
// pid.cpp
//
//    Generates a partly-random Product ID number to display in the
//    about box.
//
// History:
//
//     3/21/95 KenSh    Created, borrowing code from ACME
//     7/21/95 AjayJ    Updated for PID 2.0
//                      xxxxx-xxx-xxxxxxC-xxxx (23 characters)
//                      5 digit RPC code
//                      3 digit site code (random)
//                      6 digit serial number (random) + check digit
//                      4 digit user random number
//     8/02/95 AjayJ    Fixed comment on thunk call to GlobalMemoryStatus
//     8/31/95 KenSh    Changed middle 3 digits of PID 2.0 to be fixed
//                      (loaded from resource instead of randomly generated).
//
#include "stubpch.h"
#include "HotSetupRC.h"
#include "hotsetup.h"
#include "setup.h"
#include "registry.h"
#include "util.h"
#include "pid.h"

using namespace NGLOBALS;

//----------------------------------------------------------------------------
// Procedure   GenerateAndStorePID
//
// Purpose     Determines if a PID already exists in the .ini file /
//             registry.  If it does not exist, one is generated and then
//             added.
//
// Parameters  none
//
// Returns     nonzero if a PID was generated; zero if not.
//
// History      3/21/95 KenSh     Created
//
BOOL GenerateAndStorePID(LPGETPIDDATA pid)
{
	char  szPidKey[128];
	TCHAR szPID[PID20LENGTH + 1];
	UINT  cch;
	DWORD SiteCode, ProductID, SerialNum;
	BOOL  fPIDExists = TRUE;
	
	//
	//Copy from PID stored in memory, if any yet...
	//
	lstrcpy(szPID, GetPid());

	//
	//If no PID in globals, read it from the registry...
	//
	if ('\0' == *szPID)
	{
		EBULoadString(GetResourceInst(), STR_REGKEY_VAL_PID, szPidKey, sizeof(szPidKey));
		fPIDExists = MyGetPrivateProfileString(szPidKey, "", szPID, sizeof(szPID));
	}

	//
	//If PID already exists
	//
	if (fPIDExists)
	{
		//
		// Now check if the RPC is still valid.  1/5/96 WilliamW
		//
		char szOriginalPID[PID20LENGTH];
		
		// Set all digits in the PID to zero
		ZeroMemory(szOriginalPID, sizeof(szOriginalPID));
		
		//
		//Load the 5-digit RPC part of the pid (position 0-4)
		//
		cch = EBULoadString(GetResourceInst(), STR_PRODUCTRPC, szOriginalPID, sizeof(szOriginalPID));
		ASSERT (5 == cch);

		//If RPC part is still valid, then we are done.  Otherwise,
		//do normal PID generation.
		if (!_tcsncmp(szOriginalPID, szPID, 5))
		{
			SetPid(szPID);

			return FALSE;  // we didn't generate or write a PID
		}
	}

	SerialNum = atol(pid->SerialNumber);
	ProductID = atol(pid->ProductID);
	SiteCode = atol(pid->SiteCode);
	
	if (0 == SiteCode && (IsOEM() || pid->fOEMPid))
	{
		//
		//Flag that we have an OEM PID
		//
		SiteCode = -1;
	}
		
	//
	//Generate the new PID
	//
	GeneratePID(szPID, sizeof(szPID), SiteCode, ProductID, SerialNum);

	SetPid(szPID);

	return TRUE;
}

// History
void GeneratePID(char *pszBuf, UINT cchBuf, DWORD SiteCode, DWORD ProductID, DWORD SerialNum)
{
	int   cch;
	char  szName[128];
	int   j;
	int   nNameLen;
	DWORD dwTickCount=GetTickCount();

	//
	//Buffer must be at least PID20LENGTH bytes long.
	//
	ASSERT (PID20LENGTH <= cchBuf);

	//
	//Set all digits in the PID to zero
	//
	ZeroMemory(pszBuf, cchBuf);
	
	//
	//Load the RPC part of the PID (position 0-4)
	//
	cch = EBULoadString(GetResourceInst(), STR_PRODUCTRPC, pszBuf, cchBuf );
	ASSERT(cch==5);

	//
	//Add the hyphen (position 5)
	//
	pszBuf[5] = '-';

	//
	//If an existing or user input product ID was passed in, use it.
	//
	if (ProductID != 0)
	{
		//
		//If an OEM build, put "OEM" in sitecode field.  -1 is passed in for
		//SiteCode if an OEM PID is to be generated...
		//
		if (-1 == SiteCode)
		{
			cch = EBULoadString(GetResourceInst(), STR_PID_OEM, pszBuf + 6, cchBuf - 6);
			ASSERT(3 == cch);  // "OEM"
		}
		else
		{
			//
			//If sitecode also passed in, use it...
			//
			if (SiteCode != 0)
			{
				wsprintf(pszBuf + 6, "%03u", SiteCode);
			}
			else
			{
				//
				//Otherwise, default to resource site code (usually 442)
				//
				cch = EBULoadString(GetResourceInst(), STR_PID_SITECODE, pszBuf + 6, cchBuf - 6);

				SiteCode = atol(&pszBuf[6]);
				ASSERT(SiteCode);
			}
		}

		//
		//Now append the passed in product ID...
		//
		pszBuf[9] = '-';
		wsprintf(&pszBuf[10], "%07u", ProductID);
	}
	else
	{
		//
		//If no product ID passed in, and if this is an OEM build, load the
		//OEM site code ("OEM") and product ID (stored in resource file
		//as one string of format OEM-xxxxxxx.
		//
		if (-1 == SiteCode)
		{
			//
			//If OEM PID, load OEM-xxxxxxx (SiteCode-ProductID)
			//
			cch = EBULoadString(GetResourceInst(),
								STR_PRODUCTOEMRPC,
								pszBuf + 6,
								cchBuf - 6);

			//
			//We've got a valid site code and product ID now...
			//
			ProductID = atol(&pszBuf[10]);
			ASSERT(ProductID);
		}
		else
		{
			//
			//Otherwise, load site code (442)
			//
			cch = EBULoadString(GetResourceInst(), STR_PID_SITECODE, pszBuf + 6, cchBuf - 6);
			pszBuf[9] = '-';

			SiteCode = atol(&pszBuf[6]);
			ASSERT(SiteCode);
		}
	}

	ASSERT('-' == pszBuf[9]);

	//
	//In the case of an OEM pid, we now have the RPC-SITECODE-PRODUCTID portion of
	//the PID completed.  For a non-OEM pid, we have the RPC-SITECODE portion
	//completed and MAYBE the PRODUCTID, but only if it was passed in...
	//

	//
	//Need a name to use for randomization function, generate one from 7 to 17 letters long
	//
	nNameLen = (int) (dwTickCount % 11) + 7;
	for (j=0; j < nNameLen; ++j)
	{
		if (0 == dwTickCount)
		{
			dwTickCount = (GetTickCount() >> j) ^ (GetTickCount() << j);
		}
		
		szName[j] = 'a' + (char) (dwTickCount % 26);
		dwTickCount /= 10;
	}
	szName[j] = 0;

	//
	//Generate random product ID (position 10-16) and user code (18-22).  If Product ID
	//already has been set (via user input or from STR_PRODUCTOEMRPC resource, then
	//only generate the user code...
	//
	RandomizePID (pszBuf, PID20LENGTH, szName, 0 == ProductID ? TRUE : FALSE);
	
	//
	//Add the hyphen at position 17
	//
	pszBuf[17] = '-';
	
	//
	//Terminate PID string
	//
	pszBuf[23] = 0;
}

/*
** Purpose:
**    Randomize last 5 digits of formatted PID string.
** Arguments:
**    szPid - buffer in which to write the random digits
**    cbFormattedPID - length of the buffer (should be PID20LENGTH)
**    szName - buffer in which application name is passed to help with randomizer
**    fRandomizeSerial: fTrue for 7+1+5 random digits, fFalse for last 5
**
** 7/21/95 AjayJ  Taken directly from module CPYDISRW.CPP from ACME 2.0
****************************************************************************/
void RandomizePID(char *szPid, UINT cbFormattedPID, char *szName, BOOL fRandomizeSerial)
{
	char		 *pch;
	MEMORYSTATUS memstat;
	DWORD		 dwRandom;
	DWORD		 dwCount = GetTickCount();
	
	if (fRandomizeSerial)
	{
		unsigned long i,j;
		
		ZeroMemory(&memstat, sizeof(MEMORYSTATUS));
		memstat.dwLength = sizeof(MEMORYSTATUS);
		GlobalMemoryStatus((LPMEMORYSTATUS) &memstat);
		
        dwRandom = (DWORD) ((((DWORD)(LPSTR)szName << 9)
							^ (memstat.dwAvailPhys << 5)
							^ (memstat.dwAvailVirtual))
							% 999999 + 1);
		
		//
		//Insert randomized Product ID
		//
		wsprintf(szPid + (cbFormattedPID-7-1-5), "%06ld", dwRandom);

		//
		//Generate and store checksum digit...
		//
		for (j = 7, i = cbFormattedPID-7-1-5; i < cbFormattedPID-1-1-5; i++)
		{
			j += szPid[i] - '0';
		}
		szPid[i] = (char)('7' - j % 7);
	}

	//
	//Generate random user code...
	//
	for (pch = szName; *pch; pch++)
	{
		dwCount = (dwCount << 1) + *pch;
	}
	dwCount = dwCount % 99999 + 1;

	//
	//Insert randomized user code...
	//
	wsprintf(szPid + (cbFormattedPID-5), "%05ld", dwCount);
}