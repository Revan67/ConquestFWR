#include "msgbuffer.h"
#include <assert.h>
#include <string.h>

// **************************************************** *
bool BufferObject::resize( int newsize )
{ 
	if (newsize > maxsize)
	{
		if (newsize < (maxsize + BUF_GROWTH_SIZE))
			newsize= maxsize + BUF_GROWTH_SIZE;

		U8 * newbuf = new U8[newsize];

		if (newbuf)
		{
			if (used) memcpy( newbuf, buffer, used );
			maxsize= newsize;
			if (buffer) delete [] buffer;
			buffer= newbuf;
		}
	}

	return (maxsize >= newsize);
}

// **************************************************** *
// **************************************************** *
U8 * BufferObjectIO::AdvanceWrite( int amt )
{
	int lastpos= writepos;

	writepos+= amt;

	if (buffer->size() < writepos)
	{
		if (!allowresize || !buffer->resize(writepos))
		{
			// out of buffer space
			assert(allowoverflow);
			writepos= lastpos;
			return NULL;
		}
	}

	if (writepos >= buffer->amtused())
	{
		buffer->inc( writepos - buffer->amtused() );
	}

	return buffer->Buffer(lastpos);
}

// **************************************************** *
U8 * BufferObjectIO::AdvanceRead( int amt )
{
	int lastpos= readpos;

	// check attempted overread
	if ((readpos+= amt) > buffer->amtused())
	{
		assert(allowoverread);

		readpos= lastpos;
		return NULL;
	}

	return buffer->Buffer(lastpos);
}

// **************************************************** *
bool BufferObjectIO::SeekWrite( int pos )
{
	if ((pos >= 0) && (pos < buffer->size()))
	{
		if (pos >= buffer->amtused())
		{
			buffer->inc( pos - buffer->amtused() );
		}
		
		writepos=pos;
		return true;
	}

	assert(allowoverflow);
	return false;
}

// **************************************************** *
bool BufferObjectIO::SeekRead( int pos )
{
	if ((pos >= 0) && (pos <= buffer->amtused()))
	{
		readpos= pos;
		return true;
	}

	assert(allowoverread);
	return false;
}

// **************************************************** *
// **************************************************** *
bool WriteBuffer::WriteU8( const U8 data )
{
	U8 * ptr;
	
	if ((ptr= (U8 *) AdvanceWrite(1)))
	{
		*ptr= data;
		return true;
	}

	return false;
}
// **************************************************** *
bool WriteBuffer::WriteU16( const U16 data ) 
{
	U16 * ptr;

	if ((ptr= (U16 *) AdvanceWrite(2)))
	{
		*ptr= data;
		return true;
	}

	return false;
}
// **************************************************** *
bool WriteBuffer::WriteU32( const U32 data )
{
	U32 * ptr;

	if ((ptr= (U32 *) AdvanceWrite(4)))
	{
		*ptr= data;
		return true;
	}

	return false;
}
// **************************************************** *
bool WriteBuffer::WriteSingle( const SINGLE data )
{
	SINGLE * ptr;

	if ((ptr= (SINGLE *) AdvanceWrite(4)))
	{
		*ptr= data;
		return true;
	}

	return false;
}
// **************************************************** *
bool WriteBuffer::WriteString( const char * string )
{
	WriteData( (const U8 *)string, strlen( string ) + 1);
	return true;
}
// **************************************************** *
bool WriteBuffer::WriteData( const U8 * data, const unsigned int num_bytes )
{
	U8 * ptr;
	
	if ((ptr= (U8 *) AdvanceWrite(num_bytes)))
	{
		memcpy( ptr, data, num_bytes );
		return true;
	}

	return false;
}

// **************************************************** *
// **************************************************** *
U8 ReadBuffer::ReadU8( void )
{
	U8 * ptr;
	
	if ((ptr= (U8 *) AdvanceRead(1)))
	{
		return *ptr;
	}
	
	return 0;
}
// **************************************************** *
U16 ReadBuffer::ReadU16( void )
{
	U16 * ptr;
	
	if ((ptr= (U16 *) AdvanceRead(2)))
	{
		return *ptr;
	}
	
	return 0;
}
// **************************************************** *
U32 ReadBuffer::ReadU32( void )
{
	U32 * ptr;
	
	if ((ptr= (U32 *) AdvanceRead(4)))
	{
		return *ptr;
	}
	
	return 0;
}
// **************************************************** *
SINGLE ReadBuffer::ReadSingle( void )
{
	SINGLE * ptr;
	
	if ((ptr= (SINGLE *) AdvanceRead(4)))
	{
		return *ptr;
	}
	
	return 0;
}
// **************************************************** *
bool ReadBuffer::ReadString( char * string )
{
	char * str= string;
	U8 * ptr= buffer->Buffer(readpos);

	int sizeleft= ReadLeft();

	do
	{
		if (sizeleft-- <= 0)
		{
			assert(allowoverread);
			return false;
		}
	}
	while ( (*(str++)= *(ptr++)) != '\0');

	// string length including null
	AdvanceRead( str - string );
	return true;
}
// **************************************************** *
bool ReadBuffer::ReadData( U8 * data, const unsigned int num_bytes )
{
	U8 * ptr;
	
	if ((ptr= (U8 *) AdvanceRead(num_bytes)))
	{
		memcpy( data, ptr, num_bytes );
		return true;
	}

	return false;
}
