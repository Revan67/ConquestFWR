// IMagick.cpp
//
//
//


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

namespace ImageMagick
{
#include <ImageMagick/magick.h>
#if defined(__cplusplus) || defined(c_plusplus)
#undef class
#endif
}


#include "mtl_txt.h"
#include "names.h"
#include "rgbutils.h"

#if defined(USE_IMAGICK)

extern	int	txt_depth;
extern	int	keep_ext;

typedef unsigned char U8;
typedef unsigned short U16;

//
//
//
//
void IMagick2txm( char *file_name, txt_lib *tl, int keep_ext )
{
	VRGB *rgb; // Source data image
	unsigned char *alpha;
	txt *t;
	char txt_name[256] = {0};
	ImageMagick::ImageInfo im_imageinfo;
	ImageMagick::Image *im_image;
	S32 width, height, bpp;

	ImageMagick::GetImageInfo( &im_imageinfo );
	
	strcpy( im_imageinfo.filename, (const char *)file_name );
	
	if( (im_image = ImageMagick::ReadImage( &im_imageinfo )) == NULL ) {
		fprintf( stderr, "\nUnable to read image\n" );
		exit( 1 );
	}

	width = im_image->columns;
	height = im_image->rows;
	
	if( ImageMagick::IsPseudoClass( im_image ) && !im_image->matte ) {
		bpp = 1;
	}
	else if( im_image->matte ) {
		bpp = 4;
	}
	else {
		bpp = 3;
	}

	t = (txt *) Malloc(sizeof(txt));

	InitTexture(t);
	
	strcpy( txt_name, file_name );
	
	StripPath( txt_name );
	
	if( !keep_ext ) {
		StripExtension(txt_name);
	}

	t->name=(char*)Malloc((strlen(txt_name)+1)*sizeof(char));
	strcpy(t->name, txt_name);
	
	t->mip_count=1;
	t->mip_map=(mip**)Malloc(sizeof(mip*));
	t->mip_map[0]=(mip*)Malloc(sizeof(mip));
	InitMip(t->mip_map[0]);
	t->mip_map[0]->level=(char*)Malloc((strlen(MIP_level_0)+1)*sizeof(char));
	strcpy(t->mip_map[0]->level, MIP_level_0);
	t->mip_map[0]->x_size=width;
	t->mip_map[0]->y_size=height;
	t->mip_map[0]->color_count=256;
	t->mip_map[0]->depth	=(color_depth)txt_depth;

	if((bpp == 3) || (bpp == 1)) {
		// 1/3 for mip data
		rgb=(VRGB*)malloc((4*width*height*sizeof(VRGB))/3); 
		
		ImageMagick::SyncImage( im_image );
		ImageMagick::GetPixels( im_image, 0, 0, width, height, "RGB", ImageMagick::CharPixel, rgb );
		LoadTxtRGB(rgb, t);

		free(rgb);
	}
	else if(bpp == 4) {
		// 1/3 for mip data
		rgb=(VRGB*)Malloc((4*width*height*sizeof(VRGB))/3); 
		alpha=(unsigned char*)Malloc((4*width*height*sizeof(unsigned char))/3);
		
		ImageMagick::SyncImage( im_image );
		ImageMagick::GetPixels( im_image, 0, 0, width, height, "RGB", ImageMagick::CharPixel, rgb );
		ImageMagick::GetPixels( im_image, 0, 0, width, height, "A", ImageMagick::CharPixel, alpha );
		LoadTxtRGB(rgb, t);
	
		free(rgb);

		LoadTxtAlpha(alpha, t);
		Free(alpha);
	}
	else{
		fprintf(stderr,"Unknown format %d.\n", bpp );
	}

	InsertTexture( tl, t );
}

