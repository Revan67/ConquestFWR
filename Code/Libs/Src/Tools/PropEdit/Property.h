#ifndef PROPERTY_H
#define PROPERTY_H
//
// Property.h - General property definition
//

//
// Include files
//

#include <persistmath.h>

//
// Simple type definitions
//

typedef unsigned long PROP_TYPE;
typedef float SINGLE;
typedef double DOUBLE;

//
// Constants
//

const unsigned int PROP_FILE_VERSION = 0xBEEF0001;

const PROP_TYPE PT_UNKNOWN   = 0;  // type is unknown, data is invalid
const PROP_TYPE PT_LONG      = 1;  // type is 32 bit signed integer
const PROP_TYPE PT_ULONG     = 2;  // type is 32 bit unsigned integer
const PROP_TYPE PT_SINGLE    = 3;  // type is a single precision IEEE floating point number
const PROP_TYPE PT_DOUBLE    = 4;  // type is a double precision IEEE floating point number
const PROP_TYPE PT_VECTOR    = 5;  // type is a DA Vector
const PROP_TYPE PT_MATRIX    = 6;  // type is a DA Matrix
const PROP_TYPE PT_TRANSFORM = 7;  // type is a DA Transform
const PROP_TYPE PT_COMPONENT = 8;  // type is a DACOM component
const PROP_TYPE PT_STRING    = 9;  // type is a const char *
const PROP_TYPE PT_VOID      = 10; // type is a memory pointer

//
// Class and structure definitions.
//

// One of these at the start of the property file.
struct PersistPropHeader
{
	unsigned int fileVersion;  // property file format version
	unsigned int propCount;    // count of properties in the file
	unsigned int propOffset;   // offset from start of file to property table.
	unsigned int dataOffset;   // offset from start of file to first data entry
};

// One of these for each property.
struct PersistProperty
{
	unsigned int propType;
	unsigned int nameLen;      // length of the name for this property, including the NULL terminator
	unsigned int dataLen;      // length of the type specific data for this property
	unsigned int dataOffset;   // offset from the start of the data area to the data for this property
};

struct Property
{
	CString   name;
	CString   dataString;
	PROP_TYPE type;
	union
	{
		long					longVal;
		unsigned long           ulongVal;
		SINGLE					singleVal;
		DOUBLE					doubleVal;
		char *                  stringVal;
	};
	PersistVector               vectorVal;
	PersistMatrix		        matrixVal;
	PersistTransform            transformVal;

	Property () {}
	Property (const char *_name);
	Property (const Property &prop);
	~Property ();

	Property & operator = (const Property &prop);

	unsigned int get_data_size ();  // returns the data size for the encapsulated type
	void *get_data ();
	const char *get_type_name ();
	static const char *get_type_name (PROP_TYPE _type);
	const char *get_data_string ();  // returns a string representation of the data in the property.
	const char *get_name ();
	void clear ();

	void set_vector (const PersistVector *value);
	void set_matrix (const PersistMatrix *value);
	void set_transform (const PersistTransform *value);
	void set_long (long value);
	void set_ulong (unsigned long value);
	void set_single (SINGLE value);
	void set_double (DOUBLE value);
	void set_string (const char *value);

	void set (PROP_TYPE _type, const char *valueString);
};

#endif
