#ifndef VOXCOMPRESS_H
#define VOXCOMPRESS_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              VoxCompress.h                               //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Gboswood $

   $Header: /Conquest/App/Src/VoxCompress.h 1     9/20/98 1:04p Gboswood $
	
   Wrapper for Voxware speech compression
*/
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
/*
//-------------------------------------------------------------------------------------
//
BOOL32 IVoxCompression::SetWaveFormat (const WAVEFORMATEX *format);
	INPUT:
		format: Address of the sound format to be used as input to the compressor and as
			output from the decompressor.  This format does not have to be directly
			compatible with the input/output format required by the converter.  The
			Compress() and Decompress() routines will attempt to translate to/from the
			format specified to the format required by the actual compression algorithm.
	RETURNS:
		TRUE if the compressor can convert sound in the format specified by 'format'.
		FALSE if the compressor cannot use the suggested format.
	OUTPUT:
		IF returns TRUE, future calls to Compress() and Decompress() will use this
			input/output sound format.


//-------------------------------------------------------------------------------------
//
U32 IVoxCompression::Compress (void *outBuffer, U32 outSize, void *inBuffer, U32 inSize);
	INPUT:
		outBuffer: Address of user's buffer where compressed data will be stored.
		outSize: Number of bytes allocated for output buffer.  If the compressed
			output data exceeds the length specified by outSize, the compressed
			data will be truncated and may result in undesirable playback artifacts.
		inBuffer: Address of input buffer where uncompressed sound is stored.
		inSize: Number of bytes of sound data in input buffer.
	RETURNS:
		Number of bytes written to output buffer. Returns 0 on error.
	OUTPUT:
		Compresses the input sound buffer, writing the resulting data into the 'outBuffer'.
		If the input format has not been set (See SetWaveFormat(), above), the method will return 0.
		It is recommended that the output buffer size be as large as the input buffer,
		since compressed output should be no larger than the input data.

//-------------------------------------------------------------------------------------
//
U32 IVoxCompression::Decompress (void *outBuffer, U32 outSize, void *inBuffer, U32 inSize);
	INPUT:
		outBuffer: Address of user's buffer where uncompressed sound data will be stored.
		outSize: Number of bytes allocated for output buffer.  If the compressed
			output data exceeds the length specified by outSize, the compressed
			data will be truncated and may result in undesirable playback artifacts.
		inBuffer: Address of input buffer where compressed data is stored.
		inSize: Number of bytes of compressed data in input buffer.
	RETURNS:
		Number of bytes written to output buffer. Returns 0 on error.
	OUTPUT:
		Decompresses the compressed data, writing the resulting sound data into the 'outBuffer'.
		If the output buffer is not large enough to hold the result, the method returns 0.
		If the output format has not been set (See SetWaveFormat(), above), the method will return 0.
		It is recommended that the output buffer be somewhat larger than the largest expected input
		to the Compress() routine, since it is possible for the decompressed stream to be slightly
		larger than the original input.  A size that exceeds the original input buffer size by
		at least 10% is recommended.
*/
//--------------------------------------------------------------------------//
//
#ifndef DACOM_H
#include <DACOM.h>
#endif

struct tWAVEFORMATEX;
typedef struct tWAVEFORMATEX WAVEFORMATEX;

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IVoxCompression : public IDAComponent
{
	DEFMETHOD_(BOOL32,SetWaveFormat) (const WAVEFORMATEX *format) = 0;

	DEFMETHOD_(U32,Compress) (void *outBuffer, U32 outSize, void *inBuffer, U32 inSize) = 0;

	DEFMETHOD_(U32,Decompress) (void *outBuffer, U32 outSize, void *inBuffer, U32 inSize) = 0;
};


//---------------------------------------------------------------------------------//
//---------------------------------End VoxCompress.h-------------------------------//
//---------------------------------------------------------------------------------//
#endif