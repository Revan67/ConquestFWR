
#include <io.h>
#include <stdio.h>
#include <iostream>
using namespace std;
#include <windows.h>
#include "resource.h"

#include "..\..\app\src\IImageReader.h"

#include <DACOM.h>

//#include "..\..\app\src\include\globals.h"
#include "pixel.h"
#include <TSmartPointer.H>

//TGARead.cpp
void __stdcall CreateTGAReader (struct IImageReader ** reader);

static __int32 num_images = 0;

#define DISPLAY_WIDTH 234
#define MAX_WIDTH 3000
#define MAX_HEIGHT 3000
#define MAX_PATHLEN 1000

#define BACK_RED 63
#define BACK_GREEN 0
#define BACK_BLUE 63

#define BACK_INDEX 255
#define MAX_ICONS 5000

char SHPFileName[MAX_PATHLEN];

BYTE *DisplayBits;

HBITMAP hBmp;
HWND hShpDlg;
HWND hwndList;

U8 image[MAX_WIDTH][MAX_HEIGHT];

int pix_pos;


U8* new_icon[MAX_ICONS];
U32 new_length[MAX_ICONS];
U8* new_palette[MAX_ICONS];

U8* missingData=0;
U8* missingPal=0;
DWORD missingLength = 0;


void UpdateListBox()
{
	SendMessage(hwndList, LB_RESETCONTENT, 0, (LPARAM) 0);

	char image_number_string[100];

	for (int i = 0; i < num_images; i++) 
	{ 
		if (new_icon[i])
		{
			sprintf(image_number_string, "%d", i);
			int newStringIndex = SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM) image_number_string); 
			SendMessage(hwndList, LB_SETITEMDATA, newStringIndex, (LPARAM) i);
		}
	}
}

void __fastcall put_pix(int y, int clr)
{
	//if (pix_pos>=max_pix_x) max_pix_x=pix_pos+1;  //some SP info
	//if (y>=max_pix_y) max_pix_y=y+1;
	image[pix_pos][y]=(U8)clr;
	
	++pix_pos;
}


/*
void NEWDisplayImage2(int index)
{
	if (!new_icon[index]) return;

	for (int ii = 0; ii < DISPLAY_WIDTH; ii++)
		for (int jj = 0; jj < DISPLAY_WIDTH; jj++)
		{
			U8 r,g,b;
			if (ii < new_width && jj < new_height)
			{
				r = new_icon[index][(jj * new_width + ii) * 4];
				g = new_icon[index][(jj * new_width + ii) * 4+1];
				b = new_icon[index][(jj * new_width + ii) * 4+2];
			}
			else r = g = b = 0;
			
			DisplayBits[4 * (DISPLAY_WIDTH * jj + ii)] = b;
			DisplayBits[4 * (DISPLAY_WIDTH * jj + ii)+1] = g;
			DisplayBits[4 * (DISPLAY_WIDTH * jj + ii)+2] = r;
		}


	RedrawWindow(hShpDlg, NULL, NULL, RDW_INVALIDATE | RDW_INTERNALPAINT | RDW_ERASE);
}*/



void DisplayImage(U32 image_height, U32 image_width, U32 data_length, U8* palette, U8* image_data)
{
	int ch,b,r,i,l=0;

	for (i = 0; i < MAX_WIDTH; i++)
		memset(image[i], BACK_INDEX, MAX_HEIGHT);
	
	pix_pos=0;
	//	far_right=0;
	//	far_bottom=0;
	
	int next = 0;
	
	U8* image_end = image_data + data_length;
	
	while(true)
	{
		// read data  and decode
		ch=	*image_data++;
		
		//		if (ch==-1) break;// for last image this is end of shp
		
		if (image_data > image_end) break;
		
		r=ch%2;
		b=ch/2;
		if (b==0 && r==1) // a skip over
		{
			ch=	*image_data++;
			for (i=0; i<ch; ++i)
				put_pix(l,BACK_INDEX);
		}
		else if (b==0)   // end of line
		{
			++l; 
			pix_pos=0;
		}
		else if (r==0) // a run of bytes
		{
			ch=	*image_data++; // the color #
			for (i=0; i<b; ++i) put_pix(l,ch);
		}
		else // b!0 and r==1 ... read the next b bytes as color #'s
		{
			for (i=0; i<b; ++i)
			{
				ch=	*image_data++;
				put_pix(l,ch);
			}
		}
	} 
	for (U32 ii = 0; ii < DISPLAY_WIDTH; ii++)
		for (U32 jj = 0; jj < DISPLAY_WIDTH; jj++)
		{
			U8 r,g,b;
			if (ii < image_width && jj < image_height)
			{	
				U8 color_index = image[ii][jj];
				if (color_index == BACK_INDEX)
				{
					r = BACK_RED;
					g = BACK_GREEN;
					b = BACK_BLUE;
				}
				else
				{
					r = palette[(color_index +1)* 4+1];
					g = palette[(color_index +1)* 4+2];
					b = palette[(color_index +1)* 4+3];
				}
			}
			else r = g = b = 0;
			
			DisplayBits[4 * (DISPLAY_WIDTH * jj + ii)] = b * 4;
			DisplayBits[4 * (DISPLAY_WIDTH * jj + ii)+1] = g * 4;
			DisplayBits[4 * (DISPLAY_WIDTH * jj + ii)+2] = r * 4;
		}
		
		HWND hWndStatic = ::GetDlgItem(hShpDlg, IDC_DIMENSIONS);
		char tmpstring[100];
		sprintf(tmpstring, "%d x %d",image_width,image_height);
		if (hWndStatic) ::SetWindowText(hWndStatic, tmpstring);
		
		RedrawWindow(hShpDlg, NULL, NULL, RDW_INVALIDATE | RDW_INTERNALPAINT | RDW_ERASE);
}