void	IMagick2txm2(char *file_name, char *afile_name, txt_lib *tl, int keep_ext)
{
	VRGB			*rgb; // Source data image
	unsigned char	*alpha;
	txt				*t;
	char			txt_name[256]	={0};
	char			alpha_txt_name[256]	={0};
	S32				width, height, bpp;

	ImageMagick::ImageInfo	im_imageinfo;
	ImageMagick::Image		*im_image;

	ImageMagick::GetImageInfo(&im_imageinfo);
	
	strcpy(im_imageinfo.filename, (const char *)file_name);
	
	if((im_image = ImageMagick::ReadImage(&im_imageinfo)) == NULL)
	{
		fprintf(stderr, "\nUnable to read image\n");
		exit(1);
	}

	width	=im_image->columns;
	height	=im_image->rows;
	
	if(ImageMagick::IsPseudoClass(im_image))
	{
		bpp	=1;
	}
	else if(im_image->matte)
	{
		bpp	=4;
		fprintf(stderr, "\nWARNING:  2nd Alpha image specified to combine with an image with existing alpha.  Using existing\n");
	}
	else
	{
		bpp	=3;
	}

	t	=(txt *)Malloc(sizeof(txt));

	InitTexture(t);
	
	strcpy(txt_name, file_name);
	
	StripPath(txt_name);
	
	if(!keep_ext)
	{
		StripExtension(txt_name);
	}

	strcpy(alpha_txt_name, afile_name);
	
	StripPath(alpha_txt_name);
	
	if(!keep_ext)
	{
		StripExtension(alpha_txt_name);
	}

	if(alpha_txt_name)
	{
		t->name	=(char*)Malloc((strlen(txt_name) + strlen(alpha_txt_name) + 2) * sizeof(char));
		sprintf(t->name, "%s_%s", txt_name, alpha_txt_name);
	}
	else
	{
		t->name	=(char*)Malloc((strlen(txt_name) + 1) * sizeof(char));
		strcpy(t->name, txt_name);
	}
	
	t->mip_count=1;
	t->mip_map=(mip**)Malloc(sizeof(mip*));
	t->mip_map[0]=(mip*)Malloc(sizeof(mip));
	InitMip(t->mip_map[0]);
	t->mip_map[0]->level=(char*)Malloc((strlen(MIP_level_0)+1)*sizeof(char));
	strcpy(t->mip_map[0]->level, MIP_level_0);
	t->mip_map[0]->x_size=width;
	t->mip_map[0]->y_size=height;
	t->mip_map[0]->color_count=256;
	t->mip_map[0]->depth	=(color_depth)txt_depth;

	if((bpp == 3) || (bpp == 1)) {
		// 1/3 for mip data
		rgb=(VRGB*)malloc((4*width*height*sizeof(VRGB))/3); 
		
		ImageMagick::SyncImage( im_image );
		ImageMagick::GetPixels( im_image, 0, 0, width, height, "RGB", ImageMagick::CharPixel, rgb );
		LoadTxtRGB(rgb, t);

		free(rgb);

		if(afile_name)	//load and gscale the secondary alpha image
		{
			ImageMagick::ImageInfo	im_aimageinfo;
			ImageMagick::Image		*im_aimage;

			ImageMagick::GetImageInfo(&im_aimageinfo);
			
			strcpy(im_aimageinfo.filename, (const char *)afile_name);
			
			if((im_aimage = ImageMagick::ReadImage(&im_aimageinfo)) == NULL)
			{
				fprintf(stderr, "\nUnable to read alpha image\n");
				exit(1);
			}

			if((width != im_image->columns) || (height !=im_image->rows))
			{
				fprintf(stderr, "Alpha image is a different size than the original\n");
			}
			else
			{
				alpha	=(unsigned char*)Malloc((4*width*height*sizeof(unsigned char))/3);

				ImageMagick::RGBTransformImage(im_aimage, ImageMagick::GRAYColorspace); 
				ImageMagick::GetPixels(im_aimage, 0, 0, width, height, "R", ImageMagick::CharPixel, alpha);

				LoadTxtAlpha(alpha, t);
				free(alpha);
			}
		}
	}
	else if(bpp == 4)
	{
		//not sure if this is the right thing to do
		fprintf(stderr, "Alpha exists in source image, ignoring second alpha\n");

		// 1/3 for mip data
		rgb=(VRGB*)Malloc((4*width*height*sizeof(VRGB))/3); 
		alpha=(unsigned char*)Malloc((4*width*height*sizeof(unsigned char))/3);
		
		ImageMagick::SyncImage( im_image );
		ImageMagick::GetPixels( im_image, 0, 0, width, height, "RGB", ImageMagick::CharPixel, rgb );
		ImageMagick::GetPixels( im_image, 0, 0, width, height, "A", ImageMagick::CharPixel, alpha );
		LoadTxtRGB(rgb, t);
	
		free(rgb);

		LoadTxtAlpha(alpha, t);
		Free(alpha);
	}
	else{
		fprintf(stderr,"Unknown format %d.\n", bpp );
	}

	InsertTexture( tl, t );
}

