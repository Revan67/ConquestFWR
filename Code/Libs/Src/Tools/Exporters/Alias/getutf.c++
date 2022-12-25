// check
/*
 * 1996 - Alias|Wavefront
 */
#include <stdio.h>
#include <math.h>
#include <stdarg.h>
#include <assert.h>
#include <GL/glx.h>
#include <GL/gl.h>
#include <AlObject.h>
#include <AlDagNode.h>
#include <AlGroupNode.h>
#include <AlJoint.h>
#include <AlUniverse.h>
#include <AlShader.h>
#include <AlTexture.h>
#include <AlXevents.h>
#include <AlRenderInfo.h>
#include <AlAnimatable.h>
#include <AlChannel.h>
#include <AlAction.h>
#include <AlMotionAction.h>
#include <AlClusterNode.h>
#include <AlCluster.h>
#include <AlClusterMember.h>
#include <AlClusterable.h>
#include <AlPolysetNode.h>
#include <AlPolyset.h>
#include <AlPolysetVertex.h>
#include <AlPolygon.h>
#include <AlTM.h>
#include <AlLiveData.h>
#include <AlViewFrame.h>
#include <AlRetrieveOptions.h>

#include <Vk/VkApp.h>
#include <Vk/VkResource.h>
#include <Vk/VkSimpleWindow.h>

#include <X11/keysym.h>

extern "C" {
#include <sys/types.h>
#include <unistd.h>
#include <Dt.h>
#include <DtOM.h>
}

#include <Vk/VkApp.h>
#include <Vk/VkResource.h>
#include <Vk/VkSimpleWindow.h>

#include <utfbulletinboard.h>
#include <utfbulletinboardui.h>
#include <utfconverter.h>

#define DEFINE_UTF /* used in options.h */
#include "options.h"

#include "sgi_utf.h"
#include "rgbutils.h"
#include "3db.h"
#include "cmp.h"
#include "bone.h"

#define START_TIME 0

enum{LIGHTSOURCE=1, LAMBERT, PHONG, BLINN};

static FILE *batch_fp = NULL;

float face_normal_tolerance = FACE_NORMAL_TOLERANCE;
float vertex_normal_tolerance = VERTEX_NORMAL_TOLERANCE;

// cont LOD stuff
float lod_percent = 100.0f;
float lod_closest = 100.0f;
float lod_furthest = 1000.0f;
float lod_mtl_weight = 5.0f;
#define INIT_UV_WEIGHT 0.96f
float lod_uv_weight = INIT_UV_WEIGHT; // (0-1) .85 normal

//#define DEBUG
#undef DEBUG
int verbose_level = 0;
float fps = 30.0f;
int mip_flag; // !utf_no_mipmaps
int dither_flag = 0; // !utf_no_mipmaps
int txt_flag; // utf_output_textures
//int flag_565 = 0;
int txt_depth = PAL8;
int convex_hull_flag;
bool remove_constant_channels;
float scale_factor = 1.0f;
int split_flag = 0;
char *root_name = NULL;
bool exporting_deformable = false;
bool absolute_rev = true;
bool use_heading = true;

//int old_style = 0;
int no_physics = 0;
double init_time;
int export_vertex_colors = 0;
int ik_extents = 0;
int center_mass = 0;

char     utf_title[] = "Alias|Wavefront Real Time Games Output";
char     utf_version[] = "v2.4";

static int running = 0;
Display *dpy;

FILE *std_err;
FILE *std_out;

inline void SetTime(double t, AlDagNode *node);
inline void SetTime(double t);
void RotateByX(AlTM & tm, const double angle);
void RotateByY(AlTM & tm, const double angle);
void RotateByZ(AlTM & tm, const double angle);
void DumpAlTM(const AlTM& tm);
void DumpPT(const PersistTransform & tm);
AlGroupNode* GetHeadingNode(AlDagNode *aldagnode);
AlGroupNode* GetSkelRootNode(AlDagNode *aldagnode);

inline void CleanD( double & d )
{
  if( fabs(d) < .00001 )
  {
    d = 0.0;
  }
}

void CleanAlTM( AlTM & tm )
{
  CleanD( tm[0][0] ); CleanD( tm[0][1] ); CleanD( tm[0][2] ); CleanD( tm[0][3] );
  CleanD( tm[1][0] ); CleanD( tm[1][1] ); CleanD( tm[1][2] ); CleanD( tm[1][3] );
  CleanD( tm[2][0] ); CleanD( tm[2][1] ); CleanD( tm[2][2] ); CleanD( tm[2][3] );
  CleanD( tm[3][0] ); CleanD( tm[3][1] ); CleanD( tm[3][2] ); CleanD( tm[3][3] );
}

class GlobalFrame
{
public:

  void GT(AlDagNode *node, double t, double & x, double & y, double & z) const
  {
    SetTime(t, node);
    node->rotatePivot(x, y, z);
    CleanD( x );
    CleanD( y );
    CleanD( z );
  }

  AlTM GT(AlDagNode *node, double t) const
  {
    AlTM tm(1,1,1,1);
    GT(node, t, tm[3][0], tm[3][1], tm[3][2]);
    return tm;
  }

  AlTM GR(AlDagNode *node, double t) const
  {
    SetTime(t, node);
    AlTM tm(1,1,1,1);
    while(node)
    {
      double rx, ry, rz;
      node->rotation(rx, ry, rz);

      RotateByX(tm, rx*D2R);
      RotateByY(tm, ry*D2R);
      RotateByZ(tm, rz*D2R);
      //tm *= AlTM::rotateX(rx*D2R) * AlTM::rotateY(ry*D2R) * AlTM::rotateZ(rz*D2R);

      node = node->parentNode();
    }

    CleanAlTM ( tm );
    return tm;
  }

  AlTM GX(AlDagNode *node, double t) const
  {
    AlTM tm ( GR(node, t) );
    GT(node, t, tm[3][0], tm[3][1], tm[3][2]);
    return tm;
  }

  AlTM DGX(AlDagNode *r_node, AlDagNode *t_node, double t) const
  {
    AlTM tm ( GR(r_node, t) );
    GT(t_node, t, tm[3][0], tm[3][1], tm[3][2]);
    return tm;
  }

  void LT(AlDagNode *node, AlDagNode *parent, double t,
          double & x, double & y, double & z) const
  {
    SetTime(t, parent); SetTime(t, node);
    GT(node, t, x, y, z);
    GX(parent, t).inverse().transPoint(x, y, z);
    CleanD( x );
    CleanD( y );
    CleanD( z );
  }

  AlTM LT(AlDagNode *node, AlDagNode *parent, double t) const
  {
    AlTM tm(1,1,1,1);
    LT(node, parent, t, tm[3][0], tm[3][1], tm[3][2]);
    return tm;
  }

  AlTM LR(AlDagNode *node, AlDagNode *parent, double t) const
  {
    SetTime(t, parent); SetTime(t, node);
    AlTM tm( GR(node, t) * GR(parent, t).inverse() );
    CleanAlTM( tm );
    return tm;
  }

  AlTM LX(AlDagNode *node, AlDagNode *parent, double t) const
  {
    SetTime(t, parent); SetTime(t, node);
    AlTM tm ( GX(node, t) * GX(parent, t).inverse() );
    CleanAlTM( tm );
    return tm;
  }

  AlTM DLX(AlDagNode *r_node, AlDagNode *t_node,
           AlDagNode *r_parent, AlDagNode *t_parent, double t) const
  {
    SetTime(t, r_parent); 
    SetTime(t, t_parent); 
    SetTime(t, r_node);
    SetTime(t, t_node);
    AlTM tm(  DGX(r_node, t_node, t) * DGX(r_parent, t_parent, t).inverse() );
    CleanAlTM( tm );
    return tm;
  }
};

static GlobalFrame gf;

struct FrameIn
{
  AlGroupNode *r_node;
  AlGroupNode *t_node;
  AlGroupNode *r_parent;
  AlGroupNode *t_parent;
  int n_frames;
  Frame *f_list;
  int type;

  void Release(void)
  {
    Free(f_list);
    memset(this, 0, sizeof(*this));
  }
};

void ReadFrames(int count, FrameIn *list);
void GetFrame(Frame *frame, AlGroupNode *r_node, AlGroupNode *t_node,
              AlGroupNode *r_parent, AlGroupNode *t_parent,
              const AlTM & tm0, const AlTM & inv_tm0, double t);
void AlTM_to_Frame(Frame *frame, const AlTM & tm);

void GetBranchName(AlGroupNode *group, char branch_name[64]);
bool IsIdentity(const AlTM & tm);
void ResetState(void);
void ExportEvents(CompoundObject *c_obj);
void GetInvisibleNodes(AlDagNode* **node_list, int *count);
void ExportBoneHP(const bone_lib *bl, CompoundObject *c_obj);
int GetBoneID(const bone_lib *bl, const AlDagNode *node);
void GetSkeletonScale( CompoundObject *c_obj, AlDagNode *adjust_root);
bool HasRotData( AlDagNode *node );
bool HasTransData( AlDagNode *node );

void DumpPivots(bone_lib *bl);
AlTM BuildLocal(AlDagNode *node, double t);
AlTM BuildLocal18(AlDagNode *node, double t);

void GetBoneTMs(bone_lib *bl, double t);

Matrix AlTM_to_Matrix(const AlTM& r_tm);
PersistTransform AlTM_to_PersistTransform(const AlTM & atm);

void DumpT9(AlDagNode *node, double t);
void DumpAlTMParts(const AlTM& tm);
bool IsPivotConsistent(AlDagNode *node, double t);

void ExportBoneConnections(CompoundObject *c_obj, bone_lib *bl);
int CompareElements(const void *pt1, const void *pt2);
void TransformVertices(const bone_lib *bl, vertices *v, normals *n);
int AssignVertices(bone_lib *bl, const vertices *v, double t);

void GetBones(AlDagNode *aldagnode, bone_lib *bl, int last_bone, double t);
bool CheckBones(const bone_lib *bl);
void AddBone(bone_lib *bl, AlGroupNode *node, int depth, AlGroupNode *parent);

AlGroupNode* FindBoneRoot(AlClusterNode *skin_ref_node);
AlDagNode* FindSkin(AlClusterNode *skin_ref_node);
AlClusterNode* GetSkinRef(AlDagNode *aldagnode);

void MakeAnimConnection(CompoundObject *c_obj, AlGroupNode *group,
                        Frame *in_frame_list, int in_n_frames, int type);
void MakeSphereConnection(CompoundObject *c_obj, AlGroupNode *r_group,
                          AlGroupNode *t_group,
                          Frame *frame_list, int n_frames, 
                          AlGroupNode *r_parent, AlGroupNode *t_parent);
void MakeRevAnim(CompoundObject *c_obj, AlGroupNode *r_group, AlGroupNode *t_group,
                 Frame *frame_list, 
                 int n_frames, AlGroupNode *r_parent, AlGroupNode *t_parent);
void MakePrisAnim(CompoundObject *c_obj, AlGroupNode *group, Frame *frame_list, 
                  int n_frames, AlGroupNode *parent);
void MakeRootAnim(CompoundObject *c_obj, AlGroupNode *group, Frame *frame_list, 
                  int n_frames);
void MakeDofConnection(CompoundObject *c_obj, AlGroupNode *group, AlGroupNode *parent);

void GetFixed(AlGroupNode *r_group, AlGroupNode *t_group, Fix *fixed, 
              double t, AlGroupNode *r_parent, AlGroupNode *t_parent);
void GetDofData(AlGroupNode *r_group, AlGroupNode *t_group,
                DofData *dof_data, AlGroupNode *r_parent, AlGroupNode *t_parent);
void DefineHP(object *obj, AlGroupNode *obj_group, AlGroupNode *hp_group);

AlTM CleanAlMatrix(const AlTM& m); // used by HP
AlTM NoScale(const AlTM& m);
void GetMatrixScale(const AlTM& m, float s[]);

AlGroupNode* GetParentGroupAlNode(AlDagNode *aldagnode);

void CmdOptions(int argc, char *argv[]);
void ExitCleanup(void);
int utfExport(void);
int utfCmdExport( int argc, char **argv );
int utfExit(void);
bool LoadNextBatchFile(void);
bool LoadBatchOutputDir(void);
bool GetLine( FILE *fp, char line[256] );

void ExportDefMesh(AlClusterNode *skin_ref_node);
void Enum(CompoundObject *c_obj, AlDagNode *aldagnode, int level);
object* GetObject(CompoundObject *c_obj, const char *file_name);
void ExportMerged(object *obj, AlDagNode *node, int level, double t,
                  AlGroupNode *r_to, AlGroupNode *t_to);
void Export3DB(CompoundObject *c_obj, AlDagNode *aldagnode, AlGroupNode *group);

void defineGeometry(object *obj, AlDagNode *aldagnode,
                    AlGroupNode *rto_group,  AlGroupNode *tto_group, double t);
char* defineTexture(txt_lib *tl, char *txtName, char *alphaName);
void defineMaterials(mtl_lib *ml, anim_txt_lib *atl, AlDagNode *aldagnode);

void FindUsedMtls(AlObject *alobject, int *used_mat, int count);
int GetShapeNo(AlObject *alobject);
int GetMtlType(char *matName);
int GetMtlGlow(const char *matName, float *r, float *g, float *b);
int GetMtlIncandescence(const char *matName, float *r, float *g, float *b);
void GetMtltxtName(char *matName, char **txtName);
void GetTextureFileName(char *txtName, char **txtFileName);
void GetTextureFullFileName(char *txtName, char **txtFileName);
AlTexture* GetMyTexture(const char *txtName);
void GetAlphaName(char *mtlName, char **alphaName);

float dtDot3(DtVec3f *v1, DtVec3f *v2);
DtVec3f dtCross3(DtVec3f *v1, DtVec3f *v2);
void dtNormalize(DtVec3f *v);
float dtMagnitude(const DtVec3f *v);

/* needed by all DSO's to be found */
char *program       = "GameExport";       /* wf_plugin specific */
char *version       = "2.0";             
char *type          = "Export";         

int   DtExportCount = 1;                  /* Dt specific */

/* entry point into DSO */
DtEntryTable DtExportTable[1] = 
{
    { "UTF", utfExport, utfCmdExport, utfExit },
};

#define MAX_SHAPES  60
#define MAX_FRAMES  500

#define GEOMETRY    1
#define HEIRARCHY   2

DtStateTable *stateTable = NULL;

void defineGeometry(object *obj, AlDagNode *aldagnode,
                   AlGroupNode *rto_group, AlGroupNode *tto_group, double t)
{
  assert( (rto_group && tto_group)  || (!rto_group && !tto_group) );
  if(aldagnode==NULL) return;

  AlPolysetNode *psetnode;
  AlPolyset *pset;
  AlRenderInfo renderInfo;

  poly p;
  p.api_smg_id = (int)aldagnode; // don't merge vertices in different groups

  if( psetnode=aldagnode->asPolysetNodePtr() )
  {
    if( pset=psetnode->polyset() )
    {
       if(pset->numberOfPolygons() < 1)
       {
         Winprint("Warning: %s has 0 polygons!\n", aldagnode->name());
         return;
       }

       if(pset->numberOfVertices() < 3)
       {
         Winprint("Warning: %s has ONLY %d vertices!\n",
           aldagnode->name(), pset->numberOfVertices());
         return;
       }

       pset->renderInfo(renderInfo);

       if(renderInfo.smooth_shading == TRUE){
         p.property = p.property & ~FLAT_SHADED;
         p.property = p.property | SMOOTH_SHADED;
         // printf("smooth\n");
       }
       else{
         p.property = p.property & ~SMOOTH_SHADED;
         p.property = p.property | FLAT_SHADED;
         // printf("flat\n");
       }

       if(renderInfo.doubleSided == TRUE){
         p.property += TWO_SIDED;
         // printf("doubleSided\n");
       }
       else{
         // printf("single sided\n");
       }
    }
  }

  // default shading
  if( !(p.property & SMOOTH_SHADED) && !(p.property & FLAT_SHADED)){ 
    p.property = p.property | SMOOTH_SHADED;
  }

  SetTime(t);

  AlTM tmg;
  if(rto_group) // check since top node won't have a parent
  {
    tmg = gf.DGX(rto_group, tto_group, t).inverse();
  }
  else
  {
    tmg = gf.GX(aldagnode, t).inverse();
  }

  int poly_id;
  int v_id;
  int a_v_id;
  AlPolysetVertex *pl_vtx;
  double tmp_v[4];
  double uu,vv;
  AlShader *alshader;
  AlTexture *altexture;

  for(poly_id=0; poly_id < pset->numberOfPolygons(); poly_id++)
  {
    AlPolygon *pgon = pset->polygon(poly_id);

    // bail out on degenerate poly's
    if(pgon->numberOfVertices() < 3) continue;

    alshader=pset->firstShader(pgon->shaderIndex());
    p.material_id=GetMtlID(&(obj->ml), alshader->name());
    altexture=alshader->firstTexture();

    double uScale=1.0, vScale=1.0, uTrans=0.0, vTrans=0.0, rot=0.0;

    if(altexture)
    {
      altexture->parameter( kFLD_SHADING_COMMON_TEXTURE_UCOVERAGE, uScale );
      altexture->parameter( kFLD_SHADING_COMMON_TEXTURE_VCOVERAGE, vScale );
      altexture->parameter( kFLD_SHADING_COMMON_TEXTURE_UTRANSLATE, uTrans );
      altexture->parameter( kFLD_SHADING_COMMON_TEXTURE_VTRANSLATE, vTrans );
      altexture->parameter( kFLD_SHADING_COMMON_TEXTURE_ROTATE, rot );
      rot *= D2R;
    }

    p.vertex_count = pgon->numberOfVertices();

    for(v_id=0; v_id < pgon->numberOfVertices(); v_id++)
    {
      // adjust vertex order to get correct normals
      a_v_id=pgon->numberOfVertices()-(1+v_id);

      // get vertex
      pl_vtx=pgon->vertex(a_v_id);
      pl_vtx->worldPosition(tmp_v[0], tmp_v[1], tmp_v[2]); tmp_v[3]=1.0;

      // transform into local coords
      tmg.transPoint(tmp_v);
      p.vertices[3*v_id]=(float)tmp_v[0];
      p.vertices[3*v_id+1]=(float)tmp_v[1];
      p.vertices[3*v_id+2]=(float)tmp_v[2];

      // get UV
      pgon->st(a_v_id, uu, vv);

      double x, y;

      x = uu - uTrans;
      y = vv - vTrans;
      uu = x; vv = y;

      x = uu / uScale;
      y = vv / vScale;
      uu = x; vv = y;

      // should this rotation be 1st, 2nd or 3rd ??
      // trans is before scale!
      x =  cos(rot)*uu - sin(rot)*vv;
      y =  sin(rot)*uu + cos(rot)*vv;
      uu = x; vv = y;

      p.uv[2*v_id] = uu;
      p.uv[2*v_id+1] = vv;
    }
    InsertPolyTriangles(obj, &p);
  }
}