void NEWDisplayImage(int index)
{
	if (!new_icon[index]) return;
	if (!new_palette[index]) return;
	
	U32 image_width = 1 + *(__int32*)(new_icon[index] + 16);
	U32 image_height = 1 + *(__int32*)(new_icon[index]+20);
		
	DisplayImage(image_height, image_width,new_length[index]-24,new_palette[index],new_icon[index]+24);
}


/*void SHPDisplayImage(int index)
{
	
	__int32 image_offset = *(__int32*)(SHPbuf+8 * (index+1));
	U32 image_width = 1 + *(__int32*)(SHPbuf+image_offset+16);
	U32 image_height = 1 + *(__int32*)(SHPbuf+image_offset+20);
	__int32 palette_offset = *(__int32*)(SHPbuf+8 * (index+1)+4);
	
	cout << "image height " <<image_height << endl;
	cout << "image width " <<image_width << endl;
	
	__int32 image_data = image_offset + 24;
	
	DisplayImage(image_height, image_width, palette_offset - image_data, 
				 SHPbuf+palette_offset, SHPbuf+image_data);
	
}*/


void addChar(U8 c, U8* &writepos)
{
	writepos[0] = c;
	writepos++;
}

static U32 ColorSpace[64][64][64];
static U8 ColorMapping[64][64][64];

struct ColorSpacePlane
{
	U8 axis;
	U32 value;
	U32 my_index;
	ColorSpacePlane* above;
	ColorSpacePlane* below;
};

struct ColorSpaceCube
{
	U32 start[3];
	U32 median[3];
	U32 end[3];
	U32 priority;
	U32 level;
	ColorSpaceCube* next;
	ColorSpaceCube* prev;
	ColorSpaceCube()
	{
		next = prev = NULL;
	}
};

U32 ColorCubeListLength;
ColorSpaceCube* ColorCubeList;

void InsertColorSpaceCube(ColorSpaceCube* insertee, ColorSpaceCube* list)
{
	if (!ColorCubeList) ColorCubeList = insertee;
	else
	{
		if (insertee->priority > list->priority)
		{
			if (ColorCubeList == list) ColorCubeList = insertee;
			insertee->next = list;
			insertee->prev = list->prev;
			if (insertee->prev) insertee->prev->next = insertee;
			list->prev = insertee;
		}
		else
		{
			if (!list->next)
			{
				insertee->prev = list;
				insertee->next = NULL;
				list->next = insertee;
			}
			else InsertColorSpaceCube(insertee, list->next);
		}
	}
}

void RemoveColorSpaceCube(ColorSpaceCube* removee)
{
	if (ColorCubeList == removee)
		ColorCubeList = removee->next;
	if (removee->next) removee->next->prev = removee->prev;
	if (removee->prev) removee->prev->next = removee->next;
	removee->next = removee->prev = NULL;
	if (removee == ColorCubeList) ColorCubeList = NULL;
}

U32 next_palette_index;
#define MAX_PALETTE_INDEX 254


bool findMedian(ColorSpaceCube* theCube)
{
	U32 numPixels = 0;
	U32 numColors = 0;
	ZeroMemory(theCube->median,3 * 4);

	U32 realStart[3];
	U32 realEnd[3];
	realStart[0] = realStart[1] = realStart[2] =64;
	realEnd[0] = realEnd[1] = realEnd[2] =0;


	for (U32 i = theCube->start[0]; i < theCube->end[0]; i++)
		for (U32 j = theCube->start[1]; j < theCube->end[1]; j++)
			for (U32 k = theCube->start[2]; k < theCube->end[2]; k++)
			{
				theCube->median[0] += i * ColorSpace[i][j][k];
				theCube->median[1] += j * ColorSpace[i][j][k];
				theCube->median[2] += k * ColorSpace[i][j][k];
				numPixels += ColorSpace[i][j][k];
				if (ColorSpace[i][j][k]) 
				{
					realEnd[0] = max(realEnd[0], i+1);
					realEnd[1] = max(realEnd[1], j+1);
					realEnd[2] = max(realEnd[2], k+1);
					realStart[0] = min(realStart[0], i);
					realStart[1] = min(realStart[1], j);
					realStart[2] = min(realStart[2], k);
					numColors++;
				}
			}

	theCube->priority = numColors;
	//if (theCube->priority > 1) theCube->priority = numPixels;
	//if (theCube->priority > 1) 
	//		theCube->priority = sqrt((realEnd[0]-realStart[0])*(realEnd[0]-realStart[0]) 
	//								+ (realEnd[1]-realStart[1])*(realEnd[1]-realStart[1]) 
	//								+ (realEnd[2]-realStart[2])*(realEnd[2]-realStart[2]));


	for (i = 0; i < 3; i++)
	{
		theCube->end[i] = realEnd[i];
		theCube->start[i] = realStart[i];
	//	if (theCube->priority > 1) 
	//		theCube->priority = max(theCube->priority, realEnd[i] - realStart[i]);

	}
	
	
	
	if (numPixels == 0) return false;  // empty colorspace

	for (i = 0; i < 3; i++)
	{
		theCube->median[i] /= numPixels;
	}
	
	return true;
}

