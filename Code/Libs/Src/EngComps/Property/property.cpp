//--------------------------------------------------------------------------//
//                                                                          //
//                                Property.cpp                              //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	$Header: /Libs/dev/Src/EngComps/Property/property.cpp 11    3/21/00 4:30p Pbleisch $
*/			    
//--------------------------------------------------------------------------//

//
// Design Notes:
//      Like the rest of the engine, properties have archetypes and instances. An instance ALWAYS gets a copy of
// its archetype's properties, so fiddling with an instance's properties will affect only that instance.
//
// File format:
//      A property file has a single binary format. It consists of a file header, a table of property definitions,
// and a table of property values.
//

//
// Include files
//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

//

#include "fdump.h"
#include "tempstr.h"
#include "da_heap_utility.h"
#include "engcomp.h"
#include "SysConsumerDesc.h"
#include "tcomponent.h"
#include "TSmartPointer.h"
#include "IProperties.h"
#include "3dmath.h"
#include "icamera.h"

//

#include "handlemap.h"

//

#include "hashtable.h"

//

//
// Constants
//

const unsigned int PROP_FILE_VERSION = 0xBEEF0001;

//
// Class and structure definitions
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

struct PropertyTable : public HashTable {
	bool getProperty( int index, IProperty *&property) {
		// NOTE: This index is a raw index into the hashtable, not the Ith record!!!
		char *ptr = getVal( index );
		if( ptr ) {
			property = *((IProperty **)ptr);
			return true;
		}
		return false;
	}

	bool getProperty( const char *key, IProperty *&property ) {
		char *ptr = get( key );
		if( ptr ) {
			property = *((IProperty **)ptr);
			return true;
		}
		return false;
	}

	// NOTE: We are not calling AddRef() here. It is expected to be called outside of this
	// method. This means that we must remove previous entries before adding new ones.
	void set( const char *key, const IProperty *property ) {
		HashTable::set( key, (const char *)&property, sizeof(property) );
	}

	void copyFrom( PropertyTable &table )
	{
		for( int i=0; i<table.size(); i++ ) {
			char *key = table.getKey(i);
			IProperty **valPtr = (IProperty **) table.getVal(i);
			if( key && valPtr) {
				set( key, *valPtr );
			}
		}
	}
};

struct LocalProperty : public IProperty, public ISetProperty
{
	BEGIN_DACOM_MAP_INBOUND(LocalProperty)
	DACOM_INTERFACE_ENTRY(IProperty)
	DACOM_INTERFACE_ENTRY2(IID_IProperty,IProperty)
	DACOM_INTERFACE_ENTRY(ISetProperty)
	DACOM_INTERFACE_ENTRY2(IID_ISetProperty,ISetProperty)
	END_DACOM_MAP()

	PROP_TYPE                   type;
	union
	{
		long					longVal;
		unsigned long           ulongVal;
		SINGLE					singleVal;
		DOUBLE					doubleVal;
		struct IDAComponent *	component;
		char *                  stringVal;
		void *                  pVoid;
		Vector *		        pVector;
		Matrix *		        pMatrix;
		Transform *             pTransform;
	};

	LocalProperty ()
	{
		type = PT_UNKNOWN;
	}

	~LocalProperty ()
	{
		clear ();
	}

	GENRESULT init(DACOMDESC * info)
	{
		return GR_OK;
	}

	void set_vector (const PersistVector *value)
	{
		clear ();
		type = PT_VECTOR;
		pVector = new Vector(*value);
		*pVector = *value;
	}

	void set_matrix (const PersistMatrix *value)
	{
		clear ();
		type = PT_MATRIX;
		pMatrix = new Matrix(*value);
		*pMatrix = *value;
	}

	void set_transform (const PersistTransform *value)
	{
		clear ();
		type = PT_TRANSFORM;
		pTransform = new Transform(*value);
	}

	// ISetProperty methods.
	virtual void COMAPI set_long (long value)
	{
		clear ();
		type = PT_LONG;
		longVal = value;
	}

	virtual void COMAPI set_ulong (unsigned long value)
	{
		clear ();
		type = PT_ULONG;
		ulongVal = value;
	}

	virtual void COMAPI set_single (SINGLE value)
	{
		clear ();
		type = PT_SINGLE;
		singleVal = value;
	}

	virtual void COMAPI set_double (DOUBLE value)
	{
		clear ();
		type = PT_DOUBLE;
		doubleVal = value;
	}

