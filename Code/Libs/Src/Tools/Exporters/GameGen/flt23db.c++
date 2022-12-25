// check sources in first
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>

#include "mgapiall.h"
#include "mgapiutil4.h"

#include "sgi_utf.h"
#include "3db.h"
#include "persistcompound.h"
#include "cmp.h"
#include "rgbutils.h"

int IsPlaceHolder(mgrec *node);
// void AdjustAxis(DofData *dof_data, PersistVector *axis);
void GetRotMatrix(DofData *dof_data, Matrix *m);
void DefineHardPoint( object *obj, mgrec *dof_node);
void RevAnimation(CompoundObject *c_obj, Fix *fixed, DofData *dof_data, 
                  Frame *frame_list, int n_frames, mgrec *dof_node);
void PrisAnimation(CompoundObject *c_obj, Fix *fixed, DofData *dof_data, 
                   Frame *frame_list, int n_frames, mgrec *dof_node);
int ReadFrames(mgrec *node, Frame *frame_list, int count);
Matrix PersistMatrix_to_Matrix(PersistMatrix pm);
void findUsedTextures(mgrec *node, int *used_txt, int *txt_used_count,
                     int first_call_flag);
void findEmissiveTextures(mgrec *db, int *used_txt, int *txt_used_count);
void findMtlTxtCombinations(mgrec *node, int **mtl_txt_matrix, 
     int *mtl_used, int *combo_count, int *plain_mtl_count, mgrec *db,
     int first_call_flag);
void defineTextures(txt_lib *tl, mgrec *db, int *txt_used, int txt_count);
txt* GetTxt(mgrec *db, int txtNum, char *txtName);
mtl* GetMtl(anim_txt_lib *atl, int mg_mtl_id, int txt_id, mgrec *db);
void defineMaterials(mtl_lib *ml, anim_txt_lib *atl, mgrec *db,
                     int **mtl_txt_matrix, int *mtl_used, int mtl_count, 
                     int txt_count);
void defineGeometry(object *obj, mgrec *node, mgrec *db, 
                    int first_call_flag);
void definePolygon( object *obj, mgrec *node, mgrec *db);
void Usage(char *argv0);
void CmdOptions(int argc, char *argv[], char cmp_file_name[]);
void ReadHierarchy(CompoundObject *c_obj, mgrec *db, mgrec *node,
                   int indent);
void LoadGGObject(object *obj, mgrec *db, mgrec *node,
                  txt_lib *tl, anim_txt_lib *atl);
mgrec* GetParentGroup(mgrec *node);
int HasGeometry(mgrec *node, int first_call_flag);
int LodCount(mgrec *node);
void GetLodNodes(mgrec ***node_list, mgrec *node);
void PrintNodeName(mgrec *node);
void PrintNodeNameE(mgrec *node);
void MakeDofConnection(CompoundObject *c_obj, mgrec *node);
int ReadFixed(mgrec *xrec, Fix *fixed);
void GetFixed(mgrec *node, Fix *fixed);
void Export3DB(mgrec *db, mgrec *node, CompoundObject *c_obj);
void ExportLod3DB(mgrec *db, mgrec *node, CompoundObject *c_obj);
void MakeAnimConnection(CompoundObject *c_obj, mgrec *node);
int FrameNumber(char *name);
int ReadTransform(mgrec *xrec, Frame *frame);
void GetDofData(mgrec *dof_node, DofData *data);
PersistMatrix MMul(PersistMatrix pm, mgMatrix mgm);

bool tmp_bone_hack = false; // should go away

float face_normal_tolerance = FACE_NORMAL_TOLERANCE;
float vertex_normal_tolerance = VERTEX_NORMAL_TOLERANCE;

bool exporting_deformable = false;
bool remove_constant_channels = false;

float bytes_copied = 0;
float bytes_reallocked = 0;

float lod_mtl_weight = 200.0f;
float lod_uv_weight = 0.80;

int ik_extents = 0;
int export_vertex_colors = 0;
int convex_hull_flag = 1;
int old_style = 0;
int split_flag=0;
int flip_texture_flag=0;
int flip_normals_flag=0;
//int combine_flag=0;
int triangle_flag=0;
int verbose_level=1;
int mip_flag=1;
int dither_flag=0;
int emip_flag=1;
//int flag_565=0;
int txt_depth = PAL8;
int mdefault_flag=0;
//int scale_flag=0;
float scale_factor=1.0f;
float fps=10.0;
float density=DENSITY;
int no_physics = 0;

float lod_percent = 100.0f;    
float lod_closest = 100.0f;
float lod_furthest = 1000.0f;

char *root_name=NULL;

int main(int argc, char *argv[])
{
  mgrec *db;
  char *siteid, *message;
  CompoundObject c_obj;

  int *tst = new int[3];

  delete [] tst;
  delete [] tst;

  if(argc<2)
    Usage(argv[0]);

  InitCompoundObject(&c_obj);

  CmdOptions(argc, argv, c_obj.file_name);
  mgInit(&argc, argv);

  if(!(db=mgOpenDb(argv[1])))
  {
    mgGetError(&siteid, &message);
    fprintf(stderr,"\nError from %s: %s\n",siteid, message);
    exit(1);
  }

  ReadHierarchy(&c_obj, db, db, 0 /*indent*/);
  CombineChannels(&c_obj, 0);
  SynchronizeScriptTiming(&c_obj);
  ExtractKeyFrames(&c_obj);

  if(scale_factor != 1.0f){
    ScaleCompoundObject(&c_obj, scale_factor);
  }

/*
  if(c_obj.part_count > 1 || c_obj.n_channel_count > 0)
  {
    char *ext_name = strdup(c_obj.file_name);
    strtok(ext_name, ".");
    ComputeCompoundExtents(&c_obj, density, -1, ext_name);
    Free(ext_name);
  }
*/

  WriteCmpd(&c_obj, split_flag, TXT_ON);
  FreeCmpObject(&c_obj);

  mgCloseDb(db);

  mgExit();
  if(verbose_level>=1){
    printf("done.\n");
  }
  return 0;
}

/* traverse database with no instances, external references, or subfaces */
void defineGeometry(object *obj, mgrec *node, mgrec *db, 
                    int first_call_flag)
{ 
  int node_type;
  char *name;

  if(!node) return;

  node_type=mgGetCode(node);
  name=mgGetName(node);

  if(verbose_level>=3){
    if((node_type==fltGroup) || (node_type==fltLod)){
      printf("defineGeometry for %s %d\n",name, node_type);
    }
  }

  // sanity check
  if(first_call_flag && 
     (node_type!=fltGroup) && (node_type!=fltLod)){
    fprintf(stderr,
      "Warning: should not call defineGeometry on this node.\n");
    PrintNodeName(node);
    return;
  }

  // add geometry
  if((node_type == fltPolygon) && (mgCountChild(node)>=3) 
  && (node_type != fltPolyFlagHidden)){
    definePolygon(obj, node, db);
  }

  // add hard point
  if((node_type==fltDof) && (strstr(name, "hp_"))){
    if(verbose_level>=2){
      printf("have hard point on %s\n",name);
    }
    DefineHardPoint(obj, node);
  }

  // dereference instance nodes
  // fix - skip only instance nodes whose parent is animation
  #if 0
  if(mgIsInstance(node)){
    defineGeometry(obj, mgGetReference(node), db, 0); 
  }  
  #endif

  // down/child
  if((node_type!=fltDof) || first_call_flag ){
    defineGeometry(obj, mgGetChild(node), db, 0); 
  }

  // right/sibling
  if(!first_call_flag ){
    defineGeometry(obj, mgGetNext(node), db, 0); 
  }

  if((verbose_level>=2) && first_call_flag){
    printf("done w/ defineGeometry\n");
  }
      
  mgFree(name);
}

void defineMaterials(mtl_lib *ml, anim_txt_lib *atl, mgrec *db,
                     int **mtl_txt_matrix, int *mtl_used, int mtl_count, 
                     int txt_count)
{
  int mg_mtl_id;
  int mg_txt_id;
  int mtl_id;
  mtl *m;

  mtl_id=0;

  for(mg_mtl_id=0;mg_mtl_id<mtl_count;mg_mtl_id++){
    if(mtl_used[mg_mtl_id]){
      m=GetMtl(atl, mg_mtl_id, -1, db);
      m->api_id = mg_mtl_id;
      InsertMaterial(ml, m);
      mtl_id++;
    }
  }

  for(mg_mtl_id=0;mg_mtl_id<mtl_count;mg_mtl_id++){
    for(mg_txt_id=0;mg_txt_id<txt_count;mg_txt_id++){
      if(mtl_txt_matrix[mg_mtl_id][mg_txt_id]){
        m=GetMtl(atl, mg_mtl_id, mg_txt_id, db);
        m->api_id = mg_mtl_id;
        InsertMaterial(ml, m);
        mtl_id++;
      }
    }
  }

  if(verbose_level>=2){
    printf("done w/ defineMaterials\n");
  }
}