void divideColorCube()
{
	int i;
	ColorCubeListLength = 1;
	ColorCubeList = new ColorSpaceCube();
	
	ColorCubeList->start[0] = 0;
	ColorCubeList->start[1] = 0;
	ColorCubeList->start[2] = 0;
	ColorCubeList->end[0] = 64;
	ColorCubeList->end[1] = 64;
	ColorCubeList->end[2] = 64;

	findMedian(ColorCubeList);
	while (ColorCubeListLength < MAX_PALETTE_INDEX && ColorCubeList->priority > 1)
	{
		ColorSpaceCube* theCube = ColorCubeList;
		RemoveColorSpaceCube(ColorCubeList);
		ColorCubeListLength--;
		//cout << "removing " << ColorCubeListLength << endl;
		ColorSpaceCube* newCubeA = new ColorSpaceCube();
		ColorSpaceCube* newCubeB = new ColorSpaceCube();
		newCubeA->level = newCubeB->level = theCube->level+1;
		for (i = 0; i < 3; i++)
		{
			newCubeA->start[i] = newCubeB->start[i] = theCube->start[i];
			newCubeA->end[i] = newCubeB->end[i] = theCube->end[i];
		}
		
		U8 axis;
		if (theCube->end[0] - theCube->start[0] > max(theCube->end[1] - theCube->start[1], theCube->end[2] - theCube->start[2]))
			axis = 0;
		else if (theCube->end[1] - theCube->start[1] > theCube->end[2] - theCube->start[2])
			axis = 1;
		else
			axis = 2;


		if (theCube->median[axis] == theCube->end[axis]) theCube->median[axis]--;
		if (theCube->median[axis] == theCube->start[axis]) theCube->median[axis]++;
		
		newCubeA->end[axis] = newCubeB->start[axis] = theCube->median[axis];
		delete theCube;
		if (findMedian(newCubeA)) 
		{
			InsertColorSpaceCube(newCubeA,ColorCubeList);
			ColorCubeListLength++;
			//cout << "insertingA " << ColorCubeListLength << endl;
		}
		else delete newCubeA;
		if (findMedian(newCubeB)) 
		{
			InsertColorSpaceCube(newCubeB,ColorCubeList);
			ColorCubeListLength++;
			//cout << "insertingB " << ColorCubeListLength << endl;
		}
		else delete newCubeB;
	}
}



void createPalette(U8* palette, U8* image, U32 width,U32 height)
{
	U32 i,j;
	for (i = 0; i < 64; i++)
		for (j = 0; j < 64; j++)
		{
			ZeroMemory(ColorSpace[i][j],64 * 4);
			ZeroMemory(ColorMapping[i][j],64);
		}

	for (i = 0; i < width; i++)
		for (j = 0; j < height; j++)
		{ 
			int r = image[(j * width + i) * 4];
			int g = image[(j * width + i) * 4+1];
			int b = image[(j * width + i) * 4+2];
			
			//ColorSpace[r/4][g/4][b/4]++;
			ColorSpace[r/4][g/4][b/4]=1;
		} 

	ColorSpace[BACK_RED][BACK_GREEN][BACK_BLUE] = 0; // we can ignore the background color during palette-selection
	divideColorCube();

	int paletteIndex = 1;

	while(ColorCubeList && paletteIndex <= MAX_PALETTE_INDEX)
	{
		ColorSpaceCube* theCube = ColorCubeList;
		RemoveColorSpaceCube(ColorCubeList);

		for (U32 i = theCube->start[0]; i < theCube->end[0]; i++)
			for (U32 j = theCube->start[1]; j < theCube->end[1]; j++)
				for (U32 k = theCube->start[2]; k < theCube->end[2]; k++)
				{
					//if (ColorMapping[i][j][k]) cout << "ALREADY MAPPED SPACE2! " << (int)ColorMapping[i][j][k] << endl;
					ColorMapping[i][j][k] = paletteIndex;
				}
		palette[4+paletteIndex*4+1]= (U8) theCube->median[0];
		palette[4+paletteIndex*4+2]= (U8) theCube->median[1];
		palette[4+paletteIndex*4+3]= (U8) theCube->median[2];
		paletteIndex++;
		delete theCube;
	}

	ColorMapping[BACK_RED][BACK_GREEN][BACK_BLUE] = BACK_INDEX;

	ColorCubeList = NULL;

}



