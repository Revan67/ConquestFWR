// AssetBar

#include "stdafx.h"
#include "globals.h"

#include "AssetBar.h"
#include "DataList.h"
#include "MainFrm.h"
#include "DObjNames.h"

#include "cqTrace.h"
#include <GameTypes.h>
#include <EventSys.h>
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace Editor
{
	AssetData g_AssetData;

	const struct AssetData* GetSelectedAsset()
	{
		return &g_AssetData;
	}
}

enum IMAGE
{
	FolderOpen,
	FolderClosed,
	AsteroidField,
	BarrierField,
	NebulaField,
	MineField,
	Blackhole,
	PlanetOre,
	PlanetEarth,
	PlanetGas,
	PlanetSwamp,
	PlanetMoon,
	PlanetOther,
	PlatformTerran,
	PlatformMantis,
	PlatformCelerian,
	PlatformVyrium,
	SpaceshipTerran,
	SpaceshipMantis,
	SpaceshipCelerian,
	SpaceshipVyrium,
	TriggerRegion,
	TriggerPlayerStart,
	TriggerVariant,

	IMAGE_COUNT
};

//-----------------------------------------------------------------------------------------------------
// finds and fills out the asset bar by Object Class
//-----------------------------------------------------------------------------------------------------

struct FindByObjectClass : IArchetypeEnum
{
	OBJCLASS   clazz;
	CAssetBar* assetBar;
	HTREEITEM  parent;
	int        imageIndex;

	FindByObjectClass( OBJCLASS _clazz, CAssetBar* _this, HTREEITEM _parent, int _imageIndex )
	{
		clazz      = _clazz;
		assetBar   = _this;
		parent     = _parent;
		imageIndex = _imageIndex;
	}

	HTREEITEM insert( const char* _name, const char* _folder, int _imageIndex )
	{
		if( _folder )
		{
			HTREEITEM folder = assetBar->m_treeView.GetChildItem(parent);
			while( folder )
			{
				if( assetBar->m_treeView.GetItemText(folder) == _folder )
				{
					break;
				}
				folder = assetBar->m_treeView.GetNextSiblingItem(folder);
			}

			// create new folder (if needed)
			if( folder == NULL )
			{
				int folderImage = _imageIndex;
				int folderSelectedImage = _imageIndex;

				if( assetBar->m_treeView.GetItemImage(parent,folderImage,folderSelectedImage) )
				{
					folder = assetBar->m_treeView.InsertItem( _folder, folderImage, folderSelectedImage, parent );
				}
				else
				{
					folder = assetBar->m_treeView.InsertItem( _folder, _imageIndex, _imageIndex, parent );
				}
			}

			return assetBar->m_treeView.InsertItem( _name, _imageIndex, _imageIndex, folder );
		}
		else
		{
			return assetBar->m_treeView.InsertItem( _name, _imageIndex, _imageIndex, parent );
		}

		return 0;
	}

