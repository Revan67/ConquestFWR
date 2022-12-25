/**********************************************************************
 *<
	FILE: CQ2Material.cpp

	DESCRIPTION:	Appwizard generated plugin

	CREATED BY: 

	HISTORY: 

 *>	Copyright (c) 2003, All Rights Reserved.
 **********************************************************************/

#include "CQ2Material.h"

#include "StdMat.h"
#include <string>
#include "CQ2MatEditor.h"


#define NSUBMTL		1 // TODO: number of sub-materials supported by this plugin 

#define PBLOCK_REF	NSUBMTL

#include "icq2mtl.h"


class CQ2Material : public iCQ2Mtl 
{
	public:


		// Parameter block
		IParamBlock2	*pblock;	//ref 0

		Mtl				*submtl[NSUBMTL];  //array of sub-materials
		BOOL			mapOn[NSUBMTL];
		float			spin;
		Interval		ivalid;
		BitmapTex *     m_bitmapTex;
		std::string     m_matName;
		MatHandle       m_matHandle;
		
		ParamDlg *CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp);
		void Update(TimeValue t, Interval& valid);
		Interval Validity(TimeValue t);

		virtual const char *GetMatName()
		{
			return m_matName.c_str();
		}

		void Reset();

		void NotifyChanged();

		// From MtlBase and Mtl
		void SetAmbient(Color c, TimeValue t);		
		void SetDiffuse(Color c, TimeValue t);		
		void SetSpecular(Color c, TimeValue t);
		void SetShininess(float v, TimeValue t);
		Color GetAmbient(int mtlNum=0, BOOL backFace=FALSE);
	    Color GetDiffuse(int mtlNum=0, BOOL backFace=FALSE);
		Color GetSpecular(int mtlNum=0, BOOL backFace=FALSE);
		float GetXParency(int mtlNum=0, BOOL backFace=FALSE);
		float GetShininess(int mtlNum=0, BOOL backFace=FALSE);		
		float GetShinStr(int mtlNum=0, BOOL backFace=FALSE);
		float WireSize(int mtlNum=0, BOOL backFace=FALSE);
				

		// Shade and displacement calculation
		void Shade(ShadeContext& sc);
		float EvalDisplacement(ShadeContext& sc); 
		Interval DisplacementValidity(TimeValue t); 	

		// SubMaterial access methods
		int NumSubMtls() {return NSUBMTL;}
		Mtl* GetSubMtl(int i);
		void SetSubMtl(int i, Mtl *m);
		TSTR GetSubMtlSlotName(int i);
		TSTR GetSubMtlTVName(int i);

		// SubTexmap access methods
		int NumSubTexmaps() {return 0;}
		Texmap* GetSubTexmap(int i);
		void SetSubTexmap(int i, Texmap *m);
		TSTR GetSubTexmapSlotName(int i);
		TSTR GetSubTexmapTVName(int i);
		
		BOOL SetDlgThing(ParamDlg* dlg);
		CQ2Material(BOOL loading);

		
		// Loading/Saving
		IOResult Load(ILoad *iload);
		IOResult Save(ISave *isave);

		//From Animatable
		Class_ID ClassID() {return CQ2Material_CLASS_ID;}		
		SClass_ID SuperClassID() { return MATERIAL_CLASS_ID; }
		void GetClassName(TSTR& s) {s = GetString(IDS_CLASS_NAME);}

		RefTargetHandle Clone( RemapDir &remap );
		RefResult NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, PartID& partID,  RefMessage message);


		int NumSubs() { return 1+NSUBMTL; }
		Animatable* SubAnim(int i); 
		TSTR SubAnimName(int i);

		// TODO: Maintain the number or references here 
		int NumRefs() { return 1+NSUBMTL; }
		RefTargetHandle GetReference(int i);
		void SetReference(int i, RefTargetHandle rtarg);



		int	NumParamBlocks() { return 1; }					// return number of ParamBlocks in this instance
		IParamBlock2* GetParamBlock(int i) { return pblock; } // return i'th ParamBlock
		IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

		void DeleteThis() { delete this; }

		// new functions

		void assignTexture( const char* _filename, const char* _matname );
};