mtl* GetMtl(anim_txt_lib *atl, int mg_mtl_id, int mg_txt_id, mgrec *db)
{
  mtl *m;
  char *mtlName;
  char *txtName;
  char *em_txtName;
  mgrec *mat;
  float red, green, blue;
  float shininess;
  float alpha;

        mtlName=mgNameOfMaterial(db,mg_mtl_id);
        StripPath(mtlName);
        if(mg_txt_id!=-1)
        {
          txtName=mgGetTextureName(db, mg_txt_id); 
          StripPath(txtName);
          StripExtension(txtName);

          em_txtName=(char*)Malloc((strlen(txtName)+4)*sizeof(char));
          sprintf(em_txtName, "em_%s", txtName);

          if( GetTxtID(atl->tl, em_txtName ) < 0 )
          {
            Free(em_txtName);
          }
        }
        else
        {
          txtName=NULL;
          em_txtName=NULL;
        }

        m=(mtl*)Malloc(sizeof(mtl));
        InitMaterial(m, atl);

        // m->identifier=mg_mtl_id; // set at insertion time
        if(txtName)
        {
          m->name=(char*)Malloc((strlen(mtlName)+strlen(txtName)+1)
                                *sizeof(char));
        }
        else
        {
          m->name=(char*)Malloc((strlen(mtlName)+1)*sizeof(char));
        }

        strcpy(m->name,mtlName);
        if(txtName)
        {
          strcat(m->name,txtName);
          m->diffuse.texture_id=GetTxtID(atl->tl, txtName);
          m->diffuse.texture_name=(char*)Malloc((strlen(txtName)+1)*sizeof(char));
          strcpy(m->diffuse.texture_name, txtName);
        }  

        if(em_txtName)
        {
          m->emission.texture_id=GetTxtID(atl->tl, em_txtName);
          m->emission.texture_name=(char*)Malloc((strlen(em_txtName)+1)*sizeof(char));
          strcpy(m->emission.texture_name, em_txtName);
        }

        mat=mgGetMaterial(db, mg_mtl_id);

        if(mdefault_flag)
        {
          m->ambient.value[RED]=1.0f;
          m->ambient.value[GREEN]=1.0f;
          m->ambient.value[BLUE]=1.0f;

          m->diffuse.value[RED]=1.0f;
          m->diffuse.value[GREEN]=1.0f;
          m->diffuse.value[BLUE]=1.0f;

          m->specular.value[RED]=0.0f;
          m->specular.value[GREEN]=0.0f;
          m->specular.value[BLUE]=0.0f;

          m->emission.value[RED]=0.0f;
          m->emission.value[GREEN]=0.0f;
          m->emission.value[BLUE]=0.0f;

          m->shininess.value[0]=0.0f;
          m->transparency.value[0]=1.0f;
        }
        else{
          mgGetNormColor(mat, fltAmbient, &red, &green, &blue);
          m->ambient.value[RED]=(red);
          m->ambient.value[GREEN]=(green);
          m->ambient.value[BLUE]=(blue);

          mgGetNormColor(mat, fltDiffuse, &red, &green, &blue);
          m->diffuse.value[RED]=(red);
          m->diffuse.value[GREEN]=(green);
          m->diffuse.value[BLUE]=(blue);

          mgGetNormColor(mat, fltSpecular, &red, &green, &blue);
          m->specular.value[RED]=(red);
          m->specular.value[GREEN]=(green);
          m->specular.value[BLUE]=(blue);

          mgGetNormColor(mat, fltEmissive, &red, &green, &blue);
          m->emission.value[RED]=(red);
          m->emission.value[GREEN]=(green);
          m->emission.value[BLUE]=(blue);

          mgGetAttList(mat, fltShininess, &shininess, fltMatAlpha, &alpha,
                       mgNULL);
          shininess/=128.0;
          m->shininess.value[0]=(shininess);
          m->transparency.value[0]=(alpha);
        }

        mgFree(mtlName);
        mgFree(txtName);
        Free(em_txtName);

        return m;
}

void definePolygon( object *obj, mgrec *node, mgrec *db )
{
/*
record fltVertex { 
  mgbool (4) 		fltVHard 	TRUE if vertex to get face's
  normal mgbool (4) 	fltVLeaveNorm 	TRUE to leave normal alone when 
                                        shading
  mgbool (4) 		fltVHard2 	TRUE if 2nd vertex of hardedge 
  unsigned int (4) 	fltVColor 	vertex color, if any, without 
                                        intensity component (0- 1023)
  float (4) 		fltVIntensity 	intensity of the vertex color, if any
  record fltIcoord 	fltIcoord 	the coordinate x, y, z 
  record fltVector 	fltVNormal 	vertex normal,if any 
  float (4) 		fltVU 		Texture mapping coordinate (U)
  float (4) 		fltVV 		Texture mapping coordinate (V) 
}
*/

  unsigned char shading;
  mgrec *mgvertex;
  short tindex = -1, tmindex = -1;
  short mindex=-1;
  double x,y,z;
  float vu,vv;
  int j;
  char *txt_name;
  char *mtl_name;
  char *full_name;
  char *poly_name;
  char draw_type;

  poly p;

  if(!node){
    fprintf(stderr,"Warning: empty node\n");
    return;
  }
  if(!mgIsCode(node, fltPolygon)){
    fprintf(stderr,"Warning: NOT a polygon node\n");
    return;
  }
  if(mgIsPolyConcave(node)){
    fprintf(stderr,"Warning: polygon ");
    PrintNodeNameE(node);
    fprintf(stderr," is concave\n");
  }
  if(!mgIsCoplanar(node)){
    fprintf(stderr,"Warning: polygon ");
    PrintNodeNameE(node);
    fprintf(stderr," is NOT coplanar\n");
  }
 
  if(flip_normals_flag){ 
    mgReverse(node); // flips vertex order AND normal
  }

  poly_name=mgGetName(node);
  if(verbose_level>=4){
    printf("Poly name %s\n",poly_name);
  }
  mgGetAttList(node, fltPolyMaterial, &mindex, fltPolyTexture, &tindex, 
               fltPolyTexmap, &tmindex, mgNULL);

  if(mindex==-1){
    fprintf(stderr,"Warning: poly %s has no material.\n",poly_name);
    fprintf(stderr,"Trying to use Default.\n");
    mindex=mgIndexOfMaterial (db, "Default");
  }
  if(mindex==-1){
    fprintf(stderr,"Couldn't get Default.\n");
    exit(1);
  }

  mtl_name=mgNameOfMaterial(db, mindex);
  StripPath(mtl_name);
  if(tindex!=-1){
    txt_name=mgGetTextureName(db, tindex);
    StripPath(txt_name);
    StripExtension(txt_name);
    full_name=(char*)Malloc((strlen(mtl_name)+strlen(txt_name)+1)
                          *sizeof(char));
    strcpy(full_name,mtl_name);
    strcat(full_name,txt_name);
  }
  else{
    if(verbose_level>=2){
      printf("non textured poly %s\n",poly_name);
    }
    txt_name=NULL;
    full_name=(char*)Malloc((strlen(mtl_name)+1)*sizeof(char));
    strcpy(full_name,mtl_name);
  }

  p.material_id=GetMtlID(&(obj->ml), full_name);

  if(p.material_id < 0)
  {
    fprintf(stderr,"Impossible material mtl=%s id=%d poly=%s\n",
            full_name, p.material_id, poly_name);
    exit(1);
  }
  Free(full_name);
  if(txt_name) 
    mgFree(txt_name);
  mgFree(mtl_name);


  p.vertex_count = mgCountChild(node);

  /* for each vertex */
  for(j=0; j<p.vertex_count; j++)
  {
    mgvertex=mgGetChildNth(node, j+1);
    if(!mgIsCode(mgvertex, fltVertex))
    {
      fprintf(stderr,"Warning NOT a vertex\n");
    }

    mgGetIcoord(mgvertex, fltIcoord, &x, &y, &z);

    p.vertices[3*j] = x;
    p.vertices[3*j+1] = y;
    p.vertices[3*j+2] = z;

    if(tindex > -1) // vertex textured
    {
      mgGetAttList(mgvertex, fltVU, &vu, fltVV, &vv, mgNULL);
      p.uv[2*j] = vu;
      p.uv[2*j+1] = vv;
    }
    else // vertex not textured
    {
      p.uv[2*j  ] = 
      p.uv[2*j+1] = 0.0f;
    }
  }

  /* get property */
  mgGetAttList(node, fltGcLightMode, &shading, 
                     fltPolyDrawType, &draw_type, mgNULL);
  p.property = 0;
  switch(shading)
  {
  case 0: // none
    p.property+=FLAT_SHADED;
    break;
  case 1: // gouraud
    p.property+=SMOOTH_SHADED;
    break;
  case 2: // dynamic
    p.property+=SMOOTH_SHADED;
    break;
  case 3: // dynamic gouraud
    p.property+=SMOOTH_SHADED;
    break;
  default:
    fprintf(stderr,"Warning: bad lighting\n");
    p.property+=SMOOTH_SHADED;
  }

  if(draw_type==1){
    p.property+=TWO_SIDED;
  }

  InsertPolyTriangles(obj, &p);
  //InsertPoly(obj, &v, material_id, property);

  mgFree(poly_name);
}

void defineTextures(txt_lib *tl, mgrec *db, int *txt_used, int txt_count)
{
  txt *t=NULL;
  char *txtName;

  // note textures might not start w/ 0! this should be fixed later
  for(int i=0; i<txt_count; i++) // note textures might not start w/ 0!
  {
      txtName=mgGetTextureName(db, i); 
      StripPath(txtName);
      StripExtension(txtName);
    if(txt_used[i])
    {

      if(mip_flag && !strncmp(txtName, "em_", 3))
      {
        Swap32(&emip_flag, &mip_flag);
      }

      t=GetTxt(db, i, txtName); 
      InsertTexture(tl, t);

      if(emip_flag && !strncmp(txtName, "em_", 3))
      {
        Swap32(&emip_flag, &mip_flag);
      }

    }
    mgFree(txtName);
  }

  if(verbose_level>=2){
    printf("done w/ defineTextures\n");
  }
}

txt* GetTxt(mgrec *db, int txtNum, char *txtName)
{
  txt *t=NULL;
  mgrec *txtrec;
  int width;
  int height;
  int image_type;
  unsigned char *image;
  unsigned char *alpha;
  int x,y;

    t=(txt*)Malloc(sizeof(txt));
    InitTexture(t);
    // t->identifier=txtNum; 
    if(verbose_level>=2){
      printf("texture%d %s\n",txtNum, txtName);
    }
    t->name=(char*)Malloc((strlen(txtName)+1)*sizeof(char));
    strcpy(t->name, txtName);
    t->mip_count=1;
    
    t->mip_map=(mip**)Malloc(sizeof(mip*));
    t->mip_map[0]=(mip*)Malloc(sizeof(mip));
    InitMip(t->mip_map[0]);
    t->mip_map[0]->level=(char*)Malloc((strlen(MIP_level_0)+1)*sizeof(char));
    strcpy(t->mip_map[0]->level, MIP_level_0);

    txtrec = mgGetTextureAttributes(db, txtNum);
    mgGetAttList(txtrec, fltImgWidth, &width, fltImgHeight, &height,
                 fltImgType, &image_type, mgNULL);

    image=mgGetTextureTexels(db, txtNum);
    if(flip_texture_flag){
      mgFlipImage(image, width, height, image_type); // top to bottom
    }

    t->mip_map[0]->x_size=width;
    t->mip_map[0]->y_size=height;
    t->mip_map[0]->color_count=N_PALETTE_COLORS; // 255
    t->mip_map[0]->depth = (color_depth)txt_depth;

    switch(image_type)
    {
    case 4: // SGI RRR..GGG..BBB..
    {
      VRGB *rgb; // Source data image
      rgb=(VRGB*)Malloc((4*width*height*sizeof(VRGB))/3); // 1/3 for mip data
      for(y=0; y<height; y++){
        for(x=0; x<width; x++){
          rgb[y*width+x].r=image[(y*width+x)];
          rgb[y*width+x].g=image[height*width+(y*width+x)];
          rgb[y*width+x].b=image[2*height*width+(y*width+x)];
        }
      }
      LoadTxtRGB(rgb, t);
      Free(rgb);

      return t;
    }
    case 5: // RGBA (actually ABGR - API bug)
    {
      VRGB *rgb; // Source data image
      rgb=(VRGB*)Malloc((4*width*height*sizeof(VRGB))/3); // 1/3 for mip data
      alpha=(unsigned char*)Malloc((4*width*height*sizeof(unsigned char))/3);
      for(y=0; y<height; y++){
        for(x=0; x<width; x++){
          rgb[y*width+x].r=image[4*(y*width+x)+3];
          rgb[y*width+x].g=image[4*(y*width+x)+2];
          rgb[y*width+x].b=image[4*(y*width+x)+1];
          alpha[y*width+x]=image[4*(y*width+x)]; 
        }
      }
      LoadTxtRGB(rgb, t);
      Free(rgb);

      LoadTxtAlpha(alpha, t);
      Free(alpha);

      return t;
    }
    default:
      fprintf(stderr,"Warning: unsupported texture type %d.\n",image_type);
      return NULL;
    }
}

