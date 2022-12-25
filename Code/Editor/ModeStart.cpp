//
// ModeStart
//

// IGNORE THIS MODULE: It is an out dated concept.

#include "stdafx.h"
#include "globals.h"

#include "Mode.h"
#include "Startup.h"
#include "SysMap.h"

#include <TComponent.h>
#include <TSmartPointer.h>
#include <IConnection.h>
#include <Engine.h>
#include <EventSys.h>
#include <system.h>

#include "SystemStructs.h"


#include "HKEvent.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

class ModeStart : public IMode, public IEventCallback
{
public:

	BEGIN_DACOM_MAP_INBOUND(ModeStart)
		DACOM_INTERFACE_ENTRY(IMode)
		DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()

	ModeStart()
	{
	}

	~ModeStart()
	{
	}

	virtual bool OnCreate( LPCREATESTRUCT lpcs, CCreateContext* pContext ){ return true; }

	virtual bool Start();

	virtual bool Stop();

	virtual void Update();

	virtual void Draw();

	// IEventCallback methods

	DEFMETHOD(Notify) (U32 message, void *param);

	// locals
};

//-----------------------------------------------------------------------------------------------------

bool ModeStart::Start()
{ 
	return false; 
}

//-----------------------------------------------------------------------------------------------------

bool ModeStart::Stop()
{ 
	return false;
}

//-----------------------------------------------------------------------------------------------------

void ModeStart::Update()
{
}

//-----------------------------------------------------------------------------------------------------

void ModeStart::Draw()
{
}

//-----------------------------------------------------------------------------------------------------

GENRESULT ModeStart::Notify(U32 message, void *param)
{
	return GR_OK;
}


