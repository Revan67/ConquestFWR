// AnimatedTextures.cpp
//
//
//

//

#ifndef SGI
#include <windows.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <direct.h>

//

#include "sgi_utf.h"
#include "mtl_txt.h"

//

extern void MergeTextures( file_node *root_in, file_node *root_out );
extern void IMagick2txm( char *file_name, txt_lib *tl, int keep_ext );
extern void	LoadTextures1_6(txt_lib *tl, file_node *root);
extern void	RemoveTextures1_6(file_node *root);
extern void	ConvertTextures(txt_lib *tl, file_node *root_out, BOOL32 xflip, BOOL32 yflip);

//

#define TXM_MAX_NUM_FRAME_TEXTURES 32

//

typedef enum {
	TXM_FL_RLTB,	// right-to-left, top-to-bottom
	TXM_FL_RLBT,	// right-to-left, bottom-to-top

} TXMFRAMELAYOUT;

//

struct TXMFRAMERECT
{
	long frame_texture_index;
	float u0, v0, u1, v1;
};

//

struct TXMFRAMETEXTURE
{
	char name[MAX_PATH];
	txt_lib texture;
};

//

// 
// Returns 0 on success.
//
int CreateAnimatedTexture( char *txmlib_filename, char *animated_texture_name, char *frame_texture_spec, int frame_count, int frame_size, float frame_rate, int layout )
{
	TXMFRAMETEXTURE frame_textures[TXM_MAX_NUM_FRAME_TEXTURES];
	TXMFRAMERECT *frame_rects;
	int ft,frame_texture_count, max_width=0, max_height=0;
	int x, y;
	int frame_index;
	int done;
	char *p, *t;
	file_node *root_in, *root_out, *cur, *texture_library;
	char curdir[_MAX_PATH], fdir[_MAX_PATH];

	// Add frame textures based on filespec(s) in frame_texture_spec
	//
	done = 0;
	frame_texture_count = 0;
	p = frame_texture_spec;
			
	while( !done ) {
				
		while( *p && (*p==' ' || *p=='\t') ) p++;				// eat leading whitespace

		if( *p == 0 ) {
			done = 1;
			continue;
		}

		if( (t = strchr( p, ';' )) != NULL ) {
			*t = 0;
		}

		t = p;
		while( *p && (*p!=' ' && *p!='\t' && *p!=',' ) ) p++;	// eat filename

		if( *p == 0 ) {
			done = 1;
			// no continue;
		}

		*p = 0;	p++;

#ifndef SGI

		WIN32_FIND_DATA ft_find;
		HANDLE hfind;
		char *filespec;

		strcpy( fdir, t );

		if( (filespec = strrchr( fdir, '\\' )) != NULL ) {
			GetCurrentDirectory( _MAX_PATH, curdir );
			*filespec = 0;
			filespec++;
			SetCurrentDirectory( fdir );
		}
		else {
			filespec = t;
			fdir[0] = 0;
		}

		if( (hfind = FindFirstFile( filespec, &ft_find )) != INVALID_HANDLE_VALUE ) {
			do {
				if( frame_texture_count >= TXM_MAX_NUM_FRAME_TEXTURES ) {
					fprintf( stderr, "Error: animated texture has too many frames!\n" );
					return -1;
				}

				strcpy( frame_textures[frame_texture_count].name, ft_find.cFileName );
				
				frame_texture_count++;

			} while( FindNextFile( hfind, &ft_find ) );
		}
#endif
	}

	if( frame_texture_count == 0 ) {
		fprintf( stderr, "Error: no image files found, exiting...\r\n" );
		exit( 0 );
	}

	// Sort texture names so order is correct (texture_0, texture_1, etc...)
	//
	qsort( frame_textures, frame_texture_count, sizeof(TXMFRAMETEXTURE), (int (__cdecl *)(const void *,const void*))stricmp );

	if( frame_count <= 0 ) {

		if( (frame_rects = new TXMFRAMERECT[frame_texture_count]) == NULL ) {
			return -1;
		}

		for( int i=0; i < frame_texture_count; i++ ) {
			frame_rects[i].frame_texture_index = i;
			frame_rects[i].u0 = 0.0f;
			frame_rects[i].v0 = 0.0f;
			frame_rects[i].u1 = 1.0f;
			frame_rects[i].v1 = 1.0f;
		}
	}
	else {
		if( (frame_rects = new TXMFRAMERECT[frame_count]) == NULL ) {
			return -1;
		}
	}


	// Build input UTF tree with frame textures and animated texture data
	//
	cur = root_in = CreateNode( "\\", D );
		cur = cur->child = texture_library = CreateNode( "Texture library", D );

	cur = NULL;

	// Add frame textures
	//
	for( ft=0; ft<frame_texture_count; ft++ ) {

		InitTxtLib( &frame_textures[ft].texture );
		IMagick2txm( frame_textures[ft].name, &frame_textures[ft].texture, 0 );

		sprintf( frame_textures[ft].name, "%s_%d", animated_texture_name, ft );
		free( frame_textures[ft].texture.textures[0]->name );
		frame_textures[ft].texture.textures[0]->name = strdup( frame_textures[ft].name );
		
		if( frame_textures[ft].texture.count > 0 ) {
			
			CreateTxtLib( &frame_textures[ft].texture, NULL, texture_library );
			
			if( frame_textures[ft].texture.textures[0]->mip_map[0]->x_size > max_width ) {
				max_width = frame_textures[ft].texture.textures[0]->mip_map[0]->x_size ;
			}

			if( frame_textures[ft].texture.textures[0]->mip_map[0]->y_size > max_height ) {
				max_height = frame_textures[ft].texture.textures[0]->mip_map[0]->y_size ;
			}
		}

	}
	
	// Calculate texture coordinates
	//
	if( frame_count > 0 ) {

		x = 0;
		y = 0;
		frame_index = 0;

		assert( max_width && max_height );

		for( int i=0; i < frame_count; i++ ) {

			if( (x + frame_size) > max_width ) {

				x = 0;
				y += frame_size;

				if( (y + frame_size) > max_height ) {
					x = 0; 
					y = 0;
					frame_index++;
				}
			}

			frame_rects[i].frame_texture_index = frame_index;

			frame_rects[i].u0 = (float) x / (float) max_width;
			frame_rects[i].u1 = frame_rects[i].u0 + (frame_size / (float) max_width);

			switch( layout ) {
			
			case TXM_FL_RLTB:	// top-to-bottom
				frame_rects[i].v0 = (float) y / (float) max_height;
				frame_rects[i].v1 = frame_rects[i].v0 + (frame_size / (float) max_height);
				break;
			
			case TXM_FL_RLBT:	// bottom-to-top
				frame_rects[i].v0 = 1.0f - ((float) y / (float) max_height);
				frame_rects[i].v1 = frame_rects[i].v0 - (frame_size / (float) max_height);
				break;
			}

			x += frame_size;
		}
	}
	else {
		frame_count = frame_texture_count;
	}

	// Read current TXM file (if it exists), merge in the new data, write it back out
	//
	if( (root_out = ReadUTF( txmlib_filename )) == NULL ) {
		root_out = CreateNode("\\",D);
		root_out->child = CreateNode( "Texture library", D );
	}

	txt_lib		templib;
	file_node	*lib_out	=GetWideNodeG(root_out, "Texture library");
	file_node	*lib_in		=GetWideNodeG(root_in, "Texture library");

	LoadTextures1_6(&templib, root_in);

	RemoveTextures1_6(root_in);

	ConvertTextures(&templib, lib_in, FALSE, FALSE);

	MergeTextures(root_in, root_out);

	// Add animated texture
	cur	=CreateNode(animated_texture_name, D );
	cur->sibling	=lib_out->child;
	lib_out->child	=cur;

	cur	=cur->child	=CreateNode( "Texture count", F );
	cur->data		=(unsigned char*)&frame_texture_count;
	cur->data_size	=sizeof(frame_texture_count);

	cur	=cur->sibling	=CreateNode( "Frame count", F );
	cur->data			=(unsigned char*)&frame_count;
	cur->data_size		=sizeof(frame_count);

	cur	=cur->sibling	=CreateNode( "FPS", F );
	cur->data			=(unsigned char*)&frame_rate;
	cur->data_size		=sizeof(frame_rate);

	cur	=cur->sibling	=CreateNode( "Frame rects", F );
	cur->data			=(unsigned char*)frame_rects;
	cur->data_size		=sizeof(TXMFRAMERECT) * ( (frame_count <= 0)? frame_texture_count : frame_count );

	if( fdir[0] ) {
		SetCurrentDirectory( curdir );
	}

	WriteUTF(root_out, txmlib_filename);

	FreeTxtLib(&templib);

	for( ft=0; ft<frame_texture_count; ft++ ) {
		FreeTxtLib( &frame_textures[ft].texture );
	}	
	FreeTree( root_in );
	FreeTreeData( root_out );

	return 0;
}

