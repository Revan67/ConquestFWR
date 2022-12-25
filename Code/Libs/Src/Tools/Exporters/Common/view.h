struct FLOAT
{
	float f[1];
};
struct NamedPart
{
	char obj_name[64];
	char filename[64];
};
struct PersistVector
{
	SINGLE x[1];
	SINGLE y[1];
	SINGLE z[1];
};

struct PersistMatrix
{
	SINGLE e00[1];
	SINGLE e01[1];
	SINGLE e02[1];
	SINGLE e10[1];
	SINGLE e11[1];
	SINGLE e12[1];
	SINGLE e20[1];
	SINGLE e21[1];
	SINGLE e22[1];
};

struct PersistHPSpot
{
	char name[64];
	float point[3];
	float orientation[9];
};

struct NamedPart;
{
	char obj_name[64];
	char filename[64];
};

struct Fix
{
	char parent[64];
	char child[64];
	PersistVector pos;
	PersistMatrix orient;
};

struct Fix2
{
  Fix f1;
  Fix f2;
};

struct Fix6
{
  Fix f1;
  Fix f2;
  Fix f3;
  Fix f4;
  Fix f5;
  Fix f6;
};

struct Pris
{
	char parent[64];
	char child[64];
	PersistVector parent_point;
	PersistVector child_point;
	PersistMatrix rel_orientation;
	PersistVector axis;
        float min[1];
        float max[1];
};

struct Rev
{
	char parent[64];
	char child[64];
	PersistVector parent_point;
	PersistVector child_point;
	PersistMatrix rel_orientation;
	PersistVector axis;
        float min[1]; // Radians
        float max[1];
};

struct Pris2
{
  Pris p1;
  Pris p2;
};

struct Pris3
{
  Pris p1;
  Pris p2;
  Pris p3;
};

struct Rev2
{
  Rev r1;
  Rev r2;
};

struct Rev4
{
  Rev r1;
  Rev r2;
  Rev r3;
  Rev r4;
};

struct ChannelHeader
{
        unsigned int data_size;
        unsigned int num_frames;
        float capture_rate[1];
        unsigned int type;
};