inline void clamp_up(float *value)
{
  if( *value >= (254.0f / 255.0f) )
  {
    *value = 1.0f;
  }
}

void defineMaterials(mtl_lib *ml, anim_txt_lib *atl, AlDagNode *aldagnode)
{
    int   count;
    int   matNum;
    float ared, agreen, ablue; 
    float dred, dgreen, dblue;
    float sred, sgreen, sblue; 
    float ered, egreen, eblue;
    float shininess; 
    float transparency;
    char *txtName;
    char *mtlName;
    char *txtFileName;
    char *alphaName;
    int *used_mat;

    mtl *m=NULL;

    /* get # of materials */
    DtMtlGetSceneCount( &count );
    used_mat=(int*)Malloc(count*sizeof(int));
    memset(used_mat, 0, count*sizeof(int));
    FindUsedMtls(aldagnode, used_mat, count);

    /* query materials by number */
    for ( matNum = 0; matNum < count; matNum++ ) 
    {
      if(used_mat[matNum]==1)
      {
        m=(mtl*)Malloc(sizeof(mtl));
        InitMaterial(m, atl);
        DtMtlGetNameByID( matNum, &mtlName );
        //printf("material_id=%d %s\n",matNum, mtlName);
        m->name=(char*)Malloc((strlen(mtlName)+1)*sizeof(char));
        strcpy(m->name, mtlName);

        /* get txt name associated w/ material */
        DtTextureGetName( m->name, &txtName );

        if(txt_flag)
        {
          if(txtName)
          {
            GetAlphaName(m->name, &alphaName);
            txtFileName = defineTexture(atl->tl, txtName, alphaName);
            if(txtFileName == NULL)
            {
              char *tmp_name;
              GetMtltxtName( m->name, &tmp_name );
              if(tmp_name)
              {
                Winprint("Error: texture %s for material %s not found!\n",
                  tmp_name, m->name);
                Free(tmp_name);
              }
            }
            Free(alphaName);
          }
          else
          {
            txtFileName=NULL;
          }
        }
        else
        {
          GetMtltxtName( m->name, &txtFileName );
        }

        if ( txtFileName )
        {
          m->diffuse.texture_id = GetTxtID(atl->tl, txtFileName);
          m->diffuse.texture_name=(char*)Malloc((strlen(txtFileName)+1));
          strcpy(m->diffuse.texture_name, txtFileName);
          Free(txtFileName);
        }
        else
        {
          m->diffuse.texture_id=-1;
          m->diffuse.texture_name=NULL;
        }

        // get mtl properties
         DtMtlGetAllClrbyID( matNum,0,
         &ared, &agreen, &ablue,
         &dred, &dgreen, &dblue,
         &sred, &sgreen, &sblue,
         &ered, &egreen, &eblue,
         &shininess, &transparency );

         // this is a hack because the Dt layer munges 255 to 254
         clamp_up( &ared );
         clamp_up( &agreen );
         clamp_up( &ablue );

         clamp_up( &dred );
         clamp_up( &dgreen );
         clamp_up( &dblue );

         clamp_up( &sred );
         clamp_up( &sgreen );
         clamp_up( &sblue );

         clamp_up( &ered );
         clamp_up( &egreen );
         clamp_up( &eblue );

         m->ambient.value[RED]=(ared);
         m->ambient.value[GREEN]=(agreen);
         m->ambient.value[BLUE]=(ablue);

         m->diffuse.value[RED]=(dred);
         m->diffuse.value[GREEN]=(dgreen);
         m->diffuse.value[BLUE]=(dblue);

         m->specular.value[RED]=(sred);
         m->specular.value[GREEN]=(sgreen);
         m->specular.value[BLUE]=(sblue);

         /*
         if(GetMtlType(mtlName) == PHONG)
         {
           // PHONG
           m->emission.value[RED]=(ered)-(ared);
           m->emission.value[GREEN]=(egreen)-(agreen);
           m->emission.value[BLUE]=(eblue)-(ablue);
         }
         else
         {
           // LAMBERT
           m->emission.value[RED]=(ered);
           m->emission.value[GREEN]=(egreen);
           m->emission.value[BLUE]=(eblue);
         }
         */

         if( -1 == GetMtlIncandescence(mtlName, 
                              &(m->emission.value[RED]),
                              &(m->emission.value[GREEN]),
                              &(m->emission.value[BLUE])) )
         {
           Winprint("Error: getting Incandescence for mtl %s\n!", mtlName);
         }

         // incandescence takes presedence but if it's 0 we try glow
         if(m->emission.value[RED] == 0.0f &&
            m->emission.value[GREEN] == 0.0f &&
            m->emission.value[BLUE] == 0.0f)
         {
           if( -1 == GetMtlGlow(mtlName, 
                                &(m->emission.value[RED]),
                                &(m->emission.value[GREEN]),
                                &(m->emission.value[BLUE])) )
           {
             Winprint("Error: getting glow for mtl %s\n!", mtlName);
           }
         }

         m->shininess.value[0]=(shininess)/160.0;
         m->transparency.value[0]=1.0-(transparency);  // 1=opaque in .3db

         m->api_id = matNum;
         InsertMaterial(ml, m);
      }
    }
    Free(used_mat);

    if(verbose_level >= 1)
      printf("done w/ defineMaterials\n");
}

char* defineTexture(txt_lib *tl, char *txtName, char *alphaName)
{
  txt *t=NULL;
  int components;
  unsigned char *image;
  int width;
  int height;
  int x,y;
  unsigned char *alpha=NULL;
  char *txtFileName=NULL;
  char *alpha_file_name=NULL;

  if(!txtName)
    return NULL; 

  txtFileName=NULL;

  t=(txt*)Malloc(sizeof(txt));
  InitTexture(t);

  AlTexture *alt = GetMyTexture(txtName);
  if(alt)
  {
    double wrap_u, wrap_v;
    alt->parameter( kFLD_SHADING_COMMON_TEXTURE_UWRAP, wrap_u );
    alt->parameter( kFLD_SHADING_COMMON_TEXTURE_VWRAP, wrap_v );
    if(wrap_u == 1.0)
    {
      t->u_mode = TILE;
    }
    else
    {
      t->u_mode = CLAMP;
    }
    if(wrap_v == 1.0)
    {
      t->v_mode = TILE;
    }
    else
    {
      t->v_mode = CLAMP;
    }
  }

  GetTextureFileName(txtName, &txtFileName);

  if(verbose_level >= 1)
    printf("filename=%s\n", txtFileName);

  t->name=(char*)Malloc((strlen(txtFileName)+1)*sizeof(char));
  strcpy(t->name, txtFileName);
  t->mip_count=1;

  t->mip_map=(mip**)Malloc(sizeof(mip*));
  t->mip_map[0]=(mip*)Malloc(sizeof(mip));
  InitMip(t->mip_map[0]);
  t->mip_map[0]->level=(char*)Malloc((strlen("MIP level 0")+1)*sizeof(char));
  strcpy(t->mip_map[0]->level,"MIP level 0");
  DtTextureGetImageSize(txtName, 
                        &(t->mip_map[0]->x_size), &(t->mip_map[0]->y_size),
                        &components);
  DtTextureGetImage( txtName, &image);

  width=t->mip_map[0]->x_size;
  height=t->mip_map[0]->y_size;
  t->mip_map[0]->color_count=N_PALETTE_COLORS;
  t->mip_map[0]->depth = (color_depth)txt_depth;

      switch(components)
      {
      case 1: // with color indices and look up tables (lut)
        Winprint("Warning unsupported texture mode 1\n");
        break;
      case 2: // does not exist per man page
        Winprint("Warning non-documented texture mode 2\n");
        break;
      case 3: // with RGB values
        Winprint("Warning unsupported texture mode 3\n");
        break;
      case 4: // with RGBA values
      {
        VRGB *rgb; // Source data image
        rgb=(VRGB*)Malloc((4*width*height*sizeof(VRGB))/3); // 1/3 for mip data
        alpha=(unsigned char*)Malloc((4*width*height*sizeof(unsigned char))/3);

        for(y=0; y<height; y++){
          for(x=0; x<width; x++){
            rgb[y*width+x].r=image[4*(y*width+x)];
            rgb[y*width+x].g=image[4*(y*width+x)+1];
            rgb[y*width+x].b=image[4*(y*width+x)+2];
            alpha[y*width+x]=image[4*(y*width+x)+3];
          }
        }

        LoadTxtRGB(rgb, t);
        Free(rgb);

        if(!alphaName)
        {
          LoadTxtAlpha(alpha, t);
          Free(alpha);
        }
        else
        // read alpha from a separate texture file
        //if(alphaName && (t->mip_map[0]->alpha_8_bit==NULL))
        {
          Free(alpha);

          // DtTextureGetImageSize(alphaName, &(width), &(height), &components);
          Image img;
          InitImage(&img);
          GetTextureFullFileName(alphaName, &alpha_file_name);
          ReadPix(alpha_file_name, &img);
          Free(alpha_file_name);
          width=img.width;
          height=img.height;
          if((width==t->mip_map[0]->x_size) &&
             (height==t->mip_map[0]->y_size)){

            alpha=(unsigned char*)Malloc((4*width*height*sizeof(unsigned char))/3);
            // DtTextureGetImage( alphaName, &image);
            for(y=0; y<height; y++){
              for(x=0; x<width; x++){
                alpha[y*width+x]=(unsigned char)((
                                   img.red[y*width+x]+
                                   img.green[y*width+x]+
                                   img.blue[y*width+x]+
                                   2.99)/3.0);
              }
            }
            FreeImage(&img);

            LoadTxtAlpha(alpha, t);
            Free(alpha);

            // combine texture & alpha names
            GetTextureFileName(alphaName, &alpha_file_name);
            txtFileName=(char*)Realloc(txtFileName, 
              (strlen(txtFileName)+strlen(alpha_file_name)+2)*sizeof(char));
            strcat(txtFileName,"_");
            strcat(txtFileName,alpha_file_name);

            t->name=(char*)Realloc(t->name,
              (strlen(t->name)+strlen(alpha_file_name)+2)*sizeof(char));
            strcpy(t->name, txtFileName);

            Free(alpha_file_name);
          }
          else{
            fprintf(stderr,
            "Warning: alpha texture %s has different size than color texture %s.\n"
             ,alphaName, txtName);
            fprintf(stderr,"color x=%d y=%d   alpha x=%d y=%d\n",
                    t->mip_map[0]->x_size, t->mip_map[0]->y_size,
                    width, height);
          }
        }

        t->identifier=InsertTexture(tl, t);
        break;
      }
      default:
        fprintf(stderr,"Invalid texture mode %d\n",components);
      } 

     if(verbose_level >= 1)
       printf("done w/ defineTextures\n");
 
  return txtFileName;
}

int utfExit(void)
{
        return running;
}

// main (if GUI not selected)
int utfCmdExport( int argc, char **argv )
{
    CmdOptions(argc, argv);

    utfExport();

    if(batch_fp)
    {
      fclose(batch_fp);
      batch_fp = NULL;
    }

    return 0;
}

void CmdOptions(int argc, char *argv[])
{
  lod_percent = 100.0f;
  fps = 30.0f;
  txt_depth = PAL8;
  dither_flag = 0;
  batch_fp = NULL;

  for(int i=0; i<argc; i++)
  {
    if( !strcmp(argv[i],"-lod") )
    {
      lod_percent = atof(argv[i+1]);

      if(i+3 < argc) // def meshes don't get switching distances
      {
        lod_closest = atof(argv[i+2]);
        lod_furthest = atof(argv[i+3]);
      }

      if(i+4 < argc)
      {
        char *end_ptr;
        strtod(argv[i+4], &end_ptr);
        if(end_ptr != argv[i+4])
        {
          lod_uv_weight = atof(argv[i+4]);
        }
      }
    } 
    else
    if( !strcmp(argv[i],"-fps") )
    {
      fps = atof(argv[i+1]);
    }
    else
    if( !strcmp(argv[i],"-565") )
    {
       txt_depth = TRUE565;
    }
    else
    if( !strcmp(argv[i],"-888") )
    {
       txt_depth = TRUE888;
    }else
    if( !strcmp(argv[i],"-dither") )
    {
       dither_flag = 1;
    }else
    if( !strcmp(argv[i],"-batch") )
    {
       batch_fp = fopen(argv[i+1], "r");
    }
#if 0
    else
    if( !strcmp(argv[i],"-f") )
    {
      lod_mtl_weight = atoi(argv[i+1]);
    }
#endif
  }
}

char FileName[256] = {0};
char DirName[256] = {0};

bool GetLine( FILE *fp, char line[256] )
{
  int c = fgetc( fp );
  bool result = (c != EOF);

  for(int i = 0; i < 255 && c != EOF && c != '\n'; i++)
  {
    line[i] = c;
    c = fgetc( fp );
  }
  line[i] = 0;

  return result;
}

bool LoadBatchOutputDir(void)
{
  assert( batch_fp );

  while( GetLine( batch_fp, DirName) )
  {
    //strtok(DirName, " \t"); // spaces in path are possible
    if( strlen(DirName) && DirName[0] != ';' ) // && !strtok(DirName, " \t") )
    {
      if( DirName[ strlen(DirName) - 1 ] == '/' )
      {
        DirName[ strlen(DirName) - 1 ] = 0;
      }
      return true;
    }
  }

  return false;
}

bool LoadNextBatchFile(void)
{
  assert( batch_fp );

  //AlRetrieveOptions options;
  //AlUniverse::retrieveOptions( options ); // default for new_stage is false
  //options.new_stage = TRUE; 
  //AlUniverse::setRetrieveOptions( options );

  char vbuf[8];
  char line[256];

  while( GetLine( batch_fp, line ) )
  {
    if( strlen(line) && line[0] != ';' )
    {
      const char *in_name = line;
            char *out_name = strtok(line, " \t\n"); 
            out_name = strtok(NULL, " \t\n"); 
      if( strlen(in_name) && out_name && strlen(out_name) && 
          AlUniverse::isWireFile( in_name, vbuf ) )
      {
        //AlUniverse::deleteAll();
        AlUniverse::deleteStage( AlUniverse::currentStage() );
  
        if( sSuccess == AlUniverse::retrieve( in_name ) )
        {
          StripExtension(out_name);
          StripPath(out_name);
          strcpy(FileName, out_name);

          AlUniverse::doUpdates( TRUE );
          AlUniverse::setStageWireFileName( AlUniverse::currentStage(), out_name );
          AlUniverse::redrawScreen( kRedrawAll );

          DtOM_CleanUp();
          DtInitTransfer(printf,True);
          //Mg_RunTranslator();

          AlUniverse::doUpdates( FALSE );

          printf("Loaded %s\n", line);
          return true; 
        }
      }
    }
  }

  //fclose( batch_fp );
  //batch_fp = NULL;
  return false;
}


