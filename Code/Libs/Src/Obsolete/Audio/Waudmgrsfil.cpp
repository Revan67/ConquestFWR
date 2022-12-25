//############################################################################
//##                                                                        ##
//##  Miles Sound System                                                    ##
//##                                                                        ##
//##  WAILSFIL.C: Digital sound API module for digital sound file access    ##
//##                                                                        ##
//##  16-bit protected-mode source compatible with MSC 7.0                  ##
//##  32-bit protected-mode source compatible with MSC 9.0                  ##
//##                                                                        ##
//##  Version 1.00 of 15-Feb-95: Derived from WAILSFIL V1.00                ##
//##                                                                        ##
//##  Author: John Miles                                                    ##
//##                                                                        ##
//############################################################################
//##                                                                        ##
//##  Copyright (C) RAD Game Tools, Inc.                                    ##
//##                                                                        ##
//##  Contact RAD Game Tools at 801-322-4300 for technical support.         ##
//##                                                                        ##
//############################################################################

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#define STRICT

#include <dos.h>
#include <string.h>

#include <windows.h>

#include "mmsystem.h"
#include "audiofile.h"
#include "waudmgrsfil.h"

#include "mssw.h"

// ##########################################################################
// ################ Hacked extractions for DA's Audio Manager ###############
// ##########################################################################

//############################################################################
//##                                                                        ##
//## Extracted for wailsys.c																 ##
//##                                                                        ##
//############################################################################

static void FAR * AUDMGR_ptr_add(void FAR *ptr, U32 offset)
{
#ifdef IS_WIN32

   return (void FAR *) ((U32) ptr + offset);

#else

   return (void FAR *) ((U8 _huge *) ptr + offset);

#endif
}
//############################################################################
//##                                                                        ##
//## Extracted for wailsys.c                                                ##
//##                                                                        ##
//############################################################################

static S32 AUDMGR_strnicmp(void FAR *s1, void FAR *s2, U32 count)
{
   U8 FAR *st1=(U8 FAR *)s1;
   U8 FAR *st2=(U8 FAR *)s2;
   S32 c=count;

   for(;c;c--) {
     if (*st1<*st2)
       return(-1);
     if (*st1>*st2)
       return(1);
     if (*st1==0)
       break;
     ++st1;
     ++st2;
   }

   return(0);
}

// #####################################################################
// ############################ End DA Hacks ###########################
// #####################################################################

//
// Recognized file types
//

#define FTYP_VOC  0
#define FTYP_WAV  1

//
// .VOC file header
//

typedef struct
{
   S8  ID_string[20];

   U16 data_offset;
   U16 version;
   U16 ID_code;
}
VOC;

//
// .VOC terminator block
//

typedef struct
{
   U8 block_ID;
}
BLK_0;

//
// .VOC voice block
//

typedef struct
{
   U8 block_ID;
   U8 block_len[3];
   U8 time_constant;
   U8 pack_method;
}
BLK_1;

//
// .VOC continued voice block
//

typedef struct
{
   U8 block_ID;
   U8 block_len[3];
}
BLK_2;

//
// .VOC silence block
//

typedef struct
{
   U8  block_ID;
   U8  block_len[3];
   U16 pause_period;
   U8  time_constant;
}
BLK_3;

//
// .VOC marker block
//

typedef struct
{
   U8  block_ID;
   U8  block_len[3];
   S16 marker;
}
BLK_4;

//
// .VOC ASCIIZ comment block
//

typedef struct
{
   U8 block_ID;
   U8 block_len[3];
   S8 string;
}
BLK_5;

//
// .VOC repeat loop block
//

typedef struct
{
   U8  block_ID;
   U8  block_len[3];
   U16 repeat_count;
}
BLK_6;

//
// .VOC end-of-loop block
//

typedef struct
{
   U8 block_ID;
   U8 block_len[3];
}
BLK_7;

