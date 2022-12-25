//
// ObjectFamily.h
//

#ifndef OBJECT_FAMILY_HEADER_H
#define OBJECT_FAMILY_HEADER_H

struct IObject;
struct IScenario;

struct IObjectFamilyEnum
{
	struct FamilyInfo
	{
		const char* family;
		bool selected;
		DWORD context;
	};

	struct ObjectInfo
	{
		IObject* object;
		const char* family;
		DWORD context;
	};

	virtual void EnumFamilyInfo( FamilyInfo& _info ) = 0;
	virtual void EnumObjectInfo( ObjectInfo& _info ) = 0;
};

struct IObjectFamily : IDAComponent
{
	struct Settings
	{
		CString familyName;
	};

	virtual bool AddFamily( const char* _familyName ) = 0;

	virtual bool RemoveFamily( const char* _familyName ) = 0;

	virtual bool AddObjectToFamily( const char* _familyName, IObject* _object ) = 0;

	virtual bool RemoveObjectFromFamily( const char* _familyName, IObject* _object ) = 0;

	virtual bool RemoveObjectFromFamily( const char* _familyName, const char* _objectName ) = 0;

	virtual bool EnumFamilyList( IObjectFamilyEnum& _enum, DWORD _context = 0 ) = 0;

	virtual bool EnumObjectFamilyInfo( IObjectFamilyEnum& _enum, IObject* _object, DWORD _context = 0 ) = 0;

	virtual bool EnumObjectsInFamily( IObjectFamilyEnum& _enum, const char* _familyName, DWORD _context = 0 ) = 0;

	virtual bool Prepare( IScenario* ) = 0;

	virtual bool SelectGroupForEditor( const char* _familyName ) = 0;
};

#endif