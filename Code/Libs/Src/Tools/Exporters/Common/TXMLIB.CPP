#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
/* #include "BaseUTF.h" */
#include "sgi_utf.h"
//#include "3db.h"
#include "mtl_txt.h"
#include "names.h"

enum {L=1, R, RA, MOV, C, MIP, TEX, TEX2, AXM, CONVERT, COLLECT};

//float face_normal_tolerance = FACE_NORMAL_TOLERANCE;
//float vertex_normal_tolerance = VERTEX_NORMAL_TOLERANCE;
int export_vertex_colors = 0;

bool exporting_deformable = false;
int no_physics = 0;

int old_style=0;
float lod_mtl_weight = 150.0f;
float lod_uv_weight = 0.45f;
int convex_hull_flag = 0;
int ik_extents = 0;

int verbose_level=1;
int mip_flag=1;
int dither_flag=0;
int txt_depth = PAL8;
int txt_flag=1;
float scale_factor=1.0f;
int mode_flag;
//int flag_565=0;
int y_flip_flag = 0;
int u_mode = TILE;
int v_mode = TILE;
int keep_ext = 0;

void Usage(char* argv0);
void CmdOptions(int argc, char *argv[]);
void Move(char *name1, char *name2);
void RemoveAll(char *name);
void Remove(char *file_name, char *txt_name);
void List(char *file_name);
void Create(char *file_name);
void Pix2txm(char *file_name, char *alpha_file_name, txt_lib *tl, int keep_ext);
void UpdateMip(char *file_name);
void FixTxt(txt *t);
void SetWrapMode(txt_lib *tl);

extern void Tga2txm(char *file_name, txt_lib *tl, int keep_ext); // in tga.cpp
extern void Bmp2txm(const char *file_name, txt_lib *tl, int keep_ext); // in bmp.cpp
extern void IMagick2txm( char *file_name, txt_lib *tl, int keep_ext); // in Imagick.cpp
extern void IMagick2txm2( char *file_name, char *afile_name, txt_lib *tl, int keep_ext); // in Imagick.cpp
extern void	IMagickFile2TGA(const char *file_name, file_node *root_out);
extern int CreateAnimatedTexture( char *txmlib_filename, char *animated_texture_name, char *frame_texture_spec, int frame_count, int frame_size, float frame_rate, int layout );
extern void	ConvertTextures(txt_lib *tl, file_node *root_out, BOOL32 xflip, BOOL32 yflip);
extern void	LoadTextures1_6(txt_lib *tl, file_node *root);

void	RemoveTextures1_6(file_node *root)
{
	file_node	*node;
	
	if(GetWideNode(root,Texture_U_limit))
	{
		fprintf(stderr,"Warning: file not in new 3db format.\n");
	}

	node	=GetWideNode(root, Texture_library);
	if(!node)
	{
		fprintf(stderr,"Warning: source file has no %s.  ",Texture_library);
		fprintf(stderr,"Nothing done.\n");
		return;
	}
	
	FreeTreeData(node->child);
	node->child	=NULL;
}

