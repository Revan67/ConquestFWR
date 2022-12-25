#ifndef BONE_H
#define BONE_H

#include <AlGroupNode.h>
#include <AlClusterNode.h>
#include <AlTM.h>

class bone
{
public:
  //AlGroupNode *node;
  //AlGroupNode *parent;

  AlGroupNode *r_node;
  AlGroupNode *r_parent;
  AlGroupNode *t_node;
  AlGroupNode *t_parent;

  AlTM inv_global;
  //AlTM tm0_inv;
  int cluster_count;
  AlClusterNode **cluster_list;
  int vertex_count;
  int *vertex_id_list;
  float *vertex_weight_list;
  Frame *frame_list;
  int frame_type;
  //double trans[3];
  int depth;

  void Init(void)
  {
    //node=NULL;
    //parent = NULL;
    r_node = NULL;
    r_parent = NULL;
    t_node = NULL;
    t_parent = NULL;
    cluster_count=0;
    cluster_list=NULL;
    vertex_count=0;
    vertex_id_list=NULL;
    vertex_weight_list=NULL;
    frame_list=NULL;
    frame_type=-1;
    //tm0_inv=AlTM::identity();
    //trans[0]=trans[1]=trans[2]=0.0;
    depth = -1;
  };
 
  void Release(void)
  {
    Free(cluster_list);
    Free(vertex_id_list);
    Free(vertex_weight_list);
    Free(frame_list);
  }

  bone() { Init(); };
  ~bone() { Release(); };
};

class bone_lib
{
public:
  int count;
  bone *list;

  void Init(void) { count=0; list=NULL; };
  void Release(void)
  {
    for(int i=0; i<count; i++){
      list[i].Release();
    }
    Free(list);
  }
};

class bone_contrib
{
public:
  float relative;
  //AlGroupNode *node;
  int index;

  void Init(void){
    relative=0.0f;
    index=-1;
    //node=NULL;
  };

  bone_contrib(){
    Init();
  };
};


#endif
