#ifndef _PERSISTCHANNEL_H_
#define _PERSISTCHANNEL_H_

typedef unsigned int ChannelUINT32;

typedef float PersistTime;		//32 bit quantity
typedef ChannelUINT32 PersistOffset;

const ChannelUINT32 PersistDT_FLOAT = 1;
const ChannelUINT32 PersistDT_VECTOR = 2;
const ChannelUINT32 PersistDT_QUATERNION = 4;
const ChannelUINT32 PersistDT_EVENT = 8;

struct PersistChannelHeader
{
	ChannelUINT32 frames;

		//will be < 0.0 if data is not periodic
	float capture_rate;	//32 bit quantity

		//describes the type of data.  may be an event stream or dof data.
	ChannelUINT32 type;
};

/*
	If the capture rate is less than 0.0 then the data is not periodic.
	In that case, each frame consists of a time value of type PersistTime and
	the position or event data for the frame.
*/


#endif