void findMtlTxtCombinations(mgrec *node, int **mtl_txt_matrix, 
     int *mtl_used, int *combo_count, int *plain_mtl_count, mgrec *db,
     int first_call_flag)
{
  short material_id;
  short texture_id;
  int node_type;

  if(!node) return;
  node_type=mgGetCode(node);
  // sanity check
  if(first_call_flag &&
     (node_type!=fltGroup) && (node_type!=fltLod)){
    fprintf(stderr,
    "Warning: should not call findMtlTxtCombinations on this node.\n");
    PrintNodeName(node);
    return;
  }

  if(mgIsCode(node, fltPolygon) && (mgCountChild(node)>=3) 
  && !mgIsCode(node,fltPolyFlagHidden)){
    mgGetAttList(node, fltPolyMaterial, &material_id, 
                       fltPolyTexture, &texture_id, mgNULL);
    if(material_id==-1){
      fprintf(stderr,"Warning: poly ");
      PrintNodeName(node);
      fprintf(stderr,"has no material.\n");
      fprintf(stderr,"Trying to use Default.\n");
      material_id=mgIndexOfMaterial (db, "Default");
    }
    if(material_id==-1){
      fprintf(stderr,"Couldn't get Default.\n");
      exit(1);
    }

    if((material_id!=-1) && (texture_id!=-1)){
      if(!mtl_txt_matrix[material_id][texture_id]){
        (*combo_count)++;
        mtl_txt_matrix[material_id][texture_id]=1;
        if(verbose_level>=2){
          char *mtlName, *txtName;
          mtlName=mgNameOfMaterial(db, material_id);
          StripPath(mtlName);
          txtName=mgGetTextureName(db, texture_id); 
          StripPath(txtName);
          StripExtension(txtName);
          printf("material combo %d mtl%d=%s txt%d=%s\n",*combo_count, 
                  material_id, mtlName, texture_id, txtName);
          mgFree(mtlName);
          mgFree(txtName);
        }
      }
    }
    else{
      if(material_id!=-1){
        if(!mtl_used[material_id]){
          (*plain_mtl_count)++;
          mtl_used[material_id]=1; 
          if(verbose_level>=2){
            char *mtlName;
            mtlName=mgNameOfMaterial(db, material_id);
            StripPath(mtlName);
            printf("Material %d %s w/o a texture\n",material_id, mtlName);
            mgFree(mtlName);
          }
        }
      }
    }
  }

  // dereference instance nodes
  if(mgIsInstance(node)){
    findMtlTxtCombinations(mgGetReference(node), mtl_txt_matrix, mtl_used,
                           combo_count, plain_mtl_count, db, 0);
  }  

  // down/child
  if((node_type!=fltDof) || first_call_flag ){
    findMtlTxtCombinations(mgGetChild(node), mtl_txt_matrix, mtl_used,
                           combo_count, plain_mtl_count, db, 0);
  }

  // right/sibling
  if(!first_call_flag ){
    findMtlTxtCombinations(mgGetNext(node), mtl_txt_matrix, mtl_used,
                           combo_count, plain_mtl_count, db, 0);
  }
}
   
void findEmissiveTextures(mgrec *db, int *used_txt, int *txt_used_count)
{
  const int total_count = mgGetTextureCount(db);

  for(int i=0; i < total_count; i++)
  {
    if(used_txt[i] == 1)
    {
      char *txt_name = mgGetTextureName(db, i);
      if(txt_name)
      {
        StripPath(txt_name);
        StripExtension(txt_name);
        char target_name[256];
        sprintf(target_name, "em_%s", txt_name);
  
        for(int j=0; j < total_count; j++)
        {
          char *tmp_name = mgGetTextureName(db, j);
          if(tmp_name)
          {
            StripPath(tmp_name);
            StripExtension(tmp_name);
         
            if(used_txt[j] == 0 && !strcmp(tmp_name, target_name) )
            {
              (*txt_used_count)++;
              used_txt[j] = 1;
              mgFree(tmp_name);
              break;
            }
          }
          mgFree(tmp_name);
        }
        mgFree(txt_name);
      }
    }
  }

}

void findUsedTextures(mgrec *node, int *used_txt, int *txt_used_count,
                      int first_call_flag)
{
  short texture_id;
  int node_type;

  if(!node) return;
  node_type=mgGetCode(node);

  // sanity check
  if(first_call_flag && (node_type!=fltGroup) && (node_type!=fltLod))
  {
    fprintf(stderr,
      "Warning: should not call findUsedTextures on this node.\n");
    PrintNodeName(node);
    return;
  }

  if(mgIsCode(node, fltPolygon) && (mgCountChild(node)>=3) &&
     !mgIsCode(node,fltPolyFlagHidden))
  {
    mgGetAttList(node, fltPolyTexture, &texture_id, mgNULL);
    if(!used_txt[texture_id])
    {
      (*txt_used_count)++;
      used_txt[texture_id]=1;
    }
  }

  // dereference instance nodes
  if(mgIsInstance(node))
  {
    findUsedTextures(mgGetReference(node), used_txt, txt_used_count, 0);
  }  

  // down/child
  if((node_type!=fltDof) || first_call_flag )
  {
    findUsedTextures(mgGetChild(node), used_txt, txt_used_count, 0);
  }

  // right/sibling
  if(!first_call_flag )
  {
    findUsedTextures(mgGetNext(node), used_txt, txt_used_count, 0);
  }
}

void Usage(char *argv0)
{
  fprintf(stderr, 
    "\nUsage: %s <wire_file> [-o <outfile>] [-ft] [-fn] [-nomip] [-noemip] [-fps #] "
    "[-density #] [-lod <%% to remain> <closest> <furthest>]\n", argv0);

  fprintf(stderr,"\t-ft :\n");
  fprintf(stderr,"\t\tflip texture\n");

  fprintf(stderr,"\t-fn :\n");
  fprintf(stderr,"\t\tflip normals\n");

  fprintf(stderr,"\t-o outfile :\n");
  fprintf(stderr,"\t\tSave output file as \"outfile\" instead of using the\n");
  fprintf(stderr,"\t\tdefault extension substitution naming convention.\n");

  fprintf(stderr,"\t-nomip :\n");
  fprintf(stderr,"\t\tDon't generate any MIP levels.\n");

  fprintf(stderr,"\t-noemip :\n");
  fprintf(stderr,"\t\tDon't generate emissive MIP levels.\n");

  fprintf(stderr,"\t-565 :\n");
  fprintf(stderr,"\t\tExport textures as RGB 565.\n");

  fprintf(stderr,"\t-888 :\n");
  fprintf(stderr,"\t\tExport textures as RGB 888.\n");

  fprintf(stderr,"\t-mdefault :\n");
  fprintf(stderr,"\t\tUse default material properties.\n");

  fprintf(stderr,"\t-fps # :\n");
  fprintf(stderr,"\t\tOverride the default 10 fps for animations.\n");

  //fprintf(stderr,"\t-scale # :\n");
  //fprintf(stderr,"\t\tScale object by a factor of #.\n");

  fprintf(stderr,"\t-density # :\n");
  fprintf(stderr,"\t\tOverride the default density of .001\n");

  fprintf(stderr,"\t-split :\n");
  fprintf(stderr,"\t\tExport each part into a standalone file.\n");

  fprintf(stderr,"\t-triangles :\n");
  fprintf(stderr,"\t\tTriangulate geometry.\n");

  fprintf(stderr,"\t-lod <%% to remain> <closest> <furthest> :\n");
  fprintf(stderr,"\t\t Auto create LOD.\n");
  fprintf(stderr,"\t\t %% - percent of original faces for the lowest level\n");
  fprintf(stderr,"\t\t closest - switch in distance for the highest level\n");
  fprintf(stderr,"\t\t furthest - switch in distance for the lowest level\n");

  fprintf(stderr,"\t-uvw <#> :\n");
  fprintf(stderr,"\t\t Weight of UV for lod. ( 0.0 - 1.0; default = .8)\n");

  // fprintf(stderr,"\t-v # :\n");
  // fprintf(stderr,"\t\tSpecify verbose level (default=1).\n");

  exit(1);
}

void CmdOptions(int argc, char *argv[], char cmp_file_name[])
{
  int i;

  strcpy(cmp_file_name,argv[1]);
 
  for(i=2;i<argc;i++)
  {
    if(!strcmp(argv[i],"-ft")){
      flip_texture_flag=1;
    } else
    if(!strcmp(argv[i],"-fn")){
      flip_normals_flag=1;
    } else
    if(!strcmp(argv[i],"-o")){
      strcpy(cmp_file_name, argv[i+1]);
    }else
    if(!strcmp(argv[i],"-triangles")){
      triangle_flag=1;
    }else
    if(!strcmp(argv[i],"-v")){
      verbose_level=atoi(argv[i+1]);
    }else
    if(!strcmp(argv[i],"-nomip")){
      mip_flag=0;
    }else
    if(!strcmp(argv[i],"-noemip")){
      emip_flag=0;
    }else
    if(!strcmp(argv[i],"-565")){
      txt_depth=TRUE565;
    }else
    if(!strcmp(argv[i],"-888")){
      txt_depth=TRUE888;
    }else
    if(!strcmp(argv[i],"-dither")){
      dither_flag=1;
    }else
    if(!strcmp(argv[i],"-mdefault")){
      mdefault_flag=1;
    }else
    if(!strcmp(argv[i],"-split")){
      split_flag=1;
    }else
    if(!strcmp(argv[i],"-scale")){
      scale_factor=atof(argv[i+1]);
    }else
    if(!strcmp(argv[i],"-density")){
      density=atof(argv[i+1]);
    }else
    if(!strcmp(argv[i],"-fps")){
      fps=atof(argv[i+1]);
    }else
    if(!strcmp(argv[i],"-lod")){
      lod_percent = atof(argv[i+1]);
      lod_closest = atof(argv[i+2]);
      lod_furthest = atof(argv[i+3]);
    }else
    if(!strcmp(argv[i],"-uvw")){
      lod_uv_weight = atof(argv[i+1]);
      if(lod_uv_weight < 0.01f)
      {
         lod_uv_weight = 0.01f;
      }
      else
      if(lod_uv_weight > 0.99f)
      {
        lod_uv_weight = 0.99f;
      }
    }
  }

  strtok(cmp_file_name,".");
  if(split_flag){
    strcat(cmp_file_name, ".cmp");
  }
  else{
    strcat(cmp_file_name, ".cmp"); // .3db
  }
}

