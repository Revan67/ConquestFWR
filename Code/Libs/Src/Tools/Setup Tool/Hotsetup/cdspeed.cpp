#include "hotsetup.h"
#include "setup.h"
#include "util.h"
#include "hotsetuprc.h"

//
//CDSpeed vars
//
#define _BUFSIZE  1024 * 1024
#define SECTORSIZE 2048
#define uCYCLES (UINT) 4
#define uITERATIONS (UINT) 10
#define uFRAMERATE (UINT) 15

static DWORD WINAPI CheckSpeed(DWORD *, LPCDSPEEDDATA);
static DWORD WINAPI CheckPerfSpeed(DWORD *lpdwCDSpeed);
static int          Profile(LONGLONG, int, LPCDSPEEDDATA);

static BOOL fCPUProfileDone = FALSE;

using namespace NGLOBALS;
__inline LONGLONG ConvertToLL(LARGE_INTEGER *li)
{
	return ((LONGLONG) li->HighPart << 32 | (LONGLONG) li->LowPart);
}

typedef struct lpcdperftag
{
	LPCDSPEEDDATA cddata;
	LPCDSPEED cdspeed;
} CDPERF, *LPCDPERF;

EBURETCODE ExecuteCDSpeed(LPCDSPEED lpCDSpeed)
{
	LONGLONG lfreq=0;
	LARGE_INTEGER freq;
	DWORD  result;
	DWORD  threadres;
	DWORD  tID;
	HANDLE hThread;
	CDSPEEDDATA cdsd;
	CDPERF cdperf;
	EBURETCODE retc = EBU_OK;
	DWORD  minSpeed;
	double maxcpu;
	DWORD  status;
	HANDLE hProcess;
	int    priority;

	cdperf.cdspeed = lpCDSpeed;
	cdperf.cddata = &cdsd;

	minSpeed = lpCDSpeed->GetCDSpeedMinCD();
	maxcpu   = (double) lpCDSpeed->GetCDSpeedMaxCPU();

	//
	//Callback the setup app so it can, for example, present a dialog asking 
	//the user if they want to check CPU usage and CDROM data xfer rate...
	//
	cdsd.cdstatus = CDASK;
	retc = (*(GetAppCallback())) ((void *) &cdsd);

	ASSERT(EBU_OK == retc || EBU_ABORT == retc || EBU_CANCEL == retc || EBU_BACK == retc);

	if (EBU_OK != retc)
	{
		return retc;
	}

	//
	//Get the granularity of the performance counter...
	//
	QueryPerformanceFrequency(&freq);
	lfreq = ConvertToLL(&freq);

	if ((LONGLONG) 0 == lfreq)
	{
		return EBU_ERROR;
	}

	cdsd.nAbortCode = EBU_OK;

	//
	//Callback the setup app so it can, for instance, put up a dialog stating that
	//we're checking CPU usage during CDROM data transfer...
	//
	cdsd.cdstatus = CDDISPLAYSTATUS;
	retc = (*(GetAppCallback())) ((void *) &cdsd);

	ASSERT(EBU_OK == retc || EBU_ABORT == retc || EBU_CANCEL == retc);

	if (EBU_OK != retc)
	{
		return retc;
	}

	ForwardMessages();

	//
	//Only do data transfer test if it's important...
	//
	if (0 < minSpeed)
	{
		//
		//Check CDROM data transfer rate...
		//
		CheckSpeed((DWORD *) lpCDSpeed->GetCDSpeedFileName(), &cdsd);

		ASSERT(EBU_OK == cdsd.nAbortCode || EBU_ABORT == cdsd.nAbortCode || EBU_CANCEL == cdsd.nAbortCode);

		if (EBU_OK != cdsd.nAbortCode)
		{
			return cdsd.nAbortCode;
		}

		if (cdsd.avg_speed < minSpeed)
		{
			cdsd.cdstatus = CDCDROMFAIL;

			retc = (*(GetAppCallback())) ((void *) &cdsd);

			ASSERT(EBU_OK == retc || EBU_ABORT == retc || EBU_CANCEL == retc || EBU_BACK == retc);

			if (EBU_OK != retc)
			{
				return retc;
			}
		}
	}

	if (maxcpu == 100 || maxcpu == 0)
	{
		goto OkeyDokey;
	}

	hProcess = GetCurrentThread();
	priority = GetThreadPriority(hProcess);

	SetThreadPriority(hProcess, THREAD_PRIORITY_ABOVE_NORMAL);

	//
	//Create a separate thread to do the CDROM transfer performance check
	//
	hThread = CreateThread(NULL,
						   0,
						   (LPTHREAD_START_ROUTINE) CheckPerfSpeed,
						   &cdperf,
						   CREATE_SUSPENDED,
						   &tID);

	if (NULL == hThread)
	{
		return EBU_ERROR;
	}

	SetThreadPriority(hThread, THREAD_PRIORITY_ABOVE_NORMAL);

	//
	//Check CPU speed when CDROM drive is NOT being accessed to get a baseline
	//
	result = Profile(lfreq, 5, &cdsd);
	ForwardMessages();

	//
	//If user pressed Cancel or otherwise aborted from app space, return
	//
	ASSERT(EBU_OK == cdsd.nAbortCode || EBU_ABORT == cdsd.nAbortCode || EBU_CANCEL == cdsd.nAbortCode);
	if (EBU_OK != cdsd.nAbortCode)
	{
		fCPUProfileDone = TRUE;
		ResumeThread(hThread);
		ForwardMessages();

		return cdsd.nAbortCode;
	}

	//
	//Now start the CD performance check
	//
	ResumeThread(hThread);

	//
	//Check CPU usage while CDROM drive is being accessed...
	//
	threadres = Profile(lfreq, 5, &cdsd);

	//
	//Now let the CDROM drive accessor thread know that we're done measuring...
	//
	fCPUProfileDone = TRUE;

	//
	//Wait for the CDROM drive thread to terminate now...
	//
	status = STILL_ACTIVE;
	while (status == STILL_ACTIVE)
	{
		GetExitCodeThread(hThread, &status);
	}

	//
	//Reset to previous priority
	//
	SetThreadPriority(hProcess, priority);

	cdsd.mincpu = (1.0 - ((double) threadres / (double) result)) * 100.0;

	if (cdsd.nAbortCode != EBU_OK)
	{
		return cdsd.nAbortCode;
	}

	if (cdsd.mincpu > maxcpu)
	{
		cdsd.cdstatus = CDCPUFAIL;

		retc = (*(GetAppCallback())) ((void *) &cdsd);

		ASSERT(EBU_OK == retc || EBU_ABORT == retc || EBU_CANCEL == retc || EBU_BACK == retc);

		if (EBU_OK != retc)
		{
			return retc;
		}
	}

OkeyDokey:
	cdsd.cdstatus = CDOK;
	retc = (*(GetAppCallback())) ((void *) &cdsd);

	ASSERT(EBU_OK == retc || EBU_ABORT == retc || EBU_CANCEL == retc || EBU_BACK == retc);

	return retc;
}

