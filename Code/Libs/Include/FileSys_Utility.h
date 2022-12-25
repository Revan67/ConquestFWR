// FileSys_Utility.h
//
//
//

#ifndef FILESYS_UTILITY_H
#define FILESYS_UTILITY_H

// 

#include "FileSys.h"

//

template <class Type> 
HRESULT read_type( IFileSystem *IFS, const char *key, Type *out_type )
{
	DAFILEDESC desc( key );
	HANDLE h;
	U32 uBytesRead;
	Type foo;

	if( (h = IFS->OpenChild( &desc )) != INVALID_HANDLE_VALUE ) {
		if( IFS->ReadFile( h, &foo, sizeof(Type), &uBytesRead, NULL) && (uBytesRead == sizeof(Type)) ) {
			IFS->CloseHandle( h );
			*out_type = foo;
			return S_OK;
		}
	}

	IFS->CloseHandle( h );
	return E_FAIL;
}

//

template <class Type> 
HRESULT read_type_array( IFileSystem *IFS, const char *key, U32 num_to_read, Type *out_type )
{
	DAFILEDESC desc( key );
	HANDLE h;
	U32 uBytesRead;

	if( (h = IFS->OpenChild( &desc )) != INVALID_HANDLE_VALUE ) {
		if( IFS->ReadFile( h, out_type, sizeof(Type) * num_to_read, &uBytesRead, NULL) && (uBytesRead == (num_to_read * sizeof(Type))) ) {
			IFS->CloseHandle( h );
			return S_OK;
		}
	}

	IFS->CloseHandle( h );
	return E_FAIL;
}

//

inline HRESULT read_string( IFileSystem *IFS, const char *key, U32 max_string_len, char *out_string )
{
	DAFILEDESC desc( key );
	HANDLE h;
	U32 uBytesRead;

	if( (h = IFS->OpenChild( &desc )) != INVALID_HANDLE_VALUE ) {
		U32 uBytes = IFS->GetFileSize( h );
		U32 uBytesToRead = __min( uBytes, max_string_len-1 );
		if( IFS->ReadFile( h, out_string, uBytesToRead, &uBytesRead, NULL) && (uBytesRead == uBytesToRead) ) {
			out_string[uBytesRead] = 0;			
			IFS->CloseHandle( h );
			return S_OK;
		}
	}

	IFS->CloseHandle( h );
	return E_FAIL;
}

//

inline HRESULT read_chunk( IFileSystem *IFS, const char *key, U32 num_bytes, char *out_chunk )
{
	DAFILEDESC desc( key );
	HANDLE h;
	U32 uBytesRead;

	if( (h = IFS->OpenChild( &desc )) != INVALID_HANDLE_VALUE ) {
		
		if( num_bytes == 0 ) {
			num_bytes = IFS->GetFileSize( h );

		}

		if( IFS->ReadFile( h, out_chunk, num_bytes, &uBytesRead, NULL) && (uBytesRead == num_bytes) ) {
			IFS->CloseHandle( h );
			return S_OK;
		}
	}

	IFS->CloseHandle( h );
	return E_FAIL;
}


template <class Type> 
inline HRESULT read_alloc_chunk( IFileSystem *IFS, const char *key, U32 num_elements, Type **out_chunk )
{
	DAFILEDESC desc( key );
	HANDLE h;
	U32 uBytesRead;
	Type *buf;

	if( (h = IFS->OpenChild( &desc )) != INVALID_HANDLE_VALUE ) {
		
		if( num_elements == 0 ) {
			num_elements = IFS->GetFileSize( h ) / sizeof(Type);
		}

		if( (buf = new Type[num_elements]) == NULL ) {
			return E_FAIL;
		}

		*out_chunk = buf;

		U32 num_bytes = num_elements * sizeof(Type);

		if( IFS->ReadFile( h, buf, num_bytes, &uBytesRead, NULL) && (uBytesRead == num_bytes) ) {
			IFS->CloseHandle( h );
			return S_OK;
		}
	}

	IFS->CloseHandle( h );
	return E_FAIL;
}

//EMAURER not inline please.
template <class Type> HRESULT ReadAllocVector (IFileSystem *IFS, 
												const char *key, 
												U32& num_elements, 
												Type*& out_chunk)
{
	HRESULT gr = E_FAIL;

	DAFILEDESC desc( key );

	HANDLE h = IFS->OpenChild (&desc);

	if (h != INVALID_HANDLE_VALUE ) 
	{
		DWORD fsize = IFS->GetFileSize (h);

		U32 _num_elements = fsize / sizeof(Type);

#if DA_ERROR_LEVEL >= __SEV_TRACE_1
		if( (fsize % sizeof (Type)) != 0 )
		{
			char tmp_name[256];
			IFS->GetFileName(tmp_name, 256);
			strcat(tmp_name, " is a file w/ an inconsistent data chunk ");
			strcat(tmp_name, key);
			strcat(tmp_name, "\n");
			GENERAL_TRACE_1( tmp_name );
		}
#endif

		Type* _out_chunk = new Type[_num_elements];

		U32 uBytesRead;

		BOOL r = IFS->ReadFile(h, _out_chunk, fsize, &uBytesRead, NULL);

		if (r)
		{
			//ASSERT (fsize == uBytesRead);

			num_elements = _num_elements;
			out_chunk = _out_chunk;
			gr = S_OK;
		}
		else
			delete [] _out_chunk;

		IFS->CloseHandle( h );
	}

	return gr;
}

//


//

template <class Type> 
HRESULT write_type( IFileSystem *IFS, const char *key, Type &out_type )
{
	DAFILEDESC desc;
	HANDLE h;
	U32 uBytesRead;

	desc.lpFileName = key;
	desc.dwDesiredAccess = GENERIC_WRITE;
    desc.lpImplementation = "UTF";
    desc.dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
    desc.dwCreationDistribution = CREATE_ALWAYS;    

	if( (h = IFS->OpenChild( &desc )) != INVALID_HANDLE_VALUE ) {
		if( IFS->WriteFile( h, &out_type, sizeof(Type), &uBytesRead, NULL) && (uBytesRead == sizeof(Type)) ) {
			IFS->CloseHandle( h );
			return S_OK;
		}
	}

	IFS->CloseHandle( h );
	return E_FAIL;
}

//

template <class Type> 
HRESULT write_type_array( IFileSystem *IFS, const char *key, U32 num_to_write, Type *out_type )
{
	DAFILEDESC desc ;
	HANDLE h;
	U32 uBytesWritten;

	desc.lpFileName = key;
	desc.dwDesiredAccess = GENERIC_WRITE;
    desc.lpImplementation = "UTF";
    desc.dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
    desc.dwCreationDistribution = CREATE_ALWAYS;    


	if( (h = IFS->OpenChild( &desc )) != INVALID_HANDLE_VALUE ) {
		if( IFS->WriteFile( h, out_type, sizeof(Type) * num_to_write, &uBytesWritten, NULL) && (uBytesWritten == (num_to_write * sizeof(Type))) ) {
			IFS->CloseHandle( h );
			return S_OK;
		}
	}

	IFS->CloseHandle( h );
	return E_FAIL;
}




#endif // EOF
