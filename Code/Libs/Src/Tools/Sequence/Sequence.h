// Sequence.h : main header file for the SEQUENCE application
//
#include <TComponent.h>
#include <3DMath.h>
#include <Engine.h>
#include <Physics.h>
#include <Model.h>
#include <RendPipeline.h>
#include <ITextureLibrary.h>
#include <ITXMLib.h>
#include <IProfileParser.h>
#include <Lightman.h>
#include <IRenderPrimitive.h>
#include <system.h>
#include <renderer.h>
#include "IHardPoint.h"
#include <IAnim.h>
#include "camera.h"
#include "character.h"

#if !defined(AFX_SEQUENCE_H__3F629E05_355B_11D3_BF44_00A0CC25FE00__INCLUDED_)
#define AFX_SEQUENCE_H__3F629E05_355B_11D3_BF44_00A0CC25FE00__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// SequenceApp:
// See Sequence.cpp for the implementation of this class
//

class SequenceApp : public CWinApp
{
public:
	struct Input *		m_input;

	// Engine interface pointers
	ICOManager*			m_DACOM;

	ISystemContainer*	m_GS;
	struct IViewConstructor2*	m_PARSER;
	IEngine*			m_ENG;
	IProfileParser*		m_PROF;
	IRenderPipeline*		m_PIPE;
	IRenderer*				m_REND;
	ITextureLibrary*		m_TEXLIB;
	ILightManager*			m_LIGHTMAN;
	IPhysics*				m_PHY;
	ICollision*				m_COLL;
	IAnimation*				m_ANIM;
	IHardpoint*				m_HARDPOINT;
	IModel*					m_MODEL;
	IRenderPrimitive*		m_BATCH;
	struct ISoundManager*	m_SOUNDMAN;
	struct IStreamer*		m_STREAMER;
	struct PhysicalSystem * m_physics;
	GameCamera				*game_camera;
	Character				*CurChar;



	SequenceApp();
	~SequenceApp();

	void	SetStatusText(char *txt);
	void	SetNoTexture(void);
	void	EngineMainDraw(Character *);
	// default state is iterated vertex diffuse modulated with texture.
	void	SetDefaultTextureBlend(U32 texture_handle = ITL_INVALID_REF_ID, BOOL old_style = FALSE);
	BOOL	GetPerspectiveViewport(int & w, int & h) const;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(SequenceApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(SequenceApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SEQUENCE_H__3F629E05_355B_11D3_BF44_00A0CC25FE00__INCLUDED_)
