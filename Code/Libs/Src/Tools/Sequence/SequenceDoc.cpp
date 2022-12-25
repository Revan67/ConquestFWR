// SequenceDoc.cpp : implementation of the SequenceDoc class
//

#include "stdafx.h"
#include "Sequence.h"
#include <assert.h>
#include <math.h>
#include <string.h>
#include <dacom.h>
#include <iprofileparser.h>
#include "motion.h"
#include "filesys.h"
#include "character.h"
#include "ChannelEventTypes.h"
#include <direct.h>

#include <float.h>

#include "SequenceDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern	SequenceApp theApp;


static	float	__cdecl	Degrees2Radians(float deg)
{
	return	deg * PI / 180.0;
}

static	float	__cdecl	Radians2Degrees(float rad)
{
	return	rad * 180.0 / PI;
}


typedef	struct	AnimEnumInfoTAG
{
	int		cnt;
	char	*names;
}	AnimInfo;

static	void	__cdecl	AnimEnumCB(const char *name, void *misc)
{
	assert(name);

	if(misc)
	{
		AnimInfo	*ainfo	=(AnimInfo *)misc;

		if(ainfo->names)	//counting or storing?
		{
			strcpy(&ainfo->names[256 * ainfo->cnt++], name);
		}
		else
		{
			ainfo->cnt++;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// SequenceDoc

IMPLEMENT_DYNCREATE(SequenceDoc, CDocument)

BEGIN_MESSAGE_MAP(SequenceDoc, CDocument)
	//{{AFX_MSG_MAP(SequenceDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// SequenceDoc construction/destruction

SequenceDoc::SequenceDoc()
{
	srand(time(NULL));

	m_Character		=NULL;
	m_CharacterArch	=NULL;
	TotalEvents		=NULL;
	NumTotalEvents	=0;
	TEventSize		=0;
}

SequenceDoc::~SequenceDoc()
{
	int	i;

	if(TotalEvents)
	{
		for(i=0;i < TEventSize;i++)
		{
			delete	[]	TotalEvents[i];
		}
		delete	[]	TotalEvents;
	}
	if(m_Character)
	{
		delete	m_Character;
	}
}

BOOL SequenceDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// SequenceDoc serialization

void SequenceDoc::Serialize(CArchive& ar)
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
// SequenceDoc diagnostics

#ifdef _DEBUG
void SequenceDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void SequenceDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

 
/////////////////////////////////////////////////////////////////////////////
// SequenceDoc commands

BOOL SequenceDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
	int		i, j, CurEvent;
	char	temp[_MAX_PATH];

	assert(lpszPathName);

	if(!CDocument::OnOpenDocument(lpszPathName))
	{
		return FALSE;
	}

	//make sure the directory didn't get messed with
	GetModuleFileName(NULL, temp, _MAX_PATH);

	if(strrchr(temp, '\\'))
	{
		*strrchr(temp, '\\')	=0;
	}

	//sort of a hack here... see if it's running in
	//a debug or release dir and bump it back so that
	//the working folder thing in the project settings works
	if(!stricmp((temp + strlen(temp)-5), "debug"))
	{
		*(temp + strlen(temp)-5)	=0;
	}
	if(!stricmp((temp + strlen(temp)-7), "release"))
	{
		*(temp + strlen(temp)-7)	=0;
	}

	chdir(temp);


	if(strrchr(lpszPathName, '.'))
	{
		*strrchr(lpszPathName, '.')	=0;
	}
	if(strrchr(lpszPathName, '\\'))
	{
		m_CharacterArch	=new CharacterArchetype(strrchr(lpszPathName, '\\')+1);
	}
	else
	{
		m_CharacterArch	=new CharacterArchetype(lpszPathName);
	}
	
	m_Character	=(Character *)m_CharacterArch->create_instance();

	//build a big ass list of events
	//this is for event picking in the prop dialog
	//count up max possible events
	for(i=0;i < m_CharacterArch->num_sequences;i++)
	{
		if(m_CharacterArch->sequences[i].script)
		{
			EventIterator	ei;

			theApp.m_ANIM->get_script_events(m_CharacterArch->anim_arch, m_CharacterArch->sequences[i].script, ei);

			for(unsigned int ev=0;ev < ei.get_event_count();ev++)
			{
				if(ei.get_event_type(ev) == NAMED_EVENT)
				{
					NumTotalEvents++;
				}
			}
		}
	}

	TotalEvents	=new char*[NumTotalEvents];
	for(i=0;i < NumTotalEvents;i++)
	{
		TotalEvents[i]	=new char[80];
	}

	//eliminate duplicates
	for(i=CurEvent=0;i < m_CharacterArch->num_sequences;i++)
	{
		if(m_CharacterArch->sequences[i].script)
		{
			EventIterator	ei;

			theApp.m_ANIM->get_script_events(m_CharacterArch->anim_arch, m_CharacterArch->sequences[i].script, ei);

			for(unsigned int ev=0;ev < ei.get_event_count();ev++)
			{
				if(ei.get_event_type(ev) == NAMED_EVENT)
				{
					strcpy(TotalEvents[CurEvent], (char *)ei.get_event_data(ev));

					for(j=0;j < CurEvent;j++)	//slooooooooow
					{
						if(!strcmp(TotalEvents[j], TotalEvents[CurEvent]))
						{
							break;
						}
					}
					if(j==CurEvent)
					{
						CurEvent++;
					}
				}
			}
		}
	}

	TEventSize		=NumTotalEvents;
	NumTotalEvents	=CurEvent;

	return	TRUE;
}


BOOL SequenceDoc::OnSaveDocument(LPCTSTR lpszPathName) 
{
	int				i, j;
	char			temp[_MAX_PATH];
	FILE			*f;

	assert(lpszPathName);

  	f	=fopen(lpszPathName, "w");

	if(f)	//these things writepriv doesn't work right for
	{
		//[Sequences] - motion sequences.
		if(fputs("[Sequences]\n", f) < 0)
		{
			assert(0);
		}

		for(i=0;i < m_CharacterArch->num_sequences;i++)
		{
			if(fputs(m_CharacterArch->sequences[i].name, f) < 0)
			{
				assert(0);
			}
			fputc('\n', f);
		}

		//[Attachments]
		if(fputs("\n[Attachments]\n", f) < 0)
		{
			assert(0);
		}

		for(i=0;i < m_CharacterArch->num_attachments;i++)
		{
			if(fputs(m_CharacterArch->attachments[i].name, f) < 0)
			{
				assert(0);
			}
			fputc('\n', f);
		}

		//[Transfers]
		if(fputs("\n[Transfers]\n", f) < 0)
		{
			assert(0);
		}

		for(i=0;i < m_CharacterArch->num_transfers;i++)
		{
			if(fputs(m_CharacterArch->transfers[i].name, f) < 0)
			{
				assert(0);
			}
			fputc('\n', f);
		}
	}
	fflush(f);
	fclose(f);

	//attachments stuff
	for(i=0;i < m_CharacterArch->num_attachments;i++)
	{
		WritePrivateProfileString(m_CharacterArch->attachments[i].name,
			"active",
			(m_CharacterArch->attachments[i].active)? "true" : "false",
			lpszPathName);

		WritePrivateProfileString(m_CharacterArch->attachments[i].name,
			"child_object",
			m_CharacterArch->attachments[i].child_object,
			lpszPathName);

		WritePrivateProfileString(m_CharacterArch->attachments[i].name,
			"parent",
			m_CharacterArch->attachments[i].parent_hp,
			lpszPathName);

		WritePrivateProfileString(m_CharacterArch->attachments[i].name,
			"child",
			m_CharacterArch->attachments[i].child_hp,
			lpszPathName);

		WritePrivateProfileString(m_CharacterArch->attachments[i].name,
			"select",
			(m_CharacterArch->attachments[i].select)? "true" : "false",
			lpszPathName);
	}

	//transfers stuff
	for(i=0;i < m_CharacterArch->num_transfers;i++)
	{
		WritePrivateProfileString(m_CharacterArch->transfers[i].name,
			"event",
			m_CharacterArch->transfers[i].trigger,
			lpszPathName);

		WritePrivateProfileString(m_CharacterArch->transfers[i].name,
			"source",
			m_CharacterArch->attachments[m_CharacterArch->transfers[i].src].name,
			lpszPathName);

		WritePrivateProfileString(m_CharacterArch->transfers[i].name,
			"dest",
			m_CharacterArch->attachments[m_CharacterArch->transfers[i].dst].name,
			lpszPathName);
	}

	WritePrivateProfileString("Object",
		"mesh",
		m_CharacterArch->mesh_name,
		lpszPathName);

	WritePrivateProfileString("Object",
		"anim",
		m_CharacterArch->anim_name,
		lpszPathName);

	sprintf(temp, "%.2f", Radians2Degrees(m_CharacterArch->turn_rate));

	WritePrivateProfileString("Object",
		"turnrate",
		temp,
		lpszPathName);


	for(i=0;i < MT_NUM_MOTION_TYPES;i++)
	{
		for(j=0;j < MT_NUM_MOTION_TYPES;j++)
		{
			if(m_CharacterArch->seq_grid[i][j])
			{
				WritePrivateProfileString(MotionTypeNames[i],
					MotionTypeNames[j],
					m_CharacterArch->seq_grid[i][j]->name,
					lpszPathName);
			}
		}
	}

	for(i=0;i < m_CharacterArch->num_sequences;i++)
	{
		WritePrivateProfileString(m_CharacterArch->sequences[i].name,
			"type",
			MotionTypeNames[m_CharacterArch->sequences[i].type],
			lpszPathName);

		if(m_CharacterArch->sequences[i].num_wait_states)
		{
			for(j=0;j < m_CharacterArch->sequences[i].num_wait_states;j++)
			{
				sprintf(temp, "wait%d", j);

				WritePrivateProfileString(m_CharacterArch->sequences[i].name,
					temp,
					m_CharacterArch->sequences[i].wait_events[j],
					lpszPathName);

				sprintf(temp, "target%d", j);

				WritePrivateProfileString(m_CharacterArch->sequences[i].name,
					temp,
					m_CharacterArch->sequences[i].wait_targets[j],
					lpszPathName);
			}
		}
		else
		{
			WritePrivateProfileString(m_CharacterArch->sequences[i].name,
				"script",
				m_CharacterArch->sequences[i].script,
				lpszPathName);
		}

		WritePrivateProfileString(m_CharacterArch->sequences[i].name,
			"loop",
			(m_CharacterArch->sequences[i].loop)? "true" : "false",
			lpszPathName);

		sprintf(temp, "%.2f", m_CharacterArch->sequences[i].transition_duration);

		WritePrivateProfileString(m_CharacterArch->sequences[i].name,
			"transition",
			temp,
			lpszPathName);

		if(m_CharacterArch->sequences[i].transition_duration > 0)
		{
			WritePrivateProfileString(m_CharacterArch->sequences[i].name,
				"interrupt",
				(m_CharacterArch->sequences[i].interrupt)? "true" : "false",
				lpszPathName);
		}

		if(m_CharacterArch->sequences[i].end)
		{
			WritePrivateProfileString(m_CharacterArch->sequences[i].name,
				"end",
				m_CharacterArch->sequences[i].end->name,
				lpszPathName);
		}
	}

	//write out color values for states
	for(i=0;i < MT_NUM_MOTION_TYPES;i++)
	{
		sprintf(temp, "%d", m_CharacterArch->GDat->StateColors[i]);

		WritePrivateProfileString("StateColors",
			MotionTypeNames[i],
			temp,
			lpszPathName);
	}
	//flush
	WritePrivateProfileString(NULL, NULL, NULL, lpszPathName); 

//	return CDocument::OnSaveDocument(lpszPathName);
	return TRUE;
}

void SequenceApp::SetDefaultTextureBlend(U32 texture_handle, BOOL old_style)
{
	unsigned int	id	=0;

	if(old_style)
	{
		id	=texture_handle;
	}
	else
	{
		if (texture_handle != ITL_INVALID_REF_ID) {
			ITL_TEXTUREFRAME_IRP texture_irp;
			m_TEXLIB->get_texture_ref_frame(texture_handle, ITL_FRAME_CURRENT, &texture_irp);
			id = texture_irp.rp_texture_id;
		}
	}

	//if (texture_handle != 0)
	{
		m_BATCH->set_texture_stage_texture(0, id);
	}

	m_BATCH->set_texture_stage_state(0, D3DTSS_TEXCOORDINDEX,	0);
	m_BATCH->set_texture_stage_state(0, D3DTSS_COLOROP,			D3DTOP_MODULATE);
	m_BATCH->set_texture_stage_state(0, D3DTSS_COLORARG1,		D3DTA_TEXTURE);
	m_BATCH->set_texture_stage_state(0, D3DTSS_COLORARG2,		D3DTA_DIFFUSE);

	m_BATCH->set_texture_stage_state(0, D3DTSS_ALPHAOP,			D3DTOP_MODULATE);
	m_BATCH->set_texture_stage_state(0, D3DTSS_ALPHAARG1,		D3DTA_TEXTURE);
	m_BATCH->set_texture_stage_state(0, D3DTSS_ALPHAARG2,		D3DTA_DIFFUSE);

	m_BATCH->set_texture_stage_state(0, D3DTSS_MAGFILTER,		D3DTFG_LINEAR);
	m_BATCH->set_texture_stage_state(0, D3DTSS_MINFILTER,		D3DTFN_LINEAR);
	m_BATCH->set_texture_stage_state(0, D3DTSS_MIPFILTER,		D3DTFP_POINT);

	m_BATCH->set_texture_stage_state(1, D3DTSS_COLOROP,			D3DTOP_DISABLE);

	//ValidateTextureBlend();
}
