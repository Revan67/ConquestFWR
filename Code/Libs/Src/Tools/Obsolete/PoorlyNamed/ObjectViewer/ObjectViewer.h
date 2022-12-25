// ObjectViewer.h
//
//
//

#ifndef OBJECTVIEWER_H
#define OBJECTVIEWER_H

#define STRICT
#include <windows.h>
#include <comdef.h>
#include <afxres.h>

#include "resource.h"

#include "dacom.h"
#include "rendpipeline.h"

#include "MessageCrackers.h"
#include "DACOMProvider.h"

class App
{
public:
	BEGIN_STATIC_WP_MAPS(App)
		BEGIN_COMMAND_MAP
			// File 
			//
			ON_COMMAND(ID_FILE_NEW,OnFileNew)
			ON_COMMAND(ID_FILE_OPEN,OnFileOpen)
			ON_COMMAND(ID_FILE_INSERT,OnFileInsert)
			ON_COMMAND(ID_FILE_DEVICE,OnFileDevice)
			ON_COMMAND(ID_APP_EXIT,OnAppExit)

			// View 
			//
			ON_COMMAND(ID_VIEW_WORLDAXIS,OnViewWorldAxis)
			ON_COMMAND(ID_VIEW_FRAMERATE_COUNTERS,OnViewFrameRateCounters)
			ON_COMMAND(ID_VIEW_FRAMERATE_GRAPH,OnViewFrameRateGraph)
			ON_COMMAND(ID_VIEW_STATISTICS,OnViewStatistics)
			ON_COMMAND(ID_VIEW_ISTACK,OnViewIStack)
			ON_COMMAND(ID_VIEW_ISTACKHELP,OnViewIStackHelp)
			ON_COMMAND(ID_VIEW_TEXTURES,OnViewTextures)
			ON_COMMAND(ID_VIEW_RPMATERIALS,OnViewRPMaterials)
			
			// Object 
			//
			ON_COMMAND(ID_OBJECT_RELOAD,OnObjectReload)
			ON_COMMAND(ID_OBJECT_REMOVE,OnObjectRemove)
				// Object->View 
				ON_COMMAND(ID_OBJECT_VIEW_AXIS,OnObjectViewAxis)
				ON_COMMAND(ID_OBJECT_VIEW_WIREFRAME,OnObjectViewWireFrame)
				ON_COMMAND(ID_OBJECT_VIEW_SHADED,OnObjectViewShaded)
				ON_COMMAND(ID_OBJECT_VIEW_STATISTICS,OnObjectViewStatistics)
				ON_COMMAND(ID_OBJECT_VIEW_NAMES,OnObjectViewNames)
				ON_COMMAND(ID_OBJECT_VIEW_HARDPOINT_NAMES,OnObjectViewHardpointNames)
				ON_COMMAND(ID_OBJECT_VIEW_HARDPOINT_AXIS,OnObjectViewHardpointAxis)
				// Object->Materials
				ON_COMMAND(ID_OBJECT_MATERIALS_LIST,OnObjectMaterialsList)
				// Object->Animations
				ON_COMMAND(ID_OBJECT_VIEW_ANIMATIONS,OnObjectViewAnimations)
				// Object->Joints
				ON_COMMAND(ID_OBJECT_VIEW_JOINTS,OnObjectViewJoints)
				// Object->Hardpoints
				ON_COMMAND(ID_OBJECT_VIEW_HARDPOINTS,OnObjectViewHardpoints)
			ON_COMMAND(ID_OBJECT_PROPERTIES,OnObjectProperties)
			ON_COMMAND(ID_OBJECT_LIST,OnObjectList)

			// Light
			//
			ON_COMMAND(ID_LIGHTING_AMBIENTLIGHT,OnLightAmbient)
			ON_COMMAND(ID_LIGHT_LIST,OnLightList)

			// Camera
			//
			ON_COMMAND(ID_CAMERA_LIST,OnCameraList)

			// System
			//
			// System->RenderPipeline->Batching
			ON_COMMAND(ID_SYSTEM_RENDERPIPELINE_BATCHING_ENABLED,OnSystemRPBatchingEnable)
			ON_COMMAND(ID_SYSTEM_RENDERPIPELINE_BATCHING_LAZYSTATE,OnSystemRPBatchingLazyState)
			ON_COMMAND(ID_SYSTEM_RENDERPIPELINE_BATCHING_SAVEPROJECTION,OnSystemRPBatchingProjection)
			ON_COMMAND(ID_SYSTEM_RENDERPIPELINE_BATCHING_SAVEMODELVIEW,OnSystemRPBatchingModelview)
			ON_COMMAND(ID_SYSTEM_RENDERPIPELINE_BATCHING_SAVEVIEWPORT,OnSystemRPBatchingViewport)
			ON_COMMAND(ID_SYSTEM_RENDERPIPELINE_BATCHING_POOLSIZES,OnSystemRPBatchingPoolSizes)
			// System->RenderPipeline->Textures
			ON_COMMAND(ID_SYSTEM_RENDERPIPELINE_TEXTURE_ENABLED,OnSystemRPTextureEnable)
			ON_COMMAND(ID_SYSTEM_RENDERPIPELINE_TEXTURE_MANAGERSTRATEGY_MRU,OnSystemRPTextureManagerMRU)
			ON_COMMAND(ID_SYSTEM_RENDERPIPELINE_TEXTURE_MANAGERSTRATEGY_LRU,OnSystemRPTextureManagerLRU)
			// System->RenderPipeline->Profiling
			ON_COMMAND(ID_SYSTEM_RENDERPIPELINE_PROFILELOGGING,OnSystemRPProfileLogging)