void	IMagick2TGA(ImageMagick::Image *im_image, file_node *root_out)
{
	char					file_name[MAX_PATH], tempfn[TMP_MAX];
	ImageMagick::ImageInfo	im_imageinfo;

	ImageMagick::GetImageInfo(&im_imageinfo);
	strcpy(file_name, im_image->filename);
	
	tmpnam(tempfn);
	if(strrchr(tempfn, '.'))
	{
		*strrchr(tempfn, '.')	=0;
	}
	strcat(tempfn, ".tga");
	strcpy(im_image->filename, tempfn);
	strcpy(im_imageinfo.filename, (const char *)tempfn);

	ImageMagick::WriteImage(&im_imageinfo, im_image);
	{
		file_node	*node		=(file_node *)Malloc(sizeof(file_node));
		FILE		*tempfile	=fopen(tempfn, "r");

		//get rid of path stuff
		if(strrchr(file_name, '\\'))
		{
			node->name	=(char *)Malloc((strlen(strrchr(file_name, '\\')) + 1) * sizeof(char));
			strcpy(node->name, strrchr(file_name, '\\'));
		}
		else if(strrchr(file_name, '/'))
		{
			node->name	=(char *)Malloc((strlen(strrchr(file_name, '/')) + 1) * sizeof(char));
			strcpy(node->name, strrchr(file_name, '/'));
		}
		else
		{
			node->name	=(char *)Malloc((strlen(file_name) + 1) * sizeof(char));
			strcpy(node->name, file_name);
		}

		if(!keep_ext)
		{
			if(strrchr(node->name, '.'))
			{
				*strrchr(node->name, '.')	=0;
			}
		}

		node->type		=F;
		node->mem		=M;

		fseek(tempfile, 0, SEEK_END);

		node->data_size	=ftell(tempfile);
		node->data		=(unsigned char *)Malloc(node->data_size);
		node->sibling	=NULL;
		node->child		=NULL;

		fseek(tempfile, 0, SEEK_SET);
		fread(node->data, 1, node->data_size, tempfile);

		fclose(tempfile);

		InsertTxt(root_out, node);
	}
	unlink(tempfn);
}

void	IMagickFile2TGA(const char *file_name, file_node *root_out)
{
	ImageMagick::ImageInfo	im_imageinfo;
	ImageMagick::Image		*im_image;

	ImageMagick::GetImageInfo(&im_imageinfo);
	
	strcpy(im_imageinfo.filename, (const char *)file_name);
	
	if(!(im_image = ImageMagick::ReadImage(&im_imageinfo)))
	{
		fprintf(stderr, "\nUnable to read image\n");
		exit(69);
	}

	IMagick2TGA(im_image, root_out);
}

#else

void IMagick2txm( char *file_name, txt_lib *tl, int keep_ext )
{
	fprintf( stderr, "Unable to process '%s', no imagemagick support.\n", file_name );
}

#endif