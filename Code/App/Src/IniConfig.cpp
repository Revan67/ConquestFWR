//--------------------------------------------------------------------------//
//                                                                          //
//                              IniConfig.cpp                               //
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Tmauer $

   $Header: /Conquest/App/Src/IniConfig.cpp 15    27.09.01 14:34 Tmauer $

*/
//--------------------------------------------------------------------------//

 
#include "pch.h"
#include <globals.h>
#include "CQTrace.h"

#include <TSmartPointer.h>
#include <RPUL\RPUL_Misc.h>
#include <DEffectOpts.h>
#include "UserDefaults.h"

//#include <ddraw.h>
#include <d3d9.h>
//--------------------------------------------------------------------------//
//
struct FileData
{
	char * const pImage;
	int index;
	int offset;


	FileData (char * _pImage) : pImage(_pImage)
	{
		index = offset = 0;
	}

	bool findNextSection (void);
};
//--------------------------------------------------------------------------//
//
bool FileData::findNextSection (void)
{
	bool result = false;
	char * ptr;

	while ((ptr = strchr(pImage+offset, '[')) != 0)
	{
		if (strncmp(ptr+1, "Rend", 4) == 0)
		{
			if (ptr[5] == index+'0' && ptr[6]== ']')
			{	
				if (strncmp(ptr+7, "\r\nDeviceId = ", 13) == 0)
				{
					offset = (ptr-pImage) + 7 + 13;
					result = 1;
					index++;
					break;
				}
			}
		}

		offset = (ptr-pImage) + 1;
	}

	return result;
}
//--------------------------------------------------------------------------//
//
static BOOL WINAPI DDEnumCallback (GUID FAR *lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName, LPVOID lpContext)
{
	FileData * pData = (FileData *) lpContext;

	if (pData->findNextSection())
	{
		char buffer[64];
		ConvertGUIDToString(lpGUID, buffer);
		int len = strlen(buffer);
		memcpy(pData->pImage+pData->offset, buffer, len);
	}
	
	return 1;
}
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
void __stdcall InitRendSections (const char * filename)
{
	HANDLE hFile, hMapping=0;
	char *pImage=0;
	U32 fileSize;
	char eof;

	if ((hFile = CreateFile(filename, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0)) == INVALID_HANDLE_VALUE)
		goto Done;

	fileSize = ::GetFileSize(hFile, NULL);
	if ((hMapping = ::CreateFileMapping(hFile, 0, PAGE_READWRITE, 0, 0, NULL)) == 0)
		goto Done;
	if ((pImage = (char *) ::MapViewOfFile(hMapping, FILE_MAP_WRITE, 0, 0 , 0)) == 0)
		goto Done;
	eof = pImage[fileSize-1];
	pImage[fileSize-1] = 0;
	
	{
		FileData data(pImage);
		//DirectDrawEnumerate(DDEnumCallback, &data);
		// testing!!!
		while (data.findNextSection())
			;
	}

	pImage[fileSize-1] = eof;

Done:
	if (pImage)
		::UnmapViewOfFile(pImage);
	if (hMapping)
		::CloseHandle(hMapping);
	if (hFile!=INVALID_HANDLE_VALUE)
		::CloseHandle(hFile);
}

//--------------------------------------------------------------------------//
//
/*
static HRESULT CALLBACK D3DEnumDevicesCallback7 (LPSTR lpDeviceDescription, LPSTR lpDeviceName, LPD3DDEVICEDESC7 lpD3DDeviceDesc, LPVOID lpContext)
{
	D3DDEVICEDESC7 * d3desc = (D3DDEVICEDESC7 *) lpContext;

	BOOL fIsHardware = FALSE;

    fIsHardware = (lpD3DDeviceDesc->dwDevCaps & D3DDEVCAPS_HWRASTERIZATION); 
	if (fIsHardware)
	{
		*d3desc = *lpD3DDeviceDesc;

		return D3DENUMRET_CANCEL;
	}
	else
		return D3DENUMRET_OK;
}
*/
#define TEST_CAP(x,y,z)	\
	if ((y & z)==0) { \
		x = false;  FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), "D3D Test Failed on Cap %s: %s\n", #y, #z);  }