int main(int argc, char *argv[])
{
	int		i;
	char	tmpCollect[_MAX_PATH];

  if(argc<2)
    Usage(argv[0]); 

  CmdOptions(argc, argv);
  switch(mode_flag)
  {
    case L:
      List(argv[2]);
      break;
    case R:
      Remove(argv[2], argv[3]);
      break;
    case RA:
      RemoveAll(argv[2]);
      break;
    case MOV:
      Move(argv[2], argv[3]);
      break;
    case CONVERT:
	{
		if(!argv[2])
		{
			fprintf(stderr, "Error: input utf/3db/cmp/txm required\n");
			exit(69);
		}
		
		if(!argv[3])
		{
			fprintf(stderr, "Error: destination filename required\n");
			exit(69);
		}

		txt_lib		tl;
		file_node	*gimp, *root_in	=ReadUTF(argv[2]);

		if(!root_in)
		{
			fprintf(stderr, "Error: input utf/3db/cmp/txm required\n");
			exit(69);
		}

		//check for the old format (texturelib dir not in root)
		for(gimp = root_in->child;gimp;gimp = gimp->sibling)
		{
			if(!strcmp(gimp->name, Texture_library))
			{
				break;
			}
		}

		if(!gimp)	//must run collect first
		{
			sprintf(tmpCollect, "%scollect", argv[3]);

			FreeTree(root_in);

			MoveTexturesHome(argv[2], tmpCollect);

			root_in	=ReadUTF(tmpCollect);
		}

		LoadTextures1_6(&tl, root_in);

		RemoveTextures1_6(root_in);

		file_node	*lib_out	=GetWideNodeG(root_in, Texture_library);

		// Our textures are always stored with the origin at the lower left,
		// thus the images are flipped on the y axis.
		ConvertTextures(&tl, lib_out, FALSE, TRUE);

		WriteUTF(root_in, argv[3]);
		
		FreeTxtLib(&tl);
		FreeTree(root_in);

		if(!gimp)
		{
			unlink(tmpCollect);
		}
		break;
	}
    case COLLECT:
	{
		if(!argv[2])
		{
			fprintf(stderr, "Error: input utf/3db/cmp/txm required\n");
			exit(69);
		}
		
		if(!argv[3])
		{
			fprintf(stderr, "Error: destination filename required\n");
			exit(69);
		}

		MoveTexturesHome(argv[2], argv[3]);
		break;
	}
	case AXM:
	{
		int fcount = -1;
		float frate = 24.0f;
		int fsize = -1;
		char fspec[512];

		if( argv[2] == NULL ) {
			fprintf( stderr, "Error: animated texture name required\n" );
			exit(1);
		}
		
		if( argv[3] == NULL ) {
			fprintf( stderr, "Error: destination UTF filename required\n" );
			exit(1);
		}

		if( argc > 4 && argv[4] != NULL ) {
			strcpy( fspec, argv[4] );
		}
		else {
			sprintf( fspec, "%s*.*", argv[2] );
		}

		if( argc > 5 && argv[5] != NULL ) {
			frate = (float)atof( argv[5] );
		}

		if( argc > 6 && argv[6] != NULL ) {
			fcount = atoi( argv[6] );
		}

		if( argc > 7 && argv[7] != NULL ) {
			fsize = atoi( argv[7] );
		}

		CreateAnimatedTexture( argv[3], argv[2], fspec, fcount, fsize, frate, !y_flip_flag );
		break;
	}
    case TEX:
    {
		if(!argv[2])
		{
			fprintf(stderr, "Error: input image required\n");
			exit(69);
		}
		
		if(!argv[3])
		{
			fprintf(stderr, "Error: destination filename required\n");
			exit(69);
		}

		file_node	*root_out	=ReadUTF(argv[3]);
		
		if(!root_out)
		{
			root_out				=CreateNode("\\", D);
			root_out->child			=CreateExporterVersion();
			root_out->child->sibling=CreateNode(Texture_library, D);
		}
		file_node	*lib_out	=GetWideNodeG(root_out, Texture_library);
		txt_lib		tl;

		InitTxtLib(&tl);
		IMagick2txm(argv[2], &tl, keep_ext);

		// This does not have anything to do with the TGA xflip/yflip stuff. This flag just tells
		// this program to invert the actual bits.  This is on by default for our convention.
		if(!y_flip_flag)
		{
			for(i=0;i < tl.count;i++)
			{
				FlipTxt(tl.textures[i]);
			}
		}

		// NOTE: The flipping flags here indicate whether or not the raw bits being written out
		// are "flipped" or not from the start orientation of first row first, first column first.
		// The passed in flags are simply converted to the proper TGA flags and written out to the stored TGA file
		// to indicate the ordering of the data.
		// THIS FUNCTION DOES NOT ACTUALLY REARRANGE THE IMAGE DATA.
		// In this case, since ImageMagick always returns images with the first row first and the first column first,
		// We prefer bottom line first on disk to make texturelibrary do less work
		ConvertTextures(&tl, lib_out, FALSE, TRUE);

		WriteUTF(root_out, argv[3]);

		FreeTxtLib(&tl);		
		FreeTree(root_out);
		break;
    }
    case TEX2:
    {
		if(!argv[2])
		{
			fprintf(stderr, "Error: input image required\n");
			exit(69);
		}
		
		if(!argv[3])
		{
			fprintf(stderr, "Error: input alpha image required\n");
			exit(69);
		}
		
		if(!argv[4])
		{
			fprintf(stderr, "Error: destination filename required\n");
			exit(69);
		}

		file_node	*root_out	=ReadUTF(argv[4]);

		if(!root_out)
		{
			root_out				=CreateNode("\\", D);
			root_out->child			=CreateExporterVersion();
			root_out->child->sibling=CreateNode(Texture_library, D);
		}
		file_node	*lib_out	=GetWideNodeG(root_out, Texture_library);
		txt_lib		tl;

		InitTxtLib(&tl);
		IMagick2txm2(argv[2], argv[3], &tl, keep_ext);

		// This does not have anything to do with the TGA xflip/yflip stuff. This flag just tells
		// this program to invert the actual bits.  This is on by default for our convention.
		if(!y_flip_flag)
		{
			for(i=0;i < tl.count;i++)
			{
				FlipTxt(tl.textures[i]);
			}
		}

		// NOTE: The flipping flags here indicate whether or not the raw bits being written out
		// are "flipped" or not from the start orientation of first row first, first column first.
		// The passed in flags are simply converted to the proper TGA flags and written out to the stored TGA file
		// to indicate the ordering of the data.
		// THIS FUNCTION DOES NOT ACTUALLY REARRANGE THE IMAGE DATA.
		// In this case, since ImageMagick always returns images with the first row first and the first column first,
		// We prefer bottom line first on disk to make texturelibrary do less work
		ConvertTextures(&tl, lib_out, FALSE, TRUE);

		WriteUTF(root_out, argv[4]);

		FreeTxtLib(&tl);		
		FreeTree(root_out);
		break;
    }
    case MIP:
    {
      UpdateMip(argv[2]);
      break;
    }
    case C:
      Create(argv[2]);
      break;
    default:
      Usage(argv[0]);
  }

  return 0;
}

