#ifndef GUN_H
#define GUN_H

//

#include "object.h"
#include "physobj.h"
#include "projectile.h"

//

class GunArchetype : public ObjectArchetype, public IProjectile
{
	public:
		GunArchetype(const char * name);
		virtual ~GunArchetype(void);

		BaseObject *	create_instance(void);
		void			delete_instance(BaseObject * obj);

		char *			mesh_name;
		ARCHETYPE_INDEX	arch_idx;

		char *			flash_name;
		ARCHETYPE_INDEX	flash_idx;

		int				ammo_count;
		float			firing_rate;
		float			muzzle_velocity;

		char *			fire_hp;
		Transform		fire_frame;


	// IProjectile functions.
		virtual void render_projectile(const class GameCamera * cam, const Projectile * p);
		virtual void collide_projectile(const Vector & p, const Vector & N);

};

//

class Gun : public PhysicalObject
{
	friend GunArchetype;

	protected:
		int		ammo_count;
		float	fire_countdown;
		Vector	fire_p, fire_d;

	public:
		inline GunArchetype * get_arch(void) const
		{
			return (GunArchetype *) archetype;
		}

		Gun(void);
		virtual ~Gun(void);

		virtual bool Update(float secs);
		//virtual void OnCollision(const Vector & N);

		bool save_instance(IFileSystem* file);
		bool load_instance(IFileSystem* file);

		bool fire(void);

		void get_fire_vector(Vector & start, Vector & dir);
};

//

#endif