/* main */
/* entry function */
int utfExport(void)
{
    int                  argc = 1;
    char                 *argv[3] = {0,0,0};
    char                 str[] = "UTF";
    char                 str2[] = "";
    argv[0] =            str;
    argv[1] =            str2;
    int                  doexport = FALSE;

    if ( running )  // don't run more than one copy at a time
    {
        return -1;
    }
    running = 1;

    if(!(std_out=freopen("std_out.txt", "w", stdout))){
      Winprint("Error redirecting stdout.\n");
      exit(1);
    }

    if(!(std_err=freopen("std_err.txt", "w", stderr))){
      Winprint("Error redirecting stderr.\n");
      exit(1);
    }

    atexit(ExitCleanup);


    VkApp       *app;
    // Create an application object
    if (! getenv( "ALIAS_DT_SAFE" ) ) 
    {
        app = new VkApp("Application", &argc, argv); 
    }
    else 
    {
        app = new VkApp( AlXevents::getAppShell() ); 
    }
  

    UTFConverter *_dialog = new UTFConverter( "Vk" );
    if ( _dialog->postAndWait() == VkDialogManager::OK )
    {
        doexport = TRUE;
    }
    else
    {
        doexport = FALSE;
    }

    app->handlePendingEvents();


    if ( doexport == TRUE )
    {

      bool got_file;
      if( batch_fp )
      {
        got_file = LoadBatchOutputDir() && LoadNextBatchFile();
      }
      else
      {
        got_file = true;
      }

      while( got_file )
      {
        if( !batch_fp )
          init_time = AlUniverse::currentTime();
        else
          init_time = START_TIME;

        // default name is wirefile name
        if( !batch_fp )
        {
          char *tmp_name = NULL;
          DtGetDirectory( &tmp_name ); /* export dir */
          assert(tmp_name);
          strcpy(DirName, tmp_name);
          tmp_name = NULL;
          DtSceneGetName( &tmp_name ); /* name of output file */
          if( !batch_fp && tmp_name && (strlen(tmp_name) > 0) ) 
          {
            strcpy(FileName, tmp_name);
          }
          else
          {
            strcpy(FileName, 
                   AlUniverse::stageWireFileName(AlUniverse::currentStage()));
            StripPath(FileName);
            StripExtension(FileName);
            DtSetFilename(FileName);  // doesn't seem to work
          }
        }

        mip_flag=!utf_no_mipmaps;
        txt_flag=utf_output_textures;
        convex_hull_flag = utf_convex_hull;
        remove_constant_channels = utf_remove_constant_channels;

        // check for def mesh
        AlClusterNode *skin_ref_node;
        if(skin_ref_node=GetSkinRef(AlUniverse::firstDagNode()))
        {
          exporting_deformable = 1;
          absolute_rev = 1;
          ExportDefMesh(skin_ref_node);
          exporting_deformable = 0;
          absolute_rev = 0;
        }
        else
        {
          CompoundObject c_obj;
          InitCompoundObject(&c_obj);

          sprintf(c_obj.file_name,"%s/%s.cmp",DirName,FileName); 

          Enum(&c_obj, AlUniverse::firstDagNode(), 0);
        
          if(lod_percent < 100.0f)
          {
            for(int i=0; i < c_obj.part_count; i++)
            {
              object *obj = &(c_obj.lod_object_list[i].obj_list[0]);
            
              calcEdges(obj);
              CollapseEdges(obj, (int)((.01f * lod_percent) * obj->face_count + .5f));
              obj->lol.closest = lod_closest;
              obj->lol.furthest = lod_furthest;
            }
          }

          CombineChannels(&c_obj, 0);
          ExtractKeyFrames(&c_obj);

          WriteCmpd(&c_obj, split_flag, txt_flag); // only if count > 0
          FreeCmpObject(&c_obj);
        }
        Free(root_name);

        // hack to clear any weirdness caused by moving nodes around
        ResetState();

        app->handlePendingEvents();

        if( batch_fp )
          got_file = LoadNextBatchFile();
        else
          got_file = false;

        app->handlePendingEvents();

        if(verbose_level >= 1)
          printf("Exiting utfExport()\n");
      }
    }

    delete _dialog;
    _dialog = NULL;
    delete app;
    app = NULL;

    fflush(stdout);
    fflush(stderr);

    if(ftell(std_err) > 0)
    {
      Winprint("Please check std_err.txt for error information!");
    }

    fclose(std_out);
    fclose(std_err);

    running = 0;
    return 0;
}

void GetAlphaName(char *mtlName, char **alphaName)
{
  AlShader  *material;
  AlTexture *texture;
  const char *name;

  *alphaName=NULL;
  if ( material = AlUniverse::firstShader() ) {
    do {
      if( !strcmp(material->name(), mtlName) ){
        if ( texture = material->firstTexture()) {
          do{
            char * field = ( char * ) texture->fieldType( );
            if(!strcmp( field, "transparency" )){
              // name=texture->filename();
              name=texture->name();
              if(verbose_level>=3){
                printf("Alpha txt n=%s f=%s\n",texture->name(), texture->filename());
              }
              *alphaName=(char*)Malloc((strlen(name)+1)*sizeof(char));
              strcpy(*alphaName, name);
              return;
            }
          } while ( material->nextTextureD( texture ) == sSuccess );
        }
      }
    } while ( AlUniverse::nextShaderD( material ) == sSuccess );
  }

}

AlTexture* GetMyTexture(const char *txtName)
{
  AlShader  *material;
  AlTexture *texture;

  if(txtName==NULL)
    return NULL;

  if ( material = AlUniverse::firstShader() ) {
    do {
      if ( texture = material->firstTexture()) {
        do{
          if( !strcmp(texture->name(), txtName)){
            return texture;
          }
        } while ( material->nextTextureD( texture ) == sSuccess );
      }
    } while ( AlUniverse::nextShaderD( material ) == sSuccess );
  }
  return NULL;
}

void GetTextureFullFileName(char *txtName, char **txtFileName)
{
  AlShader  *material;
  AlTexture *texture;
  const char *name;

  *txtFileName=NULL;

  if(txtName==NULL)
    return;

  if ( material = AlUniverse::firstShader() ) {
    do {
      if ( texture = material->firstTexture()) {
        do{
          if( !strcmp(texture->name(), txtName)){
            name=texture->filename();
            if(verbose_level>=3){
              printf("n=%s f=%s\n",texture->name(), texture->filename());
            }
            *txtFileName=(char*)Malloc((strlen(name)+1)*sizeof(char));
            strcpy(*txtFileName, name);
            return;
          }
        } while ( material->nextTextureD( texture ) == sSuccess );
      }
    } while ( AlUniverse::nextShaderD( material ) == sSuccess );
  }
}

void GetTextureFileName(char *txtName, char **txtFileName)
{
  GetTextureFullFileName(txtName, txtFileName);
  StripPath(*txtFileName);
  StripExtension(*txtFileName);   
}

int GetMtlType(char *matName)
{
  AlShader  *material;

  if ( material = AlUniverse::firstShader() ) {
    do {
      if (! strcmp( material->name(), matName ) ) {
        if (! strcmp( material->shadingModel(), "LIGHTSOURCE" ) ) {
          return LIGHTSOURCE;
        }else 
        if (! strcmp( material->shadingModel(), "LAMBERT"  ) ) {
          return LAMBERT;
        }else 
        if (! strcmp( material->shadingModel(), "PHONG" ) ) {
          return PHONG;
        }else 
        if (! strcmp( material->shadingModel(), "BLINN" ) ) {
          return BLINN;   
        }
        return -1;
      }
    } while ( AlUniverse::nextShaderD( material ) == sSuccess );
  }
  fprintf(stderr,"Error: couldn't find type of material %s\n",matName);
  return -1;
}

void GetMtltxtName(char *matName, char **txtName)
{
  *txtName = NULL;

  if(matName)
  {
    AlShader  *material;

    if ( material = AlUniverse::firstShader() ) 
    {
      do 
      {
        if( !strcmp( material->name(), matName ) )
        {
          AlTexture *txt = material->firstTexture();
          while(txt)
          {
            char * field = ( char * ) txt->fieldType( );
            if(!strcmp( field, "color" ))
            {
              if(txt->filename())
              {
                char *full_name = strdup(txt->filename());
                StripPath(full_name);
                StripExtension(full_name);
                *txtName = strdup( full_name );
                Free(full_name);
              }
              return;
            }
            txt = material->nextTexture( txt );
          }
        }
      } while ( AlUniverse::nextShaderD( material ) == sSuccess );
    }
  }
}

int GetMtlGlow(const char *matName, float *r, float *g, float *b)
{
  *r = *g = *b = 0.0f;

  AlShader  *material;

  if ( material = AlUniverse::firstShader() ) 
  {
    do 
    {
      if (! strcmp( material->name(), matName ) ) 
      {
        double glow;
        double red, green, blue;
        material->parameter(kFLD_SHADING_COMMON_GLOW_INTENSITY, &glow);
        material->parameter(kFLD_SHADING_COMMON_COLOR_R, &red);
        material->parameter(kFLD_SHADING_COMMON_COLOR_G, &green);
        material->parameter(kFLD_SHADING_COMMON_COLOR_B, &blue);
        *r = (glow * red) / 255.0;
        *g = (glow * green) / 255.0;
        *b = (glow * blue) / 255.0;
        return 0;
      }
    } while ( AlUniverse::nextShaderD( material ) == sSuccess );
  }

  return -1;
}

int GetMtlIncandescence(const char *matName, float *r, float *g, float *b)
{
  *r = *g = *b = 0.0f;

  AlShader  *material;

  if ( material = AlUniverse::firstShader() ) 
  {
    do 
    {
      if (! strcmp( material->name(), matName ) ) 
      {
        double red, green, blue;
        material->parameter(kFLD_SHADING_COMMON_INCANDESCENCE_R, &red);
        material->parameter(kFLD_SHADING_COMMON_INCANDESCENCE_G, &green);
        material->parameter(kFLD_SHADING_COMMON_INCANDESCENCE_B, &blue);
        *r = red / 255.0;
        *g = green / 255.0;
        *b = blue / 255.0;
        return 0;
      }
    } while ( AlUniverse::nextShaderD( material ) == sSuccess );
  }

  return -1;
}

float dtDot3(DtVec3f *v1, DtVec3f *v2)
{
  return(v1->vec[0]*v2->vec[0]
        +v1->vec[1]*v2->vec[1]
        +v1->vec[2]*v2->vec[2]);
}

DtVec3f dtCross3(DtVec3f *v1, DtVec3f *v2)
{
  DtVec3f result;

  result.vec[0]=v1->vec[1] * v2->vec[2] - v1->vec[2] * v2->vec[1];
  result.vec[1]=v1->vec[2] * v2->vec[0] - v1->vec[0] * v2->vec[2];
  result.vec[2]=v1->vec[0] * v2->vec[1] - v1->vec[1] * v2->vec[0];

  return result;
}

void dtNormalize(DtVec3f *v)
{
  double inv_magnitude = dtMagnitude(v);

  if(inv_magnitude > 0.0)
  {
    inv_magnitude = 1.0 / dtMagnitude(v);
    v->vec[0]*=inv_magnitude;
    v->vec[1]*=inv_magnitude;
    v->vec[2]*=inv_magnitude;
  }
}

float dtMagnitude(const DtVec3f *v)
{
  return(sqrt(v->vec[0]*v->vec[0]
            + v->vec[1]*v->vec[1]
            + v->vec[2]*v->vec[2]));
}

void Enum(CompoundObject *c_obj, AlDagNode *aldagnode, int level)
{
  if(aldagnode == NULL)
    return;

  AlGroupNode *groupNode = aldagnode->asGroupNodePtr();

  // process node
  int hp_flag = 0;
  if(aldagnode->asPolysetNodePtr()) // leaf node
  {
     assert(groupNode == NULL);
     Export3DB(c_obj, aldagnode, GetParentGroupAlNode(aldagnode));
  }
  else
  if( strstr(aldagnode->name(), "hp") ) // hard point
  {
    hp_flag = 1;
    AlGroupNode *parent_group;
    char file_name[256];
    object *obj;

    parent_group=GetParentGroupAlNode(groupNode);
    if(!parent_group){
      fprintf(stderr,"Error: HP %s has no parent.\n",groupNode->name());
      exit(1);
    }
    sprintf(file_name,"%s.3db",parent_group->name());

    obj = GetObject(c_obj, file_name);  
    assert(obj);
    DefineHP(obj, parent_group /* obj node */, groupNode /* hp node */); 
  }

  // recurse across
  // poly nodes must be first
  // TODO: fix this so the order does not matter
  Enum(c_obj, aldagnode->nextNode(), level);

  // recurse down
  if(!hp_flag && (groupNode != NULL))
  {
    Enum(c_obj, groupNode->childNode(), level+1);
  }
}
       
void Export3DB(CompoundObject *c_obj, AlDagNode *aldagnode, AlGroupNode *group)
{
  char name[256];
  char group_name[256];
  char file_name[256];
  lod_object *l_obj=NULL;
  object *obj=NULL;

  // this is a hack for old non compound models
  if(!group) // top level poly node w/o a group
  {
    fprintf(stderr,"Warning: standalone object %s w/o a group node\n",
            aldagnode->name());
    strcpy(file_name, c_obj->file_name);
    strtok(file_name, ".");
    strcat(file_name, ".3db");

    strcpy(name, c_obj->file_name);
    strtok(name, ".");

    obj=(object*)Malloc(sizeof(object));
    InitObject(obj, &(c_obj->tl), &(c_obj->atl));
    obj->type=FIXED_MESH;
    ExportMerged(obj, aldagnode, 0, START_TIME, NULL, NULL); // level, time

    PostProcessMesh(obj, DENSITY, 100.0, 0, 0, true, name, 1.0f); // lod done later

    WriteObject(obj, file_name, txt_flag);
    FreeObject(obj);
    Free(obj);
    return;
  }
  else
  {
    strcpy(name, aldagnode->name());
    strcpy(group_name, group->name());
    sprintf(file_name,"%s.3db",group_name);

    if(c_obj->part_count==0)
    {
      root_name=(char*)Malloc((strlen(group_name)+1)*sizeof(char));
      strcpy(root_name, group_name);
    }

    // see if we already exported any objects in this group
    for(int i=0; i<c_obj->part_count; i++)
    {
      if(!strcmp(c_obj->lod_object_list[i].file_name, file_name))
      {
        l_obj=&(c_obj->lod_object_list[i]);
        obj=&(l_obj->obj_list[0]);
        break;
      }
    }
 
    // first object of this group, so connect the group 
    if(obj==NULL)
    {
      // also increments c_obj->part_count
      InsertCompoundName(c_obj, group_name, group_name, root_name, 
                         ".3db", 1, NULL, true); 
    
#if 0 // debug only; force fixed connection
      if(strcmp(group->name(), root_name))
      {
        Fix fixed;
        GetFixed(group, &fixed, START_TIME, NULL);
        InsertFixed(c_obj, fixed);
      }
#else
      // check if we have animation
      if( HasRotData( group ) || HasTransData( group ) ) //group->firstChannel() )
      {
        FrameIn fin;
        fin.r_node =
        fin.t_node = group;
        fin.r_parent =
        fin.t_parent = GetParentGroupAlNode(group);
        ReadFrames(1, &fin);

        MakeAnimConnection(c_obj, group, fin.f_list, fin.n_frames, fin.type);
        fin.Release();
      }
      else
      {
        MakeDofConnection(c_obj, group, NULL);
      }
#endif

      l_obj=&(c_obj->lod_object_list[c_obj->part_count-1]);
      strcpy(l_obj->file_name, file_name);
      l_obj->count=1;
      l_obj->obj_list=(object*)Malloc(l_obj->count*sizeof(object));
      obj=&(l_obj->obj_list[0]);
      InitObject(obj, &(c_obj->tl), &(c_obj->atl));
      obj->type=FIXED_MESH;
    }

    // fix to only export USED materials
    defineMaterials(&(obj->ml), obj->atl, aldagnode); // adds textures as needed
    defineGeometry(obj, aldagnode, group, group, START_TIME);
 
    PostProcessMesh(obj, DENSITY, 100.0, 0, 0, true, group_name, 1.0f);
  }
}

int GetShapeNo(AlObject *alobject)
{
  int i;
  char *shape_name;
  int numShapes;

  numShapes = DtShapeGetCount();

  for(i=0; i<numShapes; i++){
    DtShapeGetName( i, &shape_name );
    if(!strcmp(alobject->name(), shape_name)){
      return i;
    }
  }
  return -1;
}

AlGroupNode* GetParentGroupAlNode(AlDagNode *aldagnode)
{
  if(aldagnode==NULL) 
    return NULL;

  aldagnode=aldagnode->parentNode();
  while(aldagnode)
  { 
    if( aldagnode->asGroupNodePtr() && 
        strncmp(aldagnode->name(), "adjust", 6) &&
        strncmp(aldagnode->name(), "_t_", 3) ) 
    {
      assert(!strstr(aldagnode->name(), "hp"));
      return( aldagnode->asGroupNodePtr() );
    }
    aldagnode = aldagnode->parentNode();
  }

  return NULL;
}
    
void FindUsedMtls(AlObject *alobject, int *used_mat, int count)
{
  int mtl_id;
  int shapeNo;
  int numGroups;
  int groupNo;

  shapeNo=GetShapeNo(alobject);
  numGroups = DtGroupGetCount( shapeNo ); /* one group per material */

  for ( groupNo=0; groupNo < numGroups; groupNo++ ) 
  {
     DtMtlGetID( shapeNo, groupNo, &mtl_id);
     if(mtl_id >= count) // sanity check
     {
       Winprint("Error: material out of range.\n");
       exit(1);
     }
     used_mat[mtl_id]=1;
  }
}

