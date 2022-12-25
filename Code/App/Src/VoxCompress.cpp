//--------------------------------------------------------------------------//
//                                                                          //
//                              VoxCompress.cpp                              //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Jasony $

   $Header: /Conquest/App/Src/VoxCompress.cpp 3     3/22/99 9:40a Jasony $
	
   Wrapper for Voxware speech compression
*/

#include "pch.h"
#include "globals.h"
#include "startup.h"
#include "CQTrace.h"

#include <IProfileParser.h>
#include <TSmartPointer.h>

#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>

#include "VoxCompress.h"

// this comes from INI file now
// #define VOXWARE_KEY "35243410-F7340C0668-CD78867B74DAD857-AC71429AD8CAFCB5-E4E1A99E7FFD-371"

#include <TComponent.h>

#ifndef WAVE_FORMAT_MSRT24
#define WAVE_FORMAT_MSRT24 0x0082
#endif

#pragma pack(1)	// Byte pack this structure

#ifndef VOXACM_WAVEFORMATEX
typedef struct tagVOXACM_WAVEFORMATEX 
{
	WAVEFORMATEX	wfx;
	DWORD				dwCodecId;
	DWORD				dwMode;
	char				szKey[72];
} VOXACM_WAVEFORMATEX, *PVOXACM_WAVEFORMATEX, FAR *LPVOXACM_WAVEFORMATEX;
#endif

