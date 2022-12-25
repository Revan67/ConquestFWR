//
// Object.cpp
//

#include "stdafx.h"
#include "globals.h"

#include <map>
#include <string>
#include <afxadv.h>

#include "Camera.h"
#include "Object.h"
#include "DataList.h"
#include "CQTrace.h"
#include "SystemStructs.h"
#include "tinyxml\tinyxml.h"
#include "ExportImport.h"
#include "TRect.h"
#include "Editor.h"
#include "Clipboard.h"

#include <GameTypes.h>
#include <FileSys.h>
#include <TSmartPointer.h>
#include <ITextureLibrary.h>
#include <Renderer.h>
#include <IAnim.h>
#include <ICamera.h>
#include <IMesh.h>
#include <TComponent.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace Object
{
	typedef std::map<std::string,ArchetypeData> ARCHLIST;

	struct GridPosition : public GRIDVECTOR
	{
		GRIDVECTOR& set( Vector& vec )
		{
			GRIDVECTOR* This = (GRIDVECTOR*)this;
			*This = vec;
			return *This;
		}

		void fixup()
		{
			if( size == 1)
			{
				x -= 1;
				y -= 1;
			}
			else if( size == 2 )
			{
				cornerpos();
			}
			else if( size == 4 )
			{
				size *= 2; // planets take up a lot of space

				x -= size;
				y -= size;
			}
		}

		SINGLE get_x() { return (SINGLE)x; }
		SINGLE get_y() { return (SINGLE)y; }

		void set_x(U8 _x) { x = _x; }
		void set_y(U8 _y) { y = _y; }

		U32 size;
		Transform xform;
		OBJCLASS objectClazz;

		GridPosition() : size(0)
		{
			set( Vector(0,0,0) );
			objectClazz = OC_NONE;
		}
	};

	UniqueID nextUniqueID = 1;
	ARCHLIST g_ArchList;

	bool arch_callback( ARCHETYPE_INDEX parent_arch_index, ARCHETYPE_INDEX child_arch_index, void *user_data )
	{
		float *rad = (float *)user_data;
		float fp_radius;

		float local_box[6];
		REND->get_archetype_bounding_box(child_arch_index,1.0,local_box);
		fp_radius = __max(local_box[BBOX_MAX_X],-local_box[BBOX_MIN_X]),
		fp_radius = __max(fp_radius,local_box[BBOX_MAX_Z]);
		fp_radius = __max(fp_radius,-local_box[BBOX_MIN_Z]);
		fp_radius *= 2;
		*rad = __max(fp_radius,*rad);

		return true;
	}

	ArchetypeData* getArchetypeData( const char* _archname, const char* _meshname, BASIC_DATA* _basicData )
	{
		ARCHLIST::iterator aIt = g_ArchList.find(_archname);
		if( aIt != g_ArchList.end() )
		{
			return &aIt->second;
		}

		ARCHETYPE_INDEX meshIndex;
		SCRIPT_SET_ARCH animIndex;
		float           radius;

		DAFILEDESC fdesc = _meshname;
		COMPTR<IFileSystem> objFile;

		if (OBJECTDIR->CreateInstance(&fdesc, objFile) == GR_OK)
		{
			TEXLIB->load_library(objFile, 0);
		}
		else
		{
			CQFILENOTFOUND(_meshname);
			return NULL;
		}

		if( (meshIndex = ENGINE->create_archetype(fdesc.lpFileName, objFile)) == INVALID_ARCHETYPE_INDEX )
		{
			return NULL;
		}

		// try to determine the ship's footprint size for usage by ObjGen
		if (ENGINE->is_archetype_compound(meshIndex))
		{
			ENGINE->enumerate_archetype_parts(meshIndex,Object::arch_callback,&radius);
		}
		else
		{
			float local_box[6];
			REND->get_archetype_bounding_box(meshIndex,1.0,local_box);
			radius = __max(local_box[BBOX_MAX_X],-local_box[BBOX_MIN_X]);
			radius = __max(radius,local_box[BBOX_MAX_Z]);
			radius = __max(radius,-local_box[BBOX_MIN_Z]);
			radius *= 2;
		}

		animIndex = ANIM->create_script_set_arch(objFile);

		ArchetypeData d;
		d.animIndex = animIndex;
		d.basicData = _basicData;
		d.meshIndex = meshIndex;
		d.radius    = radius;
		d.ref       = 1;
		d.archname  = _archname;

		g_ArchList[_archname] = d;
		return getArchetypeData( _archname, 0, 0 );
	}

	GridPosition getObjectGrid( IObject* _object )
	{
		ObjectData data;
		_object->GetObjectData( data );

		GridPosition g;
		g.xform = data.xform;
		g.objectClazz = data.objectClass;
		g.set( data.xform.translation );

		if( data.gridSize == CPoint(0,0) )
		{
			g.size = 0;
		}
		else if( data.gridSize == CPoint(1,1) )
		{
			g.size = 1;
		}
		else if( data.gridSize == CPoint(2,2) )
		{
			g.size = 2;
		}
		else if( data.gridSize == CPoint(4,4) )
		{
			g.size = 4;
		}

		return g;
	}

	bool isValidPlace( GridPosition& gridOne, GridPosition& gridTwo )
	{
		if( gridOne.objectClazz == OC_FIELD && gridTwo.objectClazz == OC_FIELD )
		{
			return( gridOne.isMostlyEqual(gridTwo) == false );
		}
		else if( gridOne.objectClazz == OC_FIELD || gridTwo.objectClazz == OC_FIELD )
		{
			return true;
		}

		if( gridOne.size < 4 && gridTwo.size < 4 )
		{
			if( gridOne.size > 1 && gridOne.isMostlyEqual(gridTwo) )
			{
				// trying to put a big object next to either a small object or a big object
				return false;
			}
			else if( gridOne.size == 1 )
			{
				if( gridTwo.size > 1 && gridTwo.isMostlyEqual(gridOne) )
				{
					// trying to put a small ship on a big ship
					return false;
				}
				else if( gridOne == gridTwo )
				{
					// trying to put a small ship on a small ship
					return false;
				}
			}
		}
		else
		{
			gridOne.fixup();
			gridTwo.fixup();

			FRect rectOne, rectTwo;

			float gridOneStep = 2.0f;
			float gridTwoStep = 2.0f;

			if( gridOne.objectClazz == OC_SPACESHIP && gridTwo.objectClazz == OC_PLANETOID )
			{
				gridTwoStep = 1.0f;
				gridTwo.set_x( gridTwo.get_x() + 4.0f );
				gridTwo.set_y( gridTwo.get_y() + 4.0f );
			}

			float oneSize = (float)gridOne.size * gridOneStep;
			rectOne.UpperLeftCorner.X  = gridOne.get_x();
			rectOne.UpperLeftCorner.Y  = gridOne.get_y();
			rectOne.LowerRightCorner.X = gridOne.get_x() + oneSize;
			rectOne.LowerRightCorner.Y = gridOne.get_y() + oneSize;

			float twoSize = (float)gridTwo.size * gridTwoStep;
			rectTwo.UpperLeftCorner.X  = gridTwo.get_x();
			rectTwo.UpperLeftCorner.Y  = gridTwo.get_y();
			rectTwo.LowerRightCorner.X = gridTwo.get_x() + twoSize;
			rectTwo.LowerRightCorner.Y = gridTwo.get_y() + twoSize;

			if( rectOne.isRectCollided(rectTwo) || rectTwo.isRectCollided(rectOne) )
			{
				return false;
			}
		}

		return true;
	}

	//----------------------------------------------------------------------------------------------
	// Object Factory
	//----------------------------------------------------------------------------------------------

	IObject* loadGameType( const char* _archetypeName )
	{
		BASIC_DATA* pData = (BASIC_DATA*)GAMETYPES->GetArchetypeData( _archetypeName );

		switch( pData->objClass )
		{
			case OC_SPACESHIP:
			case OC_PLANETOID:
			case OC_PLATFORM:
			case OC_BLACKHOLE:
			{
				OBJECT_DACOMDESC dacomDesc( _archetypeName );
				dacomDesc.interface_name = "SpaceObject";
               
				void* spaceObject = NULL;
				if( DACOM->CreateInstance(&dacomDesc,&spaceObject) == GR_OK )
				{
					return (IObject*)spaceObject;
				}
				break;
			}

			case OC_JUMPGATE:
			{
				// these are really wormholes
				OBJECT_DACOMDESC dacomDesc( _archetypeName );
				dacomDesc.interface_name = "WormholeObject";
               
				void* wormhole = NULL;
				if( DACOM->CreateInstance(&dacomDesc,&wormhole) == GR_OK )
				{
					return (IObject*)wormhole;
				}
				break;
			}

			case OC_MINEFIELD:
			case OC_NEBULA:
			case OC_FIELD:
			{
				OBJECT_DACOMDESC dacomDesc( _archetypeName );
				dacomDesc.interface_name = "FieldObject";
               
				void* fieldBlock = NULL;
				if( DACOM->CreateInstance(&dacomDesc,&fieldBlock) == GR_OK )
				{
					return (IObject*)fieldBlock;
				}
				break;
			}

			case OC_WAYPOINT:
			case OC_PLAYERBOMB:
			case OC_TRIGGER:
			case OC_SCRIPTOBJECT:
			{
				OBJECT_DACOMDESC dacomDesc( _archetypeName );
				dacomDesc.interface_name = "TriggerObject";
               
				void* trigger = NULL;
				if( DACOM->CreateInstance(&dacomDesc,&trigger) == GR_OK )
				{
					return (IObject*)trigger;
				}
				break;
			}

			case OC_NONE:
			case OC_MEXPLODE:
			case OC_SHRAPNEL:
			case OC_LAUNCHER:
			case OC_WEAPON:
			case OC_BLAST:
			case OC_FIGHTER:
			case OC_LIGHT:
			case OC_TRAIL:
			case OC_EFFECT:
			case OC_NUGGET:
			case OC_GROUP:
			case OC_RESEARCH:
			case OC_BUILDRING:
			case OC_BUILDOBJ:
			case OC_MOVIECAMERA:
			case OC_UI_ANIM:
				break;
		}

		return NULL;
	}

	IObject* loadGenData( const char* _archetypeName )
	{
		// unimplemented (may never need to be)
		return NULL;
	}

	IObject* Create( const char* _archetypeName )
	{
		U32 gameTypesId = GAMETYPES->GetArchetypeDataID(_archetypeName);
		
		if( gameTypesId != 0 )
		{
			return loadGameType(_archetypeName);
		}

		U32 genTypeId = GENDATA->GetArchetypeDataID(_archetypeName);

		if( genTypeId )
		{
			return loadGenData( _archetypeName ); 
		}

		return NULL;
	}
};