void ReadFrames(int count, FrameIn *list)
{
  double start, end, step, t;
  AlUniverse::frameRange(kMinMax, start, end , step);
  start = START_TIME;
  // printf("start end step %f %f %f\n",start, end, step);

  int n_frames = 1 + (int)((end-start) / step);

  for(int p = 0; p < count; p++)
  {
    list[p].n_frames = n_frames;
    list[p].f_list = (Frame*)Malloc(n_frames*sizeof(Frame));
  }

  AlTM *tm0_list = (AlTM*)Malloc(count * sizeof(AlTM));
  AlTM *inv_tm0_list = (AlTM*)Malloc(count * sizeof(AlTM));
  SetTime(start);
  for(p = 0; p < count; p++)
  {
    if(list[p].r_parent)
    {
      tm0_list[p] = gf.DLX(list[p].r_node, list[p].t_node, 
                           list[p].r_parent, list[p].t_parent, start);
      inv_tm0_list[p] = tm0_list[p].inverse();
    }
    else
    {
      tm0_list[p] = gf.DGX(list[p].r_node, list[p].t_node, start); // root
      inv_tm0_list[p] = tm0_list[p].inverse();
    }
  }

  int i;
  for(i=0, t=start; i < n_frames; i++, t+=step)
  {
    SetTime(t);
   
    for(int p = 0; p < count; p++)
    {
      GetFrame(&(list[p].f_list[i]), list[p].r_node, list[p].t_node,
               list[p].r_parent, list[p].t_parent, tm0_list[p],
               inv_tm0_list[p], t);
    }
  }

  for(p = 0; p < count; p++)
  {
    list[p].type = CleanFrames(list[p].f_list, list[p].n_frames);
  }

  Free(tm0_list);
  Free(inv_tm0_list);
}

// NULL parent means global frame (used for root)
void GetFrame(Frame *frame, AlGroupNode *r_node, AlGroupNode *t_node,
              AlGroupNode *r_parent, AlGroupNode *t_parent,
              const AlTM & tm0, const AlTM & inv_tm0, double t)
{
  assert( (!r_parent && !t_parent) || (r_parent && t_parent) );
  AlTM tm;
  if( r_parent == NULL )
  {
    tm = gf.DGX(r_node, t_node, t); // root

    if( absolute_rev )
    {
      // character root always has a relative translation and absolute orientation
      tm[3][0] -= tm0[3][0];
      tm[3][1] -= tm0[3][1];
      tm[3][2] -= tm0[3][2];
    }
    else
    {
      tm *= inv_tm0;
    }
  }
  else
  {
    tm = gf.DLX(r_node, t_node, r_parent, t_parent, t); // regular node

    if( absolute_rev )
    {
      inv_tm0.transPoint(tm[3][0], tm[3][1], tm[3][2]); // character
    }
    else
    {
      tm *= inv_tm0;
    }
  }

  AlTM_to_Frame(frame, tm);
}

void AlTM_to_Frame(Frame *frame, const AlTM & tm)
{
  InitFrame( frame );

  // extract ROTATION
  Matrix m ( AlTM_to_Matrix(tm) );
  QT quat;
  mat_to_qt(&quat, &m);

  float inv_mag = sqrt(quat.d[1]*quat.d[1] +
                       quat.d[2]*quat.d[2] +
                       quat.d[3]*quat.d[3]);

  if(inv_mag > 0.00001f)
  {
    inv_mag = 1.0 / inv_mag;
  }
  else
  {
    inv_mag = 0.0f;
    quat.d[0] = 1.0f;
    quat.d[1] =
    quat.d[2] =
    quat.d[3] = 0.0f;
  }

  frame->vector_r.x = quat.d[1] * inv_mag;
  frame->vector_r.y = quat.d[2] * inv_mag;
  frame->vector_r.z = quat.d[3] * inv_mag;
  frame->angle = 2.0 * Acos(quat.d[0]);

  // extract TRANSLATION
  frame->step = sqrt(tm[3][0]*tm[3][0] + tm[3][1]*tm[3][1] + tm[3][2]*tm[3][2]);

  if(frame->step > 0.00001f)
  {
    inv_mag = 1.0 / frame->step;
  }
  else
  {
    frame->step = 0.0f;
    inv_mag = 0.0f;
  }

  frame->vector_t.x = tm[3][0] * inv_mag;
  frame->vector_t.y = tm[3][1] * inv_mag;
  frame->vector_t.z = tm[3][2] * inv_mag;
}

void MakeAnimConnection(CompoundObject *c_obj, AlGroupNode *group,
     Frame *frame_list, int n_frames, int type)
{
    assert(frame_list);
    assert(n_frames > 0);

    // root animation
    if(!strcmp(root_name, group->name()))
    {
      MakeRootAnim(c_obj, group, frame_list, n_frames); 
    }
    else
    {
#if 0 // debug only
      if(strcmp(group->name(), root_name)){
        Fix fixed;
        GetFixed(group, &fixed, START_TIME, NULL);
        InsertFixed(c_obj, fixed);
      }
#else
      switch(type)
      {
      case FFIXED: // theoretically this should not happen
        MakeDofConnection(c_obj, group, NULL); 
        break;
      case PRISMATIC:
        MakePrisAnim(c_obj, group, frame_list, n_frames, NULL);
        break;
      case REVOLUTE:
        MakeRevAnim(c_obj, group, group, frame_list, n_frames, NULL, NULL);
        break;
      case LOOSE:
        fprintf(stderr,
          "Warning: %s has both revolute & translational animation.\n",
           group->name());
        fprintf(stderr,"Erporting as REVOLUTE for now.\n");
               
        MakeRevAnim(c_obj, group, group, frame_list, n_frames, NULL, NULL);
        break;
      default:
        fprintf(stderr,"Error: bad animation type %d.\n",type);
        exit(1);
      }
#endif
    }
}

void MakeDofConnection(CompoundObject *c_obj, AlGroupNode *group, AlGroupNode *parent)
{
  Fix fixed;
  DofData dof_data;

  // root doesn't get connected
  if(!strcmp(root_name, group->name()))
  {
     return;
  }
  else
  {
    GetFixed(group, group, &fixed, START_TIME, parent, parent);

    if(group->joint())
    {
      GetDofData(group, group, &dof_data, parent, parent);

       if(exporting_deformable)
       {
          dof_data.type = SPHERICAL;
       }

      if(dof_data.type==FFIXED){
        InsertFixed(c_obj, fixed);
      }else
      if(dof_data.type==PRISMATIC){
        Pris prismatic;
        LoadPrismatic(&prismatic, &fixed, &dof_data, ALIAS_JOINT);
        InsertPris(c_obj, prismatic);
      }else
      if(dof_data.type==REVOLUTE){
        Rev revolute;
        LoadRevolute(&revolute, &fixed, &dof_data, ALIAS_JOINT);
        InsertRev(c_obj, revolute);
      }else
      if(dof_data.type==SPHERICAL){
        PersistSphere sphere;
        LoadSphere(&sphere, &fixed, &dof_data, ALIAS_JOINT);
        InsertSphere(c_obj, sphere);
      }
      else{
        Winprint("Error: bad dof connection type.\n");
        exit(1);
      }
    }
    else{
      InsertFixed(c_obj, fixed);
    }
  }
}

void MakeSphereConnection(CompoundObject *c_obj, AlGroupNode *r_group,
                          AlGroupNode *t_group,
                          Frame *frame_list, int n_frames,
                          AlGroupNode *r_parent, AlGroupNode *t_parent)
{
  Fix fixed;
  DofData dof_data;

  GetFixed(r_group, t_group, &fixed, START_TIME, r_parent, t_parent);
  GetDofData(r_group, t_group, &dof_data, r_parent, t_parent);

  if((dof_data.type != REVOLUTE && dof_data.type != SPHERICAL) &&
     exporting_deformable)
  {
    fprintf(stderr,
      "Error: character joint \"%s\" does NOT have any limits specified!\n",
             r_group->name());
  }

  float axis[3] = {0,0,0};
  SINGLE min=0, max=0;
  float min_v[3] = {0,0,0}, max_v[3]={0,0,0};
  int type = SPHERICAL;
  if(frame_list && n_frames > 0)
  {
    type = AnimAxisMinMax(frame_list, n_frames, axis, &min, &max, REVOLUTE,
                        min_v, max_v);
  }
  //printf("min max axis %f %f    %f %f %f\n",min,max,axis[0],axis[1],axis[2]);

  dof_data.axis[0]=axis[0];
  dof_data.axis[1]=axis[1];
  dof_data.axis[2]=axis[2];

  if(!exporting_deformable)
  {
    if(min<dof_data.min_angle)
      dof_data.min_angle=min;
    if(max>dof_data.max_angle)
      dof_data.max_angle=max;
  }

  if(type==SPHERICAL || type==REVOLUTE)
  {
    PersistSphere sphere;

    LoadSphere(&sphere, &fixed, &dof_data, ALIAS_JOINT);

    if(exporting_deformable &&
       (dof_data.type == REVOLUTE || dof_data.type == SPHERICAL))
       // if type is other one summary warning was already issued above
    {
        #define ANGLE_TOLERANCE 0.0001f
        if(min_v[0] + ANGLE_TOLERANCE < sphere.min_about_i){
            fprintf(stderr,"Error: min DOF angle about X for %s is set to\n%.5f but"
                     " animation goes up to %.5f !\n",
            r_group->name(), sphere.min_about_i*R2D, min_v[0]*R2D);
            //sphere.min_about_i = min_v[0];
        }
        if(max_v[0] - ANGLE_TOLERANCE > sphere.max_about_i){
            fprintf(stderr,"Error: max DOF angle about X for %s is set to\n%.5f but"
                     " animation goes up to %.5f !\n",
            r_group->name(), sphere.max_about_i*R2D, max_v[0]*R2D);
            //sphere.max_about_i = max_v[0];
        }

        if(min_v[1] + ANGLE_TOLERANCE < sphere.min_about_j){
            fprintf(stderr,"Error: min DOF angle about Y for %s is set to\n%.5f but"
                     " animation goes up to %.5f !\n",
            r_group->name(), sphere.min_about_j*R2D, min_v[1]*R2D);
            //sphere.min_about_j = min_v[1];
        }
        if(max_v[1] - ANGLE_TOLERANCE > sphere.max_about_j){
            fprintf(stderr,"Error: max DOF angle about Y for %s is set to\n%.5f but"
                     " animation goes up to %.5f !\n",
            r_group->name(), sphere.max_about_j*R2D, max_v[1]*R2D);
            //sphere.max_about_j = max_v[1];
        }

        if(min_v[2] + ANGLE_TOLERANCE < sphere.min_about_k){
            fprintf(stderr,"Error: min DOF angle about Z for %s is set to\n%.5f but"
                     " animation goes up to %.5f !\n",
            r_group->name(), sphere.min_about_k*R2D, min_v[2]*R2D);
            //sphere.min_about_k = min_v[2];
        }
        if(max_v[2] - ANGLE_TOLERANCE > sphere.max_about_k){
            fprintf(stderr,"Error: max DOF angle about Z for %s is set to\n%.5f but"
                     " animation goes up to %.5f !\n",
            r_group->name(), sphere.max_about_k*R2D, max_v[2]*R2D);
            //sphere.max_about_k = max_v[2];
        }
        #undef ANGLE_TOLERANCE
    }
    else
    {
      if(dof_data.type!=-1)
      {
        sphere.min_about_i = (dof_data.min_r[0] < min_v[0]) 
                             ? dof_data.min_r[0] : min_v[0];
        sphere.min_about_j = (dof_data.min_r[1] < min_v[1]) 
                             ? dof_data.min_r[1] : min_v[1];
        sphere.min_about_k = (dof_data.min_r[2] < min_v[2]) 
                             ? dof_data.min_r[2] : min_v[2];

        sphere.max_about_i = (dof_data.max_r[0] > max_v[0])
                             ? dof_data.max_r[0] : max_v[0];
        sphere.max_about_j = (dof_data.max_r[1] > max_v[1])
                             ? dof_data.max_r[1] : max_v[1];
        sphere.max_about_k = (dof_data.max_r[2] > max_v[2])
                             ? dof_data.max_r[2] : max_v[2];
      }
      else
      {
        // void QuatToEuler(Quat &q, float *ang, int type);
        sphere.min_about_i=sphere.min_about_j=sphere.min_about_k=(float)(-360.0*D2R);
        sphere.max_about_i=sphere.max_about_j=sphere.max_about_k=(float)( 360.0*D2R);
      }
    }

    InsertSphere(c_obj, sphere);
  }
  else
  {
    Winprint("Error: invalid joint type.\n");
    exit(1);
  }
}

void GetMatrixScale(const AlTM & m, float s[3])
{
  s[0] = (float)sqrt(m[0][0]*m[0][0] + m[0][1]*m[0][1] + m[0][2]*m[0][2]);
  s[1] = (float)sqrt(m[1][0]*m[1][0] + m[1][1]*m[1][1] + m[1][2]*m[1][2]);
  s[2] = (float)sqrt(m[2][0]*m[2][0] + m[2][1]*m[2][1] + m[2][2]*m[2][2]);
}

void RotateByX(AlTM & tm, const double angle)
{
  const double s = sin(angle);
  const double c = cos(angle);

  for(int i=0; i<4; i++)
  {
    const double t = tm[i][1];

    tm[i][1] = t * c - tm[i][2] * s;
    tm[i][2] = t * s + tm[i][2] * c;
  }
}

void RotateByY(AlTM & tm, const double angle)
{
  const double s = sin(angle);
  const double c = cos(angle);

  for(int i=0; i<4; i++)
  {
    const double t = tm[i][0];

    tm[i][0] = t * c + tm[i][2] * s;
    tm[i][2] = tm[i][2] * c - t * s;
  }
}

void RotateByZ(AlTM & tm, const double angle)
{
  const double s = sin(angle);
  const double c = cos(angle);

  for(int i=0; i<4; i++)
  {
    const double t = tm[i][0];

    tm[i][0] = t * c - tm[i][1] * s;
    tm[i][1] = t * s + tm[i][1] * c;
  }
}

bool IsIdentity(const AlTM & tm)
{
  const float t = .0001;
  return(
   fabs(tm[0][1]) < t &&
   fabs(tm[0][2]) < t &&
   fabs(tm[0][3]) < t &&
   fabs(tm[1][0]) < t &&
   fabs(tm[1][2]) < t &&
   fabs(tm[1][3]) < t &&
   fabs(tm[2][0]) < t &&
   fabs(tm[2][1]) < t &&
   fabs(tm[2][3]) < t &&
   fabs(tm[3][0]) < t &&
   fabs(tm[3][1]) < t &&
   fabs(tm[3][2]) < t &&
   fabs(1.0 - tm[0][0]) < t &&
   fabs(1.0 - tm[1][1]) < t &&
   fabs(1.0 - tm[2][2]) < t &&
   fabs(1.0 - tm[3][3]) < t 
  );
}

void GetFixed(AlGroupNode *r_group, AlGroupNode *t_group,
              Fix *fixed, double t, AlGroupNode *r_parent, AlGroupNode *t_parent)
{
  assert( (!r_parent && !t_parent) || (r_parent && t_parent) );
  SetTime(t);

  InitFixed(fixed);

  if(r_parent == NULL)
  {
    r_parent = GetParentGroupAlNode(r_group);
    t_parent = GetParentGroupAlNode(t_group);
  }

  if(r_parent==NULL) // sanity check
  {
    fprintf(stderr,"Error: GetFixed failed looking for group parent of %s\n",
    r_group->name());
    exit(1);
  }

  strcpy(fixed->child, r_group->name());
  if(exporting_deformable)
    strtok(fixed->child, "#");
  if(!strcmp(root_name, r_parent->name()))
  {
    strcpy(fixed->parent, ROOT_OBJ_NAME);
  }
  else
  {
    strcpy(fixed->parent, r_parent->name());
    if(exporting_deformable)
      strtok(fixed->parent, "#");
  }

  AlTM tm ( gf.DLX(r_group, t_group, r_parent, t_parent, t) );

  fixed->pos.x = tm[3][0];
  fixed->pos.y = tm[3][1];
  fixed->pos.z = tm[3][2];

  if( !absolute_rev ) // alias and DA vectors are opposite
  {
    fixed->orient.e00 = tm[0][0];
    fixed->orient.e01 = tm[1][0];
    fixed->orient.e02 = tm[2][0];

    fixed->orient.e10 = tm[0][1];
    fixed->orient.e11 = tm[1][1];
    fixed->orient.e12 = tm[2][1];

    fixed->orient.e20 = tm[0][2];
    fixed->orient.e21 = tm[1][2];
    fixed->orient.e22 = tm[2][2];
  }

  if( !IsRightHanded(&(fixed->orient)) )
  {
    Winprint("Error: Fixed parent=%s  child=%s is LEFT handed!\n",
      fixed->parent, fixed->child);
  }
  //printf("fixed orient\n");
  //printf("%f %f %f\n",fixed->orient.e00, fixed->orient.e01, fixed->orient.e02);
  //printf("%f %f %f\n",fixed->orient.e10, fixed->orient.e11, fixed->orient.e12);
  //printf("%f %f %f\n",fixed->orient.e20, fixed->orient.e21, fixed->orient.e22);
}

AlTM NoScale(const AlTM & m)
{
  AlTM tm ( m );

  for(int i=0; i<3; i++)
  {
    float magnitude = (float)sqrt(m[i][0]*m[i][0] + m[i][1]*m[i][1] + m[i][2]*m[i][2]);
    if(magnitude>0.0f)
    {
      //magnitude = fabs(1.0f / magnitude);
      magnitude = 1.0f / magnitude;
      tm[i][0] *= magnitude;
      tm[i][1] *= magnitude;
      tm[i][2] *= magnitude;
    }
  }
  return tm;
}