void ReadHierarchy(CompoundObject *c_obj, mgrec *db, mgrec *node, int indent)
{
  int node_type;
  static int root_flag=1;
  int lod_count;
  mgbool anim_flag1, anim_flag2;
  char *group_name;
  char *name;

  if(!node) 
    return;

  node_type=mgGetCode(node); // fltObject  fltGroup  fltDof fltHeader

  // display progress only
  if(verbose_level>=1){
    if((node_type==fltObject) || (node_type==fltGroup) ||
       (node_type==fltHeader) || (node_type==fltDof) || 
       (node_type==fltLod)){
      if((node_type!=fltGroup) || 
        ((node_type==fltGroup) && (HasGeometry(node, 1) || IsPlaceHolder(node)) )){
        Indent(indent);
        name=mgGetName(node);
        printf("%s %d\n",name, mgGetCode(node));
        mgFree(name);
      }else
      if(verbose_level>=2){
        Indent(indent);
        name=mgGetName(node);
        printf("%s %d (Ignored)\n",name, mgGetCode(node));
        mgFree(name);
      }
    }
  }

  if((node_type==fltGroup) && 
     (HasGeometry(node, 1) || IsPlaceHolder(node)) && 
     ((mgGetCode(mgGetParent(node))==fltDof) || 
      (mgGetCode(mgGetParent(node))==fltHeader))){

      group_name=mgGetName(node);
      if(root_flag){ // store actual name of "Root" group node
        root_name=(char*)Malloc((strlen(group_name)+1)*sizeof(char));
        strcpy(root_name, group_name);
        root_flag=0;
      }
      if(verbose_level>=2){
        printf("\nWorking on group %s\n",group_name);
      }

      lod_count=LodCount(node);
      if(lod_count){
        if(lod_percent < 100.0f)
        {
          Winprint("Warning: auto generating LOD for %s, EVEN THOUGH artist\n"
                   " generated lod is already present!\n", group_name);
        }
        ExportLod3DB(db, node, c_obj);
      }
      else{
        Export3DB(db, node, c_obj); // checks for geometry
      }

      // make connection
      if(GetParentGroup(node)!=mgNULL){
       // see if we have animation
       mgGetAttList(node, fltGrpFlagAnimation, &anim_flag1,
                          fltGrpFlagAnimationFB, &anim_flag2, mgNULL);
       if(anim_flag1 || anim_flag2){
         if(verbose_level>=2){
          printf("%s has anim\n",mgGetName(node));
         }
         MakeAnimConnection(c_obj, node);
       }
       else{
         MakeDofConnection(c_obj, node);
       }
      } 
 
      mgFree(group_name);
  }

  /* traverse down */
  if(mgGetChild(node)) {
    indent++;
    ReadHierarchy(c_obj, db, mgGetChild(node), indent);
    indent--;
  }

  /* traverse right */
  if(mgGetNext(node)) {
    ReadHierarchy(c_obj, db, mgGetNext(node), indent);
  }

  if((verbose_level>=2) && (indent==0)){
    printf("done w/ ReadHierarchy\n");
    printf("Total of %d parts\n",c_obj->part_count);
  }
}

void ExportLod3DB(mgrec *db, mgrec *node, CompoundObject *c_obj)
{
  int lod_count;
  mgrec **node_list;
  int i, j;
  double *in_list; 
  double *out_list;
  mgrec *tmp_mgrec;
  double tmp_double;
  lod_object *lod_obj;
  char *name;

  // insert name into hierarchy 
  name=mgGetName(node);
  InsertCompoundName(c_obj, name, name, root_name, ".3db", 1, NULL, true); // increments c_obj->part_count

  //c_obj->lod_object_list=(lod_object*)Realloc(c_obj->lod_object_list,
    //c_obj->part_count*sizeof(lod_object));

  lod_obj=&(c_obj->lod_object_list[c_obj->part_count-1]);

  //InitLodObject(lod_obj);

  strcpy(lod_obj->file_name, name);
  CheckNameLenght(lod_obj->file_name);
  strcat(lod_obj->file_name, ".3db");

  lod_count = LodCount(node);

  node_list=(mgrec**)Malloc(lod_count*sizeof(*node_list));
  lod_obj->obj_list=(object*)Malloc(sizeof(*(lod_obj->obj_list)));
  in_list=(double*)Malloc(lod_count*sizeof(*in_list));
  out_list=(double*)Malloc(lod_count*sizeof(*out_list));

  GetLodNodes(&node_list, node);

  // get switching distance of LOD nodes
  for(i=0;i<lod_count;i++){
    mgGetAttList(node_list[i], 
      fltLodSwitchIn, &in_list[i],
      fltLodSwitchOut, &out_list[i],
      mgNULL);
  }

  // sort LOD nodes by distance of switch_in
  // cheesy bubble sort but LOD count is always small
  for(j=0; j<lod_count-1; j++){
    for(i=0; i<lod_count-(j+1); i++){
      if(in_list[i] > in_list[i+1]){
        tmp_double=in_list[i];
        in_list[i]=in_list[i+1];
        in_list[i+1]=tmp_double;
        tmp_mgrec=node_list[i];
        node_list[i]=node_list[i+1];
        node_list[i+1]=tmp_mgrec;
      }
    }
  }

  // load highest lod 0
  LoadGGObject(&(lod_obj->obj_list[0]), db, node_list[0], &(c_obj->tl), &(c_obj->atl));

  PostProcessMesh(&(lod_obj->obj_list[0]), density, 100.0f, 0, 0, false, NULL, 1.0f);
  //RemoveUnusedMaterials(&(lod_obj->obj_list[0]));
  //MergeGroupsByMaterial(&(lod_obj->obj_list[0]));
  //SortGroupsByTexture(&(lod_obj->obj_list[0]));
  //SortFacesByArea(&(lod_obj->obj_list[0]));
  //calcVertexNormals(&(lod_obj->obj_list[0]), 0 /*SMOOTH_SHADED*/);
  //calcEdges(&(lod_obj->obj_list[0]));

  // add hard points to lod 0
  mgrec *hp_node;
  char *hp_name; 

  hp_node = mgGetChildNth( mgGetParent(node_list[0]), 1);
  while(hp_node)
  {
    hp_name=mgGetName(hp_node);
    if( mgIsCode(hp_node, fltDof) && strstr(hp_name, "hp_") )
    {
      DefineHardPoint(&(lod_obj->obj_list[0]), hp_node);
    }
    mgFree(hp_name);
    hp_node = mgGetNext(hp_node);
  }

  if(scale_factor != 1.0f)
  {
    ScaleObject(&(lod_obj->obj_list[0]), scale_factor);
  }

  if(lod_percent < 100.0f)
  {
      lod_obj->count = 1;

      object *obj = &(lod_obj->obj_list[0]);
      CollapseEdges(obj, (int)( (.01f * lod_percent) * obj->face_count + .5f) );
      obj->lol.closest = lod_closest;
      obj->lol.furthest = lod_furthest;

      //GenerateLod(lod_obj, lod_percent, lod_closest, lod_furthest);
  }
  else
  {
    lod_obj->count = lod_count;
    lod_obj->obj_list=(object*)Realloc(lod_obj->obj_list,
                      lod_count*sizeof(*(lod_obj->obj_list)));

    // populate lod object
    for(i=1; i<lod_count; i++)
    {
      LoadGGObject(&(lod_obj->obj_list[i]), db, node_list[i],
        &(c_obj->tl), &(c_obj->atl));
  
      PostProcessMesh(&(lod_obj->obj_list[i]), density, 100.0f, 0, 0, false, NULL, 1.0f);
      //RemoveUnusedMaterials(&(lod_obj->obj_list[i]));
      //MergeGroupsByMaterial(&(lod_obj->obj_list[i]));
      //SortGroupsByTexture(&(lod_obj->obj_list[i]));
      //SortFacesByArea(&(lod_obj->obj_list[i]));
      //calcVertexNormals(&(lod_obj->obj_list[i]), 0 /*SMOOTH_SHADED*/);
      //calcEdges(&(lod_obj->obj_list[i]));
      if(scale_factor != 1.0f)
      {
        ScaleObject(&(lod_obj->obj_list[i]), scale_factor);
      }
    }

    // populate switch in distances (0 assumed for o level)
    lod_obj->switch_list=(float*)Malloc((lod_count-1)
                         *sizeof(*(lod_obj->switch_list)));
    for(i=0; i<lod_count-1; i++)
    {
      lod_obj->switch_list[i] = (float)in_list[i];
      if(scale_factor != 1.0f)
      {
        lod_obj->switch_list[i] *= scale_factor;
      }
    } 
  }

  // rigid body is computed on highest detail object
  calcRigidBody(&(lod_obj->obj_list[0]), density, -1, name, true);

  Free(node_list);
  Free(in_list);
  Free(out_list);

  mgFree(name);

  if(verbose_level>=2){
    printf("done w/ ExportLod3DB\n");
  }
}

void Export3DB(mgrec *db, mgrec *node, CompoundObject *c_obj)
{
  char *name;
  object *obj;
  lod_object *l_obj;
  int dummy=0;

  if(IsPlaceHolder(node)){
    dummy=1;
  }

  // insert name into hierarchy 
  name=mgGetName(node);
  if(dummy){
    InsertCompoundName(c_obj, name, name, root_name, NULL, 1, NULL, true); // c_obj->part_count++
  }
  else{
    InsertCompoundName(c_obj, name, name, root_name, ".3db", 1, NULL, true); // c_obj->part_count++
  }

  //c_obj->lod_object_list=(lod_object*)Realloc(c_obj->lod_object_list,
    //c_obj->part_count*sizeof(lod_object));

  l_obj=&(c_obj->lod_object_list[c_obj->part_count-1]);

  //InitLodObject(l_obj);

  if(dummy){
    l_obj->export_flag=0;
  }
  strcpy(l_obj->file_name, name);
  CheckNameLenght(l_obj->file_name);
  strcat(l_obj->file_name, ".3db");

  l_obj->count=1;
  l_obj->obj_list=(object*)Malloc(sizeof(object));
  obj=&(l_obj->obj_list[0]);
  InitObject(obj, &(c_obj->tl), &(c_obj->atl));
  obj->type = FIXED_MESH;
  
  if(HasGeometry(node,1) && !dummy)
  {
    LoadGGObject(obj, db, node, &(c_obj->tl), &(c_obj->atl));
    PostProcessMesh(obj, density, 100.0f, 0, 0, false, NULL, 1.0f);
    //RemoveUnusedMaterials(obj);
    //RemoveUnusedMaterials(obj);
    //SortGroupsByTexture(obj);
    //SortFacesByArea(obj);
    //calcVertexNormals(obj, 0 /*SMOOTH_SHADED*/);
    //calcEdges(obj);

    if(scale_factor != 1.0f){
      ScaleObject(obj, scale_factor);
    }

    if(lod_percent < 100.0f)
    {
      CollapseEdges(obj, (int)( (.01f * lod_percent) * obj->face_count + .5f) );
      obj->lol.closest = lod_closest;
      obj->lol.furthest = lod_furthest;
    }

    calcRigidBody(obj, density, -1, name, true);
  }

  mgFree(name);
}