bool OpenTGA(int index)
{
	U32 i,j;
	ICOManager *DACOM = DACOM_Acquire();
	
	//if (DACOM->SetINIConfig(cmd_line_ini) != GR_OK)
	{
		//	cout << "Open failed on INI file "<< cmd_line_ini << endl;
		//	endl;
	}
	
	
	OPENFILENAME filedlg;
	char FileName[1000]="";
    memset(&filedlg,0,sizeof(filedlg));
    filedlg.lStructSize = sizeof(filedlg);
    filedlg.Flags = OFN_HIDEREADONLY;
    filedlg.lpstrFile=FileName; 
	filedlg.nMaxFile=1000;
    filedlg.lpstrFilter="TGA File\0*.tga\0";
    if(GetOpenFileName(&filedlg) == FALSE) return false;
	
	COMPTR<IImageReader> reader;
	
	FILE* f;

	//cout << "opening " << FileName << endl;
	U8* filebuf = 0;
	U32 buflen = 0;
	f = fopen(FileName, "rb");
	if (f)
	{
		fseek(f,0,SEEK_END);
		buflen=ftell(f);
		fseek(f,0,SEEK_SET);
		
		// allocate memory and read the entire file
		filebuf = new U8[buflen];
		if (filebuf)
		{
			U32 bytesRead = fread( filebuf, sizeof(char), buflen, f );
			if (bytesRead!=buflen) 
			{
				delete [] filebuf;
				filebuf = NULL;
				buflen = 0;
			}
		}
		fclose (f);
	}
	if (!filebuf) 
	{
		return false;
	}

		
	U32 subImage = 0;
	CreateTGAReader(reader);
	reader->LoadImage(filebuf, buflen, subImage);
	
	U32 new_width = reader->GetWidth();
	U32 new_height = reader->GetHeight();


	U8* TGAbuffer = new U8[reader->GetWidth() * reader->GetHeight()*4];
	if (!TGAbuffer) 
	{
		return false;
	}

	reader->GetImage(PF_RGBA, TGAbuffer);

	new_palette[index] = new U8[1024];

	if (!new_palette[index]) return false;

	new_palette[index][0]= 255;
	new_palette[index][1]= 0;
	new_palette[index][2]= 0;
	new_palette[index][3]= 0;
	
	for (i = 0; i < 255; i++) new_palette[index][4+i*4]= (U8) i; // place index-numbers in palette (for some reason...?)
	
	createPalette(new_palette[index], TGAbuffer, new_width, new_height);

		for (i = 0; i < new_width; i++)
		for (j = 0; j < new_height; j++)
		{
			int r = TGAbuffer[(j * new_width + i) * 4];
			int g = TGAbuffer[(j * new_width + i) * 4+1];
			int b = TGAbuffer[(j * new_width + i) * 4+2];
			image[i][j] = ColorMapping[r/4][g/4][b/4];
		}

	delete [] TGAbuffer;

	U8* tempBuffer = new U8[reader->GetWidth() * reader->GetHeight() * 2];
	if (!tempBuffer) 
	{
		return false;
	}
	U8* writepos = tempBuffer;

	new_height--;
	new_width--;

	int cnum;
	U8 cmp[2048];

    addChar((U8) (new_height%256),writepos);
    addChar((U8) (new_height/256),writepos);
    addChar((U8) (new_width%256),writepos);
    addChar((U8) (new_width/256),writepos);
    for (int k = 0; k < 12; k++) addChar(0,writepos); 
    addChar((U8) (new_width%256),writepos); 
	addChar((U8) (new_width/256),writepos);
    addChar(0,writepos); 
	addChar(0,writepos);
    addChar((U8) (new_height%256),writepos); 
	addChar((U8) (new_height/256),writepos);
    addChar(0,writepos); 
	addChar(0,writepos);

  for (U32 y=0; y<=new_height; ++y)
  {
    // compress a line of data
    // we will write a line of compressed data to cmp
    // keeping track of the end position in it with
    // endi and the counter position with cti;
    // x will be the position in image
    U32 x=0; U32 endi=0; U32 cti=0;
    do
    {
      cmp[endi]=0;
      if (image[x][y]==BACK_INDEX) // transparent area
      {
        cmp[endi]=1; // ie repeat transparent
        ++endi;
        cti=endi;
        cmp[endi]=0;
        ++endi;     // next place to write in cmp
        cmp[endi]=0;
        while (image[x][y]==BACK_INDEX && x<=new_width && cmp[cti]<255)
        {
          ++cmp[cti];
          ++x;
        } 
        if (x>=new_width && endi>2)     // this is transparent finishing row;
                         // add && endi>2 for blank line...0.46
        {
         --endi; --endi;
         cmp[endi]=0;
        }
      } // end run of back color
      else if (image[x][y]==image[x+1][y]&&
            image[x][y]==image[x+2][y]) // a run of at least 3 same
      {
        cti=endi;
        ++endi;
        cmp[cti]=2; // even is same
        cmp[endi]=image[x][y];
        cnum=image[x][y];
        ++endi;
        cmp[endi]=0;
        ++x;
       // while (cnum==image[x][y] && x<new_width &&cmp[cti]<0xFE) V 0.42
        while (cnum==image[x][y] && x<=new_width &&cmp[cti]<0xFE) //V0.45
        {
          ++x;
          cmp[cti]+=2;
        }
      }    // end run of same
      else // a run of different
      {
         cti=endi;
         cmp[cti]=1;  // odd for a run of different
         ++endi;
         while ((image[x][y]!=image[x+1][y]|| image[x][y]!=image[x+2][y])
                   && x<=new_width && cmp[cti]<0xFF  
                   && image[x][y]!=BACK_INDEX)  
         {
           cmp[cti]+=2;
           cmp[endi]=image[x][y];
           ++endi;
           cmp[endi]=0;
           ++x;
         }
      } // end run of different
    }while (x<=new_width);  

    if (endi>0)
    for (U32 i=0; i<=endi; ++i)
    {
      addChar(cmp[i],writepos);
    }
  }

	if (new_icon[index]) delete[] new_icon[index];
	new_icon[index] = new U8[writepos - tempBuffer];

	new_length[index] = writepos - tempBuffer;
	
	if (!new_icon[index]) return false;
	memcpy(new_icon[index], tempBuffer, writepos - tempBuffer);
	delete[] tempBuffer;
	
	return true;
}

