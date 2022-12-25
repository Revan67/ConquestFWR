//----------------------------------------------------------------------------------------------
//
// Material.cpp 
//
//----------------------------------------------------------------------------------------------

#include "stdafx.h"
#include <IMaterialManager.h>
#include "Material.h"
#include "InternalMaterialManager.h"
#include "Modifier.h"

#include "resource.h"

#include <tcomponent.h>
#include <da_heap_utility.h>
#include <filesys.h>
#include <commctrl.h>
#include <TSmartPointer.h>

#include <string>

#include <TManager.h>

char fullTexturePath[MAX_PATH];//used to return values out to the material in 3DMAX

/////////////////////////////////////////////////////////////////////////////////////////////
//IMaterial

void Material::DrawVB(IDirect3DVertexBuffer9* VB, IDirect3DIndexBuffer9* IB, int start_vertex, int num_verts, int start_index, int num_indices, IModifier * modList,SINGLE frameTime)
{
	UINT passCount = 1;
	if (shaderEffect)
	{
		SetShaderConstants(modList,frameTime);
		(*shaderEffect)->Begin(&passCount, 0);
	}
	else
	{
		passCount = 1;
	}
	for (U32 i = 0; i < passCount; i++)
	{
		PreparePass(i);
		if (IB)
		{
			owner->GetPipe()->draw_indexed_primitive_vb( D3DPT_TRIANGLELIST, (U32)VB, start_vertex, num_verts,  IB, num_indices, 0);
		}
		else
		{
			owner->GetPipe()->draw_primitive_vb( D3DPT_TRIANGLELIST, (U32) VB, start_vertex, num_verts, 0);
		}
		EndPass(i);
	}
	if (shaderEffect)
	{
		(*shaderEffect)->End();
	}
	
}

/////////////////////////////////////////////////////////////////////////////////////////////
// prepares to render a pass

void Material::SetShaderConstants(IModifier * modList,SINGLE frameTime)
{
	IRenderPipeline * PIPE = owner->GetPipe();

	// default renderstates, moved here from MeshRender.cpp
	PIPE->set_render_state(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);
	PIPE->set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX, 0);
	PIPE->set_render_state(D3DRS_LIGHTING,FALSE);
	PIPE->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	PIPE->set_render_state(D3DRS_ALPHATESTENABLE,FALSE);
	PIPE->set_render_state(D3DRS_ALPHAREF,0x0000000A);
	PIPE->set_render_state(D3DRS_CULLMODE,D3DCULL_CW);


	Modifier * mod = NULL;
	if(modList)
		mod = (Modifier *)modList;

	MatTexture* tmp = textureList;
	while (tmp)
	{
		PIPE->set_texture_stage_texture(tmp->channel, tmp->textureID);
		tmp = tmp->next;
	}

	MatState * sSearch = stateList;
	while(sSearch)
	{
		bool bPlaced = false;
		Modifier * modSearch = mod;
		while(modSearch)
		{
			if(modSearch->stateNode == sSearch)
			{
				PIPE->set_render_state((D3DRENDERSTATETYPE)(modSearch->stateValue),sSearch->value);
				bPlaced = true;
				break;
			}
			modSearch = modSearch->next;
		}
		if(!bPlaced)
		PIPE->set_render_state((D3DRENDERSTATETYPE)(sSearch->state),sSearch->value);
		sSearch = sSearch->next;
	}

	SINGLE overRideConst[256*4];
	if(vertexConst)
	{
		if(animUV)//if I am animating the UV coords
		{
			SINGLE columnSize = 1.0/animUV->columns;
			SINGLE rowSize = 1.0/animUV->rows;
			U32 frameCount = animUV->columns*animUV->rows;
			U32 currentFrame = ((U32)(frameTime*animUV->frameRate)) % frameCount;

			U32 currentRow = currentFrame%animUV->rows;
			U32 currentColumn = currentFrame/animUV->rows;

			SINGLE transX = currentColumn*columnSize;
			SINGLE transY = currentRow*rowSize;
			
			vertexConst[animUV->reg-startVertexIndex] = transX;
			vertexConst[(animUV->reg-startVertexIndex)+1] = transY;
		}
		bool bOverride = false;
        
		Modifier * modSearch = mod;
		while(modSearch)
		{
			if(modSearch->modType == Modifier::MT_FLOAT)
			{
				MatFloat * floatSearch = floatList[MRT_VERTEX];
				while(floatSearch)
				{
					if(floatSearch == modSearch->floatNode)
					{
						if(!bOverride)
							memcpy(overRideConst,vertexConst,vertexConstCount*sizeof(SINGLE));
						bOverride = true;
						overRideConst[floatSearch->reg-startVertexIndex] = modSearch->floatValue;
						break;
					}
					floatSearch = floatSearch->next;
				}
			}
			else if(modSearch->modType == Modifier::MT_INT)
			{
				MatInt * intSearch = intList[MRT_VERTEX];
				while(intSearch)
				{
					if(intSearch == modSearch->intNode)
					{
						if(!bOverride)
							memcpy(overRideConst,vertexConst,vertexConstCount*sizeof(SINGLE));
						bOverride = true;
						overRideConst[intSearch->reg-startVertexIndex] = modSearch->intValue;
						break;
					}
					intSearch = intSearch->next;
				}
			}
			else if(modSearch->modType == Modifier::MT_COLOR)
			{
				MatColor * colorSearch = colorList[MRT_VERTEX];
				while(colorSearch)
				{
					if(colorSearch == modSearch->colorNode)
					{
						if(!bOverride)
							memcpy(overRideConst,vertexConst,vertexConstCount*sizeof(SINGLE));
						bOverride = true;
						overRideConst[colorSearch->reg-startVertexIndex] = modSearch->colorValue.red/255.0;
						overRideConst[(colorSearch->reg+1)-startVertexIndex] = modSearch->colorValue.green/255.0;
						overRideConst[(colorSearch->reg+2)-startVertexIndex] = modSearch->colorValue.blue/255.0;
						break;
					}
					colorSearch = colorSearch->next;
				}
			}
			modSearch = modSearch->next;
		}

		if(bOverride)
		{
			PIPE->set_vs_constants(startVertexIndex/4, (float*)overRideConst,vertexConstCount/4);
		}
		else
		{
			PIPE->set_vs_constants(startVertexIndex/4, (float*)vertexConst,vertexConstCount/4);
		}
	}
	if(pixelConst)
	{
		bool bOverride = false;
        
		Modifier * modSearch = mod;
		while(modSearch)
		{
			if(modSearch->modType == Modifier::MT_FLOAT)
			{
				MatFloat * floatSearch = floatList[MRT_PIXEL];
				while(floatSearch)
				{
					if(floatSearch == modSearch->floatNode)
					{
						if(!bOverride)
							memcpy(overRideConst,pixelConst,pixelConstCount*sizeof(SINGLE));
						bOverride = true;
						overRideConst[floatSearch->reg-startPixelIndex] = modSearch->floatValue;
						break;
					}
					floatSearch = floatSearch->next;
				}
			}
			else if(modSearch->modType == Modifier::MT_INT)
			{
				MatInt * intSearch = intList[MRT_PIXEL];
				while(intSearch)
				{
					if(intSearch == modSearch->intNode)
					{
						if(!bOverride)
							memcpy(overRideConst,pixelConst,pixelConstCount*sizeof(SINGLE));
						bOverride = true;
						overRideConst[intSearch->reg-startPixelIndex] = modSearch->intValue;
						break;
					}
					intSearch = intSearch->next;
				}
			}
			else if(modSearch->modType == Modifier::MT_COLOR)
			{
				MatColor * colorSearch = colorList[MRT_PIXEL];
				while(colorSearch)
				{
					if(colorSearch == modSearch->colorNode)
					{
						if(!bOverride)
							memcpy(overRideConst,pixelConst,pixelConstCount*sizeof(SINGLE));
						bOverride = true;
						overRideConst[colorSearch->reg-startPixelIndex] = modSearch->colorValue.red/255.0;
						overRideConst[(colorSearch->reg+1)-startPixelIndex] = modSearch->colorValue.green/255.0;
						overRideConst[(colorSearch->reg+2)-startPixelIndex] = modSearch->colorValue.blue/255.0;
						break;
					}
					colorSearch = colorSearch->next;
				}
			}
			modSearch = modSearch->next;
		}

		if(bOverride)
		{
			PIPE->set_ps_constants(startPixelIndex/4, (float*)overRideConst,pixelConstCount/4);
		}
		else
		{
			PIPE->set_ps_constants(startPixelIndex/4, (float*)pixelConst,pixelConstCount/4);
		}
	}
}