void Move(char *name1, char *name2)
{
  file_node *root_in, *root_out;

  root_in=ReadUTF(name1);
  root_out=ReadUTF(name2);

  MergeTextures(root_in, root_out);

  WriteUTF(root_in, name1);
  WriteUTF(root_out, name2);

  FreeTreeData(root_in);
  FreeTreeData(root_out);
}
      
void RemoveAll(char *name)
{
  file_node *root;
 
  root=ReadUTF(name);

  RemoveTextures1_6(root);

  WriteUTF(root, name);
  FreeTreeData(root);
}
      
void Remove(char *file_name, char *txt_name)
{
  file_node *root;
  file_node *txt_lib_node;
  file_node *texture;

  root=ReadUTF(file_name);
  txt_lib_node=GetWideNode(root, Texture_library);
  if(!txt_lib_node){
    fprintf(stderr,"Warning: source file has no %s.",Texture_library);
    fprintf(stderr,"Nothing done.\n");
    return;
  }
  texture=GetWideNode(txt_lib_node, txt_name);
  DetachTxt(txt_lib_node, txt_name);
  FreeTreeData(texture);

  WriteUTF(root, file_name);
  FreeTreeData(root);
}
      
void List(char *file_name)
{
  file_node *root;
  file_node *txt_lib_node;
  file_node *texture;

  root=ReadUTF(file_name);
  txt_lib_node=GetWideNode(root, Texture_library);
  if(!txt_lib_node){
    fprintf(stderr,"Warning: source file has no %s.  ",Texture_library);
    fprintf(stderr,"Nothing done.\n");
    return;
  }
  texture=txt_lib_node->child;
  while(texture){
    printf("%s\n",texture->name);
    texture=texture->sibling;
  }

  FreeTreeData(root);
}

void Create(char *file_name)
{
  file_node *root;

  root=CreateNode("\\",D);
  root->child=CreateNode(Texture_library,D);
  WriteUTF(root, file_name);
  FreeTreeData(root);
}