// used by HP
AlTM CleanAlMatrix(const AlTM& m)
{
  AlTM tm(1,1,1,1);
  double translate[3], scale[3], rotate[3], shear[3];

  m.decompose(translate, scale, rotate, shear);

  tm=AlTM::rotateX(rotate[0]*D2R) * 
     AlTM::rotateY(rotate[1]*D2R) * 
     AlTM::rotateZ(rotate[2]*D2R);

  tm[3][0]=translate[0]/scale[0]; // don't scale ?
  tm[3][1]=translate[1]/scale[1];
  tm[3][2]=translate[2]/scale[2];

  return(tm);
}
      
void GetDofData(AlGroupNode *r_group, AlGroupNode *t_group, DofData *data,
                AlGroupNode *r_parent, AlGroupNode *t_parent)
{
  assert( (!r_parent && !t_parent) || (r_parent && t_parent) );
  boolean rotations[3];
  boolean translations[3];
  double min_r[3], max_r[3];
  double min_t[3], max_t[3];
  double rotRest[3], transRest[3];
  int stiffness_r[3], stiffness_t[3];
  int i;
  double pivot[4];

  SetTime(START_TIME);

  InitDofData(data);
  strcpy(data->name, r_group->name()); 
  if(exporting_deformable)
    strtok(data->name, "#");

  // pivot in world space coords
  r_group->rotatePivot(pivot[0], pivot[1], pivot[2]);
  pivot[3]=1.0f;

  // convert to parent's local
  if(r_parent == NULL)
  {
    r_parent = GetParentGroupAlNode(r_group);
    t_parent = GetParentGroupAlNode(t_group);
  }

  AlTM tm ( gf.DGX(r_parent, t_parent, START_TIME).inverse() );
 
  tm.transPoint(pivot);
  CleanD( pivot[0] );
  CleanD( pivot[1] );
  CleanD( pivot[2] );
  data->pivot[0]=pivot[0];
  data->pivot[1]=pivot[1];
  data->pivot[2]=pivot[2];

  AlJoint *r_joint = r_group->joint();
  if(r_joint)
  {
    // statusCode AlJoint::useTransforms( boolean translations[3], 
    //                                    boolean rotations[3]) const
    r_joint->useLimits(translations, rotations);
    r_joint->rotation(min_r, max_r, stiffness_r);
    r_joint->restPose(rotRest, transRest);
  
    for(i=0; i<3; i++)
    {
      if(rotations[i])
      {
        data->min_r[i]=NormAngle((min_r[i]-rotRest[i])*D2R);
        data->max_r[i]=NormAngle((max_r[i]-rotRest[i])*D2R);
      }
    }
  }

  AlJoint *t_joint = t_group->joint();
  if(t_joint)
  {
    // statusCode AlJoint::useTransforms( boolean translations[3], 
    //                                    boolean rotations[3]) const
    t_joint->useLimits(translations, rotations);
    t_joint->translation(min_t, max_t, stiffness_t);
    t_joint->restPose(rotRest, transRest);
  
    for(i=0; i<3; i++)
    {
      if(translations[i])
      {
        data->min_t[i]=min_t[i]-transRest[i];
        data->max_t[i]=max_t[i]-transRest[i];
      }
    }
  }

  CleanDofData(data);
}
    
object* GetObject(CompoundObject *c_obj, const char *file_name)
{
  int i;

  for(i=0; i<c_obj->part_count; i++){
    if(!strcmp(c_obj->lod_object_list[i].file_name, file_name)){
      return (&(c_obj->lod_object_list[i].obj_list[0]));
    }
  }

  fprintf(stderr, "Error: object %s not found.\n", file_name);
  exit(1);

  return NULL;
}
 
void DefineHP(object *obj, AlGroupNode *obj_group, AlGroupNode *hp_group)
{
  if(hp_group==NULL)
    return; // should not happen

  SetTime(START_TIME);

  PersistHPFixed hp_fixed;
  InitHPFixed(&hp_fixed);

  char tmp_name[256];
  strcpy(tmp_name, hp_group->name());
  strtok(tmp_name, "#");

  char *pt=tmp_name;
  for(int i=0; i<strlen(tmp_name); i++)
  {
    if(!strncmp(pt, "hp", 2))
    {
      break;
    }
    pt++; 
  }
  strcpy(hp_fixed.name, pt);

  // create WORLD matrix
  double rx, ry, rz;
  hp_group->localRotationAngles(rx, ry, rz);
  if(verbose_level >= 2)
  {
    printf("group=%s, hp=%s\n", obj_group->name(), hp_group->name());
    printf("HP angles %f %f %f\n", rx, ry, rz);
  }

  AlTM tm ( AlTM::rotateX(rx*D2R) * AlTM::rotateY(ry*D2R) * AlTM::rotateZ(rz*D2R) );

  if( !absolute_rev ) // deformable orientations all export as identity
  {
    // multiply by parent's inv world
    AlTM inv_p_tm = gf.GX(obj_group, START_TIME).inverse();
    inv_p_tm = CleanAlMatrix(inv_p_tm);
    tm *= inv_p_tm;
  }

  // set orientation
  hp_fixed.orientation.e00=tm[0][0];
  hp_fixed.orientation.e01=tm[1][0];
  hp_fixed.orientation.e02=tm[2][0];

  hp_fixed.orientation.e10=tm[0][1];
  hp_fixed.orientation.e11=tm[1][1];
  hp_fixed.orientation.e12=tm[2][1];

  hp_fixed.orientation.e20=tm[0][2];
  hp_fixed.orientation.e21=tm[1][2];
  hp_fixed.orientation.e22=tm[2][2];

  if(verbose_level>=2){
    printf("hp orient\n");
    printf("%f %f %f\n",hp_fixed.orientation.e00,
      hp_fixed.orientation.e01, hp_fixed.orientation.e02);
    printf("%f %f %f\n",hp_fixed.orientation.e10,
      hp_fixed.orientation.e11, hp_fixed.orientation.e12);
    printf("%f %f %f\n",hp_fixed.orientation.e20,
      hp_fixed.orientation.e21, hp_fixed.orientation.e22);
  }

  // set position
  DofData dof_data;
  GetDofData(hp_group, hp_group, &dof_data, NULL, NULL);
  hp_fixed.point.x=dof_data.pivot[0];
  hp_fixed.point.y=dof_data.pivot[1];
  hp_fixed.point.z=dof_data.pivot[2];

  // convert limits to new coord system ??

  if(dof_data.type==FFIXED){
    InsertHPFixed(obj, hp_fixed);
  }else
  if(dof_data.type==PRISMATIC){
    PersistHPPrismatic hp_prismatic;

    hp_prismatic.spot=hp_fixed;
    hp_prismatic.axis.x=dof_data.axis[0];
    hp_prismatic.axis.y=dof_data.axis[1];
    hp_prismatic.axis.z=dof_data.axis[2];
    hp_prismatic.min=dof_data.min_step;
    hp_prismatic.max=dof_data.max_step;
    InsertHPPrismatic(obj, hp_prismatic);
  }else
  if(dof_data.type==REVOLUTE){
    PersistHPRevolute hp_revolute;

    hp_revolute.spot=hp_fixed;
    hp_revolute.axis.x=dof_data.axis[0];
    hp_revolute.axis.y=dof_data.axis[1];
    hp_revolute.axis.z=dof_data.axis[2];
    hp_revolute.min=dof_data.min_angle;
    hp_revolute.max=dof_data.max_angle;
    InsertHPRevolute(obj, hp_revolute);
  }else
  if(dof_data.type==SPHERICAL)
  {
    Winprint("Error: SPHERICAL HP not yet supported %s.\n",hp_group->name());
    exit(1);
  }
  else{
    Winprint("Error: unknown HP type %s.hp_group->name()\n");
    exit(1);
  }
}

void MakeRevAnim(CompoundObject *c_obj, AlGroupNode *r_group, AlGroupNode *t_group,
                 Frame *frame_list, 
                 int n_frames, AlGroupNode *r_parent, AlGroupNode *t_parent)
{
  assert( (r_parent && t_parent) || (!r_parent && !t_parent) );
  Fix fixed;
  DofData dof_data;
  int type;
  float axis[3];
  SINGLE min, max;
  NamedChannel n_channel;
  float tmp_v[3];
  int i;

  GetFixed(r_group, t_group, &fixed, START_TIME, r_parent, t_parent);
  GetDofData(r_group, t_group, &dof_data, r_parent, t_parent);

  if((dof_data.type != REVOLUTE && dof_data.type != SPHERICAL) &&
     exporting_deformable)
  {
    fprintf(stderr,
      "Error: character joint \"%s\" does NOT have any limits specified!\n",
             r_group->name());
  }

  float min_v[3], max_v[3];
  type = AnimAxisMinMax(frame_list, n_frames, axis, &min, &max, REVOLUTE,
                      min_v, max_v);
  //printf("min max axis %f %f    %f %f %f\n",min,max,axis[0],axis[1],axis[2]);

  dof_data.axis[0]=axis[0];
  dof_data.axis[1]=axis[1];
  dof_data.axis[2]=axis[2];

  if(!exporting_deformable)
  {
    if(min<dof_data.min_angle)
      dof_data.min_angle=min;
    if(max>dof_data.max_angle)
      dof_data.max_angle=max;
  }

  char branch_name[64];
  if(r_parent)
    strcpy(branch_name, r_parent->name());
  else
    GetBranchName(r_group, branch_name);

  if(type==REVOLUTE && !exporting_deformable)
  {
    Rev revolute;

    LoadRevolute(&revolute, &fixed, &dof_data, ALIAS_JOINT);
    InsertRev(c_obj, revolute);

    // Add Channel
    InitNamedChannel(&n_channel);

    if(!exporting_deformable)
    {
      sprintf(n_channel.name, "Ch_%s_%s", branch_name, r_group->name());
    }
    else
    {
      sprintf(n_channel.name, "Ch_%s_%s", FileName, r_group->name());
      strtok(n_channel.name, "#");
    }

    n_channel.first_frame=frame_list[0].index;
    n_channel.last_frame=frame_list[n_frames-1].index;

    n_channel.channel.header.frames=n_frames;
    n_channel.channel.header.capture_rate=(float)(1.0/fps); // fix later
    n_channel.channel.header.type=PersistDT_FLOAT;
    n_channel.channel.data=
      (unsigned char*)Malloc(n_channel.channel.header.frames*sizeof(float));

    for(i=0;i<n_frames;i++){
      tmp_v[0]=frame_list[i].vector_r.x;
      tmp_v[1]=frame_list[i].vector_r.y;
      tmp_v[2]=frame_list[i].vector_r.z;
      *(float*)(n_channel.channel.data+4*i)=frame_list[i].angle *
               (float)cos(GetAngle(axis, tmp_v));
      // printf("angle %f\n",R2D*GetAngle(axis, tmp_v));
    }

  }else
  if(type==SPHERICAL || type==REVOLUTE){
    PersistSphere sphere;

    LoadSphere(&sphere, &fixed, &dof_data, ALIAS_JOINT);

    if(exporting_deformable &&
       (dof_data.type == REVOLUTE || dof_data.type == SPHERICAL))
       // if type is other one summary warning was already issued above
    {
        #define ANGLE_TOLERANCE 0.0001f
        if(min_v[0] + ANGLE_TOLERANCE < sphere.min_about_i){
            fprintf(stderr,"Error: min DOF angle about X for %s is set to\n%.5f but"
                     " animation goes up to %.5f !\n",
            r_group->name(), sphere.min_about_i*R2D, min_v[0]*R2D);
            //sphere.min_about_i = min_v[0];
        }
        if(max_v[0] - ANGLE_TOLERANCE > sphere.max_about_i){
            fprintf(stderr,"Error: max DOF angle about X for %s is set to\n%.5f but"
                     " animation goes up to %.5f !\n",
            r_group->name(), sphere.max_about_i*R2D, max_v[0]*R2D);
            //sphere.max_about_i = max_v[0];
        }

        if(min_v[1] + ANGLE_TOLERANCE < sphere.min_about_j){
            fprintf(stderr,"Error: min DOF angle about Y for %s is set to\n%.5f but"
                     " animation goes up to %.5f !\n",
            r_group->name(), sphere.min_about_j*R2D, min_v[1]*R2D);
            //sphere.min_about_j = min_v[1];
        }
        if(max_v[1] - ANGLE_TOLERANCE > sphere.max_about_j){
            fprintf(stderr,"Error: max DOF angle about Y for %s is set to\n%.5f but"
                     " animation goes up to %.5f !\n",
            r_group->name(), sphere.max_about_j*R2D, max_v[1]*R2D);
            //sphere.max_about_j = max_v[1];
        }

        if(min_v[2] + ANGLE_TOLERANCE < sphere.min_about_k){
            fprintf(stderr,"Error: min DOF angle about Z for %s is set to\n%.5f but"
                     " animation goes up to %.5f !\n",
            r_group->name(), sphere.min_about_k*R2D, min_v[2]*R2D);
            //sphere.min_about_k = min_v[2];
        }
        if(max_v[2] - ANGLE_TOLERANCE > sphere.max_about_k){
            fprintf(stderr,"Error: max DOF angle about Z for %s is set to\n%.5f but"
                     " animation goes up to %.5f !\n",
            r_group->name(), sphere.max_about_k*R2D, max_v[2]*R2D);
            //sphere.max_about_k = max_v[2];
        }
        #undef ANGLE_TOLERANCE
    }
    else
    {
      if(dof_data.type!=-1)
      {
        sphere.min_about_i = (dof_data.min_r[0] < min_v[0]) 
                             ? dof_data.min_r[0] : min_v[0];
        sphere.min_about_j = (dof_data.min_r[1] < min_v[1]) 
                             ? dof_data.min_r[1] : min_v[1];
        sphere.min_about_k = (dof_data.min_r[2] < min_v[2]) 
                             ? dof_data.min_r[2] : min_v[2];

        sphere.max_about_i = (dof_data.max_r[0] > max_v[0])
                             ? dof_data.max_r[0] : max_v[0];
        sphere.max_about_j = (dof_data.max_r[1] > max_v[1])
                             ? dof_data.max_r[1] : max_v[1];
        sphere.max_about_k = (dof_data.max_r[2] > max_v[2])
                             ? dof_data.max_r[2] : max_v[2];
      }
      else
      {
        // void QuatToEuler(Quat &q, float *ang, int type);
        sphere.min_about_i=sphere.min_about_j=sphere.min_about_k=(float)(-360.0*D2R);
        sphere.max_about_i=sphere.max_about_j=sphere.max_about_k=(float)( 360.0*D2R);
      }
    }

    InsertSphere(c_obj, sphere);
		
    // Channel
    InitNamedChannel(&n_channel);
    sprintf(n_channel.name, "Ch_%s_%s", FileName, r_group->name());
    if(exporting_deformable)
      strtok(n_channel.name, "#");

    n_channel.first_frame=frame_list[0].index;
    n_channel.last_frame=frame_list[n_frames-1].index;
    
    n_channel.channel.header.frames=n_frames;
    n_channel.channel.header.capture_rate=(float)(1.0/fps); // fix later
    n_channel.channel.header.type=PersistDT_QUATERNION;
    n_channel.channel.data=
      (unsigned char*)Malloc(n_channel.channel.header.frames*
      sizeof(PersistQuaternion));
    for(i=0;i<n_frames;i++){
      *(PersistQuaternion*)(n_channel.channel.data+sizeof(PersistQuaternion)*i)=
      frame_list[i].quat.v;
    }
  }
  else{
    Winprint("Error: invalid joint type.\n");
    exit(1);
  }

  // Script
  NamedScript script;
  InitScript(&script);
  if(exporting_deformable)
  {
    sprintf(script.name, "Sc_%s", FileName);
  }
  else
  {
    sprintf(script.name, "Sc_%s", branch_name); 
  }

  script.channel_count=1;
  script.channel_list=(PersistAnimChannelMapping*)
                      Malloc(sizeof(PersistAnimChannelMapping));
  strcpy(script.channel_list[0].parent, fixed.parent);
  strcpy(script.channel_list[0].child, fixed.child);
  strcpy(script.channel_list[0].channel, n_channel.name);

  // add data to cmp object
  InsertNamedChannel(c_obj, n_channel);
  InsertScript(c_obj, script);
}

void GetBranchName(AlGroupNode *group, char branch_name[64])
{
   branch_name[0] = 0;

   AlGroupNode *node = group;
   while(node && node->parentNode())
   {
     if( !strcmp(node->parentNode()->name(), root_name) )
     {
       strcpy(branch_name, node->name());
       break;
     }

     node = node->parentNode();
   }

   if(branch_name[0] == 0)
   {
     Winprint("Error: could not find GetBranchName() for %s\n",
       group->name());
   }
}