//
// .VOC extended attribute block 
//
// (always followed by block 1)
//

typedef struct
{
   U8  block_ID;
   U8  block_len[3];
   U16 time_constant;
   U8  pack_method;
   U8  voice_mode;
}
BLK_8;

// 
// .VOC extended voice block
//
// (replaces blocks 1 and 8)
//

typedef struct
{
   U8  block_ID;
   U8  block_len[3];
   U32 sample_rate;
   U8  bits_per_sample;
   U8  channels;
   U16 format;
   U8  reserved[4];
}
BLK_9;

//
// .WAV file headers
//

typedef struct
{
   S8  RIFF_string[4];
   U32 chunk_size;
   S8  ID_string[4];
   U8  data[1];
}
RIFF;

//
// .WAV PCM file format chunk
//

typedef struct
{
   S8   FMT_string[4];
   U32  chunk_size;
   
   S16  format_tag;
   S16  channels;
   S32  sample_rate;
   S32  average_data_rate;
   S16  alignment;
   S16  bits_per_sample;
}
FMT;

//
// .WAV file data chunk
//

typedef struct
{
   S8  DATA_string[4];
   U32 chunk_size;
   U8  data[1];
}
DATA;

//############################################################################
//##                                                                        ##
//## Get length of .VOC block                                               ##
//##                                                                        ##
//############################################################################

//GTB// static U32 AUDMGR_VOC_block_len(void FAR *block)

static U32 AUDMGR_VOC_block_len(void FAR *block)
{
   return (*(U32 FAR *) block) >> 8;
}

//############################################################################
//##                                                                        ##
//## Terminate playback of .VOC file                                        ##
//##                                                                        ##
//## Invoke application callback function, if any, and release the sample   ##
//## allocated to play this file                                            ##
//##                                                                        ##
//############################################################################

//GTB// static void AIL_VOC_terminate(HSAMPLE sample)

static void AUDMGR_VOC_terminate(HSAMPLE sample)
{
   if (sample->system_data[SSD_EOD_CALLBACK] != (U32) NULL)
      {
      WAIL_do_cb1( (AILSAMPLECB),
        (AILSAMPLECB)sample->system_data[SSD_EOD_CALLBACK], sample->driver->callingDS,sample->system_data[SSD_EOD_CB_WIN32S],
          sample);
      }

   if (sample->system_data[SSD_RELEASE] > 0)
      {
      AIL_release_sample_handle(sample);
      }

   sample->system_data[SSD_RELEASE] = -1;
}

//############################################################################
//##                                                                        ##
//## Process .VOC file block                                                ##
//##                                                                        ##
//## Called by .VOC initialization code and as end-of-sample callback       ##
//## function (interrupt-based)                                             ##
//##                                                                        ##
//## If play_flag clear, search for first block after desired marker (if    ##
//## any) and return without playing it                                     ##
//##                                                                        ##
//############################################################################

//GTB// static void AIL_process_VOC_block(HSAMPLE sample, S32 play_flag)

