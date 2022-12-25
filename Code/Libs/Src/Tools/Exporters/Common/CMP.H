#ifndef _CMP_H
#define _CMP_H
#include "persistmath.h"
#include "persistcompound.h"
#include "channel.h"
#include "persistanim.h"
#include "channeleventtypes.h"
//#include "persistchannel.h"
//#include "persisthardpoint.h"
#include "3db.h"
#include "xform.h"

#ifndef D2R
#define D2R (M_PI/180.0)
#endif
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// const int MAXIMUM_PATH = PARTNAME_MAX;

enum {UNKNOWN=0, PIVOT, ANIM, ALIAS_JOINT, MAX_ANIM, CMPND, OPENFLAME,
      MULTILEVEL, RIGID, BONE, END5=INT_MAX};

const int NONE=0;
const int FFIXED=1;
const int PRISMATIC=2;
const int REVOLUTE=4;
const int SPHERICAL=8;
// const int PRISMATIC_SPHERICAL=16; // channel type
const int DOF=32;
const int LOOSE=64;
const int TRANSLATIONAL=128;
const int EVENT=256;
const int OPEN=512;

// typedef union
class QT
{
public:
  union{
    PersistQuaternion v; 
    SINGLE d[4];
  };

  void Init()
  {
    d[0]=1.0f;
    d[1]=d[2]=d[3]=0.0f; 
  };

  QT()
  {
    Init();
  };
};

class DofData
{
public:
  int type;
  char name[256];
  float pivot[3];

  // dof
  float min_t[3];
  float max_t[3];
  float min_r[3];
  float max_r[3];
  float axis_pt[3]; // gamegen only
  float plane_pt[3]; // gamegen only

  float axis[3];

  // animations
  float min_angle;
  float max_angle;
  float min_step;
  float max_step;

  void Dump(FILE *strm) const;
};

typedef struct
{
  char *name;
  char *parent_name;
  int pris_count;  // flag
  int rev_count;

  PersistVector vector_t; // i j k // axis
  PersistVector vector_r;


  // translate
  PersistVector from;
  PersistVector delta;

  float step;

  // rotate
  PersistVector center;
  PersistMatrix orient;

  float angle;
  QT quat;

  int index; // used for sorting frames
}Frame;

typedef struct
{
	char obj_name[PARTNAME_MAX];
	char file_name[PARTFILENAME_MAX];
	int index;
	bool export; // false for double hp connected bones
}NamedPart;

typedef struct : public Script
{
	char name[256];
}NamedScript;

typedef struct
{
  char file_name[256];

  int part_count;
  NamedPart *part_list;
  lod_object *lod_object_list;
  PersistTransform *transform_list; // part's world to local xform 
				    // (world is relative to root object)

//  txt_lib tl; // optional
//  anim_txt_lib atl;

  mtl_lib ml;

  int fix_count;
  Fix *fix_list;

  int pris_count;
  Pris *pris_list;

  int rev_count;
  Rev *rev_list;

  int sphere_count;
  PersistSphere *sphere_list;

  int trans_count;
  Trans *trans_list;

  int loose_count;
  Loose *loose_list;

  int n_channel_count;
  NamedChannel *n_channel_list;

  int script_count;
  NamedScript *script_list;
  //Script *script_list;

  PersistTransform root_transform;
  float scale;

  Extents extents;

}CompoundObject;

void Transform_mul(Transform & dest, const Transform & m1, const Transform & m2);
int GetCmpParent(const CompoundObject *c_obj, int id);
PersistTransform GetTransform(const CompoundObject *c_obj, const char *name);
PersistTransform GetWorldTransform(const CompoundObject *c_obj, int id);
//void ComputeCompoundExtents(CompoundObject *c_obj, float density, float mass, const char *name);
void ComputeBoneExtents(const object *obj, CompoundObject *c_obj, const float density);
void ComputeBoneExtentsNurbCV(const object *obj, CompoundObject *c_obj, const float density);
void ComputeBoneExtentsBezCV(const Bezier_mesh *b_mesh, CompoundObject *c_obj, const float density);
void TransformBoneVertices(object *obj, const CompoundObject *c_obj);
void CenterMass(CompoundObject *c_obj, object *d_obj);
file_node* CreateBoneExtents(const CompoundObject *c_obj);

