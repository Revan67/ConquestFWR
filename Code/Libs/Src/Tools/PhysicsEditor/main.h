//
// <main.h> - DACOM/GL testbed include file
//

#ifndef MAIN_H
#define MAIN_H

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#undef  DA_ERROR_LEVEL
#define DA_ERROR_LEVEL  __SEV_TRACE_5

#include "dacom.h"
#include "xform.h"
#include "3dmath.h"
#include "engine.h"
#include "gamesys.h"
#include "lightman.h"
#include "renderer.h"
#include "windowmanager.h"
#include "ITextureLibrary.h"
#include "physics.h"
#include "ihardpoint.h"
#include "ichannel.h"
#include "ianim.h"
#include "timer.h"
#include "collision.h"
#include "rendpipeline.h"

#include "opengl.h"

// DACOM components:

extern IEngine *           ENGINE;
extern IPhysics *          PHYSICS;
extern ICollision *        COLLISION;
extern IRenderer *         RENDERER;
extern IHardpoint *        HARDPOINT;
extern ICOManager *        DACOM;
extern ILightManager *     LIGHT;
extern IWindowManager *    WIN;
extern ITextureLibrary *   TEXTURELIB;
extern IAnimation *        ANIM;
extern IChannel *          CHANNEL;
extern IRenderPipeline *   RP;
extern ISystemContainer	*  SYSTEM;

// Windows components:

extern HINSTANCE   appInstance;
extern HWND        windowHandle;
extern HDC         globalDC;

// App globals:

extern BOOL32      fullScreen;
extern BOOL32      exitProgram;

extern Timer       timer;

extern void AppFatal(C8 *message);

extern const char * appName;

#endif