void ClearSHP()
{
	for (int i = 0 ; i < MAX_ICONS; i++)
		{
			if (new_icon[i]) 
			{
				delete[] new_icon[i];
				new_icon[i] = 0;
			}
			if (new_palette[i])
			{
				delete[] new_palette[i];
				new_palette[i] = 0;
			}
		}
	num_images = 0;
	for (i = 0; i < MAX_WIDTH; i++)
		memset(image[i], BACK_INDEX, MAX_HEIGHT);
	
}

bool OpenSHP(bool file_dialog = TRUE)
{
	U32 i;

	if (file_dialog)
	{
		OPENFILENAME file;
		
	    memset(&file,0,sizeof(file));
	    file.lStructSize = sizeof(file);
	    file.Flags = OFN_HIDEREADONLY;
	    file.lpstrFile=SHPFileName; 
		file.nMaxFile=1000;
	    file.lpstrFilter="SHP File\0*.shp\0";
	    if(GetOpenFileName(&file) == FALSE) return false;
	}
	
	ClearSHP();

	FILE* f;
	U8* SHPbuf;

	long SHPbuflen;

	//cout << "opening " << SHPFileName << endl;
	f = fopen(SHPFileName, "rb");
	if (f)
	{
		fseek(f,0,SEEK_END);
		SHPbuflen=ftell(f);
		fseek(f,0,SEEK_SET);
		
		// allocate memory and read the entire file
		SHPbuf = new U8[SHPbuflen];
		if (SHPbuf)
		{
			long bytesRead = fread( SHPbuf, sizeof(char), SHPbuflen, f );
			if (bytesRead!=SHPbuflen) 
			{
				delete [] SHPbuf;
				SHPbuf = NULL;
				SHPbuflen = 0;
			}
		}
		fclose (f);
	}
	if (!SHPbuf) 
		MessageBox(NULL,"Failed to open SHP file!", "Unhappy MessageBox!", MB_OK);
	
	U32 num_image_entries = *(__int32*)(SHPbuf+4);
	//cout << "This file contains "<< num_image_entries << " images" << endl;
	
	
	for (i = 0; i < num_image_entries; i++) 
	{ 
		__int32 image_offset = *(__int32*)(SHPbuf+8 * (i+1));
		U32 image_width = 1 + *(__int32*)(SHPbuf+image_offset+16);
		U32 image_height = 1 + *(__int32*)(SHPbuf+image_offset+20);
		__int32 palette_offset = *(__int32*)(SHPbuf+8 * (i+1)+4);
		
		
		new_length[i] = palette_offset - image_offset;
	
		if (new_length[i] != missingLength
			|| memcmp(SHPbuf+image_offset, missingData, missingLength))  // we will hide the "missing image" icon from the user
		{
			num_images = i+1;
			new_icon[i] = new U8[new_length[i]];
			new_palette[i] = new U8[1024];
			memcpy(new_icon[i], SHPbuf+image_offset, new_length[i]);	
			memcpy(new_palette[i], SHPbuf+palette_offset, 1024);	

			if (!new_palette[i] || !new_icon[i] ) 
			{
				MessageBox(NULL,"New failed!",NULL, MB_OK);
				exit(0);
			}

		}
	

	} 
	
	delete[] SHPbuf;

	SetFocus(hwndList); 

	return true;
}

