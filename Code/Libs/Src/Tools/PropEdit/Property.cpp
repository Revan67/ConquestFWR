//
// Property.cpp - Source for the Property class
//

//
// Include files
//

#include "stdafx.h"

#include <stdio.h>

#include "property.h"

//
// Local variables
//

static const char *typeNames[] =
{
	"Unknown",
	"Long",
	"Unsigned Long",
	"SINGLE",
	"DOUBLE",
	"Vector",
	"Matrix",
	"Transform",
	"Component",
	"String",
	"Void"
};

//
// Methods
//

Property::Property (const char *_name)
{
	name = _name;
	type = PT_UNKNOWN;
	longVal = 0;
}

Property::Property (const Property &prop)
{
	type = PT_UNKNOWN;
	longVal = 0;

	*this = prop;
}

Property::~Property ()
{
	clear ();
}

Property & Property::operator = (const Property &prop)
{
	clear ();
	name = prop.name;
	switch (prop.type)
	{
	case PT_LONG:
		set_long (prop.longVal);
		break;

	case PT_ULONG:
		set_ulong(prop.ulongVal);
		break;

	case PT_SINGLE:
		set_single(prop.singleVal);
		break;

	case PT_DOUBLE:
		set_double(prop.doubleVal);
		break;

	case PT_VECTOR:
		set_vector(&prop.vectorVal);
		break;

	case PT_MATRIX:
		set_matrix(&prop.matrixVal);
		break;

	case PT_TRANSFORM:
		set_transform(&prop.transformVal);
		break;

	case PT_STRING:
		set_string(prop.stringVal);
		break;
	}
	return *this;
}

unsigned int Property::get_data_size ()
{
	unsigned int size = 0;

	switch (type)
	{
	case PT_LONG:
		size = sizeof(long);
		break;

	case PT_ULONG:
		size = sizeof(unsigned long);
		break;

	case PT_SINGLE:
		size = sizeof(SINGLE);
		break;

	case PT_DOUBLE:
		size = sizeof(DOUBLE);
		break;

	case PT_VECTOR:
		size = sizeof(PersistVector);
		break;

	case PT_MATRIX:
		size = sizeof(PersistMatrix);
		break;

	case PT_TRANSFORM:
		size = sizeof(PersistTransform);
		break;

	case PT_STRING:
		size = strlen (stringVal) + 1;
		break;
	}

	return size;
}

const char *Property::get_type_name ()
{
	return typeNames[type];
}

const char *Property::get_type_name (PROP_TYPE _type)
{
	return typeNames[_type];
}

const char *Property::get_name ()
{
	return name;
}

void *Property::get_data ()
{
	void *result = NULL;

	switch (type)
	{
	case PT_LONG:
	case PT_ULONG:
	case PT_SINGLE:
	case PT_DOUBLE:
		result = &longVal;
		break;

	case PT_VECTOR:
		result = &vectorVal;
		break;

	case PT_MATRIX:
		result = &matrixVal;
		break;

	case PT_TRANSFORM:
		result = &transformVal;
		break;

	case PT_STRING:
		result = stringVal;
		break;
	}

	return result;
}

void Property::clear ()
{
	if (type == PT_STRING)
	{
		if (stringVal != NULL)
		{
			delete stringVal;
			stringVal = NULL;
		}
	}
	dataString = "<UNKNOWN>";

	type = PT_UNKNOWN;
}


void Property::set_vector (const PersistVector *value)
{
	clear ();
	type = PT_VECTOR;
	vectorVal = *value;
	dataString.Format ("<%.3f, %.3f, %.3f>", value->x, value->y, value->z);
}

void Property::set_matrix (const PersistMatrix *value)
{
	clear ();
	type = PT_MATRIX;
	matrixVal = *value;
	dataString.Format
	(
		"<%.3f, %.3f, %.3f> <%.3f, %.3f, %.3f> <%.3f, %.3f, %.3f>",
		value->e00, value->e01, value->e02,
		value->e10, value->e11, value->e12,
		value->e20, value->e21, value->e22
	);
}

void Property::set_transform (const PersistTransform *value)
{
	clear ();
	type = PT_TRANSFORM;
	transformVal = *value;
	dataString.Format
	(
		"<%.3f, %.3f, %.3f> <%.3f, %.3f, %.3f> <%.3f, %.3f, %.3f> <%.3f, %.3f, %.3f>",
		value->m.e00, value->m.e01, value->m.e02,
		value->m.e10, value->m.e11, value->m.e12,
		value->m.e20, value->m.e21, value->m.e22,
		value->v.x, value->v.y, value->v.z
	);
}

void Property::set_long (long value)
{
	clear ();
	type = PT_LONG;
	longVal = value;
	dataString.Format ("%d", value);
}

void Property::set_ulong (unsigned long value)
{
	clear ();
	type = PT_ULONG;
	ulongVal = value;
	dataString.Format ("%u", value);
}

void Property::set_single (SINGLE value)
{
	clear ();
	type = PT_SINGLE;
	singleVal = value;
	dataString.Format ("%.3f", value);
}

void Property::set_double (DOUBLE value)
{
	clear ();
	type = PT_DOUBLE;
	doubleVal = value;
	dataString.Format ("%.3f", value);
}

void Property::set_string (const char *value)
{
	clear ();
	type = PT_STRING;
	stringVal = strdup (value);
	// Yes, I know this is inefficient.
	dataString.Format ("\"%s\"", stringVal);
}

const char *Property::get_data_string ()
{
	return dataString;
}

void Property::set (PROP_TYPE _type, const char *valueString)
{
	switch (_type)
	{
	case PT_LONG:
		set_long (atoi(valueString));
		break;

	case PT_ULONG:
		// Yes, there is a range issue here.
		set_ulong(atoi(valueString));
		break;

	case PT_SINGLE:
		set_single(atof(valueString));
		break;

	case PT_DOUBLE:
		set_double (atof(valueString));
		break;

	case PT_VECTOR:
	case PT_MATRIX:
	case PT_TRANSFORM:
		// You can't do this.
		break;

	case PT_STRING:
		set_string (valueString);
		break;
	}
}

