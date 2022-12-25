//
// ObjectFamily.cpp
//

#include "stdafx.h"
#include "globals.h"

#include "ObjectFamily.h"
#include "Object.h"
#include "SaveLoad.h"
#include "tinyxml\tinyxml.h"
#include "SystemStructs.h"
#include "Editor.h"
#include "CQTrace.h"
#include "campaign.h"
#include "Scenario.h"

#include <TComponent.h>
#include <TSmartPointer.h>
#include <IConnection.h>
#include <Engine.h>
#include <EventSys.h>
#include <system.h>
#include <startup.h>
#include <FileSys.h>

#include <list>
#include <map>
#include <string>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//----------------------------------------------------------------------------------------------

struct ObjectFamily : public IObjectFamily, ISaverLoader
{
	BEGIN_DACOM_MAP_INBOUND(ObjectFamily)
		DACOM_INTERFACE_ENTRY(IObjectFamily)
		DACOM_INTERFACE_ENTRY(ISaverLoader)
	END_DACOM_MAP()

	// IObjectFamily

	virtual bool AddFamily( const char* _familyName );
	virtual bool RemoveFamily( const char* _familyName );
	virtual bool AddObjectToFamily( const char* _familyName, IObject* _object );
	virtual bool RemoveObjectFromFamily( const char* _familyName, IObject* _object );
	virtual bool RemoveObjectFromFamily( const char* _familyName, const char* _objectName );
	virtual bool EnumFamilyList( IObjectFamilyEnum& _enum, DWORD _context );
	virtual bool EnumObjectFamilyInfo( IObjectFamilyEnum& _enum, IObject* _object, DWORD _context );
	virtual bool EnumObjectsInFamily( IObjectFamilyEnum& _enum, const char* _familyName, DWORD _context );
	virtual bool Prepare( IScenario* _scenario );
	virtual bool SelectGroupForEditor( const char* _familyName );

	// ISaverLoader

	virtual bool Save( class TiXmlNode& );
	virtual bool Load( class TiXmlNode& );
	virtual bool Save( struct IFileSystem& );
	virtual bool Load( struct IFileSystem& );

	// locals

	struct FamilyObject
	{
		CString objectFamilyName;
		CString objectScripHandle;
	};

	// needs to be the light weight version of the object list
	struct FamilyObjectList : public ObjectQuickList
	{
		bool selected;
		FamilyObjectList() : selected(false) {}
	};

	class FamilyList : public std::list<FamilyObject>{};
	class ObjectMap  : public std::map<std::string,FamilyObjectList>{};

	ObjectMap  m_ObjectMap;
	FamilyList m_FamilyList;
};


//----------------------------------------------------------------------------------------------

bool ObjectFamily::AddFamily( const char* _familyName )
{
	ObjectMap::iterator it = m_ObjectMap.find(_familyName);
	if( it == m_ObjectMap.end() )
	{
		FamilyObjectList olist;
		m_ObjectMap[ _familyName ] = olist;
	}
	return true;
}

//----------------------------------------------------------------------------------------------

bool ObjectFamily::RemoveFamily( const char* _familyName )
{
	ObjectMap::iterator it = m_ObjectMap.find(_familyName);
	if( it != m_ObjectMap.end() )
	{
		m_ObjectMap.erase( it );
		return true;
	}
	return false;
}

//----------------------------------------------------------------------------------------------

bool ObjectFamily::AddObjectToFamily( const char* _familyName, IObject* _object )
{
	ObjectMap::iterator it = m_ObjectMap.find(_familyName);
	if( it == m_ObjectMap.end() )
	{
		AddFamily( _familyName );
		return AddObjectToFamily( _familyName, _object );
	}

	// push onto object list
	ObjectQuickList& olist = it->second;
	olist.push_back( _object );
	return true;
}

//----------------------------------------------------------------------------------------------

bool ObjectFamily::RemoveObjectFromFamily( const char* _familyName, IObject* _object )
{
	if( _familyName == NULL )
	{
		// erasing from all families
		for( ObjectMap::iterator it = m_ObjectMap.begin(); it != m_ObjectMap.end(); it++ )
		{
			FamilyObjectList& olist = it->second;

			for( FamilyObjectList::iterator oit = olist.begin(); oit != olist.end(); )
			{
				if( _object == *oit )
				{
					oit = olist.erase(oit);
				}
				else
				{
					oit++;
				}
			}
		}
	}
	else
	{
		ObjectMap::iterator it = m_ObjectMap.find(_familyName);
		if( it != m_ObjectMap.end() )
		{
			FamilyObjectList& olist = it->second;

			for( FamilyObjectList::iterator oit = olist.begin(); oit != olist.end(); oit++ )
			{
				if( _object == *oit )
				{
					olist.erase(oit);
					return true;
				}
			}
		}
	}

	// did not find this object/family pair
	return false;
}

//----------------------------------------------------------------------------------------------

