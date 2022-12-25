#pragma once

// remember to include <afxpriv.h> to use a CSharedFile
class CSharedFile;

struct IClipboardObject : public IDAComponent
{
	virtual const char* GetType() = 0;

	virtual bool Copy( CSharedFile& _memfile ) = 0;

	virtual bool Paste( CSharedFile& _memfile ) = 0; 

	virtual bool Append( CSharedFile& _memfile ) = 0; 
};

struct IClipboard : public IDAComponent
{
	// empties out the clipboard, and copies data to it
	//
	virtual bool Copy( IClipboardObject& _object ) = 0;

	// places more data into the clipboard, it must be the same type as the previous Copy operation started or it fails
	//
	virtual bool Append( IClipboardObject& _object ) = 0;

	// gets the first block from the clipboard of this type. It sets _moreData to TRUE if more blocks are waiting
	//
	virtual bool Paste( IClipboardObject& _object ) = 0;

	// determines if there is valid data for this type
	//
	virtual bool HasDataForType( const char* _type ) = 0;
};