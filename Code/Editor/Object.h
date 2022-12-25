//
// Object.h
//

#ifndef OBJECT_HEADER_H
#define OBJECT_HEADER_H

#include <list>
class Transform;
class TiXmlNode;

struct IObject : IDAComponent
{
	// the few get methods

	virtual Transform& GetTransform() = 0;

	virtual UniqueID GetID() = 0;

	virtual void GetObjectData( struct ObjectData& _data ) = 0;

	// the many set methods

	virtual bool SetTransform( Transform& _xform ) = 0;

	virtual void SetSystemID( U32 _systemID ) = 0;

	virtual void SetPlayerID( U8 _playerID ) = 0;

	virtual void SetStringHandle( U32 _stringHandle ) = 0;

	virtual void SetDataOverride( struct MISSION_DATA_OVERRIDE& _missionDataOverride ) = 0;

	virtual void SetScriptHandle( const char* _scriptHandle ) = 0;

	// custom editor data

	virtual bool  SetCustomData( void* _data, int _dataSize ) = 0;

	virtual void* GetCustomData( int* _dataSize ) = 0;

	virtual bool  AddTab( HWND _tabCtrl ) = 0;

	virtual void  ResetData( void ) = 0;

	// operations

	virtual void Render() = 0;

	virtual void Delete() = 0;
};

//----------------------------------------------------------------------------------------------

struct ObjectList : std::list<IObject*>
{
	IObject* Find( UniqueID _uid );

	IObject* FindByHandle( const char* scriptHandle, IObject * lastObj = NULL );

	void Render();

	bool Delete(IObject*);

	bool Detach(IObject*);

	bool ValidatePlacement( IObject* _object );

	virtual ~ObjectList();

	void Save( TiXmlNode& _node );
	void Load( TiXmlNode& _node );
};

//----------------------------------------------------------------------------------------------

struct ObjectQuickList : ObjectList
{
	virtual ~ObjectQuickList()
	{
		// does not delete object pointers
		clear();
	}

	void Render()
	{
		// no rendering
	}

	bool Delete(struct IObject* _object )
	{ 
		for( ObjectList::iterator it = begin(); it != end(); it++ )
		{
			if( *it == _object )
			{
				erase(it);
				return true;
			}
		}
		return false;
	}

	bool ValidatePlacement( IObject* _object )
	{ 
		// no placement validation
		return false;
	}

	// can not Save or Load a quick list
	void Save( TiXmlNode& _node ){}
	void Load( TiXmlNode& _node ){}
};

//----------------------------------------------------------------------------------------------

struct IObjectSelection : IDAComponent
{
	virtual bool Add( IObject* _object ) = 0;

	virtual bool Remove( IObject* _object ) = 0;

	virtual bool AddInArea( ObjectList& _objectList, Vector& _upLeft, Vector& _downRight ) = 0;

	virtual bool GetList( ObjectQuickList& _list ) = 0;

	virtual bool Reset( void ) = 0;

	virtual bool HasObjects( void ) = 0;
};

//----------------------------------------------------------------------------------------------
// Object "globals"

struct BASIC_DATA;

// from IAnim.h
typedef int SCRIPT_SET_ARCH;
typedef int SCRIPT_INST;

namespace Object
{
	struct OBJECT_DACOMDESC : public DACOMDESC
	{
		const C8 * archname;
		
		OBJECT_DACOMDESC( const C8 * _archname )
		{
			archname = _archname;
			interface_name = "IDAComponent";
			size = sizeof(*this);
		}
	};

	// loads and creates an object instances
	IObject* Create( const char* _archetypeName );

	bool Delete( IObject* );

	extern UniqueID nextUniqueID;

	// loads & manages archetype data
	struct ArchetypeData
	{
		BASIC_DATA*     basicData;
		ARCHETYPE_INDEX meshIndex;
		SCRIPT_SET_ARCH animIndex;
		float           radius;
		U32             ref;
		CString         archname;
	};

	ArchetypeData* getArchetypeData( const char* _archname, const char* _meshname, BASIC_DATA* _basicData );
}

#endif