#if 0

void main( int argc, char *argv[] )
{
// axmmake	<-tga / -pix /-bmp>
//			<tex size> <frame size> <number of frames>
//			<utf_name / animation_name / base_texture_name>
//			[-nomip]

// parse command line
	if (argc <= 4)
	{
		printf("axmmake.exe:\n <-tga/-pix/-bmp> <input tex size> <frame size> <num frames> <tex name> [-flip] [-nomip]");
		return;
	}

	// -TGA, -PIX, .....
	char txmswitch [MAX_PATH];
	strcpy( txmswitch, argv[1] );
	
	// file extension is the stuff past first dash
	char ext [MAX_PATH];
	strcpy( ext, txmswitch+1 );

	int texsize = atoi(argv[2]);
	int framesize = atoi(argv[3]);
	int frame_count = atoi(argv[4]);

	if (texsize < framesize)
	{
		printf("specified texture size less than the size of one frame!\n");
		return;
	}

	// utf/axm/base texture name
	char name [MAX_PATH];
	strcpy( name, argv[5] );

	// [-flip]
	bool flip=false;
	if (argc > 6)
		flip= !stricmp( "-flip", argv[6] );

	// [-no mip]
	char mip [MAX_PATH];
	if (argc > 7)
		strcpy( mip, argv[7] );
	else
		mip[0]=0;

// cheesy frame rectangle computation
	int tex_ext= 0;
	FRAME_RECT * rects = new FRAME_RECT[frame_count];
	{
		int x=0, y=0;

		for( int i=0;i < frame_count;i++)
		{
			if ((x + framesize) > texsize)
			{
				x=0;
				y+= framesize;

				if ((y + framesize) > texsize)
				{
					x=0; y=0;
					tex_ext++;
				}
			}

			rects[i].texext = tex_ext;

			rects[i].u0= (float) x / (float) texsize;
			rects[i].u1= rects[i].u0 + (framesize / (float) texsize);
			
			if (!flip)
			{
				rects[i].v0= 1.0f - ((float) y / (float) texsize);
				rects[i].v1= rects[i].v0 - (framesize / (float) texsize);
			}
			else
			{
				rects[i].v0= (float) y / (float) texsize;
				rects[i].v1= rects[i].v0 + (framesize / (float) texsize);
			}
			
			x+= framesize;
		}
	}
	printf("created frames....\n");

// create a directory structure with appropriate data
	char tmpdir[L_tmpnam];
	int mkdir_result;

	// make a tmp directory
	char* r = tmpnam (tmpdir);
	assert (r && "couldnt create name for temp dir");

	mkdir_result= _mkdir (tmpdir);
	assert (!mkdir_result && "couldnt create temp dir");
	printf("mkdir %s\n", tmpdir);

	static char axmlib[MAX_PATH];
	static char axminst[MAX_PATH];

	FILE * block=NULL;

// write animation data
	MAKE_DIR( "Animation library", tmpdir, axmlib );
	{
		MAKE_DIR( name, axmlib, axminst );
		{
			block= OPEN_BLOCK( "Texture count", axminst );
			{
				int texture_count = tex_ext+1;
				fwrite( &texture_count, 4, 1, block );
			}
			CLOSE_BLOCK( &block );

			block= OPEN_BLOCK( "Frame count", axminst );
			{
				fwrite( &frame_count, 4, 1, block );
			}
			CLOSE_BLOCK( &block );

			block= OPEN_BLOCK( "Frame rects", axminst );
			{
				fwrite( rects, sizeof(FRAME_RECT), frame_count, block );
			}
			CLOSE_BLOCK( &block );
		}
	}

// provide directory hook for texture lib
	MAKE_DIR( "Texture library", tmpdir );

// use external tools to make this work
	char cmd[MAX_PATH];
	int cmd_result;

// pack.exe the directory structure
	sprintf (cmd, "pack %s %s.txm", tmpdir, name);
	//printf ("%s\n", cmd);
	cmd_result= system (cmd);

	sprintf (cmd, "deltree /Y %s", tmpdir);
	printf ("%s\n", cmd);
	system (cmd);

// txmlib.exe the textures into the .utf
	for (int i = 0; i <= tex_ext; i++)
	{
		sprintf (cmd, "txmlib  %s %s_%d.%s %s.txm %s", 
					txmswitch, 
					name,i,ext, 
					name, mip);

		printf ("%s\n", cmd);
		system (cmd);
	}

	int debug_catch;
	debug_catch=0;
}
#endif