void Usage(char *argv0)
{
  fprintf(stdout, "\nUsage: %s [mode] <file(s)> [option(s)]\n", argv0);
  
  fprintf(stdout, "MODES:\n");
  fprintf(stdout,"\t-l <file> : ");
  fprintf(stdout,"\n\t\tList textures\n\n");

  fprintf(stdout,"\t-r <file> texture_name : ");
  fprintf(stdout,"\n\t\tRemove texture\n\n");

  fprintf(stdout,"\t-ra <file> : ");
  fprintf(stdout,"\n\t\tRemove all textures\n\n");

  fprintf(stdout,"\t-m <file1> <file2> : ");
  fprintf(stdout,"\n\t\tMove all textures from file1 to file2\n\n");

  fprintf(stdout,"\t-c <file.txm> : ");
  fprintf(stdout,"\n\t\tCreate a new empty texture library.\n\n");

  fprintf(stdout,"\t-tex <file.foo> <file.txm> : ");
  fprintf(stdout,"\n\t\tAdd/Update texture in file.foo (format foo) to .txm.\n\n");

  fprintf(stdout,"\t-tex2 <file.foo> <file_alpha.foo> <file.txm> : ");
  fprintf(stdout,"\n\t\tAdd/Update texture in file.foo (format foo) to .txm with the second as alpha.\n\n");

  fprintf(stdout,"\t-mip <file.3db> : ");
  fprintf(stdout,"\n\t\tAdd/Fix mip levels.\n\n");

  fprintf(stdout,"\t-axm <animated_texture_name> <file.txm> [files] [fps] [fcount] [fsize]: ");
  fprintf(stdout,"\n\t\tAdd animated texture .txm.\n\n");

  fprintf(stdout,"\t-convert <input.utf/3db/cmp/txm> <output> : ");
  fprintf(stdout,"\n\t\tConvert a file containing textures to the latest format.\n\n");

  fprintf(stdout,"\t-collect <input.utf/3db/cmp/txm> <output> : ");
  fprintf(stdout,"\n\t\tEnsure all texture libraries contain a single texture dir.\n\n");

  fprintf(stdout, "OPTIONS:\n");
  fprintf(stdout,"\t-nomip : ");
  fprintf(stdout,"\n\t\tDon't generate mip maps.\n\n");

  fprintf(stdout,"\t-dither : ");
  fprintf(stdout,"\n\t\tDither image (useful for 8 & 16 bit output)\n\n");

  fprintf(stdout,"\t-yflip : ");
  fprintf(stdout,"\n\t\tFlip image vertically\n\n");

  fprintf(stdout,"\t-565 : ");
  fprintf(stdout,"\n\t\tExport as 16-bit (565) (default is 8 bit palletized)\n\n");

  fprintf(stdout,"\t-888 : ");
  fprintf(stdout,"\n\t\tExport as 24/32-bit (888/8) (default is 8 bit palletized)\n\n");

  fprintf(stdout,"\t-keep_ext : ");
  fprintf(stdout,"\n\t\tKeep input file extension as part of the texture name.\n\n");

  fflush(stdout);

  //fprintf(stdout,"\t-mips # # ... # <file.3db> : ");
  //fprintf(stdout,"\n\t\tGenerate only specified MIP levels.\n\n");

  // fprintf(stdout,"\t-v # : ");
  // fprintf(stdout,"\n\t\tSpecify verbose level (default=1).\n\n");

  exit(1);
}