void MakePrisAnim(CompoundObject *c_obj, AlGroupNode *group, Frame *frame_list, 
                  int n_frames, AlGroupNode *parent)
{
  Fix fixed;
  DofData dof_data;
  int type;
  float axis[3];
  SINGLE min, max;
  NamedChannel n_channel;
  float tmp_v[3];
  int i;

  if(exporting_deformable)
  {
    Winprint("Error: character's node %s has PRISMATIC animation!\n",
             group->name());
  }

  GetFixed(group, group, &fixed, START_TIME, parent, parent);
  GetDofData(group, group, &dof_data, parent, parent);

  float min_v[3], max_v[3];
  type=AnimAxisMinMax(frame_list, n_frames, axis, &min, &max, PRISMATIC,
                      min_v, max_v);
  //printf("min max axis %f %f    %f %f %f\n",min,max,axis[0],axis[1],axis[2]);

  dof_data.axis[0]=axis[0];
  dof_data.axis[1]=axis[1];
  dof_data.axis[2]=axis[2];

  if(min<dof_data.min_step)
    dof_data.min_step=min;
  if(max>dof_data.max_step)
    dof_data.max_step=max;

  char branch_name[64];
  if(parent)
    strcpy(branch_name, parent->name());
  else
    GetBranchName(group, branch_name);

  if(1/*type==PRISMATIC*/) // TRANSLATIONAL not allowed due to IK
  {
    Pris prismatic;
    LoadPrismatic(&prismatic, &fixed, &dof_data, ALIAS_JOINT);

    if(min<prismatic.min)
      prismatic.min=min;
    if(max>prismatic.max)
      prismatic.max=max;
		
    InsertPris(c_obj, prismatic);

    // Channel
    InitNamedChannel(&n_channel);

    if(!exporting_deformable)
    {
      sprintf(n_channel.name, "Ch_%s_%s", branch_name, group->name());
    }
    else
    {
      sprintf(n_channel.name, "Ch_%s_%s", FileName, group->name());
      strtok(n_channel.name, "#");
    }

    n_channel.first_frame=frame_list[0].index;
    n_channel.last_frame=frame_list[n_frames-1].index;

    n_channel.channel.header.frames=n_frames;
    n_channel.channel.header.capture_rate=(float)(1.0/fps); // fix later
    n_channel.channel.header.type=PersistDT_FLOAT;
    n_channel.channel.data=
      (unsigned char*)Malloc(n_channel.channel.header.frames*sizeof(float));

    axis[0]=prismatic.axis.x;
    axis[1]=prismatic.axis.y;
    axis[2]=prismatic.axis.z;

    for(i=0;i<n_frames;i++){
      tmp_v[0]=frame_list[i].vector_t.x;
      tmp_v[1]=frame_list[i].vector_t.y;
      tmp_v[2]=frame_list[i].vector_t.z;
      *(float*)(n_channel.channel.data+4*i)=frame_list[i].step*
               (float)cos(GetAngle(axis, tmp_v));
      // printf("angle %f\n",R2D*GetAngle(axis, tmp_v));
    }
  }else
  if(type==TRANSLATIONAL){ // 3D
    InsertTrans(c_obj, (Trans)fixed);

    // Channel
    InitNamedChannel(&n_channel);
    sprintf(n_channel.name, "Ch_%s_%s", FileName, group->name());
    if(exporting_deformable)
      strtok(n_channel.name, "#");

    n_channel.first_frame=frame_list[0].index;
    n_channel.last_frame=frame_list[n_frames-1].index;

    n_channel.channel.header.frames=n_frames;
    n_channel.channel.header.capture_rate=(float)(1.0/fps); // fix later
    n_channel.channel.header.type=PersistDT_VECTOR;
    n_channel.channel.data=
      (unsigned char*)Malloc(n_channel.channel.header.frames*sizeof(PersistVector));
    for(i=0;i<n_frames;i++){
      *(PersistVector*)(n_channel.channel.data+sizeof(PersistVector)*i)=
      frame_list[i].vector_t;
    }
  }
  else{
    fprintf(stderr,"Error: invalid joint type.\n");
    exit(1);
  }

  // Script
  NamedScript script;
  InitScript(&script);
  if(exporting_deformable)
  {
    sprintf(script.name, "Sc_%s", FileName);
  }
  else
  {
    sprintf(script.name, "Sc_%s", branch_name); 
  }
  script.channel_count=1;
  script.channel_list=(PersistAnimChannelMapping*)
                      Malloc(sizeof(PersistAnimChannelMapping));
  strcpy(script.channel_list[0].parent, fixed.parent);
  strcpy(script.channel_list[0].child, fixed.child);
  strcpy(script.channel_list[0].channel, n_channel.name);

  // add data to cmp object
  InsertNamedChannel(c_obj, n_channel);
  InsertScript(c_obj, script);

}

AlGroupNode* GetSkelRootNode(AlDagNode *aldagnode)
{
  return FindBoneRoot( GetSkinRef( AlUniverse::firstDagNode() ) );
}

AlGroupNode* GetHeadingNode(AlDagNode *aldagnode)
{
  if(aldagnode == NULL)
    return NULL;

  if(aldagnode->asGroupNodePtr() &&
     (!strcmp("heading", aldagnode->name()) ||
      !strcmp("Heading", aldagnode->name())) )
  {
    return aldagnode->asGroupNodePtr();
  }

  // traverse across
  AlGroupNode *result = GetHeadingNode(aldagnode->nextNode());
  if(result)
  {
    return result;
  } 
  else // traverse down
  if(aldagnode->asGroupNodePtr())
  {
    return GetHeadingNode(aldagnode->asGroupNodePtr()->childNode());
  }
 
  return NULL; 
}

AlClusterNode* GetSkinRef(AlDagNode *aldagnode)
{
  if(aldagnode == NULL) return NULL;

  if(aldagnode->asClusterNodePtr() &&  // make sure it's a cluster
     aldagnode->asClusterNodePtr()->cluster()->firstMember() ) 
     // make sure it's not empty
  {
    return aldagnode->asClusterNodePtr();
  }

  // traverse down
  if(aldagnode->asGroupNodePtr())
  {
    AlClusterNode *result;
    result=GetSkinRef(aldagnode->asGroupNodePtr()->childNode());
    if(result)
    {
      return result;
    } 
  }

  // traverse across
  return GetSkinRef(aldagnode->nextNode());
}

struct NodeEvent
{
  AlDagNode *node;
  int event_count;
  double *time_list;
};

void ResetState(void)
{
  AlUniverse::doUpdates( TRUE );

  double start, end, step;
  AlUniverse::frameRange(kMinMax, start, end, step);

  if(init_time+step <= end)
  {
    SetTime(init_time+step);
    AlUniverse::redrawScreen(kRedrawAll);
  }
  else
  if(init_time-step >= start)
  {
    SetTime(init_time-step);
    AlUniverse::redrawScreen(kRedrawAll);
  }

  SetTime(init_time);
  AlUniverse::redrawScreen(kRedrawAll);

  AlUniverse::doUpdates( FALSE );
}

void GetInvisibleNodes(NodeEvent **event_list, int *count)
{
  int i;
  double t;
  double start, end, step;
  AlUniverse::frameRange(kMinMax, start, end, step);
  start = START_TIME;
  int n_frames = 1 + (int)((end-start)/step);

  *count = 0;
  *event_list = NULL;

  SetTime(start);
  AlDagNode *node = AlUniverse::firstDagNode();
  while(node)
  {
    SetTime(start, node);
    boolean visible = node->isDisplayModeSet(kDisplayModeInvisible);
    //printf("%s %d\n",node->name(), visible);

    for(i=1,t=start+step; i < n_frames; i++,t+=step)
    {
      SetTime(t, node);
      if (visible != node->isDisplayModeSet(kDisplayModeInvisible))
      {
        (*count)++;
        (*event_list) =
          (NodeEvent*)Realloc((*event_list), (*count) * sizeof(NodeEvent));
        (*event_list)[(*count)-1].node = node;
        (*event_list)[(*count)-1].event_count = 0;
        (*event_list)[(*count)-1].time_list = NULL;
        break;
      }
    }

    SetTime(start, node);
    node = node->nextNode();
  }

  // store times
  int j;
  for(j=0; j < *count; j++)
  {
    AlDagNode *node = (*event_list)[j].node;
    for(i=0,t=start; i < n_frames; i++,t+=step)
    {
      SetTime(t, node);
      if ( !(node->isDisplayModeSet(kDisplayModeInvisible)) )
      {
        (*event_list)[j].event_count++;
        (*event_list)[j].time_list =
	  (double*)Realloc((*event_list)[j].time_list,
	  (*event_list)[j].event_count * sizeof(double));
        (*event_list)[j].time_list[(*event_list)[j].event_count-1] = t/fps;
      }
    }
  }

  SetTime(start);
}

void ExportEvents(CompoundObject *c_obj)
{
  // Add Channel
  NamedChannel n_channel;
  InitNamedChannel(&n_channel);
  
  sprintf(n_channel.name, "Ch_%s_events", FileName);
  n_channel.channel.header.capture_rate = -1.0f;
  n_channel.channel.header.type = PersistDT_EVENT;
  n_channel.channel.header.frames = 0;
  n_channel.data_size = 0;
  n_channel.channel.data = NULL;
  
  // read data from OpenAlias
  NodeEvent *event_list=NULL;
  int count;
  GetInvisibleNodes(&event_list, &count);
  if(count > 0)
  {
    assert(event_list);
    int total_events = 0;
    for(int i=0; i < count; i++)
    {
      //printf("%s\n",event_list[i].node->name());
      for(int j=0; j<event_list[i].event_count; j++)
      {
        //printf("   %d t=%f\n",j,event_list[i].time_list[j]);
        total_events++;
  
        InsertChannelEvent(n_channel, event_list[i].time_list[j],
          NAMED_EVENT, event_list[i].node->name()); 
      }
    }
    assert(n_channel.channel.header.frames == total_events);
  }

  double start, end, step;
  AlUniverse::frameRange(kMinMax, start, end, step);
  start = START_TIME;
  InsertChannelEvent(n_channel, start/fps, CHANNEL_BEGIN, NULL);
  InsertChannelEvent(n_channel, end/fps, CHANNEL_END, NULL);
  
  // Script
  NamedScript script;
  InitScript(&script);
  sprintf(script.name, "Sc_%s", FileName);
  script.channel_count=1;
  script.channel_list=(PersistAnimChannelMapping*)
                      Malloc(sizeof(PersistAnimChannelMapping));
  script.channel_list[0].parent[0]=0;
  script.channel_list[0].child[0]=0;
  strcpy(script.channel_list[0].channel, n_channel.name);

  // add data to cmp object
  InsertNamedChannel(c_obj, n_channel);
  InsertScript(c_obj, script);

  // Cleanup
  for(int i=0; i<count; i++)
  {
    Free(event_list[i].time_list);
  }
  Free(event_list);
  
}

void ExportDefMesh(AlClusterNode *skin_ref_node)
{
  AlGroupNode *heading_node = GetHeadingNode( AlUniverse::firstDagNode() );
  if(use_heading && !heading_node)
  {
    fprintf(stderr, "Error: %s has NO Heading node!\n",
      AlUniverse::stageWireFileName(AlUniverse::currentStage()));
    return;
  }

  double start, end , step;
  AlUniverse::frameRange(kMinMax, start, end, step);

  if(start > 0 || end < 0)
  {
    Winprint("Error: there must be a key frame at time 0!");
    fprintf(stderr,"start=%f end=%f step=%f\n",start,end,step);
    return;
  }

  CompoundObject c_obj;
  InitCompoundObject(&c_obj);

  object obj;
  InitObject(&obj, &(c_obj.tl), &(c_obj.atl));
  obj.type = DEF_MESH;

  sprintf(c_obj.file_name,"%s/%s.cmp",DirName,FileName); 

  AlDagNode *skin_node = FindSkin(skin_ref_node); // AlGroupNode or AlPolysetNode
  assert(skin_node);

  char mesh_file_name[256];
  sprintf(mesh_file_name, "%s.3db",skin_node->name());

  AlGroupNode *bone_root_node = FindBoneRoot(skin_ref_node);
  assert(bone_root_node);

  if(use_heading)
  {
    root_name = (char*)Malloc((strlen(heading_node->name())+1)*sizeof(char));
    strcpy(root_name, heading_node->name());
  }
  else
  {
    root_name = (char*)Malloc((strlen(bone_root_node->name())+1)*sizeof(char));
    strcpy(root_name, bone_root_node->name());
  }

  bone_lib bl;
  bl.Init();
  if(use_heading)
  {
    AddBone(&bl, heading_node, 0, NULL);
  }
  GetBones(bone_root_node, &bl, -1, START_TIME); // last_bone, time
  if(use_heading)
  {
    bl.list[0].r_node = heading_node;
    bl.list[0].t_node = bone_root_node;
    bl.list[0].r_parent = NULL;
    bl.list[0].t_parent = NULL;

    bl.list[1].r_node = bone_root_node;
    bl.list[1].t_node = bone_root_node;
    bl.list[1].r_parent = heading_node;
    bl.list[1].t_parent = bone_root_node;
  }

  if( CheckBones(&bl) == false )
  {
    bl.Release();
    FreeCmpObject(&c_obj);
    FreeObject(&obj);
    return;
  }

  // export skin
  ExportMerged(&obj, skin_node, 0, START_TIME, bl.list[0].r_node,
                                               bl.list[0].t_node); // level, time
  PostProcessMesh(&obj, DENSITY, 100.0, 0, 0, true, skin_node->name(), 1.0f);

#if 0
  WriteObject(&obj, mesh_file_name, TXT_ON);
#endif

  if(utf_animation)
  {
    ExportEvents(&c_obj); // visible/invisible nodes
    ResetState();
  }

  // used for deform lod  TransformBoneVertices
  // no longer - also TransformVertices()
  GetBoneTMs(&bl, START_TIME); // stores inv_bone_to_world xform

  SetTime(START_TIME);
  if(AssignVertices(&bl, &(obj.v), START_TIME) == -1)
  {
    bl.Release();
    FreeCmpObject(&c_obj);
    FreeObject(&obj);
    return;
  }

  // compute vertices in bone local coord systems
  TransformVertices(&bl, &(obj.v), &(obj.n));

  // extract scale from root adjust node
  GetSkeletonScale( &c_obj, bone_root_node->parentNode() );


  // root xform used for floor height
  c_obj.root_transform = AlTM_to_PersistTransform( 
    gf.DGX(bl.list[0].r_node, bl.list[0].t_node, START_TIME) );
  c_obj.root_transform.v.x /= c_obj.scale;
  c_obj.root_transform.v.y /= c_obj.scale;
  c_obj.root_transform.v.z /= c_obj.scale;

  ExportBoneConnections(&c_obj, &bl);

  // prevent bones from ending up in .cmp
  for(int oid=0; oid < c_obj.part_count; oid++)
  {
    c_obj.lod_object_list[oid].export_flag = 0;
  }

  assert(c_obj.part_count == bl.count);
  ExportBoneHP(&bl, &c_obj);

  // do LOD
  if(lod_percent < 100.0f)
  {
    bool restore_uv_weight = false;
    if(lod_uv_weight == INIT_UV_WEIGHT)
    {
      lod_uv_weight = .65f;
      restore_uv_weight = true;
    }
    obj.lol.closest = lod_closest;
    obj.lol.furthest = lod_furthest;

    calcEdges(&obj);
    CollapseEdges(&obj,
      (int)(.01f * lod_percent * obj.face_count + .5f));

    ConsolidateDuplicateAssignments(&obj);
    FreeLodLib(&(obj.lol));
    RemoveUnusedNormals(&obj);

    TransformBoneVertices(&obj, &c_obj);

    // if any groups now have 0 faces remove them
    RemoveUnusedFaceGroups(&obj);

    if(restore_uv_weight)
    {
      lod_uv_weight = INIT_UV_WEIGHT; 
    }
  } 

  ComputeBoneExtents(&obj, &c_obj, DENSITY);

  if(utf_animation)
  {
    CombineChannels(&c_obj, 1);
    ExtractKeyFrames(&c_obj);
  }

  // write out file(s)
  file_node *root;
  char out_file_name[256];
  char skeleton_name[256];

  AlDagNode *adjust_root = bone_root_node->parentNode();
  if(!strstr(adjust_root->name(), "adjustroot_"))
  {
     Winprint("Error: root adjust node has bad name %s!\n"
              "Should start w/ \"adjustroot_\"\n", adjust_root->name());
     bl.Release();
     FreeCmpObject(&c_obj);
     FreeObject(&obj);
     return;
  }
  else
  {
    strcpy(skeleton_name, adjust_root->name());
    memmove(skeleton_name, skeleton_name+strlen("adjustroot_"), 
      strlen(adjust_root->name())+1);
    strtok(skeleton_name, "#");
    strcat(skeleton_name, ".cmp");
  }

  if(utf_mesh)
  {
    root = CreateNode("\\",D);
    root->child = CreateExporterVersion();
    root->child->sibling = CreateObject(&obj, TXT_ON);
    root->child->sibling->sibling = CreateRigidBody(obj.extents);
    root->child->sibling->sibling->sibling = CreateSkeletonName(skeleton_name);

    root->child->sibling->sibling->sibling->sibling = 
      CreateCmpnd(&c_obj, 0, 0, TXT_OFF); // split_flag, anim_flag

    root->child->sibling->sibling->sibling->sibling->sibling =
      CreateBoneExtents(&c_obj);
    sprintf(out_file_name,"%s/%s.dfm", DirName, FileName); 

    WriteUTF(root, out_file_name);
    FreeTree(root);
  }

  if(utf_animation)
  {
    root = CreateNode("\\",D);
    root->child = CreateExporterVersion();
    root->child->sibling = CreateAnimation(&c_obj);
    if(root->child->sibling)
    {
      root->child->sibling->sibling = CreateSkeletonName(skeleton_name);

      sprintf(out_file_name, "%s/%s.anm", DirName, FileName);
      WriteUTF(root, out_file_name);
    }
    FreeTree(root);
  }

  bl.Release();
  FreeCmpObject(&c_obj);
  FreeObject(&obj);
}