void GetLodNodes(mgrec ***node_list, mgrec *node)
{
  static int lod_count=0;
  static int level=0;
  int node_type;

  if(!node)
    return;

  node_type=mgGetCode(node);
  if(node_type==fltLod){
    (*node_list)[lod_count]=node;
    lod_count++;
  }

  if((level<2) && 
     ((node_type==fltGroup) || (node_type==fltDof))){
    level++;
    GetLodNodes(node_list, mgGetChild(node));
    level--;
  }

  if(level>0){
    GetLodNodes(node_list, mgGetNext(node));
  }

  if(level==0){
    lod_count=0;
  }
}

int LodCount(mgrec *node)
{
  static int lod_count=0;
  static int level=0;
  int node_type;
  int tmp;

  if(!node)
    return 0;

  node_type=mgGetCode(node);
  if(node_type==fltLod){
    lod_count++;
  }

  if((level<2) && 
     ((node_type==fltGroup) || (node_type==fltDof))){
    level++;
    LodCount(mgGetChild(node));
    level--;
  }

  if(level>0){
    LodCount(mgGetNext(node));
  }
    
  if(lod_count && (level==0)){
    tmp=lod_count;
    lod_count=0;
    return tmp;
  }
  else
    return 0;
}

mgrec* GetParentGroup(mgrec *node)
{
  mgrec *parent;

  if(!node) return mgNULL;
  
  // API bug: mgGetParent(db)should be == NULL but is NOT!
  parent=mgGetParent(node);
  while((parent!=NULL) &&
        (mgGetCode(parent)!=fltHeader) && (mgGetCode(parent)!=fltDof)){
    parent=mgGetParent(parent);
  }
  parent=mgGetParent(parent);

  if((mgGetCode(parent)==fltGroup) && HasGeometry(parent, 1)){
    return parent;
  }
  else{
    return mgNULL;
  }
}
    
void PrintNodeName(mgrec *node)
{
  char *name;

  name=mgGetName(node);
  printf("%s\n",name);
  fflush(stdout);
  mgFree(name);
}

void PrintNodeNameE(mgrec *node)
{
  char *name;

  name=mgGetName(node);
  fprintf(stderr,"%s\n",name);
  fflush(stdout);
  mgFree(name);
}

int HasGeometry(mgrec *node, int first_call_flag)
{
  static int geometry_flag=0;
  int node_type;
  
  if(!node) return 0;

  // char *name;
  // name=mgGetName(node);
  // printf("type=%d %s\n",mgGetCode(node),name);
  // mgFree(name);

  node_type=mgGetCode(node);

  if(first_call_flag){
    geometry_flag=0;

    // sanity check
    if(node_type!=fltGroup){
      fprintf(stderr,
             "Warning: should not call HasGeometry on a non group node.\n");
      PrintNodeName(node);
      return 0;
    }
  }

  if(node_type==fltPolygon){  // we have geometry
    geometry_flag=1;
    return 1;
  }

  // child ALL children except DOF nodes should be looked at
  if((node_type!=fltDof)){
    HasGeometry(mgGetChild(node), 0);
  }

  // sibling ALL siblings except root's siblings should be looked at
  if(!first_call_flag){
    HasGeometry(mgGetNext(node), 0);
  }

  return geometry_flag;
}

int ReadFixed(mgrec *xrec, Fix *fixed)
{
  mgxfllcode xtype;
  double x, y, z;

  xtype=mgGetXformType(xrec); // use fltXmLimitMax ??
  switch(xtype)
  {
    case XLL_TRANSLATE : // translation
    {
                     // fltXmTranslateFrom
      mgGetIcoord(xrec, fltXmTranslateDelta, &x, &y, &z);
      fixed->pos.x=(float)x;
      fixed->pos.y=(float)y;
      fixed->pos.z=(float)z;

      return 1;
    }
    case XLL_ROTPT : // rotation about a point
    {
      mgMatrix matrix;

      if(mgTRUE==mgGetMatrix(xrec, fltMatrix, &matrix)){
        fixed->orient.e00=matrix[0];
        fixed->orient.e01=matrix[4];
        fixed->orient.e02=matrix[8];

        fixed->orient.e10=matrix[1];
        fixed->orient.e11=matrix[5];
        fixed->orient.e12=matrix[9];

        fixed->orient.e20=matrix[2];
        fixed->orient.e21=matrix[6];
        fixed->orient.e22=matrix[10];
#if 0 // wrong order
        fixed->orient.e00=matrix[0];
        fixed->orient.e01=matrix[1];
        fixed->orient.e02=matrix[2];

        fixed->orient.e10=matrix[4];
        fixed->orient.e11=matrix[5];
        fixed->orient.e12=matrix[6];

        fixed->orient.e20=matrix[8];
        fixed->orient.e21=matrix[9];
        fixed->orient.e22=matrix[10];
#endif
        return 1;
      }
      else{
        fprintf(stderr,"Warning: could not get rotation matrix.\n");
        return 0;
      }
    }
    case XLL_ROTEDGE : // rotation about an edge
    {
      fprintf(stderr,"Warning: unsupported XLL_ROTEDGE transformation.\n");
      return 0;
    }
    case XLL_SCALE : // scale about a point
    {
      fprintf(stderr,"Warning: unsupported XLL_SCALE transformation.\n");
      return 0;
    }
    case XLL_TOPOINT : // simultaneous scale and rotate
    {
      fprintf(stderr,"Warning: unsupported XLL_TOPOINT transformation.\n");
      return 0;
    }
    case XLL_PUT : // simultaneous translate, rotate, and scale
    {
      fprintf(stderr,"Warning: unsupported XLL_PUT transformation.\n");
      return 0;
    }
    case XLL_GENERAL : // matrix of one or more undefined tranformations
    {
      fprintf(stderr,"Warning: unsupported XLL_GENERAL transformation.\n");
      return 0;
    }
    default:
      fprintf(stderr,"Error: unsupported DOF transformation.\n");
      exit(1);
  }
  return 0;
}
          
// receives a group node
void MakeDofConnection(CompoundObject *c_obj, mgrec *node)
{
  mgrec *dof_node;
  Fix fixed;
  DofData dof_data;

  GetFixed(node, &fixed);

  dof_node=mgGetParent(node);

  // sanity check
  if(mgGetCode(dof_node)!=fltDof){
    fprintf(stderr,"Error: dof connection w/o a dof node\n");
    exit(0);
  }

  GetDofData(dof_node, &dof_data);

  if(dof_data.type==FFIXED){
    InsertFixed(c_obj, fixed);
  }else
  if(dof_data.type==PRISMATIC){
    Pris prismatic;
    LoadPrismatic(&prismatic, &fixed, &dof_data, PIVOT);
    //AdjustAxis(&dof_data, &(prismatic.axis));
    InsertPris(c_obj, prismatic);
  }else
  if(dof_data.type==REVOLUTE){
    Rev revolute;
    LoadRevolute(&revolute, &fixed, &dof_data, PIVOT);
    //AdjustAxis(&dof_data, &(revolute.axis));
    InsertRev(c_obj, revolute);
  }else
  if(dof_data.type==SPHERICAL){
    fprintf(stderr, "Error: SPHERICAL type not yet supported.\n");
    exit(1);
  } 
  else{
    fprintf(stderr,"Error: bad dof connection type.\n");
    exit(1);
  }
}

void GetFixed(mgrec *node, Fix *fixed)
{
  char name[256];
  char *parent_name;
  int xform_flag;
  mgrec *xrec;

  // init connection data
  InitFixed(fixed);	

  /* name connection */
  strcpy(name, mgGetName(node));
  strtok(name, "#"); // used for multiply referenced nodes w/ different object names
                     // and same file name
  parent_name=mgGetName(GetParentGroup(node));

  if(!strcmp(parent_name, root_name)){
    mgFree(parent_name);
    parent_name=(char*)Malloc((strlen(ROOT_OBJ_NAME)+1)*sizeof(char));
    strcpy(parent_name, ROOT_OBJ_NAME);
  }

  if(verbose_level>=2){
    printf("Fixed parent=%s child=%s\n",parent_name, name);
  }

  CheckNameLenght(name);
  CheckNameLenght(parent_name);
  strcpy(fixed->child, name);
  strcpy(fixed->parent, parent_name);

  mgFree(parent_name);

  xform_flag=0;
  // xform if needed
  if(mgHasXform(node)){

    xrec=mgGetXform(node);
    xform_flag+=ReadFixed(xrec, fixed);
    while(xrec=mgGetNext(xrec)){
      xform_flag+=ReadFixed(xrec, fixed);
    }
  }

  if(xform_flag>2){
    PrintNodeName(node);
    fprintf(stderr,"Error: Group node should only have 2 transforms");
    fprintf(stderr," (translation & rotation)\n");
    exit(1);
  }
}