	virtual	BOOL32 ArchetypeEnum (const char * name, void *data, U32 size, DWORD context)
	{
		BASIC_DATA* base = (BASIC_DATA*)data;

		if( base->objClass == clazz )
		{
			HTREEITEM item = 0;

			if( clazz == OC_SPACESHIP )
			{
				BASE_SPACESHIP_DATA * shipData = (BASE_SPACESHIP_DATA *)data;

				const char* race = "UNKNOWN";
				int index = imageIndex;
				switch( shipData->techActive.raceID )
				{
					case M_TERRAN:   race = "TERRAN";   index = SpaceshipTerran;   break;
					case M_MANTIS:   race = "MANTIS";   index = SpaceshipMantis;   break;
					case M_SOLARIAN: race = "SOLARIAN"; index = SpaceshipCelerian;   break;
					case M_VYRIUM:   race = "VYRIUM";   index = SpaceshipVyrium;   break;
				}

				item = insert( name, race, index );
			}
			else if( clazz == OC_PLATFORM )
			{
				BASE_PLATFORM_DATA * platData = (BASE_PLATFORM_DATA *) data;

				const char* race = "UNKNOWN";
				int index = imageIndex;
				switch( platData->techActive.raceID )
				{
					case M_TERRAN:   race = "TERRAN";   index = PlatformTerran;   break;
					case M_MANTIS:   race = "MANTIS";   index = PlatformMantis;   break;
					case M_SOLARIAN: race = "SOLARIAN"; index = PlatformCelerian; break;
					case M_VYRIUM:   race = "VYRIUM";   index = PlatformVyrium;   break;
				}
				item = insert( name, race, index );
			}
			else if( clazz == OC_PLANETOID )
			{
				BT_PLANET_DATA * planetData = (BT_PLANET_DATA *) data;
				const char* type = "UNKNOWN";
				int index = imageIndex;

				if( planetData->bMoon )
				{
					type  = "MOON"; 
					index = PlanetMoon;
				}
				else if( planetData->bUncommon )
				{
					type  = "UNCOMMON";
					index = PlanetOther;
				}
				else
				{
					switch( planetData->planetType )
					{
						case BT_PLANET_DATA::M_CLASS:      type = "EARTH"; index = PlanetEarth;   break;
						case BT_PLANET_DATA::METAL_PLANET: type = "METAL"; index = PlanetOre;   break;
						case BT_PLANET_DATA::GAS_PLANET:   type = "GAS";   index = PlanetGas;   break;
						case BT_PLANET_DATA::OTHER_PLANET: type = "SWAMP"; index = PlanetSwamp;   break;
					}
				}
				item = insert( name, type, index );
			}
			else if( clazz == OC_FIELD )
			{
				BASE_FIELD_DATA* fieldData = (BASE_FIELD_DATA*) data;

				if( fieldData->fieldClass == FC_ASTEROIDFIELD && context == FC_ASTEROIDFIELD )
				{
					item = assetBar->m_treeView.InsertItem( name, imageIndex, imageIndex, parent );
				}
				else if( fieldData->fieldClass == FC_ANTIMATTER && context == FC_ANTIMATTER )
				{
					item = assetBar->m_treeView.InsertItem( name, imageIndex, imageIndex, parent );
				}
			}
			else
			{
				item = assetBar->m_treeView.InsertItem( name, imageIndex, imageIndex, parent );
			}

			// save off the archetype ID
			if( item )
			{
				AssetData data;
				data.isParent = false;
				data.archID   = GAMETYPES->GetArchetypeDataID(name);
				assetBar->m_treeView.SetItemData( item, assetBar->m_assetArray.Add(data) );
			}
		}

		return true;
	}
};

/////////////////////////////////////////////////////////////////////////////
// AssetBar

BEGIN_MESSAGE_MAP(CAssetBar, CSidebar)
	//{{AFX_MSG_MAP(CAssetBar)
	ON_WM_CREATE()
    ON_WM_RBUTTONDOWN()
	ON_WM_ACTIVATE()
	ON_WM_LBUTTONUP()
	ON_WM_PARENTNOTIFY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static HICON s_Images[IMAGE_COUNT];

/////////////////////////////////////////////////////////////////////////////
// message handlers

int CAssetBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CSidebar::OnCreate(lpCreateStruct) == -1)
	{
		return -1;
	}

	// load icons
	s_Images[FolderOpen]		 = ::ExtractIcon( ::AfxGetApp()->m_hInstance, "res\\icons\\folder_open.ico", 0 );
	s_Images[FolderClosed]		 = ::ExtractIcon( ::AfxGetApp()->m_hInstance, "res\\icons\\folder_close.ico", 0 );
	s_Images[AsteroidField]		 = ::ExtractIcon( ::AfxGetApp()->m_hInstance, "res\\icons\\asteroid_field.ico", 0 );
	s_Images[BarrierField]		 = ::ExtractIcon( ::AfxGetApp()->m_hInstance, "res\\icons\\barrier_field.ico", 0 );
	s_Images[NebulaField]		 = ::ExtractIcon( ::AfxGetApp()->m_hInstance, "res\\icons\\nebula_field.ico", 0 );
	s_Images[MineField]			 = ::ExtractIcon( ::AfxGetApp()->m_hInstance, "res\\icons\\mine_field.ico", 0 );
	s_Images[Blackhole]			 = ::ExtractIcon( ::AfxGetApp()->m_hInstance, "res\\icons\\blackhole.ico", 0 );
	s_Images[PlanetOre]			 = ::ExtractIcon( ::AfxGetApp()->m_hInstance, "res\\icons\\ore_planet.ico", 0 );
	s_Images[PlanetEarth]		 = ::ExtractIcon( ::AfxGetApp()->m_hInstance, "res\\icons\\earth_planet.ico", 0 );
	s_Images[PlanetGas]			 = ::ExtractIcon( ::AfxGetApp()->m_hInstance, "res\\icons\\gas_planet.ico", 0 );
	s_Images[PlanetSwamp]		 = ::ExtractIcon( ::AfxGetApp()->m_hInstance, "res\\icons\\swamp_planet.ico", 0 );
	s_Images[PlanetMoon]	     = ::ExtractIcon( ::AfxGetApp()->m_hInstance, "res\\icons\\moon_planet.ico", 0 );
	s_Images[PlanetOther]	     = ::ExtractIcon( ::AfxGetApp()->m_hInstance, "res\\icons\\moon_planet.ico", 0 );
	s_Images[PlatformTerran]	 = ::ExtractIcon( ::AfxGetApp()->m_hInstance, "res\\icons\\terran_plats.ico", 0 );
	s_Images[PlatformMantis]	 = ::ExtractIcon( ::AfxGetApp()->m_hInstance, "res\\icons\\mantis_plats.ico", 0 );
	s_Images[PlatformCelerian]	 = ::ExtractIcon( ::AfxGetApp()->m_hInstance, "res\\icons\\cel_plats.ico", 0 );
	s_Images[PlatformVyrium]	 = ::ExtractIcon( ::AfxGetApp()->m_hInstance, "res\\icons\\vyrium_plats.ico", 0 );
	s_Images[SpaceshipTerran]	 = ::ExtractIcon( ::AfxGetApp()->m_hInstance, "res\\icons\\terran_ships.ico", 0 );
	s_Images[SpaceshipMantis]	 = ::ExtractIcon( ::AfxGetApp()->m_hInstance, "res\\icons\\mantis_ships.ico", 0 );
	s_Images[SpaceshipCelerian]	 = ::ExtractIcon( ::AfxGetApp()->m_hInstance, "res\\icons\\cel_ships.ico", 0 );
	s_Images[SpaceshipVyrium]	 = ::ExtractIcon( ::AfxGetApp()->m_hInstance, "res\\icons\\vyrium_ships.ico", 0 );
	s_Images[TriggerRegion]		 = ::ExtractIcon( ::AfxGetApp()->m_hInstance, "res\\icons\\trigger_region.ico", 0 );
	s_Images[TriggerPlayerStart] = ::ExtractIcon( ::AfxGetApp()->m_hInstance, "res\\icons\\trigger_playerstart.ico", 0 );
	s_Images[TriggerVariant]	 = ::ExtractIcon( ::AfxGetApp()->m_hInstance, "res\\icons\\trigger_variant.ico", 0 );



	if( !m_imageList.Create( 16, 16, ILC_MASK | ILC_COLOR32, 0, 0) )
	{
		// error
		return -1;
	}
	m_imageList.SetImageCount( IMAGE_COUNT );
	m_imageList.Replace( FolderClosed,       s_Images[FolderClosed] );
	m_imageList.Replace( FolderOpen,         s_Images[FolderOpen] );
	m_imageList.Replace( AsteroidField,		 s_Images[AsteroidField] );
	m_imageList.Replace( BarrierField,		 s_Images[BarrierField] );
	m_imageList.Replace( NebulaField,		 s_Images[NebulaField] );
	m_imageList.Replace( MineField,			 s_Images[MineField] );
	m_imageList.Replace( Blackhole,			 s_Images[Blackhole] );
	m_imageList.Replace( PlanetOre,			 s_Images[PlanetOre] );
	m_imageList.Replace( PlanetEarth,		 s_Images[PlanetEarth] );
	m_imageList.Replace( PlanetGas,			 s_Images[PlanetGas] );
	m_imageList.Replace( PlanetSwamp,		 s_Images[PlanetSwamp] );
	m_imageList.Replace( PlanetMoon,		 s_Images[PlanetMoon] );
	m_imageList.Replace( PlanetOther,		 s_Images[PlanetOther] );
	m_imageList.Replace( PlatformTerran,	 s_Images[PlatformTerran] );
	m_imageList.Replace( PlatformMantis,	 s_Images[PlatformMantis] );
	m_imageList.Replace( PlatformCelerian,	 s_Images[PlatformCelerian] );
	m_imageList.Replace( PlatformVyrium,	 s_Images[PlatformVyrium] );
	m_imageList.Replace( SpaceshipTerran,	 s_Images[SpaceshipTerran] );
	m_imageList.Replace( SpaceshipMantis,	 s_Images[SpaceshipMantis] );
	m_imageList.Replace( SpaceshipCelerian,	 s_Images[SpaceshipCelerian] );
	m_imageList.Replace( SpaceshipVyrium,	 s_Images[SpaceshipVyrium] );
	m_imageList.Replace( TriggerRegion,		 s_Images[TriggerRegion] );
	m_imageList.Replace( TriggerPlayerStart, s_Images[TriggerPlayerStart] );
	m_imageList.Replace( TriggerVariant,	 s_Images[TriggerVariant] );

	DWORD id = 'ASTB';
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT;

	if( !m_treeView.Create( dwStyle, CRect(0,0,0,0), this, id ) )
	{
		return -1;
	}
	m_treeView.SetImageList( &m_imageList, TVSIL_NORMAL );

	return 0;
}