LONGLONG GetCounter()
{
   LARGE_INTEGER l;

   QueryPerformanceCounter(&l);

   return ((LONGLONG) l.HighPart<<32 | (LONGLONG) l.LowPart);
}

static DWORD WINAPI CheckSpeed(DWORD *filename, LPCDSPEEDDATA cddata)
{
	HANDLE hFile = INVALID_HANDLE_VALUE;
	char   *buf = NULL;
	UINT   x,y;
	DWORD  tickstart,tickend,tickavg=0;
	DWORD  BytesRead;
	DWORD  lowSize,highSize;
	double speed,accum;
	char   szSource[_MAX_PATH];
	DWORD  lpBytesPerCluster;	// address of bytes per Cluster
	DWORD  lpNumberOfFreeClusters;	// address of number of free clusters
	DWORD  wResult = 0;

#ifdef _DEBUG
	if(((char *)filename)[1] != ':')
		GetModuleDirectory( szSource, sizeof(szSource) );
	else
		*szSource = '\0';
#else
		GetModuleDirectory( szSource, sizeof(szSource) );
#endif

#ifdef _DEBUG
	if (IsDBCS())
	{
		ASSERT( (*pszGetLast5C(szSource)=='\\' && *(char *)filename!='\\') ||
				(*pszGetLast5C(szSource)!='\\' && *(char *)filename=='\\')    );
	}
#endif

	lstrcat(szSource, (char *) filename);

	while (TRUE)
	{
		hFile = EBUCreateFile(szSource,GENERIC_READ,0,NULL,
						   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING,
						   NULL);

		if (hFile == INVALID_HANDLE_VALUE)
		{
			if (Alert(GetWndParent(), MB_ICONEXCLAMATION | MB_OKCANCEL, STR_ERROR_CANTFINDCD) == IDCANCEL)
			{
				if (Alert(GetWndParent(), MB_ICONEXCLAMATION | MB_OKCANCEL, STR_ABORT_SETUP)==IDOK)
				{
					cddata->nAbortCode = EBU_ABORT;
					wResult = IDABORT;
					goto Done;
				}
			}

			continue;
		}
		else
		{
			break;
		}
	}

	ASSERT(INVALID_HANDLE_VALUE != hFile);

	lowSize = GetFileSize(hFile,&highSize);

	if (!FNewMemory((void **)&buf, _BUFSIZE))
	{
		wResult = 1;
		goto Done;
	}

	accum = 0.0;
	szSource[3] = '\0';
	MyGetDiskFreeSpace(szSource,
					   &lpBytesPerCluster,
					   &lpNumberOfFreeClusters);

	for (y=0; y < uCYCLES; y++)
	{
		SetFilePointer(hFile,(-1) * ((LONG)lpBytesPerCluster), NULL, FILE_END);
		EBUReadFile(hFile, buf, _BUFSIZE, &BytesRead, NULL);
		SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

		for (;;)
		{
			ForwardMessages();

			if (EBU_OK != cddata->nAbortCode || fCPUProfileDone)
			{
				goto Done;
			}

			tickstart = GetTickCount();

			for (x = 0; x < uITERATIONS; x++)
			{
				EBUReadFile(hFile, buf, _BUFSIZE, &BytesRead, NULL);

				if((x+2) * _BUFSIZE > lowSize)
				{
					SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
				}

				if (BytesRead != _BUFSIZE || EBU_OK != cddata->nAbortCode || fCPUProfileDone)
				{
					wResult = 1;
					goto Done;
				}

			}

			tickend = GetTickCount();
			ForwardMessages();

			if (EBU_OK != cddata->nAbortCode)
			{
				wResult = 1;
				goto Done;
			}

			if (tickend > tickstart)
				break;
		}

		if (EBU_OK != cddata->nAbortCode || fCPUProfileDone)
		{
			wResult = 1;
			goto Done;
		}

		tickavg = tickend - tickstart;
		speed = (double) _BUFSIZE * (double) uITERATIONS / 1024.00;
		speed *= (1000.00 / (double) tickavg);
		accum += speed;
	}

	cddata->avg_speed = (DWORD) (accum / (double) uCYCLES);

Done:
	if (buf)
		FreeMemory(buf);

	if (INVALID_HANDLE_VALUE != hFile)
		CloseHandle(hFile);

	return wResult;
}

