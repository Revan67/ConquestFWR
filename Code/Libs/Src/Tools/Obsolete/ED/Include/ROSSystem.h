// Author: Shaival Varma
// --------------------------------------------------------------------------
#ifndef ROSSystem_h
#define ROSSystem_h
// --------------------------------------------------------------------------
#include "Typedefs.h"
#include "ROSDLL.h"
#include "SceneEvent.h"
// --------------------------------------------------------------------------
struct IEngine;
struct IDAComponent;
class Vector;
class Matrix;
// --------------------------------------------------------------------------
namespace ROSSystem
{
// --------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif
// --------------------------------------------------------------------------
typedef U32 PerformanceStyle;

static const PerformanceStyle	kPTOnce		= 0;	// Perform once and stop at end. Default.
static const PerformanceStyle	kPTRepeat	= 1;	// Perform to end and repeat from time 0
static const PerformanceStyle	kPTLoop		= 2;	// Perform to end and continue at or past beginning in order to preserve loop duration
// --------------------------------------------------------------------------
/* ROS system startup and shutdown */
// ROS startup. Returns true on success, or false on failure. Invoke this first.
CPP_DECL	bool __cdecl startup										(IDAComponent* system, IEngine* engine, HWND wndH);
typedef		bool (__cdecl *startup_func_type)							(IDAComponent* system, IEngine* engine, HWND wndH);

// ROS shutdown. Invoke this last.
CPP_DECL	void __cdecl shutdown										();
typedef		void (__cdecl *shutdown_func_type)							();
// --------------------------------------------------------------------------
const U32 kStringLength = 200;
// --------------------------------------------------------------------------
typedef C8	CharString[kStringLength];	// Null terminated
class Scene;
class SceneEntity;
// --------------------------------------------------------------------------
typedef void (__cdecl *Callback)(const Scene* scene, ROS::SceneEvent scene_event, const CharString entity_name, const CharString entity_category, const void** entity, void** entity_user_data, ROS::SceneEventFlag* flag);
// --------------------------------------------------------------------------
/* Scene startup and shutdown */
// Scene creation. Returns NULL on failure. Invoke this first.
CPP_DECL	const Scene*		__cdecl create										(Callback callback, void* scene_user_data);
typedef		const Scene*		(__cdecl *create_func_type)							(Callback callback, void* scene_user_data);
// Scene shutdown. Invoke this last.												
CPP_DECL	void				__cdecl destroy										(const Scene* scene);
typedef		void				(__cdecl *destroy_func_type)						(const Scene* scene);
																					
/* Scene file reading */															
// Reads scene file. Returns true on success; false otherwise
CPP_DECL	bool				__cdecl read										(const Scene* scene, const CharString scene_file_name);
typedef		bool				(__cdecl *read_func_type)							(const Scene* scene, const CharString scene_file_name);
																					
/* Accessors */																		
CPP_DECL	void				__cdecl set_performance_style						(const Scene* scene, PerformanceStyle style);
typedef		void				(__cdecl *set_performance_style_type)				(const Scene* scene, PerformanceStyle style);

CPP_DECL	PerformanceStyle	__cdecl get_performance_style						(const Scene* scene);
typedef		PerformanceStyle	(__cdecl *get_performance_style_type)				(const Scene* scene);

CPP_DECL	void				__cdecl set_use_initial_entity_state				(const Scene* scene, bool use);
typedef		void				(__cdecl *set_use_initial_entity_state_func_type)	(const Scene* scene, bool use);

CPP_DECL	void*				__cdecl get_user_data								(const Scene* scene);
typedef		void*				(__cdecl *get_user_data_func_type)					(const Scene* scene);
																					
CPP_DECL	void				__cdecl get_name									(const Scene* scene, CharString name);
typedef		void				(__cdecl *get_name_func_type)						(const Scene* scene, CharString name);
																					
CPP_DECL	void				__cdecl get_filename								(const Scene* scene, CharString filename);
typedef		void				(__cdecl *get_filename_func_type)					(const Scene* scene, CharString filename);
																					
CPP_DECL	SINGLE				__cdecl get_duration								(const Scene* scene);
typedef		SINGLE				(__cdecl *get_duration_func_type)					(const Scene* scene);
																					
CPP_DECL	SINGLE				__cdecl get_current_time							(const Scene* scene);
typedef		SINGLE				(__cdecl *get_current_time_func_type)				(const Scene* scene);
																					
CPP_DECL	bool				__cdecl is_playing									(const Scene* scene);
typedef		bool				(__cdecl *is_playing_func_type)						(const Scene* scene);
															
/* Mutatators */
CPP_DECL	bool				__cdecl is_using_initial_entity_state				(const Scene* scene);
typedef		bool				(__cdecl *is_using_initial_entity_state_func_type)	(const Scene* scene);

CPP_DECL	void				__cdecl set_user_data								(const Scene* scene, void* user_data);
typedef		void				(__cdecl *set_user_data_func_type)					(const Scene* scene, void* user_data);
																					
CPP_DECL	void				__cdecl set_duration								(const Scene* scene, SINGLE duration);
typedef		void				(__cdecl *set_duration_func_type)					(const Scene* scene, SINGLE duration);
																					
CPP_DECL	void				__cdecl set_current_time							(const Scene* scene, SINGLE time);
typedef		void				(__cdecl *set_current_time_func_type)				(const Scene* scene, SINGLE time);
																					
CPP_DECL	void				__cdecl play										(const Scene* scene);
typedef		void				(__cdecl *play_func_type)							(const Scene* scene);
																					
CPP_DECL	void				__cdecl update										(const Scene* scene, SINGLE time_delta);
typedef		void				(__cdecl *update_func_type)							(const Scene* scene, SINGLE time_delta);
																					
CPP_DECL	void				__cdecl pause										(const Scene* scene);
typedef		void				(__cdecl *pause_func_type)							(const Scene* scene);
																					
CPP_DECL	void				__cdecl save										(const Scene* scene, const CharString filename);
typedef		void				(__cdecl save_func_type)							(const Scene* scene, const CharString filename);

/* Entity Accessors */
CPP_DECL	void				__cdecl get_location								(const SceneEntity* scene_entity, Vector& location);
typedef		void				(__cdecl *get_location_func_type)					(const SceneEntity* scene_entity, Vector& location);

CPP_DECL	void				__cdecl get_orientation								(const SceneEntity* scene_entity, Matrix& orientation);
typedef		void				(__cdecl *get_orientation_func_type)				(const SceneEntity* scene_entity, Matrix& orientation);
// --------------------------------------------------------------------------
#ifdef __cplusplus
}   // extern "C"
#endif
//---------------------------------------------------------------------------
}
//---------------------------------------------------------------------------
#endif