void GetSkeletonScale( CompoundObject *c_obj, AlDagNode *adjust_root)
{
  assert(adjust_root);

  // get root_adjust/skeleton scale
  double sx, sy, sz;
  adjust_root->scale(sx, sy, sz);
  if(sx != sy || sx != sz)
  {
    Winprint("Error: adust_root %s node has NON uniform sacle %.3f %.3f %.3f\n!",
      adjust_root->name(), sx, sy, sz);
  }
  c_obj->scale = (sx + sy + sz) / 3.0f;
}

void ExportBoneHP(const bone_lib *bl, CompoundObject *c_obj)
{
  assert(bl->count == c_obj->part_count);

  for(int i=0; i<bl->count; i++)
  {
    AlDagNode *child=bl->list[i].r_node->childNode();

    while(child)
    {
      if(child->asGroupNodePtr())
      {
        if(strstr(child->name(), "hp"))
        {
          object & obj = c_obj->lod_object_list[i].obj_list[0];

          DefineHP(&obj, bl->list[i].r_node /* obj node */,
                   child->asGroupNodePtr() /* hp node */); 
        }
      }
      child=child->nextNode();
    }
  }
}

int GetBoneID(const bone_lib *bl, const AlDagNode *node)
{
  for(int i=0; i<bl->count; i++)
  {
    if( !strcmp(bl->list[i].r_node->name(), node->name()) )
    {
      return i;
    }
  }

  return -1;
}

bool HasRotData( AlDagNode *node )
{
  AlChannel *ch = node->firstChannel();

  while( ch )
  {
    if( ch->type() != kChannelType )
    {
      Winprint("Error: channel of %s has a bad type %d!\n", 
               node->name(), ch->type());
      return false;
    }

    if( !ch->animatedItem()->asDagNodePtr() )
    {
      Winprint("Error: channel of %s is not animation a DAG node!\n", node->name());
      return false;
    }

    if( ch->channelType() == kAnimChannel) // kExprChannel
    { 
      int parameter = ch->parameter();

      switch( parameter )
      {
        case kFLD_DAGNODE_XROTATE:
        case kFLD_DAGNODE_YROTATE:
        case kFLD_DAGNODE_ZROTATE:
          return true;
      }
    }

    ch = node->nextChannel( ch );
  }

  return false;
}

bool HasTransData( AlDagNode *node )
{
  AlChannel *ch = node->firstChannel();

  while( ch )
  {
    if( ch->type() != kChannelType )
    {
      Winprint("Error: channel of %s has a bad type %d!\n", 
               node->name(), ch->type());
      return false;
    }

    if( !ch->animatedItem()->asDagNodePtr() )
    {
      Winprint("Error: channel of %s is not animation a DAG node!\n", node->name());
      return false;
    }

    if( ch->channelType() == kAnimChannel) // kExprChannel
    { 
      int parameter = ch->parameter();

      switch( parameter )
      {
        case kFLD_DAGNODE_XTRANSLATE:
        case kFLD_DAGNODE_YTRANSLATE:
        case kFLD_DAGNODE_ZTRANSLATE:
          return true;
      }
    }

    ch = node->nextChannel( ch );
  }

  return false;
}

void ExportBoneConnections(CompoundObject *c_obj, bone_lib *bl)
{
  FrameIn *fin = (FrameIn*)Malloc(bl->count * sizeof(FrameIn));
  for(int i = 0; i < bl->count; i++)
  {
    assert((i != 0 && bl->list[i].r_parent) || 
           (i == 0 && !bl->list[i].r_parent) );
         
    fin[i].r_node = bl->list[i].r_node;
    fin[i].t_node = bl->list[i].t_node;
    fin[i].r_parent = bl->list[i].r_parent;
    fin[i].t_parent = bl->list[i].t_parent;
    fin[i].n_frames = 0;
    fin[i].f_list = NULL;
  }

  if(utf_animation)
  {
    ReadFrames(bl->count, fin);
  }

  char object_name[256];
  strcpy(object_name, bl->list[0].r_node->name());
  if(strcmp(object_name, root_name))
  {
    strtok(object_name, "#"); // don't strip if root name
                              // "Root" will be used anyway
  }
 
  // used to compute bone extents and transform bone vertices in case of lod
  PersistTransform o_to_r;
  o_to_r.identity();
 
  InsertCompoundName(c_obj, object_name,
    (char*)bl->list[0].r_node->name(), root_name, ".3db", 2, &o_to_r, true); 

  SetTime(START_TIME);

  if( utf_animation &&
      !( remove_constant_channels && 
         !HasRotData(bl->list[0].r_node) &&
         !HasTransData(bl->list[0].t_node) )
    )
  {
    MakeRootAnim(c_obj, bl->list[0].r_node, fin[0].f_list, fin[0].n_frames); 
  }

  for(i = 1; i < bl->count; i++)
  {
    o_to_r = AlTM_to_PersistTransform( bl->list[0].inv_global.inverse() *
                                       bl->list[i].inv_global );

    strcpy(object_name, bl->list[i].r_node->name());
    strtok(object_name, "#"); // filter multiple skeleton versions
    InsertCompoundName(c_obj, object_name,
      (char*)bl->list[i].r_node->name(), root_name, ".3db", 2, &o_to_r, true); 

    if( !utf_animation || 
        ( remove_constant_channels &&
          !HasRotData( bl->list[i].r_node )
          //( ((AlAnimatable*)(bl->list[i].r_node))->firstChannel() == NULL) &&
          //( ((AlAnimatable*)(bl->list[i].t_node))->firstChannel() == NULL)
        ) 
      )
    {
      MakeSphereConnection(c_obj, bl->list[i].r_node, bl->list[i].t_node,
                           fin[i].f_list, fin[i].n_frames,
                           bl->list[i].r_parent, bl->list[i].t_parent);
    }
    else
    {
      MakeRevAnim(c_obj, bl->list[i].r_node, bl->list[i].t_node,
                  fin[i].f_list, fin[i].n_frames,
                  bl->list[i].r_parent, bl->list[i].t_parent);
    }
  }

  for(i = 0; i < bl->count; i++)
  {
    fin[i].Release();
  }
  Free(fin);
}

void ExportMerged(object *obj, AlDagNode *node, int level, double t,
                  AlGroupNode *r_to, AlGroupNode *t_to)
{
  if(node==NULL)
    return;

  SetTime(t);

  //printf("merging %s\n",node->name());
  if(node->asPolysetNodePtr())
  {
    defineMaterials(&(obj->ml), obj->atl, node); // adds textures as needed
    defineGeometry(obj, node, r_to, t_to, t);
  }

  // recurse
  // traverse across
  if(level > 0)
  {
    ExportMerged(obj, node->nextNode(), level, t, r_to, t_to);
  }

  // traverse down
  if(node->asGroupNodePtr())
  {
    ExportMerged(obj, node->asGroupNodePtr()->childNode(), level+1, t, r_to, t_to);
  }
}

AlDagNode* FindSkin(AlClusterNode *skin_ref_node)
{
  if(skin_ref_node==NULL) return NULL;

  AlCluster *cluster = skin_ref_node->cluster();
  assert(cluster);

  AlClusterMember *cl_member = cluster->firstMember();
  assert(cl_member); // fails if empty

  while(cl_member) // in case firstMember doesn't belomg to a alpolysetnode (it should)
  {
    AlObject *alobj = cl_member->object();
    assert(alobj);

    if(alobj->asPolysetVertexPtr())
    {
      AlPolysetVertex *pl_vtx = alobj->asPolysetVertexPtr();
      AlPolyset *alpolyset = pl_vtx->polyset();
      AlPolysetNode *alpolysetnode = alpolyset->polysetNode();
      AlDagNode *dag_node = alpolysetnode;
      AlDagNode *result = alpolysetnode;

      while(dag_node->parentNode())
      {
        dag_node = dag_node->parentNode();
        if(dag_node->asGroupNodePtr())
        {
          result = dag_node;
        }
      }

      return result;
    }

    cl_member = cl_member->nextClusterMember();
  }

  return NULL;
}

AlGroupNode* FindBoneRoot(AlClusterNode *skin_ref_node)
{
  if(skin_ref_node == NULL) return NULL;

  AlGroupNode *result=NULL;
  AlDagNode *dag_node=skin_ref_node;

  while(dag_node->parentNode()){
    dag_node=dag_node->parentNode();
    if(dag_node->asGroupNodePtr() &&
       strncmp(dag_node->name(), "adjust", 6) &&
       strncmp(dag_node->name(), "_t_", 3) )
    {
      result=dag_node->asGroupNodePtr();
    }
  }

  return result;
}

void TransformVertices(const bone_lib *bl, vertices *v, normals *n)
{
  bone_contrib **vertex_bone_matrix;
  int v_id;
  float sum;
  int i,j;

  vertex_bone_matrix=(bone_contrib**)Malloc(v->object_count * sizeof(bone_contrib*));
  vertex_bone_matrix[0]=(bone_contrib*)Malloc(v->object_count * bl->count *
                        sizeof(bone_contrib));
  for(i=1; i<v->object_count; i++)
  {
    vertex_bone_matrix[i]=vertex_bone_matrix[0] + i * bl->count;
  }
 
  // init matrix 
  for(i=0; i<v->object_count; i++)
  {
    for(j=0; j<bl->count; j++)
    {
      vertex_bone_matrix[i][j].relative=0.0f;
      //vertex_bone_matrix[i][j].node=NULL;
      vertex_bone_matrix[i][j].index=-1;
    }
  }

  // populate matrix
  for(j=0; j<bl->count; j++)
  {
    for(i=0; i<bl->list[j].vertex_count; i++)
    {
      v_id=bl->list[j].vertex_id_list[i];
      vertex_bone_matrix[v_id][j].relative=bl->list[j].vertex_weight_list[i];
      //vertex_bone_matrix[v_id][j].node=bl->list[j].r_node;
      vertex_bone_matrix[v_id][j].index=j;
    }
  }

  // cleanup contributions
  for(i=0; i<v->object_count; i++)
  {
    // sort by weight
    qsort(&(vertex_bone_matrix[i][0]), bl->count, sizeof(bone_contrib), 
          CompareElements);

    // normalize
    sum=0.0f;
    for(j=0; j<bl->count; j++)
    {
      sum+=vertex_bone_matrix[i][j].relative; 
    }
    if(sum>0.0f)
    {
      sum=1.0f/sum;
      for(j=0; j<bl->count; j++)
      {
        vertex_bone_matrix[i][j].relative*=sum;
      }
    }
    else // sanity check
    {
      fprintf(stderr,"Error: vertex %d has 0 sum weight.\n",i);
      exit(1);
    }

    // cutoff <20% of strongest bone
    for(j=0; j<bl->count; j++)
    {
      sum=0.0f;
      if(vertex_bone_matrix[i][j].relative < .2*vertex_bone_matrix[i][0].relative)
      {
        vertex_bone_matrix[i][j].relative=0.0f;
      }
      else
      {
        sum+=vertex_bone_matrix[i][j].relative;
      }
    }

    // renormalize
    if(sum>0.0)
    {
      sum=1.0f/sum;
      for(j=0; j<bl->count; j++)
      {
        vertex_bone_matrix[i][j].relative*=sum;
      }
    }
  }
 
  int b_id;
  int n_id;
  double tmp_v[4];
  int index = 0;

  AlTM root_bone_tm ( gf.DGX( bl->list[0].r_node, bl->list[0].t_node, START_TIME) );
  //AlTM root_bone_tm ( bl->list[0].inv_global.inverse() );

  // transform
  v->vertex_bone_count=(int*)Malloc(v->object_count * sizeof(int));
  v->first_vertex=(int*)Malloc(v->object_count * sizeof(int));
  for(i=0; i<v->object_count; i++)
  {
    v->first_vertex[i] = index;
    for(j=0; j<bl->count; j++)
    {
      if(vertex_bone_matrix[i][j].relative > 0.0f)
      {
        index++;
        v->bone_id_list=(int*)Realloc(v->bone_id_list, 
                               index * sizeof(int));
        v->bone_weight_list=(float*)Realloc(v->bone_weight_list, 
                               index * sizeof(float));
        v->bone_vertex_list=(float*)Realloc(v->bone_vertex_list, 
                               3 * index * sizeof(float));
        v->bone_normal_list=(float*)Realloc(v->bone_normal_list, 
                               3 * index * sizeof(float));

        // transform vertex into bone's local coords
        tmp_v[0]=v->object_list[3*i];
        tmp_v[1]=v->object_list[3*i+1];
        tmp_v[2]=v->object_list[3*i+2];
        tmp_v[3]=1.0;

        b_id=vertex_bone_matrix[i][j].index;
        root_bone_tm.transPoint(tmp_v);
        bl->list[b_id].inv_global.transPoint(tmp_v);

        v->bone_vertex_list[3*(index-1)]=tmp_v[0];
        v->bone_vertex_list[3*(index-1)+1]=tmp_v[1];
        v->bone_vertex_list[3*(index-1)+2]=tmp_v[2];

        // transform normal
        n_id=v->normal[i];
        tmp_v[0]=n->list[3*n_id];
        tmp_v[1]=n->list[3*n_id+1];
        tmp_v[2]=n->list[3*n_id+2];
        tmp_v[3]=1.0;

        root_bone_tm.transNormal(tmp_v[0], tmp_v[1], tmp_v[2]);
        bl->list[b_id].inv_global.transNormal(tmp_v[0], tmp_v[1], tmp_v[2]);
        
        v->bone_normal_list[3*(index-1)]=tmp_v[0];
        v->bone_normal_list[3*(index-1)+1]=tmp_v[1];
        v->bone_normal_list[3*(index-1)+2]=tmp_v[2];
        Normalize3(&(v->bone_normal_list[3*(index-1)]));

        // cross reference bone id to vertex
        v->bone_id_list[index-1]=b_id;

        // assign weight
        v->bone_weight_list[index-1]=vertex_bone_matrix[i][j].relative;
      }
      else
      {
        break;  // break out early since the matrix is sorted
      }
    }
    v->vertex_bone_count[i]=j;
  }

  Free(vertex_bone_matrix[0]);
  Free(vertex_bone_matrix);
}

// highest to lowest
int CompareElements(const void *pt1, const void *pt2)
{
  if( ((bone_contrib*)pt1)->relative < ((bone_contrib*)pt2)->relative ){
    return 1;
  }else
  if( ((bone_contrib*)pt1)->relative > ((bone_contrib*)pt2)->relative ){
    return -1;
  }
  return 0;
}

int AssignVertices(bone_lib *bl, const vertices *v, double t)
{
  SetTime(t);

  unsigned char *flag_list=(unsigned char*)Malloc(v->object_count*sizeof(unsigned char));
  memset(flag_list, 0, v->object_count*sizeof(unsigned char));

  AlTM tmg ( gf.DGX(bl->list[0].r_node, bl->list[0].t_node, t).inverse() );

  for(int i=0; i<bl->count; i++)
  {
    for(int j=0; j<bl->list[i].cluster_count; j++)
    {
      AlClusterNode *cluster_node=bl->list[i].cluster_list[j];
      AlCluster *cluster=cluster_node->cluster();
      AlClusterMember *cl_member=cluster->firstMember();

      while(cl_member)
      {
        AlObject *alobj=cl_member->object();

        AlPolysetVertex *pl_vtx=alobj->asPolysetVertexPtr();
        if(pl_vtx)
        {
          double tmp_v[4];
          float v_xyz[3];
          int v_id;

          float effect = pl_vtx->percentEffect(cluster);
          AlPolyset *alpolyset = pl_vtx->polyset();

          pl_vtx->worldPosition(tmp_v[0], tmp_v[1], tmp_v[2]); tmp_v[3]=1.0;

          // transform into root bone's coord
          tmg.transPoint(tmp_v);

          v_xyz[0]=(float)tmp_v[0];
          v_xyz[1]=(float)tmp_v[1];
          v_xyz[2]=(float)tmp_v[2];

// TODO: use api id's
          v_id=GetVertexID(v, v_xyz, 0, -1, 0);
          if(v_id < 0)  // sanity check
          {
            Winprint("ERROR: can't find vertex assignment!\n");
            return -1;
          }
          else
          {
            flag_list[v_id]=1;
            bl->list[i].vertex_count++;

            bl->list[i].vertex_id_list=(int*)Realloc(bl->list[i].vertex_id_list,
              bl->list[i].vertex_count*sizeof(int));
            bl->list[i].vertex_id_list[bl->list[i].vertex_count-1]=v_id; 

            bl->list[i].vertex_weight_list=
              (float*)Realloc(bl->list[i].vertex_weight_list,
              bl->list[i].vertex_count*sizeof(float));
            bl->list[i].vertex_weight_list[bl->list[i].vertex_count-1]=effect;
          }

          //printf("world %.3f %.3f %.3f id=%d\n",x,y,z,v_id);
        }
        cl_member=cl_member->nextClusterMember();
      }
    }
  }

  // check all vertices in mesh are assigned to a bone
  for(i=0; i<v->object_count; i++)
  {
    if(!flag_list[i])
    {
      Winprint("Error: vertex %d not assigned to any bone!\n",i);
      return -1;
    }
  }

  Free(flag_list);
  return 0;
}