#pragma pack()		// Restore normal packing

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct DACOM_NO_VTABLE VoxCompression : public IVoxCompression
{
	BEGIN_DACOM_MAP_INBOUND(VoxCompression)
	DACOM_INTERFACE_ENTRY(IVoxCompression)
	END_DACOM_MAP()
	//
	// IVoxCompression functions
	//

	DEFMETHOD_(BOOL32,SetWaveFormat) (const WAVEFORMATEX *format);

	DEFMETHOD_(U32,Compress) (void *outBuffer, U32 outSize, void *inBuffer, U32 inSize);

	DEFMETHOD_(U32,Decompress) (void *outBuffer, U32 outSize, void *inBuffer, U32 inSize);

	//
	//	VoxCompression functions & data
	//

	VoxCompression()
		: inputHandle(NULL),
		  compHandle(NULL),
		  decompHandle(NULL),
		  outputHandle(NULL),
		  inputHeader(NULL),
		  compHeader(NULL),
		  decompHeader(NULL),
		  outputHeader(NULL) {}

	~VoxCompression();

private:

	//
	// VoxCompression implementation-specific member functions
	//

	BOOL32 CheckBufferSize(HACMSTREAM handle, LPACMSTREAMHEADER* header, U32 size);

	// VoxCompression implementation-specific data members
	//

	HACMSTREAM	inputHandle;
	HACMSTREAM	compHandle;
	HACMSTREAM	decompHandle;
	HACMSTREAM	outputHandle;

	LPACMSTREAMHEADER inputHeader;
	LPACMSTREAMHEADER compHeader;
	LPACMSTREAMHEADER decompHeader;
	LPACMSTREAMHEADER outputHeader;
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//-- VoxCompression functions that overload IVoxCompression pure virtuals
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

VoxCompression::~VoxCompression()
{
	if (inputHandle && inputHeader)
	{
		acmStreamUnprepareHeader(inputHandle, inputHeader, 0);
		delete inputHeader->pbSrc;
		delete inputHeader->pbDst;
		delete inputHeader;
	}
	if (compHandle && compHeader)
	{
		acmStreamUnprepareHeader(compHandle, compHeader, 0);
		delete compHeader->pbSrc;
		delete compHeader->pbDst;
		delete compHeader;
	}
	if (decompHandle && decompHeader)
	{
		acmStreamUnprepareHeader(decompHandle, decompHeader, 0);
		delete decompHeader->pbSrc;
		delete decompHeader->pbDst;
		delete decompHeader;
	}
	if (outputHandle && outputHeader)
	{
		acmStreamUnprepareHeader(outputHandle, outputHeader, 0);
		delete outputHeader->pbSrc;
		delete outputHeader->pbDst;
		delete outputHeader;
	}

	if (inputHandle)
	{
		acmStreamClose(inputHandle, 0);
	}
	if (compHandle)
	{
		acmStreamClose(compHandle, 0);
	}
	if (decompHandle)
	{
		acmStreamClose(decompHandle, 0);
	}
	if (outputHandle)
	{
		acmStreamClose(outputHandle, 0);
	}
}

BOOL32 VoxCompression::SetWaveFormat(const WAVEFORMATEX* externalFormat)
{
	COMPTR<IProfileParser> parser;
	HANDLE hSection;
	char keyBuffer[128];

	if (DACOM->QueryInterface("IProfileParser", parser) != GR_OK)
		return 0;
	if ((hSection = parser->CreateSection("Sound")) == 0)
		return 0;
	if (parser->ReadKeyValue(hSection, "Voxware", keyBuffer, sizeof(keyBuffer)) == 0)
	{
		parser->CloseSection(hSection);
		return 0;
	}
	parser->CloseSection(hSection);


	//
	// Wave format definition for 8000hz 16 bit mono.
	// RT24 requires this input format.
	//

	WAVEFORMATEX intermediateFormat;

	intermediateFormat.wFormatTag = WAVE_FORMAT_PCM;
   intermediateFormat.nChannels = 1;
	intermediateFormat.nSamplesPerSec = 8000; 
   intermediateFormat.nBlockAlign = 2;
	intermediateFormat.nAvgBytesPerSec = 16000;
	intermediateFormat.wBitsPerSample = 16;
	intermediateFormat.cbSize = 0;

	//
	// Allocate a buffer large enough to hold max size format data
	//

	DWORD maxSize;
	acmMetrics(NULL, ACM_METRIC_MAX_SIZE_FORMAT, &maxSize);

	//
	// Lookup up the wave format settings for the RT24 format
	//

	MMRESULT mmr;

	LPWAVEFORMATEX compFormat = (LPWAVEFORMATEX)new char[maxSize];

	memset(compFormat, 0, maxSize);
	compFormat->wFormatTag = WAVE_FORMAT_MSRT24;

	mmr = acmFormatSuggest(NULL, &intermediateFormat, compFormat, maxSize,
		                    ACM_FORMATSUGGESTF_WFORMATTAG);
	if (mmr) 
	{
		CQERROR1("ACM error #%08X", mmr);
		goto ErrorExit;
	}

	//
	// Set the license key part of the RT24 wave format structure
	//
	
	if (maxSize >= sizeof(VOXACM_WAVEFORMATEX))
	{
		VOXACM_WAVEFORMATEX* voxFormat = (VOXACM_WAVEFORMATEX*)compFormat;
		char *ptr;
		if ((ptr = strchr(keyBuffer, ';')) != 0)
			*ptr = 0;
		if ((ptr = strchr(keyBuffer, ' ')) != 0)
			*ptr = 0;
		strncpy(voxFormat->szKey, keyBuffer, sizeof(voxFormat->szKey)-1);
	}

	//
	// Open the ACM stream needed to convert from the input format
	// to the 8000hz PCM format
	//

	mmr = acmStreamOpen(&inputHandle, NULL,
		                 (WAVEFORMATEX*)externalFormat, &intermediateFormat,
		                 NULL, 0, 0, ACM_STREAMOPENF_NONREALTIME);

	if (mmr) 
	{
		CQERROR1("ACM error #%08X", mmr);
		goto ErrorExit;
	}

	//
	// Open the ACM stream needed to convert from the 8000hz PCM format
	// to the compressed format
	//

	mmr = acmStreamOpen(&compHandle, NULL,
							  &intermediateFormat, compFormat,
		                 NULL, 0, 0, ACM_STREAMOPENF_NONREALTIME);

	if (mmr) 
	{
		CQERROR1("ACM error #%08X", mmr);
		goto ErrorExit;
	}

	//
	// Open the ACM stream needed to convert from the compressed format
	// to the 8000hz PCM format
	//

	mmr = acmStreamOpen(&decompHandle, NULL,
		                 compFormat, &intermediateFormat,
		                 NULL, 0, 0, ACM_STREAMOPENF_NONREALTIME);

	if (mmr) 
	{
		CQERROR1("ACM error #%08X", mmr);
		goto ErrorExit;
	}

	//
	// Open the ACM stream needed to convert from the 8000hz PCM format
	// to the external format
	//

	mmr = acmStreamOpen(&outputHandle, NULL,
							  &intermediateFormat, (WAVEFORMATEX*)externalFormat,
		                 NULL, 0, 0, ACM_STREAMOPENF_NONREALTIME);

	if (mmr) 
	{
		CQERROR1("ACM error #%08X", mmr);
		goto ErrorExit;
	}

	delete compFormat;
	compFormat = NULL;

	return TRUE;

ErrorExit:
	if (compFormat)
	{
		delete compFormat;
		compFormat = NULL;
	}
	if (inputHandle)
	{
		acmStreamClose(inputHandle, 0);
		inputHandle = NULL;
	}
	if (compHandle)
	{
		acmStreamClose(compHandle, 0);
		compHandle = NULL;
	}
	if (decompHandle)
	{
		acmStreamClose(decompHandle, 0);
		decompHandle = NULL;
	}
	if (outputHandle)
	{
		acmStreamClose(outputHandle, 0);
		outputHandle = NULL;
	}
	return FALSE;
}

//--------------------------------------------------------------------------//

U32 VoxCompression::Compress(void *outBuffer, U32 outSize, void *inBuffer, U32 inSize)
{
	if (!compHandle) return 0;

	//
	// Make sure the headers exist and their buffers
	// are large enough to hold the input stream.  The output
	// streams should be no larger than the input streams.
	//

	if (!CheckBufferSize(inputHandle, &inputHeader, inSize)) return 0;
	if (!CheckBufferSize(compHandle, &compHeader, inSize)) return 0;

	memcpy(inputHeader->pbSrc, inBuffer, inSize);

	inputHeader->cbSrcLength = inSize;

	MMRESULT mmr = acmStreamConvert(inputHandle, inputHeader, ACM_STREAMCONVERTF_START);

	if (mmr) return 0;

	memcpy(compHeader->pbSrc, inputHeader->pbDst,
									  inputHeader->cbDstLengthUsed);

	compHeader->cbSrcLength = inputHeader->cbDstLengthUsed;

	mmr = acmStreamConvert(compHandle, compHeader, ACM_STREAMCONVERTF_START);

	if (mmr) return 0;

	if (compHeader->cbDstLengthUsed > outSize)
	{
		compHeader->cbDstLengthUsed = outSize;
	}


	memcpy(outBuffer, compHeader->pbDst, compHeader->cbDstLengthUsed);

	return compHeader->cbDstLengthUsed;
}

//--------------------------------------------------------------------------//

U32 VoxCompression::Decompress(void *outBuffer, U32 outSize, void *inBuffer, U32 inSize)
{
	if (!decompHandle) return 0;

	//
	// Make sure the headers exist and their buffers
	// are large enough to hold the output stream.  The input
	// streams should be no larger than the output streams.
	//

	if (!CheckBufferSize(decompHandle, &decompHeader, outSize)) return 0;
	if (!CheckBufferSize(outputHandle, &outputHeader, outSize)) return 0;

	memcpy(decompHeader->pbSrc, inBuffer, inSize);

	decompHeader->cbSrcLength = inSize;

	MMRESULT mmr = acmStreamConvert(decompHandle, decompHeader, ACM_STREAMCONVERTF_START);

	if (mmr) return 0;

	memcpy(outputHeader->pbSrc, decompHeader->pbDst,
									    decompHeader->cbDstLengthUsed);

	outputHeader->cbSrcLength = decompHeader->cbDstLengthUsed;

	mmr = acmStreamConvert(outputHandle, outputHeader, ACM_STREAMCONVERTF_START);

	if (mmr) return 0;

	if (outputHeader->cbDstLengthUsed > outSize)
	{
		outputHeader->cbDstLengthUsed = outSize;
	}

	memcpy(outBuffer, outputHeader->pbDst, outputHeader->cbDstLengthUsed);

	return outputHeader->cbDstLengthUsed;
}

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//-- Private class functions
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

BOOL32 VoxCompression::CheckBufferSize(HACMSTREAM handle, LPACMSTREAMHEADER* header, U32 size)
{
	if (!(*header) || size > (*header)->cbSrcLength)
	{
		if (*header)
		{
			acmStreamUnprepareHeader(handle, *header, 0);

			delete (*header)->pbSrc;
			delete (*header)->pbDst;
		}
		else
		{
			*header = new ACMSTREAMHEADER;
		}

		LPACMSTREAMHEADER h = *header;

		memset(h, 0, sizeof *h);

		h->cbStruct = sizeof *h;
		h->pbSrc = new UCHAR[size];
		h->cbSrcLength = size;
		h->cbSrcLengthUsed = 0;
		h->pbDst = new UCHAR[size];
		h->cbDstLength = size;
		h->cbDstLengthUsed = 0;

		if (acmStreamPrepareHeader(handle, h, 0))
		{
			return FALSE;
		}
	}
	return TRUE;
}

//--------------------------------------------------------------------------//

struct _voxCompression : GlobalComponent
{
	virtual void Startup (void)
	{
		VOXCOMP = new DAComponent<VoxCompression>;
		AddToGlobalCleanupList((IDAComponent **) &VOXCOMP);
	}

	virtual void Initialize (void)
	{
	}
};

static _voxCompression voxCompression;