bool ObjectFamily::RemoveObjectFromFamily( const char* _familyName, const char* _objectName )
{
	if( _familyName == NULL )
	{
		// erasing from all families
		// TODO: implement this
		return false;
	}
	else
	{
		ObjectData data;

		ObjectMap::iterator it = m_ObjectMap.find(_familyName);
		if( it != m_ObjectMap.end() )
		{
			FamilyObjectList& olist = it->second;

			for( FamilyObjectList::iterator oit = olist.begin(); oit != olist.end(); oit++ )
			{
				(*oit)->GetObjectData(data);

				if( data.scriptHandle == _objectName )
				{
					olist.erase(oit);
					return true;
				}
			}
		}
	}

	// did not find this object/family pair
	return false;
}

//----------------------------------------------------------------------------------------------

bool ObjectFamily::EnumFamilyList( IObjectFamilyEnum& _enum, DWORD _context )
{
	IObjectFamilyEnum::FamilyInfo info;
	info.context = _context;

	for( ObjectMap::iterator it = m_ObjectMap.begin(); it != m_ObjectMap.end(); it++ )
	{
		info.family = it->first.c_str();
		info.selected = it->second.selected;
		_enum.EnumFamilyInfo( info );
	}

	return( m_ObjectMap.size() != 0 );
}

//----------------------------------------------------------------------------------------------

bool ObjectFamily::EnumObjectFamilyInfo( IObjectFamilyEnum& _enum, IObject* _object, DWORD _context )
{
	// send in an object pointer to filter out all other objects, and to get the families for the object
	// send in a NULL for _object to get all the objects in each family

	IObjectFamilyEnum::ObjectInfo info;
	info.context = _context;

	for( ObjectMap::iterator it = m_ObjectMap.begin(); it != m_ObjectMap.end(); it++ )
	{
		FamilyObjectList& olist = it->second;

		for( FamilyObjectList::iterator oit = olist.begin(); oit != olist.end(); oit++ )
		{
			if( _object == NULL )
			{
				info.family = it->first.c_str();
				info.object = *oit;
				_enum.EnumObjectInfo( info );
			}
			else if( _object == *oit )
			{
				info.family = it->first.c_str();
				info.object = *oit;
				_enum.EnumObjectInfo( info );
			}
		}
	}

	return( m_ObjectMap.size() != 0 );
}

//----------------------------------------------------------------------------------------------

bool ObjectFamily::EnumObjectsInFamily( IObjectFamilyEnum& _enum, const char* _familyName, DWORD _context )
{
	IObjectFamilyEnum::ObjectInfo nfo;
	nfo.context = _context;
	nfo.family  = _familyName;

	ObjectMap::iterator it = m_ObjectMap.find(_familyName);
	if( it != m_ObjectMap.end() )
	{
		FamilyObjectList& olist = it->second;

		for( FamilyObjectList::iterator oit = olist.begin(); oit != olist.end(); oit++ )
		{
			nfo.object  = *oit;
			_enum.EnumObjectInfo( nfo );
		}

		return true;
	}

	return false;
}

//----------------------------------------------------------------------------------------------

bool ObjectFamily::Prepare( IScenario* _scenario )
{
	if( m_FamilyList.size() )
	{
		IScenario* s = _scenario;
		if( !s )
		{
			s = CAMPAIGN->GetCurrentScenario();
		}
		ASSERT(s);

		for( FamilyList::iterator it = m_FamilyList.begin(); it != m_FamilyList.end(); it++ )
		{
			FamilyObject& familyObject = *it;

			IObject* pObject = s->FindObjectByScriptHandle( familyObject.objectScripHandle );
			if( pObject )
			{
				AddObjectToFamily( familyObject.objectFamilyName, pObject );
			}
		}

		// only do this once!
		m_FamilyList.clear();
	}

	return 0;
}

//----------------------------------------------------------------------------------------------

bool ObjectFamily::SelectGroupForEditor( const char* _familyName )
{
	// need to unselect all families first
	for( ObjectMap::iterator it = m_ObjectMap.begin(); it != m_ObjectMap.end(); it++ )
	{
		it->second.selected = false;
	}

	// select "found family" (if any)
	it = m_ObjectMap.find(_familyName);
	if( it != m_ObjectMap.end() )
	{
		it->second.selected = true;
		return true;
	}

	return false;
}

//----------------------------------------------------------------------------------------------

bool ObjectFamily::Save( class TiXmlNode& _node )
{
	if( m_ObjectMap.size() )
	{
		for( ObjectMap::iterator it = m_ObjectMap.begin(); it != m_ObjectMap.end(); it++ )
		{
			FamilyObjectList& olist = it->second;

			TiXmlElement family("OBJECTFAMILY");
			family.SetAttribute( "name", it->first.c_str() );

			for( FamilyObjectList::iterator oit = olist.begin(); oit != olist.end(); oit++ )
			{
				TiXmlElement object("OBJECT");

				ObjectData data;
				(*oit)->GetObjectData(data);
				object.SetAttribute("scriptHandle", data.scriptHandle);

				family.InsertEndChild(object);
			}

			_node.InsertEndChild( family );
		}
		return true;
	}
	return false;
}

