//////////////////////////////////////////////////////////////////////////////
//  File:    OssInclude.h
//
//  Purpose: 
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
//  You should have received a copy of the GNU Lesser General Public License 
//  along with this library (see the file license.txt); if not, write to the 
//  Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//////////////////////////////////////////////////////////////////////////////	

#ifndef	_OSSINCLUDE_H
#define	_OSSINCLUDE_H

//This switch will cause the open source stuff to be included
//instead of the Humgonous Entertainment Core library
#define OSS_RELEASE



#ifdef OSS_RELEASE
#pragma message (" ---- OSS_RELEASE defined - Building Open Source Version ----")
/*
This header defines a useful set of defines, types, pragmas and
includes that I like to always have available. It should very rarely
be changed, and should be included from everywhere.
*/

// Some generally useful header files that I want always available.
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <stdarg.h>
#include <string>
#include <vector>
#include <time.h>
#include <assert.h>
#include <cassert>



namespace HumongousOpenSource
{


#ifdef	_MSC_VER
	// Disable the STUPID!!! STL in use warning. This is done here so that
	// it will be done before any of the STL headers are included - necessary
	// to making the warning go away.
	#pragma warning(disable : 4786)
	// This disgusting macro forces VisualC++ to use the ANSI C++ rules
	// for the scoping of variables declared inside for statements.
	// Trust me - it works. Using this evil macro makes it much easier
	// to write portable code. This will be unnecessary starting with
	// version 7.0 of VC++
	// This hack is actually documented (recommended?) in MSDN, q167748
	#define	for if (0) ; else for
	#define	HEDEBUG_BREAK()	__asm { int 3 }
#else
	#error Unknown compiler - please implement HEDEBUG_BREAK and any other platform specific things.
#endif

// Cool compile time assert. Use for things like:
// CompileTimeAssert(sizeof(BITMAPINFOHEADER) == sizeof(PortableBITMAPINFOHEADER));
// From Writing Bug-Free C Code for Windows, Jerry Jongerius, Prentice Hall, 1995
#define CompileTimeAssert(exp) extern char _CompileTimeAssert[(exp)?1:-1]

// The COMPILERMSG macro is used to get the compiler to display a message whenever a
// file is compiled, typically a reminder to do something. The message is printed out
// in a format such that if you double click on it you are taken to the line and file
// it is in.
#define COMPILERMSGSTR(x)    #x	// Internally used macro.
#define COMPILERMSGSTR2(X)    COMPILERMSGSTR(X)	// Internally used macro.
#define COMPILERMSG(msg) message(__FILE__ "(" COMPILERMSGSTR2(__LINE__) ") : " msg)
// Here is an example of how to use the COMPILERMSG macro:
//#pragma COMPILERMSG("Don't forget to fix this code.")
// Note the '#pragma' at the beginning, and no semicolon at the end.



#ifdef  _DEBUG
	#define verify(exp) assert(exp)
#else
	#define	verify(exp)	exp
#endif

// I love this macro for finding the the number of elements in an array.
// What can I say - I know the preprocessor is evil, but it works so
// damned well for a few key things!
#define	NUMELEMENTS(x)	(sizeof(x) / sizeof(x[0]))

// Some generally useful functions that I want to have always available.

// These debug printing functions are just stubbed out here. Feel free to fill these in with your
// own real versions.
#define dprintf 0 && 
#define rdprintf 0 && 
#define dprintv(x, y) ((void)0)
#define rdprintv(x, y) ((void)0)

//the following two functions allow you to get the link time from either a dll
//or from an executable - very useful in version labelling.
const char* GetExeLinkDate();
const char* GetDllLinkDate();

typedef	signed char int8_t;
typedef	unsigned char uint8_t;
typedef	signed short int16_t;
typedef	unsigned short uint16_t;
typedef	signed int int32_t;
typedef	unsigned int uint32_t;
typedef	signed __int64 int64_t;
typedef unsigned __int64 uint64_t;


#define HEInitializeLibrary() 0
#define HandleException(a, b) 0




// String printing functions
// These allow you to do a variable argument printf style call to 
// print formatted output into a C++ string.
// 
// They're not exceptionally fast, so don't use them for games, but they
// do remove any arbitrary limitations by not having any fixed-length buffers.



inline std::string vstringprintf(const char* format, va_list args)
{
	// Allocate a printf buffer.
	std::vector<char>      buffer(500);
	int result;

	do
	{
		result = _vsnprintf(&buffer[0], buffer.size() - 1, format, args);
		// If we didn't have enough room, keep increasing the buffer size until
		// we do have enough room.
		if (result == -1)
			buffer.resize(buffer.size() * 2);
	} while (result == -1);

	// Make sure the buffer is null terminated - I don't trust _vsnprintf to
	// do this in the boundary cases.
	buffer[buffer.size() - 1] = 0;

	return &buffer[0];
}

inline std::string stringprintf(char const *format, ...)
{
	va_list args; 
	va_start(args, format);

	std::string strReturn = vstringprintf(format, args);		

	va_end(args);

	return strReturn;		
}

inline int snprintfAssert(char *buffer, size_t count, const char *format, ...)
{
	va_list args; 
	va_start(args, format);
	int result = _vsnprintf(buffer, count, format, args);
	assert(result < int(count));
	va_end(args);
	return result;
}

inline int vsnprintfAssert(char *buffer, size_t count, const char *format, va_list args)
{
	int result = _vsnprintf(buffer, count, format, args);
	assert(result < int(count));
	return result;
}


///////////////////////////////////////////////////////////////////////////////
// Function:	GetLinkDate
//
// Purpose:		Given a module base address, extract the link date (as a string)
//
// Inputs:
//		ModuleAddress - base address of the module (dll or exe)
//
// Outputs:
//
// Return:
//		String containing the link time and date
//
///////////////////////////////////////////////////////////////////////////////
inline const char* GetLinkDate(unsigned int ModuleAddress)
{
	static char	datebuffer[100];

#ifdef	_WIN32

	PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)ModuleAddress;	// Read the DOS header.
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
	