int Profile(LONGLONG lfreq, int length, LPCDSPEEDDATA cd)
{
    LONGLONG start, end, fullstart, period;
    int      lp;
    int      bob;
    LONGLONG fred;
    DWORD    result = 0;
    int      count = 0;
    int      ralph;
    int      counter, fullcounter = 0;

    end = 0;
    fullstart = GetCounter();

    for (lp=1; lp<=length; lp++)
    {
        counter = 0;
        start = GetCounter();

        while ((GetCounter() - start) < lfreq)
        {
			//
			//Use the CPU in a few different ways
			//
            counter++;
            bob = counter * 13;     
            fred = start - 19;  
            ralph = lp / counter;
            fullcounter++;
        }

        period = (GetCounter() - start);
        end += period;

        ForwardMessages();

        if (EBU_OK != cd->nAbortCode)
		{
            return 0;
		}
    }

    result += (int) fullcounter / (int) (end / lfreq);

    return result;
}

DWORD WINAPI CheckPerfSpeed(DWORD *lpdwCDSpeed)
{
	LPCDSPEEDDATA cd = ((LPCDPERF) lpdwCDSpeed)->cddata;
	LPCDSPEED     lpCDSpeed = ((LPCDPERF) lpdwCDSpeed)->cdspeed;

	HANDLE hFile;
	char   *buf = NULL;
	UINT   x, y;
	DWORD  tickstart,tickend,tickavg=0,tickdur;
	DWORD  minSpeed = lpCDSpeed->GetCDSpeedMinCD();
	DWORD  BytesRead;
	DWORD  lowSize,highSize;
	char   szSource[_MAX_PATH];
	UINT   itercycles;
	char   *filename;
	DWORD  wReturn = 0;

	DWORD  lpBytesPerCluster; // address of bytes per Cluster
	DWORD  lpNumberOfFreeClusters;	// address of number of free clusters

	//
	//Initialize wacky error count to zero
	//
	cd->nCDErrCount = 0;

	itercycles = (minSpeed * 1024) / SECTORSIZE / uFRAMERATE + 1;
	filename = lpCDSpeed->GetCDSpeedFileName();

#ifdef _DEBUG
	if (filename[1] != ':')
		GetModuleDirectory( szSource, sizeof(szSource) );
	else
		*szSource = '\0';
#else
	GetModuleDirectory(szSource, sizeof(szSource));
#endif

	lstrcat(szSource, (char *) lpCDSpeed->GetCDSpeedFileName());

	while (TRUE)
	{
		hFile = EBUCreateFile(szSource, GENERIC_READ, 0, NULL,
						   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING,
						   NULL);

		if (INVALID_HANDLE_VALUE == hFile)
		{
			if (Alert(GetWndParent(), MB_ICONEXCLAMATION | MB_OKCANCEL, STR_ERROR_CANTFINDCD) == IDCANCEL)
			{
				if (Alert(GetWndParent(), MB_ICONEXCLAMATION | MB_OKCANCEL, STR_ABORT_SETUP)==IDOK)
				{
					cd->nAbortCode = EBU_ABORT;

					ExitThread(EBU_ABORT);
				}
			}

			continue;
		}
		else
		{
			break;
		}
	}

	lowSize = GetFileSize(hFile, &highSize);

	if (!FNewMemory((void **) &buf, SECTORSIZE))
	{
		wReturn = 1;
		goto Done;
	}

	szSource[3] = '\0';
	MyGetDiskFreeSpace(szSource,
					   &lpBytesPerCluster,
					   &lpNumberOfFreeClusters);

	for (y=0; y < uFRAMERATE; y++)
	{
		if (fCPUProfileDone)
		{
			wReturn = 1;
			goto Done;
		}

		SetFilePointer(hFile, (-1) * ((LONG) lpBytesPerCluster), NULL, FILE_END);
		EBUReadFile(hFile, buf, SECTORSIZE, &BytesRead, NULL);
		SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

		for (;;)
		{
			tickstart = GetTickCount();

			for (x=0; x < itercycles; x++)
			{
				EBUReadFile(hFile, buf, SECTORSIZE, &BytesRead, NULL);

				if ((x+2) * SECTORSIZE > lowSize)
				{
					SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
				}

				if (BytesRead != SECTORSIZE || fCPUProfileDone || cd->nAbortCode != EBU_OK)
				{
					wReturn = 1;
					goto Done;
				}
			}

			tickend = GetTickCount();
			tickdur = tickend-tickstart;

			if (tickdur > 66)
			{
				cd->nCDErrCount++;
			}
			else
			{
				if (tickdur < 66)
				{
					ForwardMessages();

					if (EBU_OK != cd->nAbortCode || fCPUProfileDone)
					{
						goto Done;
					}

					if ((tickdur = GetTickCount() - tickstart) < 66)
					{
						Sleep(66 - tickdur);
					}
				}
			}
		}
	}

Done:
	if (buf)
		FreeMemory(buf);

	CloseHandle(hFile);
	ExitThread(0);

	return 0;
}