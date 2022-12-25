// **************************************************** *
// read and write buffers for use with network communications
//
// classes: BufferObject, BufferObjectIO, 
//          ReadBuffer, WriteBuffer
//
// NOTE: the message buffers are currently not type aligned
//       nor endian savy; if this goes off of intel
//       machines the underside will need some more work
//      the string functions are not UNICODE wise
//
// NOTE: ReadData / WriteData isnt capable of being endian specific
//       apps that use these are on their own
//
// Copyright (C) Digital Anvil, Inc. 1998 (tlp)
//
//
// Header: /Libs/Include/MsgBuffer.h 12	7/16/98 10:48a Jasony $
//
// **************************************************** *
#ifndef MSGBUFFER_H
#define MSGBUFFER_H

#ifndef US_TYPEDEFS
#include "Typedefs.h"
#endif

#ifndef _INC_STDLIB
#include <stdlib.h> 
#endif

#define AVG_BUFFER_SIZE (1024 * 8)
#define BUF_GROWTH_SIZE (1024)

// **************************************************** *
// BufferObject
//
// object where buffered information resides
// tracks reference count of buffer
// current amount of info held in buffer can 
// grow or shrink to instance fixed max buffer size
// **************************************************** *

class BufferObject
{
public:
	BufferObject( const int alloc_size= AVG_BUFFER_SIZE )
	{
		used=0;
		refs=1;
		maxsize= 0;
		buffer= NULL;
		resize( alloc_size );
	}

	int AddRef( void )
	{
		return ++refs;
	}

	int Release( void )
	{
		#ifdef assert
			assert(refs); 
		#endif 

		int r= --refs;
		if (r <= 0)
			delete this;
		return r;
	}

	bool resize( int newsize );
	
	int size( void ) const  { return maxsize; }
	int amtused( void ) const { return used; }
	
	// no error checking on inc
	int inc( int amt ) { return used+= amt; }
	void clear( void )  { used= 0; }

	U8 * Buffer( int i ) { return &buffer[i]; }
	
private:
	void operator= (const BufferObject &) { }

	~BufferObject( void ) { 
		#ifdef assert
			assert(!refs); 
		#endif 
	
		if (buffer) delete buffer; 
	}

	int maxsize;	// total size of buffer
	int used;
	int refs;
	U8 * buffer;	// buffer memory
};

// **************************************************** *
// BufferObjectIO
//
// provides low-level io to buffer objects
// via independent, seekable read/write pointers
// **************************************************** *

class BufferObjectIO
{
public:
	BufferObjectIO(const int alloc_size= AVG_BUFFER_SIZE) :
		writepos(0),
		readpos(0),
		allowoverflow(false),
		allowoverread(false),
		allowresize(false)
	{ 
		buffer= new BufferObject(alloc_size);
	}

	BufferObjectIO( BufferObjectIO & m ) :
		buffer(m.buffer),
		writepos(0),
		readpos(0),
		allowresize(m.allowresize),
		allowoverflow(m.allowoverflow),
		allowoverread(m.allowoverread)
	{ 
		buffer->AddRef();
	}

	virtual ~BufferObjectIO( void ) 
	{
		if (buffer) {
			buffer->Release();
			buffer= NULL;
		}
	}

	int BufferLength( void ) const { return buffer->amtused(); }
	int BufferLeft( void ) const   { return (buffer->size()-buffer->amtused()); }

	int WritePos( void ) const { return writepos; }
	int ReadPos( void ) const  { return readpos; }

	int WriteLeft( void ) const {  return (buffer->size()-writepos); }
	int ReadLeft( void ) const  {  return (buffer->amtused()-readpos); }

	U8 * Buffer( int i=0 ) const { return buffer->Buffer(i); }

	bool Resize( int newsize )
	{
		return (buffer->size() >= newsize) || (allowresize && buffer->resize(newsize));
	}

	void Clear( void )
	{
		readpos= 0;
		writepos= 0;

		buffer->clear();
	}

	// move the write position;
	// grow the used buffer by the passed amount
	bool SeekWrite( int pos );

	// there's a gotcha here such you can
	// seek to the end of the used buffer and
	// get an overread error if you attempt to read 
	// from that last (still unused) position
	bool SeekRead( int pos );
	
	// with allow's off (default)
	// assert overflowing/overreading
	bool allowoverflow : 1,
		 allowoverread : 1,
		 allowresize   : 1;

protected:
	// somewhat archaic/historical functions
	U8 * AdvanceWrite( int amt );
	U8 * AdvanceRead( int amt );
	
protected:
	int writepos, readpos;

	BufferObject * buffer;
};

// **************************************************** *
// WriteBuffer
// an extened write capable interface to the buffer io
// **************************************************** *

class WriteBuffer : public BufferObjectIO
{
public:
	WriteBuffer( const int alloc_size= AVG_BUFFER_SIZE ) :
		BufferObjectIO(alloc_size)
	{ }

	WriteBuffer( BufferObjectIO & m ) :
		BufferObjectIO( m )
	{ }
	
	bool WriteU8	( const U8 );
	bool WriteU16   ( const U16 ); 
	bool WriteU32   ( const U32 );
	bool WriteSingle( const SINGLE );
	bool WriteString( const char * string );
	bool WriteData  ( const U8 * data, const unsigned int num_bytes );

#ifdef VECTOR_H
	bool WriteVector( const Vector & v );
#endif
#ifdef QUAT_H
	bool WriteQuat  ( const Quaternion & q );
#endif
};

// **************************************************** *
// ReadBuffer
// an extened read capable interface to the buffer io
// **************************************************** *

class ReadBuffer : public BufferObjectIO
{
public:
	ReadBuffer( const int alloc_size= AVG_BUFFER_SIZE ) :
		BufferObjectIO(alloc_size)
	{ }

	ReadBuffer( BufferObjectIO & b ) :
		BufferObjectIO(b)
	{ }

	U8	 ReadU8	( void );
	U16	ReadU16   ( void ); 
	U32	ReadU32   ( void );
	SINGLE ReadSingle( void );
	bool   ReadString( char * string );
	bool   ReadData  ( U8 * data, const unsigned int num_bytes );

#ifdef VECTOR_H
	Vector ReadVector( void );
#endif
#ifdef QUAT_H
	Quaternion ReadQuat( void );
#endif 
};

// **************************************************** *
// inlined functions
// **************************************************** *

#ifdef VECTOR_H

inline bool WriteBuffer::WriteVector( const Vector & v )
{
	return  WriteSingle( v.x ) &&
			WriteSingle( v.y ) &&
			WriteSingle( v.z );
}

inline Vector ReadBuffer::ReadVector( void )
{
	Vector v;

	v.x= ReadSingle();
	v.y= ReadSingle();
	v.z= ReadSingle();
	
	return v;
}

#endif

#ifdef QUAT_H

inline bool WriteBuffer::WriteQuat( const Quaternion & q )
{
	for (int i=0; i< 4; i++)
	{
		if (!WriteSingle(q.d[i]))
			return false;
	}

	return true;
}


inline Quaternion ReadBuffer::ReadQuat( void )
{
	Quaternion q;
	
	for (int i=0;i<4;i++)
	{
		q.d[i]= ReadSingle();
	}

	return q;
}
	
#endif 

#endif // MSGBUFFER_H
