// PropEditDoc.cpp : implementation of the CPropEditDoc class
//

#include "stdafx.h"
#include "PropEdit.h"

#include "PropEditDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPropEditDoc

IMPLEMENT_DYNCREATE(CPropEditDoc, CDocument)

BEGIN_MESSAGE_MAP(CPropEditDoc, CDocument)
	//{{AFX_MSG_MAP(CPropEditDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropEditDoc construction/destruction

CPropEditDoc::CPropEditDoc()
{
	// TODO: add one-time construction code here
}

CPropEditDoc::~CPropEditDoc()
{
}

BOOL CPropEditDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CPropEditDoc serialization

void CPropEditDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CPropEditDoc diagnostics

#ifdef _DEBUG
void CPropEditDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CPropEditDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CPropEditDoc commands

BOOL CPropEditDoc::OnSaveDocument(LPCTSTR lpszPathName) 
{
	// Save the data out to the given file directly instead of through the serialization method.
	CFile destFile;
	if (!destFile.Open (lpszPathName, CFile::modeWrite | CFile::modeCreate))
	{
		return FALSE;
	}

	// Write the header.

	PersistPropHeader hdr;
	hdr.fileVersion = PROP_FILE_VERSION;
	hdr.propCount = props.GetCount();
	hdr.propOffset = sizeof(hdr);
	hdr.dataOffset = sizeof(PersistProperty) * hdr.propCount + sizeof(hdr);

	destFile.Write (&hdr, sizeof(hdr));

	// Write the property table first.
	unsigned int curDataOffset = 0;
	POSITION curPos = props.GetHeadPosition();
	int i;
	for (i = 0; i < hdr.propCount; ++i)
	{
		Property &p = props.GetNext (curPos);

		PersistProperty pp;
		pp.nameLen = p.name.GetLength() + 1;
		pp.propType = p.type;
		pp.dataOffset = curDataOffset;
		pp.dataLen = pp.nameLen + p.get_data_size();

		destFile.Write (&pp, sizeof(pp));

		curDataOffset += pp.dataLen;
	}

	// Now write the data buffer
	curPos = props.GetHeadPosition();
	for (i = 0; i < hdr.propCount; ++i)
	{
		Property &p = props.GetNext (curPos);

		const char *name = p.name;
		destFile.Write (name, strlen(name)+1);
		destFile.Write (p.get_data(), p.get_data_size());
	}

	// Close the file. We are done.
	destFile.Close ();

	// Clear the modified flag.
	SetModifiedFlag (FALSE);
	return TRUE;
}

BOOL CPropEditDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
	CFile srcFile;
	if (!srcFile.Open (lpszPathName, CFile::modeRead))
	{
		return FALSE;
	}

	DeleteContents ();

	CFileStatus status;
	if (!srcFile.GetStatus (status))
	{
		srcFile.Close();
		return FALSE;
	}

	// Read the entire file into a buffer.
	char *buffer = new char[status.m_size];
	if (srcFile.Read (buffer, status.m_size) != status.m_size)
	{
		delete buffer;
		srcFile.Close();
		return FALSE;
	}

	srcFile.Close ();

	// Ensure that the header is proper.
	PersistPropHeader *hdr = (PersistPropHeader *) buffer;
	if (hdr->fileVersion != PROP_FILE_VERSION)
	{
		delete buffer;
		return FALSE;
	}

	PersistProperty *propStart = (PersistProperty *) (buffer + hdr->propOffset);
	char *dataStart = buffer + hdr->dataOffset;
	int count = hdr->propCount;

	for (int i = 0; i < count; ++i)
	{
		// Parse this property.

		// Find this property's data record and name.

		PersistProperty *p = &propStart[i];
		char *record = dataStart + p->dataOffset;
		ASSERT (p->nameLen != 0);
		ASSERT (record[p->nameLen-1] == '\0');
		char *name = record;
		char *data = record + p->nameLen;
		PROP_TYPE type = (PROP_TYPE) p->propType;

		Property *value = new Property (name);
		bool valid = true;

		switch (type)
		{
		case  PT_LONG:
			value->set_long (*((long *) data));
			break;

		case  PT_ULONG:
			value->set_ulong (*((unsigned long *) data));
			break;

		case  PT_SINGLE:
			value->set_single (*((SINGLE *) data));
			break;

		case  PT_DOUBLE:
			value->set_double (*((DOUBLE *) data));
			break;

		case  PT_VECTOR:
			value->set_vector ((PersistVector *) data);
			break;

		case  PT_MATRIX:
			value->set_matrix ((PersistMatrix *) data);
			break;

		case  PT_TRANSFORM:
			value->set_transform ((PersistTransform *) data);
			break;

		case  PT_COMPONENT:
			OutputDebugString("Component properties cannot be loaded from files!\n");
			delete value;
			value = NULL;
			break;

		case  PT_STRING:
			value->set_string ((const char *) data);
			break;

		case  PT_VOID:
			OutputDebugString ("Void properties cannot be loaded from files!\n");
			delete value;
			value = NULL;
			break;

		default:
			valid = false;
			OutputDebugString ("Unsupported property type found.\n");
			break;
		}

		if (value)
		{
			props.AddTail (*value);
			delete value;
		}
	}

	delete buffer;
	SetModifiedFlag (FALSE);
	
	return TRUE;
}

void CPropEditDoc::DeleteContents() 
{
	// TODO: Add your specialized code here and/or call the base class
	props.RemoveAll ();
	
	CDocument::DeleteContents();
}

Property *CPropEditDoc::get_property (int index)
{
	Property *p = NULL;
	POSITION pos = props.FindIndex(index);
	if (pos)
	{
		p = &props.GetAt(pos);
	}
	return p;
}

void CPropEditDoc::add_property (Property &p)
{
	props.AddTail (p);
	SetModifiedFlag ();
}

void CPropEditDoc::del_property (int index)
{
	Property *p = NULL;
	POSITION pos = props.FindIndex(index);
	if (pos)
	{
		props.RemoveAt(pos);
		SetModifiedFlag ();
	}
}

