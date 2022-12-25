//
// WavLib.cpp - Sound Forge .WAV file reading and writing library.
//

//
// The Sound Forge .WAV Format (.WAV+)
//      The Sound Forge tool creates regular RIFF .WAV files, but adds some extra chunks to specify cue points
// and regions.
//      The first chunk is direct part of the WAVE form. The id is "cue ", and its data consists of a DWORD count of
// cues, then count instances of a cue structure.
//  struct Cue
//  {
//      DWORD index;   // which cue is this, starting with 1
//      DWORD offset;  // offset of cue point, in samples!
//      char  key[4] = 'data'; // what the hell is this for?
//      DWORD pad[2] = {0,0};  // sound forge specific data goes here?
//      DWORD offset2; // another copy of the offset in samples
//  };
//
//  struct CueChunkData
//  {
//      DWORD count;   // number of subsequent Cue instances
//      Cue   data[count];
//  };
//
//      Sound Forge also adds a LIST form which contains 'ltxt' and 'labl' chunks, one for each region.
// The 'ltxt' chunk contains the index of the cue which starts the region and the length of the region. The 'labl'
// chunk contains the name of the region.  We do not read the 'labl' chunk, but we do write it.
//  struct LtxtChunkData
//  {
//      DWORD cueIndex;  // the index of the cue which starts this region, starting at 1
//      DWORD length;    // the length of the region, in samples
//      char  key[4] = 'rgn '; // what the hell is this for?
//      DWORD pad[2] = {0,0};  // sound forge specific data goes here?
//  };
//  struct LablChunkData
//  {
//      DWORD regionIndex; // the index of this region, starting at 1
//      char  [];          // a null terminated string makes up the rest of the chunk
//  };
//

//
// Include files
//

#include <windows.h>
#include <tsmartpointer.h>
#include "wavlib.h"
#include "FDump.h"
#include "tempstr.h"
//
// Macros
//

#define CHUNK_NAME(a,b,c,d) \
	  (unsigned long) (a)       + \
	(((unsigned long) (b))<<8)  + \
	(((unsigned long) (c))<<16) + \
	(((unsigned long) (d))<<24)

#define DEFAULT_REGION_LABEL "LoopRegion"

//
// Constants
//

const unsigned long RIFF_ID = CHUNK_NAME('R','I','F','F');
const unsigned long LIST_ID = CHUNK_NAME('L','I','S','T');
const unsigned long WAVE_ID = CHUNK_NAME('W','A','V','E');
const unsigned long fmt_ID  = CHUNK_NAME('f','m','t',' ');
const unsigned long ltxt_ID = CHUNK_NAME('l','t','x','t');
const unsigned long labl_ID = CHUNK_NAME('l','a','b','l');
const unsigned long data_ID = CHUNK_NAME('d','a','t','a');
const unsigned long cue_ID  = CHUNK_NAME('c','u','e',' ');
const unsigned long adtl_ID = CHUNK_NAME('a','d','t','l');
const unsigned long rgn_ID  = CHUNK_NAME('r','g','n',' ');

//
// Simple Type Definitions
//

//
// Inline routines
//

inline unsigned long str2id (char *str)
{
	return
	  (unsigned long) str[0]       + 
	(((unsigned long) str[1])<<8)  +
	(((unsigned long) str[2])<<16) +
	(((unsigned long) str[3])<<24);
}

//
// Class and structure definitions
//

// These represent the chunks as they exist in the file
#pragma pack(1)
struct RiffChunk
{
	union
	{
		char          idArray[4];
		unsigned long id;
	};
	unsigned long size;
};

struct RiffForm : public RiffChunk
{
	union
	{
		char          formTypeArray[4];
		unsigned long formType;
	};
};

struct CueData
{
	DWORD index;
	DWORD offset;
	union
	{
		char          key[4];  // set to 'data'
		unsigned long keyLong;
	};
	DWORD pad[2];  // set both to 0
	DWORD offset2; // set same as offset
};

struct LtxtData
{
	DWORD cueIndex;
	DWORD length;
	union
	{
		char          key[4];  // set to 'rgn '
		unsigned long keyLong;
	};
	DWORD pad[2];  // set both to 0
};
#pragma pack()