static void AUDMGR_process_VOC_block(HSAMPLE sample, S32 play_flag)
{
   S32  voice_block;
   void FAR *b;

   voice_block = 0;

   //
   // Loop until voice block is found
   //

   while (!voice_block)
      {
      b = (void FAR *) sample->system_data[VOC_BLK_PTR];

      switch (*(U8 FAR *) b)
         {
         //
         // Terminator block
         //

         case 0:

            //
            // Terminate playback, then return without trying to advance
            // to next block
            //

            AUDMGR_VOC_terminate(sample);

            return;

         //
         // Voice block
         // 

         case 1:

            //
            // Skip block if desired marker has not been found
            //

            if (!sample->system_data[VOC_MARKER_FOUND])
               {
               break;
               }

            //
            // Set up sample data and start playback
            //

            AIL_set_sample_address(sample,
                                   AUDMGR_ptr_add(b, sizeof(BLK_1)),
                                   AUDMGR_VOC_block_len(b) - 2);

            AIL_set_sample_playback_rate(sample,(U32)
                             1000000L / (256 - ((BLK_1 FAR *) b)->time_constant));
        
            AIL_set_sample_type(sample,DIG_F_MONO_8,0);

            if (play_flag)
               AIL_start_sample(sample);

            voice_block = 1;
            break;

         //
         // Marker block
         // 

         case 4:

            //
            // Ignore if entire file to be played
            //

            if (sample->system_data[VOC_MARKER] == -1)
               {
               break;
               }

            //
            // If this is the desired marker, set MARKER_FOUND flag --
            // otherwise, clear MARKER_FOUND flag to prevent playback
            // of future voice blocks
            //

            if (sample->system_data[VOC_MARKER] == (S32)
                                                   ((BLK_4 FAR *) b)->marker)
               {
               sample->system_data[VOC_MARKER_FOUND] = 1;
               }
            else
               {
               sample->system_data[VOC_MARKER_FOUND] = 0;
               }

            break;

         //
         // Repeat block
         //

         case 6:

            //
            // Log repeat count and starting address of repeat block
            //

            sample->system_data[VOC_REP_BLK] = (U32) b;
                                            
            sample->system_data[VOC_N_REPS]  = (U32)
                                             ((BLK_6 FAR *) b)->repeat_count;
            break;

         //
         // End repeat block
         //

         case 7:

            //
            // If finite repeat block active, check and decrement repeat  
            // count
            //

            if (sample->system_data[VOC_N_REPS] != 0xffff)
               {
               if (sample->system_data[VOC_N_REPS]-- == 0)
                  {
                  break;
                  }
               }

            b = (void FAR *) sample->system_data[VOC_REP_BLK];
            break;

         //
         // Extended attribute block 
         // (followed by block 1)
         // 

         case 8:

            //
            // Skip block if desired marker has not been found
            //

            if (!sample->system_data[VOC_MARKER_FOUND])
               {
               break;
               }

            //
            // Set up sample data and start playback
            //

            if (((BLK_8 FAR *) b)->voice_mode)
               {
               AIL_set_sample_type(sample,DIG_F_STEREO_8,0);

               AIL_set_sample_playback_rate(sample,(U32) 
                  128000000L / (65536L - ((BLK_8 FAR *) b)->time_constant));
               }
            else
               {
               AIL_set_sample_type(sample,DIG_F_MONO_8,0);

               AIL_set_sample_playback_rate(sample,(U32) 
                  256000000L / (65536L - ((BLK_8 FAR *) b)->time_constant));
               }

            //
            // Advance to paired voice block (type 1) in .VOC image
            // 

            b = AUDMGR_ptr_add(b, AUDMGR_VOC_block_len(b) + 4);

            //
            // Set sample address and size, and start playback
            //

            AIL_set_sample_address(sample,
                                   AUDMGR_ptr_add(b, sizeof(BLK_1)),
                                   AUDMGR_VOC_block_len(b) - 2);

            if (play_flag)
               AIL_start_sample(sample);

            voice_block = 1;
            break;

         //
         // Extended voice block
         //

         case 9:

            //
            // Skip block if desired marker has not been found
            //

            if (!sample->system_data[VOC_MARKER_FOUND])
               {
               break;
               }

            //
            // Set up sample data and start playback
            //

            AIL_set_sample_address(sample,
                                   AUDMGR_ptr_add(b, sizeof(BLK_9)),
                                   AUDMGR_VOC_block_len(b) - 12);

            AIL_set_sample_playback_rate(sample, ((BLK_9 FAR *) b)->sample_rate);
        
            if ((((BLK_9 FAR *) b)->channels == 1) &&
                (((BLK_9 FAR *) b)->format   == 0))
               {
               AIL_set_sample_type(sample,DIG_F_MONO_8,0);
               }
            else if ((((BLK_9 FAR *) b)->channels == 2) &&
                     (((BLK_9 FAR *) b)->format   == 0))
               {
               AIL_set_sample_type(sample,DIG_F_STEREO_8,0);
               }
            else if ((((BLK_9 FAR *) b)->channels == 1) &&
                     (((BLK_9 FAR *) b)->format   == 4))
               {
               AIL_set_sample_type(sample,DIG_F_MONO_16,DIG_PCM_SIGN);
               }
            else if ((((BLK_9 FAR *) b)->channels == 2) &&
                     (((BLK_9 FAR *) b)->format   == 4))
               {
               AIL_set_sample_type(sample,DIG_F_STEREO_16,DIG_PCM_SIGN);
               }

            if (play_flag)
               AIL_start_sample(sample);

            voice_block = 1;
            break;
         }

      //
      // Advance pointer to next block in .VOC image
      // 

      sample->system_data[VOC_BLK_PTR] = (U32)
                                         AUDMGR_ptr_add(b, AUDMGR_VOC_block_len(b) + 4);
      }
}

