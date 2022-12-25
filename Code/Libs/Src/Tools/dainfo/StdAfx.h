// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__3CEC806D_82B9_48C5_85FF_3B7C431ED004__INCLUDED_)
#define AFX_STDAFX_H__3CEC806D_82B9_48C5_85FF_3B7C431ED004__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#include "DACOM.h"
#include "FDUMP.h"
#include "libver.h"
#include "RPUL.h"
#include "system.h"
#include "engine.h"

// for interface IID_*
#include "3dmath.h"					// IID_I3DMathEngine );
#include "DACOM.H"					// IID_IAggregateComponent) ;
#include "ENGINE.H"					// IID_IEngine ) ;
#include "FILESYS.H"				// IID_IFileSystem ) ;
#include "FontImage.h"				// IID_IFontImage ) ;
#include "IAnim.h"					// IID_IAnimation ) ;
#include "ICamera.h"				// IID_ICamera ) ;
#include "IChannel2.h"				// IID_IChannel2 ) ;
#include "IDDBackDoor.h"			// IID_IDDBackDoor) ;
#include "IDeformable.h"			// IID_IDeformable ) ;
#include "IDocClient.h"				// IID_IDocumentClient) ;
#include "IExtentContainer.h"		// IID_IExtentContainer  ) ;
#include "IGammaControl.h"			// IID_IGammaControl) ;
#include "IHardPoint.h"				// IID_IHardpoint ) ;
#include "ILight.h"					// IID_ILight ) ;
#include "IMaterial.h"				// IID_IMaterial ) ;
#include "IMaterialLibrary.h"		// IID_IMaterialLibrary ) ;
#include "IMaterialProperties.h"	// IID_IMaterialProperties ) ;
#include "IMesh.h"					// IID_IMesh ) ;
#include "IParticleSystem.h"		// IID_IParticleSystem  ) ;
#include "IProfileParser.h"			// IID_IProfileParser ) ;
#include "IProperties.h"			// IID_IProperty ) ;
#include "IRenderComponent.h"		// IID_IRenderComponent ) ;
#include "IRenderPrimitive.h"		// IID_IRenderPrimitive ) ;
#include "IRigidBody.h"				// IID_IRigidBody  ) ;
#include "IRigidBodyState.h"		// IID_IRigidBodyState  ) ;
#include "ISound.h"					// IID_ISoundSource ) ;
#include "ISoundListener.h"			// IID_ISoundListener ) ;
#include "ISoundManager.h"			// IID_ISoundManager ) ;
#include "IStateMaterial.h"			// IID_IStateMaterial ) ;
#include "ITextureLibrary.h"		// IID_ITextureLibrary ) ;
#include "IUTFWriter.h"				// IID_IUTFWriter ) ;
#include "IVertexBufferManager.h"	// IID_IVertexBufferManager ) ;
#include "IVideoStreamControl.h"	// IID_IVideoStreamControl) ;
#include "SearchPath.h"				// IID_ISearchPath ) ;
#include "Streamer.h"				// IID_IStreamer ) ;
#include "WindowManager.h"			// IID_IWindowManager ) ;
#include "bigimage.h"				// IID_IBigImage ) ;
#include "collision.h"				// IID_ICollision) ;
#include "engcomp.h"				// IID_IEngineComponent ) ;
#include "ichannel.h"				// IID_IChannel ) ;
#include "iimagesource.h"			// IID_IImageSource ) ;
#include "lightman.h"				// IID_ILightManager ) ;
#include "physics.h"				// IID_IPhysics ) ;
#include "renderer.h"				// IID_IRenderer ) ;
#include "rendpipeline.h"			// IID_IRenderPipeline );



//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__3CEC806D_82B9_48C5_85FF_3B7C431ED004__INCLUDED_)
