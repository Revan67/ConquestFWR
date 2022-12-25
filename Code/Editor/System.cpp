//--------------------------------------------------------------------------//
//                                                                          //
//                                System.cpp                                //
//                                                                          //
// implments the SYSTEM component with is like the SysMap from CQ           //
//                                                                          //
//--------------------------------------------------------------------------//
 
#include "stdafx.h"
#include "globals.h"

#include "SystemStructs.h"
#include "CQTrace.h"
#include "tinyxml\tinyxml.h"
#include "Object.h"
#include "ExportImport.h"
#include "Campaign.h"
#include "Scenario.h"
#include "Editor.h"
#include "StringTable.h"
#include "TerrainMap.h"
#include "FieldManager.h"
#include "Camera.h"

#include <TSmartPointer.H>
#include <FileSys.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//-----------------------------------------------------------------------------------------------------

bool System_ExportJumpGate( System& _system, IFileSystem& _fileSystem, int _nJumpGate )
{
	if (_fileSystem.SetCurrentDirectory("JGATE!!Jumpgate") == 0)
	{
		_fileSystem.CreateDirectory("JGATE!!Jumpgate");
		if (_fileSystem.SetCurrentDirectory("JGATE!!Jumpgate") == 0)
		{
			return false;
		}
	}

	if (_fileSystem.SetCurrentDirectory("MT_QJGATELOAD") == 0)
	{
		_fileSystem.CreateDirectory("MT_QJGATELOAD");
		if (_fileSystem.SetCurrentDirectory("MT_QJGATELOAD") == 0)
		{
			return false;
		}
	}

	// prepare data

	JumpPoint& jumpPoint = _system.jList[_nJumpGate];

	jumpPoint.x = jumpPoint.fPoint.X * _system.sizeX;
	jumpPoint.y = jumpPoint.fPoint.Y * _system.sizeY;

	MT_QJGATELOAD gate;
	ZeroMemory(&gate,sizeof(gate));

	gate.gate_id	  = jumpPoint.id;           // ID of gate
	gate.exit_gate_id = jumpPoint.destWormholeID; // ID of gate where this gate exits
	gate.bJumpAllowed = (jumpPoint.bJumpAllowed != false);

	Vector pos( jumpPoint.x, jumpPoint.y, 0 );
	gate.pos.init( pos, _system.id );

	// write out data

	CString gateName;
	gateName.Format("Gate%d", _system.jList[_nJumpGate].id);

	DAFILEDESC fdesc = gateName;
	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	COMPTR<IFileSystem> file;
	if (_fileSystem.CreateInstance(&fdesc, file) != GR_OK)
	{
		_fileSystem.GetLastError();
		return false;
	}

	// record the gate
	DWORD dwWritten;
	file->WriteFile(0, &gate, sizeof(gate), &dwWritten);

	_fileSystem.SetCurrentDirectory("..");
	_fileSystem.SetCurrentDirectory("..");

	return true;
}

//-----------------------------------------------------------------------------------------------------

