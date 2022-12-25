#ifndef DOCUMENT_H
#define DOCUMENT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               Document.H                                 //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Rmarr $
*/			    
//--------------------------------------------------------------------------//

/*
	IDocument is a kind of IFileSystem that is designed to provide structured 
        data to instances of IDocumentClient (See IDocClient.h). In addition, it keeps 
	track of modifications so the application knows what needs to be saved. 
	A document provides a means of keeping multiple clients synchonized with
	respect to their view of the data. 

	All IFileSystem methods work as previously documented with one exception: 
	IDocument::OpenChild() always fails, and returns INVALID_HANDLE_VALUE.
*/

#ifndef FILESYS_H
#include "FileSys.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct DOCDESC : public DAFILEDESC
{
	void *				memory;
	U32					memoryLength;

	DOCDESC (const C8 *_file_name = NULL, const C8 *_interfaceName = "IDocument") : DAFILEDESC(_file_name, _interfaceName)
	{
		memset(((char *)this)+sizeof(DAFILEDESC), 0, sizeof(*this)-sizeof(DAFILEDESC));
		size = sizeof(*this);
	};
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//

#define IID_IDocument MAKE_IID( "IDocument", 1 )

//

struct DACOM_NO_VTABLE IDocument : public IFileSystem
{
	/*
	GENRESULT Document::CreateInstance (DACOMDESC *descriptor, void **instance)
		INPUT:
			descriptor: Address of a DOCDESC structure.
			instance: Address of a IDocument pointer.
		RETURNS:
			GR_OK if created an instance of IDocument. The new instance pointer
			is returned in '*instance'.
		OUTPUT:
			The behavior of this method is similar to the CreateInstance of IFileSystem, 
			except that it returns a pointer to an instance of IDocument instead
			of IFileSystem. 
		NOTES:
			If 'memory' (See DOCDESC description) is a valid pointer, the instance is 
			created using data from memory instead from disk. (The instance creates
			it's own copy of the data.)
	*/
	virtual GENRESULT COMAPI CreateInstance (DACOMDESC *descriptor, void **instance) = 0;


	/*
	GENRESULT GetChildDocument (const C8 *fileName, struct IDocument **document);
		INPUT:
			filename: ASCIIZ name of sub-document.  e.g. "data\\saucer\\rotation.dat"
			document: Address of IDocument pointer. 
		RETURNS:
			GR_OK if successfully found the subdocument. '*document' is filled with
			the address of the child document instance.
			Otherwise, returns some failure code, and '*document' is set to NULL.
		OUTPUT:
			Finds a subfile. Multiple calls to GetChildDocument using the same 'fileName'
			parameter will result in the same returned object.
			The calling function must release the child document by calling its 
			Release method before disposing of the pointer.
		NOTES:
			If successful, the returned instance is given the same access rights as the
			parent system, with read&write sharing enabled.
	*/
	virtual GENRESULT COMAPI GetChildDocument (const C8 *fileName, struct IDocument **document) = 0;
	
	/*
    BOOL32 IsModified (void);
		RETURNS:
			TRUE iff the data associated with the document (or any sub-document) has been marked "modified".
		REMARKS:
			Data can be marked "modified" explicitly by calling the SetModified method,
			or implicitly by writing new data to the document, deleting a sub-document, or
			creating a new sub-document.
	*/
	virtual BOOL32 COMAPI IsModified (void) = 0;

	/*
	BOOL32 SetModified (BOOL32 modifyValue = 1);
		INPUT:
			modifyValue: New value to set for modified flag.
		RETURNS:
			Same value as 'modifyValue'.
		OUTPUT:
			Sets the document's "modified" flag to the new value. If the new value is FALSE, 
			all sub-documents are also marked FALSE. If the new value is not FALSE, the message
			is not passed to sub-documents.
	*/
	virtual BOOL32 COMAPI SetModified (BOOL32 modifyValue = 1) = 0;

	/*
	GENRESULT UpdateAllClients (const C8 *message, void *parm);
		INPUT:
			message: Message to be sent to IDocumentClients.
			parm: Extra data to send along with message.
		RETURNS:
			GR_OK.
		OUTPUT:
			Calls the OnUpdate method for each IDocumentClient instance associated with the document.
			Calls "up" the heirarchy, passing the update message to each parent until it reaches
			the root document. Message and parm values are defined by the implementer of the
			IDocumentClient interface. (See IDocClient.h)
	*/
	virtual GENRESULT COMAPI UpdateAllClients (const C8 *message=0, void *parm=0) = 0;

	/*
	GENRESULT CloseAllClients (void);
		RETURNS:
			GR_OK.
		OUTPUT:
			Calls the OnClose method for each IDocumentClient instance associated with the document.
			Recursively calls sub-documents with the same message.
	*/
	virtual GENRESULT COMAPI CloseAllClients (void) = 0;

	/*
  	BOOL32 Rename (const C8 *newName);
		INPUT:
			newName: ASCIIZ name used for new name
		RETURNS
			TRUE if the document's name was changed.
		OUTPUT:
			Renames the directory or filename associated with the instance.
                        NOTE: The file position is undefined when this function returns.
	*/
	virtual BOOL32 COMAPI Rename (const C8 *newName) = 0;
};




//--------------------------------------------------------------------------//
//----------------------------End Document.h--------------------------------//
//--------------------------------------------------------------------------//

#endif