	virtual void COMAPI set_vector (const Vector &value)
	{
		clear ();
		type = PT_VECTOR;
		pVector = new Vector;
		*pVector = value;
	}

	virtual void COMAPI set_matrix (const Matrix &value)
	{
		clear ();
		type = PT_MATRIX;
		pMatrix = new Matrix;
		*pMatrix = value;
	}

	virtual void COMAPI set_transform (const Transform &value)
	{
		clear ();
		type = PT_TRANSFORM;
		pTransform = new Transform;
		*pTransform = value;
	}

	virtual void COMAPI set_component (IDAComponent *value)
	{
		clear ();
		type = PT_COMPONENT;
		component = value;
		component->AddRef();
	}

	virtual void COMAPI set_string (const char *value)
	{
		clear ();
		type = PT_STRING;
		stringVal = strdup (value);
	}

	virtual void COMAPI set_void (void *value)
	{
		clear ();
		type = PT_VOID;
		pVoid = value;
	}

	virtual void COMAPI clear ()
	{
		if (type == PT_VECTOR)
		{
			delete pVector;
		}

		if (type == PT_MATRIX)
		{
			delete pMatrix;
		}

		if (type == PT_TRANSFORM)
		{
			delete pTransform;
		}

		if (type == PT_STRING)
		{
			delete stringVal;
		}

		if (type == PT_COMPONENT)
		{
			// We release our hold on the component
			component->Release();
		}

		type = PT_UNKNOWN;
	}

	// IProperty methods.
	virtual PROP_TYPE COMAPI get_type ()
	{
		return type;
	}

	// These methods retrieve the known value types. No coersion is done on the values.
	// If the type does not match the type implicit in the function, it returns GR_INVALID_PARMS.
	virtual GENRESULT COMAPI get_long (long &value)
	{
		if (type == PT_LONG)
		{
			value = longVal;
			return GR_OK;
		}
		return GR_INVALID_PARMS;
	}

	virtual GENRESULT COMAPI get_ulong (unsigned long &value)
	{
		if (type == PT_ULONG)
		{
			value = ulongVal;
			return GR_OK;
		}
		return GR_INVALID_PARMS;
	}

	virtual GENRESULT COMAPI get_single (SINGLE &value)
	{
		if (type == PT_SINGLE)
		{
			value = singleVal;
			return GR_OK;
		}
		return GR_INVALID_PARMS;
	}

	virtual GENRESULT COMAPI get_double (DOUBLE &value)
	{
		if (type == PT_DOUBLE)
		{
			value = doubleVal;
			return GR_OK;
		}
		return GR_INVALID_PARMS;
	}

	virtual GENRESULT COMAPI get_vector (Vector &value)
	{
		if (type == PT_VECTOR)
		{
			value = *pVector;
			return GR_OK;
		}
		return GR_INVALID_PARMS;
	}

	virtual GENRESULT COMAPI get_matrix (Matrix &value)
	{
		if (type == PT_MATRIX)
		{
			value = *pMatrix;
			return GR_OK;
		}
		return GR_INVALID_PARMS;
	}

	virtual GENRESULT COMAPI get_transform (Transform &value)
	{
		if (type == PT_TRANSFORM)
		{
			value = *pTransform;
			return GR_OK;
		}
		return GR_INVALID_PARMS;
	}

	virtual GENRESULT COMAPI get_component (IDAComponent *&value)
	{
		if (type == PT_TRANSFORM)
		{
			value = component;
			return GR_OK;
		}
		return GR_INVALID_PARMS;
	}

	virtual GENRESULT COMAPI get_string (const char *&value)
	{
		if (type == PT_STRING)
		{
			value = (const char *) stringVal;
			return GR_OK;
		}
		return GR_INVALID_PARMS;
	}

	virtual GENRESULT COMAPI get_void (void *&value)
	{
		if (type == PT_VOID)
		{
			value = pVoid;
			return GR_OK;
		}
		return GR_INVALID_PARMS;
	}
};