//
// Global functions
//

// Attempts to load the given filename in the given file system as a .WAV or .WAV+ file.
bool LoadWAV (IFileSystem *fs, SoundFile &data)
{
	// *** Should this be converted to just loading the entire file into memory instead of seeking so many times?

	// NOTE: I should probably be using a RIFF file reader, but the format is standard, so no harm is being done
	// here other than it taking more time to initially write this. -TNB
	// *** Man this code is ugly. -TNB

	COMPTR<IFileSystem> pFile = fs;
	bool haveCue;
	DWORD startPos;

	// Verify that the file is a RIFF/WAVE file.

	RiffForm form;
	DWORD count;
	if (!pFile->ReadFile (0, &form, sizeof(form), &count))
	{
		return false;
	}

	if (count != sizeof(form) || form.id != RIFF_ID || form.formType != WAVE_ID)
	{
		return false;
	}

	// This is a RIFF WAVE file, so read in the 'fmt ' chunk.
	// WARNING: This assumes that the 'fmt ' chunk occurs before the 'data' chunk occurs before the 'cue ' chunk
	// occurs before the 'LIST' chunk.

	startPos = pFile->GetFilePosition (0);
	
	RiffChunk chunk;
	while (true)
	{
		// Read in this chunk, aborting on an error
		if (!pFile->ReadFile (0, &chunk, sizeof(chunk), &count))
		{
			return false;
		}

		if (count != sizeof(chunk))
		{
			return false;
		}

		// If this is the 'fmt ' chunk, break out, otherwise skip to the next chunk.

		if (chunk.id == fmt_ID)
		{
			break;
		}
		else
		{
			pFile->SetFilePointer (0, chunk.size, 0, FILE_CURRENT);
		}
	}

	// The current file posiiton now points to the data for the fmt chunk. Read in the fmt chunk, which
	// contains a WAVEFORMATEX structure.

	WAVEFORMATEX fmtData;
	if (!pFile->ReadFile (0, &fmtData, min(sizeof(WAVEFORMATEX), chunk.size), &count))
	{
		return false;
	}

	// If this is not a PCM format WAVE, abort.
	
	if (fmtData.wFormatTag != WAVE_FORMAT_PCM)
	{
		return false;
	}

	// Fill out the SoundFile's format structure, clearing out the rest of the structure at the same time.

	data.format.num_channels = fmtData.nChannels;
	data.format.bytes_per_channel = fmtData.wBitsPerSample/8;
	data.format.samples_per_sec = (unsigned short) fmtData.nSamplesPerSec;
	data.format.bytes_per_sample = data.format.num_channels * data.format.bytes_per_channel;
	data.num_samples = 0;
	data.length = 0;
	data.loop_start = 0;
	data.loop_end = 0;
	data.samples = NULL;

	// Find the 'data' chunk, which contains the actual samples.

	pFile->SetFilePointer (0, startPos, 0);

	while (true)
	{
		// Read in this chunk, aborting on an error
		if (!pFile->ReadFile (0, &chunk, sizeof(chunk), &count))
		{
			return false;
		}

		if (count != sizeof(chunk))
		{
			return false;
		}

		// If this is the 'fmt ' chunk, break out, otherwise skip to the next chunk.

		if (chunk.id == data_ID)
		{
			break;
		}
		else
		{
			pFile->SetFilePointer (0, chunk.size, 0, FILE_CURRENT);
		}
	}

	// Allocate a buffer on the heap for the sound data, read it in, then fill out the rest of the 
	// SoundFile structure.
	// NOTE: Regular .WAV files, as opposed to .WAV+ files, default to the loop points being at the start and
	// end of the file.

	data.samples = new char[chunk.size];
	if (!pFile->ReadFile (0, data.samples, chunk.size, &count))
	{
		delete data.samples;
		data.samples = NULL;
		return false;
	}

	data.length = chunk.size;
	data.num_samples = data.length / data.format.bytes_per_sample;
	data.loop_end = data.num_samples;

	// Check for the existance of a 'cue ' chunk.  If there is not one, the default loop data will be used.

	pFile->SetFilePointer (0, startPos, 0);

	haveCue = false;
	while (pFile->ReadFile (0, &chunk, sizeof(chunk), &count))
	{
		if (count != sizeof(chunk))
		{
			break;
		}

		// If this is the 'cue ' chunk, set the start of the region, indicate that we have a cue, and exit the
		// loop. Otherwise, skip to the next chunk

		if (chunk.id == cue_ID)
		{
			haveCue = true;

			// Read the count of cues, then the first cue.
			DWORD cueCount;

			if (!pFile->ReadFile (0, &cueCount, sizeof(DWORD), &count))
			{
				goto abort_marker_parse_failure;
			}

			if (cueCount < 1)
			{
				goto abort_marker_parse_failure;
			}

			CueData cData;
			if (!pFile->ReadFile (0, &cData, sizeof(cData), &count))
			{
				goto abort_marker_parse_failure;
			}

			// NOTE: Assumption here is that the first cue is index == 1.
			data.loop_start = cData.offset;
			break;
		}
		else
		{
			pFile->SetFilePointer (0, chunk.size, 0, FILE_CURRENT);
		}
	}

	// If we found a cue chunk, so find the LIST form which will contain the region data.

	if (haveCue)
	{
		pFile->SetFilePointer (0, startPos, 0);
		while (pFile->ReadFile (0, &chunk, sizeof(chunk), &count))
		{
			if (count != sizeof(chunk))
			{
				goto abort_marker_parse_failure;
			}

			// If this is the LIST form, search it for the 'ltxt' chunk whose index is 1.
			if (chunk.id == LIST_ID)
			{
				form.id = chunk.id;
				form.size = chunk.size;
				if (!pFile->ReadFile (0, &form.formType, sizeof(DWORD), &count))
				{
					goto abort_marker_parse_failure;
				}

				if (form.formType != adtl_ID)
				{
					// This is not the LIST we are looking for, so skip it.
					pFile->SetFilePointer (0, chunk.size - sizeof(DWORD), 0, FILE_CURRENT);
					continue;
				}

				bool foundIt = false;
				while (pFile->ReadFile (0, &chunk, sizeof(chunk), &count))
				{
					if (count != sizeof(chunk))
					{
						goto abort_marker_parse_failure;
					}

					if (chunk.id == ltxt_ID)
					{
						// We found it.
						LtxtData lData;

						if (!pFile->ReadFile (0, &lData, sizeof(lData), &count))
						{
							goto abort_marker_parse_failure;
						}

						data.loop_end = data.loop_start + lData.length;
						foundIt = true;
						break;
					}
					else
					{
						pFile->SetFilePointer (0, chunk.size, 0, FILE_CURRENT);
					}
				}

				if (foundIt)
				{
					break;
				}
			}
			else
			{
				pFile->SetFilePointer (0, chunk.size, 0, FILE_CURRENT);
			}
		}
	}

	// All is well, so return true.
	return true;

	// return the buffer and inform the user that the marker data was bad before exiting
	abort_marker_parse_failure:
//		old behavior: delete data.samples;
//		old behavior: data.samples = NULL;
//		old behavior: return false;
		char buffer[256];
		pFile->GetFileName(buffer, 256);
		GENERAL_ERROR(TEMPSTR("WavLib:Unable to parse marker data for file <%s>.  Loaded file without marker data.", buffer));
		// reset the loop markers to the beginning and end of the file
		data.loop_start = 0;
		data.loop_end = data.num_samples;
		return true;
}