//--------------------------------------------------------------------------//
//
bool __stdcall TestDeviceFor3D (const char * pGUID)
{
/*	COMPTR<IDirectDraw7> ddraw;
	GUID guid;
	ConvertStringToGUID(const_cast<char *>(pGUID), &guid );

	if (DirectDrawCreateEx(&guid, ddraw, IID_IDirectDraw7, 0) == DD_OK)
	{
		DDCAPS caps;
		memset(&caps, 0, sizeof(caps));
		caps.dwSize = sizeof(caps);

		if (ddraw->GetCaps(&caps, NULL) == DD_OK)
		{
			char buffer[8];

			if (DEFAULTS->GetDataFromRegistry("VidTest", buffer, sizeof(buffer)) == 0)
			{
				// set default video options to all off/on based on available memory
				CQEFFECTS.bBackground =
				CQEFFECTS.bEmissiveTextures =
				CQEFFECTS.bExpensiveTerrain = 
				CQEFFECTS.bWeaponTrails = (caps.dwVidMemTotal < 0xA00000) ? CQEFFECTS.off : CQEFFECTS.on;
				DEFAULTS->GetDefaults()->bHardwareCursor = (caps.dwVidMemTotal < 0xA00000);
				if(CQEFFECTS.bBackground)
					CQEFFECTS.nFlatShipScale = 4;
				else
					CQEFFECTS.nFlatShipScale = 0;


				DEFAULTS->SetDataInRegistry("VidTest", buffer, 1);
			}

			if ((caps.dwCaps & DDCAPS_3D) != 0)
			{
				COMPTR<IDirect3D7> d3d7;

				if (ddraw->QueryInterface(IID_IDirect3D7, d3d7) == DD_OK)
				{
					D3DDEVICEDESC7 d3desc;
					bool success = true;

					memset(&d3desc, 0, sizeof(d3desc));
					d3d7->EnumDevices(D3DEnumDevicesCallback7, &d3desc);
					
					TEST_CAP(success, d3desc.dwDevCaps, D3DDEVCAPS_DRAWPRIMTLVERTEX)
					TEST_CAP(success, d3desc.dwDeviceRenderBitDepth, DDBD_16)
					TEST_CAP(success, d3desc.dwDeviceZBufferBitDepth , DDBD_16)
					TEST_CAP(success, d3desc.dwMinTextureWidth, 0xFFFFFFFF)
					TEST_CAP(success, d3desc.dwMinTextureHeight, 0xFFFFFFFF)
					TEST_CAP(success, d3desc.dwMaxTextureWidth, 0xFFFFFFFF)
					TEST_CAP(success, d3desc.dwMaxTextureHeight, 0xFFFFFFFF)
					TEST_CAP(success, d3desc.wMaxTextureBlendStages, 0xFFFFFFFF)

					if (success)
					{
						// the device meets all of our requirements
						return true;
					}
				}
			}
		}
	}

	// if we've gotten here than the device is not good enough for our game
	*/
	return true;
}

bool bSupports32Bit;
//--------------------------------------------------------------------------//
//
/*
void __stdcall EnumVideoModes (const char * pGUID, LPDDENUMMODESCALLBACK2 callback, LPVOID lpContext)
{
	
	COMPTR<IDirectDraw7> ddraw;
	GUID guid;
	ConvertStringToGUID(const_cast<char *>(pGUID), &guid );

	if (DirectDrawCreateEx(&guid, ddraw, IID_IDirectDraw7, 0) == DD_OK)
	{
		DDCAPS caps;
		memset(&caps, 0, sizeof(caps));
		caps.dwSize = sizeof(caps);

		if (ddraw->GetCaps(&caps, NULL) == DD_OK)
		{
			if ((caps.dwCaps & DDCAPS_3D) != 0)
			{
				COMPTR<IDirect3D7> d3d7;

				if (ddraw->QueryInterface(IID_IDirect3D7, d3d7) == DD_OK)
				{
					D3DDEVICEDESC7 d3desc;
					bool success = true;

					memset(&d3desc, 0, sizeof(d3desc));
					d3d7->EnumDevices(D3DEnumDevicesCallback7, &d3desc);
					
					TEST_CAP(success, d3desc.dwDevCaps, D3DDEVCAPS_DRAWPRIMTLVERTEX)
					TEST_CAP(success, d3desc.dwDeviceRenderBitDepth, DDBD_16)
					TEST_CAP(success, d3desc.dwDeviceZBufferBitDepth , DDBD_16)
					TEST_CAP(success, d3desc.dwMinTextureWidth, 0xFFFFFFFF)
					TEST_CAP(success, d3desc.dwMinTextureHeight, 0xFFFFFFFF)
					TEST_CAP(success, d3desc.dwMaxTextureWidth, 0xFFFFFFFF)
					TEST_CAP(success, d3desc.dwMaxTextureHeight, 0xFFFFFFFF)
					TEST_CAP(success, d3desc.wMaxTextureBlendStages, 0xFFFFFFFF)

					bSupports32Bit = true;
					TEST_CAP(bSupports32Bit, d3desc.dwDeviceRenderBitDepth, DDBD_32);

					if (success)
					{
						DDSURFACEDESC2 ddesc;
						memset(&ddesc, 0, sizeof(ddesc));
						ddesc.dwSize = sizeof(ddesc);
						ddesc.dwFlags = DDSD_CAPS;
						ddesc.ddsCaps.dwCaps = DDSCAPS_3DDEVICE;		// look for 3D devices

						ddraw->EnumDisplayModes(0, &ddesc, lpContext, callback);
					}
				}
			}
		}
	}
}*/

//--------------------------------------------------------------------------//
//
/*
HRESULT __stdcall GetDisplayDeviceIDFromGUID (const char * pGUID, DDDEVICEIDENTIFIER2 * pIdent2)
{
	COMPTR<IDirectDraw7> ddraw;
	GUID guid;
	HRESULT hr;
	ConvertStringToGUID(const_cast<char *>(pGUID), &guid );

	if ((hr=DirectDrawCreateEx(&guid, ddraw, IID_IDirectDraw7, 0)) == DD_OK)
	{
		hr = ddraw->GetDeviceIdentifier(pIdent2, 0);
	}

	return hr;
}
*/

//------------------------------------------------------------------------------//
//-----------------------------End IniConfig.cpp--------------------------------//
//------------------------------------------------------------------------------//

