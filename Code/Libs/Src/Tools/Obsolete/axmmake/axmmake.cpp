// for now all textures will have to be the same size;
// im stealing code left and right to get this done quickly

// this makes one utf per texture animation
// axmmake	<-tga / -pix /-bmp>
//			<tex size> <frame size> <number of frames>
//			<utf_name / animation_name / base_texture_name>
//			[-nomip]

// texture count is set at one for now
// size and number parms should be made based on file
// name range given

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <direct.h>

struct FRAME_RECT
{
	long texext;
	float u0, v0, u1, v1;
};

// **************************************************** *
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

inline void CLOSE_BLOCK( FILE ** block ) 
{
	if (*block) 
	{ 
		fclose(*block); 
		(*block)=0; 
	}
}

// **************************************************** *
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