//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
// Object List implmentation
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------

IObject* ObjectList::Find( UniqueID _uid )
{
	if( size() )
	{
		for( iterator it = begin(); it != end(); it++ )
		{
			if( (*it)->GetID() == _uid )
			{
				return *it;
			}
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------------------------------

IObject* ObjectList::FindByHandle( const char* _scriptHandle, IObject * _lastObj )
{
	ObjectData data;

	if( size() )
	{
		for( iterator it = begin(); it != end(); it++ )
		{
			(*it)->GetObjectData(data);

			if( data.scriptHandle == _scriptHandle )
			{
				return *it;
			}
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------------------------------

bool ObjectList::Delete( IObject* _object )
{
	if( Detach(_object) )
	{
		_object->Delete();
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------------------------------

bool ObjectList::Detach(IObject* _object)
{
	if( size() )
	{
		for( iterator it = begin(); it != end(); it++ )
		{
			if( *it == _object )
			{
				// erase from list
				erase(it);
				return true;
			}
		}
	}
	return false;
}

//-----------------------------------------------------------------------------------------------------

void ObjectList::Save( TiXmlNode& _node )
{
	TiXmlElement objectListNode("OBJECTLIST");

	for( iterator it = begin(); it != end(); it++ )
	{
		IObject* obj = *it;

		COMPTR<ISaverLoader> saver;
		if( obj->QueryInterface("ISaverLoader", saver) == GR_OK )
		{
			saver->Save( objectListNode );
		}
	}

	_node.InsertEndChild(objectListNode);
}

//-----------------------------------------------------------------------------------------------------

void ObjectList::Load( TiXmlNode& _node )
{
	TiXmlElement* objectListNode = _node.FirstChildElement("OBJECTLIST");

	if( objectListNode )
	{
		TiXmlElement* childNode = objectListNode->FirstChildElement("OBJECT");
		while( childNode )
		{
			IObject* obj = NULL;

			if( childNode->Attribute("SpaceObject") )
			{
				obj = Object::Create( childNode->Attribute("SpaceObject") );
			}
			else if( childNode->Attribute("FieldObject") )
			{
				obj = Object::Create( childNode->Attribute("FieldObject") );
			}
			else if( childNode->Attribute("TriggerObject") )
			{
				obj = Object::Create( childNode->Attribute("TriggerObject") );
			}

			if( obj )
			{
				COMPTR<ISaverLoader> loader;
				if( obj->QueryInterface("ISaverLoader", loader) == GR_OK )
				{
					if( loader->Load(*childNode) )
					{
						push_back( obj );
					}
				}
			}

			childNode = childNode->NextSiblingElement();
		}
	}
}

//-----------------------------------------------------------------------------------------------------

bool ObjectList::ValidatePlacement( IObject* _object )
{
	ObjectData data;
	_object->GetObjectData( data );

	System* currentSystem = Editor::GetActiveSystem();
	if( currentSystem && currentSystem->terrainMap != NULL )
	{
		NETGRIDVECTOR grid;
		grid.init( data.xform.translation, currentSystem->id );

		if( !currentSystem->terrainMap->IsGridInSystem(grid) )
		{
			return false;
		}
	}

	BASIC_DATA* basicData = (BASIC_DATA*)GAMETYPES->GetArchetypeData( data.archetype );

	if( basicData )
	{
		if( basicData->objClass == OC_PLATFORM )
		{
			BASE_PLATFORM_DATA* basePlatformData = (BASE_PLATFORM_DATA*)basicData;

			// is this a Jump Gate (that goes around wormholes)?
			if( !data.bJumpGate )
			{
				return true;
			}

			// for any "JumpGate" objects, need to be ontop of a wormhole (and not already have an object there)

			Object::GridPosition grid = Object::getObjectGrid( _object );
			grid.size = basePlatformData->size;

			for( iterator it = begin(); it != end(); it++ )
			{
				if( *it != _object )
				{
					Object::GridPosition objGrid = Object::getObjectGrid(*it);

					if( !Object::isValidPlace(grid,objGrid) )
					{
						return false;
					}
				}
			}
		}
		else
		{
			Object::GridPosition grid = Object::getObjectGrid( _object );

			for( iterator it = begin(); it != end(); it++ )
			{
				if( *it != _object )
				{
					Object::GridPosition objGrid = Object::getObjectGrid(*it);

					if( !Object::isValidPlace(grid,objGrid) )
					{
						return false;
					}
				}
			}
		}
	}

	return true;
}

//-----------------------------------------------------------------------------------------------------

ObjectList::~ObjectList()
{
	if( size() )
	{
		// release resources
		for( iterator it = begin(); it != end(); it++ )
		{
			(*it)->Delete();
		}

		// release pointers
		for( it = begin(); it != end(); it++ )
		{
			IObject* obj = (*it);
			delete obj;
		}

		// release the map
		clear();
	}
}

//-----------------------------------------------------------------------------------------------------
// ObjectSelection
//-----------------------------------------------------------------------------------------------------

struct ObjectSelection : IObjectSelection, IClipboardObject
{
	BEGIN_DACOM_MAP_INBOUND(ObjectSelection)
		DACOM_INTERFACE_ENTRY(IObjectSelection)
		DACOM_INTERFACE_ENTRY(IClipboardObject)
	END_DACOM_MAP()

	// IObjectSelection

	virtual bool Add( IObject* _object );
	virtual bool Remove( IObject* _object );
	virtual bool AddInArea( ObjectList& _objectList, Vector& _upLeft, Vector& _downRight );
	virtual bool GetList( ObjectQuickList& _list );
	virtual bool Reset( void );
	virtual bool HasObjects( void );

	// IClipboardObject
	virtual const char* GetType();
	virtual bool Copy( CSharedFile& _memfile );
	virtual bool Paste( CSharedFile& _memfile );
	virtual bool Append( CSharedFile& _memfile );

	// locals

	ObjectQuickList m_objectList;
};

//----------------------------------------------------------------------------------------------

bool ObjectSelection::Add( IObject* _object )
{
	if( !_object )
	{
		return false;
	}

	for( ObjectList::iterator it = m_objectList.begin(); it != m_objectList.end(); it++ )
	{
		if( *it == _object )
		{
			return false;
		}
	}

	m_objectList.push_back( _object );
	return true;
}

//----------------------------------------------------------------------------------------------

bool ObjectSelection::Remove( IObject* _object )
{
	for( ObjectList::iterator it = m_objectList.begin(); it != m_objectList.end(); it++ )
	{
		if( *it == _object )
		{
			m_objectList.erase(it);
			return true;
		}
	}

	return false;
}

//----------------------------------------------------------------------------------------------

bool ObjectSelection::AddInArea( ObjectList& _objectList, Vector& _upLeft, Vector& _downRight )
{
	bool bFoundSome = false;

	for( ObjectList::iterator it = _objectList.begin(); it != _objectList.end(); it++ )
	{
		IObject* obj = *it;

		ObjectData data;
		obj->GetObjectData(data);

		if( data.xform.translation.x >= _upLeft.x && data.xform.translation.x <= _downRight.x )
		{
			if( data.xform.translation.y >= _upLeft.y && data.xform.translation.y <= _downRight.y )
			{
				bFoundSome = Add( obj );
			}
		}
	}

	return bFoundSome;
}

//----------------------------------------------------------------------------------------------

bool ObjectSelection::GetList( ObjectQuickList& _list )
{
	// this list should be clear
	_list.clear();

	for( ObjectList::iterator it = m_objectList.begin(); it != m_objectList.end(); it++ )
	{
		_list.push_back( *it );
	}

	return( _list.size() > 0 );
}

//----------------------------------------------------------------------------------------------

bool ObjectSelection::Reset( void )
{
	m_objectList.clear();
	return true;
}

//----------------------------------------------------------------------------------------------

bool ObjectSelection::HasObjects( void )
{
	return( m_objectList.size() > 0 );
}

//----------------------------------------------------------------------------------------------

const char* ObjectSelection::GetType()
{
	return "ObjectSelection";
}

//----------------------------------------------------------------------------------------------

bool ObjectSelection::Copy( CSharedFile& _memfile )
{
	bool bValidCopy = false;

	for( ObjectList::iterator it = m_objectList.begin(); it != m_objectList.end(); it++ )
	{
		COMPTR<IClipboardObject> clip;

		IObject* obj = *it;
		if( obj && obj->QueryInterface("IClipboardObject",clip) == GR_OK )
		{
			// record the start position of mem file
			DWORD startFile = _memfile.GetPosition();
			DWORD fileSize = 0;

			// reserve a dword for file size
			_memfile.Write( &fileSize, sizeof(DWORD) );

			// write down the object's type
			char nullChar = 0;
			const char* clipType = clip->GetType();
			_memfile.Write( clipType, strlen(clipType) ); 
			_memfile.Write( &nullChar, sizeof(char) );

			// copy & record the end of this record
			clip->Copy( _memfile );
			fileSize = _memfile.GetPosition() - startFile;

			// go back to beginning to record the file size
			_memfile.Seek( startFile, CFile::SeekPosition::begin );
			_memfile.Write( &fileSize, sizeof(DWORD) );
			_memfile.Seek( 0, CFile::SeekPosition::end );

			// this is a valid clipboard instance
			bValidCopy = true;
		}
	}

	return bValidCopy;
}

//----------------------------------------------------------------------------------------------

bool ObjectSelection::Paste( CSharedFile& _memfile )
{
	if( CLIPBOARD->HasDataForType(GetType()) )
	{
		int bytesLeft = _memfile.GetLength() - _memfile.GetPosition();
		while( bytesLeft > sizeof(DWORD) )
		{
			// get filesize
			DWORD fileSize = 0;
			if( _memfile.Read(&fileSize, sizeof(DWORD)) < sizeof(DWORD) )
			{
				break;
			}
			CQASSERT( fileSize < (DWORD)bytesLeft );

			if( fileSize )
			{
				// get object type
				CString objectType;

				char ch;
				_memfile.Read(&ch,1);
				while( ch )
				{
					objectType += ch;
					_memfile.Read(&ch,1);
				}

				// adjust for size of object type identifier, plus NULL terminator, plus the DWORD size number
				fileSize -= objectType.GetLength() + sizeof(char) + sizeof(DWORD);

				// read this object's data
				void* buffer = alloca(fileSize);
				_memfile.Read( buffer, fileSize );

				// prepare the "file" to paste with
				CSharedFile objectFile;
				objectFile.Write(buffer,fileSize);
				objectFile.SeekToBegin();

				// try to make any kind of valid object
				Object::OBJECT_DACOMDESC dacomDesc("");
				dacomDesc.interface_name = objectType;
		    
				void* object = NULL;
				if( DACOM->CreateInstance(&dacomDesc,&object) == GR_OK )
				{
					if( object )
					{
						IObject* obj = (IObject*)object;

						COMPTR<IClipboardObject> clip;
						if( obj->QueryInterface("IClipboardObject",clip) == GR_OK )
						{
							if( clip->Paste(objectFile) )
							{
								Add( obj );

								// temporarily add to the system's object list until end of paste, so that it can be tested against for placement
								System* currentSystem = Editor::GetActiveSystem();
								if( currentSystem )
								{
									currentSystem->objectList.push_back( obj );
								}
							}
						}
					}
				}
			}
		}
	}

	// detach objects from current system
	if( HasObjects() )
	{
		System* currentSystem = Editor::GetActiveSystem();
		if( currentSystem )
		{
			for( ObjectList::iterator it = m_objectList.begin(); it != m_objectList.end(); it++ )
			{
				currentSystem->objectList.Detach( *it );
			}
		}
	}

	return true;
}

//----------------------------------------------------------------------------------------------

bool ObjectSelection::Append( CSharedFile& _memfile )
{
	return 0;
}

//-----------------------------------------------------------------------------------------------------
// startup

#include "startup.h"

struct _ObjectSelection : GlobalComponent
{
	ObjectSelection * objectselection;

	virtual void Startup (void)
	{
		OBJECTSELECTION = objectselection = new DAComponent<ObjectSelection>;
		AddToGlobalCleanupList((IDAComponent **) &OBJECTSELECTION);
	}

	virtual void Initialize (void)
	{
	}
};
static _ObjectSelection __ObjectSelection;