bool IsBone(AlDagNode *aldagnode)
{
  if(aldagnode->asGroupNodePtr() &&
     aldagnode->joint() &&
     strncmp(aldagnode->name(), "adjust", 6) &&
     strncmp(aldagnode->name(), "_t_", 3) &&
     !strstr(aldagnode->name(), "hp") )
     // ((AlAnimatable*)aldagnode)->firstChannel() )
     // has animation; we don't care if it's a joint since all a joint has
     // are limits
  {
    return true;
  }
  else
  {
    return false;
  }
}

void AddCluster(bone *b, AlClusterNode *cluster)
{
  b->cluster_count++;
  b->cluster_list=(AlClusterNode**)Realloc(
     b->cluster_list, b->cluster_count * sizeof(AlClusterNode*));
  b->cluster_list[b->cluster_count-1] = cluster;
}

void AddBone(bone_lib *bl, AlGroupNode *node, int depth, AlGroupNode *parent)
{
    bl->count++;
    bl->list=(bone*)Realloc(bl->list, bl->count*sizeof(bone));
    bl->list[bl->count-1].Init();
    bl->list[bl->count-1].r_node =
    bl->list[bl->count-1].t_node = node;
    bl->list[bl->count-1].r_parent =
    bl->list[bl->count-1].t_parent = parent;
    bl->list[bl->count-1].depth = depth;
}

bool CheckBones(const bone_lib *bl)
{
   for(int i = 1; i < bl->count; i++)
   {
     if( GetBoneID(bl, bl->list[i].r_parent) < 0 )
     {
       Winprint("Error: parent %s of %s is not in bone_lib!\n",
         bl->list[i].r_parent->name(), bl->list[i].r_node->name());
       return false;
     }
   }

   return true;
}

void GetBones(AlDagNode *aldagnode, bone_lib *bl, int last_bone, double t)
{
  static int depth = 1; // heading bone is 0
  // if group & ! adjust add to list
  if(aldagnode == NULL) return;

  SetTime(t);

  // add cluster
  if(aldagnode->asClusterNodePtr())
  {
    assert(last_bone >= 0);
    AddCluster( &(bl->list[last_bone]), aldagnode->asClusterNodePtr() );
  }

  // add to bone lib
  if( IsBone(aldagnode) )
  {
    AddBone(bl, aldagnode->asGroupNodePtr(), depth,
            GetParentGroupAlNode(aldagnode));

    // traverse down
    if(aldagnode->asGroupNodePtr())
    {
      depth++;
      GetBones(aldagnode->asGroupNodePtr()->childNode(), bl, bl->count-1, t);
      depth--;
    }
  }
  else
  {
    // traverse down
    if(aldagnode->asGroupNodePtr())
    {
      depth++;
      GetBones(aldagnode->asGroupNodePtr()->childNode(), bl, last_bone, t);
      depth--;
    }
  }

  // traverse across
  GetBones(aldagnode->nextNode(), bl, last_bone, t);
}

void DF(const float f, const int i1)
{
  if(f > 0.0f)
  {
    printf(" %.*f ", i1, fabs(f));
  }
  else
  {
    printf("%.*f ", i1, f);
  }
}

void DumpPT(const PersistTransform & tm)
{
  DF(tm.m.e00, 3); DF(tm.m.e01, 3); DF(tm.m.e02, 3);
  printf("\n"); 
  DF(tm.m.e10, 3); DF(tm.m.e11, 3); DF(tm.m.e12, 3);
  printf("\n"); 
  DF(tm.m.e20, 3); DF(tm.m.e21, 3); DF(tm.m.e22, 3);
  printf("\n"); 
  DF(tm.v.x, 3); DF(tm.v.y, 3); DF(tm.v.z, 3);
  printf("\n"); 
  printf("\n"); 

  fflush(stdout);
}

void DumpAlTM(const AlTM& tm)
{
  DF(tm[0][0], 3); DF(tm[0][1], 3); DF(tm[0][1], 3); DF(tm[0][3], 3);
  printf("\n"); 
  DF(tm[1][0], 3); DF(tm[1][1], 3); DF(tm[1][1], 3); DF(tm[1][3], 3);
  printf("\n"); 
  DF(tm[2][0], 3); DF(tm[2][1], 3); DF(tm[2][1], 3); DF(tm[2][3], 3);
  printf("\n"); 
  DF(tm[3][0], 3); DF(tm[3][1], 3); DF(tm[3][1], 3); DF(tm[3][3], 3);
  printf("\n"); 
  printf("\n"); 

  fflush(stdout);
}

void DumpAlTMParts(const AlTM& tm)
{
  double translate[3], scale[3], rotate[3], shear[3];

  tm.decompose(translate, scale, rotate, shear);
  printf("trans %.3f %.3f %.3f\n",translate[0], translate[1], translate[2]); 
  printf("rotat %.3f %.3f %.3f\n",rotate[0], rotate[1], rotate[2]); 
  printf("scale %.3f %.3f %.3f\n",scale[0], scale[1], scale[2]); 
  printf("shear %.3f %.3f %.3f\n",shear[0], shear[1], shear[2]);
}
     
void MakeRootAnim(CompoundObject *c_obj, AlGroupNode *group, Frame *frame_list, 
                  int n_frames)
{
	for(int i=0;i<n_frames;i++)
        {
	  frame_list[i].vector_t.x *= (frame_list[i].step / c_obj->scale);
	  frame_list[i].vector_t.y *= (frame_list[i].step / c_obj->scale);
	  frame_list[i].vector_t.z *= (frame_list[i].step / c_obj->scale);
	}

	// Channel
	NamedChannel n_channel;
	InitNamedChannel(&n_channel);
	sprintf(n_channel.name, "Ch_%s_%s", FileName, group->name());
        if(exporting_deformable)
          strtok(n_channel.name, "#"); // strip anything after #

	n_channel.first_frame=frame_list[0].index;
	n_channel.last_frame=frame_list[n_frames-1].index;

	n_channel.channel.header.frames=n_frames; 
	n_channel.channel.header.capture_rate=(float)(1.0/fps); // fix later
	n_channel.channel.header.type=(PersistDT_VECTOR | PersistDT_QUATERNION);
	n_channel.channel.data=
	  (unsigned char*)Malloc(n_channel.channel.header.frames*
	                         (sizeof(PersistQuaternion)+sizeof(PersistVector)));
	for(i=0;i<n_frames;i++)
        {
	  *(PersistVector*)(n_channel.channel.data + (sizeof(PersistQuaternion) +
          sizeof(PersistVector)) * i)=
	    frame_list[i].vector_t;
	  *(PersistQuaternion*)(n_channel.channel.data + 
	  (sizeof(PersistQuaternion)+sizeof(PersistVector)) * i + 
          sizeof(PersistVector))=
	    frame_list[i].quat.v;
	}	

	// Script
	NamedScript script;
	InitScript(&script);
        sprintf(script.name, "Sc_%s", FileName);
	script.channel_count=1;
	script.channel_list=
          (PersistAnimChannelMapping*)Malloc(sizeof(PersistAnimChannelMapping));
	strcpy(script.channel_list[0].parent, ROOT_OBJ_NAME);
	script.channel_list[0].child[0]=0;
	strcpy(script.channel_list[0].channel, n_channel.name);

	// add data to cmp object
	InsertNamedChannel(c_obj, n_channel);
	InsertScript(c_obj, script);
}

void ExitCleanup(void)
{
        fflush(stdout);
        fflush(stderr);

        if(std_err && ftell(std_err) > 0)
        {
          Winprint("Please check std_err.txt for error information!");
        }
 
        if( std_out )
	{
          fclose(std_out);
          std_out = NULL;
        }

        if( std_err )
        {
          fclose(std_err);
          std_err = NULL;
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

  AlAnswerType ans = kOK; // kOK, kYes, kNo, kCancel

  // kOK_Cancel, kYes_No_Cancel, kOK_Only
  AlPromptBox(kOK_Cancel, buffer, &ans, -1, -1); 

  return ans;
}

PersistTransform AlTM_to_PersistTransform(const AlTM & atm)
{
  PersistTransform pt;

  // Alias uses row vectors but DA uses column vectors
  pt.m.e00 = atm[0][0];
  pt.m.e10 = atm[0][1];
  pt.m.e20 = atm[0][2];

  pt.m.e01 = atm[1][0];
  pt.m.e11 = atm[1][1];
  pt.m.e21 = atm[1][2];

  pt.m.e02 = atm[2][0];
  pt.m.e12 = atm[2][1];
  pt.m.e22 = atm[2][2];

  pt.v.x = atm[3][0];
  pt.v.y = atm[3][1];
  pt.v.z = atm[3][2];

  return pt;
}

Matrix AlTM_to_Matrix(const AlTM & r_tm)
{
  Matrix m;

  m.d[0][0] = r_tm[0][0];
  m.d[0][1] = r_tm[1][0];
  m.d[0][2] = r_tm[2][0];

  m.d[1][0] = r_tm[0][1];
  m.d[1][1] = r_tm[1][1];
  m.d[1][2] = r_tm[2][1];

  m.d[2][0] = r_tm[0][2];
  m.d[2][1] = r_tm[1][2];
  m.d[2][2] = r_tm[2][2];

  return m;
}

inline void SetTime(double t)
{
  if(t != AlUniverse::currentTime())
  {
    double start, end, step;
    AlUniverse::frameRange(kMinMax, start, end, step);
    if((t<start) || (t>end))
    {
      fprintf(stderr,"Error: setting to invalid time %f start=%f end=%f step=%f\n",
              t, start, end, step);
    }
    else
    {
      AlViewFrame::viewFrame(t);
    }
  }
}

inline void SetTime(double t, AlDagNode *node)
{
  assert(node);
  if(t != AlUniverse::currentTime())
  {
    double start, end, step;
    AlUniverse::frameRange(kMinMax, start, end, step);
    if((t<start) || (t>end))
    {
      fprintf(stderr,"Error: setting %s to invalid time %f start=%f end=%f step=%f\n",
              node->name(), t, start, end, step);
    }
    else
    {
      AlViewFrame::viewFrame(node, t, AlViewFrame::kObject);
      //AlViewFrame::viewFrame(node, t, AlViewFrame::kObjectAndAbove);
    }
  }
}

void GetBoneTMs(bone_lib *bl, double t)
{    
  SetTime(t);

  for(int i=0; i < bl->count; i++)
  {
    bl->list[i].inv_global = gf.DGX(bl->list[i].r_node, bl->list[i].t_node, t).inverse();
  }
}

bool IsPivotConsistent(AlDagNode *node, double t)
{
  SetTime(t, node);
  double p[3];
  double rpi[3];
  double rpo[3];
  double spi[3];
  double spo[3];

  node->rotatePivot(p[0], p[1], p[2]);
  node->rotatePivotIn (rpi[0], rpi[1], rpi[2]);
  node->rotatePivotOut(rpo[0], rpo[1], rpo[2]);
  node->scalePivotIn (spi[0], spi[1], spi[2]);
  node->scalePivotOut(spo[0], spo[1], spo[2]);

  bool result = true;
  if( fabs(p[0] - rpi[0]) > 0.001 ||
      fabs(p[1] - rpi[1]) > 0.001 ||
      fabs(p[2] - rpi[2]) > 0.001 )
  {
    fprintf(stderr, "Warning: node %s has inconsistent rotatePivotIn pivot!\n", 
            node->name());
    fprintf(stderr, "%.3f %.3f %.3f\n", p[0], p[1], p[2]);
    fprintf(stderr, "%.3f %.3f %.3f\n", rpi[0], rpi[1], rpi[2]);
    result = false;
  }

  if( fabs(p[0] - rpo[0]) > 0.001 ||
      fabs(p[1] - rpo[1]) > 0.001 ||
      fabs(p[2] - rpo[2]) > 0.001 )
  {
    fprintf(stderr, "Warning: node %s has inconsistent rotatePivotOut pivot!\n", 
            node->name());
    fprintf(stderr, "%.3f %.3f %.3f\n", p[0], p[1], p[2]);
    fprintf(stderr, "%.3f %.3f %.3f\n", rpo[0], rpo[1], rpo[2]);
    result = false;
  }

  if( fabs(p[0] - spi[0]) > 0.001 ||
      fabs(p[1] - spi[1]) > 0.001 ||
      fabs(p[2] - spi[2]) > 0.001 )
  {
    fprintf(stderr, "Warning: node %s has inconsistent scalePivotIn pivot!\n", 
            node->name());
    fprintf(stderr, "%.3f %.3f %.3f\n", p[0], p[1], p[2]);
    fprintf(stderr, "%.3f %.3f %.3f\n", spi[0], spi[1], spi[2]);
    result = false;
  }

  if( fabs(p[0] - spo[0]) > 0.001 ||
      fabs(p[1] - spo[1]) > 0.001 ||
      fabs(p[2] - spo[2]) > 0.001 )
  {
    fprintf(stderr, "Warning: node %s has inconsistent scalePivotOut pivot!\n", 
            node->name());
    fprintf(stderr, "%.3f %.3f %.3f\n", p[0], p[1], p[2]);
    fprintf(stderr, "%.3f %.3f %.3f\n", spo[0], spo[1], spo[2]);
    result = false;
  }

  return result;
}

AlTM BuildLocal18(AlDagNode *node, double t)
{
  SetTime(t);
  AlTM t1(1,1,1,1), t2(1,1,1,1), t3(1,1,1,1), t4(1,1,1,1), t5(1,1,1,1), 
       t6(1,1,1,1), t7(1,1,1,1), t8(1,1,1,1);
  AlTM result;
  double rotation[3];


  node->scalePivotIn(t1[3][0], t1[3][1], t1[3][2]);

  node->scale(t2[0][0], t2[1][1], t2[2][2]);

  node->scalePivotOut(t3[3][0], t3[3][1], t3[3][2]);

  node->rotatePivotIn(t4[3][0], t4[3][1], t4[3][2]);

  node->rotation(rotation[0], rotation[1], rotation[2]);

  t5=AlTM::rotateX(rotation[0]*D2R);
  t6=AlTM::rotateY(rotation[1]*D2R);
  t7=AlTM::rotateZ(rotation[2]*D2R);

  node->rotatePivotOut(t8[3][0], t8[3][1], t8[3][2]);

  result=t1*t2*t3*t4*t5*t6*t7*t8; 

  return result;
}

AlTM BuildLocal(AlDagNode *node, double t)
{
  SetTime(t);
  AlTM t9(1,1,1,1);
  node->translation(t9[3][0], t9[3][1], t9[3][2]);

  AlTM result = BuildLocal18(node, t) * t9;

  return result;
}

void DumpT9(AlDagNode *node, double t)
{
  double d[3];
       
  SetTime(t);
 
  node->scale(d[0], d[1], d[2]);
  printf("%.3f %.3f %.3f T2 scale\n",d[0], d[1], d[2]);

  node->rotation(d[0], d[1], d[2]);
  printf("%.3f %.3f %.3f T5 T6 T7 rotation\n",d[0], d[1], d[2]); 

  node->translation(d[0], d[1], d[2]);
  printf("%.3f %.3f %.3f T9 translation\n",d[0], d[1], d[2]);

  node->rotatePivotIn(d[0], d[1], d[2]);
  printf("%.3f %.3f %.3f T4 rotatePivotIn\n",d[0], d[1], d[2]);

  node->scalePivotIn(d[0], d[1], d[2]);
  printf("%.3f %.3f %.3f T1 scalePivotIn\n",d[0], d[1], d[2]);

  node->rotatePivotOut(d[0], d[1], d[2]);
  printf("%.3f %.3f %.3f T8 rotatePivotOut\n",d[0], d[1], d[2]);

  node->scalePivotOut(d[0], d[1], d[2]);
  printf("%.3f %.3f %.3f T3 scalePivotOut\n",d[0], d[1], d[2]);
}

void DumpPivots(bone_lib *bl)
{
  int i;
  double d[3];

  SetTime(-1);

  for(i=0; i<bl->count; i++)
  {
    if( bl->list[i].r_node->parentNode() && 
        !strncmp(bl->list[i].r_node->parentNode()->name(), "adjust", 6) &&
        !strncmp(bl->list[i].r_node->parentNode()->name(), "_t_", 3) )
    {
      printf("%s\n",bl->list[i].r_node->name());
      bl->list[i].r_node->rotatePivotIn(d[0], d[1], d[2]);
      printf("rotatePivotIn %f %f %f\n",d[0], d[1], d[2]);
      bl->list[i].r_node->rotatePivotOut(d[0], d[1], d[2]);
      printf("rotatePivotOut %f %f %f\n",d[0], d[1], d[2]);
      bl->list[i].r_node->rotatePivot(d[0], d[1], d[2]);
      printf("rotatePivot %f %f %f\n",d[0], d[1], d[2]);

      printf("%s\n",bl->list[i].r_node->parentNode()->name());
      bl->list[i].r_node->parentNode()->rotatePivotIn(d[0], d[1], d[2]);
      printf("rotatePivotIn %f %f %f\n",d[0], d[1], d[2]);
      bl->list[i].r_node->parentNode()->rotatePivotOut(d[0], d[1], d[2]);
      printf("rotatePivotOut %f %f %f\n",d[0], d[1], d[2]);
      bl->list[i].r_node->parentNode()->rotatePivot(d[0], d[1], d[2]);
      printf("rotatePivot%f %f %f\n",d[0], d[1], d[2]);
    }
  }
}
