#ifndef IDOCCLIENT_H
#define IDOCCLIENT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IDocClient.H                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Rmarr $
*/			    
//--------------------------------------------------------------------------//

/*

	//-------------------------------------------
	//
	GENRESULT OnUpdate (struct IDocument *doc, const C8 *message, void *parm);
		INPUT:
			doc: Pointer to the IDocument instance sending the message.
			message: message name. (Defined by client) 
			parm: Variable data, defined by the message type.
		RETURNS:
			GR_OK to acknowledge the update. 
			GR_GENERIC if not able to process the message.
		NOTES:
			The associated document sends an update message to registered clients in response 
			to an UpdateAllClients() call.

  	//-------------------------------------------
	//
	GENRESULT OnClose (struct IDocument *doc);
		INPUT:
			doc: Pointer to the IDocument instance sending the message.
		RETURNS:
			GR_OK to acknowledge the Close message.
		NOTES:
			This message is sent by the document instance, notifying associated clients that
			the document is closing.



	GUIDELINES for implememting IDocumentClient:

	Constructor:
	-----------
		The constructor for an IDocumentClient component should register itself with the its
		IDocument component, and store the doc pointer without adding a reference to it.

		example construction of a DataViewer instance, which implements IDocumentClient:
			
				// get the connection point
				IDAConnectionPoint *connection;
 
				if (lpDesc->doc->QueryOutgoingInterface("IDocumentClient", &connection) != GR_OK)
				{
					result = GR_GENERIC;
					goto Done;
				}
	
				if ((pNewInstance = new DataViewer) == 0)
				{
					result = GR_OUT_OF_MEMORY;
					goto Done;
				}
			
				pNewInstance->doc = lpDesc->doc;				// lpDesc -> DOCDESC struct
				connection->Advise((IViewer *)pNewInstance,		// register ourselves with document
						&pNewInstance->connHandle);				// connHandle = handle used in Unadvise call
				connection->Release();							// done with this interface pointer


	OnClose:
	-----
		OnClose is used to notify the client instance that an associated document is about to close.
		
		Duties of Close():
		1) Unregister itself with the document.
		2) Discontinue visual display of the document. (e.g. Close windows corresponding to the document).
		3) Set document pointer to null.
		4) Free appropriate resources.

		example code:

			GENRESULT DataViewer::OnClose (struct IDocument *document)
			{
				if (document == doc && doc)	//is this the right instance?
				{
					COMPTR<IDAConnectionPoint> connection;

					if (doc->QueryOutgoingInterface("IDocumentClient", connection) != GR_OK)
						return GR_GENERIC;

					// ... close windows, write data, etc...
					//
					
					connection->Unadvise(connHandle);
					doc = 0;
				}

				return GR_OK;
			}

		Other warnings: More than one object may have a pointer to your instance. It is possible for 
		a client instance to remain alive (references>0) even though all associated documents have been
		closed. Therefore, you must be careful to guard all public methods will checks to make sure
		you still have a valid document.



	Release:
	--------
		Free all references to external components (e.g. the document) and free resources.

		example code:

			U32 DataViewer::Release (void)
			{
				if (dwRefs > 0)
					dwRefs--;
				if (dwRefs == 0)
				{
					// free resources used by instance here ...

  					if (doc)
						OnClose(doc);
					
					delete this;
					return 0;
				}

				return dwRefs;
			}
*/

#ifndef DACOM_H
#include "DACOM.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

#define IID_IDocumentClient "IDocumentClient"

struct DACOM_NO_VTABLE IDocumentClient : public IDAComponent
{
	DEFMETHOD(OnUpdate) (struct IDocument *doc, const C8 *message = 0, void *parm = 0) = 0;

	DEFMETHOD(OnClose) (struct IDocument *doc) = 0;
};



//--------------------------------------------------------------------------//
//---------------------------End IDocClient.h--------------------------------//
//--------------------------------------------------------------------------//

#endif