class CQ2MaterialClassDesc : public ClassDesc2 
{
	public:
	int 			IsPublic() { return TRUE; }
	void *			Create(BOOL loading = FALSE) { return new CQ2Material(loading); }
	const TCHAR *	ClassName() { return GetString(IDS_CLASS_NAME); }
	SClass_ID		SuperClassID() { return MATERIAL_CLASS_ID; }
	Class_ID		ClassID() { return CQ2Material_CLASS_ID; }
	const TCHAR* 	Category() { return GetString(IDS_CATEGORY); }
	const TCHAR*	InternalName() { return _T("CQ2Material"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }					// returns owning module handle
};

static CQ2MaterialClassDesc CQ2MaterialDesc;
ClassDesc2* GetCQ2MaterialDesc() { return &CQ2MaterialDesc; }


enum { cq2material_params };


//-------------------------------------------------------------------------------------------------------------
// Custom dlg proc

struct CQ2DlgProc : public ParamMap2UserDlgProc, public CQ2MatEditor
{
	// interface
	virtual BOOL DlgProc(TimeValue t, IParamMap2 *map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	virtual void DeleteThis();

	virtual void SetThing(ReferenceTarget *m)
	{
		if( m->ClassID() == CQ2Material_CLASS_ID )
		{
			m_CQ2Material = (CQ2Material*)m;
			setup();
		}
		else
		{
			ParamMap2UserDlgProc::SetThing(m);
		}
	}

	CQ2DlgProc() : m_CQ2Material(0), m_MainWnd(0) {}

protected:

	void draw(HDC hdc, Rect &rect );
	void setup();
	void update();

private:

	CQ2Material* m_CQ2Material;
	HWND         m_MainWnd;
};

CQ2DlgProc g_CQ2DlgProc;

static ParamBlockDesc2 cq2material_param_blk 
( 
	cq2material_params, 
	_T("params"),  
	0, 
	&CQ2MaterialDesc, 
	P_AUTO_CONSTRUCT + P_AUTO_UI, 
	PBLOCK_REF, 

	//rollout
	IDD_PANEL, 
	IDS_PARAMS, 
	0, 
	0, 
	&g_CQ2DlgProc,

	// params

	end
);

//-------------------------------------------------------------------------------------------------------------

CQ2Material::CQ2Material(BOOL loading) 
{
	for (int i=0; i<NSUBMTL; i++) submtl[i] = NULL;
	pblock = NULL;

	if (!loading) 
		Reset();
}


void CQ2Material::Reset() 
{
	ivalid.SetEmpty();
	for (int i=0; i<NSUBMTL; i++) 
	{
		if( submtl[i] )
		{ 
			DeleteReference(i);
			submtl[i] = NULL;
		}
	}

	m_matHandle = 0;
	m_bitmapTex = NULL;
	CQ2MaterialDesc.MakeAutoParamBlocks(this);
}



ParamDlg* CQ2Material::CreateParamDlg(HWND hwMtlEdit, IMtlParams *imp) 
{
	IAutoMParamDlg* masterDlg = CQ2MaterialDesc.CreateParamDlgs(hwMtlEdit, imp, this);
	masterDlg->SetThing( this );
	
	// TODO: Set param block user dialog if necessary
	return masterDlg;
}

BOOL CQ2Material::SetDlgThing(ParamDlg* dlg)
{
	return FALSE;
}

Interval CQ2Material::Validity(TimeValue t)
{
	Interval valid = FOREVER;		

	for (int i=0; i<NSUBMTL; i++) 
	{
		if (submtl[i]) 
			valid &= submtl[i]->Validity(t);
	}
	
	return valid;
}

/*===========================================================================*\
 |	Subanim & References support
\*===========================================================================*/

RefTargetHandle CQ2Material::GetReference(int i) 
{
	if (i < NSUBMTL )
		return submtl[i];
	else return pblock;
}

void CQ2Material::SetReference(int i, RefTargetHandle rtarg) 
{
	if (i < NSUBMTL)
		submtl[i] = (Mtl *)rtarg; 
	else pblock = (IParamBlock2 *)rtarg; 
}

TSTR CQ2Material::SubAnimName(int i) 
{
	if (i < NSUBMTL)
		return GetSubMtlTVName(i);
	else return TSTR(_T(""));
}

Animatable* CQ2Material::SubAnim(int i) 
{
	if (i < NSUBMTL)
		return submtl[i]; 
	else return pblock;
}

RefResult CQ2Material::NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message ) 
{
	switch (message) 
	{
		case REFMSG_CHANGE:
			ivalid.SetEmpty();
			if (hTarget == pblock)
			{
				ParamID changing_param = pblock->LastNotifyParamID();
				cq2material_param_blk.InvalidateUI(changing_param);
			}
			break;

	}
	return REF_SUCCEED;
}

/*===========================================================================*\
 |	SubMtl get and set
\*===========================================================================*/

Mtl* CQ2Material::GetSubMtl(int i)
{
	if (i < NSUBMTL )
		return submtl[i];
	return NULL;
}

void CQ2Material::SetSubMtl(int i, Mtl *m)
{
	ReplaceReference(i,m);
	// TODO: Set the material and update the UI	
}

TSTR CQ2Material::GetSubMtlSlotName(int i)
{
	// Return i'th sub-material name 
	return _T(""); 
}

TSTR CQ2Material::GetSubMtlTVName(int i)
{
	return GetSubMtlSlotName(i);
}

/*===========================================================================*\
 |	Texmap get and set
 |  By default, we support none
\*===========================================================================*/

Texmap* CQ2Material::GetSubTexmap(int i)
{
	return NULL;
}

void CQ2Material::SetSubTexmap(int i, Texmap *m)
{
}

TSTR CQ2Material::GetSubTexmapSlotName(int i)
{
	return _T("");
}

TSTR CQ2Material::GetSubTexmapTVName(int i)
{
	// Return i'th sub-texture name 
	return GetSubTexmapSlotName(i);
}

/*===========================================================================*\
 |	Standard IO
\*===========================================================================*/

#define MTL_HDR_CHUNK 0x4000
#define CQ2_HDR_CHUNK 0x4001

IOResult CQ2Material::Save(ISave *isave) 
{ 
	IOResult res;

	isave->BeginChunk(MTL_HDR_CHUNK);
		res = MtlBase::Save(isave);
		if (res!=IO_OK) return res;
	isave->EndChunk();

	// write out material name (if any)
	if( m_matName.size() > 0 )
	{
		isave->BeginChunk(CQ2_HDR_CHUNK);
			res = isave->WriteCString( m_matName.c_str() );
			if (res!=IO_OK) return res;
		isave->EndChunk();
	}

	return IO_OK;
}	

IOResult CQ2Material::Load(ILoad *iload) 
{ 
	IOResult res;
	int id;
	while (IO_OK==(res=iload->OpenChunk())) 
	{
		switch(id = iload->CurChunkID())  
		{
			case MTL_HDR_CHUNK:
				res = MtlBase::Load(iload);
				break;
			case CQ2_HDR_CHUNK:
			{
				char* matname;
				res = iload->ReadCStringChunk(&matname);
				if( res == IO_OK )
				{
					m_matName = matname;
				}
				break;
			}
		}
		iload->CloseChunk();
		if (res!=IO_OK) 
			return res;
	}

	return IO_OK;
}


/*===========================================================================*\
 |	Updating and cloning
\*===========================================================================*/

RefTargetHandle CQ2Material::Clone(RemapDir &remap) 
{
	CQ2Material *mnew = new CQ2Material(FALSE);
	*((MtlBase*)mnew) = *((MtlBase*)this); 
	mnew->ReplaceReference(NSUBMTL,remap.CloneRef(pblock));

	mnew->ivalid.SetEmpty();	
	for (int i = 0; i<NSUBMTL; i++) 
	{
		mnew->submtl[i] = NULL;
		if (submtl[i])
			mnew->ReplaceReference(i,remap.CloneRef(submtl[i]));
		mnew->mapOn[i] = mapOn[i];
	}
	BaseClone(this, mnew, remap);
	return (RefTargetHandle)mnew;
}

void CQ2Material::NotifyChanged() 
{
	NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
}

void CQ2Material::Update(TimeValue t, Interval& valid) 
{
	if (!ivalid.InInterval(t)) 
	{

		ivalid.SetInfinite();

		for (int i=0; i<NSUBMTL; i++) 
		{
			if (submtl[i]) 
				submtl[i]->Update(t,ivalid);
		}
	}
	valid &= ivalid;
}

/*===========================================================================*\
 |	Determine the characteristics of the material
\*===========================================================================*/

void CQ2Material::SetAmbient(Color c, TimeValue t) {}		
void CQ2Material::SetDiffuse(Color c, TimeValue t) {}		
void CQ2Material::SetSpecular(Color c, TimeValue t) {}
void CQ2Material::SetShininess(float v, TimeValue t) {}
				
Color CQ2Material::GetAmbient(int mtlNum, BOOL backFace)
{
	return submtl[0]?submtl[0]->GetAmbient(mtlNum,backFace):Color(0,0,0);
}

Color CQ2Material::GetDiffuse(int mtlNum, BOOL backFace)
{
	return submtl[0]?submtl[0]->GetDiffuse(mtlNum,backFace):Color(0,0,0);
}

Color CQ2Material::GetSpecular(int mtlNum, BOOL backFace)
{
	return submtl[0]?submtl[0]->GetSpecular(mtlNum,backFace):Color(0,0,0);
}

float CQ2Material::GetXParency(int mtlNum, BOOL backFace)
{
	return submtl[0]?submtl[0]->GetXParency(mtlNum,backFace):0.0f;
}

float CQ2Material::GetShininess(int mtlNum, BOOL backFace)
{
	return submtl[0]?submtl[0]->GetShininess(mtlNum,backFace):0.0f;
}

float CQ2Material::GetShinStr(int mtlNum, BOOL backFace)
{
	return submtl[0]?submtl[0]->GetShinStr(mtlNum,backFace):0.0f;
}

float CQ2Material::WireSize(int mtlNum, BOOL backFace)
{
	return submtl[0]?submtl[0]->WireSize(mtlNum,backFace):0.0f;
}

		
/*===========================================================================*\
 |	Actual shading takes place
\*===========================================================================*/

void CQ2Material::Shade(ShadeContext& sc) 
{
	Mtl *sm1 = mapOn[0]?submtl[0]:NULL;
	if (gbufID) sc.SetGBufferID(gbufID);

	if(sm1) sm1->Shade(sc);
	
	// TODO: compute the color and transparency output returned in sc.out.
}

float CQ2Material::EvalDisplacement(ShadeContext& sc)
{
	Mtl *sm1 = mapOn[0]?submtl[0]:NULL;
	return (sm1)?sm1->EvalDisplacement(sc):0.0f;
}

Interval CQ2Material::DisplacementValidity(TimeValue t)
{
	Mtl *sm1 = mapOn[0]?submtl[0]:NULL;

	Interval iv; iv.SetInfinite();
	if(sm1) iv &= sm1->DisplacementValidity(t);

	return iv;	
}

//----------------------------------------------------------------------------------------------
// NEW code

void CQ2Material::assignTexture( const char* _filename, const char* _matname )
{
	// remember material's name
	m_matName = _matname;
	// does the file exist?
	if( ::GetFileAttributes(_filename) != 0XFFFFFFFF )
	{

		// need a material?
		if( !submtl[0] )
		{
			Color col(1.0f,1.0f,1.0f);

			StdMat *mtl = NewDefaultStdMat();
			mtl->SetName(_T("CQ2Material"));
			mtl->SetTwoSided(TRUE);
			mtl->SetAmbient(col, 0);
			mtl->SetDiffuse(col, 0);
			mtl->SetSpecular(col, 0);
			mtl->SetShininess(1.0, 0);
			mtl->SetShinStr(1.0, 0);
			mtl->SetOpacity(1.0, 0);
			mtl->SetMtlFlag(MTL_TEX_DISPLAY_ENABLED, true);

			// assigning submtl[0] to equal mtl
			SetSubMtl( 0, mtl );
		}

		// need a bitmap?
		if( !m_bitmapTex )
		{
			m_bitmapTex = NewDefaultBitmapTex();
			m_bitmapTex->SetMapName(_T((char*)_filename));
			m_bitmapTex->SetMtlFlag(MTL_TEX_DISPLAY_ENABLED, true);
			m_bitmapTex->SetAlphaSource(ALPHA_NONE);
		}
		else
		{
			m_bitmapTex->SetMapName(_T((char*)_filename));
			m_bitmapTex->ReloadBitmapAndUpdate();
		}

		// assign the bitmap to the material
		StdMat *mtl = (StdMat *)GetSubMtl(0);
		mtl->SetSubTexmap(ID_DI, m_bitmapTex);
		mtl->EnableMap(ID_DI, true);
		mtl->SetActiveTexmap(m_bitmapTex);

		// update viewport with new texture map
		GetCOREInterface()->ActivateTexture(m_bitmapTex, this);
		GetCOREInterface()->ForceCompleteRedraw();

		// notify all about the change
		NotifyChanged();
	}
}

//----------------------------------------------------------------------------------------------
// CQ2 Material Dlg Proc
//----------------------------------------------------------------------------------------------

BOOL CQ2DlgProc::DlgProc(TimeValue t, IParamMap2* map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if( msg == WM_INITDIALOG )
	{
		m_MainWnd = hWnd;
	}
	else if( msg == WM_COMMAND )
	{
		DWORD dwCode = HIWORD(wParam);
		DWORD dwId   = LOWORD(wParam);
		HWND  hWnd   = (HWND)lParam;

		if( m_CQ2Material )
		{
			if( dwId == IDC_BUTTON_APPLY)
			{
				HWND combo = GetDlgItem(m_MainWnd,IDC_COMBO_MATS);
				int  idx   = ComboBox_GetCurSel(combo);

				if( idx != CB_ERR )
				{
					int handle = ComboBox_GetItemData(combo,idx);

					const char* fn = CQ2MatEditor::GetFilename( (MatHandle)handle );
					const char* mn = CQ2MatEditor::GetMatname( (MatHandle)handle );
					if( fn )
					{
						m_CQ2Material->assignTexture( fn, mn );
						update();
					}
				}
			}
			else if( dwId == IDC_BUTTON_MATEDITOR)
			{
				CQ2MatEditor::StartEditor();
			}
			else if( dwId == IDC_BUTTON_PREVIEW )
			{
				MessageBox(hWnd, "IDC_BUTTON_PREVIEW", "IDC_BUTTON_PREVIEW", MB_OK );
			}
		}
	}
	else if( msg == WM_PAINT )
	{
		PAINTSTRUCT ps;
		Rect rect;
		HDC hdc = BeginPaint( hWnd, &ps );
		if (!IsRectEmpty(&ps.rcPaint)) 
		{
			GetClientRect( hWnd, &rect );
			draw( hdc, rect );
		}
		EndPaint( hWnd, &ps );
	}

	return 0;
}

void CQ2DlgProc::DeleteThis()
{
	// ??? do we need this ???
	// delete this;
}

void CQ2DlgProc::draw(HDC hdc, Rect &rect )
{
	// overload drawing here
}

void CQ2DlgProc::setup()
{
	if( m_CQ2Material )
	{
		CQ2MatEditor::Init( GetDlgItem(m_MainWnd,IDC_COMBO_MATS) );
		update();
	}
}

void CQ2DlgProc::update()
{
	if( m_CQ2Material->m_matName.size() )
	{
		const char* matname = m_CQ2Material->m_matName.c_str();
		MatHandle matHandle = CQ2MatEditor::SetMaterialName( matname );
		if( matHandle )
		{
			m_CQ2Material->m_matHandle = matHandle;
			CQ2MatEditor::FillOutMatProps( matHandle, GetDlgItem(m_MainWnd,IDC_EDIT_PROPERTIES), GetDlgItem(m_MainWnd,IDC_COMBO_MATS) );
		}
	}
}