struct DACOM_NO_VTABLE PropertyComp : public IEngineComponent, public IProperties
{
	BEGIN_DACOM_MAP_INBOUND(PropertyComp)
	DACOM_INTERFACE_ENTRY(IAggregateComponent)
	DACOM_INTERFACE_ENTRY(IEngineComponent)
	DACOM_INTERFACE_ENTRY(IProperties)
	DACOM_INTERFACE_ENTRY2(IID_IAggregateComponent,IAggregateComponent)
	DACOM_INTERFACE_ENTRY2(IID_IEngineComponent,IEngineComponent)
	DACOM_INTERFACE_ENTRY2(IID_IProperties,IProperties)
	END_DACOM_MAP()

//
// Implementation-specific stuff.
//
	class PArchetype
	{
	public:
		PropertyTable props;

		PArchetype()
		{
		}

		~PArchetype ()
		{
			release_all_props();
		}

		void release_all_props ()
		{
			// Release all of the properties and clear the has table

			for (int i = 0; i < props.size(); ++i)
			{
				IProperty *value;
				if (props.getProperty (i, value))
				{
					value->Release();
				}
			}

			props.clear ();
		}

		bool init (void *buffer, U32 size)
		{
			char *buf = (char *) buffer;
			// Verify the header.
			PersistPropHeader *hdr = (PersistPropHeader *) buf;
			if (hdr->fileVersion != PROP_FILE_VERSION)
			{
				return false;
			}

			PersistProperty *propStart = (PersistProperty *) (buf + hdr->propOffset);
			char *dataStart = buf + hdr->dataOffset;
			int count = hdr->propCount;

			for (int i = 0; i < count; ++i)
			{
				// Parse this property.

				// Find this property's data record and name.

				PersistProperty *p = &propStart[i];
				char *record = dataStart + p->dataOffset;
				ASSERT (p->nameLen != 0);
				ASSERT (record[p->nameLen-1] == '\0');
				char *name = record;
				char *data = record + p->nameLen;
				PROP_TYPE type = (PROP_TYPE) p->propType;

				LocalProperty *value = new DAComponent<LocalProperty>;
				bool valid = true;

				switch (type)
				{
				case  PT_LONG:
					value->set_long (*((long *) data));
					break;

				case  PT_ULONG:
					value->set_ulong (*((unsigned long *) data));
					break;

				case  PT_SINGLE:
					value->set_single (*((SINGLE *) data));
					break;

				case  PT_DOUBLE:
					value->set_double (*((DOUBLE *) data));
					break;

				case  PT_VECTOR:
					value->set_vector ((PersistVector *) data);
					break;

				case  PT_MATRIX:
					value->set_matrix ((PersistMatrix *) data);
					break;

				case  PT_TRANSFORM:
					value->set_transform ((PersistTransform *) data);
					break;

				case  PT_COMPONENT:
					GENERAL_WARNING ("Component properties cannot be loaded from files!\n");
					delete value;
					value = NULL;
					break;

				case  PT_STRING:
					value->set_string ((const char *) data);
					break;

				case  PT_VOID:
					GENERAL_WARNING ("Void properties cannot be loaded from files!\n");
					delete value;
					value = NULL;
					break;

				default:
					valid = false;
					GENERAL_WARNING ("Unsupported property type found.\n");
					break;
				}

				if (value)
				{
					props.set (name, value);
				}
			}

			return true;
		}

		PArchetype &operator = (PArchetype &srcArch)
		{
			release_all_props();
			props.copyFrom (srcArch.props);

			// Run through the list of properties we just copied and increase the reference count on each
			for (int i = 0; i < props.size(); ++i)
			{
				IProperty *value;
				if (props.getProperty (i, value))
				{
					value->AddRef();
				}
			}

			return *this;
		}
	};

	class PInstance
	{
	public:
		INSTANCE_INDEX archIndex;
		INSTANCE_INDEX objectIndex;
//		int			   listIndex;
		PropertyTable  props;

		PInstance(INSTANCE_INDEX archIndexIn, int listIndexIn) :
			archIndex(archIndexIn),
			objectIndex(INVALID_INSTANCE_INDEX)
//			,listIndex(listIndex)
		{
		}

		~PInstance ()
		{
			release_all_props();
		}

		void release_all_props ()
		{
			// Release all of the properties and clear the has table

			for (int i = 0; i < props.size(); ++i)
			{
				IProperty *value;
				if (props.getProperty (i, value))
				{
					value->Release();
				}
			}

			props.clear ();
		}
	};

	typedef arch_handlemap< PArchetype* > arch_map;
	typedef inst_handlemap< PInstance* > inst_map;

	arch_map			archetypes;
	mutable inst_map	instances;

//	DynamicArray<INSTANCE_INDEX> instanceList;
//	unsigned int                 instanceCount;

