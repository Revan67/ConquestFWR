#include <assert.h>
#include "sockets.h"
#include <string.h>

struct channel_data 
{
	bool server_connected, client_connected;
	BufferObjectIO c_buffer, s_buffer;

	channel_data(void) : c_buffer( AVG_BUFFER_SIZE ), s_buffer( AVG_BUFFER_SIZE ),
					server_connected( false ),
					client_connected( false )
	{
		s_buffer.allowoverread= 
		c_buffer.allowoverread= 
		s_buffer.allowoverflow=
		c_buffer.allowoverflow= 
		s_buffer.allowresize=
		c_buffer.allowresize= true;
	}

};

#define NUM_CHANNELS 2

static channel_data channels[NUM_CHANNELS];

bool LocalNode_ClientConnected( int channel )
{
	assert( channel < NUM_CHANNELS );
	
	return channels[channel].client_connected;
}

bool LocalNode_ServerConnected( int channel )
{
	assert( channel < NUM_CHANNELS );

	return channels[channel].server_connected;
}

LocalNode::LocalNode( int channel_num, channel_side endpoint ) :
	nextmsgsize(0),
	channel( channel_num )
{
	assert( channel < NUM_CHANNELS );
	
	client= (endpoint == CLIENT_SIDE);

	if (client)
	{
		assert( channels[channel].client_connected == false );
		channels[channel].client_connected = true;

		// forced descendent cast ok
		// no data in read/write buffer
		in  = (ReadBuffer*) &channels[channel].c_buffer;
		out = (WriteBuffer*) &channels[channel].s_buffer;
	}
	else
	{
		assert( channels[channel].server_connected == false);
		channels[channel].server_connected = true;
		
		// forced descendent cast ok
		// no data in read/write buffer
		in  = (ReadBuffer*) &channels[channel].s_buffer;
		out = (WriteBuffer*) &channels[channel].c_buffer;
	}
}

LocalNode::~LocalNode( void )
{
	Close();
}

bool LocalNode::Open( void ) 
{ 
	return true; 
}

void LocalNode::Close( void )
{ 
	if (client) 
		channels[channel].client_connected= false; 
	else 
		channels[channel].server_connected= false;
}

int LocalNode::DataWaiting( void )
{
	return (in->WritePos() > in->ReadPos());
}

// reads in data from the internal buffer
// placing data in "into" 
int LocalNode::RecvBuffer( BufferObjectIO * into, void * from_addr )
{	
	if (!DataWaiting())
		return 0;

	// nextmsgsize is class var;
	// needs to be persistant across recvs 
	// in case not enough room to recv
	if (nextmsgsize <= 0)
		nextmsgsize= (int) in->ReadU16();

	if (!into->Resize( into->BufferLength() + nextmsgsize ))
		return 0;

	// append to end of into
	// read from current read pos
	memcpy( into->Buffer(into->BufferLength()), 
			in->Buffer(in->ReadPos()), 
			nextmsgsize );
		
	// seek to the end of the buffer
    // thereby expanding it as necessary
	// for the above appended data
    int oldpos= into->WritePos();
    into->SeekWrite( into->BufferLength() + nextmsgsize );
    into->SeekWrite( oldpos );
	
	// advance the io object's readptr
	// so that send can sense reads
	// and compact the buffer as appropriate
	// and so subsequent recvs get the correct data
	
	in->SeekRead( in->ReadPos() + nextmsgsize );

	int recvd=nextmsgsize;
	nextmsgsize= 0;
	
	return recvd;
}

int LocalNode::SendBuffer( const BufferObjectIO & outfrom, const void * to_addr )
{
	// pull out any unneeded data
	// compact the buffer object
	// compression done in send b/c 
	// apps often have multiple recvs
	// but only single sends
	// do this before early outs to try to provide 
	// maximum room possible for recv
	if (out->ReadPos() > 0)
	{
		memmove( out->Buffer(), out->Buffer(out->ReadPos()), out->ReadPos() );

		// pull the write pos back the compacted amount
		out->SeekWrite( out->WritePos() - out->ReadPos() );
		out->SeekRead(0);
	}
	
	if (outfrom.BufferLength() <= 0)
		return 0;

	// get enough room in internal buffer to send this message
	if (!out->Resize( out->WritePos() + outfrom.BufferLength() + int(sizeof(U16)) ))
	{
		// debatable on whether or not this should
		// be an error as recv buffer can pull off 
		// data to provide room.
		return -1;
	}

	out->WriteU16( outfrom.BufferLength() );
	out->WriteData( outfrom.Buffer(), outfrom.BufferLength() );

	return outfrom.BufferLength();
}
