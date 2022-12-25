#ifndef FRICTION_H
#define FRICTION_H


#include "ode.h"

//

struct FrictionEquation : public IODE
{
	Vector	u;
	float	Wz;
	Matrix	K;
	float	mu;
	float	t;
	int		phase;
	bool	sticking;
	Vector	eta;
	Vector kx;
	Vector ky;
	Vector kz;

	FrictionEquation(void)
	{
		t = 0;
		phase = 0;
		sticking = false;
		Wz = 0;
	}

	void set_initial_u(const Vector & _u)
	{
		u = _u;
	}

	void set_initial_K(const Matrix & _K)
	{
		K = _K;

		kx.set(K.d[0][0], K.d[0][1], K.d[0][2]);
		ky.set(K.d[1][0], K.d[1][1], K.d[1][2]);
		kz.set(K.d[2][0], K.d[2][1], K.d[2][2]);
	}

	void set_friction_coefficient(float _mu)
	{
		mu = _mu;
	}

	bool check_sticking(void)
	{
		bool result;
		float tangential = sqrt(u.x * u.x + u.y * u.y);
		if (tangential < 1e-2)
		{
			result = sticking = true;
			u.x = u.y = 0;
		}
		else
		{
			result = false;
		}
		return result;
	}

	void phase1(void)
	{
		phase = 1;
	}
	void phase2(float uz)
	{
		phase = 2;
		u.z = uz;
	}

	virtual S32	get_y_length(void)
	{
		return (sticking) ? 0 : 3;
	}

	virtual void get_y(SINGLE * dst, SINGLE t)
	{
		if (!sticking)
		{
			switch (phase)
			{
				case 1:
					*(dst++) = u.x;
					*(dst++) = u.y;
					*(dst++) = Wz;
					break;
				case 2:
					*(dst++) = u.x;
					*(dst++) = u.y;
					*(dst++) = u.z;
					break;
			}
		}
	}

	virtual void get_dydt(SINGLE * dst, SINGLE * y, SINGLE time)
	{
		if (!sticking)
		{
			float ysave[3];
			get_y(ysave, t);
			set_y(y);

			float tangential = sqrt(u.x * u.x + u.y * u.y);

			if (tangential < 1e-5)
			{
				sticking = true;
			}
			else
			{
				float bottom = -mu / tangential;
				eta.x = u.x * bottom;
				eta.y = u.y * bottom;
				eta.z = 1.0;

				switch (phase)
				{
					case 1:
					{
						float coeff = 1.0 / dot_product(kz, eta);

						*(dst++) = coeff * dot_product(kx, eta);
						*(dst++) = coeff * dot_product(ky, eta);
						*(dst++) = coeff * u.z;
						break;
					}
					case 2:
					{
						float coeff = 1.0 / u.z;
						*(dst++) = coeff * dot_product(kx, eta);
						*(dst++) = coeff * dot_product(ky, eta);
						*(dst++) = coeff * dot_product(kz, eta);
						break;
					}
				}
			}
			set_y(ysave);
		}
	}

	virtual void set_y(SINGLE * src)
	{
		if (!sticking)
		{
			u.x = *(src++);
			u.y = *(src++);
			switch (phase)
			{
				case 1:
					Wz = *(src++);
					break;
				case 2:
					u.z = *(src++);
					break;
			}
		}
	}

	virtual SINGLE get_t(void) const
	{
		return t;
	}

	virtual void set_t(SINGLE time)
	{
		t = time;
	}
};

//

#endif