bool SaveSHP()
{
	if (num_images == 0) return false;
	int i;
	FILE* f;

	//cout << "saving " << SHPFileName << endl;
	f = fopen(SHPFileName, "wb");
	if (!f) return false;

	
	fwrite("1.10",4,1,f);
	fwrite(&num_images,4,1,f);
	
	U32 next_entry = 8 + (num_images * 8);

	U32 missing_image_entry = 0;
	U32 missing_palette_entry = 0;
	

	for (i = 0; i < num_images; i++)
	{
		size_t DataLength;
		if (new_icon[i])
		{
			DataLength = new_length[i];
			fwrite(&next_entry, 4, 1, f);
			next_entry += DataLength;
			fwrite(&next_entry, 4, 1, f);
			next_entry += 1024;
		}
		else
		{
			if (missing_image_entry)
			{
				fwrite(&missing_image_entry, 4, 1, f);
				fwrite(&missing_palette_entry, 4, 1, f);
			}
			else
			{
				missing_image_entry = next_entry;
				missing_palette_entry = missing_image_entry + missingLength;
				fwrite(&next_entry, 4, 1, f);
				next_entry += missingLength;
				fwrite(&next_entry, 4, 1, f);
				next_entry += 1024;
			}
		}
	}
	bool missing_image_written = false;
	for (i = 0; i < num_images; i++)
	{
		if (new_icon[i])
		{
			size_t DataLength = new_length[i];
			fwrite(new_icon[i], DataLength, 1, f);
			fwrite(new_palette[i], 1024, 1, f);
		}
		else
		{
			if (!missing_image_written)
			{
				fwrite(missingData, missingLength, 1, f);
				fwrite(missingPal, 1024, 1, f);
				missing_image_written = true;
			}

		}
	}

	fclose (f);
	
	return true;
}

bool SaveAsSHP()
{
	if (num_images == 0) return false;
	
	OPENFILENAME file;
	
    memset(&file,0,sizeof(file));
    file.lStructSize = sizeof(file);
    file.Flags = OFN_HIDEREADONLY;
    file.lpstrFile=SHPFileName; 
	file.nMaxFile=1000;
    file.lpstrFilter="SHP File\0*.shp\0";
    if(GetSaveFileName(&file) == FALSE) return false;
	int i;
	for (i = 0; i < MAX_PATHLEN-4 && SHPFileName[i]; i++){}
	//cout << "i = " << i << endl;
	if (i > 4)
	{
		if (SHPFileName[i-4] != '.')
		{
			SHPFileName[i] = '.';
			SHPFileName[i+1] = 's';
			SHPFileName[i+2] = 'h';
			SHPFileName[i+3] = 'p';
			SHPFileName[i+4] = 0;
		}
	}
	return SaveSHP();
}


static int InputNumber;

LRESULT CALLBACK NumDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{	
	switch( uMsg )
	{
	case WM_INITDIALOG:
		{
			char EditText[100];
			sprintf(EditText,"%d",num_images);
			HWND hwndEdit = GetDlgItem(hDlg, IDC_EDIT1);
			SendMessage(hwndEdit, EM_REPLACESEL, (WPARAM)0, (LPARAM) EditText);
		}
		break;
	case WM_COMMAND:
		switch( wParam )
		{ 
			case IDOK:
				{
					char EditData[100];
					EditData[0] = 99;
					HWND hwndEdit = GetDlgItem(hDlg, IDC_EDIT1);
					int chars_copied = SendMessage(hwndEdit, EM_GETLINE, (WPARAM)0, (LPARAM) EditData);
					if (chars_copied == 0) EndDialog( hDlg, TRUE );
					EditData[chars_copied]=0;
					InputNumber = atoi(EditData);
					EndDialog( hDlg, TRUE );
				}
				break;
			case IDCANCEL:
				{
					InputNumber = -2;
					EndDialog( hDlg, TRUE );
				}
				break;
		}
		return TRUE;
		break;
	case WM_CLOSE:
		InputNumber = -2;
		EndDialog( hDlg, TRUE );
		return TRUE;
		break;
	}
	return FALSE;
}



