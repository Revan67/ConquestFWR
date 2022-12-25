#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "sgi_utf.h"
#include "3db.h"
#include "cmp.h"
#include "faceprop.h"

enum {NOTHING=0, FLAT, SMOOTH, SINGLE_S, DOUBLE_S};

class garbage{
public:
  int count;
  void **mem_list;
  void (**fc_list)(void*);  // array of function pointers

  garbage()
  {
    count=0;
    mem_list=NULL;
    fc_list=NULL;
  }

  ~garbage()
  {
    Empty();
  }

  void Empty()
  {
    for(int i=0; i<count; i++){
      fc_list[i](mem_list[i]);
      Free(mem_list[i]);
    }
    Free(mem_list);
    Free(fc_list);
    count=0;
  }

  void Add(void *pt, void (*func)(void* pt) );
};

class mods{
public:
  float ambient[3];
  float diffuse[3];
  float specular[3];
  float emission[3];
  float shininess;
  float transparency;
  int shading;
  int sided;
  float mass;

  mods()
  {
    ambient[0]=ambient[1]=ambient[2]=1.0;
    diffuse[0]=diffuse[1]=diffuse[2]=1.0;
    specular[0]=specular[1]=specular[2]=0.0;
    emission[0]=emission[1]=emission[2]=0.0;
    shininess=0.0;
    transparency=1.0; // 1=opaque
    shading=NOTHING;
    sided=NOTHING;
    mass=-1;
  }
};

garbage thrash;
mods changes;

int verbose_level=1;
int flag_565=0;
int mip_flag=1;
int txt_flag=1;
int scale_flag=0;
float scale_factor=1.0;

int a_flag=0, d_flag=0, s_flag=0, e_flag=0, sh_flag=0, t_flag=0;
int mtl_default_flag=0; 
int mtl_list_flag=0;
int info_flag=0;
int mass_flag=0;
int changed_flag=0;

void Usage(char* argv0);
void CmdOptions(int argc, char *argv[], mods *changes);
void FixMtl(mtl_lib *ml, mods changes);
file_node* ProcessMtl(file_node* node);
void ProcessTree(file_node *node, file_node *parent, char *name, int depth,
                 int verbose_level, file_node* (*func)(file_node* node) );
file_node* ProcessFaces(file_node* node);
void FixFaces(faces *f, mods changes);
file_node* ProcessRigidBody(file_node* node);
file_node* PrintLodObj(file_node* node);

int main(int argc, char *argv[])
{
  file_node *root=NULL;

  if(argc<2)
    Usage(argv[0]); 

  CmdOptions(argc, argv, &changes);

  root=ReadUTF(argv[1]);
  if(root==NULL){
    fprintf(stderr,"Error: file %s not found.\n",argv[1]);
    exit(1);
  }

  if(mtl_list_flag){
	ProcessTree(root->child, root, Material_library, 0, mtl_list_flag, ProcessMtl);
  }

  ProcessTree(root->child, root, Faces, 0, 0, ProcessFaces);

  if(mass_flag || info_flag){
    ProcessTree(root->child->sibling, root->child, Rigid_body, 0, 0, ProcessRigidBody);
  }

  if(info_flag){
    // faces, verts, mtls, txt's, rigidb, hp's, levels
    // conn's, anims, bones 
    ProcessTree(root->child, root, openFLAME_3D_N_mesh, 0, 1, PrintLodObj);
  }

  if(changed_flag){
    WriteUTF(root, argv[1]);
  }

  FreeTreeData(root);
  thrash.Empty();

  return 0;
}

void Usage(char *argv0)
{
  fprintf(stderr,"Usage: %s <file.3db>\n", argv0);
  fprintf(stderr,"\t[-mlist]\n");
  fprintf(stderr,"\t\tList material values.\n");
  fprintf(stderr,"\t[-mdefault]\n");
  fprintf(stderr,"\t\tSet materials to default values.\n");
  fprintf(stderr,"\t[-ambient|diffuse|specular|emission r# g# b# ]\n");
  fprintf(stderr,"\t[-shininess|transparency #]\n");
  fprintf(stderr,"\t\tSet materials to command line values.\n");
  fprintf(stderr,"\t[-smooth|flat]\n");
  fprintf(stderr,"\t\tSet smooth or flat shaded faces.\n");
  fprintf(stderr,"\t[-single|double]\n");
  fprintf(stderr,"\t\tSet single or double sided faces.\n");
  fprintf(stderr,"\t[-mass #]\n");
  fprintf(stderr,"\t\tSet object's mass.\n");
  //fprintf(stderr,"\t[-scale #] (.3db only)\n");
  //fprintf(stderr,"\t\tRescale object.\n");
  fprintf(stderr,"\t[-info]\n");
  fprintf(stderr,"\t\tPrint object info.\n");

  exit(1);  
}