void LoadGGObject(object *obj, mgrec *db, mgrec *node, txt_lib *tl, anim_txt_lib *atl)
{
  int txt_count;
  int txt_used_count;
  int *txt_used=NULL;
  int mtl_count;
  int combo_count;
  int **mtl_txt_matrix;
  int i;
  int plain_mtl_count;
  int *mtl_used=NULL;

  InitObject(obj, tl, atl);
  obj->type = FIXED_MESH;

  /* identify used textures */
  txt_count=mgGetTextureCount(db);

  if(txt_count>0)
  {
    txt_used=(int*)Malloc(txt_count*sizeof(int));
    memset(txt_used, 0, txt_count*sizeof(int));
    txt_used_count=0;

    // finds textures actually used by faces
    findUsedTextures(node, txt_used, &txt_used_count, 1);

    findEmissiveTextures(db, txt_used, &txt_used_count);

    if(verbose_level>=2){
      printf("%d txt's in flt file; %d used\n",txt_count, txt_used_count);
    }
    /* put textures into library */
    defineTextures(obj->tl, db, txt_used, txt_count);
    Free(txt_used);
  }

  /* identify used plain materials */
  mtl_count=mgGetMaterialCount(db);
  mtl_used=(int*)Malloc(mtl_count*sizeof(int));
  memset(mtl_used, 0, mtl_count*sizeof(int));

  /* identify used material/texture combinations */
  if(txt_count>0){
    mtl_txt_matrix=(int**)Malloc(mtl_count*sizeof(int*));
    mtl_txt_matrix[0]=(int*)Malloc(mtl_count*txt_count*sizeof(int));
    for(i=1;i<mtl_count;i++){
      mtl_txt_matrix[i]=mtl_txt_matrix[0]+i*txt_count;
    }
    memset(mtl_txt_matrix[0], 0, mtl_count*txt_count*sizeof(int));
  }

  combo_count=0;
  plain_mtl_count=0;
  findMtlTxtCombinations(node, mtl_txt_matrix, mtl_used, &combo_count, 
                         &plain_mtl_count, db, 1);
  if(verbose_level>=2){
    printf("%d plain materials used\n",plain_mtl_count);
    printf("%d mtl/txt combinations used\n",combo_count);
  }

  /* put material into library */
  defineMaterials(&(obj->ml), obj->atl, db, mtl_txt_matrix, mtl_used, 
                  mtl_count, txt_count);

  Free(mtl_used);
  Free(mtl_txt_matrix[0]);
  Free(mtl_txt_matrix);

  defineGeometry(obj, node, db, 1);

  //calcVertexNormals(obj, 0 /*SMOOTH_SHADED*/);
  //calcEdges(obj);

  if(verbose_level>=2){
    printf("done w/ LoadGGObject ");
    PrintNodeName(node);
  }
}
      
void MakeAnimConnection(CompoundObject *c_obj, mgrec *node)
{
  int i;
  int children;
  int n_frames;
  Frame *frame_list;
  int rev_flag;
  int pris_flag;
  DofData dof_data;
  Fix fixed;
  mgrec *dof_node;
  
  // read dof node info
  dof_node=mgGetParent(node);
  // sanity check
  if(mgGetCode(dof_node)!=fltDof){
    fprintf(stderr,"Error: dof connection w/o a dof node\n");
    exit(0);
  }

  // read parent group transform
  GetFixed(node, &fixed);
  // read dof transform
  GetDofData(dof_node, &dof_data);

  // read frames in sorted order
  children=mgCountChild(node);
  frame_list=(Frame*)Malloc(children*sizeof(Frame));
  n_frames=ReadFrames(node, frame_list, children);

  // check for consistent pivot point for frames 2 to n-1
  for(i=2; i<n_frames-1; i++){
    if(frame_list[1].center.x!=frame_list[i].center.x){
      fprintf(stderr,"Warning: inconsistent pivot in x frame %d.\n",i);
      break;
    }
    if(frame_list[1].center.y!=frame_list[i].center.y){
      fprintf(stderr,"Warning: inconsistent pivot in y frame %d.\n",i);
      break;
    }
    if(frame_list[1].center.z!=frame_list[i].center.z){
      fprintf(stderr,"Warning: inconsistent pivot in z frame %d.\n",i);
      break;
    }
  }
  // assign pivot to dof node
  dof_data.pivot[0]=frame_list[1].center.x;
  dof_data.pivot[1]=frame_list[1].center.y;
  dof_data.pivot[2]=frame_list[1].center.z;

  // decide connection type
  pris_flag=0;
  rev_flag=0;
  for(i=0; i<n_frames; i++){
    pris_flag+=frame_list[i].pris_count;
    rev_flag+=frame_list[i].rev_count;
  }

  // sanity checks
  if(pris_flag && rev_flag){
    fprintf(stderr,
      "Error: can't animate rotation & translation simultaneously.\n");
    exit(1);
  }else
  if(!pris_flag && !rev_flag){
    fprintf(stderr,
      "Error: no rotation or translation in animation.\n");
    exit(1);
  }else
  if(rev_flag){
    RevAnimation(c_obj, &fixed, &dof_data, frame_list, n_frames, dof_node);
  }else
  if(pris_flag){
    PrisAnimation(c_obj, &fixed, &dof_data, frame_list, n_frames, dof_node);
  }

  for(i=0; i<children; i++){
    mgFree(frame_list[i].name);
	// Free(frame_list[i].parent_name);
  }
  Free(frame_list);
}

int FrameNumber(char *name)
{
  int i;
  int length;
  char *pt;

  length=strlen(name);

  for(i=length-1; i>=0; i--){
    pt=name+i;
    if(*pt < 48 || *pt > 57){
      break;
    }
  }
  pt++;

  if(*pt < 48 || *pt > 57){
    return(INT32_MAX);
  }
  else{
    return(atoi(pt));
  }
}

int ReadTransform(mgrec *xrec, Frame *frame)
{
  mgxfllcode xtype;

  xtype=mgGetXformType(xrec); // use fltXmLimitMax ??
  switch(xtype)
  {
    case XLL_TRANSLATE : // translation
    {
      double x,y,z;

      frame->pris_count++;
      mgGetIcoord(xrec, fltXmTranslateDelta, &x, &y, &z);
      frame->delta.x=(float)x;
      frame->delta.y=(float)y;
      frame->delta.z=(float)z;

      // presently not used for anything
      mgGetIcoord(xrec, fltXmTranslateFrom, &x, &y, &z);
      frame->from.x=(float)x;
      frame->from.y=(float)y;
      frame->from.z=(float)z;

      return 1;
    }
    case XLL_ROTPT : // rotation about a point
    {
      mgMatrix matrix;
      double x,y,z;
      float angle;
      float i,j,k;
 
      frame->rev_count++;
      mgGetIcoord (xrec, fltXmRotateCenter, &x, &y, &z);
      frame->center.x=(float)x;
      frame->center.y=(float)y;
      frame->center.z=(float)z;

      // presently not used for anything
      mgGetAttList (xrec, fltVectorI, &i, fltVectorJ, &j, fltVectorK, &k, mgNULL);
      frame->vector_r.x=i;
      frame->vector_r.y=j;
      frame->vector_r.z=k;

      mgGetAttList (xrec, fltXmRotateAngle, &angle, mgNULL);
      frame->angle=angle*D2R;

      if(mgTRUE==mgGetMatrix(xrec, fltMatrix, &matrix)){
        frame->orient=MMul(frame->orient, matrix);
        return 1;
      }
      else{
        fprintf(stderr,"Warning: could not get rotation matrix.\n");
        return 0;
      }
    }
    case XLL_ROTEDGE : // rotation about an edge
    {
      fprintf(stderr,"Warning: unsupported XLL_ROTEDGE transformation.\n");
      return 0;
    }
    case XLL_SCALE : // scale about a point
    {
      fprintf(stderr,"Warning: unsupported XLL_SCALE transformation.\n");
      return 0;
    }
    case XLL_TOPOINT : // simultaneous scale and rotate
    {
      fprintf(stderr,"Warning: unsupported XLL_TOPOINT transformation.\n");
      return 0;
    }
    case XLL_PUT : // simultaneous translate, rotate, and scale
    {
      fprintf(stderr,"Warning: unsupported XLL_PUT transformation.\n");
      return 0;
    }
    case XLL_GENERAL : // matrix of one or more undefined tranformations
    {
      fprintf(stderr,"Warning: unsupported XLL_GENERAL transformation.\n");
      return 0;
    }
    default:
      fprintf(stderr,"Error: unsupported DOF transformation.\n");
      exit(1);
  }
  return 0;
}

void GetDofData(mgrec *dof_node, DofData *data)
{
  char *name;
  double pivot[3];
  double min_t[3], max_t[3];
  double min_r[3], max_r[3];
  double axis_pt[3]; 
  double plane_pt[3];

  InitDofData(data);

  if((!dof_node) || (mgGetCode(dof_node)!=fltDof))
    return;
      
  name=mgGetName(dof_node);
  strcpy(data->name,name);
  mgFree(name);

  mgGetAttList(dof_node, 
    fltDofPutAnchorX, &pivot[0],
    fltDofPutAnchorY, &pivot[1],
    fltDofPutAnchorZ, &pivot[2],

    fltDofMinX, &min_t[0], 
    fltDofMinY, &min_t[1], 
    fltDofMinZ, &min_t[2],
    fltDofMaxX, &max_t[0], 
    fltDofMaxY, &max_t[1], 
    fltDofMaxZ, &max_t[2],

    fltDofMinAzim, &min_r[0], 
    fltDofMinIncl, &min_r[1], 
    fltDofMinTwist, &min_r[2],
    fltDofMaxAzim, &max_r[0], 
    fltDofMaxIncl, &max_r[1], 
    fltDofMaxTwist, &max_r[2],

    fltDofPutAlignX, &axis_pt[0], 
    fltDofPutAlignY, &axis_pt[1], 
    fltDofPutAlignZ, &axis_pt[2], 

    fltDofPutTrackX, &plane_pt[0],
    fltDofPutTrackY, &plane_pt[1],
    fltDofPutTrackZ, &plane_pt[2],
    mgNULL);

  data->pivot[0]=(float)pivot[0];
  data->pivot[1]=(float)pivot[1];
  data->pivot[2]=(float)pivot[2];
  
  data->min_t[0]=(float)min_t[0];
  data->min_t[1]=(float)min_t[1];
  data->min_t[2]=(float)min_t[2];
  data->max_t[0]=(float)max_t[0];
  data->max_t[1]=(float)max_t[1];
  data->max_t[2]=(float)max_t[2];

  data->min_r[0]=(float)min_r[0];
  data->min_r[1]=(float)min_r[1];
  data->min_r[2]=(float)min_r[2];
  data->max_r[0]=(float)max_r[0];
  data->max_r[1]=(float)max_r[1];
  data->max_r[2]=(float)max_r[2];

  data->axis_pt[0]=(float)axis_pt[0];
  data->axis_pt[1]=(float)axis_pt[1];
  data->axis_pt[2]=(float)axis_pt[2];

  data->plane_pt[0]=(float)plane_pt[0];
  data->plane_pt[1]=(float)plane_pt[1];
  data->plane_pt[2]=(float)plane_pt[2];

  if(verbose_level>=3){
    printf("axis pt  %f %f %f\n",data->axis_pt[0], 
           data->axis_pt[1], data->axis_pt[2]);
    printf("plane pt %f %f %f\n",data->plane_pt[0], 
           data->plane_pt[1], data->plane_pt[2]);
    printf("min_r %f %f %f\n",data->min_r[0], data->min_r[1], data->min_r[2]);
    printf("max_r %f %f %f\n",data->max_r[0], data->max_r[1], data->max_r[2]);
  }

  data->min_r[0]*=D2R;
  data->min_r[1]*=D2R;
  data->min_r[2]*=D2R;
  data->max_r[0]*=D2R;
  data->max_r[1]*=D2R;
  data->max_r[2]*=D2R;

  CleanDofData(data);
}
    