bool Material::PreparePass(int pass)
{
	//old version of directX needed this.
	if (shaderEffect)
	{
		(*shaderEffect)->BeginPass(pass);
	}
	
	switch (pass)
	{
	case 0:
		{
			// set initial renderstates
			return true;
		}
	case 1: 
		{
			return false;
		}
	}
	return false;
}

bool Material::EndPass(int pass)
{
	//old version of directX needed this.
	if (shaderEffect)
	{
		(*shaderEffect)->EndPass();
	}
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Material::DrawUP(void* vertices, U16* indices, int num_verts, int num_indices, IModifier * modList)
{
	
}

/////////////////////////////////////////////////////////////////////////////////////////////
CQ_VertexType Material::GetVertexType()
{
	return VT_STANDARD;
}

/////////////////////////////////////////////////////////////////////////////////////////////


const char* Material::GetDefaultBaseTexture( void ) 
{ 
	MatTexture * search = textureList;
	MatTexture * best = NULL;
	while(search)
	{
		if(!best)
			best = search;
		else if(best->channel > search->channel)
		{
			best = search;
		}
		search = search->next;
	}
	if(best)
	{
		char externalPath[512];
		owner->GetDirectory()->GetFileName(externalPath,511);

		sprintf(fullTexturePath,"%s\\Textures\\%s",externalPath,best->filename);
		return fullTexturePath;
	}
	return ""; 
}

/////////////////////////////////////////////////////////////////////////////////////////////
//

const char* Material::GetName( void )
{ 
	return szFileName.c_str(); 
}

/////////////////////////////////////////////////////////////////////////////////////////////
//

SINGLE Material::GetFloat(MatRegisterType type, U32 index)
{
	MatFloat * search = floatList[type];
	while(search)
	{
		if(search->reg == index)
			return search->value;
		search = search->next;
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//

SINGLE Material::GetFloat(const char * name)
{
	for(U32 i = 0; i < 3; ++i)
	{
		MatFloat * search = floatList[i];
		while(search)
		{
			if(strcmp(search->name,name) == 0)
				return search->value;
			search = search->next;
		}
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//

void Material::SetFloat(MatRegisterType type, U32 index, SINGLE value)
{
	MatFloat * search = floatList[type];
	while(search)
	{
		if(search->reg == index)
		{
			bChanged = true;
			search->value = value;
			if(bRealized)
				Realize();
			return;
		}
		search = search->next;
	}
	return;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//

void Material::SetFloat(const char * name, SINGLE value)
{
	for(U32 i = 0; i < 3; ++i)
	{
		MatFloat * search = floatList[i];
		while(search)
		{
			if(strcmp(search->name,name) == 0)
			{
				bChanged = true;
				search->value = value;
				if(bRealized)
					Realize();
				return;
			}
			search = search->next;
		}
	}
	return;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//

S32 Material::GetInt(MatRegisterType type, U32 index)
{
	MatInt * search = intList[type];
	while(search)
	{
		if(search->reg == index)
			return search->value;
		search = search->next;
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//

S32 Material::GetInt(const char * name)
{
	for(U32 i = 0; i < 3; ++i)
	{
		MatInt * search = intList[i];
		while(search)
		{
			if(strcmp(search->name,name) == 0)
				return search->value;
			search = search->next;
		}
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//

void Material::SetInt(MatRegisterType type, U32 index, S32 value)
{
	MatInt * search = intList[type];
	while(search)
	{
		if(search->reg == index)
		{
			bChanged = true;
			search->value = value;
			if(bRealized)
				Realize();
			return;
		}
		search = search->next;
	}
	return;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//

void Material::SetInt(const char * name, S32 value)
{
	for(U32 i = 0; i < 3; ++i)
	{
		MatInt * search = intList[i];
		while(search)
		{
			if(strcmp(search->name,name) == 0)
			{
				bChanged = true;
				search->value = value;
				if(bRealized)
					Realize();
				return;
			}
			search = search->next;
		}
	}
	return;
}
/////////////////////////////////////////////////////////////////////////////////////////////
//

void Material::GetColor(MatRegisterType type, U32 index,U8 & red, U8 & green, U8 & blue)
{
	MatColor * search = colorList[type];
	while(search)
	{
		if(search->reg == index)
		{
			red = search->red;
			green = search->green;
			blue = search->blue;
			return;
		}
		search = search->next;
	}
	red = 255;
	green = 255;
	blue = 255;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//

void Material::GetColor(const char * name,U8 & red, U8 & green, U8 & blue)
{
	for(U32 i = 0; i < 3; ++i)
	{
		MatColor * search = colorList[i];
		while(search)
		{
			if(strcmp(search->name,name) == 0)
			{
				red = search->red;
				green = search->green;
				blue = search->blue;
				return;
			}
			search = search->next;
		}
	}
	red = 255;
	green = 255;
	blue = 255;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//

void Material::SetColor(MatRegisterType type, U32 index, U8 red, U8 green, U8 blue)
{
	MatColor * search = colorList[type];
	while(search)
	{
		if(search->reg == index)
		{
			bChanged = true;
			search->red = red;
			search->green = green;
			search->blue = blue;
			if(bRealized)
				Realize();
			return;
		}
		search = search->next;
	}
	return;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//

void Material::SetColor(const char * name, U8 red, U8 green, U8 blue)
{
	for(U32 i = 0; i < 3; ++i)
	{
		MatColor * search = colorList[i];
		while(search)
		{
			if(strcmp(search->name,name) == 0)
			{
				bChanged = true;
				search->red = red;
				search->green = green;
				search->blue = blue;
				if(bRealized)
					Realize();
				return;
			}
			search = search->next;
		}
	}
	return;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//

const char * Material::GetTextureName(U32 index)
{
	MatTexture * search = textureList;
	while(search)
	{
		if(search->channel == index)
		{
			return search->filename;
		}
		search = search->next;
	}
	return "";
}

/////////////////////////////////////////////////////////////////////////////////////////////
//

const char * Material::GetTextureName(const char * name)
{
	MatTexture * search = textureList;
	while(search)
	{
		if(strcmp(search->name,name) == 0)
		{
			return search->filename;
		}
		search = search->next;
	}
	return "";
}

/////////////////////////////////////////////////////////////////////////////////////////////
//

void Material::SetTextureName(U32 index, const char * newName)
{
	MatTexture * search = textureList;
	while(search)
	{
		if(search->channel == index)
		{
			bChanged = true;
			strcpy(search->filename,newName);
			if(bRealized)
				Realize();
			return ;
		}
		search = search->next;
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////
//

void Material::SetTextureName(const char * name, const char * newName)
{
	MatTexture * search = textureList;
	while(search)
	{
		if(strcmp(search->name,name) == 0)
		{
			bChanged = true;
			strcpy(search->filename,newName);
			if(bRealized)
				Realize();
			return ;
		}
		search = search->next;
	}
	if(owner->GetCallback())
		owner->GetCallback()->Changed(this);
}

/////////////////////////////////////////////////////////////////////////////////////////////
//

DWORD Material::GetRenderStateValue(U32 state)
{
	MatState * search = stateList;
	while(search)
	{
		if(search->state == state)
		{
			return search->value;
		}
		search = search->next;
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//

DWORD Material::GetRenderStateValue(const char * name)
{
	MatState * search = stateList;
	while(search)
	{
		if(strcmp(search->name,name) == 0)
		{
			return search->value;
		}
		search = search->next;
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//

void Material::SetRenderStateValue(U32 state, DWORD value)
{
	MatState * search = stateList;
	while(search)
	{
		if(search->state == state)
		{
			bChanged = true;
			search->value = value;
			if(bRealized)
				Realize();
			return;
		}
		search = search->next;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
//

void Material::SetRenderStateValue(const char * name, DWORD value)
{
	MatState * search = stateList;
	while(search)
	{
		if(strcmp(search->name,name) == 0)
		{
			bChanged = true;
			search->value = value;
			if(bRealized)
				Realize();
			return;
		}
		search = search->next;
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////
//
U32 Material::GetAnimUVColumns()
{
	if(animUV)
	{
		return animUV->columns;
	}
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
void Material::SetAnimUVColumns(U32 numColumns)
{
	if(animUV)
	{
		animUV->columns = numColumns;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
U32 Material::GetAnimUVRows()
{
	if(animUV)
	{
		return animUV->rows;
	}
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
void Material::SetAnimUVRows(U32 numRows)
{
	if(animUV)
	{
		animUV->rows = numRows;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
SINGLE Material::GetAnimUVFrameRate()
{
	if(animUV)
	{
		return animUV->frameRate;
	}
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
void Material::SetAnimUVFrameRate(SINGLE newFrameRate)
{
	if(animUV)
	{
		animUV->frameRate = newFrameRate;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
//

void Material::Realize()
{
	// load the textures!

	if(pixelConst)
	{
		delete [] pixelConst;
		pixelConst = NULL;
	}
	if(vertexConst)
	{
		delete [] vertexConst;
		vertexConst = NULL;
	}
	bRealized = true;
	if(shader)
	{
		shaderEffect = owner->GetPipe()->load_effect(shader->GetFilename(),owner->GetShaderDir());

		ITManager* TMANAGER = owner->GetTManager();
		MatTexture* tmp = textureList;
		while (tmp)
		{
			if(tmp->filename[0])
			{
				{
					tmp->textureID = TMANAGER->CreateTextureFromFile(tmp->filename, (IComponentFactory *)owner->GetTextureDir(), DA::TGA, PF_4CC_DAA8);
				}
			}
			else
				tmp->textureID = 0;
			tmp = tmp->next;
		}

		// compile the VS constants
		{
			U32 firstIndex = 90000;
			U32 lastIndex = 0;

			MatFloat * fSearch = floatList[MRT_VERTEX];
			while(fSearch)
			{
				if(fSearch->reg > lastIndex)
					lastIndex = fSearch->reg;
				if(fSearch->reg < firstIndex)
					firstIndex = fSearch->reg;
				fSearch = fSearch->next;
			}

			MatInt * iSearch = intList[MRT_VERTEX];
			while(iSearch)
			{
				if(iSearch->reg > lastIndex)
					lastIndex = iSearch->reg;
				if(iSearch->reg < firstIndex)
					firstIndex = iSearch->reg;
				iSearch = iSearch->next;
			}

			MatColor * cSearch = colorList[MRT_VERTEX];
			while(cSearch)
			{
				if(cSearch->reg+2 > lastIndex)
					lastIndex = cSearch->reg+2;
				if(cSearch->reg < firstIndex)
					firstIndex = cSearch->reg;
				cSearch = cSearch->next;
			}

			if(animUV)
			{
				if(animUV->reg+1 > lastIndex)
					lastIndex = animUV->reg+1;
				if(animUV->reg < firstIndex)
					firstIndex = animUV->reg;
			}

			if(lastIndex >= firstIndex)
			{
				U32 mod = firstIndex %4;
				firstIndex = firstIndex-mod;
				mod = lastIndex%4;
				lastIndex = lastIndex+(4-mod);
				vertexConstCount = (lastIndex-firstIndex);
				startVertexIndex = firstIndex;
				vertexConst = new SINGLE[vertexConstCount];
				memset(vertexConst,0,vertexConstCount*sizeof(SINGLE));

				MatFloat * fSearch = floatList[MRT_VERTEX];
				while(fSearch)
				{
					vertexConst[fSearch->reg-startVertexIndex] = fSearch->value;
					fSearch = fSearch->next;
				}

				MatInt * iSearch = intList[MRT_VERTEX];
				while(iSearch)
				{
					vertexConst[iSearch->reg-startVertexIndex] = iSearch->value;
					iSearch = iSearch->next;
				}

				MatColor * cSearch = colorList[MRT_VERTEX];
				while(cSearch)
				{
					vertexConst[cSearch->reg-startVertexIndex] = cSearch->red/255.0;
					vertexConst[(cSearch->reg+1)-startVertexIndex] = cSearch->green/255.0;
					vertexConst[(cSearch->reg+2)-startVertexIndex] = cSearch->blue/255.0;
					cSearch = cSearch->next;
				}
			}
			else
			{
				vertexConstCount = 0;
				vertexConst =NULL;
				startVertexIndex = 0;
			}
		}

		// compile the PS constants
		{
			U32 firstIndex = 90000;
			U32 lastIndex = 0;

			MatFloat * fSearch = floatList[MRT_PIXEL];
			while(fSearch)
			{
				if(fSearch->reg > lastIndex)
					lastIndex = fSearch->reg;
				if(fSearch->reg < firstIndex)
					firstIndex = fSearch->reg;
				fSearch = fSearch->next;
			}

			MatInt * iSearch = intList[MRT_PIXEL];
			while(iSearch)
			{
				if(iSearch->reg > lastIndex)
					lastIndex = iSearch->reg;
				if(iSearch->reg < firstIndex)
					firstIndex = iSearch->reg;
				iSearch = iSearch->next;
			}

			MatColor * cSearch = colorList[MRT_PIXEL];
			while(cSearch)
			{
				if(cSearch->reg+2 > lastIndex)
					lastIndex = cSearch->reg+2;
				if(cSearch->reg < firstIndex)
					firstIndex = cSearch->reg;
				cSearch = cSearch->next;
			}

			if(lastIndex >= firstIndex)
			{
				U32 mod = firstIndex %4;
				firstIndex = firstIndex-mod;
				mod = lastIndex%4;
				lastIndex = lastIndex+(4-mod);
				pixelConstCount = (lastIndex-firstIndex);
				startPixelIndex = firstIndex;
				pixelConst = new SINGLE[pixelConstCount];
				memset(pixelConst,0,pixelConstCount*sizeof(SINGLE));

				MatFloat * fSearch = floatList[MRT_PIXEL];
				while(fSearch)
				{
					pixelConst[fSearch->reg-startPixelIndex] = fSearch->value;
					fSearch = fSearch->next;
				}

				MatInt * iSearch = intList[MRT_PIXEL];
				while(iSearch)
				{
					pixelConst[iSearch->reg-startPixelIndex] = iSearch->value;
					iSearch = iSearch->next;
				}

				MatColor * cSearch = colorList[MRT_PIXEL];
				while(cSearch)
				{
					pixelConst[cSearch->reg-startPixelIndex] = cSearch->red/255.0;
					pixelConst[(cSearch->reg+1)-startPixelIndex] = cSearch->green/255.0;
					pixelConst[(cSearch->reg+2)-startPixelIndex] = cSearch->blue/255.0;
					cSearch = cSearch->next;
				}
			}
			else
			{
				pixelConstCount = 0;
				pixelConst =NULL;
				startPixelIndex = 0;
			}
		}
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////
//

bool Material::IsRealized()
{
	return bRealized;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//

IModifier * Material::CreateModifierInt(char * name,S32 value,IModifier * oldModifier)
{
	Modifier * mod = NULL;
	if(oldModifier)
		mod = (Modifier *)oldModifier;
	else 
		mod = new Modifier(owner);
	mod->material = this;
	mod->intValue = value;
	mod->intNode = NULL;
	mod->modType = Modifier::MT_INT;
	for(U32 i = 0; i < 3; ++i)
	{
		MatInt * search = intList[i];
		while(search)
		{
			if(strcmp(search->name,name) == 0)
			{
				mod->intNode = search;
				return mod;
			}
			search = search->next;
		}
	}
	return mod;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//

IModifier * Material::CreateModifierFloat(char * name,SINGLE value,IModifier * oldModifier)
{
	Modifier * mod = NULL;
	if(oldModifier)
		mod = (Modifier *)oldModifier;
	else 
		mod = new Modifier(owner);
	mod->material = this;
	mod->floatValue = value;
	mod->floatNode = NULL;
	mod->modType = Modifier::MT_FLOAT;
	for(U32 i = 0; i < 3; ++i)
	{
		MatFloat * search = floatList[i];
		while(search)
		{
			if(strcmp(search->name,name) == 0)
			{
				mod->floatNode = search;
				return mod;
			}
			search = search->next;
		}
	}
	return mod;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//

IModifier * Material::CreateModifierColor(char * name,U8 red, U8 green, U8 blue,IModifier * oldModifier)
{
	Modifier * mod = NULL;
	if(oldModifier)
		mod = (Modifier *)oldModifier;
	else 
		mod = new Modifier(owner);
	mod->material = this;
	mod->colorValue.red = red;
	mod->colorValue.green = green;
	mod->colorValue.blue = blue;
	mod->colorNode = NULL;
	mod->modType = Modifier::MT_COLOR;
	for(U32 i = 0; i < 3; ++i)
	{
		MatColor * search = colorList[i];
		while(search)
		{
			if(strcmp(search->name,name) == 0)
			{
				mod->colorNode = search;
				return mod;
			}
			search = search->next;
		}
	}
	return mod;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//

IModifier * Material::CreateModifierRenderState(U32 state, DWORD value,IModifier * oldModifier)
{
	Modifier * mod = NULL;
	if(oldModifier)
		mod = (Modifier *)oldModifier;
	else 
		mod = new Modifier(owner);
	mod->material = this;
	mod->stateValue = value;
	mod->stateNode = NULL;
	mod->modType = Modifier::MT_RENDERSTATE;
	MatState * search = stateList;
	while(search)
	{
		if(search->state == state)
		{
			mod->stateNode = search;
			return mod;
		}
		search = search->next;
	}
	return mod;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//Material

Material::Material(IInternalMaterialManager * _owner)
{
	shaderEffect = NULL;
	bChanged = true;
	bRealized = false;
	owner = _owner;
	shader = NULL;
	floatList[0] = NULL;
	floatList[1] = NULL;
	floatList[2] = NULL;
	intList[0] = NULL;
	intList[1] = NULL;
	intList[2] = NULL;
	colorList[0] = NULL;
	colorList[1] = NULL;
	colorList[2] = NULL;
	textureList = NULL;
	stateList = NULL;
	animUV = NULL;

	pixelConst = NULL;
	startPixelIndex = 0;
	pixelConstCount = 0;

	vertexConst = NULL;
	startVertexIndex = 0;
	vertexConstCount = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
Material::Material(const Material & source)
{
	shaderEffect = NULL;
	szMaterialName = source.szMaterialName;
	szFileName = source.szFileName;
	szShaderName = source.szShaderName;
	shader = source.shader;
	owner = source.owner;

	floatList[0] = NULL;
	floatList[1] = NULL;
	floatList[2] = NULL;

	for(U32 i = 0; i < 3; ++i)
	{
		MatFloat * search = source.floatList[i];
		while(search)
		{
			MatFloat * copy = new MatFloat(*search);
			copy->next = floatList[i];
			floatList[i] = copy;
			search = search->next;
		}
	}

	intList[0] = NULL;
	intList[1] = NULL;
	intList[2] = NULL;
	for(U32 i = 0; i < 3; ++i)
	{
		MatInt * search = source.intList[i];
		while(search)
		{
			MatInt * copy = new MatInt(*search);
			copy->next = intList[i];
			intList[i] = copy;
			search = search->next;
		}
	}

	colorList[0] = NULL;
	colorList[1] = NULL;
	colorList[2] = NULL;
	for(U32 i = 0; i < 3; ++i)
	{
		MatColor * search = source.colorList[i];
		while(search)
		{
			MatColor * copy = new MatColor(*search);
			copy->next = colorList[i];
			colorList[i] = copy;
			search = search->next;
		}
	}

	textureList = NULL;
	MatTexture * search = source.textureList;
	while(search)
	{
		MatTexture * copy = new MatTexture(*search);
		copy->next = textureList;
		textureList = copy;
		search = search->next;
	}

	stateList = NULL;
	MatState * stateSearch = source.stateList;
	while(stateSearch)
	{
		MatState * copy = new MatState(*stateSearch);
		copy->next = stateList;
		stateList = copy;
		stateSearch = stateSearch->next;
	}

	animUV = NULL;
	if(source.animUV)
	{
		animUV = new MatAnimUV(*(source.animUV));
	}

	bChanged = source.bChanged;

	pixelConst = NULL;
	startPixelIndex = 0;
	pixelConstCount = 0;

	vertexConst = NULL;
	startVertexIndex = 0;
	vertexConstCount = 0;

	bRealized = false;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//

Material::~Material()
{
	for(U32 i = 0; i < 3; ++i)
	{
		while(floatList[i])
		{
			MatFloat * tmp = floatList[i];
			floatList[i] = floatList[i]->next;
			delete tmp;
		}
		while(intList[i])
		{
			MatInt * tmp = intList[i];
			intList[i] = intList[i]->next;
			delete tmp;
		}
		while(colorList[i])
		{
			MatColor * tmp = colorList[i];
			colorList[i] = colorList[i]->next;
			delete tmp;
		}
	}

	while(textureList)
	{
		MatTexture * tmp = textureList;
		textureList = textureList->next;
		delete tmp;
	}

	while(stateList)
	{
		MatState * tmp = stateList;
		stateList = stateList->next;
		delete tmp;
	}
	if(pixelConst)
	{
		delete [] pixelConst;
		pixelConst = NULL;
	}
	if(vertexConst)
	{
		delete [] vertexConst;
		vertexConst = NULL;
	}

	if(animUV)
	{
		delete animUV;
		animUV = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
//

#define MAT_SAVE_VERSION 1

void readLine(char * buffer, IFileSystem * file)
{
	char read;
	int index = 0;
	U32 dwBytesRead = 0;

	while((file->ReadFile(NULL, &read, 1, &dwBytesRead) == 1) && index < 254)
	{
		if(read == '\n')
			break;
		buffer[index] = read;
		++index;
	}
	buffer[index] = 0;
};

#define MS_END "END"
#define MS_FLOAT "FLOAT"
#define MS_INT "INT"
#define MS_COLOR "COLOR"
#define MS_TEXTURE "TEXTURE"
#define MS_STATE "STATE"
#define MS_ANIMUV "ANIMUV"

void Material::Load(IFileSystem * matDir, const char * filename)
{
	szFileName = filename;
	char buffer[255];
	const char * strEnd = strrchr(filename,'.');
	strncpy(buffer,filename,(strEnd-filename));
	buffer[strEnd-filename] = 0;
	szMaterialName = buffer;

	DAFILEDESC desc = filename;
	COMPTR<IFileSystem> matFile;
	if(matDir->CreateInstance(&desc,matFile) == GR_OK)
	{
		readLine(buffer,matFile);
		SINGLE version = (SINGLE)(atof(buffer));
		if(version != 0)
		{
			readLine(buffer,matFile);
			szShaderName = buffer;
			shader = owner->FindShader(szShaderName.c_str());
			//now read parameter info
			readLine(buffer,matFile);
			while(strncmp(buffer,MS_END,strlen(MS_END)) != 0)
			{
				if(strncmp(buffer,MS_FLOAT,strlen(MS_FLOAT))==0 )
				{
					char token[64];
					//FLOAT <regType> <value> <register> "<name>"
					MatFloat * newFloat = new MatFloat;
					U32 regType;
					sscanf(buffer,"%s %d %f %d \"%[^\"]",token,&regType,&(newFloat->value),&(newFloat->reg),newFloat->name);
					newFloat->next = floatList[regType];
					floatList[regType] = newFloat;
				}
				else if(strncmp(buffer,MS_INT,strlen(MS_INT))==0 )
				{
					char token[64];
					//INT <regType> <value> <register> "<name>"
					MatInt * newInt = new MatInt;
					U32 regType;
					sscanf(buffer,"%s %d %d %d \"%[^\"]",token,&regType,&(newInt->value),&(newInt->reg),newInt->name);
					newInt->next = intList[regType];
					intList[regType] = newInt;
				}
				else if(strncmp(buffer,MS_COLOR,strlen(MS_COLOR))==0 )
				{
					char token[64];
					//COLOR <regType> <valueRed> <valueGreen> <valueBlue> <register> "<name>"
					MatColor * newColor = new MatColor;
					U32 regType;
					U32 red;
					U32 green;
					U32 blue;
					sscanf(buffer,"%s %d %d %d %d %d \"%[^\"]",token,&regType,&(red),&(green),&(blue),&(newColor->reg),newColor->name);
					newColor->red = (U8)red;
					newColor->green = (U8)green;
					newColor->blue = (U8)blue;
					newColor->next = colorList[regType];
					colorList[regType] = newColor;
				}
				else if(strncmp(buffer,MS_TEXTURE,strlen(MS_TEXTURE))==0 )
				{
					char token[64];
					//TEXTURE <channel> "<filename>" "<name>"
					MatTexture * newTexture = new MatTexture;
					sscanf(buffer,"%s %d \"%[^\"]\" \"%[^\"]",token,&(newTexture->channel),newTexture->filename,newTexture->name);
					newTexture->next = textureList;
					textureList = newTexture;
				}
				else if(strncmp(buffer,MS_STATE,strlen(MS_STATE))==0 )
				{
					char token[64];
					//STATE <value> <state> "<name>"
					MatState * newState = new MatState;
					sscanf(buffer,"%s %d %d \"%[^\"]",token,&(newState->value),&(newState->state),newState->name);
					newState->next = stateList;
					stateList = newState;
				}
				else if(strncmp(buffer,MS_ANIMUV,strlen(MS_ANIMUV))==0 )
				{
					char token[64];
					//ANIMUV <register> <rows> <columns> <frameRate> "<name>"
					animUV = new MatAnimUV;
					sscanf(buffer,"%s %d %d %d %f \"%[^\"]",token,&(animUV->reg),&(animUV->rows),&(animUV->columns),&(animUV->frameRate),animUV->name);
				}
				readLine(buffer,matFile);
			}
		}
		RemapParameters();
	}
	bChanged = false;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//

void Material::Save(IFileSystem * matDir)
{
	if(bChanged)
	{
		DAFILEDESC desc = szFileName.c_str();
		desc.dwCreationDistribution = CREATE_ALWAYS;
		desc.dwDesiredAccess = GENERIC_WRITE;
		COMPTR<IFileSystem> matFile;
		if(matDir->CreateInstance(&desc,matFile) == GR_OK)
		{
			bChanged = false;
			U32 dwWriten;
			char buffer[255];
			sprintf(buffer,"%d\n",MAT_SAVE_VERSION);
			matFile->WriteFile(NULL,buffer,strlen(buffer),&dwWriten);

			sprintf(buffer,"%s\n",szShaderName.c_str());
			matFile->WriteFile(NULL,buffer,strlen(buffer),&dwWriten);

			//save parameter info

			//save floats
			for(U32 i = 0; i < 3; ++i)
			{
				MatFloat * search = floatList[i];
				while(search)
				{
					//FLOAT <regType> <value> <register> "<name>"
					sprintf(buffer,"%s %d %f %d \"%s\"\n",MS_FLOAT,i,search->value,search->reg,search->name);
					matFile->WriteFile(NULL,buffer,strlen(buffer),&dwWriten);
					search = search->next;
				}
			}

			//save ints
			for(U32 i = 0; i < 3; ++i)
			{
				MatInt * search = intList[i];
				while(search)
				{
					//INT <regType> <value> <register> "<name>"
					sprintf(buffer,"%s %d %d %d \"%s\"\n",MS_INT,i,search->value,search->reg,search->name);
					matFile->WriteFile(NULL,buffer,strlen(buffer),&dwWriten);
					search = search->next;
				}
			}

			//save color
			for(U32 i = 0; i < 3; ++i)
			{
				MatColor * search = colorList[i];
				while(search)
				{
					//COLOR <regType> <valueRed> <valueGreen> <valueBlue> <register> "<name>"
					sprintf(buffer,"%s %d %d %d %d %d \"%s\"\n",MS_COLOR,i,search->red,search->green,search->blue,search->reg,search->name);
					matFile->WriteFile(NULL,buffer,strlen(buffer),&dwWriten);
					search = search->next;
				}
			}

			//save texture
			MatTexture * texSearch = textureList;
			while(texSearch)
			{
				//TEXTURE <channel> "<filename>" "<name>"
				sprintf(buffer,"%s %d \"%s\" \"%s\"\n",MS_TEXTURE,texSearch->channel,texSearch->filename,texSearch->name);
				matFile->WriteFile(NULL,buffer,strlen(buffer),&dwWriten);
				texSearch = texSearch->next;
			}

			//save state
			MatState * stateSearch = stateList;
			while(stateSearch)
			{
				//STATE <value> <state> "<name>"
				sprintf(buffer,"%s %d %d \"%s\"\n",MS_STATE,stateSearch->value,stateSearch->state,stateSearch->name);
				matFile->WriteFile(NULL,buffer,strlen(buffer),&dwWriten);
				stateSearch = stateSearch->next;
			}

			//save animUV
			if(animUV)
			{
				//ANIMUV <register> <rows> <columns> <frameRate> "<name>"
				sprintf(buffer,"%s %d %d %d %f \"%s\"\n",MS_ANIMUV,animUV->reg,animUV->rows,animUV->columns,animUV->frameRate,animUV->name);
				matFile->WriteFile(NULL,buffer,strlen(buffer),&dwWriten);
			}

			//end tag
			sprintf(buffer,"%s\n",MS_END);
			matFile->WriteFile(NULL,buffer,strlen(buffer),&dwWriten);
		}
		else
		{
			char buffer[512];
			sprintf(buffer,"Could not create save file for %s",szFileName.c_str());
			::MessageBox(NULL,buffer,"Save Error",MB_OK);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
//

void Material::Rename(const char * filename)
{
	bChanged = true;
	szFileName = filename;
	char buffer[255];
	const char * strEnd = strrchr(filename,'.');
	strncpy(buffer,filename,(strEnd-filename));
	buffer[strEnd-filename] = 0;
	szMaterialName = buffer;
	if(owner->GetCallback())
		owner->GetCallback()->Changed(this);
	if(bRealized)
		Realize();
}

/////////////////////////////////////////////////////////////////////////////////////////////
//

void Material::SetShader(struct IShader * newShader)
{
	shader = newShader;
	if(shader)
	{
		if(strcmp(szShaderName.c_str(),shader->GetName()))
			bChanged = true;
		szShaderName = shader->GetName();
	}
	else
	{
		if(strcmp(szShaderName.c_str(),""))
			bChanged = true;
		szShaderName = "";
	}

	RemapParameters();
	if(owner->GetCallback())
		owner->GetCallback()->Changed(this);

	if(bRealized)
		Realize();
}

/////////////////////////////////////////////////////////////////////////////////////////////
//

void Material::RemapParameters()
{
	//set up new lists
	MatFloat * newFloatList[3];
	newFloatList[0] = 0;
	newFloatList[1] = 0;
	newFloatList[2] = 0;

	MatInt * newIntList[3];
	newIntList[0] = 0;
	newIntList[1] = 0;
	newIntList[2] = 0;

	MatColor * newColorList[3];
	newColorList[0] = 0;
	newColorList[1] = 0;
	newColorList[2] = 0;

	MatTexture * newTextureList = NULL;

	MatState * newStateList = NULL;

	MatAnimUV * newAnimUV = NULL;

	//fill out new lists

	if(shader)
	{
		DataNode * search = shader->GetDataList();
		while(search)
		{
			if(search->dataType == MDT_FLOAT)
			{
				MatFloat * node = new MatFloat();
				node->reg = search->data.floatData.rValue;
				node->value = search->data.floatData.def;
				strcpy(node->name,search->name);

				//look for old values
				MatFloat * old = floatList[search->data.floatData.rType];
				while(old)
				{
					if(strcmp(old->name,node->name) == 0)
					{
						if(old->value > search->data.floatData.max)
							node->value = search->data.floatData.max;
						else if(old->value < search->data.floatData.min)
							node->value = search->data.floatData.min;
						else
							node->value = old->value;
						break;
					}
					old = old->next;
				}

				node->next = newFloatList[search->data.floatData.rType];
				newFloatList[search->data.floatData.rType] = node;
			}
			else if(search->dataType == MDT_INT)
			{
				MatInt * node = new MatInt();
				node->reg = search->data.intData.rValue;
				node->value = search->data.intData.def;
				strcpy(node->name,search->name);

				//look for old values
				MatInt * old = intList[search->data.intData.rType];
				while(old)
				{
					if(strcmp(old->name,node->name) == 0)
					{
						if(old->value > search->data.intData.max)
							node->value = search->data.intData.max;
						else if(old->value < search->data.intData.min)
							node->value = search->data.intData.min;
						else
							node->value = old->value;
						break;
					}
					old = old->next;
				}

				node->next = newIntList[search->data.intData.rType];
				newIntList[search->data.intData.rType] = node;			
			}
			else if(search->dataType == MDT_COLOR)
			{
				MatColor * node = new MatColor();
				node->reg = search->data.colorData.rValue;
				node->red = search->data.colorData.defRed;
				node->green = search->data.colorData.defGreen;
				node->blue = search->data.colorData.defBlue;
				strcpy(node->name,search->name);

				//look for old values
				MatColor * old = colorList[search->data.colorData.rType];
				while(old)
				{
					if(strcmp(old->name,node->name) == 0)
					{
						node->red = old->red;
						node->green = old->green;
						node->blue = old->blue;
						break;
					}
					old = old->next;
				}

				node->next = newColorList[search->data.colorData.rType];
				newColorList[search->data.colorData.rType] = node;			
			}
			else if(search->dataType == MDT_TEXTURE)
			{
				MatTexture * node = new MatTexture();
				node->channel = search->data.textureData.channel;
				strcpy(node->name,search->name);
				node->filename[0] = 0;

				//look for old values
				MatTexture * old = textureList;
				while(old)
				{
					if(strcmp(old->name,node->name) == 0)
					{
						strcpy(node->filename,old->filename);
						break;
					}
					old = old->next;
				}

				node->next = newTextureList;
				newTextureList = node;			
			}
			else if(search->dataType == MDT_STATE)
			{
				MatState * node = new MatState();
				node->state = search->data.stateData.state;
				strcpy(node->name,search->name);
				node->value = search->data.stateData.defValue;

				//look for old values
				MatState * old = stateList;
				while(old)
				{
					if(strcmp(old->name,node->name) == 0)
					{
						node->value = old->value;
						break;
					}
					old = old->next;
				}

				node->next = newStateList;
				newStateList = node;			
			}
			else if(search->dataType == MDT_ANIMUV)
			{
				if(newAnimUV)
				{
					delete newAnimUV;
					newAnimUV = NULL;
				}
				newAnimUV = new MatAnimUV();
				newAnimUV->reg = search->data.animUVData.rValue;
				newAnimUV->rows = 1;
				newAnimUV->columns = 1;
				newAnimUV->frameRate = 10.0f;
				strcpy(newAnimUV->name,search->name);

				//look for old values
				if(animUV)
				{
					if(strcmp(animUV->name,newAnimUV->name) == 0)
					{
						newAnimUV->rows = animUV->rows;
						newAnimUV->columns = animUV->columns;
						newAnimUV->frameRate = animUV->frameRate;
					}
				}
			}

			search = search->next;
		}
	}

	//free old lists
	for(U32 i = 0; i < 3; ++i)
	{
		while(floatList[i])
		{
			MatFloat * tmp = floatList[i];
			floatList[i] = floatList[i]->next;
			delete tmp;
		}
		while(intList[i])
		{
			MatInt * tmp = intList[i];
			intList[i] = intList[i]->next;
			delete tmp;
		}
		while(colorList[i])
		{
			MatColor * tmp = colorList[i];
			colorList[i] = colorList[i]->next;
			delete tmp;
		}
	}
	while(textureList)
	{
		MatTexture * tmp = textureList;
		textureList = textureList->next;
		delete tmp;
	}
	while(stateList)
	{
		MatState * tmp = stateList;
		stateList = stateList->next;
		delete tmp;
	}
	if(animUV)
	{
		delete animUV;
		animUV = NULL;
	}

	//replace old lists
	floatList[0] = newFloatList[0];
	floatList[1] = newFloatList[1];
	floatList[2] = newFloatList[2];
	intList[0] = newIntList[0];
	intList[1] = newIntList[1];
	intList[2] = newIntList[2];
	colorList[0] = newColorList[0];
	colorList[1] = newColorList[1];
	colorList[2] = newColorList[2];
	textureList = newTextureList;
	stateList = newStateList;
	animUV = newAnimUV;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//

