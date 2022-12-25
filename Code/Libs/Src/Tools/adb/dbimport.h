//--------------------------------------------------------------------------//
//                                                                          //
//                               DbImport.h	    							//
//                                                                          //
//                  COPYRIGHT (C) 2003 Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	$Header: /Libs/Src/Tools/adb/dbimport.h 5     11/13/03 1:52p Ajackson $

	$Author: Ajackson $
*/
//--------------------------------------------------------------------------//

#ifndef DBIMPORTHEADERFILE
#define DBIMPORTHEADERFILE

struct BasicImport
{
	virtual void Init( struct IFileSystem* _datafile ) = 0;

	virtual void Execute( HINSTANCE _hInstance, HWND _parent ) = 0;

	virtual bool Update( HWND _progressBar ) = 0;

	virtual bool Uninit( void ) = 0;
};

struct AccessFileImport : public BasicImport
{
	enum ProcessType
	{
		PT_NONE,
		PT_IMPORT_FILES,
		PT_CLEANUP,
	};

	class DbSymbols*    symbols;
	enum ProcessType    processType;
	struct IFileSystem* datafile;
	int                 symbolTableIndex;
	int                 currentColumn;

	int                 currentStructureSize;
	BYTE*               newStructure;
	BYTE*               pStructureData;
	BYTE*               pLastStructure;
	int                 currentBits;
	int                 recordSize;

	virtual void Init( struct IFileSystem* _datafile );

	virtual void Execute( HINSTANCE _hInstance, HWND _parent );

	virtual bool Update( HWND _progressBar );

	virtual bool Uninit( void );

	private:

	void ResetStructure( struct Symbol* );

	void ApplyPadding( void );
};

struct ADBFileImport : public BasicImport
{
	enum ProcessType
	{
		PT_NONE,
		PT_IMPORT_FILES,
		PT_CLEANUP,
		PT_DONE,
	};

	char fileCurrentFile[MAX_PATH];

	enum   ProcessType  processType;
	struct IFileSystem* inFile;
	struct IFileSystem* outFile;

	HWND treeView;
	HWND fileView; // IDC_TREE_FILELIST

	bool bAutoOverwrite;
	bool bSelectAll;
	bool bSomethingElse;

	virtual void Init( struct IFileSystem* _datafile );

	virtual void Execute( HINSTANCE _hInstance, HWND _parent );

	virtual bool Update( HWND _progressBar );

	virtual bool Uninit( void );

	protected:

	bool DetectChange( const char* _dirname, const char* _filename, IFileSystem* _inFile, IFileSystem* _outFile );
};

struct ChangeListImport : public BasicImport
{
	enum ProcessType
	{
		PT_NONE,
		PT_IMPORT_FILES,
		PT_CLEANUP,
		PT_DONE,
		PT_CANCEL,
	};
	ProcessType processType;

	char szDest[256];

	struct IFileSystem* datafile;
	
	virtual void Init( struct IFileSystem* _datafile );

	virtual void Execute( HINSTANCE _hInstance, HWND _parent );

	virtual bool Update( HWND _progressBar );

	virtual bool Uninit( void );
};

#endif
