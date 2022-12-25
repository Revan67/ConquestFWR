//////////////////////////////////////////////////////////////////////////////
//  File:    ConsoleEmbed.cpp
//
//  Purpose: Implements the entry point for the console application.
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2001 by Humongous Entertainment
//  
//  This library is free software; you can redistribute it and/or modify it 
//  under the terms of the GNU Lesser General Public License as published by 
//  the Free Software Foundation; either version 2.1 of the License, or  any 
//  later version.
//  
//  This library is distributed in the hope that it will be useful, but 
//  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
//  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public 
//  License for more details.
//  
//  You she hope that it will be useful, but 
//  WITHOUT ANY WARRANTY; withouould have received a copy of the GNU Lesser General Public License 
//  along with this library (see the file license.txt); if not, write to the 
//  Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//////////////////////////////////////////////////////////////////////////////	


#include "stdafx.h"
#include "ConsoleEmbed.h"
#include "hapdbg/hapdbg.h"




void ParseParams(int argc, char* argv[], int& nPort);
const char* BuildImageDate();



//////////////////////////////////////////////////////////////////////////////
// Handledmain
//
// This is the basic main function except that it is called from main within
// a try except block to enable better exception handling
int Handledmain(int argc, char* argv[])
{
	printf("Hap Client ver: 4.1 (");
	printf(BuildImageDate());
	printf(")\n");

	printf("Python ver: ");
	printf(Py_GetVersion());
#ifdef _DEBUG
	printf(" - DEBUG VERSION");
#endif
	printf("\n");

	int nPort = HE_DEBUGGER_PORTNUM;

	ParseParams(argc, argv, nPort);

	//initialize python
	Py_Initialize();
	
	if ( ! hapdbg::Initialize(nPort))
	{
		goto PYTHON_EXIT;
	}

	try	
	{
		if (hapdbg::WaitForConnection())
		{
			string script, dir, params;

			hapdbg::GetInitVals(script, dir, params);
			
			if (script.length())
			{
				//set the current working directory
				if (dir.length() > 0)
					SetCurrentDirectory(dir.c_str());
				
				//store the command parameters
				hapdbg::SetPythonArguments(params, script);

				//execute the command line under debugger control
				hapdbg::RunPythonScript(script.c_str());
			}
			else
				printf("no python script specified");
		}
	}
	// DO NOT!!! use catch(...) here, as that will catch CPU exceptions as well as
	// C++ exceptions. Very bad.
	catch(const char* exception)
	{
		dprintf("Exception Caught - %s\n", exception);
		printf("Exception Caught - %s\n", exception);
	}



	//cleanup the debugger
	hapdbg::Uninitialize();

PYTHON_EXIT:
	//cleanup python
	Py_Finalize();

	return 0;

} // Handledmain



//////////////////////////////////////////////////////////////////////////////
// main
//
int main(int argc, char* argv[])
{
	int result = 0;

#ifdef	OSS_RELEASE

	result = Handledmain(argc, argv);

#else

	__try
	{
		result = Handledmain(argc, argv);
	}
	__except (he::HandleException(GetExceptionInformation(), "Main Thread"))
	{
		// Empty handler block - all the work is done in HandleException
	}

#endif

	return result;
} // main

//////////////////////////////////////////////////////////////////////////////
// ParseParams
//
void ParseParams(int argc, char* argv[], int& nPort)
{
	if (argc > 1)
	{
		for (int i = 1; i < argc; i++)
		{
			char* pCurParam = argv[i];
			assert(pCurParam);
			if ((strlen(pCurParam) > 1) && (pCurParam[0] == '-'))
			{
				if ((pCurParam[1] == 'p') || (pCurParam[1] == 'P'))
				{
					int NewPort = strtoul(pCurParam+2, NULL, 10);
					if (NewPort > 0)
					{
						nPort = NewPort;
						dprintf("Port %d selected\n", NewPort);
					}
				}
			}
		}
	}
} // ParseParams

//////////////////////////////////////////////////////////////////////////////
// BuildImageDate
//
const char* BuildImageDate()
{
	static char	datebuffer[100];

#ifdef	_WIN32

	// This method of getting the build date gets the link date - guaranteed to be
	// updated every time we rebuild.
	char *ProgramBase = (char *)GetModuleHandle(0);					// Get the base address of the program.
	PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)ProgramBase;	// Read the DOS header.
	assert(dosHeader->e_magic == IMAGE_DOS_SIGNATURE);
	// Skip to the NT header.
	PIMAGE_NT_HEADERS pNTHeader = PIMAGE_NT_HEADERS((char *)dosHeader + dosHeader->e_lfanew);
	// The time stamp for an image indicates the date and time that the image was created by the
	// Win32 linker. The value is represented in the number of seconds elapsed since midnight
	// (00:00:00), January 1, 1970, Universal Coordinated Time, according to the system clock.
	// The time stamp can be printed using the C run-time (CRT) function ctime or equivalent.
	struct tm LinkTime = *localtime((time_t *)(&pNTHeader->FileHeader.TimeDateStamp));
	//sprintf(datebuffer, "UTC link time: %08lx - %s", pNTHeader->FileHeader.TimeDateStamp, asctime(&LinkTime));
	sprintf(datebuffer, "%s", asctime(&LinkTime));
	datebuffer[strlen(datebuffer) - 1] = 0;	// Knock off the terminating \n character.

#else

	// This method of getting the build date gets compile date for this file.
	strcpy(datebuffer, __DATE__ " " __TIME__);

#endif
	return datebuffer;
} // BuildImageDate








