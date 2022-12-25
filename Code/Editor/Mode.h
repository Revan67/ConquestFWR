//
// Mode.h
//

#ifndef EDITOR_MODE_HEADER_H_
#define EDITOR_MODE_HEADER_H_

#ifndef DACOM_H
#include "DACOM.h"
#endif

#define IID_IEditorMode MAKE_IID("IEditorMode",1)

class DACOM_NO_VTABLE IMode : IDAComponent
{
public:
	virtual bool OnCreate( LPCREATESTRUCT lpcs, CCreateContext* pContext ) = 0;

	virtual bool Start() = 0;

	virtual bool Stop() = 0;

	virtual void Update() = 0;

	virtual void Draw() = 0;
};

#endif