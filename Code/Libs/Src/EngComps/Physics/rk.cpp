//
//
//

#include <math.h>
#include <stdlib.h>

#include "rk.h"
#include "ode.h"
#include "fdump.h"
#include "Tfuncs.h"

//
// Allow warning-free conversion from 'const double' to float.
//
#pragma warning(disable : 4244 4305)

//

float a2, a3, a4, a5, a6;
float b21;
float b31, b32;
float b41, b42, b43;
float b51, b52, b53, b54;
float b61, b62, b63, b64, b65;

float c1, c2, c3, c4, c5, c6;
float cs1, cs2, cs3, cs4, cs5, cs6;
float cd1, cd2, cd3, cd4, cd5, cd6;

int		state_len;
float *	state;
float *	k1;
float *	k2;
float *	k3;
float *	k4;
float *	k5;
float *	k6;
float *	error;
float *	work;
float *	work2;

//

static void alloc_buffers(void)
{
	state	= new float[state_len];
	k1		= new float[state_len];
	k2		= new float[state_len];
	k3		= new float[state_len];
	k4		= new float[state_len];
	k5		= new float[state_len];
	k6		= new float[state_len];
	error	= new float[state_len];
	work	= new float[state_len];
	work2	= new float[state_len];
}

//

static void free_buffers(void)
{
	delete [] state;
	state = NULL;
	delete [] k1;
	k1 = NULL;
	delete [] k2;
	k2 = NULL;
	delete [] k3;
	k3 = NULL;
	delete [] k4;
	k4 = NULL;
	delete [] k5;
	k5 = NULL;
	delete [] k6;
	k6 = NULL;
	delete [] error;
	error = NULL;
	delete [] work;
	work = NULL;
	delete [] work2;
	work2 = NULL;
}

//

void RKInit(void)
{
	state_len	= 0;
	state		=
	k1			=
	k2			=
	k3			=
	k4			=
	k5			=
	k6			=
	error		=
	work		= 
	work2		= NULL;

	a2 = 1.0f/5.0f;
	a3 = 3.0f/10.0f;
	a4 = 3.0f/5.0f;
	a5 = 1.0f;
	a6 = 7.0f/8.0f;

	b21 =  1.0f/5.0f;
	b31 =  3.0f/40.0f;
	b32 =  9.0f/40.0f;
	b41 =  3.0f/10.0f;
	b42 = -9.0f/10.0f;
	b43 =  6.0f/5.0f;
	b51 = -11.0f/54.0f;
	b52 =  5.0f/2.0f;
	b53 = -70.0f/27.0f;
	b54 =  35.0f/27.0f;
	b61 =  1631.0f/55296.0f;
	b62 =  175.0f/512.0f;
	b63 =  575.0f/13824.0f;
	b64 =  44275.0f/110592.0f;
	b65 =  253.0f/4096.0f;

	c1 = 37.0f/378.0f;
	c2 = 0.0f;
	c3 = 250.0f/621.0f;
	c4 = 125.0f/594.0f;
	c5 = 0.0f;
	c6 = 512.0f/1771.0f;

	cs1 = 2825.0f/27648.0f;
	cs2 = 0.0f;
	cs3 = 18575.0f/48384.0f;
	cs4 = 13525.0f/55296.0f;
	cs5 = 277.0f/14336.0f;
	cs6 = 1.0f/4.0f;

	cd1 = c1 - cs1;
	cd2 = c2 - cs2;
	cd3 = c3 - cs3;
	cd4 = c4 - cs4;
	cd5 = c5 - cs5;
	cd6 = c6 - cs6;
}

//

void RKShutdown(void)
{
	free_buffers();
}

//

static void ScaleVector(float * vec, int len, float scale)
{
	ASSERT(!(len && (!vec)));	//len is nonzero and vec ! null

	float * s = vec;
	for (int i = 0; i < len; i++, s++)
	{
		*s *= scale;
	}
}

//

