//----------------------------------------------------------------------------------------------
//
// MaterialManager.cpp 
//
//----------------------------------------------------------------------------------------------

#ifndef _MATERIAL_MANAGER_HEADER_FILE_H_
#define _MATERIAL_MANAGER_HEADER_FILE_H_

#include "dacom.h"

#include "da_d3dtypes.h"
#include "Vector.h"

//#include "rendpipeline.h"

struct IMaterial;

enum MatRegisterType
{
	MRT_CODE = 0,
	MRT_PIXEL,
	MRT_VERTEX
};

enum CQ_VertexType
{
	VT_STANDARD
};

#define D3DFVF_STANDARD (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX5)

struct StandardVertex
{
	Vector			pos;
	Vector			norm;
	U8 b, g, r, a;
	float			u,v;
	float			u2,v2;
	float			tanX, tanY;
	float			binX, binY;
	float			tanZ, binZ;
};


struct IMaterialCallback
{
	virtual void Added( IMaterial* ){}

	virtual void Changed( IMaterial* ){}

	virtual void Removed( IMaterial* ){}
};

struct IMaterial;

struct DACOM_NO_VTABLE IModifier
{
	virtual void SetNextModifier(IModifier * other) = 0;

	virtual IModifier * GetNextModifier() = 0;

	virtual IMaterial * GetMaterial() = 0;

	virtual void Release() = 0;

	virtual void AddRef() = 0;
};

struct DACOM_NO_VTABLE IMaterial
{
	virtual void DrawVB(IDirect3DVertexBuffer9* VB, IDirect3DIndexBuffer9* IB, int start_vertex, int num_verts, int start_index, int num_indices, IModifier * modList,SINGLE frameTime = 0.0f) = 0;
	
	virtual void DrawUP(void* vertices, U16* indices, int num_verts, int num_indices, IModifier * modList) = 0;

	virtual CQ_VertexType GetVertexType() = 0;

	virtual const char* GetDefaultBaseTexture( void ) = 0;

	virtual const char* GetName( void ) = 0;

	virtual SINGLE GetFloat(MatRegisterType type, U32 index) = 0;

	virtual SINGLE GetFloat(const char * name) = 0;

	virtual void SetFloat(MatRegisterType type, U32 index, SINGLE value) = 0;

	virtual void SetFloat(const char * name, SINGLE value) = 0;

	virtual S32 GetInt(MatRegisterType type, U32 index) = 0;

	virtual S32 GetInt(const char * name) = 0;

	virtual void SetInt(MatRegisterType type, U32 index, S32 value) = 0;

	virtual void SetInt(const char * name, S32 value) = 0;

	virtual void GetColor(MatRegisterType type, U32 index,U8 & red, U8 & green, U8 & blue) = 0;

	virtual void GetColor(const char * name,U8 & red, U8 & green, U8 & blue) = 0;

	virtual void SetColor(MatRegisterType type, U32 index, U8 red, U8 green, U8 blue) = 0;

	virtual void SetColor(const char * name, U8 red, U8 green, U8 blue) = 0;

	virtual const char * GetTextureName(U32 index) = 0;

	virtual const char * GetTextureName(const char * name) = 0;

	virtual void SetTextureName(U32 index, const char * newName) = 0;

	virtual void SetTextureName(const char * name, const char * newName) = 0;

	virtual DWORD GetRenderStateValue(U32 state) = 0;

	virtual DWORD GetRenderStateValue(const char * name) = 0;

	virtual void SetRenderStateValue(U32 state, DWORD value) = 0;

	virtual void SetRenderStateValue(const char * name, DWORD value) = 0;

	virtual U32 GetAnimUVColumns() = 0;

	virtual void SetAnimUVColumns(U32 numColumns) = 0;

	virtual U32 GetAnimUVRows() = 0;

	virtual void SetAnimUVRows(U32 numRows) = 0;

	virtual SINGLE GetAnimUVFrameRate() = 0;

	virtual void SetAnimUVFrameRate(SINGLE newFrameRate) = 0;

	virtual void Realize() = 0;

	virtual bool IsRealized() = 0;

	virtual IModifier * CreateModifierInt(char * name,S32 value,IModifier * oldModifier) = 0;

	virtual IModifier * CreateModifierFloat(char * name,SINGLE value,IModifier * oldModifier) = 0;

	virtual IModifier * CreateModifierColor(char * name,U8 red, U8 green, U8 blue,IModifier * oldModifier) = 0;

	virtual IModifier * CreateModifierRenderState(U32 state, DWORD value,IModifier * oldModifier) = 0;
};

struct DACOM_NO_VTABLE IMaterialManager : IDAComponent
{
	struct InitInfo
	{
		int dataThatWillInitMatManager;
		struct IFileSystem* MATDIR;
		struct IRenderPipeline* PIPE;
		struct ITManager* TMANAGER;
	};

	virtual GENRESULT COMAPI Initialize( InitInfo& ) = 0;

	virtual GENRESULT COMAPI OpenEditWindow( IMaterialCallback* callback = NULL ) = 0;

	virtual GENRESULT COMAPI IsEditWindowOpen( void ) = 0;

	virtual GENRESULT COMAPI FindFirstMaterial( IMaterial** ppMaterial ) = 0;

	virtual GENRESULT COMAPI FindNextMaterial( IMaterial* pThisMat, IMaterial** ppNextMat ) = 0;

	virtual IMaterial* FindMaterial( const char* name ) = 0;
};

#define IID_IMaterialManager MAKE_IID("MaterialManager",1)

#endif