int GetNumber()
{
	InputNumber = -1;
	HWND numdlg = ::CreateDialog(NULL,MAKEINTRESOURCE(IDD_GETNUMBER),NULL,(DLGPROC) NumDlgProc);
	if(numdlg!=NULL)
	{
		// show dialog
		::ShowWindow(numdlg,SW_SHOW);
	}
	else
	{
		printf("Failed to create dialog\n");
		return -1;
	}


	MSG msg;
	while(InputNumber == -1)
	{
		if(::PeekMessage(&msg,numdlg,0,0,PM_REMOVE))
		{
			// process message
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
		else 
		{
			Sleep(100);
		}
	}


	//cout << "got number " << InputNumber << endl;
	return InputNumber;
}


LRESULT CALLBACK ShpDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	HINSTANCE Inst;
	
	//cout << "shpdlgproc!" << endl;

	bool save_enable = (num_images > 0);
	//EnableWindow( GetDlgItem( hShpDlg, IDC_SAVE ), save_enable );
	EnableWindow( GetDlgItem( hShpDlg, IDC_SAVEAS ), save_enable );


	int selected_index = SendMessage(hwndList, LB_GETCURSEL, 0, 0);
	int selected_picture = SendMessage(hwndList, LB_GETITEMDATA, selected_index, 0); 
		
	bool delete_enable = (selected_picture != LB_ERR);
	EnableWindow( GetDlgItem( hShpDlg, IDC_DELETE ), delete_enable );
	EnableWindow( GetDlgItem( hShpDlg, IDC_REPLACE ), delete_enable );
		
	switch( uMsg )
	{
	case WM_PAINT:
		//cout << "painting!" << endl;
		
		
		PAINTSTRUCT ps;
		HDC hDC, hMemDC;
		
		// get the DCs
		hDC = BeginPaint( hDlg, &ps );
		hMemDC = CreateCompatibleDC( hDC );
		Inst = GetModuleHandle(0);
		
		SelectObject( hMemDC, hBmp );
		BitBlt( hDC, 220, 30, DISPLAY_WIDTH, DISPLAY_WIDTH, hMemDC, 0, 0, SRCCOPY );
		DeleteDC( hMemDC );
		EndPaint( hDlg, &ps );
		return TRUE;
		
	case WM_INITDIALOG:
		
		hwndList = GetDlgItem(hDlg, IDC_LIST1);
		
		
		
		return 0;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDC_LIST1 && HIWORD (wParam) == LBN_SELCHANGE)
		{
			int selected_index = SendMessage(hwndList, LB_GETCURSEL, 0, 0);
			//if (selected_index == LB_ERR) cout << "ERROR IN SELECTION!" << endl;
			int selected_picture = SendMessage(hwndList, LB_GETITEMDATA, selected_index, 0); 
			//if (selected_picture == LB_ERR) cout << "ERROR IN SELECTION!" << endl;
			
			//cout << "index " << selected_index << " picture " << selected_picture << endl;
			if (new_icon[selected_picture]) NEWDisplayImage(selected_picture);
		//	else SHPDisplayImage(selected_picture);
		}
		
		else switch( wParam )
		{ 
			case IDC_NEW:
			{
				SHPFileName[0] = 0;
				
					HWND hWndStatic = ::GetDlgItem(hShpDlg, IDC_FILENAME);
					if (hWndStatic) ::SetWindowText(hWndStatic, SHPFileName);

				ClearSHP();
				
				UpdateListBox();
				for (int i = 0; i < DISPLAY_WIDTH * DISPLAY_WIDTH * 4; i++) DisplayBits[i] = 0;
				RedrawWindow(hShpDlg, NULL, NULL, RDW_INVALIDATE | RDW_INTERNALPAINT | RDW_ERASE);
				break;
			}
			case IDC_OPEN:
			{
				if (OpenSHP())
				{
					UpdateListBox();
					NEWDisplayImage(0);
					SendMessage(hwndList, LB_SETCURSEL, 0, 0);
					HWND hWndStatic = ::GetDlgItem(hShpDlg, IDC_FILENAME);
					if (hWndStatic) ::SetWindowText(hWndStatic, SHPFileName);
					EnableWindow( GetDlgItem( hShpDlg, IDC_SAVE ), false );
				}
				break;
			}
			case IDC_SAVEAS:
			{
				if (SaveAsSHP())
				{
					HWND hWndStatic = ::GetDlgItem(hShpDlg, IDC_FILENAME);
					if (hWndStatic) ::SetWindowText(hWndStatic, SHPFileName);
					EnableWindow( GetDlgItem( hShpDlg, IDC_SAVE ), false );
				}
				break;
			}
			case IDC_SAVE:
			{
				if (SHPFileName[0]==0 ? SaveAsSHP() : SaveSHP())
				{
					HWND hWndStatic = ::GetDlgItem(hShpDlg, IDC_FILENAME);
					if (hWndStatic) ::SetWindowText(hWndStatic, SHPFileName);
					EnableWindow( GetDlgItem( hShpDlg, IDC_SAVE ), false );
				}
				break;
			}
			case IDC_INSERT:
				{
					int insertSlot = GetNumber();
					if (insertSlot >= 0)
					{
						if (insertSlot > MAX_ICONS) 
						{
							MessageBox(hDlg,"We don't support that many icons in one SHP file!","Index too high!", MB_YESNO);
								return true;
						}
						if (insertSlot+1 > num_images) num_images = insertSlot +1;
						if (new_icon[insertSlot] && MessageBox(hDlg,"¿Replace the existing image?","¿Really?", MB_YESNO) != IDYES) return true;
						
						OpenTGA(insertSlot);
						NEWDisplayImage(insertSlot);
						UpdateListBox();
						SendMessage(hwndList, LB_SETCURSEL, insertSlot, 0);
						EnableWindow( GetDlgItem( hShpDlg, IDC_SAVE ), true);
					}
				}
				break;
			case IDC_REPLACE:
			{
				int selected_index = SendMessage(hwndList, LB_GETCURSEL, 0, 0);
				if (selected_index == LB_ERR) return true;
				int selected_picture = SendMessage(hwndList, LB_GETITEMDATA, selected_index, 0); 
				if (selected_picture == LB_ERR) return true;
				OpenTGA(selected_picture);
				NEWDisplayImage(selected_picture);
				EnableWindow( GetDlgItem( hShpDlg, IDC_SAVE ), true);
				break;
			}
			
			case IDC_DELETE:
			{
				int selected_index = SendMessage(hwndList, LB_GETCURSEL, 0, 0);
				if (selected_index == LB_ERR) return true;

				int selected_picture = SendMessage(hwndList, LB_GETITEMDATA, selected_index, 0); 
				if (selected_picture == LB_ERR) return true;
				if (selected_index)
				if (MessageBox(hDlg,"¿Are you sure you want to delete this image?","¿Really?", MB_YESNO) != IDYES) return true;
				delete new_icon[selected_picture];
				new_icon[selected_picture] = 0;
				delete new_palette[selected_picture];
				new_palette[selected_picture] = 0;
				if (selected_picture == num_images-1) num_images--;
				SendMessage(hwndList, LB_DELETESTRING, selected_index, (LPARAM) 0);
				
				for (int i = 0; i < DISPLAY_WIDTH * DISPLAY_WIDTH * 4; i++) DisplayBits[i] = 0;
				RedrawWindow(hShpDlg, NULL, NULL, RDW_INVALIDATE | RDW_INTERNALPAINT | RDW_ERASE);
				EnableWindow( GetDlgItem( hShpDlg, IDC_SAVE ), true);
				break;
			}

		}
		return TRUE;
		break;
	case WM_CLOSE:
		if (IsWindowEnabled(GetDlgItem( hShpDlg, IDC_SAVE )))
		{
			if (MessageBox(hDlg,"¿ Exit without saving ?", "Are you sure??", MB_YESNO) != IDYES)
				return true;
		}

		EndDialog( hDlg, TRUE );
		exit(0);
		return TRUE;
		break;
	}
	return FALSE;
}