int IsInRange(float angle, const Vector& axis, const Vector& min, const Vector& max);
float GetQuatDiffAngles(const PersistQuaternion & qt1, const PersistQuaternion & qt2,
				   float *rot_angle, float *axis_angle);
void quat_slerp(PersistQuaternion & dst, const PersistQuaternion & q1,
                const PersistQuaternion & q2, SINGLE t);
Matrix Matrix_mul_Matrix(const Matrix& m1, const Matrix& m2);
Vector Transform_vector(const Transform & t, const Vector & v);
void DumpMatrix(const Matrix& m);
void mat_to_qt(QT *dst, const Matrix *m);
void qt_to_mat(Matrix *dst, const QT *q);
void InitCompoundObject(CompoundObject *c_obj);
void InitNamedPart(NamedPart *part);
int GetChannelType(CompoundObject *c_obj, int id);
void ScaleCompoundObject(CompoundObject *c_obj, const float scale_factor);
void ScaleChannel(Channel *ch, float scale_factor, int type);
void InsertFixed(CompoundObject *c_obj, const Fix& fixed);
void InsertLoose(CompoundObject *c_obj, const Loose& loose);
void InsertRev(CompoundObject *c_obj, const Rev& revolute);
void InsertPris(CompoundObject *c_obj, const Pris& prismatic);
void InsertTrans(CompoundObject *c_obj, const Trans& trans);
void InsertSphere(CompoundObject *c_obj, const PersistSphere & sphere);
int InsertCompoundName(CompoundObject *c_obj, const char *name, const char *f_name, 
                       const char *root_name, const char *extenson, int obj_flag,
					   PersistTransform *pt, const bool export);
void InsertScript(CompoundObject *c_obj, const NamedScript& script);
void InsertNamedChannel(CompoundObject *c_obj, const NamedChannel& n_channel);
void InsertChannelEvent(NamedChannel & n_channel, float time,
channel_event type, const char *name);
//void WordAllignChannel(NamedChannel *n_channel);
void CheckNameLenght(char *name);
file_node* CreateSkeletonName(const char *name);
file_node* CreateCmpnd(CompoundObject *c_obj, int split_flag, int anim_flag,
			int txt_flag);
file_node* CreateAnimation(CompoundObject *c_obj);
file_node *CreateChannel( CompoundObject *c_obj, const char *name );
file_node* CreateParts(CompoundObject *c_obj);
void WriteCmpd(CompoundObject *c_obj, int split_flag, int txt_flag);
void FreeCmpObject(CompoundObject *c_obj);
void FreeNamedChannels(NamedChannel *list, int count);
file_node* CreateFix(CompoundObject *c_obj);
file_node* CreatePris(CompoundObject *c_obj);
file_node* CreateRev(CompoundObject *c_obj);
file_node* CreateSphere(CompoundObject *c_obj);
file_node* CreateLoose(CompoundObject *c_obj);
file_node* CreateTrans(CompoundObject *c_obj);
void InitFixed(Fix *fixed);
void InitPris(Pris *prismatic);
void InitRev(Rev *revolute);
void InitSphere(PersistSphere *sphere);
//void LoadCompoundObject(CompoundObject *c_obj, file_node *root);
void LoadCompoundNames(CompoundObject *c_obj, file_node *root);
void LoadCompoundFix(CompoundObject *c_obj, file_node *root);
void LoadCompoundPris(CompoundObject *c_obj, file_node *root);
void LoadCompoundRev(CompoundObject *c_obj, file_node *root);
void LoadCompoundSphere(CompoundObject *c_obj, file_node *root);
void LoadNChannels(CompoundObject *c_obj, file_node *root);
//void LoadScripts(CompoundObject *c_obj, file_node *root);
void PrintQuat(const float quat[]);
void PrintPersistVector(PersistVector *vec);
void PrintPersistMatrix(PersistMatrix *mtx);
void PrintRev(Rev *con);
void PrintSphere(PersistSphere *con);
void PrintPris(Pris *con);
void PrintFix(Fix *con);
void PrintScript(NamedScript *script, int id);
void PrintNChannel(CompoundObject *c_obj, NamedChannel *n_ch, int id);
int HasRotation(Fix *fixed);
int HasTranslation(Fix *fixed);
void InitFrame(Frame *frame);
void InitDofData(DofData *data);

