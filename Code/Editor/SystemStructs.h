//
// SystemStructs.h
//
// editor defs of objects contained in a sector
//
#ifndef SYSTEM_OBJECTS_H
#define SYSTEM_OBJECTS_H

#include <DSector.h>
#include <DQuickSave.h>
#include "TRect.h"
#include "SaveLoad.h"
#include "TerrainMap.h"
#include <TSmartPointer.H>
#include "object.h"
#include "GridVector.h"

typedef TRect<float> FRect;
typedef TPoint<float> FPoint;

enum OBJCLASS;

// basic object data info to save/load
struct ObjectData
{
	CString   archetype;
	Transform xform;
	U32       id;
	CPoint    gridSize;
	U32       slotsNeeded;
	CString   scriptHandle;
	U32       stringHandle;
	OBJCLASS  objectClass;
	bool      bJumpGate;
	U8        playerID;

	bool bUseDataOverride;
	MISSION_DATA_OVERRIDE dataOverride;
};

// describes a location of a wormhole in some System
struct JumpInfo
{
	// used to be in Jump

	JumpInfo() : x(0), y(0), startX(0), startY(0), bJumpAllowed(true), archname("JGATE!!Jumpgate"), parentJumpGate(NULL), wormholeObject(NULL)
	{
		partName = "partname";
	}

	// TRUE system coordinates
	S32 x, y, startX, startY;

	CString  archname;
	BOOL32   bJumpAllowed;
	M_STRING partName;
	IObject* parentJumpGate;
	IObject* wormholeObject;
};

// describes a jump point between systems
struct JumpPoint : public JumpInfo
{
	DWORD  id;             // ID for this jump point with inside this system
	DWORD  destSystemID;   // system ID that owns this point
	DWORD  destWormholeID; // jump ID in that sytem this connects to
	CPoint cPoint;         // in the system (absolute)
	FPoint fPoint;         // in the system (normalized)

	JumpPoint() : id(0xff), destSystemID(0xff), destWormholeID(0xff){}
};

struct JList : public CArray<JumpPoint,JumpPoint>
{
	JumpPoint* FindByJumpIdx( DWORD _wormholeID )
	{
		for( int i = 0; i < GetCount(); i++ )
		{
			JumpPoint& point = ElementAt(i);

			if( point.id == _wormholeID )
			{
				return &point;
			}
		}
		return NULL;
	}
};

// this will contain the system kit, gate links, and an object list
struct System
{
	bool          bEmpty;     // is this system entry begin used?
	CRect         cRect;      // system rect (absolute)
	FRect         fRect;      // system rect (normalized)
	JList         jList;      // jump point list
	CString       name;       // name of system
	DWORD         id;         // uniqued system id (within a Sector)
	GT_SYSTEM_KIT systemKit;  // system kit data (filled out at RunTime)
	ObjectList    objectList; // list of objects in this system
	DWORD         nameID;     // the string table ID of this system's name
	CAMERA_DATA   cameraData; // the current data location and orientation

	// id and other database info (archetype names)
	CString systemKitName;
	CString backgroundName;

	// absolute Sector coords
	S32 x, y, sizeX, sizeY;

	// in System coords (where the camera starts)
	S32 startX, startY;

	COMPTR<ITerrainMap> terrainMap;

	System();
	~System(); // do not inheret this struct!

	JumpPoint* getJumpPoint( DWORD _uid )
	{
		for( int i = 0; i < jList.GetSize(); i++ )
		{
			if( jList[i].id == _uid )
			{
				return &jList[i];
			}
		}
		return NULL;
	}

	DWORD getNumObjects()
	{
		return objectList.size() + jList.GetSize();
	}

	IObject* find( UniqueID _uid );

	bool prepareForEditing( void );
	bool refresh(void);
	bool prepareForSaving( float _maxSectorSize, float _maxSystemSize );

	void updateWormholes();
	void updateSystemName();

	bool Save( class TiXmlNode& );
	bool Load( class TiXmlNode& );

	bool Save( struct IFileSystem& );
	bool Load( struct IFileSystem& );
};


struct ISector : public IDAComponent
{
	// settings

	struct Settings
	{
		wchar_t  name[128];
		FILETIME lastSaved;
	};

	virtual bool SetSettings( Settings& ) = 0;

	virtual Settings& GetSettings( void ) = 0;

	// operations

	virtual System* GetActiveSystem() = 0;

	virtual System* NewSystem( const wchar_t* _name ) = 0;

	virtual bool DeleteSystem( struct System* _system ) = 0;

	virtual System* FindSystemByIdx( U32 _sysIdx ) = 0;

	virtual System* FindSystemByName( const wchar_t* _name ) = 0;

	virtual void Render( void ) = 0;

	// from conquest app (for camera)

	virtual BOOL32 SetCurrentSystem (U32 _systemID) = 0;

	virtual U32 GetCurrentSystem (void) const = 0;

	virtual BOOL32 GetSystemRect (U32 SystemID, struct tagRECT * rect,bool bAbsolute) const = 0;

	virtual void GetDefaultSystemSize (S32 &_sizeX,S32 &_sizeY) = 0;

	virtual BOOL32 GetSectorCenter (S32 *x, S32 *y) const = 0;

	virtual struct GT_SYSTEM_KIT GetSystemLightKit(U32 systemID) = 0;

	virtual void GetSystemName(wchar_t * nameBuffer, U32 nameBufferrSize, U32 systemID) = 0;

	virtual void SetSystemName(U32 systemID, U32 stringID) = 0;

	virtual void SetLightingKit(U32 systemID, char * lightingKit) = 0;

	virtual int GetNumSystems() = 0;

	// factory

	static ISector* New();
	static bool     Delete( ISector* );
};

struct OpenGridVector : NETGRIDVECTOR
{
	U8 x;
	U8 y;
	U8 systemID;
};

#endif