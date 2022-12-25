#include <stdlib.h>
#include <string.h>
#include "names.h"
#include "mtl_txt.h"

#ifndef SGI
#pragma warning( error : 4701 ) // variable may be used without having been initialized
#pragma warning( error : 4700 )
#pragma warning( disable : 4514 ) // unreferenced inline function has been removed
#endif


/*void AddMtl(file_node *previous, mtl_lib *ml, int index)
{
  file_node *current;
  file_node *tmp;

  previous->sibling = current = CreateNode((ml->cq2Materials[index])->name, D);

  current->child = tmp = CreateNode(Material_identifier,F);
  tmp->data_size=sizeof((ml->materials[index])->identifier);
  tmp->data=(unsigned char*)&((ml->materials[index])->identifier);

  
  tmp->sibling = CreateMtlProperty(&(ml->materials[index]->diffuse));
  tmp = tmp->sibling;

  //if(ml->materials[index]->ambient.value[0] > 0.0f ||
	 //ml->materials[index]->ambient.value[1] > 0.0f ||
	 //ml->materials[index]->ambient.value[2] > 0.0f ||
	 //ml->materials[index]->ambient.texture_name)
  {
	tmp->sibling = CreateMtlProperty(&(ml->materials[index]->ambient));
	tmp = tmp->sibling;
  }

  if(ml->materials[index]->emission.value[0] > 0.0f ||
	 ml->materials[index]->emission.value[1] > 0.0f ||
	 ml->materials[index]->emission.value[2] > 0.0f ||
	 ml->materials[index]->emission.texture_name)
  {
	tmp->sibling = CreateMtlProperty(&(ml->materials[index]->emission));
	tmp = tmp->sibling;
  }

  if(ml->materials[index]->specular.value[0] > 0.0f ||
	 ml->materials[index]->specular.value[1] > 0.0f ||
	 ml->materials[index]->specular.value[2] > 0.0f ||
	 ml->materials[index]->specular.texture_name)
  {
	tmp->sibling = CreateMtlProperty(&(ml->materials[index]->specular));
	tmp = tmp->sibling;
  }

#ifdef SGI
  if(ml->materials[index]->shininess.value[0] > 0.0f ||
	 ml->materials[index]->shininess.texture_name)
#else // MAX
  if(ml->materials[index]->shininess.value[1] > 0.0f || // strenght/height
	 ml->materials[index]->shininess.texture_name)
#endif
  {
	  tmp->sibling = CreateMtlProperty(&(ml->materials[index]->shininess));
	  tmp = tmp->sibling;
  }

  if(ml->materials[index]->transparency.value[0] < 1.0f ||
	 ml->materials[index]->transparency.texture_name )
  {
	tmp->sibling = CreateMtlProperty(&(ml->materials[index]->transparency));
	tmp = tmp->sibling;
  }

  // value[0] always 1 in MAX exporter since it's not specified explicitly
  if(ml->materials[index]->bump.value[0] != 0.0f ||
	 ml->materials[index]->bump.texture_name)
  {
	tmp->sibling = CreateMtlProperty(&(ml->materials[index]->bump));
	tmp = tmp->sibling;
  }
  if((index+1) < ml->count)
  {
    AddMtl(current, ml, index+1);
  }
}*/


void AddMtlCQ2(file_node *previous, mtl_lib *ml, int index)
{
  file_node *current;
  file_node *tmp;

  previous->sibling = current = CreateNode((ml->cq2Materials[index])->name, D);
  current->child = tmp = CreateNode(Material_identifier,F);
  tmp->data_size=sizeof((ml->cq2Materials[index])->identifier);
  tmp->data=(unsigned char*)&((ml->cq2Materials[index])->identifier);
  
  if((index+1) < ml->cq2Count)
  {
    AddMtlCQ2(current, ml, index+1);
  }
}

file_node* CreateMtlLib(mtl_lib *ml)
{
  file_node *node=0;
  file_node *tmp = 0;
  if (ml->cq2Count > 0)
  {
	tmp = CreateNode("CQ2_Material_Library",D);
	if (node) node->sibling = tmp;
	else node = tmp;
	
	tmp->child=CreateNode(Material_count,F);
	tmp=tmp->child;
	tmp->data_size=sizeof(ml->cq2Count);
	tmp->data=(unsigned char*)&(ml->cq2Count);
	AddMtlCQ2(tmp, ml, 0);
  }

  if(verbose_level>=3)
  {
    printf("done w/ CreateMtlLib\n");
  }

  return node;
}



void InitMtlLib(mtl_lib *ml)
{
  ml->cq2Count=0;
  ml->cq2Materials=NULL;
}

void InitMaterialCQ2(cq2Mtl *m)
{
  m->name = NULL;
//  m->type[0] = 0;
  m->identifier = -1;
  m->api_id = -1;
}



bool SameMtl(const cq2Mtl * const m1, const cq2Mtl * const m2)
{
	// toDo: implement a better comparison later
	return (m1 == m2);
}

InsertCq2Material(mtl_lib * ml, cq2Mtl * m)
{
assert(ml);
 
  if(m==NULL) return -1;
  assert(m->identifier == -1);
  
  /* new material */
  m->identifier = ml->cq2Count;
  ml->cq2Count++;
  ml->cq2Materials=(cq2Mtl**)Realloc(ml->cq2Materials, ml->cq2Count*sizeof(cq2Mtl*));
  ml->cq2Materials[ml->cq2Count-1]=m;
  if(verbose_level>=2)
  {
    printf("inserted CQ2 mtl_id=%d name=%s\n", ml->cq2Count-1, m->name);
  }

  m=NULL;
  return(ml->cq2Count-1);
}


int GetCQ2MtlID(const mtl_lib *ml, const char *name)
{
  if(name)
  {
	  for(int i = 0; i < ml->cq2Count; i++)
	  {
		if(!strcmp_icase(ml->cq2Materials[i]->name, name))
		{
			if(strcmp(ml->cq2Materials[i]->name, name))
			{
				Winprint("Error: materials %s and %s differ only in case, which is illegal!\n",
					ml->cq2Materials[i]->name, name);
			}
			return i;
		}
	  }
  }

  return -1;
}

int GetCQ2MtlID(const mtl_lib *ml, const int api_id)
{
  for(int i = 0; i < ml->cq2Count; i++)
  {
	if(ml->cq2Materials[i]->api_id == api_id)
	{
		return i;
	}
  }
  return -1;
}