	IEngine * engine;

public: // Interface

	PropertyComp () ;
	~PropertyComp ();
	DA_HEAP_DEFINE_NEW_OPERATOR(PropertyComp);

	GENRESULT init(SYSCONSUMERDESC* info);

	// IAggregate
	virtual GENRESULT	COMAPI Initialize(void);

	// IEngineComponent methods
	BOOL32 COMAPI create_archetype( ARCHETYPE_INDEX arch_index, struct IFileSystem *filesys ) ;
	void COMAPI	duplicate_archetype( ARCHETYPE_INDEX new_arch_index, ARCHETYPE_INDEX old_arch_index ) ;
	void COMAPI destroy_archetype( ARCHETYPE_INDEX arch_index ) ;
	GENRESULT COMAPI query_archetype_interface( ARCHETYPE_INDEX arch_index, const char *iid, IDAComponent **out_iif ) ;
	BOOL32 COMAPI create_instance( INSTANCE_INDEX inst_index, ARCHETYPE_INDEX arch_index ) ;
	void COMAPI destroy_instance( INSTANCE_INDEX inst_index ) ;
	void COMAPI update_instance( INSTANCE_INDEX inst_index, SINGLE dt ) ;
	enum vis_state COMAPI render_instance( struct ICamera *camera, INSTANCE_INDEX inst_index, float lod_fraction, U32 flags, const Transform *modifier_transform ) ;
	GENRESULT COMAPI query_instance_interface( INSTANCE_INDEX inst_index, const char *iid, IDAComponent **out_iif ) ;
	void COMAPI update(SINGLE dt) ;

	// IProperties methods.
	int COMAPI get_count (INSTANCE_INDEX idx);
	const char* COMAPI get_name (INSTANCE_INDEX idx, int propIndex);
	bool COMAPI exists (INSTANCE_INDEX idx, const char *name);
	GENRESULT COMAPI get_by_name (INSTANCE_INDEX idx, const char *name, IProperty **prop);
	GENRESULT COMAPI set_by_name (INSTANCE_INDEX idx, const char *name, IProperty *prop);
	bool COMAPI del_by_name (INSTANCE_INDEX idx, const char *name);
	GENRESULT COMAPI get_by_index (INSTANCE_INDEX idx, int propIndex, IProperty **prop);
	GENRESULT COMAPI set_by_index (INSTANCE_INDEX idx, int propIndex, IProperty *prop);
	bool COMAPI del_by_index (INSTANCE_INDEX idx, int propIndex);
	int COMAPI arch_get_count (ARCHETYPE_INDEX idx);
	const char* COMAPI arch_get_name (ARCHETYPE_INDEX idx, int propIndex);
	bool COMAPI arch_exists (ARCHETYPE_INDEX idx, const char *name);
	GENRESULT COMAPI arch_get_by_name (ARCHETYPE_INDEX idx, const char *name, IProperty **prop);
	GENRESULT COMAPI arch_set_by_name (ARCHETYPE_INDEX idx, const char *name, IProperty *prop);
	bool COMAPI arch_del_by_name (ARCHETYPE_INDEX idx, const char *name);
	GENRESULT COMAPI arch_get_by_index (ARCHETYPE_INDEX idx, int propIndex, IProperty **prop);
	GENRESULT COMAPI arch_set_by_index (ARCHETYPE_INDEX idx, int propIndex, IProperty *prop);
	bool COMAPI arch_del_by_index (ARCHETYPE_INDEX idx, int propIndex);

protected: // Interface

// Local methods
	static bool LoadFile (const char *fileName, void *& buffer, U32 &size, IFileSystem * parent);

	IDAComponent* get_base(void)
	{
		return (IEngineComponent*)this;
	}

	//

	inline PArchetype *get_archetype (ARCHETYPE_INDEX idx)
	{
		if (idx == INVALID_ARCHETYPE_INDEX)
		{
			return NULL;
		}

		arch_map::iterator arch;

		if( (arch = archetypes.find( idx )) == archetypes.end() ) {
			return NULL;
		}

		return arch->second;
	}

	//

	inline PInstance *get_instance (INSTANCE_INDEX idx)
	{
		if (idx == INVALID_INSTANCE_INDEX)
		{
			return NULL;
		}

		inst_map::iterator inst;

		if( (inst = instances.find( idx )) == instances.end() ) {
			return NULL;
		}

		return inst->second;
	}
};