void CAssetBar::OnRButtonDown(UINT nFlags, CPoint point)
{
	CSidebar::OnRButtonDown(nFlags, point);
}

//-----------------------------------------------------------------------------------------------------

CAssetBar::~CAssetBar()
{
}

//-----------------------------------------------------------------------------------------------------

int CAssetBar::DoPaint( CPaintDC& )
{
	return -1;
}

//-----------------------------------------------------------------------------------------------------

void CAssetBar::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized) 
{
	CSidebar::OnActivate(nState, pWndOther, bMinimized);
}

//-----------------------------------------------------------------------------------------------------

bool CAssetBar::Reset()
{
	m_assetArray.RemoveAll();
	m_assetArray.SetSize( 1024 * 4 );

	// add a dummy asset
	m_assetArray.Add( AssetData() );

	loadObjects();
	loadTriggers();
	loadPlanets();
	loadFields();
	loadObstructions();
//	loadCameras();

	m_assetArray.FreeExtra();

	m_treeView.SortChildren(NULL);

	return true;
}

//-----------------------------------------------------------------------------------------------------

void CAssetBar::loadSystemKits()
{
	HTREEITEM item = m_treeView.InsertItem( "System Kits", FolderClosed, FolderOpen);
	m_treeView.InsertItem( "System One", 1, 1, item );
}

//-----------------------------------------------------------------------------------------------------

void CAssetBar::loadObjects()
{
	HTREEITEM parent;

	// OC_SPACESHIP all the other space ships

	parent = m_treeView.InsertItem( "Space Ships", FolderClosed, FolderOpen );
	FindByObjectClass enumSpaceShips( OC_SPACESHIP, this, parent, FolderClosed );
	GAMETYPES->EnumerateArchetypeData( &enumSpaceShips, (DWORD)this );

	// OC_PLATFORM platforms

	parent = m_treeView.InsertItem( "Platforms", FolderClosed, FolderOpen );
	FindByObjectClass enumPlatforms( OC_PLATFORM, this, parent, FolderClosed );
	GAMETYPES->EnumerateArchetypeData( &enumPlatforms, (DWORD)this );
}

//-----------------------------------------------------------------------------------------------------

void CAssetBar::loadTriggers()
{
	HTREEITEM item = m_treeView.InsertItem( "Triggers", FolderClosed, FolderOpen );

	// OC_TRIGGER (not sure what this is...)

	HTREEITEM parent = m_treeView.InsertItem( "Region", FolderClosed, FolderOpen, item);
	FindByObjectClass enumTriggers( OC_TRIGGER, this, parent, TriggerRegion );
	GAMETYPES->EnumerateArchetypeData( &enumTriggers, (DWORD)this );

	// OC_PLAYERBOMB player start points

	parent = m_treeView.InsertItem( "Player Start", FolderClosed, FolderOpen, item );
	FindByObjectClass enumPlayerStarts( OC_PLAYERBOMB, this, parent, TriggerPlayerStart );
	GAMETYPES->EnumerateArchetypeData( &enumPlayerStarts, (DWORD)this );

	// OC_SCRIPTOBJECT 

	parent = m_treeView.InsertItem( "Script Objects", FolderClosed, FolderOpen, item );
	FindByObjectClass enumScriptObjects( OC_SCRIPTOBJECT, this, parent, TriggerVariant );
	GAMETYPES->EnumerateArchetypeData( &enumScriptObjects, (DWORD)this );

	// OC_WAYPOINT

	parent = m_treeView.InsertItem( "Waypoints", FolderClosed, FolderOpen, item );
	FindByObjectClass enumWaypoints( OC_WAYPOINT, this, parent, TriggerVariant );
	GAMETYPES->EnumerateArchetypeData( &enumWaypoints, (DWORD)this );
}

//-----------------------------------------------------------------------------------------------------

void CAssetBar::loadPlanets()
{
	HTREEITEM item = m_treeView.InsertItem( "Planets", FolderClosed, FolderOpen );

	// OC_PLANETOID

	HTREEITEM parent = item;
	FindByObjectClass enumPlanets( OC_PLANETOID, this, parent, FolderClosed );
	GAMETYPES->EnumerateArchetypeData( &enumPlanets, (DWORD)this );
}