void RKStep(IODE * eq, float h, float & h_used, float & hnext, float tol)
{
	ASSERT(eq);	//everquest?	ASSERT(eq->get_played_time() < MAX_FLT);

	float time = eq->get_t();

	int len = eq->get_y_length();
	if (len)
	{
	// If we don't have enough room, start over.
		if (state_len < len)
		{
			free_buffers();
		}

		if (!state)
		{
			state_len = len;
			alloc_buffers();
		}

		float max_error;
		bool done = false;
		while (!done)
		{
		//
		// k1 = h * f(yn, tn);
		//
			eq->get_y(state, time);
			eq->get_dydt(k1, state, time);
			ScaleVector(k1, len, h);

		//
		//k2 = h * f(yn + b21 * k1, tn + a2);
		//
			for (int i = 0; i < len; i++)
			{
				work[i] = state[i] + b21 * k1[i];
			}
			eq->get_dydt(k2, work, time + a2);
			ScaleVector(k2, len, h);

		//
		// k3 = h * f(yn + b31 * k1 + b32 * k2, tn + a3);
		//
			for (i = 0; i < len; i++)
			{
				work[i] = state[i] + b31 * k1[i] + b32 * k2[i];
			}
			eq->get_dydt(k3, work, time + a3);
			ScaleVector(k3, len, h);

		//
		// k4 = h * f(yn + b41 * k1 + b42 * k2 + b43 * k3, tn + a4);
		//
			for (i = 0; i < len; i++)
			{
				work[i] = state[i] + b41 * k1[i] + b42 * k2[i] + b43 * k3[i];
			}
			eq->get_dydt(k4, work, time + a4);
			ScaleVector(k4, len, h);

		//
		// k5 = h * f(yn + b51 * k1 + b52 * k2 + b53 * k3 + b54 * k4, tn + a5);
		//
			for (i = 0; i < len; i++)
			{
				work[i] = state[i] + b51 * k1[i] + b52 * k2[i] + b53 * k3[i] + b54 * k4[i];
			}
			eq->get_dydt(k5, work, time + a5);
			ScaleVector(k5, len, h);

		//
		// k6 = h * f(yn + b61 * k1 + b62 * k2 + b63 * k3 + b64 * k4 + b65 * k5, tn + a6);
		//
			for (i = 0; i < len; i++)
			{
				work[i] = state[i] + b61 * k1[i] + b62 * k2[i] + b63 * k3[i] + b64 * k4[i] + b65 * k5[i];
			}
			eq->get_dydt(k6, work, time + a6);
			ScaleVector(k6, len, h);

		//
		// Take 5th-order step:
		//
			for (i = 0; i < len; i++)
			{
				work[i] = state[i] + c1 * k1[i] + c2 * k2[i] + c3 * k3[i] + c4 * k4[i] + c5 * k5[i] + c6 * k6[i];
			}

		//
		// Take 4th-order step:
		//
			for (i = 0; i < len; i++)
			{
				work2[i] = state[i] + cs1 * k1[i] + cs2 * k2[i] + cs3 * k3[i] + cs4 * k4[i] + cs5 * k5[i] + cs6 * k6[i];
			}

			max_error = 0;
			for (i = 0; i < len; i++)
			{
				error[i] = cd1 * k1[i] + cd2 * k2[i] + cd3 * k3[i] + cd4 * k4[i] + cd5 * k5[i] + cd6 * k6[i];
				float fe = fabs(error[i]) * state[i];
			// ¿¿¿tolerance???
				fe /= tol;
				if (fe > max_error)
				{
					max_error = fe;
				}
			}

			if (max_error <= 1.0)
			{
				h_used = h;
				eq->set_y(work);
				done = true;
			}
			else
			{
			//
			// Need to redo step with smaller timestep.
			//
				float htemp = 0.9f * h * pow(max_error, -0.25f);
				h = (h >= 0) ? Tmax(htemp, 0.1f * h) : Tmin(htemp, 0.1f * h);
			}
		}

		hnext = (max_error > 1.89e-4) ? 0.9f * h * pow(max_error, -0.2f) : h * 5.0f;
	}

// Update time.
	time += h;
	eq->set_t(time);
}

//