// EOF



#if USE_TLIB_CODE

//

#include "DACOM.h"
#include "TSmartPointer.h"
#include "IProfileParser_Utility.h"

//

struct TTBEDATA
{
	TL_TEXTURELIBRARY *library;
	COMPTR<ICOManager> DACOM;
	char *texture_name;
};

struct TTBETEXTURENAME
{
	TTBETEXTURENAME *next, *prev;
	char name[255+1];
};

BOOL32 __stdcall tlop_texture_build_enum( IProfileParser * parser, const C8 * sectionName, void *context )
{
	TTBEDATA *data = (TTBEDATA*)context;
	
	if( !data->texture_name || !strcmp( sectionName, data->texture_name ) ) {

		TL_TEXTURE *texture;
		
		if( (texture = data->library->find_texture( sectionName, TEXTURE_FORCE_CREATE )) != NULL ) {

			HANDLE h;
			if( (h = parser->CreateSection( sectionName )) == 0 ) {
				return TRUE;
			}

			parser->CloseSection( h );

			char names[255+1], *p, *t;
			char frame[255+1];
			char def_frame[255+1];
			U32 num_frames;
			float fps;
			LList<TTBETEXTURENAME> frame_textures;

			int done = 0;

			opt_get_string( data->DACOM, parser, sectionName, "Textures", "", names, 255 );
			
			p = names;
			while( !done ) {
				
				while( *p && (*p==' ' || *p=='\t') ) p++;				// eat leading whitespace

				if( *p == 0 ) {
					done = 1;
					continue;
				}

				if( (t = strchr( p, ';' )) != NULL ) {
					*t = 0;
				}

				t = p;
				while( *p && (*p!=' ' && *p!='\t' && *p!=',' ) ) p++;	// eat filename

				if( *p == 0 ) {
					done = 1;
					// no continue;
				}

				*p = 0;	p++;

				WIN32_FIND_DATA ft_find;
				HANDLE hfind;

				if( (hfind = FindFirstFile( t, &ft_find )) != INVALID_HANDLE_VALUE ) {
					do {
						TTBETEXTURENAME *ft_name = new TTBETEXTURENAME();
						strcpy( ft_name->name, ft_find.cFileName );
						
						TTBETEXTURENAME *l;
						for( l=frame_textures.first(); l; l=l->next ) {
							if( strcmp( l->name, ft_name->name ) > 0 ) {
								frame_textures.link( ft_name, l );
								break;
							}
						}
						if( l == NULL ) {
							frame_textures.link( ft_name );
						}
						fprintf( stdout, "Trace: added frame texture '%s'\n", ft_name->name );
					} while( FindNextFile( hfind, &ft_find ) );
				}

			}

			if( frame_textures.count() == 0 ) {
				fprintf( stdout, "Trace: No frame textures found, skipping\n" );
				return TRUE;
			}

			fprintf( stdout, "Trace: %d frame textures\n", frame_textures.count() );

			opt_get_u32( data->DACOM, parser, sectionName, "NumFrames", frame_textures.count(), &num_frames );

			fprintf( stdout, "Trace: %d frames\n", num_frames );

			opt_get_float( data->DACOM, parser, sectionName, "Framerate", 24.0, &fps );

			fprintf( stdout, "Trace: %f fps\n", fps );

			TL_TEXTUREFRAME *tf = new TL_TEXTUREFRAME[num_frames];
			
			float def_frame_num_delta = ((float)frame_textures.count())/num_frames;

			for( U32 f=0; f<num_frames; f++ ) {

				sprintf( frame, "Frame_%d", f );
				sprintf( def_frame, "{ %d, 0,0, 1,1 }", (U32)((float)f * def_frame_num_delta ) );
				opt_get_string( data->DACOM, parser, sectionName, frame, def_frame, names, 255 );

				p = names;
				while( *p && (*p==' ' || *p=='\t' || *p==',') ) p++;				// eat leading whitespace


				while( *p && (*p!='{') ) p++;							// get '{'
				p++;

				while( *p && (*p==' ' || *p=='\t' || *p==',') ) p++;	// eat whitespace
				t = p;
				while( *p && (*p!=' ' && *p!='\t' && *p!=',' ) ) p++;	// eat framenum
				*p = 0;
				p++;
				tf[f].texture_num_idx = atoi( t );

				while( *p && (*p==' ' || *p=='\t' || *p==',') ) p++;	// eat whitespace
				t = p;
				while( *p && (*p!=' ' && *p!='\t' && *p!=',' ) ) p++;	// eat float
				*p = 0;
				p++;
				tf[f].u0 = (float)atof( t );

				while( *p && (*p==' ' || *p=='\t' || *p==',') ) p++;	// eat whitespace
				t = p;
				while( *p && (*p!=' ' && *p!='\t' && *p!=',' ) ) p++;	// eat float
				*p = 0;
				p++;
				tf[f].v0 = (float)atof( t );

				while( *p && (*p==' ' || *p=='\t' || *p==',') ) p++;	// eat whitespace
				t = p;
				while( *p && (*p!=' ' && *p!='\t' && *p!=',' ) ) p++;	// eat float
				*p = 0;
				p++;
				tf[f].u1 = (float)atof( t );

				while( *p && (*p==' ' || *p=='\t' || *p==',') ) p++;	// eat whitespace
				t = p;
				while( *p && (*p!=' ' && *p!='\t' && *p!=',' && *p!='}' ) ) p++;	// eat float
				*p = 0;
				p++;
				tf[f].v1 = (float)atof( t );

				tf[f].texture_num_idx = __min( tf[f].texture_num_idx, frame_textures.count() );

				fprintf( stdout, "Trace: Frame %d: %d %f %f %f %f\n", f, tf[f].texture_num_idx, tf[f].u0, tf[f].v0, tf[f].u1, tf[f].v1 );
			}


			if( num_frames && frame_textures.count() ) {

				TTBETEXTURENAME *ftn;
				U32 num;
				for( num=0,ftn=frame_textures.first(); ftn; ftn=ftn->next,num++ ) {

					char ftname[255+1];
					sprintf( ftname, "%s_%d", sectionName, num ); 

					TL_TEXTURE *ftexture;
					if( (ftexture = data->library->find_texture( ftname, TEXTURE_FORCE_CREATE )) != NULL ) {

						TL_TEXTUREIMAGE *image;
						if( (image = ftexture->find_image( (char*)"MIP0", IMAGE_FORCE_CREATE )) != NULL ) {
							if( image->load_from_file( ftn->name, 0, 0, PixelFormat() ) != 0 ) {
								fprintf( stdout, "Warning: could not find image '%s' for frame texture num %d in %s\n", ftn->name, num, sectionName );
							}
						}
					}
				}
				
				texture->set_frame_rate( fps );
				texture->set_frames( num_frames, tf );
				texture->set_frame_texture_count( frame_textures.count() );
			}
			
			frame_textures.free();
		}
	}

	return TRUE;
}