//############################################################################
//##                                                                        ##
//## End-of-sample callback handler for .VOC file playback                  ##
//##                                                                        ##
//############################################################################

static void WINAPI AUDMGR_VOC_EOS(HSAMPLE sample) // WINAPI because same DS as caller
{
   AUDMGR_process_VOC_block(sample,1);
}

//############################################################################
//##                                                                        ##
//## Create sample instance by parsing .WAV file                            ##
//##                                                                        ##
//############################################################################

//GTB// static void AIL_process_WAV_image(void FAR *file_image, HSAMPLE sample)

static void AUDMGR_process_WAV_image(AudioFile* file,
												 void*   file_image,
												 HSAMPLE sample,
                                     S16*    channels,
                                     S32*    sample_rate,
                                     S16*    bits_per_sample,
										       S32*    sample_size,
												 S32*    sample_offset)
{
   FMT  FAR *f;
   DATA FAR *d;

   //
   // Find mandatory <fmt-ck>
   //

   f = (FMT FAR *) (((RIFF FAR *) file_image)->data);

   while (AUDMGR_strnicmp(f->FMT_string,"fmt ",4))
      {
      f = (FMT*)AUDMGR_ptr_add(f, f->chunk_size + 8 + (f->chunk_size & 1));
      }

   //
   // Configure sample type and rate based on FMT chunk
   //

   if ((f->channels        == 1) &&
       (f->bits_per_sample == 8))
      {
      AIL_set_sample_type(sample,DIG_F_MONO_8,0);
      }
   else if ((f->channels        == 2) &&
            (f->bits_per_sample == 8))
      {
      AIL_set_sample_type(sample,DIG_F_STEREO_8,0);
      }
   else if ((f->channels        == 1) &&
            (f->bits_per_sample == 16))
      {
      AIL_set_sample_type(sample,DIG_F_MONO_16,DIG_PCM_SIGN);
      }
   else if ((f->channels        == 2) &&
            (f->bits_per_sample == 16))
      {
      AIL_set_sample_type(sample,DIG_F_STEREO_16,DIG_PCM_SIGN);
      }
   
   AIL_set_sample_playback_rate(sample,f->sample_rate);

   //
   // Find mandatory <data-ck>
   //

   d = (DATA FAR *) (((RIFF FAR *) file_image)->data);

   while (AUDMGR_strnicmp(d->DATA_string,"data",4))
      {
      d = (DATA*)AUDMGR_ptr_add(d, d->chunk_size + 8 + (d->chunk_size & 1));
      }

   //
   // Configure sample address and length based on DATA chunk
   //

	if (f->bits_per_sample == 16)
	{
		d->chunk_size &= 0xfffffffe; // Make sure its an even #
	}

	//
	// Return DA AudioManager data
	//
	if (channels)			*channels        = f->channels;
	if (bits_per_sample)	*bits_per_sample = f->bits_per_sample;
	if (sample_rate)		*sample_rate     = f->sample_rate;
	if (sample_size)		*sample_size     = d->chunk_size;
	if (sample_offset)	*sample_offset	  = (char*)d->data - (char*)file_image;
}

