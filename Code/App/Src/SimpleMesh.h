#ifndef SIMPLEMESH_H
#define SIMPLEMESH_H

#include "Vector.h"

struct SVertex
{
	Vector pt;  //vertex
	Vector n;	//normal

	SVertex(void)
	{
		memset(this, 0, sizeof(*this));
	}
};

struct SFace
{
	Vector n;	//normal
	U16 v[3];	//index into vertex list
	
	SFace(void)
	{
		memset(this, 0, sizeof(*this));
	}
};

struct SEdge
{
	U16 v[2];
	U16 f[2];
};

struct SMesh
{
	U16 v_cnt,f_cnt,e_cnt;
	SVertex *v_list;
	SFace *f_list;
	U16 *f_edge_list;
	SEdge *e_list;
	
	SMesh(void)
	{
		memset(this, 0, sizeof(*this));
	}
	
	~SMesh(void)
	{
		free();
	}

	void free (void)
	{
		delete [] v_list;
		v_list = 0;
		delete [] f_list;
		f_list = 0;
		delete [] e_list;
		e_list = 0;
		delete [] f_edge_list;
		f_edge_list = 0;
	}

	void init(int _f,int _v)
	{
		f_list = new SFace[_f];
		f_cnt = _f;
		v_list = new SVertex[_v];
		v_cnt = _v;
	}

	void MakeEdges();

	int GetNeighborOnEdge(int f,int e);

	BOOL32 save(const char *fileName);

	BOOL32 load(const char *fileName);
	BOOL32 load(IFileSystem *file);

	void MakeCollisionMesh(struct CollisionMesh *mesh);

	void Sort(const Vector &n);
};


#endif