	assert(strlen(datebuffer) > 0);
	datebuffer[strlen(datebuffer) - 1] = 0;	// Knock off the terminating \n character.

#else
	// This method of getting the build date gets compile date for this file.
	strcpy(datebuffer, __DATE__ " " __TIME__);
#endif

	return datebuffer;

} // GetLinkDate


///////////////////////////////////////////////////////////////////////////////
// Function:	GetDllLinkDate
//
// Get the link date of a DLL - this function should be called from within the 
// DLL itself in order to get the correct date.
//
// Inputs:
// Outputs:
//
// Return:
//		String containing the link time and date
//
///////////////////////////////////////////////////////////////////////////////
inline const char* GetDllLinkDate()
{
#ifdef	_WIN32
	unsigned int uiCodePtr = 0;

	//a little assembly to get a pointer to some code - if this library
	//becomes a dll, this function needs to be inline allowing to get
	//the base address of the dll that is using this function.

	__asm
	{
CODE_PTR_LABEL:
		lea EAX, CODE_PTR_LABEL		//load the address of the label into EAX
		mov uiCodePtr, EAX			//store the address into uiCodePtr
	}

	//Using the code ptr, get the allocation base - this is the same as the 
	//HMODULE and is also the location of the PIMAGE_DOS_HEADER in memory
	//which we can use to extract the link date.
	
	MEMORY_BASIC_INFORMATION mbi;
	VirtualQuery((const void*)uiCodePtr, &mbi, sizeof(mbi));
	return GetLinkDate((unsigned int) mbi.AllocationBase);

#else
	//non win32, just use the __DATE__ and __TIME__ macros
	return GetLinkDate(0);
#endif
}

///////////////////////////////////////////////////////////////////////////////
// Function:	GetExeLinkDate
//
// Purpose:		Get the link date of an executable
//
// Inputs:		None.
//
// Outputs:		None.
//
// Return:
//		String containing the link time and date
//
///////////////////////////////////////////////////////////////////////////////
inline const char* GetExeLinkDate()
{
#ifdef	_WIN32
	//The module handle (HMODULE) is also a pointer to the base of the executable
	//so it points to the PIMAGE_DOS_HEADER for the module.
	return GetLinkDate((unsigned int) GetModuleHandle(0));
#else
	//non win32, just use the __DATE__ and __TIME__ macros
	return GetLinkDate(0);
#endif
}




//The following code is only applicable with multi-threaded builds
#ifdef _MT

//forward reference (see below)
class CCodeSingleUseEntry;


///////////////////////////////////////////////////////////////////////////////
// class CCodeSingleUseGate
//
// This class is used in conjunction with the CCodeSingleUseEntry to protect a
// resource or section of code from being used at the same time by two different
// threads.
//
// Each resource that needs to be protected should have an instance of this class
// associated with it.  Before accessing the resource, an instance of 
// CCodeSingleUseEntry should be created with a reference to the associated 
// CCodeSingleUseGate object.  This constructor will block until all other 
// threads have released their Entry objects.  

class CCodeSingleUseGate
{
	//allow access to Enter and Leave functions
	friend class CCodeSingleUseEntry;

public:
	CCodeSingleUseGate()
	{
		InitializeCriticalSection(&m_CritSection);
	}
	~CCodeSingleUseGate()
	{
		DeleteCriticalSection(&m_CritSection);
	}

private:
	//no copying allowed
	CCodeSingleUseGate(const CCodeSingleUseEntry& Copy);
	CCodeSingleUseGate& operator=(const CCodeSingleUseEntry& Copy);

	//these functions are accessible through the constructor and destructor
	//of the CCodeSingleUseEntry class
	
	//call to enter the protected section - blocks until the gate is clear
	void EnterGate()
	{
		EnterCriticalSection(&m_CritSection);
	}

	//leave the protected section
	void LeaveGate()
	{
		LeaveCriticalSection(&m_CritSection);
	}

private:
	//member data - 

#ifdef	_WIN32
	//critical section under windows
	CRITICAL_SECTION m_CritSection;

//#elif defined(__APPLE__)
	//critical 'region' under mac
//	MPCriticalRegionID m_CritRegion;

#else
#error Unknown operating system - needs critical section implementation.
#endif

}; //CCodeSingleUseGate

///////////////////////////////////////////////////////////////////////////////
// class CCodeSingleUseEntry
//
// This class is used to gain access to a resource protected by a CCodeSingleUseGate
// object.  Simply construct an instance of this object with a reference to the 
// gate object before accessing the resource, when the object is destroyed, the
// resource will be open to access once more.

class CCodeSingleUseEntry
{
public:
	CCodeSingleUseEntry(CCodeSingleUseGate& Gate)
	:m_rGate(Gate)
	{
		m_rGate.EnterGate();
	}
	~CCodeSingleUseEntry()
	{
		m_rGate.LeaveGate();
	}

private:
	//no copying allowed
	CCodeSingleUseEntry(const CCodeSingleUseEntry& Copy);
	CCodeSingleUseEntry& operator=(const CCodeSingleUseEntry& Copy);

private:
	//reference to the gate object that we are entering
	CCodeSingleUseGate& m_rGate;

}; // CCodeSingleUseEntry
#endif //_MT - end of multi thread only code


} // end of namespace
#endif  // OSS_RELEASE

#endif	// _OSSINCLUDE_H
