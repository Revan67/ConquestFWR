#ifndef __TIMETAGDATA_H
#define __TIMETAGDATA_H

#include "ChannelEventTypes.h"

namespace TimeTagData
{
typedef float Time;
typedef unsigned int Offset;
typedef unsigned int TypeID;
typedef int Tag;

inline void* const create_time_tag_data (float time[], int tag[], unsigned int count)
{
	if (count == 0)
	{
		return NULL;
	}
	else
	{
		const unsigned int	frame_size = 		sizeof (Time)
											+	sizeof (Offset);
		const unsigned int	tag_size = 			sizeof (TypeID)
											+	sizeof (Tag);
									
		unsigned int data_size = count * (frame_size + tag_size);

		char* const data_start = (char*)malloc (data_size);
		
		if (data_start)
		{	
			char* const tag_start = data_start + (count * frame_size);

			char* frame_curr = data_start;
			char* tag_curr = tag_start;

			for (unsigned int idx = 0; idx < count; ++idx)
			{
				// Write the time and offset
				*((Time*)frame_curr) = time[idx];
				*((Offset*)(frame_curr + sizeof (Time))) = tag_curr - tag_start;

				// Write the type id and tag
				*((TypeID*)tag_curr) = EVENT_TAG;
				*((Tag*)(tag_curr + sizeof (TypeID))) = tag[idx];

				frame_curr += frame_size;
				tag_curr += tag_size;
			}
		}

		return data_start;
	}
}

}

#endif
