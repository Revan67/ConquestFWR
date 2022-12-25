// SequenceDoc.h : interface of the SequenceDoc class
//
/////////////////////////////////////////////////////////////////////////////

#include "motion.h"
#include "ianim.h"
#include "engine.h"
#include "system.h"
#include "deform.h"
#include "character.h"
#include "camera.h"

#if !defined(AFX_SEQUENCEDOC_H__3F629E0D_355B_11D3_BF44_00A0CC25FE00__INCLUDED_)
#define AFX_SEQUENCEDOC_H__3F629E0D_355B_11D3_BF44_00A0CC25FE00__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class SequenceDoc : public CDocument
{
protected: // create from serialization only
	SequenceDoc();
	DECLARE_DYNCREATE(SequenceDoc)

// Attributes
public:
	int		NumTotalEvents;
	int		TEventSize;
	char	**TotalEvents;

// Operations
public:
	CharacterArchetype	*m_CharacterArch;	//can't think of a good name for this
	Character			*m_Character;
	GraphUIData			*GraphDat;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(SequenceDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~SequenceDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

private:
	void	command(CommandType com);
	BOOL	start_sequence(MotionType m);
protected:

// Generated message map functions
protected:
	//{{AFX_MSG(SequenceDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SEQUENCEDOC_H__3F629E0D_355B_11D3_BF44_00A0CC25FE00__INCLUDED_)
