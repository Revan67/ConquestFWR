//--------------------------------------------------------------------------//
//                                                                          //
//                               dbexport.h		   							//
//                                                                          //
//                  COPYRIGHT (C) 2003 Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	$Header: /Libs/Src/Tools/adb/dbexport.h 5     11/13/03 5:03p Ajackson $

	$Author: Ajackson $
*/
//--------------------------------------------------------------------------//
#ifndef _DBEXPORT_H
#define _DBEXPORT_H

struct BasicExport
{
	virtual void Init( struct IFileSystem* _datafile, const char* _outFile, int _numFiles ) = 0;

	virtual void Execute( HINSTANCE _hInstance, HWND _parent ) = 0;

	virtual bool Update( HWND _progressBar ) = 0;

	virtual bool Uninit( void ) = 0;
};

class ExcelExport : public BasicExport
{
	private:
		WIN32_FIND_DATA findDir;
		HANDLE hDir;

		WIN32_FIND_DATA findFile;
		HANDLE hFile;

		struct IFileSystem* dataFile;
		char* outFilename;
		int numFiles;

	public:
		virtual void Init( struct IFileSystem* _datafile, const char* _outFile, int _numFiles );

		virtual void Execute( HINSTANCE _hInstance, HWND _parent );

		virtual bool Update( HWND _progressBar );

		virtual bool Uninit( void );
};

class MicrosoftDatabaseExport : public BasicExport
{
	private:
		struct IFileSystem* dataFile;
		char* outFilename;

	protected:
		void UpdateFiles( void );

	public:
		virtual void Init( struct IFileSystem* _datafile, const char* _outFile, int _numFiles );

		virtual void Execute( HINSTANCE _hInstance, HWND _parent );

		virtual bool Update( HWND _progressBar );

		virtual bool Uninit( void );
};

class PathExport : public BasicExport
{
	private:
		struct IFileSystem* dataFile;
		char*               outFilename;
		WIN32_FIND_DATA     fData;
		HANDLE              hData;
		bool                bWriting;

	protected:
		void UpdateFiles( void );

	public:
		virtual void Init( struct IFileSystem* _datafile, const char* _outFile, int _numFiles );

		virtual void Execute( HINSTANCE _hInstance, HWND _parent );

		virtual bool Update( HWND _progressBar );

		virtual bool Uninit( void );

		unsigned long       numFiles;
};

#endif
