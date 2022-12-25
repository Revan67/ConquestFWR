// TextureSourceMap.h
//
//
//

#ifndef __TextureSourceMap_h__
#define __TextureSourceMap_h__

//

#pragma warning( disable : 4786 )
#include <map>
#pragma warning( disable : 4786 )
#include <string>
#pragma warning( disable : 4786 )

using namespace std;

//

// TextureSourceMap
//
//
//
struct TextureSourceMap
{
public:

	GENRESULT reset( void );
	GENRESULT register_texture_source( IFileSystem *IFS );
	GENRESULT find_texture_source( const char *texture_name, char *out_texture_source );

	TextureSourceMap();
	~TextureSourceMap();

protected:
	typedef map<string,string> TextureSourceMapMap;

	TextureSourceMapMap m_Map;
};

//

TextureSourceMap::TextureSourceMap()
{
	reset();
}

//

TextureSourceMap::~TextureSourceMap()
{
	reset();
}

//

GENRESULT TextureSourceMap::reset( void )
{
	m_Map.clear();
	
	return GR_OK;
}

//

GENRESULT TextureSourceMap::register_texture_source( IFileSystem *IFS )
{
	const char *library_name = "Texture library";
	const char *animation_library_name = "Animation library";

	char ifs_buffer[MAX_PATH];
	char path_buffer[MAX_PATH];
	char full_path[MAX_PATH];
	WIN32_FIND_DATA DirSearchData, TxmSearchData;
	HANDLE hDirSearch, hTxmSearch;

	COMPTR<IFileSystem> Parent;

	if( SUCCEEDED( IFS->GetParentSystem( Parent ) ) ) {
		Parent->GetAbsolutePath( ifs_buffer, "", MAX_PATH );
	}
	else {
		strcpy( ifs_buffer, "" );
	}

	if( (hDirSearch = IFS->FindFirstFile( "*", &DirSearchData )) != INVALID_HANDLE_VALUE ) {

		do {
			
			if( DirSearchData.cFileName[0] == '.' ) {
				continue;
			}

			if( strcmp( DirSearchData.cFileName, library_name ) == 0 ) {

				// Add all normal textures
				//
				if( IFS->SetCurrentDirectory( library_name ) ) {

					if( (hTxmSearch = IFS->FindFirstFile( "*", &TxmSearchData )) != INVALID_HANDLE_VALUE ) {
						do {
							if( TxmSearchData.cFileName[0] != '.' ) {
								if( IFS->GetAbsolutePath( path_buffer, "", MAX_PATH ) ) {
									sprintf( full_path, "%s%s%s", ifs_buffer, path_buffer, TxmSearchData.cFileName );
									if( m_Map.find( TxmSearchData.cFileName ) == m_Map.end() ) {
										GENERAL_TRACE_1( TEMPSTR( "\n\nTexture: '%s' -> '%s'\n\n", TxmSearchData.cFileName, full_path ) );
										m_Map.insert( TextureSourceMapMap::value_type( string( TxmSearchData.cFileName ), string( full_path ) ) );
									}
								}
							}
						} while( IFS->FindNextFile( hTxmSearch, &TxmSearchData ) );

						IFS->FindClose (hTxmSearch);
					}

					IFS->SetCurrentDirectory( ".." );
				}
			}
			else if( strcmp( DirSearchData.cFileName, animation_library_name ) == 0 ) {

				// Add all animated textures
				//
				if( IFS->SetCurrentDirectory( animation_library_name ) ) {

					if( (hTxmSearch = IFS->FindFirstFile( "*", &TxmSearchData )) != INVALID_HANDLE_VALUE ) {
						do {
							if( TxmSearchData.cFileName[0] != '.' ) {
								if( IFS->GetAbsolutePath( path_buffer, "", MAX_PATH ) ) {
									sprintf( full_path, "%s%s%s", ifs_buffer, path_buffer, TxmSearchData.cFileName );
									if( m_Map.find( TxmSearchData.cFileName ) == m_Map.end() ) {
										GENERAL_TRACE_1( TEMPSTR( "\n\nTexture: '%s' -> '%s'\n\n", TxmSearchData.cFileName, full_path ) );
										m_Map.insert( TextureSourceMapMap::value_type( string( TxmSearchData.cFileName ), string( full_path ) ) );
									}
								}
							}
						} while( IFS->FindNextFile( hTxmSearch, &TxmSearchData ) );

						IFS->FindClose (hTxmSearch);
					}

					IFS->SetCurrentDirectory( ".." );
				}
			}
			else if( DirSearchData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
				// Recurse
				//
				if( IFS->SetCurrentDirectory( DirSearchData.cFileName ) ) {
					register_texture_source( IFS );
					IFS->SetCurrentDirectory( ".." );
				}
			}
		
		} while( IFS->FindNextFile( hDirSearch, &DirSearchData ) );
	}

	return GR_OK;
}

//

GENRESULT TextureSourceMap::find_texture_source( const char *texture_name, char *out_texture_source )
{
	TextureSourceMapMap::iterator ts;
	string tn( texture_name );

	if( (ts = m_Map.find( tn )) == m_Map.end() ) {
		return GR_GENERIC;
	}

	strcpy( out_texture_source, ts->second.c_str() );
	
	return GR_OK;
}

//


//

#endif // EOF
