// Author: Shaival Varma
// --------------------------------------------------------------------------
#pragma warning( disable : 4251 )
#pragma warning( disable : 4786 )

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC OLE automation classes
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#include <list>
#include <memory>
#include <string>
#include <vector>
#include "ARole.h"
#include "ASceneEntity.h"
#include "AStaticSceneEntity.h"
#include "CodeMsg.h"
#include "Color.h"
#include "ConstDynamicsStateAccessor.h"
#include "ConstSceneEntityStateAccessor.h"
#include "ConstStaticsStateAccessor.h"
#include "DynamicsStateAccessor.h"
#include "GLUtils.h"
#include "Location.h"
#include "MatrixUtil.h"
#include "ModelNS.h"
#include "Observer.h"
#include "Orientation.h"
#include "Position.h"
#include "Scene.h"
#include "SceneEntityStateAccessor.h"
#include "StaticsStateAccessor.h"
#include "StringType.h"
#include "StringUtils.h"
#include "TimeType.h"
#include "Update.h"
#include "Utils.h"