//

void tlop_texture_build( void *library_name, void *deffile_name, void *texture_name, void *d )			
{
	TL_TEXTURELIBRARY *library;
	if( (library = find_library( (char*)library_name, TRUE )) != NULL ) {
		
		COMPTR<IProfileParser> IPP;
		PROFPARSEDESC desc;
		COMPTR<ICOManager> DACOM = DACOM_Acquire();
		
		if( FAILED( DACOM->CreateInstance( &desc, IPP ) ) ) {
			fprintf( stdout, "Error: Could not get profile parser\n" );
			return;
		}

		if( SUCCEEDED( IPP->Initialize( (C8*)deffile_name ) ) ) {
			TTBEDATA data;
			data.library = library;
			data.texture_name = (char*)texture_name;
			data.DACOM = DACOM;
			IPP->EnumerateSections( tlop_texture_build_enum, &data );
		}
	}
}

#endif // USE_TLIB_CODE

// 

static int mkdir_result;
inline void MAKE_DIR( char * name, char * base, char * out=NULL )
{
	char tmp[MAX_PATH];
	
	_makepath(tmp, NULL, base, name, NULL);
	mkdir_result= _mkdir(tmp);

	printf("mkdir %s\n",tmp);

	if (out)
		strcpy( out, tmp );
}

//

inline FILE * OPEN_BLOCK( char * name, char * base, char * out=NULL )
{
	char tmp[MAX_PATH];
	FILE * block=NULL;
	
	_makepath(tmp, NULL, base, name, NULL); 
	block = fopen (tmp, "w+b"); 
	assert (block);

	if (out)
		strcpy( out, tmp );

	return block;
}

//

inline void CLOSE_BLOCK( FILE ** block ) 
{
	if (*block) 
	{ 
		fclose(*block); 
		(*block)=0; 
	}
}

