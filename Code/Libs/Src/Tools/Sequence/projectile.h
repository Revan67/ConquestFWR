#ifndef PROJECTILE_H
#define PROJECTILE_H

//

#include "vector.h"

//

struct IProjectile
{
	virtual void render_projectile(const class GameCamera * cam, const struct Projectile * p) = 0;
	virtual void collide_projectile(const Vector & p, const Vector & N) = 0;
};

//

struct Projectile
{
	Vector			prev_pos;
	Vector			pos;
	Vector			vel;
	IProjectile *	proj;
	float			lifetime;
	bool			active;
	bool			kill;

	Projectile(void)
	{
		memset(this, 0, sizeof(*this));
	}
};

//

struct ProjectileManager
{
	int				max_projectiles;
	Projectile *	projectiles;

	int				num_projectiles;
	int				index;

	ProjectileManager(int max_p);
	~ProjectileManager(void);

	bool create(const Vector & p, const Vector & v, IProjectile * proj);

	void update(float dt);
	void render(const class GameCamera * cam);
};

//

#endif