void CmdOptions(int argc, char *argv[], mods *changes)
{
  int i;

  for(i=2;i<argc;i++){
    if(!strcmp(argv[i],"-ambient")){
      a_flag=1;
      changed_flag=1;
      changes->ambient[0]=(float)atof(argv[i+1]);
      changes->ambient[1]=(float)atof(argv[i+2]);
      changes->ambient[2]=(float)atof(argv[i+3]);
    } else
    if(!strcmp(argv[i],"-diffuse")){
      d_flag=1;
      changed_flag=1;
      changes->diffuse[0]=(float)atof(argv[i+1]);
      changes->diffuse[1]=(float)atof(argv[i+2]);
      changes->diffuse[2]=(float)atof(argv[i+3]);
    } else
    if(!strcmp(argv[i],"-specular")){
      s_flag=1;
      changed_flag=1;
      changes->specular[0]=(float)atof(argv[i+1]);
      changes->specular[1]=(float)atof(argv[i+2]);
      changes->specular[2]=(float)atof(argv[i+3]);
    } else
    if(!strcmp(argv[i],"-emission")){
      e_flag=1;
      changed_flag=1;
      changes->emission[0]=(float)atof(argv[i+1]);
      changes->emission[1]=(float)atof(argv[i+2]);
      changes->emission[2]=(float)atof(argv[i+3]);
    } else
    if(!strcmp(argv[i],"-shininess")){
      changed_flag=1;
      sh_flag=1;
      changes->shininess=(float)atof(argv[i+1]);
    } else
    if(!strcmp(argv[i],"-transparency")){
      changed_flag=1;
      t_flag=1;
      changes->transparency=(float)atof(argv[i+1]);
    }else
    if(!strcmp(argv[i],"-mdefault")){
      changed_flag=1;
      mtl_default_flag=1;
    }else
    if(!strcmp(argv[i],"-flat")){
      changed_flag=1;
      changes->shading=FLAT; 
    }else
    if(!strcmp(argv[i],"-smooth")){
      changed_flag=1;
      changes->shading=SMOOTH; 
    }else
    if(!strcmp(argv[i],"-single")){
      changed_flag=1;
      changes->sided=SINGLE_S; 
    }else
    if(!strcmp(argv[i],"-double")){
      changed_flag=1;
      changes->sided=DOUBLE_S; 
    }else
    if(!strcmp(argv[i],"-mlist")){
      mtl_list_flag=1;
    }else
    if(!strcmp(argv[i],"-info")){
      info_flag=1;
    }else
    if(!strcmp(argv[i],"-mass")){
      changed_flag=1;
      mass_flag=1;
      changes->mass=(float)atof(argv[i+1]);
    } else
    if(!strcmp(argv[i],"-scale")){
      changed_flag=1;
      scale_flag=1;
      scale_factor=(float)atof(argv[i+1]);
    } else
    if(!strcmp(argv[i],"-v")){
      verbose_level=(int)atoi(argv[i+1]);
    }
  }
}
    
void FixMtl(mtl_lib *ml, mods changes)
{
  int i;

  for(i=0; i<ml->count; i++){
    if(mtl_list_flag){
      PrintMtl(ml->materials[i], i);
      printf("\n");
    }
    if(a_flag || mtl_default_flag){
      ml->materials[i]->ambient[0]=changes.ambient[0];
      ml->materials[i]->ambient[1]=changes.ambient[1];
      ml->materials[i]->ambient[2]=changes.ambient[2];
    }
    if(d_flag || mtl_default_flag){
      ml->materials[i]->diffuse[0]=changes.diffuse[0];
      ml->materials[i]->diffuse[1]=changes.diffuse[1];
      ml->materials[i]->diffuse[2]=changes.diffuse[2];
    }
    if(s_flag || mtl_default_flag){
      ml->materials[i]->specular[0]=changes.specular[0];
      ml->materials[i]->specular[1]=changes.specular[1];
      ml->materials[i]->specular[2]=changes.specular[2];
    }
    if(e_flag || mtl_default_flag){
      ml->materials[i]->emission[0]=changes.emission[0];
      ml->materials[i]->emission[1]=changes.emission[1];
      ml->materials[i]->emission[2]=changes.emission[2];
    }
    if(sh_flag || mtl_default_flag){
      ml->materials[i]->shininess=changes.shininess;
    }
    if(t_flag || mtl_default_flag){
      ml->materials[i]->transparency=changes.transparency;
    }
  }
}

void FixFaces(faces *f, mods changes)
{
  int i;

  for(i=0; i<f->count; i++){
    if(changes.shading==FLAT){
      f->property[i]=(f->property[i] | FLAT_SHADED);
      f->property[i]=(f->property[i] & (0xffffffff - SMOOTH_SHADED));

    }else
    if(changes.shading==SMOOTH){
      f->property[i]=(f->property[i] | SMOOTH_SHADED);
      f->property[i]=(f->property[i] & (0xffffffff - FLAT_SHADED));

    }

    if(changes.sided==SINGLE_S){ // TWO_SIDED
      f->property[i]=(f->property[i] & (0xffffffff - TWO_SIDED));
    }else
    if(changes.sided==DOUBLE_S){
      f->property[i]=(f->property[i] | TWO_SIDED);
    }
  }
}