bool System_RemoveObject( System& _system, IObject* _object )
{
	for( ObjectList::iterator it = _system.objectList.begin(); it != _system.objectList.end(); it++ )
	{
		IObject* obj = *it;
		if( obj == _object )
		{
			// this removes but does not DELETE the object!
			_system.objectList.erase(it);
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------------------------------

System::System() : bEmpty(true), id(0xff), x(0), y(0), sizeX(0), sizeY(0), startX(0), startY(0), nameID(0)
{
	ZeroMemory( &systemKit, sizeof(systemKit) );
	ZeroMemory( &cameraData, sizeof(cameraData) );

	name           = "name";
	systemKitName  = "new1";
	backgroundName = "backgrounds\\bkg_system01.3db";
}

//-----------------------------------------------------------------------------------------------------

System::~System()
{
}

//----------------------------------------------------------------------------------------------

bool System::prepareForEditing( void )
{
	bool ret = false;

	if( CAMPAIGN && CAMPAIGN->GetCurrentScenario() && CAMPAIGN->GetCurrentScenario()->GetActiveSector() )
	{
		const char* kitname = systemKitName;
		CAMPAIGN->GetCurrentScenario()->GetActiveSector()->SetLightingKit(id, (char*)kitname );
	}

	// set up default camera if wonky
	if( CAMERA && !cameraData.FOV_x && !cameraData.FOV_y && !cameraData.version )
	{
		CAMERA->SetCameraDefaults(cameraData);
		CAMERA->GetStateInfo(&cameraData);
	}
	else
	{
		CAMERA->SetStateInfo(&cameraData, true);
	}

	for( int i = 0; i < jList.GetCount(); i++ )
	{
		JumpPoint& point = jList.ElementAt(i);

		if( point.wormholeObject == NULL )
		{
			point.wormholeObject = Object::Create(point.archname);
		}

		if( point.wormholeObject )
		{
			point.wormholeObject->SetSystemID( id );

			Vector pos;
			pos.x = point.fPoint.X * sizeX;
			pos.y = point.fPoint.Y * sizeY;
			pos.z = 0;

			NETGRIDVECTOR grid;
			grid.init( pos, id );
			grid.centerpos();

			TRANSFORM xform;
			xform.translation.x = grid.getX() * GRIDSIZE;
			xform.translation.y = grid.getY() * GRIDSIZE;
			xform.rotate_about_i( CQ2EDToRadian(90) );

			point.wormholeObject->SetTransform( xform );
			ret = true;
		}
	}

	updateWormholes();
	updateSystemName();

	if( terrainMap == NULL )
	{
		ret = CreateTerrainMap(terrainMap);
		CQASSERT(terrainMap);
	}

	if( terrainMap )
	{
		CRect rect( 0, 0, sizeX, sizeY );
		terrainMap->SetWorldRect( rect );
	}

	return ret;
}

//-----------------------------------------------------------------------------------------------------

bool System::refresh(void)
{
	CAMERA->GetStateInfo(&cameraData);
	return true;
}

//-----------------------------------------------------------------------------------------------------

bool System::prepareForSaving( float _maxSectorSize, float _maxSystemSize )
{
	// basic info
	x	  = fRect.UpperLeftCorner.X * _maxSectorSize;
	y	  = fRect.UpperLeftCorner.Y * _maxSectorSize;
	sizeX = fRect.getWidth() * _maxSectorSize;
	sizeY = fRect.getHeight() * _maxSectorSize;

	// guard against bad data
	if( sizeX > _maxSystemSize || sizeY > _maxSystemSize )
	{
		sizeX = _maxSystemSize;
		sizeY = _maxSystemSize;
	}

	// if no player bomb on map
	startX = sizeX / 2;
	startX = sizeY / 2;

	// set up default camera if wonky
	if( CAMERA && !cameraData.FOV_x && !cameraData.FOV_y && !cameraData.version )
	{
		CAMERA->SetCameraDefaults(cameraData);
		CAMERA->GetStateInfo(&cameraData);
	}

	return true;
}

//-----------------------------------------------------------------------------------------------------

IObject* System::find( UniqueID _uid )
{
	IObject* obj = objectList.Find(_uid);
	if( !obj )
	{
		for( int i = 0; i < jList.GetCount(); i++ )
		{
			JumpPoint& point = jList.ElementAt(i);

			if( point.wormholeObject && point.wormholeObject->GetID() == _uid )
			{
				obj = point.wormholeObject;
				break;
			}
		}
	}
	return obj;
}

//-----------------------------------------------------------------------------------------------------

void System::updateWormholes()
{
	for( int i = 0; i < jList.GetCount(); i++ )
	{
		JumpPoint& point = jList.ElementAt(i);

		if( point.wormholeObject == NULL )
		{
			IObject* jumpGate    = point.parentJumpGate;
			System*  jumpGateSys = this;

			// remove linked wormhole
			System* dstSys = Editor::GetSystem( 0, point.destSystemID );
			if( dstSys )
			{
				for( int j = 0; j < dstSys->jList.GetCount(); j++ )
				{
					JumpPoint& dstPoint = dstSys->jList.ElementAt(j);
					if( dstPoint.id == point.destWormholeID )
					{
						// record linked jump gate
						if( dstPoint.parentJumpGate )
							jumpGate = dstPoint.parentJumpGate;
						else
							jumpGateSys = dstSys;

						dstSys->jList.RemoveAt(j);
					}
				}
			}

			// remove jump gate, if any
			if( jumpGate && jumpGateSys && System_RemoveObject(*jumpGateSys,jumpGate) )
			{
				jumpGate->Delete();
			}

			// remove from this list
			jList.RemoveAt(i);

			// to start over the iterator
			updateWormholes();
			return;
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void System::updateSystemName()
{
	const wchar_t* widename = STRINGTABLE->GetStringByID( nameID );
	if( widename )
	{
		CString szName(widename);
		name = szName;
	}
}

//-----------------------------------------------------------------------------------------------------

bool System::Save( class TiXmlNode& _node ) 
{ 
	TiXmlElement system ("SYSTEM");
	system.SetAttribute( "name", CString(name) );
	system.SetAttribute( "bEmpty", bEmpty );
	system.SetAttribute( "systemKitName", systemKitName );
	system.SetAttribute( "backgroundName", backgroundName );
	system.SetAttribute( "x", x );
	system.SetAttribute( "y", y );
	system.SetAttribute( "sizeX", sizeX );
	system.SetAttribute( "sizeY", sizeY );
	system.SetAttribute( "startX", startX );
	system.SetAttribute( "startY", startY );

	Export::XML::rect( cRect, system );
	Export::XML::frect( fRect, system );
	Export::XML::jlist( jList, system );

	objectList.Save( system );

	return( _node.InsertEndChild(system) != NULL );
}

//-----------------------------------------------------------------------------------------------------

bool System::Load( class TiXmlNode& _node )
{ 
	TiXmlElement* system = _node.ToElement();
	if( !system )
	{
		return false;
	}

	int num;
	system->QueryIntAttribute("bEmpty",&num);
	bEmpty = num != false;

	wchar_t buffer[128];
	Import::XML::widestring(buffer,128,"name",system);
	name = buffer;

	system->QueryIntAttribute( "x", (int*)&x );
	system->QueryIntAttribute( "y", (int*)&y );
	system->QueryIntAttribute( "sizeX", (int*)&sizeX );
	system->QueryIntAttribute( "sizeY", (int*)&sizeY );
	system->QueryIntAttribute( "startX", (int*)&startX );
	system->QueryIntAttribute( "startY", (int*)&startY );

	if( system->Attribute("systemKitName") )
	{
		CString kit( system->Attribute("systemKitName") );
		systemKitName = kit;
	}

	if( system->Attribute("backgroundName") )
	{
		CString bkname( system->Attribute("backgroundName") );
		backgroundName = bkname;
	}

	Import::XML::rect( cRect, system->FirstChild("RECT") );
	Import::XML::frect( fRect, system->FirstChild("FRECT") );
	Import::XML::jlist( jList, system->FirstChild("JUMPPOINTLIST") );

	objectList.Load( *system );

	return true;
}

//-----------------------------------------------------------------------------------------------------

bool System::Save( struct IFileSystem& _fileSystem )
{ 
	// go through each gate and save it out

	for( int i = 0; i < jList.GetSize(); i++ )
	{
		System_ExportJumpGate( *this, _fileSystem, i );
	}

	// initially the system has NO fields to filter
	FIELDMANAGER->Reset(this);

	// go through all objects and save them out

	for( ObjectList::iterator it = objectList.begin(); it != objectList.end(); it++ )
	{
		IObject* obj = *it;

		COMPTR<ISaverLoader> saver;
		if( obj->QueryInterface("ISaverLoader", saver) == GR_OK )
		{
			saver->Save( _fileSystem );
		}
	}

	FIELDMANAGER->Save( _fileSystem );

	return true; 
}

//-----------------------------------------------------------------------------------------------------

bool System::Load( struct IFileSystem& )
{ 
	return false; 
}