//
// Local variables
//

HINSTANCE	hInstance;	// DLL instance handle
ICOManager *DACOM;		// Handle to component manager

C8 *interface_name = "IProperties";  // Interface name used for registration     
C8 *prop_interface_name = "IProperty";  // Interface name used for registration     

//
// Methods
//

PropertyComp::PropertyComp () 
{
	engine = NULL;
}

//

GENRESULT PropertyComp::init(SYSCONSUMERDESC * info)
{
	return GR_OK;
}

//

PropertyComp::~PropertyComp()
{
	inst_map::iterator ibeg = instances.begin();
	inst_map::iterator iend = instances.end();
	inst_map::iterator inst;

	for( inst=ibeg; inst!=iend; inst++ ) {
		GENERAL_NOTICE( TEMPSTR( "Property: dtor: Instance %d has dangling properties\n", (*inst).first ) );
	}

	arch_map::iterator abeg = archetypes.begin();
	arch_map::iterator aend = archetypes.end();
	arch_map::iterator arch;

	for( arch=abeg; arch!=aend; arch++ ) {
		GENERAL_NOTICE( TEMPSTR( "Property: dtor: Archetype %d has dangling properties\n", (*arch).first ) );
	}
}

//

GENRESULT COMAPI PropertyComp::Initialize(void)
{
	if (get_base()->QueryInterface(IID_IEngine, (void **) &engine) == GR_OK)
	{
		get_base()->Release();
	}

	return GR_OK;
}

// IEngineComponent methods

BOOL32 COMAPI PropertyComp::create_archetype(ARCHETYPE_INDEX archIndex, IFileSystem * parent)
{
	ASSERT( archIndex != INVALID_ARCHETYPE_INDEX );
	ASSERT( parent != NULL );

	PArchetype *archetype;
	void *fBuf = NULL;
	DWORD fSize = 0;

	if( LoadFile ("Properties", fBuf, fSize, parent) == 0 ) {
		return FALSE;
	}

	archetype = new PArchetype;
	ASSERT( archetype );

	archetype->init( fBuf, fSize );
	delete fBuf;

	archetypes.insert( archIndex, archetype );

	return TRUE;
}

//

void COMAPI PropertyComp::duplicate_archetype(ARCHETYPE_INDEX new_arch_index, ARCHETYPE_INDEX old_arch_index )
{
	ASSERT( new_arch_index != INVALID_ARCHETYPE_INDEX );
	ASSERT( old_arch_index != INVALID_ARCHETYPE_INDEX );

	PArchetype *old_arch, *new_arch;
	
	if( (old_arch = get_archetype( old_arch_index )) != NULL ) {
		new_arch = new PArchetype;
		*new_arch = *old_arch;
		archetypes.insert( new_arch_index, new_arch );
	}
}

//

void COMAPI PropertyComp::destroy_archetype(ARCHETYPE_INDEX archIndex)
{
	ASSERT( archIndex != INVALID_ARCHETYPE_INDEX );

	arch_map::iterator arch;

	if( (arch = archetypes.find( archIndex )) != archetypes.end() ) {
		delete arch->second;
		archetypes.erase( archIndex );
	}
}

//

GENRESULT COMAPI PropertyComp::query_archetype_interface( ARCHETYPE_INDEX, const char *, IDAComponent ** )
{
	return GR_GENERIC;
}

//

BOOL32 COMAPI PropertyComp::create_instance(INSTANCE_INDEX  instIndex, ARCHETYPE_INDEX archIndex)
{
	ASSERT( instIndex != INVALID_INSTANCE_INDEX );
	ASSERT( archIndex != INVALID_ARCHETYPE_INDEX );

	PInstance *instance;
	PArchetype *archetype;

	if( (archetype = get_archetype( archIndex )) == NULL ) {
		return FALSE;
	}

	instance = new PInstance( archIndex, instIndex );

	ASSERT( instance );

	// If this instance's archetype has properties, we copy them. Note that we are copying the
	// property objects from the archetype, so we don't have our own private copy of the values, just
	// the object pointers.
	// This means that if you retrieve a property for an instance, query its ISetProperty interface, then
	// use that interface to modify the property, you will have done so for that property on all instances
	// of the archetype. If, however, you create an new IProperty and set it on the instance, you will be
	// changing the property for that instance only.

	instance->props.copyFrom( archetype->props );

	// We need to AddRef the properties we just got from the archetype.
	// Prior to this call, we didn't have any properties, so we can just AddRef all our
	// current properties.
	for( int i = 0; i < instance->props.size(); ++i ) {
		IProperty *value;
		if( instance->props.getProperty (i, value)) {
			value->AddRef();
		}
	}

	instances.insert( instIndex, instance );

	return TRUE;
}