//void RemoveUnusedTextures(CompoundObject *c_obj);
//bool IsUsedTexture(const CompoundObject *c_obj, const char *name);
//void RemoveTexture(txt_lib *tl, int id);

void InitChannel(Channel *channel);
void InitNamedChannel(NamedChannel *n_channel);
void InitHeader(Header *header);
void InitScript(NamedScript *script);
void SwapChannels(CompoundObject *c_obj, int id1, int id2);
void SwapScripts(CompoundObject *c_obj, int id1, int id2);
void RemoveChannel(CompoundObject *c_obj, int i);
void SubNamedChannel(const NamedChannel & n_channel, NamedChannel * sub_channel,
				const int first, const int last, bool root_flag);
//bool IsConstantChannel(CompoundObject *c_obj, int id);
//bool FirstFrameIsIdentity(CompoundObject *c_obj, int id);
//bool IsIdentityChannel(CompoundObject *c_obj, int id);

//void RemoveConstantChannels(CompoundObject *c_obj);
//void RemoveIdentityChannels(CompoundObject *c_obj);
//void ShortenConstantChannels(CompoundObject *c_obj);
void ShortenChannel(CompoundObject *c_obj, int id);
void ExtractKeyFrames(CompoundObject *c_obj);
bool IsLinear1D(const float a1, const float a2, const float a3, const float fraction,
				const float tolerance);
bool IsLinear3D(const float * const v1, const float * const v2, const float * const v3,
				const float fraction, const float tolerance);
bool IsLinearQuat(const PersistQuaternion * const q1, const PersistQuaternion * const q2,
				  const PersistQuaternion * const q3, const float fraction, const float tolerance);
void KeyFrameChannel(Channel *channel, int type);
void CombineChannels(CompoundObject *c_obj, int force_flag);
void SynchronizeScriptTiming(CompoundObject *c_obj);
void SequenceAnim(CompoundObject *c_obj, int *ch_id_list, int n_channels);
int CleanFrames(Frame *frame_list, int n_frames);
void NormalizeFrames(Frame *frame_list, int n_frames);
void RotatePersistVector( PersistVector *v, const Matrix& m);
void PreRotatePersistMatrix(PersistMatrix *pm, const Matrix& m);
void RotateTransFrames(Frame *frame_list, int n_frames, const Fix& fixed);
void RotateCmpd(CompoundObject *c_obj, const Matrix& m);

void RotateLooseChannel(unsigned char *data, int n_frames, const Matrix& m);
void RotateSphereChannel(unsigned char *data, int n_frames, const Matrix& m);
void RotateTransChannel(unsigned char *data, int n_frames, const Matrix& m);

void SetQuats(Frame *frame_list, int n_frames);
void DumpFrames(Frame *frame_list, int n_frames);
QT QTMul(const QT& q1, const QT& q2);
int AnimAxisMinMax(Frame* frame_list, int n_frames, float *axis, float *min,
                float *max, int type, float min_v[3], float max_v[3]);
#if 0
void DofAxisMinMax(DofData *dof_data, PersistVector *axis,
                   SINGLE *min, SINGLE *max, int type);
#endif
void CleanDofData(DofData *data);

int GetChId(CompoundObject *c_obj, char *ch_name);
//mtl* GetMtlIDbyTXT(const void *cmp_ptr, const char *txt_name);
void GetPostfix1(char *out, const char *in);
void GetPrefix2(char *out, const char *in);
void StripPostfix(char *buffer);
float NormAngle(float angle);

void LoadPrismatic(Pris *prismatic, Fix *fixed, DofData *dof_data, int type);
void LoadRevolute(Rev *revolute, Fix *fixed, DofData *dof_data, int type);
void LoadSphere(PersistSphere *sphere, Fix *fixed, DofData *dof_data, int type);
int GetNodeType(file_node *node);

#endif // _CMP_H