			// Engine
			//
			ON_COMMAND(ID_ENGINE_OPTICS_LISTARCHETYPES,OnEngineOpticsViewArchetypes)
			ON_COMMAND(ID_ENGINE_POLYMESH_LISTARCHETYPES,OnEnginePolymeshViewArchetypes)


		END_COMMAND_MAP
		BEGIN_MESSAGE_MAP
			ON_MESSAGE(WM_DESTROY,OnDestroy)
			ON_MESSAGE_DEFAULT(OnDefault)
		END_MESSAGE_MAP
	END_STATIC_WP_MAPS

	// Message Handlers
	//
	
	// File
	//
	HRESULT OnFileNew( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnFileOpen( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnFileInsert( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnFileDevice( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnAppExit( UINT wID, HWND hControl, UINT NotifyCode );

	// View
	//
	HRESULT OnViewWorldAxis( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnViewFrameRateCounters( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnViewFrameRateGraph( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnViewStatistics( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnViewIStack( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnViewIStackHelp( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnViewTextures( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnViewMaterials( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnViewRPMaterials( UINT wID, HWND hControl, UINT NotifyCode );

	// Object 
	//
	HRESULT OnObjectReload( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnObjectRemove( UINT wID, HWND hControl, UINT NotifyCode );
	// Object->View 
	HRESULT OnObjectViewAxis( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnObjectViewWireFrame( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnObjectViewShaded( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnObjectViewStatistics( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnObjectViewNames( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnObjectViewHardpointNames( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnObjectViewHardpointAxis( UINT wID, HWND hControl, UINT NotifyCode );	

	// Object->Materials
	HRESULT OnObjectMaterialsList( UINT wID, HWND hControl, UINT NotifyCode );
	// Object->Animations
	HRESULT OnObjectViewAnimations( UINT wID, HWND hControl, UINT NotifyCode );
	// Object->Joints
	HRESULT OnObjectViewJoints( UINT wID, HWND hControl, UINT NotifyCode );
	// Object->Hardpoints
	HRESULT OnObjectViewHardpoints( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnObjectProperties( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnObjectList( UINT wID, HWND hControl, UINT NotifyCode );

	// Light
	//
	HRESULT OnLightAmbient( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnLightOrbit( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnLightList( UINT wID, HWND hControl, UINT NotifyCode );

	// Camera
	//
	HRESULT OnCameraOrbit( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnCameraList( UINT wID, HWND hControl, UINT NotifyCode );

	// Tests
	//
	HRESULT OnTestDPDIP( UINT wID, HWND hControl, UINT NotifyCode );

	// System
	//
	// System->RenderPipeline->Batching
	HRESULT OnSystemRPBatchingEnable( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnSystemRPBatchingLazyState( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnSystemRPBatchingProjection( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnSystemRPBatchingModelview( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnSystemRPBatchingViewport( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnSystemRPBatchingPoolSizes( UINT wID, HWND hControl, UINT NotifyCode );
	// System->RenderPipeline->Textures
	HRESULT OnSystemRPTextureEnable( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnSystemRPTextureManagerMRU( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnSystemRPTextureManagerLRU( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnSystemRPMaterials( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnSystemRPProfileLogging( UINT wID, HWND hControl, UINT NotifyCode );

	// Engine
	//
	// Engine->Optics
	HRESULT OnEngineOpticsViewArchetypes( UINT wID, HWND hControl, UINT NotifyCode );
	// Engine->PolyMesh
	HRESULT OnEnginePolymeshViewArchetypes( UINT wID, HWND hControl, UINT NotifyCode );
	// Engine->TxmLib
	HRESULT OnEngineTXMLibViewTextures( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnEngineTXMLibListTextures( UINT wID, HWND hControl, UINT NotifyCode );

	// Test
	//
	HRESULT OnTestDrawIndexedPrimitive( UINT wID, HWND hControl, UINT NotifyCode );

	// Windows message handlers
	//
	HRESULT OnDestroy( UINT message, WPARAM wParam, LPARAM lParam );
	HRESULT OnDefault( UINT message, WPARAM wParam, LPARAM lParam );
	HRESULT OnKeyDown( UINT message, WPARAM wParam, LPARAM lParam );

	// Dialog Procs
	//
	static BOOL CALLBACK ObjectList_DlgProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	// Initialization and Cleanup Code
	//
	HRESULT Initialize( HINSTANCE hInst, LPSTR lpCmdLine );
	HRESULT Cleanup();
	HRESULT ParseCommandLine( LPSTR lpCmdLine );
	HRESULT OnCreateWindow();
	HRESULT Create3DWindow();
	HRESULT OnDestroyWindow();
	HRESULT OnIdle();
	HRESULT MessagePump();

	App();
	~App();

protected:

	HRESULT App::CreatePopupWindow( const char *clsid, IDAComponent **out_win = NULL );

	void SetMenuItemCheck( U32 menu_item_id, bool checked );
	bool GetMenuItemCheck( U32 menu_item_id );

	void SetMenuItemEnable( U32 menu_item_id, bool enabled );
	bool GetMenuItemEnable( U32 menu_item_id );

	void AddMenuItem( HMENU hMenu, U32 menu_item_id, const char *caption, U32 new_menu_item_id );
	void RemoveMenuItem( HMENU hMenu, U32 menu_item_id );

protected:
	HINSTANCE			m_hInst;
	HWND				m_hWnd;
	CDACOMProvider	   *m_DACOM;


	IRenderPipeline	   *m_IRenderPipe;
	IDAComponent	   *m_3DWindow;

	U32					m_Width;
	U32					m_Height;
	U32					m_MouseWheelMsg;
};



#endif
