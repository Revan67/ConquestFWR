// IObjectDatabase
//
//
//

#ifndef IObjectDatabase_H
#define IObjectDatabase_H


#include "DACOM.h"
//#include "3dmath.h"
#include "DACOM_Utility.h"

static const char *IID_IObjectDatabase = "IObjectDatabase";

//

enum IDB_OBJECTTABLETYPE
{
	IDB_TABLE_OBJECTS	= 1,
	IDB_TABLE_VISUALS	= 2,
	IDB_TABLE_CAMERAS	= 3,
	IDB_TABLE_LIGHTS	= 4,
	IDB_TABLE_USER		= 5,

	// Leave me last
	IDB_TABLE_MAX
};

//

enum IDB_RANGETYPE
{
	IDB_RT_BOX		= 1,
	IDB_RT_SPHERE	= 2,
	IDB_RT_PLANES	= 3
};

//

#define IDB_RF_OBJECT_SPACE	(1<<0)
#define IDB_RF_SCREEN_SPACE	(1<<1)

//

#define IDB_RANGE_MAX_PLANE_EQUATIONS 8


struct IDB_RANGE
{
	IDB_RANGETYPE range_type;
	U32			  rf_flags;		// IDB_RF_ flags
	union IDB_RANGEDATA 
	{
		struct IDB_RANGEDATA_BOX 
		{
			float min[3];
			float max[3];
		} box;

		struct IDB_RANGEDATA_SPHERE 
		{
			float center[3];
			float radius;
		} sphere;

		struct IDB_RANGEDATA_PLANES
		{
			float equations[IDB_RANGE_MAX_PLANE_EQUATIONS][4];	// Ax + By + Cz + D
			U32   num_equations;
		} planes;
	} range_data;
};

//

dacom_interface( IObjectDatabase )
{
	DACOM_INTERFACE_METHOD( ImportData,			( IDAComponent *data ));
												
	DACOM_INTERFACE_METHOD( InsertObject,		( IDAComponent *object ));
	DACOM_INTERFACE_METHOD( UpdateObject,		( IDAComponent *object ));
	DACOM_INTERFACE_METHOD( DeleteObject,		( IDAComponent *object ));

	DACOM_INTERFACE_METHOD( ExecuteListQuery,	( IDB_OBJECTTABLETYPE table ));
	DACOM_INTERFACE_METHOD( ExecuteNameQuery,	( IDB_OBJECTTABLETYPE table,  const char *name ));
	DACOM_INTERFACE_METHOD( ExecuteRangeQuery,	( IDB_OBJECTTABLETYPE table, IDB_RANGE &range ));
	DACOM_INTERFACE_METHOD( CloneResults,		( IDAComponent **out_results ));
	
	DACOM_INTERFACE_METHOD( GetCount,	( U32 *out_item_count ));
	DACOM_INTERFACE_METHOD( Reset,		( void ));
	DACOM_INTERFACE_METHOD( Item,		( U32 item, IDAComponent **out_item ));
	DACOM_INTERFACE_METHOD( Next,		( IDAComponent **out_item ));

};

#endif  //  EOF

