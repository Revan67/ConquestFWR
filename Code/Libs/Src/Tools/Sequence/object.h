#ifndef OBJECT_H
#define OBJECT_H

#include "engine.h"

#include "Llist.h"
#include <IAnim.h>
#include "gametypes.h"
#include "extent.h"

//

// Always add new types to the end just above NUM_OBJ_TYPES to preserving backward compatible loading/saving
/*enum ObjectType 
{
	OT_INVALID_OBJ_TYPE = -1,
	OT_BODY,
	OT_BOMB,
	OT_STATIC_GEOMETRY,
	OT_CHARACTER,
	OT_GAME_OBJECT,
	OT_LIGHT,
	OT_EXPLOSION,
	OT_GUN,
	OT_NUM_OBJ_TYPES
};*/

extern char* ObjectTypeNames[];
extern char* ObjectTypePrettyNames[];

//

class ObjectArchetype;

class BaseObject : public IEngineInstance
{
	public:
		BaseObject(void) {};
		virtual ~BaseObject(void) {};

		virtual INSTANCE_INDEX get_index(void) const = 0;

		virtual const Vector & get_position(void) const = 0;
		virtual void set_position(const Vector & new_pos) = 0;

		virtual const Matrix & get_orientation(void) const = 0;
		virtual void set_orientation(const Matrix & new_orient) = 0;

		virtual void Render(struct ICamera * the_camera, U32 flags) = 0;
		virtual bool Update(float secs) = 0;

		virtual bool save_instance(IFileSystem* file) = 0;
		virtual bool load_instance(IFileSystem* file) = 0;

	// eventually this will need more information about what is has collided with.
		virtual void OnCollision(const Vector & N) = 0;

	// IEngineInstance methods follow:
		virtual void   COMAPI initialize_instance (INSTANCE_INDEX index) = 0;
		virtual void   COMAPI destroy_instance (INSTANCE_INDEX index) = 0;

		virtual void   COMAPI set_position(INSTANCE_INDEX index, const Vector & position) = 0;
		virtual const Vector & COMAPI get_position(INSTANCE_INDEX index) const = 0;

		virtual void   COMAPI set_orientation(INSTANCE_INDEX index, const Matrix & orientation) = 0;
		virtual const Matrix & COMAPI get_orientation(INSTANCE_INDEX index) const = 0;

		virtual void      COMAPI set_transform(INSTANCE_INDEX index, const Transform & transform) = 0;
		virtual const Transform & COMAPI get_transform(INSTANCE_INDEX index) const = 0;

		virtual const Vector & COMAPI get_velocity (INSTANCE_INDEX object) const = 0;
		virtual const Vector & COMAPI get_angular_velocity (INSTANCE_INDEX object) const = 0;

		virtual void COMAPI set_velocity (INSTANCE_INDEX object, const Vector & vel) = 0;
		virtual void COMAPI set_angular_velocity (INSTANCE_INDEX object, const Vector & ang) = 0;

		virtual void COMAPI set_centered_radius(INSTANCE_INDEX index, const float r, const Vector & c) = 0;
		virtual void COMAPI get_centered_radius(INSTANCE_INDEX index, float * r, Vector * c) const = 0;

		virtual const BaseExtent* get_extent() const = 0;

		virtual void GetUserData(void) = 0;

		virtual bool was_destroyed(void) const = 0;

		virtual ObjectType get_type(void) const;

		ObjectArchetype* archetype;

		char*			name;

		bool			editor_hidden;
		bool			editor_frozen;
		bool			designer_created;

		// The name of the hp on our parent..NULL if no parent
		char*			parent_hp_name;
		// The name of the hp on ourself that connects to our parent..NULL if no parent
		char*			hp_name;
};

//

class GameObject : public BaseObject //, public GameObjectData
{
	public:
		GameObject();
		virtual ~GameObject();

		virtual INSTANCE_INDEX get_index(void) const
		{
			return index;
		}

		virtual const Vector & get_position(void) const;
		virtual void set_position(const Vector & new_pos);
		virtual const Matrix & get_orientation(void) const;
		virtual void set_orientation(const Matrix& new_orient);

		virtual void COMAPI set_centered_radius(INSTANCE_INDEX index, const float r, const Vector & c)
		{
			radius = r;
		}

		virtual void COMAPI get_centered_radius(INSTANCE_INDEX index, float * r, Vector * c) const
		{
			*r = radius;
			c->zero();
		}

		virtual void Render(struct ICamera * the_camera, U32 flags);
		virtual bool Update(float secs);

		virtual void OnCollision(const Vector & N) {}
		virtual void GetUserData();

		virtual bool save_instance(IFileSystem* file);
		virtual bool load_instance(IFileSystem* file);

	//protected:
		INSTANCE_INDEX	index;
		Transform		m_transform;
		float			radius;
		int				current_state;
		bool			destroyed:1;

		virtual void COMAPI initialize_instance (INSTANCE_INDEX index);
		virtual void COMAPI destroy_instance (INSTANCE_INDEX index);

		virtual void COMAPI set_position(INSTANCE_INDEX index, const Vector & position);
		virtual const Vector & COMAPI get_position(INSTANCE_INDEX index) const;
		virtual void COMAPI set_orientation(INSTANCE_INDEX index, const Matrix & orientation);
		virtual const Matrix & COMAPI get_orientation(INSTANCE_INDEX index) const;
		virtual void COMAPI set_transform(INSTANCE_INDEX index, const Transform & transform);
		virtual const Transform & COMAPI get_transform(INSTANCE_INDEX index) const;
		virtual const Vector & COMAPI get_velocity (INSTANCE_INDEX object) const;
		virtual const Vector & COMAPI get_angular_velocity (INSTANCE_INDEX object) const;
		virtual void COMAPI set_velocity (INSTANCE_INDEX object, const Vector & vel) {}
		virtual void COMAPI set_angular_velocity (INSTANCE_INDEX object, const Vector & ang) {}

		virtual const BaseExtent* get_extent() const;

		virtual bool was_destroyed(void) const;
};

//

class ObjectArchetype {
	public:
		ObjectArchetype(const char* name);
		virtual ~ObjectArchetype();

		virtual void add_ref();
		virtual	void remove_ref();

		virtual BaseObject* create_instance() = 0;
		virtual	void delete_instance(BaseObject* obj) = 0;

	//protected:
		ObjectType type;
		char* m_name;

		int ref_count;
		bool delete_if_no_ref;

		unsigned int cur_instance_num;
};

class GameObjectArchetype : public ObjectArchetype {
	public:
		GameObjectArchetype(const char* name);
		~GameObjectArchetype();

		BaseObject* create_instance();
		void delete_instance(BaseObject* obj);

	protected:
		char* mesh_name;
		ARCHETYPE_INDEX arch_idx;
};

class StaticGeomArchetype : public ObjectArchetype {
	public:
		StaticGeomArchetype(const char* name);
		~StaticGeomArchetype();

		BaseObject* create_instance();
		void delete_instance(BaseObject* obj);

	protected:
		char* mesh_name;
		ARCHETYPE_INDEX arch_idx;
};

#endif OBJECT_H