//-----------------------------------------------------------------------------------------------------

void CAssetBar::loadFields()
{
	HTREEITEM item = m_treeView.InsertItem( "Fields", FolderClosed, FolderOpen );

	// OC_FIELD (asteroids & antimatter)

	HTREEITEM parent = m_treeView.InsertItem( "Asteroids", FolderClosed, FolderOpen, item );
	FindByObjectClass enumafields( OC_FIELD, this, parent, AsteroidField );
	GAMETYPES->EnumerateArchetypeData( &enumafields, FC_ASTEROIDFIELD );

	parent = m_treeView.InsertItem( "Barriers", FolderClosed, FolderOpen, item );
	FindByObjectClass enumbarriers( OC_FIELD, this, parent, BarrierField );
	GAMETYPES->EnumerateArchetypeData( &enumbarriers, FC_ANTIMATTER );

	// OC_NEBULA

	parent = m_treeView.InsertItem( "Nebulas", FolderClosed, FolderOpen, item );
	FindByObjectClass enumNebulas( OC_NEBULA, this, parent, NebulaField );
	GAMETYPES->EnumerateArchetypeData( &enumNebulas, (DWORD)this );

	// OC_MINEFIELD

	parent = m_treeView.InsertItem( "Minefield", FolderClosed, FolderOpen, item );
	FindByObjectClass enumMinefields( OC_MINEFIELD, this, parent, MineField );
	GAMETYPES->EnumerateArchetypeData( &enumMinefields, (DWORD)this );
}

//-----------------------------------------------------------------------------------------------------

void CAssetBar::loadLights()
{
	HTREEITEM item = m_treeView.InsertItem( "Lights", FolderClosed, FolderOpen );

	// OC_LIGHT

	HTREEITEM parent = item;
	FindByObjectClass enumLights( OC_LIGHT, this, parent, FolderClosed );
	GAMETYPES->EnumerateArchetypeData( &enumLights, (DWORD)this );
}

//-----------------------------------------------------------------------------------------------------

void CAssetBar::loadObstructions()
{
	HTREEITEM item = m_treeView.InsertItem( "Obstructions", FolderClosed, FolderOpen );

	// OC_BLACKHOLE

	HTREEITEM parent = m_treeView.InsertItem( "Black Holes", FolderClosed, FolderOpen, item );
	FindByObjectClass enumOC_BLACKHOLE( OC_BLACKHOLE, this, parent, Blackhole );
	GAMETYPES->EnumerateArchetypeData( &enumOC_BLACKHOLE, (DWORD)this );
}

//-----------------------------------------------------------------------------------------------------

void CAssetBar::loadCameras()
{
	HTREEITEM item = m_treeView.InsertItem( "Cameras", FolderClosed, FolderOpen );

	// OC_MOVIECAMERA

	HTREEITEM parent = item;
	FindByObjectClass enumCams( OC_MOVIECAMERA, this, parent, FolderClosed );
	GAMETYPES->EnumerateArchetypeData( &enumCams, (DWORD)this );
}

//-----------------------------------------------------------------------------------------------------

void CAssetBar::OnLButtonUp(UINT nFlags, CPoint point) 
{
	CSidebar::OnLButtonUp(nFlags, point);
}

//-----------------------------------------------------------------------------------------------------

void CAssetBar::OnParentNotify(UINT message, LPARAM lParam) 
{
	CSidebar::OnParentNotify(message, lParam);
	
	if( message == WM_LBUTTONDOWN )
	{
		CPoint p(lParam);
		HTREEITEM item = m_treeView.HitTest(p);
		if( item )
		{
			DWORD arrayIdx = m_treeView.GetItemData(item);
			if( arrayIdx )
			{
				Editor::g_AssetData = m_assetArray.GetAt(arrayIdx);

				if( EVENTSYS )
				{
					EVENTSYS->Send(CQE_ASSET_CHANGE);
				}
			}
		}
	}
}

/*
	System Kits

	Objects
		Ships
		Flagships
		Combative
		Non Combative
		Platforms

	Triggers
		Sound
		Region
		Waypoints
		Player Starting Points

	Planets
		Earth
		Gas
		Metal
		Swamp

	Fields
		Asteroid
		Gas
		Mines
		Debris & Nuggets
		Nebula

	Lights
		Point
		Omni

	Obstructions
		Antimatter
		Black Holes

	Cameras
		Normal

	Unknown
*/
