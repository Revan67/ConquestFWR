// Code to write properties out.

const unsigned int PROP_FILE_VERSION = 0xBEEF0001;

enum PROP_TYPE
{
	PT_UNKNOWN   = 0,  // type is unknown, data is invalid
	PT_LONG      = 1,  // type is 32 bit signed integer
	PT_ULONG     = 2,  // type is 32 bit unsigned integer
	PT_SINGLE    = 3,  // type is a single precision IEEE floating point number
	PT_DOUBLE    = 4,  // type is a double precision IEEE floating point number
	PT_VECTOR    = 5,  // type is a DA Vector
	PT_MATRIX    = 6,  // type is a DA Matrix
	PT_TRANSFORM = 7,  // type is a DA Transform
	PT_COMPONENT = 8,  // type is a DACOM component
	PT_STRING    = 9,  // type is a const char *
	PT_VOID      = 10  // type is a memory pointer
};

//
// Class and structure definitions.
//

// One of these for each property.
struct PersistProperty
{
	unsigned int propType;
	unsigned int nameLen;      // length of the name for this property, including the NULL terminator
	unsigned int dataLen;      // length of the type specific data for this property (includes nameLen)
	unsigned int dataOffset;   // offset from the start of the data area to the data for this property
};

// One of these at the start of the property file.
struct PersistPropHeader
{
	unsigned int fileVersion;  // property file format version
	unsigned int propCount;    // count of properties in the file
	unsigned int propOffset;   // offset from start of file to property table.
	unsigned int dataOffset;   // offset from start of file to first data entry

	void Init(void)
	{
		fileVersion = PROP_FILE_VERSION;
		propCount = 0;
		propOffset = sizeof(PersistPropHeader);
		dataOffset = 0;
	}

	void Add(void)
	{
		propCount++;
		dataOffset = propOffset + propCount * sizeof(PersistProperty);
	}
};


struct PersistProperty2 : public PersistProperty
{
	char *name;
	unsigned char *data;

	void Init(void)
	{
		propType = PT_UNKNOWN;
		nameLen = 0;
		dataLen = 0;
		dataOffset = 0;
		name = NULL;
		data = NULL;
	}

	void Release(void)
	{
		if(name) { free(name); name = NULL; }
		if(data) { free(data); data = NULL; }
	}
};