//----------------------------------------------------------------------------------------------

bool ObjectFamily::Load( class TiXmlNode& _node )
{
	TiXmlElement* family = _node.FirstChildElement("OBJECTFAMILY");
	while( family )
	{
		const char* familyName = family->Attribute("name");

		if( !familyName )
		{
			family = family->NextSiblingElement("OBJECTFAMILY");
			continue;
		}

		TiXmlElement* object = family->FirstChildElement("OBJECT");
		while( object )
		{
			const char* scriptHandle = object->Attribute("scriptHandle");
			if( scriptHandle )
			{
				FamilyObject fo;
				fo.objectFamilyName  = familyName;
				fo.objectScripHandle = scriptHandle;

				m_FamilyList.push_back( fo );
			}

			object = object->NextSiblingElement("OBJECT");
		}

		family = family->NextSiblingElement("OBJECTFAMILY");
	}
	return true;
}

//----------------------------------------------------------------------------------------------

bool ObjectFamily::Save( struct IFileSystem& _fs )
{
	if( m_ObjectMap.size() )
	{
		for( ObjectMap::iterator it = m_ObjectMap.begin(); it != m_ObjectMap.end(); it++ )
		{
			const char* familyName = it->first.c_str();

			FamilyObjectList& olist = it->second;

			_fs.CreateDirectory("\\ObjectGroups");
			if( _fs.SetCurrentDirectory("\\ObjectGroups") == 0 )
			{
				CQERROR0("QuickSave failed on Directory 'ObjectGroups'");
				return false;
			}

			// save off basic family info
			_fs.CreateDirectory(familyName);
			if( _fs.SetCurrentDirectory(familyName) != 0 )
			{
				MT_OBJECTFAMILY_QLOAD objectFamily;
				objectFamily.numObjects = olist.size();
				strncpy( objectFamily.name, familyName, countof(objectFamily.name)-1 );

				DAFILEDESC desc = familyName;
				desc.lpImplementation = "UTF";
				desc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
				desc.dwShareMode = 0;  // no sharing
				desc.dwCreationDistribution = CREATE_NEW;		// fail if file already exists

				DWORD dwWritten;
				HANDLE hChild = _fs.OpenChild(&desc);
				if( hChild != INVALID_HANDLE_VALUE )
				{
					_fs.WriteFile( hChild, &objectFamily, sizeof(objectFamily), &dwWritten );
					_fs.CloseHandle( hChild );
				}

				for( FamilyObjectList::iterator oit = olist.begin(); oit != olist.end(); oit++ )
				{
					ObjectData data;
					(*oit)->GetObjectData(data);

					MT_OBJECTFAMILYENTRY_QLOAD objectEntry;
					strncpy( objectEntry.scriptHandle, data.scriptHandle, countof(objectEntry.scriptHandle)-1 );

					DAFILEDESC desc = data.scriptHandle;
					desc.lpImplementation = "UTF";
					desc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
					desc.dwShareMode = 0;  // no sharing
					desc.dwCreationDistribution = CREATE_NEW;		// fail if file already exists

					DWORD dwWritten;
					HANDLE hChild = _fs.OpenChild(&desc);
					if( hChild != INVALID_HANDLE_VALUE )
					{
						_fs.WriteFile( hChild, &objectEntry, sizeof(objectEntry), &dwWritten );
						_fs.CloseHandle( hChild );
					}
				}

				_fs.SetCurrentDirectory("..");
			}

		}
		return true;
	}
	return false;
}

//----------------------------------------------------------------------------------------------

bool ObjectFamily::Load( struct IFileSystem& )
{
	return 0;
}


//-----------------------------------------------------------------------------------------------------
// startup

struct ObjectFamilyFactory : public IComponentFactory
{
	BEGIN_DACOM_MAP_INBOUND(ObjectFamilyFactory)
		DACOM_INTERFACE_ENTRY(IComponentFactory)
	END_DACOM_MAP()

	// IComponentFactory

	virtual GENRESULT COMAPI CreateInstance (DACOMDESC *descriptor, void **instance)
	{
		*instance = NULL;

		if( !strcmp(descriptor->interface_name,"ObjectFamily") )
		{
			ObjectFamily* objectFamily = new DAComponent<ObjectFamily>;

objectFamily->AddFamily("TestOne");
objectFamily->AddFamily("TestTwo");

			*instance = objectFamily;
			return GR_OK;
		}

		// something bad happended
		return GR_INTERFACE_UNSUPPORTED;
	}
};

struct _objectfamilyfactory : GlobalComponent
{
	ObjectFamilyFactory * factory;

	virtual void Startup (void)
	{
		factory = new DAComponent<ObjectFamilyFactory>;
		AddToGlobalCleanupList((IDAComponent **) &factory);
	}

	virtual void Initialize (void)
	{
		DACOM->RegisterComponent( factory, "ObjectFamily", 0);
	}
};

static _objectfamilyfactory  __objectfamilyfactory ;