file_node* PrintLodObj(file_node *node)
{
  lod_object *lod_obj;
  object *obj;
  int i;

  lod_obj=(lod_object*)Malloc(sizeof(lod_object));
  LoadLodObject(lod_obj, node);

  obj=&(lod_obj->obj_list[0]);

  printf("%d face(s)\n",obj->f.count);
  printf("%d physical vertice(s)\n",obj->v.object_count);
  printf("%d refferenced vertice(s)\n",obj->v.reference_count);
  printf("%d material(s)\n",obj->ml.count);
  printf("%d texture(s)\n",obj->tl.count);

  printf("%f radius\n",obj->extents.sphere.radius);
  printf("%f mass\n",obj->extents.mass.mass);
 
  if(lod_obj->count>1){
    printf("%d level(s) of detail.\n",lod_obj->count);
  }

  if(obj->hp_fix_count>0){
    printf("%d fixed hard point(s)\n",obj->hp_fix_count);
    for(i=0; i<obj->hp_fix_count; i++){
      printf("  %s\n",obj->hp_fix_list[i].name);
    }
  } 

  if(obj->hp_pris_count>0){
    printf("%d prismatic hard point(s)\n",obj->hp_pris_count);
    for(i=0; i<obj->hp_pris_count; i++){
      printf("  %s\n",obj->hp_pris_list[i].spot.name);
    }
  } 

  if(obj->hp_rev_count>0){
    printf("%d revolute hard point(s)\n",obj->hp_rev_count);
    for(i=0; i<obj->hp_rev_count; i++){
      printf("  %s\n",obj->hp_rev_list[i].spot.name);
    }
  } 

  Free(lod_obj);
  return node;
}

file_node* ProcessFaces(file_node* node)
{
  faces *f;

  f=(faces*)Malloc(sizeof(faces));
  thrash.Add((void*)f, FreeFaces);

  LoadFaces(f, node);
  FreeTreeData(node);

  FixFaces(f, changes);

  return CreateFaces(f);
  // f gets freed later as part of garbage collection
}

file_node* ProcessMtl(file_node* node)
{
  mtl_lib *ml;

  ml=(mtl_lib*)Malloc(sizeof(mtl_lib));
  thrash.Add((void*)ml, FreeMtlLib);

  LoadMaterials(ml, node);
  FreeTreeData(node);

  FixMtl(ml, changes);

  return CreateMtlLib(ml);
  // ml gets freed later as part of garbage collection
}

file_node* ProcessRigidBody(file_node* node)
{
  Extents *extents;

  extents=(Extents*)Malloc(sizeof(Extents));
  thrash.Add((void*)extents, free);

  LoadRigidBody(extents, node);
  FreeTreeData(node);

  extents->compute_mass(DENSITY, changes.mass);

  return CreateRigidBody(*extents);
  // extents gets freed later as part of garbage collection
}

void ProcessTree(file_node *node, file_node *parent, char *name, int depth,
                 int verbose_level, file_node* (*func)(file_node* node) )
{
  if(node==NULL) return;

  if(verbose_level >= 2){
    Indent(depth);
    printf("Procesing %s\n", node->name);
  }
  else{
    if((verbose_level >= 1) && 
       ( strstr(node->name, "3db") || strstr(node->name,"Level") )){
      Indent(depth);
      printf("%s\n", node->name);
    }
  }

  if(strstr(node->name, name)){

    file_node *sibling = node->sibling;
    node->sibling = NULL;

    if(parent->child == node){
	  node = func(node);
      parent->child = node;
      parent->child->sibling = sibling;
    }else   
    if(parent->sibling == node){
	  node = func(node);
      parent->sibling = node;
      parent->sibling->sibling = sibling;
    }
    else{ // sanity check
      fprintf(stderr,"Confused ProcessTree\n");
      exit(1);
    }

  }
  else{
    ProcessTree(node->child, node, name, depth+1, verbose_level, func);
  }

  ProcessTree(node->sibling, node, name, depth, verbose_level, func); 
}
  
void garbage::Add(void *pt, void (*func)(void* pt) )
{
  count++;
  mem_list=(void**)Realloc(thrash.mem_list, thrash.count*sizeof(void*));
  mem_list[thrash.count-1]=pt; 

  fc_list= (void (**)(void *)) Realloc(thrash.fc_list, 
                  thrash.count*sizeof(void*));
  fc_list[thrash.count-1]=func;
}


int Winprint(const char *format, ...)
{
  va_list args;
  char buffer[256];

  va_start(args, format);
  vsprintf(buffer, format, args);
  va_end(args);

  fprintf(stderr,"%s\n", buffer);
  fflush(stderr);

  return 0;
}