//

void COMAPI PropertyComp::destroy_instance(INSTANCE_INDEX instIndex)
{
	ASSERT( instIndex != INVALID_INSTANCE_INDEX );

	PInstance* instance ;
	
	if( (instance = get_instance( instIndex )) == NULL ) {
		return;
	}

	delete instance;
	instances.erase( instIndex );
}

//

void COMAPI PropertyComp::update(SINGLE time_step)
{
	// Nothing to do here.
}

//

void COMAPI PropertyComp::update_instance (INSTANCE_INDEX index, SINGLE time_step)
{
	// Nothing to do here.
}

//

vis_state COMAPI PropertyComp::render_instance(struct ICamera * camera,
											  INSTANCE_INDEX	instIndex,
											  float lod_fraction,
											  U32					flags,
											  const Transform * tr)
{
	// Nothing to do here.
	return VS_UNKNOWN;
}

//

GENRESULT COMAPI PropertyComp::query_instance_interface( INSTANCE_INDEX, const char *, IDAComponent ** )
{
	return GR_GENERIC;
}

//

// IProperties methods
int COMAPI PropertyComp::get_count (INSTANCE_INDEX idx)
{
	int result = 0;
	PInstance *inst = get_instance (idx);
	if (inst)
	{
		result = inst->props.getCount();
	}
	return result;
}

const char* COMAPI PropertyComp::get_name (INSTANCE_INDEX idx, int propIndex)
{
	const char *result = NULL;
	PInstance *inst = get_instance (idx);
	if (inst)
	{
		int hashIndex = inst->props.get_ith_index (propIndex);
		if (hashIndex  != -1)
		{
			result = inst->props.getKey(hashIndex);
		}
	}
	return result;
}

bool COMAPI PropertyComp::exists (INSTANCE_INDEX idx, const char *name)
{
	bool result = 0;
	PInstance *inst = get_instance (idx);
	if (inst && name != NULL)
	{
		result = (inst->props.lookup(name) != -1);
	}
	return result;
}

GENRESULT COMAPI PropertyComp::get_by_name (INSTANCE_INDEX idx, const char *name, IProperty **prop)
{
	GENRESULT result = GR_INVALID_PARMS;
	PInstance *inst = get_instance (idx);
	if (inst && prop != NULL && name != NULL)
	{
		if (inst->props.getProperty(name, *prop))
		{
			// We add a reference here, expecting the caller to release it when they are done.
			(*prop)->AddRef();
			result = GR_OK;
		}
	}
	return result;
}

GENRESULT COMAPI PropertyComp::set_by_name (INSTANCE_INDEX idx, const char *name, IProperty *prop)
{
	GENRESULT result = GR_INVALID_PARMS;
	PInstance *inst = get_instance (idx);
	if (inst && prop != NULL && name != NULL)
	{
		// Release the reference to any existing property with this name.
		IProperty *oldProp;

		if (inst->props.getProperty(name, oldProp))
		{
			oldProp->Release();
		}
		inst->props.set(name, prop);
		prop->AddRef();
		result = GR_OK;
	}
	return result;
}

bool COMAPI PropertyComp::del_by_name (INSTANCE_INDEX idx, const char *name)
{
	bool result = 0;
	PInstance *inst = get_instance (idx);
	if (inst)
	{
		// Release the reference to any existing property with this name.
		IProperty *oldProp;

		if (inst->props.getProperty(name, oldProp))
		{
			oldProp->Release();
		}
		inst->props.set(name, NULL);
		result = true;
	}
	return result;
}

GENRESULT COMAPI PropertyComp::get_by_index (INSTANCE_INDEX idx, int propIndex, IProperty **prop)
{
	GENRESULT result = GR_INVALID_PARMS;
	PInstance *inst = get_instance (idx);
	if (inst && prop != NULL)
	{
		int hashIndex = inst->props.get_ith_index (propIndex);
		if (hashIndex  != -1)
		{
			if (inst->props.getProperty(hashIndex, *prop))
			{
				// We add a reference here, expecting the caller to release it when they are done.
				(*prop)->AddRef();
				result = GR_OK;
			}
		}
	}
	return result;
}

