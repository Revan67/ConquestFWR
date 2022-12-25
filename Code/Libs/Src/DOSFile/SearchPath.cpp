//--------------------------------------------------------------------------//
//                                                                          //
//                           SearchPath.cpp                                 //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Libs/dev/Src/DOSFile/SearchPath.cpp 6     3/21/00 4:30p Pbleisch $
	
   A component that implements a data file search path .
*/
//--------------------------------------------------------------------------//

#include <windows.h>

#include "SearchPath.h"
#include "FileSys.h"
#include "TComponent.h"
#include "TSmartPointer.h"

#include "da_heap_utility.h"

extern ICOManager *DACOM;
//--------------------------------------------------------------------------//
//----------------------SearchPath implementation---------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE DAPath : public ISearchPath
{
	BEGIN_DACOM_MAP_INBOUND(DAPath)
	DACOM_INTERFACE_ENTRY(ISearchPath)
	DACOM_INTERFACE_ENTRY(IComponentFactory)
	DACOM_INTERFACE_ENTRY2(IID_ISearchPath,ISearchPath)
	DACOM_INTERFACE_ENTRY2(IID_IComponentFactory,IComponentFactory)
	END_DACOM_MAP()

	//--------------------------
	// data items
	//--------------------------

	char path[MAX_PATH+4];
	COMPTR<IFileSystem> * systems;
	int numSystems;

	~DAPath (void)
	{
		reset();
	}

	DA_HEAP_DEFINE_NEW_OPERATOR(DAPath);

	GENRESULT init (SEARCHPATHDESC * desc)
	{
		return GR_OK;
	}

	void reset (void)
	{
		delete [] systems;
		numSystems = 0;
		path[0] = 0;
	}

	// *** IComponentFactory methods ***
	
	DEFMETHOD(CreateInstance) (DACOMDESC *descriptor, void **instance);

    // *** ISearchPath methods ***

	DEFMETHOD(SetPath) (const C8 *path);

	DEFMETHOD_(U32,GetPath) (C8 *path, U32 bufferSize) const;

    // *** DAPath methods ***

static bool DAPath::createFileSystem (IComponentFactory * factory, char * buffer, IFileSystem ** resultFile);

static int __stdcall parse (char * buffer, COMPTR<IFileSystem> * & ptrs, int recurseCount=0);

};
//--------------------------------------------------------------------------//
//
GENRESULT DAPath::CreateInstance (DACOMDESC *descriptor, void **instance)
{
	GENRESULT result = GR_FILE_ERROR;
	int i;
	
	*instance = 0;
	for (i = 0; i < numSystems; i++)
		if (systems[i]!=0 && (result = systems[i]->CreateInstance(descriptor, instance)) == GR_OK)
			break;

	return result;
}
//--------------------------------------------------------------------------//
//
void __fastcall switchchar_convert (char * string);
//--------------------------------------------------------------------------//
//
GENRESULT DAPath::SetPath (const C8 *newpath)
{
	GENRESULT result = GR_INVALID_PARMS;
	char buffer[MAX_PATH];

	if (newpath == 0)
		goto Done;
	//
	// free the old path
	//
	reset();
	strncpy(path, newpath, MAX_PATH);
	switchchar_convert(path);
	memcpy(buffer, path, sizeof(buffer));

	numSystems = parse(buffer, systems);

	result = GR_OK;
Done:
	return result;
}
//--------------------------------------------------------------------------//
// find the ';' characters, create an array of pointers to each subpath
//
int DAPath::parse (char * buffer, COMPTR<IFileSystem> * & ptrs, int recurseCount)
{
	char * ptr;

	if ((ptr = strchr(buffer, ';')) != 0)
	{
		*ptr = 0;
		if (ptr==buffer)
			return parse(ptr+1, ptrs, recurseCount);
		else
		{
			int i;
			i = parse(ptr+1, ptrs, recurseCount+1);
			createFileSystem(DACOM, buffer, ptrs[recurseCount]);
			return i;
		}
	}
	else // base case (no semicolons)
	{
		if (buffer[0])
			recurseCount++;

		if (recurseCount)
			ptrs = new COMPTR<IFileSystem>[recurseCount];

		if (buffer[0])
			createFileSystem(DACOM, buffer, ptrs[recurseCount-1]);

		return recurseCount;
	}
}
//--------------------------------------------------------------------------//
//
bool DAPath::createFileSystem (IComponentFactory * factory, char * buffer, IFileSystem ** resultFile)
{
	char * ptr=0;
	bool result = false;
	DAFILEDESC fdesc = buffer;
	COMPTR<IFileSystem> file;

	*resultFile = 0;
	if (buffer[0] == '\\' && buffer[1] == '\\')   // network file descriptor     \\Arrakis\Dev
	{
		if (buffer[2] == '\\')
			goto Done;
		if ((ptr = strchr(buffer+3, '\\')) == 0)
			goto Done;
		if (ptr[1] == '\\')
			goto Done;
		if ((ptr = strchr(ptr+1, '\\')) != 0)
			*ptr++ = 0;
	}
	else
	if (isalpha(buffer[0]) && buffer[1]==':')    //  d:'\'
	{
		if ((ptr = strchr(buffer+3, '\\')) != 0)
			*ptr++ = 0;
	}
	else
	if ((ptr = strchr(buffer+1, '\\')) != 0)      //   '\'
		*ptr++ = 0;

	if (factory->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;
	if (ptr && ptr[0])
	{
		result = createFileSystem(file, ptr, resultFile);
	}
	else
	{
		*resultFile = file;
		file->AddRef();
		result = true;
	}

Done:
	return result;
}
//----------------------------------------------------------------------------//
//
U32 DAPath::GetPath (C8 *buffer, U32 bufferSize) const
{
   bufferSize = __min(bufferSize, (U32)strlen(path)+1);

   if (bufferSize > 0 && buffer)
   {
      memcpy(buffer, path, bufferSize);
	  buffer[bufferSize-1] = 0;
   }

   return bufferSize;
}
//----------------------------------------------------------------------------//
//
IComponentFactory * CreateSearchPathFactory (void)
{
	return new DAComponentFactory<DAComponent<DAPath>, SEARCHPATHDESC> ("ISearchPath");
}
//----------------------------------------------------------------------------//
//-------------------------------End SearchPath.cpp---------------------------//
//----------------------------------------------------------------------------//