PersistMatrix MMul(PersistMatrix pm, mgMatrix mgm)
{
  PersistMatrix result;

  result.e00 = mgm[0]*pm.e00 + mgm[1]*pm.e10 + mgm[2]*pm.e20;
  result.e10 = mgm[4]*pm.e00 + mgm[5]*pm.e10 + mgm[6]*pm.e20;
  result.e20 = mgm[8]*pm.e00 + mgm[9]*pm.e10 + mgm[10]*pm.e20;

  result.e01 = mgm[0]*pm.e01 + mgm[1]*pm.e11 + mgm[2]*pm.e21;
  result.e11 = mgm[4]*pm.e01 + mgm[5]*pm.e11 + mgm[6]*pm.e21;
  result.e21 = mgm[8]*pm.e01 + mgm[9]*pm.e11 + mgm[10]*pm.e21;

  result.e02 = mgm[0]*pm.e02 + mgm[1]*pm.e12 + mgm[2]*pm.e22;
  result.e12 = mgm[4]*pm.e02 + mgm[5]*pm.e12 + mgm[6]*pm.e22;
  result.e22 = mgm[8]*pm.e02 + mgm[9]*pm.e12 + mgm[10]*pm.e22;

#if 0 // wrong order
  result.e00 = pm.e00*mgm[0] + pm.e01*mgm[4] + pm.e02*mgm[8];
  result.e10 = pm.e10*mgm[0] + pm.e11*mgm[4] + pm.e12*mgm[8];
  result.e20 = pm.e20*mgm[0] + pm.e21*mgm[4] + pm.e22*mgm[8];

  result.e01 = pm.e00*mgm[1] + pm.e01*mgm[5] + pm.e02*mgm[9];
  result.e11 = pm.e10*mgm[1] + pm.e11*mgm[5] + pm.e12*mgm[9];
  result.e21 = pm.e20*mgm[1] + pm.e21*mgm[5] + pm.e22*mgm[9];

  result.e02 = pm.e00*mgm[2] + pm.e01*mgm[6] + pm.e02*mgm[10];
  result.e12 = pm.e10*mgm[2] + pm.e11*mgm[6] + pm.e12*mgm[10];
  result.e22 = pm.e20*mgm[2] + pm.e21*mgm[6] + pm.e22*mgm[10];
#endif

  return result;
}      

Matrix PersistMatrix_to_Matrix(PersistMatrix pm)
{
  Matrix m;

  m.d[0][0]=pm.e00;
  m.d[0][1]=pm.e01;
  m.d[0][2]=pm.e02;

  m.d[1][0]=pm.e10;
  m.d[1][1]=pm.e11;
  m.d[1][2]=pm.e12;

  m.d[2][0]=pm.e20;
  m.d[2][1]=pm.e21;
  m.d[2][2]=pm.e22;

  return m;
}
 
// gets animation transform frame data 
int ReadFrames(mgrec *node, Frame *frame_list, int count)
{
  int n_frames;
  int i,j;
  Frame tmp;
  mgrec *xrec;
  mgrec *next;

  n_frames=0;
  next=mgGetChild(node);
  for(i=0;i<count;i++)
  {
    InitFrame(&frame_list[i]);
    frame_list[i].name=mgGetName(next);
    frame_list[i].index=FrameNumber(frame_list[i].name);
    // only group nodes are valid frames
    if( ((mgGetCode(next)!=fltGroup) && (mgGetCode(next)!=fltObject)) ||
        !mgHasXform(next) )
    {
      frame_list[i].index=INT32_MAX;
    }
    else
    {
      //printf("%d %s\n", n_frames, frame_list[i].name);

      n_frames++;
      // read transform
      if(mgHasXform(next))
      {
        xrec=mgGetXform(next);
        while(xrec)
        {
          ReadTransform(xrec, &frame_list[i]);
          xrec=mgGetNext(xrec);
        }
      }
      if((frame_list[i].rev_count>3) || (frame_list[i].pris_count>1)){
        PrintNodeName(next);
        fprintf(stderr,"Error: Group node should only have 4 transforms");
        fprintf(stderr," (translation & 3 rotations)\n");
        exit(1);
      }
    }
    next=mgGetNext(next);
  }
  if(next!=NULL){ // sanity check
    fprintf(stderr,"Error: too many frame nodes.\n");
  }

  // sort frames (cheesy bubble sort)
  for(j=0; j<count-1; j++){
    for(i=0; i<count-(j+1); i++){
      if(frame_list[i].index>frame_list[i+1].index){
        tmp=frame_list[i];
        frame_list[i]=frame_list[i+1];
        frame_list[i+1]=tmp;
      }
    }
  }

  return n_frames;
}
    
void RevAnimation(CompoundObject *c_obj, Fix *fixed, DofData *dof_data, 
                  Frame *frame_list, int n_frames, mgrec *dof_node)
{
  int i;
  char *dof_name;
  int type;

  // compute axis and angle
  Matrix m;
  float magnitude;

  // compute axis and angle per frame
  for(i=0; i<n_frames; i++)
  {
    m=PersistMatrix_to_Matrix(frame_list[i].orient);
    mat_to_qt(&frame_list[i].quat, &m);
    // is this same as loaded angle ??
    if(frame_list[i].quat.v.w > 1.0)
      frame_list[i].quat.v.w=1.0;
    if(frame_list[i].quat.v.w < -1.0)
      frame_list[i].quat.v.w=-1.0;
    frame_list[i].angle=2.0*acos(frame_list[i].quat.v.w);  
    frame_list[i].angle=NormAngle(frame_list[i].angle);

    // flip axis (this should not be becessary but is ???)
    frame_list[i].quat.v.x=-frame_list[i].quat.v.x;
    frame_list[i].quat.v.y=-frame_list[i].quat.v.y;
    frame_list[i].quat.v.z=-frame_list[i].quat.v.z;
    
    frame_list[i].vector_r.x=frame_list[i].quat.v.x;
    frame_list[i].vector_r.y=frame_list[i].quat.v.y;
    frame_list[i].vector_r.z=frame_list[i].quat.v.z;

    magnitude=sqrt(frame_list[i].vector_r.x*frame_list[i].vector_r.x +
              frame_list[i].vector_r.y*frame_list[i].vector_r.y +
              frame_list[i].vector_r.z*frame_list[i].vector_r.z);
    frame_list[i].vector_r.x/=magnitude; 
    frame_list[i].vector_r.y/=magnitude; 
    frame_list[i].vector_r.z/=magnitude; 

    if(verbose_level >= 5){
      printf("raw %d %f %f %f %f\n",i,frame_list[i].angle/D2R,
        frame_list[i].vector_r.x,
        frame_list[i].vector_r.y,
        frame_list[i].vector_r.z);
    }
  }

  CleanFrames(frame_list, n_frames);

  if(verbose_level>=3){
    for(i=0; i<n_frames; i++){
      printf("angle %f   axis %f %f %f\n",frame_list[i].angle/D2R,
           frame_list[i].vector_r.x, 
           frame_list[i].vector_r.y, 
           frame_list[i].vector_r.z);
    }
  }

  float min_v[3], max_v[3];
  type=AnimAxisMinMax(frame_list, n_frames, dof_data->axis, &dof_data->min_angle,
             &dof_data->max_angle, REVOLUTE, min_v, max_v);

  NamedChannel n_channel;
  InitNamedChannel(&n_channel);
  dof_name=mgGetName(dof_node);
  sprintf(n_channel.name, "Ch_%s", dof_name);
  mgFree(dof_name);
  n_channel.first_frame=frame_list[0].index;
  n_channel.last_frame=frame_list[n_frames-1].index;

  if(type==REVOLUTE){
    if(verbose_level>=3){
      printf("final min=%f max=%f axis %f %f %f\n",
              dof_data->min_angle/D2R, dof_data->max_angle/D2R,
              dof_data->axis[0], dof_data->axis[1], dof_data->axis[2]);
    }

    Rev revolute;
  
    LoadRevolute(&revolute, fixed, dof_data, ANIM);
    // AdjustAxis(dof_data, &(revolute.axis));
    c_obj->rev_count++;
    c_obj->rev_list=(Rev*)Realloc(c_obj->rev_list,
                     c_obj->rev_count*sizeof(Rev));
    c_obj->rev_list[c_obj->rev_count-1]=revolute;
  
  
    n_frames++; // dummy first frame
    n_channel.channel.header.frames=n_frames;
    n_channel.channel.header.capture_rate=1.0/fps;
    n_channel.channel.header.type=PersistDT_FLOAT;
    n_channel.channel.data=
      (unsigned char*)Malloc(n_channel.channel.header.frames*sizeof(float));
  
  
    *(float*)(n_channel.channel.data)=0.0; // add stating frame at 0
    // printf("%d %.3f\n",0, 0.0);
    for(i=1;i<n_frames;i++){
      *(float*)(n_channel.channel.data+sizeof(float)*i)=
      frame_list[i-1].angle;
      // printf("%d %.3f\n",i, *(float*)(n_channel.channel.data+4*i)/D2R);
    }
  }
  else{ // SPHERICAL

    PersistSphere sphere;
  
    LoadSphere(&sphere, fixed, dof_data, ANIM);

    c_obj->sphere_count++;
    c_obj->sphere_list=(PersistSphere*)Realloc(c_obj->sphere_list,
                       c_obj->sphere_count*sizeof(PersistSphere));
    c_obj->sphere_list[c_obj->sphere_count-1]=sphere;
  
    n_frames++; // dummy first frame
    n_channel.channel.header.frames=n_frames;
    n_channel.channel.header.capture_rate=1.0/fps;
    n_channel.channel.header.type=PersistDT_QUATERNION;
    n_channel.channel.data=
      (unsigned char*)Malloc(n_channel.channel.header.frames*
      sizeof(PersistQuaternion));
  
    PersistQuaternion pq;
    pq.w=1.0f; 
    pq.x=pq.y=pq.z=0.0f;
    *(PersistQuaternion*)(n_channel.channel.data)=pq; // add stating frame at 0
    for(i=1;i<n_frames;i++){
      *(PersistQuaternion*)
       (n_channel.channel.data+sizeof(PersistQuaternion)*i)=
       frame_list[i-1].quat.v;
#if 0
      (*(PersistQuaternion*)
        (n_channel.channel.data+sizeof(PersistQuaternion)*i)).w=
        cos(frame_list[i-1].angle * .5);
      (*(PersistQuaternion*)
        (n_channel.channel.data+sizeof(PersistQuaternion)*i)).x=
        frame_list[i].vector.x * sin(frame_list[i-1].angle * .5);
      (*(PersistQuaternion*)
        (n_channel.channel.data+sizeof(PersistQuaternion)*i)).y=
        frame_list[i].vector.y * sin(frame_list[i-1].angle * .5);
      (*(PersistQuaternion*)
        (n_channel.channel.data+sizeof(PersistQuaternion)*i)).z=
        frame_list[i].vector.z * sin(frame_list[i-1].angle * .5);
#endif
    }
  }
  
  NamedScript script;
  InitScript(&script);
  script.channel_count=1;
  script.channel_list=(PersistAnimChannelMapping*)
                      Malloc(sizeof(PersistAnimChannelMapping));
  strcpy(script.channel_list[0].parent, fixed->parent);
  strcpy(script.channel_list[0].child, fixed->child);
  strcpy(script.channel_list[0].channel, n_channel.name);

  // add data to cmp object
  InsertNamedChannel(c_obj, n_channel);
  InsertScript(c_obj, script);
}