void ShowSHPDialog(int argc, char** argv )
{
	
	// create the dialog window
	hShpDlg = ::CreateDialog(NULL,MAKEINTRESOURCE(IDD_SHPDIALOG),NULL,(DLGPROC) ShpDlgProc);
	


	if(hShpDlg!=NULL)
	{
		// show dialog
		::ShowWindow(hShpDlg,SW_SHOW);
	}
	else
	{
		printf("Failed to create dialog\n");
		return;
	}
	


	if (argc > 1) 
	{
		sprintf(SHPFileName,"%s",argv[1]);
		if (OpenSHP(false))
		{
			UpdateListBox();
			NEWDisplayImage(0);
			SendMessage(hwndList, LB_SETCURSEL, 0, 0);
			HWND hWndStatic = ::GetDlgItem(hShpDlg, IDC_FILENAME);
			if (hWndStatic) ::SetWindowText(hWndStatic, SHPFileName);
		}
	}
	
	
	// message loop to process user input
	MSG msg;
	while(1)
	{
		if(::PeekMessage(&msg,hShpDlg,0,0,PM_REMOVE))
		{
			// process message
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
		else 
		{
			Sleep(100);
		}
	}
}

void main( int argc, char** argv )
{
	BITMAPINFOHEADER  bmih;
	bmih.biSize = sizeof(BITMAPINFOHEADER);
	bmih.biWidth = DISPLAY_WIDTH;
	bmih.biHeight = -DISPLAY_WIDTH;
	bmih.biPlanes = 1;
	bmih.biBitCount = 32;
	bmih.biCompression = BI_RGB;
	bmih.biSizeImage = 0;
	bmih.biXPelsPerMeter = 0;
	bmih.biYPelsPerMeter = 0;
	bmih.biClrUsed = 0;
	bmih.biClrImportant = 0;
	hBmp = CreateDIBSection(NULL, (BITMAPINFO*) &bmih, 0, (void**)&DisplayBits, NULL, 0);
	
	HRSRC dataHrsrc = FindResource(NULL, MAKEINTRESOURCE(IDR_MISSINGDATA), "BINARY");
	HRSRC palHrsrc = FindResource(NULL, MAKEINTRESOURCE(IDR_MISSINGPAL), "BINARY");
	if (!dataHrsrc || !palHrsrc) 
	{
		//cout << "FAILED TO FIND RESOURCE!" << endl;
		exit(0);
	}
	
	HGLOBAL dataHglobal = LoadResource(NULL, dataHrsrc);
	HGLOBAL palHglobal = LoadResource(NULL, palHrsrc);

	
	missingLength = SizeofResource(NULL,dataHrsrc);

	if (!dataHglobal || !palHglobal || !missingLength) 
	{
		//cout << "FAILED TO LOAD RESOURCE!" << endl;
		exit(0);
	}
	

	missingData = (U8*) LockResource(dataHglobal);
	missingPal = (U8*) LockResource(palHglobal);
	
	if (!missingData || !missingPal) 
	{
		//cout << "FAILED TO LOCK RESOURCE!" << endl;
		exit(0);
	}

	ClearSHP();



	ShowSHPDialog(argc, argv );

}