// Attempts to save the given sound data as a .WAV+ file in the given file system.
bool SaveWAV (IFileSystem *fs, SoundFile &data)
{
	// *** TODO: Make sure that the data stored in 'data' makes sense.

	// We assume here that the given file system is a file.

	COMPTR<IFileSystem> pFile = fs;
	DWORD startPos = pFile->GetFilePosition (0);

	// Build the chunk structures, calculating their sizes.

	RiffForm riffWave;
	RiffChunk fmt;
	RiffChunk dataChunk;
	RiffChunk cue;
	RiffForm listAdtl;
	RiffChunk ltxt;
	RiffChunk labl;

	ltxt.id = ltxt_ID;
	ltxt.size = sizeof(LtxtData);
	labl.id = labl_ID;
	labl.size = sizeof(DWORD) + strlen(DEFAULT_REGION_LABEL) + 1;
	listAdtl.id = LIST_ID;
	listAdtl.formType = adtl_ID;
	listAdtl.size = sizeof(DWORD) + sizeof(RiffChunk) * 2 + ltxt.size + labl.size;

	fmt.id = fmt_ID;
	fmt.size = sizeof(WAVEFORMATEX);
	dataChunk.id = data_ID;
	dataChunk.size = data.length;
	cue.id = cue_ID;
	cue.size = sizeof(DWORD) + sizeof(CueData);
	riffWave.id = RIFF_ID;
	riffWave.formType = WAVE_ID;
	riffWave.size = sizeof(DWORD) + 4 * sizeof(RiffChunk) + fmt.size + dataChunk.size + cue.size + listAdtl.size;

	//
	// Write out the data.
	//

	DWORD count;

	// RIFF/WAVE header
	if (!pFile->WriteFile (0, &riffWave, sizeof(riffWave), &count))
	{
		return false;
	}

	// fmt chunk
	if (!pFile->WriteFile (0, &fmt, sizeof(fmt), &count))
	{
		return false;
	}

	{
		WAVEFORMATEX waveFmt;
		waveFmt.wFormatTag = WAVE_FORMAT_PCM;
		waveFmt.nChannels = data.format.num_channels;
		waveFmt.wBitsPerSample = data.format.bytes_per_channel * 8;
		waveFmt.nSamplesPerSec = data.format.samples_per_sec;
		waveFmt.nBlockAlign = data.format.num_channels * data.format.bytes_per_channel;
		waveFmt.nAvgBytesPerSec = waveFmt.nSamplesPerSec * waveFmt.nBlockAlign;
		waveFmt.cbSize = 0;

		if (!pFile->WriteFile (0, &waveFmt, sizeof(waveFmt), &count))
		{
			return false;
		}
	}

	// data chunk.

	if (!pFile->WriteFile (0, &dataChunk, sizeof(dataChunk), &count))
	{
		return false;
	}

	if (!pFile->WriteFile (0, data.samples, data.length, &count))
	{
		return false;
	}

	// cue chunk

	if (!pFile->WriteFile (0, &cue, sizeof(cue), &count))
	{
		return false;
	}

	{
		DWORD cueCount = 1;

		if (!pFile->WriteFile (0, &cueCount, sizeof(cueCount), &count))
		{
			return false;
		}

		CueData cd;
		cd.index = 1;
		cd.offset = data.loop_start;
		cd.keyLong = data_ID;
		cd.pad[0] = 0;
		cd.pad[1] = 0;
		cd.offset2 = cd.offset;

		if (!pFile->WriteFile (0, &cd, sizeof(cd), &count))
		{
			return false;
		}
	}

	// LIST/adtl header

	if (!pFile->WriteFile (0, &listAdtl, sizeof(listAdtl), &count))
	{
		return false;
	}

	// ltxt chunk

	if (!pFile->WriteFile (0, &ltxt, sizeof(ltxt), &count))
	{
		return false;
	}

	{
		LtxtData ld;

		ld.cueIndex = 1;
		ld.length = data.loop_end - data.loop_start;
		ld.keyLong = rgn_ID;
		ld.pad[0] = 0;
		ld.pad[1] = 0;


		if (!pFile->WriteFile (0, &ld, sizeof(ld), &count))
		{
			return false;
		}
	}

	// labl chunk

	if (!pFile->WriteFile (0, &labl, sizeof(labl), &count))
	{
		return false;
	}

	{
		DWORD cueIndex = 1;
		if (!pFile->WriteFile (0, &cueIndex, sizeof(cueIndex), &count))
		{
			return false;
		}

		if (!pFile->WriteFile (0, DEFAULT_REGION_LABEL, strlen(DEFAULT_REGION_LABEL)+1, &count))
		{
			return false;
		}
	}

	// All is well, so return true.
	return true;
}
