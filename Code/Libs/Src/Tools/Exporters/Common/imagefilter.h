#ifndef IMAGEFILTER_H
#define IMAGEFILTER_H

#include <stdio.h>
#include <math.h>
#include <float.h>
#include "misc.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

inline float RGB_to_I( const float r, const float g, const float b )
{
	return 0.299f * r +  0.587f * g +  0.114f * b;
}

inline void RGB_to_YIQ(const float r, const float g, const float b, float *y, float *i, float *q)
{
	*y = 0.299f * r +  0.587f * g +  0.114f * b; //        0 to 255
	*i = 0.596f * r + -0.275f * g + -0.321f * b; // -151.980 to 151.98
	*q = 0.212f * r + -0.523f * g +  0.311f * b; // -133.365 to 133.365
}

inline void YIQ_to_RGB(const float y, const float i, const float q, float *r, float *g, float *b)
{
	*r = y +  0.955688060359f * i +  0.619858094458f * q;
	*g = y + -0.271581796944f * i + -0.646873816138f * q;
	*b = y + -1.108177326680f * i +  1.705064559920f * q; 
}

class ImageFilter;

void PixelFilter(const ImageFilter & fl, const float *in, float *out,
				 const int in_width, const int in_height);
void PixelDouble(const float *in, float *out, const int in_width, const int in_height);

void RGBtoYIQ ( const int length, 
				const unsigned char *in_r, const unsigned char *in_g, const unsigned char *in_b, 
				float *out_y, float *out_i, float *out_q );
void YIQtoRGB( const int length, 
				const float *in_y, const float *in_i, const float *in_q, 
				unsigned char *out_r, unsigned char *out_g, unsigned char *out_b );

class ImageFilter {
public:
	// it probably does not make much sense to use different width & height in most cases
	// (even for non square textures) maybe for special filtering effects
	int width;
	int height;

	float *values;

	ImageFilter(void)
	{
		memset(this, 0, sizeof(*this));
	}

	/*
	ImageFilter(const ImageFilter & in)
	{
		memcpy(this, &in, sizeof(*this));

		values = (float**)Malloc(height * sizeof(float*));
		values[0] = (float*)Malloc(height * width * sizeof(float));
		for(int i=1; i < height; i++)
		{
			values[i] = values[0] + i*width;
		}

		memcpy(values[0], in.values[0], width*height*sizeof(float));
	}
	*/

	ImageFilter(const int x, const int y)
	{
		memset(this, 0, sizeof(*this));
		Init(x, y);
	}

	ImageFilter(const int s)
	{
		memset(this, 0, sizeof(*this));
		Init(s, s);
	}

	~ImageFilter(void)
	{
		Release();
	}

	void Release(void)
	{
		if(values)
		{
			Free(values);
		}
		memset(this, 0, sizeof(*this));
	}

	inline float* operator [] (const int i)
	{
		return values + i * width;
	}

	inline const float* operator [] (const int i) const
	{
		return values + i * width;
	}

	bool SelectGoodSize(int image_w, int image_h)
	{
		bool result = false;

		int new_size = 10;

		if(new_size > image_w || new_size > image_h)  // can only use high order filter when it fits at least once
		{
			new_size = _MIN(image_w, image_h);
			if(new_size < 6)
				new_size = 2;  // size 4 is worse than 2
		}

		if(width != new_size || height != new_size)
		{
			Release();
			Init(new_size, new_size);
			result = true;
		}

		return result;
	}

	/*
	inline const ImageFilter & operator = (const ImageFilter & in)
	{
		if(values)
		{
			Free(values[0]);
			Free(values);
		}

		memcpy(this, &in, sizeof(*this));

		values = (float**)Malloc(height * sizeof(float*));
		values[0] = (float*)Malloc(height * width * sizeof(float));
		for(int i=1; i < height; i++)
		{
			values[i] = values[0] + i*width;
		}

		memcpy(values[0], in.values[0], width*height*sizeof(float));
		
		return *this;
	}
	*/

	static void Copy(const ImageFilter & in, ImageFilter & out)
	{
		if(in.width != out.width || in.height != out.height)
		{
			if(out.values)
			{
				Free(out.values);
			}

			out = in;
			out.values = (float*)Malloc(out.height * out.width * sizeof(float));
		}

		memcpy(out.values, in.values, out.width*out.height*sizeof(float));
	}

	void Init(const int x, const int y)
	{
		// just to be safe
		Release();
	
		assert( !(x & 0x1) );
		assert( !(y & 0x1) );

		width = x;
		height = y;
		values = (float*)Malloc(height * width * sizeof(float));

		float cx = (width - 1.0f) / 2.0f;
		float cy = (height - 1.0f) / 2.0f;
		// assuming square filter // texture can still be non square
		float radius = (float)sqrt( (cx + 1.0f) * (cx + 1.0f) + 0.5f * 0.5f);
		
		for(int row=0; row < height; row++)
		{
			float x_dist = (float)row - cx;

			for(int column=0; column < width; column++)
			{
				float y_dist = (float)column - cy;
				float dist = (float)sqrt(x_dist*x_dist + y_dist*y_dist);
				
				float sinc = (dist == 0.0f) ?
					1.0f : (float)sin(dist * 0.5f * (float)M_PI) / (dist * 0.5f * (float)M_PI);
				
				float window = (dist >= radius) ?
					0.0f : .5f * (1.0f + (float)cos((dist * M_PI) / radius ));
				
				values[row * width + column] = sinc * window;

				/*
				t = dist
			    if (fabs(t) > 0.5) return 0.0;
			    double window = 0.54 + 0.46*cos(2.0*PI*t);
			    double sinc = (t == 0.0) ? 1.0 : sin(PI*t) / (PI*t);
			    return window * sinc;
			    */
			}
		}

		Normalize();
	}

	void Normalize(void)
	{
		float sum = 0.0f;
		for(int row=0; row < height; row++)
		{
			for(int column=0; column < width; column++)
			{
				if(values[row * width + column] != FLT_MAX)
				{
					sum += values[row * width + column];
				}
			}
		}

		if(sum > 0.0f)
		{
			sum = 1.0f / sum;
			for(int row=0; row < height; row++)
			{
				for(int column=0; column < width; column++)
				{
					if(values[row * width + column] != FLT_MAX)
					{
						values[row * width + column] *= sum;
					}
				}
			}
		}
		else
		{
			Winprint("Error: Image filter values sum to 0.0!\n");
		}
	}

};

#endif