void CmdOptions(int argc, char *argv[])
{
  int i;

  for(i=1;i<argc;i++){
    if(!strcmp(argv[i],"-l")){
      mode_flag=L;
    }else
    if(!strcmp(argv[i],"-r")){
      mode_flag=R;
    }else
    if(!strcmp(argv[i],"-ra")){
      mode_flag=RA;
    }else
    if(!strcmp(argv[i],"-m")){
      mode_flag=MOV;
    }else
    if(!strcmp(argv[i],"-c")){
      mode_flag=C;
    }else
    if(!strcmp(argv[i],"-axm")){
      mode_flag=AXM;
    }else
    if(!strcmp(argv[i],"-convert")){
      mode_flag=CONVERT;
    }else
    if(!strcmp(argv[i],"-collect")){
      mode_flag=COLLECT;
    }else
    if(!strcmp(argv[i],"-tex")){
      mode_flag=TEX;
    }else
    if(!strcmp(argv[i],"-tex2")){
      mode_flag=TEX2;
    }else
    if(!strcmp(argv[i],"-nomip")){
      mip_flag=0;
    }else
    if(!strcmp(argv[i],"-mip")){
      mode_flag=MIP;
      mip_flag=1;
    }else
	if(!strcmp(argv[i],"-dither")){
      dither_flag=1;
    }else
	if(!strcmp(argv[i],"-yflip")){
      y_flip_flag=1;
    }else
    if(!strcmp(argv[i],"-v")){
      verbose_level=atoi(argv[i+1]);
    }else
	if(!strcmp(argv[i],"-565")){
      txt_depth = TRUE565;
    }else
	if(!strcmp(argv[i],"-888")){
      txt_depth = TRUE888;
    }else
	if(!strcmp(argv[i],"-keep_ext")){
      keep_ext = 1;
    }
  }
}
      
void UpdateMip(char *file_name)
{
  file_node *root=NULL;
  root=ReadUTF(file_name);

  txt_lib tl;
  LoadTextures(&tl, root);

  for(int txt_id=0; txt_id<tl.count; txt_id++)
  {
	  FixTxt(tl.textures[txt_id]);
	  if(u_mode == CLAMP)
	  {
		  tl.textures[txt_id]->u_mode = CLAMP;
	  }
	  if(v_mode == CLAMP)
	  {
		  tl.textures[txt_id]->v_mode = CLAMP;
	  }

	  if(y_flip_flag)
	  {
		  FlipTxt(tl.textures[txt_id]);
	  }
  }

  file_node *old_node = GetDeepNodeG(root, Texture_library);
  file_node *new_node = CreateTxtLib(&tl, NULL, NULL);

  ReplaceNode(root, old_node, new_node);

  WriteUTF(root, file_name);
  FreeTreeData(root);
}

void FixTxt(txt *t)
{
  int i;
  int width, height;
  int x, y;
  int index;
  unsigned char *alpha=NULL;
  VRGB *rgb=NULL;
 
  for(i=1; i<t->mip_count; i++){
    FreeMip(t->mip_map[i]);
  }
  t->mip_count = 1;

  width=t->mip_map[0]->x_size;
  height=t->mip_map[0]->y_size;

  rgb=(VRGB*)Malloc((4*width*height*sizeof(VRGB))/3);
  for(y=0; y<height; y++){
     for(x=0; x<width; x++){
       index=t->mip_map[0]->image_indices[y*width+x];
       rgb[y*width+x].r=t->mip_map[0]->palette_rgb_888[3*index];
       rgb[y*width+x].g=t->mip_map[0]->palette_rgb_888[3*index+1];
       rgb[y*width+x].b=t->mip_map[0]->palette_rgb_888[3*index+2];
     }
   }

  if(t->mip_map[0]->alpha_8_bit){
    alpha=(unsigned char*)Malloc((4*width*height*sizeof(unsigned char))/3);
    for(y=0; y<height; y++){
       for(x=0; x<width; x++){
         alpha[y*width+x]=t->mip_map[0]->alpha_8_bit[y*width+x];
       }
    }
  }

  Free(t->mip_map[0]->palette_rgb_888);
  Free(t->mip_map[0]->image_indices);
  Free(t->mip_map[0]->alpha_8_bit);
  //Free(t->mip_map[0]->palette_8_bit);

  //t->mip_map[0]->depth = txt_depth;
  LoadTxtRGB(rgb, t);
  Free(rgb);
  
  if(alpha){
    LoadTxtAlpha(alpha, t);
    Free(alpha);
  }
}

int Winprint(const char *format, ...)
{
  va_list args;
  char buffer[256] = {0};

  va_start(args, format);
  vsprintf(buffer, format, args);
  va_end(args);

  fprintf(stderr,"%s\n",buffer);
  fflush(stderr);

  return 0;
}