GENRESULT COMAPI PropertyComp::set_by_index (INSTANCE_INDEX idx, int propIndex, IProperty *prop)
{
	GENRESULT result = GR_INVALID_PARMS;
	PInstance *inst = get_instance (idx);
	if (inst && prop != NULL)
	{
		int hashIndex = inst->props.get_ith_index (propIndex);
		if (hashIndex != -1)
		{
			char *name = inst->props.getKey (hashIndex);
			if (name)
			{
				// Release the reference to any existing property with this name.
				IProperty *oldProp;

				if (inst->props.getProperty(hashIndex, oldProp))
				{
					oldProp->Release();
				}
				inst->props.set(name, prop);
				prop->AddRef();
				result = GR_OK;
			}
		}
	}
	return result;
}

bool COMAPI PropertyComp::del_by_index (INSTANCE_INDEX idx, int propIndex)
{
	bool result = 0;
	PInstance *inst = get_instance (idx);
	if (inst)
	{
		int hashIndex = inst->props.get_ith_index (propIndex);
		if (hashIndex != -1)
		{
			char *name = inst->props.getKey (hashIndex);
			if (name)
			{
				// Release the reference to any existing property with this name.
				IProperty *oldProp;

				if (inst->props.getProperty(hashIndex, oldProp))
				{
					oldProp->Release();
				}
				inst->props.set(name, NULL);
				result = true;
			}
		}
	}
	return result;
}

int COMAPI PropertyComp::arch_get_count (ARCHETYPE_INDEX idx)
{
	int result = 0;
	PArchetype *arch = get_archetype (idx);
	if (arch)
	{
		result = arch->props.getCount();
	}
	return result;
}

const char* COMAPI PropertyComp::arch_get_name (ARCHETYPE_INDEX idx, int propIndex)
{
	const char *result = NULL;
	PArchetype *arch = get_archetype (idx);
	if (arch)
	{
		int hashIndex = arch->props.get_ith_index (propIndex);
		if (hashIndex  != -1)
		{
			result = arch->props.getKey(hashIndex);
		}
	}
	return result;
}

bool COMAPI PropertyComp::arch_exists (ARCHETYPE_INDEX idx, const char *name)
{
	bool result = 0;
	PArchetype *arch = get_archetype (idx);
	if (arch && name != NULL)
	{
		result = (arch->props.lookup(name) != -1);
	}
	return result;
}

GENRESULT COMAPI PropertyComp::arch_get_by_name (ARCHETYPE_INDEX idx, const char *name, IProperty **prop)
{
	GENRESULT result = GR_INVALID_PARMS;
	PArchetype *arch = get_archetype (idx);
	if (arch && prop != NULL && name != NULL)
	{
		if (arch->props.getProperty(name, *prop))
		{
			// We add a reference here, expecting the caller to release it when they are done.
			(*prop)->AddRef();
			result = GR_OK;
		}
	}
	return result;
}

GENRESULT COMAPI PropertyComp::arch_set_by_name (ARCHETYPE_INDEX idx, const char *name, IProperty *prop)
{
	GENRESULT result = GR_INVALID_PARMS;
	PArchetype *arch = get_archetype (idx);
	if (arch && prop != NULL && name != NULL)
	{
		// Release the reference to any existing property with this name.
		IProperty *oldProp;

		if (arch->props.getProperty(name, oldProp))
		{
			oldProp->Release();
		}
		arch->props.set(name, prop);
		prop->AddRef();
		result = GR_OK;
	}
	return result;
}

bool COMAPI PropertyComp::arch_del_by_name (ARCHETYPE_INDEX idx, const char *name)
{
	bool result = 0;
	PArchetype *arch = get_archetype (idx);
	if (arch)
	{
		// Release the reference to any existing property with this name.
		IProperty *oldProp;

		if (arch->props.getProperty(name, oldProp))
		{
			oldProp->Release();
		}
		arch->props.set(name, NULL);
		result = true;
	}
	return result;
}