void PrisAnimation(CompoundObject *c_obj, Fix *fixed, DofData *dof_data, 
                   Frame *frame_list, int n_frames, mgrec *dof_node)
{
  int i;
  char *dof_name;
  float magnitude;

  // compute axis and step
  for(i=0; i<n_frames; i++){
    frame_list[i].vector_t.x=frame_list[i].delta.x;
    frame_list[i].vector_t.y=frame_list[i].delta.y;
    frame_list[i].vector_t.z=frame_list[i].delta.z;

    magnitude=sqrt(frame_list[i].vector_t.x*frame_list[i].vector_t.x +
              frame_list[i].vector_t.y*frame_list[i].vector_t.y +
              frame_list[i].vector_t.z*frame_list[i].vector_t.z);
    frame_list[i].step=magnitude;
    frame_list[i].vector_t.x/=magnitude; 
    frame_list[i].vector_t.y/=magnitude; 
    frame_list[i].vector_t.z/=magnitude; 
   
    if(verbose_level>=3){
      printf("step %f   axis %f %f %f\n",frame_list[i].step,
           frame_list[i].vector_t.x, 
           frame_list[i].vector_t.y, 
           frame_list[i].vector_t.z);
    }
  }

  CleanFrames(frame_list, n_frames);

  float min_v[3], max_v[3];
  AnimAxisMinMax(frame_list, n_frames, dof_data->axis, &dof_data->min_step,
             &dof_data->max_step, PRISMATIC, min_v, max_v);

  Pris prismatic;

  LoadPrismatic(&prismatic, fixed, dof_data, ANIM);
  //AdjustAxis(dof_data, &(prismatic.axis));
  c_obj->pris_count++;
  c_obj->pris_list=(Pris*)Realloc(c_obj->pris_list,
                   c_obj->pris_count*sizeof(Pris));
  c_obj->pris_list[c_obj->pris_count-1]=prismatic;

  NamedChannel n_channel;
  InitNamedChannel(&n_channel);
  dof_name=mgGetName(dof_node);
  sprintf(n_channel.name, "Ch_%s", dof_name);
  mgFree(dof_name);
  n_channel.first_frame=frame_list[0].index;
  n_channel.last_frame=frame_list[n_frames-1].index;

  n_frames++; // dummy first frame
  n_channel.channel.header.frames=n_frames;
  n_channel.channel.header.capture_rate=1.0/fps;
  n_channel.channel.header.type=PersistDT_FLOAT;
  n_channel.channel.data=
    (unsigned char*)Malloc(n_channel.channel.header.frames*sizeof(float));

  *(float*)(n_channel.channel.data)=0.0;  // add first frame
  for(i=1;i<n_frames;i++){
    *(float*)(n_channel.channel.data+4*i)=frame_list[i-1].step;
  }

  NamedScript script;
  InitScript(&script);
  script.channel_count=1;
  script.channel_list=(PersistAnimChannelMapping*)
                      Malloc(sizeof(PersistAnimChannelMapping));
  strcpy(script.channel_list[0].parent, fixed->parent);
  strcpy(script.channel_list[0].child, fixed->child);
  strcpy(script.channel_list[0].channel, n_channel.name);

  // add data to cmp object
  InsertNamedChannel(c_obj, n_channel);
  InsertScript(c_obj, script);
}

void DefineHardPoint( object *obj, mgrec *dof_node)
{
  DofData dof_data;
  Matrix m;
  char *dof_name;
  PersistHPFixed hp_fixed;

  dof_name=mgGetName(dof_node);

  GetDofData(dof_node, &dof_data);
  GetRotMatrix(&dof_data, &m);

  InitHPFixed(&hp_fixed);
  strcpy(hp_fixed.name, dof_name);
  hp_fixed.point.x=dof_data.pivot[0];
  hp_fixed.point.y=dof_data.pivot[1];
  hp_fixed.point.z=dof_data.pivot[2];

  Matrix_to_PersistMatrix(&m, &hp_fixed.orientation);
   
  if(dof_data.type==FFIXED){ // parent fixed hard point
    obj->hp_fix_count++;
    obj->hp_fix_list=(PersistHPFixed*)Realloc(obj->hp_fix_list,
                     obj->hp_fix_count*sizeof(PersistHPFixed)); 
    obj->hp_fix_list[obj->hp_fix_count-1]=hp_fixed;
  }else

  if(dof_data.type==PRISMATIC){ // child pris hard point
    obj->hp_pris_count++;
    obj->hp_pris_list=(PersistHPPrismatic*)Realloc(obj->hp_pris_list,
                     obj->hp_pris_count*sizeof(PersistHPPrismatic)); 

    // SetIdentityPersistMatrix(&(hp_fixed.orientation)); // if we wanted to rotate axis
    obj->hp_pris_list[obj->hp_pris_count-1].spot=hp_fixed;
#if 0
    DofAxisMinMax(&dof_data,
      &(obj->hp_pris_list[obj->hp_pris_count-1].axis),
      &(obj->hp_pris_list[obj->hp_pris_count-1].min),
      &(obj->hp_pris_list[obj->hp_pris_count-1].max), HP);
#endif
    obj->hp_pris_list[obj->hp_pris_count-1].axis.x=dof_data.axis[0];
    obj->hp_pris_list[obj->hp_pris_count-1].axis.y=dof_data.axis[1];
    obj->hp_pris_list[obj->hp_pris_count-1].axis.z=dof_data.axis[2];
    obj->hp_pris_list[obj->hp_pris_count-1].min=dof_data.min_step;
    obj->hp_pris_list[obj->hp_pris_count-1].max=dof_data.max_step;
  }else

  if(dof_data.type==REVOLUTE){ // child rev hard point
    obj->hp_rev_count++;
    obj->hp_rev_list=(PersistHPRevolute*)Realloc(obj->hp_rev_list,
                     obj->hp_rev_count*sizeof(PersistHPRevolute)); 

    // SetIdentityPersistMatrix(&(hp_fixed.orientation));  // if we wanted to rotate axis
    obj->hp_rev_list[obj->hp_rev_count-1].spot=hp_fixed;
#if 0
    DofAxisMinMax(&dof_data,
      &(obj->hp_rev_list[obj->hp_rev_count-1].axis),
      &(obj->hp_rev_list[obj->hp_rev_count-1].min),
      &(obj->hp_rev_list[obj->hp_rev_count-1].max), HP);
#endif
    obj->hp_rev_list[obj->hp_rev_count-1].axis.x=dof_data.axis[0];
    obj->hp_rev_list[obj->hp_rev_count-1].axis.y=dof_data.axis[1];
    obj->hp_rev_list[obj->hp_rev_count-1].axis.z=dof_data.axis[2];
    obj->hp_rev_list[obj->hp_rev_count-1].min=dof_data.min_angle;
    obj->hp_rev_list[obj->hp_rev_count-1].max=dof_data.max_angle;
  }

  else{
    fprintf(stderr,"Error: bad hard point type.\n");
    exit(1);
  }
  mgFree(dof_name);
}

void GetRotMatrix(DofData *dof_data, Matrix *m)
{
    float ii[3], jj[3], kk[3];
    float v[3];

    // adjust dof axis to world coordinates 
    ii[0]=dof_data->axis_pt[0]-dof_data->pivot[0];
    ii[1]=dof_data->axis_pt[1]-dof_data->pivot[1];
    ii[2]=dof_data->axis_pt[2]-dof_data->pivot[2];
    Normalize3(ii);

    v[0]=dof_data->plane_pt[0]-dof_data->pivot[0];
    v[1]=dof_data->plane_pt[1]-dof_data->pivot[1];
    v[2]=dof_data->plane_pt[2]-dof_data->pivot[2];
    Normalize3(v);

    Cross(ii, v, kk);
    Normalize3(kk);

    Cross(kk, ii, jj);
    Normalize3(jj);

    m->d[0][0]=ii[0];
    m->d[1][0]=ii[1];
    m->d[2][0]=ii[2];

    m->d[0][1]=jj[0];
    m->d[1][1]=jj[1];
    m->d[2][1]=jj[2];

    m->d[0][2]=kk[0];
    m->d[1][2]=kk[1];
    m->d[2][2]=kk[2];
}

int IsPlaceHolder(mgrec *node)
{
  if(!node) return 0;
  if(!mgIsCode(node, fltGroup)) return 0;
  if(!mgIsCode(mgGetParent(node), fltDof)) return 0;

  char *name;
  name=mgGetName(mgGetParent(node));
  if(!strncmp(name, "null",4)){
    mgFree(name);
    return 1;
  }
  else{
    mgFree(name);
    return 0;
  }
}

int Winprint(const char *format, ...)
{
  va_list args;
  char buffer[256];

  va_start(args, format);
  vsprintf(buffer, format, args);
  va_end(args);

  fprintf(stderr,"%s\n",buffer);
  fflush(stderr);

  return 0;
}

#if 0
void AdjustAxis(DofData *dof_data, PersistVector *axis)
{
  Matrix m;
  float v[3];

  // adjust axis orientation for non major axes
  GetRotMatrix(dof_data, &m);

  v[0]=axis->x;
  v[1]=axis->y;
  v[2]=axis->z;

  axis->x = v[0]*m.d[0][0] + v[1]*m.d[0][1] + v[2]*m.d[0][2];
  axis->y = v[0]*m.d[1][0] + v[1]*m.d[1][1] + v[2]*m.d[1][2];
  axis->z = v[0]*m.d[2][0] + v[1]*m.d[2][1] + v[2]*m.d[2][2];
}
#endif
