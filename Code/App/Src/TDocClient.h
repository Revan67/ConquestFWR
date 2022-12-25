#ifndef TDOCCLIENT_H
#define TDOCCLIENT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                TDocClient.h                              //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Header: /Conquest/App/Src/TDocClient.h 1     9/20/98 1:03p Gboswood $
*/			    
//--------------------------------------------------------------------------//

#ifndef IDOCCLIENT_H
#include <IDocClient.h>
#endif

#ifndef TCOMPONENT_H
#include <TComponent.h>
#endif

#ifndef DOCUMENT_H
#include <Document.h>
#endif

#ifndef ICONNECTION_H
#include <IConnection.h>
#endif

#ifndef TSMARTPOINTER_H
#include <TSmartPointer.h>
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct DACOM_NO_VTABLE DocumentClient : public IDocumentClient
{
	IDocument * doc;
	U32 handle;

	DocumentClient (void)
	{
		doc = 0;
	}

	~DocumentClient (void)
	{
		if (doc)
			OnClose(doc);
	}

	/* IDocumentClient methods */
	
	DEFMETHOD(OnClose) (struct IDocument *_doc)
	{
		if (_doc && _doc == doc)
		{
			COMPTR<IDAConnectionPoint> connection;
			GENRESULT result;
			
			if ((result = _doc->QueryOutgoingInterface("IDocumentClient", connection)) == GR_OK)
				connection->Unadvise(handle);
			doc = 0;
			return result;
		}
		return GR_OK;
	}

	/* DocumentClient methods */

	BOOL32 MakeConnection (IDocument * _doc)
	{
		COMPTR<IDAConnectionPoint> connection;
		
		if (_doc->QueryOutgoingInterface("IDocumentClient", connection) == GR_OK)
		{
			if (connection->Advise(this, &handle) == GR_OK)
			{
				doc = _doc;
				return 1;
			}
		}

		return 0;
	}
};


//--------------------------------------------------------------------------//
//----------------------------End TDocClient.h------------------------------//
//--------------------------------------------------------------------------//
#endif
