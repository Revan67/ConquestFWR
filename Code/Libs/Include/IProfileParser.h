#ifndef IPROFILEPARSER_H
#define IPROFILEPARSER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IProfileParser.h                            //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Rmarr $

   $Header: /Conquest/Libs/Include/IProfileParser.h 5     4/28/00 11:57p Rmarr $

   Interface used for parsing INI files that may not necessarily conform to
   the Windows INI standard. For example:
	  1) There may be multiple keys with the same name.
	  2) Keys may not always follow the "key=value" format.
	  3) Keys may start with a non-alpha [a-z][A-Z] character. (eg. [1-9][!@#$%&*()]


	//-----------------------------
	//
	GENRESULT Initialize (const C8 *fileName, ACCESS access = READ_ACCESS );
		INPUT:
			fileName: ASCIIZ name of the INI file to process.
			access: READ_ACCESS for read-only access, file cannot be modified.
					WRITE_ACCESS for both read and write access.
		RETURNS:
			GR_OUT_OF_MEMORY  if not enough memory to load file.
			GR_FILE_ERROR	  if file could not be opened.
			GR_OK             if successful
		OUTPUT:
			Closes any previous file that was opened, invalidating any outstanding Section handles.
   
	//-----------------------------
	//
	BOOL32 EnumerateSections (ENUM_PROC proc = 0, void *context=0);
		INPUT:
			proc: User defined callback routine.
			context: User defined data item passed to the callback function
		RETURNS:
			TRUE if the enumeration ran to completion.
			FALSE if 'proc' was null or the callback function returned 0 to stop the 
			enumeration early.]
		OUTPUT:
			Calls the user defined callback function for each section found in the INI file.
		NOTES:
			Calling CreateSection() from within the callback routine using the 'sectionName' parameter
			of the callback will cause the method to return a handle to the current section, rather than
			first occurrence in the file. This is the only way to get a handle to a section when the 
			name is used more than once in the file.

	//-----------------------------
	//
	HANDLE CreateSection (const C8 *sectionName, CREATE_MODE mode = PP_OPENEXISTING);
		INPUT:
			sectionName: ASCIIZ name of the section to find.
			mode: PP_OPENEXISTING open an existing section.
		RETURNS:
			Non-NULL handle on success.
			NULL handle if the section could not be opened / created.
		OUTPUT:
			If successful, the method returns a handle that you can use in calls to 
			ReadProfileLine() and ReadKeyvalue().
			Call CloseSection() when you are done using a handle.
		NOTES:
			This method will fail if 'mode' is anything but PP_OPENEXISTING. The other modes
			haven't been implemented yet. In the future, INI files will be readable and writable.
			This method always returns a handle to the first matching instance of the section name 
			found in the ini file. To access successive matching names, you must use the enumeration
			method. (See EnumerateSections(), above.)
	   
	//-----------------------------
	//
	BOOL32 CloseSection (HANDLE hSection);
		INPUT:
			hSection: Handle to a section, returned by a call to CreateSection().
		RETURNS:
			Returns FALSE if the handle was invalid.
		OUTPUT:
			Closes internal resources associated with the handle.
	 
	//-----------------------------
	//
	U32 ReadProfileLine (HANDLE hSection, U32 lineNumber, C8 * buffer, U32 bufferSize);
		INPUT:
			hSection: Handle to a section, returned by a call to CreateSection().
			lineNumber: Specifies which line to return from with the section. 'lineNumber' is 0 based,
				so that 0 will return the first line after the [section] marker.
			buffer: User supplied buffer which receives the text found on that line of the INI file.
			bufferSize: size of the user supplied buffer.
		RETURNS:
			The actual number of bytes placed in the buffer, not counting the terminating 0. If 'buffer' is NULL,
			no text is copied, but the return value indicates how many bytes would have been transfered.
		OUTPUT:
			Locates a line of text from the INI file, placing it in the user's buffer. If the handle is
			invalid or 'lineNumber' is greater than or equal to the actual number of lines in the section, 
			the method returns 0 and the buffer is unchanged.
		   
	//-----------------------------
	//
	U32 ReadKeyValue (HANDLE hSection, const C8 * keyName, C8 * buffer, U32 bufferSize);
		INPUT:
			hSection: Handle to a section, returned by a call to CreateSection().
			keyName: The ASCIIZ name of the key to search for within the section. The search is 
				case sensitive.
			buffer: User supplied buffer which receives the text found on the line after the '=' character.
			bufferSize: size of the user supplied buffer.
		RETURNS:
			The actual number of bytes placed in the buffer, not counting the terminating 0. If 'buffer' is NULL,
			no text is copied, but the return value indicates how many bytes would have been transfered.
		OUTPUT:
			Locates a line of text from the INI file, placing it in the user's buffer. If the handle is
			invalid or 'keyName' cannot be found within the section, the method returns 0 and the buffer is unchanged.
			The returned string contains the remainder of the text on the line after the 'key =' part.
		NOTES:
			This method always returns the first matching keyname. To find other matching keys, you must use the
			ReadProfileLine() method.

*/
//--------------------------------------------------------------------------//


#ifndef DACOM_H
#include "DACOM.h"
#endif

typedef void *HANDLE;

//---------------------
//-- structure used by CreateInstance method
//---------------------


struct PROFPARSEDESC : public AGGDESC
{
	PROFPARSEDESC (const C8 *_interfaceName = "IProfileParser") : AGGDESC(_interfaceName)
	{
		size = sizeof(*this);
	};
};
struct PROFPARSEDESC2 : public AGGDESC
{
	PROFPARSEDESC2 (const C8 *_interfaceName = "IProfileParser2") : AGGDESC(_interfaceName)
	{
		size = sizeof(*this);
	};
};

//

#define IID_IProfileParser MAKE_IID("IProfileParser",1)


// IProfileParser
//
//
//
struct DACOM_NO_VTABLE IProfileParser : public IDAComponent
{
	// enumeration callback routine - return TRUE to continue the enumeration

	typedef BOOL32  (__stdcall * ENUM_PROC) (struct IProfileParser * parser, 
													const C8 * sectionName,
													void *context);

	enum ACCESS
	{
		READ_ACCESS = 0,
		WRITE_ACCESS
	};

	enum CREATE_MODE
	{
		PP_CREATENEW=1,
		PP_CREATEALWAYS=2,
		PP_OPENEXISTING=3,
		PP_OPENALWAYS=4,
		PP_TRUNCATEEXISTING=5
	};
	
	DEFMETHOD(Initialize) (const C8 *fileName, ACCESS access = READ_ACCESS ) = 0;

	DEFMETHOD_(BOOL32,EnumerateSections) (ENUM_PROC proc = 0, void *context=0) = 0;
	
	DEFMETHOD_(HANDLE,CreateSection) (const C8 *sectionName, CREATE_MODE mode = PP_OPENEXISTING) = 0;

	DEFMETHOD_(BOOL32,CloseSection) (HANDLE hSection) = 0;

	DEFMETHOD_(U32,ReadProfileLine) (HANDLE hSection, U32 lineNumber, C8 * buffer, U32 bufferSize) = 0;

	DEFMETHOD_(U32,ReadKeyValue) (HANDLE hSection, const C8 * keyName, C8 * buffer, U32 bufferSize) = 0;
};
//-----------------------------------------------------------------------------
//---------------------------IProfileParser2 interface-------------------------
//-----------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE IProfileCallback
{
	virtual BOOL32 __stdcall ProfileCallback (struct IProfileParser2 * parser, const C8 * name, const C8 * value, void *context) = 0;
};

//-----------------------------------------------------------------------------
//

#define IID_IProfileParser2 MAKE_IID("IProfileParser2",IProfileParser2)

struct DACOM_NO_VTABLE IProfileParser2 : public IDAComponent
{
	virtual GENRESULT __stdcall Initialize (const C8 *fileName, IProfileParser::ACCESS access = IProfileParser::READ_ACCESS ) = 0;

	virtual GENRESULT __stdcall Initialize2 (const C8 *buffer, U32 bufferSize ) = 0;

	virtual HANDLE __stdcall CreateSection (const C8 *sectionName, IProfileParser::CREATE_MODE mode = IProfileParser::PP_OPENEXISTING) = 0;

	virtual BOOL32 __stdcall CloseSection (HANDLE hSection) = 0;

	virtual U32 __stdcall ReadProfileLine (HANDLE hSection, U32 lineNumber, C8 * buffer, U32 bufferSize) = 0;

	virtual U32 __stdcall ReadKeyValue (HANDLE hSection, const C8 * keyName, C8 * buffer, U32 bufferSize) = 0;

	virtual BOOL32 __stdcall EnumerateSections (IProfileCallback * callback, void *context=0) = 0;

	virtual BOOL32 __stdcall EnumerateKeys (IProfileCallback * callback, HANDLE hSection, void *context=0) = 0;
};


//--------------------------------------------------------------------------//
//----------------------------END IProfileParser.h--------------------------//
//--------------------------------------------------------------------------//
#endif