//############################################################################
//##                                                                        ##
//## End-of-sample callback handler for .WAV file playback                  ##
//##                                                                        ##
//############################################################################

//GTB// void WINAPI AIL_WAV_EOS(HSAMPLE sample) // WINAPI because same DS as caller

static void WINAPI AUDMGR_WAV_EOS(HSAMPLE sample) // WINAPI because same DS as caller
{
   if (sample->system_data[SSD_EOD_CALLBACK] != (U32) NULL)
      {
      WAIL_do_cb1( (AILSAMPLECB),
        (AILSAMPLECB)sample->system_data[SSD_EOD_CALLBACK], sample->driver->callingDS,sample->system_data[SSD_EOD_CB_WIN32S],
          sample);
      }

   if (sample->system_data[SSD_RELEASE] > 0)
      {
      AIL_release_sample_handle(sample);
      }

   sample->system_data[SSD_RELEASE] = -1;
}

//############################################################################
//##                                                                        ##
//## Set parameters of existing HSAMPLE according to file data              ##
//##                                                                        ##
//## Returns 0 on error, else 1                                             ##
//##                                                                        ##
//############################################################################

//GTB// S32 AIL_API_set_sample_file(HSAMPLE S, void FAR *file_image, S32 block)

S32 AUDMGR_set_sample_file(HSAMPLE S,
								   AudioFile* file,
									S32    block,
									S16*   channels,
									S32*   sample_rate,
									S16*   bits_per_sample,
									S32*   sample_size,
									S32*   sample_offset)
{ 
   S32 type;

   if ((S==NULL) || (file==NULL))
     return(0);

    S32 max_header_size = 1024;

    void* file_image = new char[max_header_size];
    
    if (!file->fetch(file_image, 0, max_header_size))
    {
      AIL_set_error("Unable to fetch audio file data\n");
      return 0;
    }

   //
   // Identify file type
   //
   // Note: Currently only single-sample PCM .WAV files are supported, since
   // no known applications generate other formats for testing
   //

   if (!AUDMGR_strnicmp(((VOC FAR *) file_image)->ID_string,"Creative",8))
      {
      type = FTYP_VOC;
      }
   else if (!AUDMGR_strnicmp(((RIFF FAR *) file_image)->ID_string,"WAVE",4))
      {
      type = FTYP_WAV;
      }
   else
      {
      AIL_set_error("Unrecognized digital audio file type\n");
      return 0;
      }

   //
   // Copy file attributes to sample
   //

   switch (type)
      {
      case FTYP_VOC:

         S->system_data[VOC_BLK_PTR]      = (U32) AUDMGR_ptr_add(file_image,
                                                 ((VOC FAR *) file_image)->
                                                 data_offset);

         S->system_data[VOC_MARKER]       = block;
         S->system_data[VOC_MARKER_FOUND] = (block == -1);

         S->system_data[SSD_RELEASE]      = 0;

         AUDMGR_process_VOC_block(S,0);
         break;

      case FTYP_WAV:

         S->system_data[SSD_RELEASE] = 0;

         AUDMGR_process_WAV_image(file, file_image,S,
                                  channels,
                                  sample_rate,
                                  bits_per_sample,
										    sample_size,
											 sample_offset);
         break;
      }

	delete[] file_image;

   //
   // Return NULL if parser rejected sample file image, or 1 if OK
   //

   if (S->system_data[SSD_RELEASE] == -1)
      {
      AIL_set_error("Invalid or missing data block\n");
      return 0;
      }

   return 1;
}

