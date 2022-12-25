#ifndef TGA_CLASS_H
#define TGA_CLASS_H

#include "..\Exporters\Common\mtl_txt.h"

//---------------------------------------------------------------------------
// TGA helper class
//---------------------------------------------------------------------------

struct DABitmap;

class TGA : public TGA_Header
{
	private:
		void* m_Pixels;
		void* m_Palette;
		CString m_Name;

	protected:
		void close();
		char * alloc();

	public:
		TGA(void);
		virtual ~TGA();

		bool Load( const char* _filename );
		bool Load( struct IFileSystem* _file );

		bool Save( const char* _filename );
		bool Save( struct IFileSystem* _file );

		void ConvertToBitmap( DABitmap& _bitmap );
		void ConvertFromBitmap( DABitmap& _bitmap );
};

#endif