GENRESULT COMAPI PropertyComp::arch_get_by_index (ARCHETYPE_INDEX idx, int propIndex, IProperty **prop)
{
	GENRESULT result = GR_INVALID_PARMS;
	PArchetype *arch = get_archetype (idx);
	if (arch && prop != NULL)
	{
		int hashIndex = arch->props.get_ith_index (propIndex);
		if (hashIndex  != -1)
		{
			if (arch->props.getProperty(hashIndex, *prop))
			{
				// We add a reference here, expecting the caller to release it when they are done.
				(*prop)->AddRef();
				result = GR_OK;
			}
		}
	}
	return result;
}

GENRESULT COMAPI PropertyComp::arch_set_by_index (ARCHETYPE_INDEX idx, int propIndex, IProperty *prop)
{
	GENRESULT result = GR_INVALID_PARMS;
	PArchetype *arch = get_archetype (idx);
	if (arch && prop != NULL)
	{
		int hashIndex = arch->props.get_ith_index (propIndex);
		if (hashIndex != -1)
		{
			char *name = arch->props.getKey (hashIndex);
			if (name)
			{
				// Release the reference to any existing property with this name.
				IProperty *oldProp;

				if (arch->props.getProperty(hashIndex, oldProp))
				{
					oldProp->Release();
				}
				arch->props.set(name, prop);
				prop->AddRef();
				result = GR_OK;
			}
		}
	}
	return result;
}

bool COMAPI PropertyComp::arch_del_by_index (ARCHETYPE_INDEX idx, int propIndex)
{
	bool result = 0;
	PArchetype *arch = get_archetype (idx);
	if (arch)
	{
		int hashIndex = arch->props.get_ith_index (propIndex);
		if (hashIndex != -1)
		{
			char *name = arch->props.getKey (hashIndex);
			if (name)
			{
				// Release the reference to any existing property with this name.
				IProperty *oldProp;

				if (arch->props.getProperty(hashIndex, oldProp))
				{
					oldProp->Release();
				}
				arch->props.set(name, NULL);
				result = true;
			}
		}
	}
	return result;
}

// Local methods

bool PropertyComp::LoadFile (const char *fileName, void *&buffer, U32 &size, IFileSystem * parent)
{
	DAFILEDESC fdesc = fileName;
	HANDLE hFile = parent->OpenChild(&fdesc);
	bool result = false;

	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD highSize;
		DWORD fSize = (U32) parent->GetFileSize (hFile, &highSize);

		if (highSize == 0)
		{
			char *buf = new char[fSize];

			DWORD dwRead;

			if (parent->ReadFile(hFile, buf, fSize, &dwRead, 0) != 0)
			{
				if (dwRead == fSize)
				{
					buffer = buf;
					size = fSize;
					result = true;
				}
				else
				{
					GENERAL_TRACE_1 ("PropertyComps::LoadFile(): failed to read entire file.\n");
				}
			}
		}
		else
		{
			GENERAL_WARNING ("PropertyComps::LoadFile() does not support files larger than 4Gb.\n");
		}
		parent->CloseHandle(hFile);
	}

	return result;
}

//
// DLL Entry point
//

BOOL COMAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	IComponentFactory *server;

	switch (fdwReason)
	{
	//
	// DLL_PROCESS_ATTACH: Create object server component and register it 
	// with DACOM manager
	//
		case DLL_PROCESS_ATTACH:

			hInstance = hinstDLL;

			DA_HEAP_ACQUIRE_HEAP(HEAP);
			DA_HEAP_DEFINE_HEAP_MESSAGE(hinstDLL);

		// Create a factory for the IProperties component

			server = new DAComponentFactory2<DAComponentAggregate<PropertyComp>, SYSCONSUMERDESC> (interface_name);

			if (server == NULL)
			{
				break;
			}

			DACOM = DACOM_Acquire();

		// Register at object-renderer priority
			if (DACOM != NULL)
			{
				DACOM->RegisterComponent(server, interface_name, DACOM_NORMAL_PRIORITY);
			}

			server->Release();

		// Create a factory for IProperty components

			server = new DAComponentFactory<DAComponent<LocalProperty>, DACOMDESC>(prop_interface_name);

		// Register at normal priority
			if (DACOM != NULL)
			{
				DACOM->RegisterComponent(server, prop_interface_name, DACOM_NORMAL_PRIORITY);
			}

			server->Release();

			break;

	//
	// DLL_PROCESS_DETACH: Release DACOM manager instance
	//
		case DLL_PROCESS_DETACH:

			if (DACOM != NULL)
			{
				DACOM->Release();
				DACOM = NULL;
			}
			break;
	}

	return TRUE;
}
