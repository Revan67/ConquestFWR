#include "imagefilter.h"

void PixelFilter(const ImageFilter & fl, const float *in, float *out, const int in_width, const int in_height)
{
	//assert ( 0 == (in_width & 1) ); // even
	//assert ( 0 == (in_height & 1) );

	const int out_width  = in_width  / 2;
	const int out_height = in_height / 2;
	
	const int x_overlap = (fl.width  - 2) / 2;
	const int y_overlap = (fl.height - 2) / 2;

	const int out_x_border = (int)ceil((float)x_overlap / 2.0f);
	const int out_y_border = (int)ceil((float)y_overlap / 2.0f);

	const int fx_offset = 1 - (fl.width  / 2);
	const int fy_offset = 1 - (fl.height / 2);
	
	
	#define  IN(row, col)   in[((row) * in_width ) + (col)]
	#define OUT(row, col)  out[((row) * out_width) + (col)]

	#define  IN_P(row, col)  (in  + ((row) * in_width ) + (col))
	#define OUT_P(row, col)  (out + ((row) * out_width) + (col))

	// image center
	{
		for(int out_row = out_y_border; out_row < out_height - out_y_border; out_row++)
		{
			int in_row = 2 * out_row;
			for(int out_column = out_x_border; out_column < out_width - out_x_border; out_column++)
			{
				const int in_column = 2 * out_column;

				float * const op = OUT_P(out_row, out_column);
				float tmp_sum = 0.0f;
				for(int fr = 0; fr < fl.height; fr++)
				{
					const float * ip  = IN_P(in_row + fr+fy_offset, in_column + fx_offset);
					const float *flp = fl[fr];
					const float * const end = flp + fl.width - 3;
					for( ; flp < end; flp+=4, ip+=4)
					{
						tmp_sum +=  flp[0] * ip[0];
						tmp_sum +=  flp[1] * ip[1];
						tmp_sum +=  flp[2] * ip[2];
						tmp_sum +=  flp[3] * ip[3];
					}
					const float * const end2 = end + 3;
					for( ; flp < end2; flp++, ip++)
					{
						tmp_sum +=  flp[0] * ip[0];
					}
				}

				*op = tmp_sum;
			}
		}
	}

	ImageFilter flt;
	// top border
	{
		for(int out_row = 0; out_row < out_y_border; out_row++)
		{
			int in_row = 2 * out_row;

			assert(y_overlap - in_row < fl.height / 2);
			
			ImageFilter::Copy(fl, flt);
			// zero out unused parts of filter and renormalize
			for(int fy = 0; fy < y_overlap - in_row; fy++)
			{
				for(int fx=0; fx < flt.width; fx++)
				{
					flt[fy][fx] = FLT_MAX;
				}
			}
			flt.Normalize();
			
			for(int out_column = out_x_border; out_column < out_width - out_x_border; out_column++)
			{
				int in_column = 2 * out_column;

				OUT(out_row, out_column) = 0.0f;
				for(int fr = 0; fr < flt.height; fr++)
				{
					int iny = in_row + fr+fy_offset;
					if( iny >= 0 )
					{
						for(int fc = 0; fc < flt.width; fc++)
						{
							assert( flt[fr][fc] != FLT_MAX);
							OUT(out_row, out_column) += 
								( flt[fr][fc] * IN(iny, in_column + fc+fx_offset) );
						}
					}
				}
			}
		}
	}

	// bottom border
	{
		for(int out_row = out_height - out_y_border; out_row < out_height; out_row++)
		{
			int in_row = 2 * out_row;

			ImageFilter::Copy(fl, flt);
			
			for(int fy = flt.height - y_overlap + (in_height - 2 - in_row); fy < flt.height; fy++)
			{
				for(int fx=0; fx < flt.width; fx++)
				{
					flt[fy][fx] = FLT_MAX;
				}
			}
			flt.Normalize();
			
			for(int out_column = out_x_border; out_column < out_width - out_x_border; out_column++)
			{
				int in_column = 2 * out_column;

				OUT(out_row, out_column) = 0.0f;
				for(int fr = 0; fr < flt.height; fr++)
				{
					int iny = in_row + fr+fy_offset;
					if( iny < in_height)
					{
						for(int fc = 0; fc < flt.width; fc++)
						{
							assert(flt[fr][fc] != FLT_MAX);
							OUT(out_row, out_column) += 
								( flt[fr][fc] * IN(iny, in_column + fc+fx_offset) );
						}
					}
				}
			}
		}
	}
	

	// left border
	{
		for(int out_column = 0; out_column < out_x_border; out_column++)
		{
			int in_column = 2 * out_column;

			ImageFilter::Copy(fl, flt);
		
			// zero out unused parts of filter and renormalize
			for(int fy = 0; fy < flt.height; fy++)
			{
				for(int fx = 0; fx < x_overlap - in_column; fx++)
				{
					flt[fy][fx] = FLT_MAX;
				}
			}
			flt.Normalize();
			
			
			for(int out_row = out_y_border; out_row < out_height - out_y_border; out_row++)
			{
				int in_row = 2 * out_row;
				
				OUT(out_row, out_column) = 0.0f;
				for(int fr = 0; fr < flt.height; fr++)
				{
					for(int fc = 0; fc < flt.width; fc++)
					{
						int inx = in_column + fc+fx_offset;
						if( inx >= 0 )
						{
							assert( flt[fr][fc] != FLT_MAX);
							OUT(out_row, out_column) += 
								( flt[fr][fc] * IN(in_row + fr+fy_offset, inx) );
						}
					}
				}
			}
		}
	}

	// right border
	{
		for(int out_column = out_width - out_x_border; out_column < out_width; out_column++)
		{
			int in_column = 2 * out_column;

			ImageFilter::Copy(fl, flt);
			
			// zero out unused parts of filter and renormalize
			for(int fy = 0; fy < flt.height; fy++)
			{
				for(int fx = flt.width - x_overlap + (in_width - 2 - in_column); fx < flt.width; fx++)
				{
					flt[fy][fx] = FLT_MAX;
				}
			}
			flt.Normalize();
			
			for(int out_row = out_y_border; out_row < out_height - out_y_border; out_row++)
			{
				int in_row = 2 * out_row;
				
				OUT(out_row, out_column) = 0.0f;
				for(int fr = 0; fr < flt.height; fr++)
				{
					for(int fc = 0; fc < flt.width; fc++)
					{
						int inx = in_column + fc+fx_offset;
						if( inx < in_width )
						{
							assert( flt[fr][fc] != FLT_MAX);
							OUT(out_row, out_column) += 
								( flt[fr][fc] * IN(in_row + fr+fy_offset, inx) );
						}
					}
				}
			}
		}
	}

	// corners
	// top left
	{
		for(int out_row = 0; out_row < out_y_border; out_row++)
		{
			int in_row = 2 * out_row;

			for(int out_column = 0; out_column < out_x_border; out_column++)
			{
				int in_column = 2 * out_column;

				ImageFilter::Copy(fl, flt);
				
				// zero out unused parts of filter and renormalize
				{
                  // top
                  int fy, fx;
			      for(fy = 0; fy < y_overlap - in_row; fy++)
			      {
				      for(fx=0; fx < flt.width; fx++)
				      {
				        flt[fy][fx] = FLT_MAX;
				      }
			      }
                  // left
			      for(fy = 0; fy < flt.height; fy++)
			      {
				      for(fx = 0; fx < x_overlap - in_column; fx++)
				      {
				        flt[fy][fx] = FLT_MAX;
				      }
			      }
                }
				flt.Normalize();

				OUT(out_row, out_column) = 0.0f;
				for(int fr = 0; fr < flt.height; fr++)
				{
					int iny = in_row + fr+fy_offset;
					if( iny >= 0 )
					{
						for(int fc = 0; fc < flt.width; fc++)
						{
							int inx = in_column + fc+fx_offset;
							if( inx >= 0)
							{
								assert( flt[fr][fc] != FLT_MAX);
								OUT(out_row, out_column) += 
									( flt[fr][fc] * IN(iny, inx) );
							}
						}
					}
				}
			}
		}
	}

	// bottom left
	{
		for(int out_row = out_height - out_y_border; out_row < out_height; out_row++)
		{
			int in_row = 2 * out_row;

			for(int out_column = 0; out_column < out_x_border; out_column++)
			{
				int in_column = 2 * out_column;

				ImageFilter::Copy(fl, flt);
				
				// zero out unused parts of filter and renormalize
				{
                  // bottom
                  int fy, fx;
			      for(fy = flt.height - y_overlap + (in_height - 2 - in_row); fy < flt.height; fy++)
			      {
				      for(fx=0; fx < flt.width; fx++)
				      {
						flt[fy][fx] = FLT_MAX;
				      }
			      }
                  // left
			      for(fy = 0; fy < flt.height; fy++)
			      {
				      for(fx = 0; fx < x_overlap - in_column; fx++)
				      {
				        flt[fy][fx] = FLT_MAX;
				      }
			      }
                }
				flt.Normalize();


				OUT(out_row, out_column) = 0.0f;
				for(int fr = 0; fr < flt.height; fr++)
				{
					int iny = in_row + fr+fy_offset;
					if( iny < in_height )
					{
						for(int fc = 0; fc < flt.width; fc++)
						{
							int inx = in_column + fc+fx_offset;
							if( inx >= 0 )
							{
								assert( flt[fr][fc] != FLT_MAX);
								OUT(out_row, out_column) +=
									( flt[fr][fc] * IN(iny, inx) );
							}
						}
					}
				}
				
			}
		}
	}


	// top right
	{
		for(int out_row = 0; out_row < out_y_border; out_row++)
		{
			int in_row = 2 * out_row;

			for(int out_column = out_width - out_x_border; out_column < out_width; out_column++)
			{
				int in_column = 2 * out_column;

				ImageFilter::Copy(fl, flt);
				
				// zero out unused parts of filter and renormalize
				{
                  // top
                  int fy, fx;
			      for(fy = 0; fy < y_overlap - in_row; fy++)
			      {
				      for(fx=0; fx < flt.width; fx++)
				      {
				        flt[fy][fx] = FLT_MAX;
				      }
			      }
                  // right
			      for(fy = 0; fy < flt.height; fy++)
			      {
				      for(fx = flt.width - x_overlap + (in_width - 2 - in_column); fx < flt.width; fx++)
				      {
						flt[fy][fx] = FLT_MAX;
				      }
			      }
                }
				flt.Normalize();

				OUT(out_row, out_column) = 0.0f;
				for(int fr = 0; fr < flt.height; fr++)
				{
					int iny = in_row + fr+fy_offset;
					if( iny >= 0 )
					{
						for(int fc = 0; fc < flt.width; fc++)
						{
							int inx = in_column + fc+fx_offset;
							if( inx < in_width)
							{
								assert( flt[fr][fc] != FLT_MAX);
								OUT(out_row, out_column) += 
									( flt[fr][fc] * IN(iny, inx) );
							}
						}
					}
				}
			}
		}
	}


	// bottom right
	{
		for(int out_row = out_height - out_y_border; out_row < out_height; out_row++)
		{
			int in_row = 2 * out_row;

			for(int out_column = out_width - out_x_border; out_column < out_width; out_column++)
			{
				int in_column = 2 * out_column;

				ImageFilter::Copy(fl, flt);
				
				// zero out unused parts of filter and renormalize
				{
                  // bottom
                  int fy, fx;
			      for(fy = flt.height - y_overlap + (in_height - 2 - in_row); fy < flt.height; fy++)
			      {
				      for(fx=0; fx < flt.width; fx++)
				      {
						flt[fy][fx] = FLT_MAX;
				      }
			      }
                  // right
			      for(fy = 0; fy < flt.height; fy++)
			      {
				      for(fx = flt.width - x_overlap + (in_width - 2 - in_column); fx < flt.width; fx++)
				      {
						flt[fy][fx] = FLT_MAX;
				      }
			      }
                }
				flt.Normalize();

				OUT(out_row, out_column) = 0.0f;
				for(int fr = 0; fr < flt.height; fr++)
				{
					int iny = in_row + fr+fy_offset;
					if( iny < in_height )
					{
						for(int fc = 0; fc < flt.width; fc++)
						{
							int inx = in_column + fc+fx_offset;
							if( inx < in_width )
							{
								assert( flt[fr][fc] != FLT_MAX);
								OUT(out_row, out_column) +=
									( flt[fr][fc] * IN(iny, inx) );
							}
						}
					}
				}
				
			}
		}
	}

	#undef IN
	#undef OUT
	#undef IN_P
	#undef OUT_P
}

void PixelDouble(const float *in, float *out, const int in_width, const int in_height)
{
	const int out_width  = 2 * in_width;
	const int out_height = 2 * in_height;
#if 1
	/*
	float d1 = 1.0f / (float)sqrt(.25f*.25f + .25f*.25f);
	float d2 = 1.0f / (float)sqrt(.75f*.75f + .25f*.25f);
	float d3 = 1.0f / (float)sqrt(.75f*.75f + .75f*.75f);
	//d1 *= d1;
	//d2 *= d2;
	//d3 *= d3;
	float d_sum = 1.0f / (d1 + d2 + d2 + d3); // note d2 is used twice

	const float w1 = d1 * d_sum;
	const float w2 = d2 * d_sum;
	const float w3 = d3 * d_sum;
	*/

	// plain bilinear interpolation
	const float w1 = .75f * .75f;
	const float w2 = .75f * .25f;
	const float w3 = .25f * .25f;
#else
	const float w1 = 9.0 / 16.0; // 9 + 3 + 3 + 1 == 16
	const float w2 = 3.0 / 16.0;
	const float w3 = 1.0 / 16.0;
#endif

	#define  IN(row, col)   in[((row) * in_width ) + (col)]
	#define OUT(row, col)  out[((row) * out_width) + (col)]

	// center
	for(int in_row = 0; in_row < in_height-1; in_row++)
	{
		int out_row = 2 * in_row + 1;
		for(int in_col = 0; in_col < in_width-1; in_col++)
		{
			int out_col = 2 * in_col + 1;

			const float in1 = IN(in_row    , in_col    );
			const float in2 = IN(in_row    , in_col + 1);
			const float in3 = IN(in_row + 1, in_col    );
			const float in4 = IN(in_row + 1, in_col + 1);

			OUT(out_row, out_col) =
				w1 * in1 +
				w2 * in2 +
				w2 * in3 +
				w3 * in4;

			OUT(out_row, out_col+1) =
				w2 * in1 +
				w1 * in2 +
				w3 * in3 +
				w2 * in4;

			OUT(out_row+1, out_col) =
				w2 * in1 +
				w3 * in2 +
				w1 * in3 +
				w2 * in4;

			OUT(out_row+1, out_col+1) =
				w3 * in1 +
				w2 * in2 +
				w2 * in3 +
				w1 * in4;
		}
	}

	// edges
#if 0
	float d4 = 1.0f / (float)sqrt(.25f*.25f + .25f*.25f);
	float d5 = 1.0f / (float)sqrt(.75f*.75f + .25f*.25f);
	//d4 *= d4;
	//d5 *= d5;
	float d_sume = 1.0f / (d4 + d5);

	const float w4 = d4 * d_sume;
	const float w5 = d5 * d_sume;
#else
	const float w4 = .75f;
	const float w5 = .25f;
#endif

	// horizontal edges
	for(int in_col = 0; in_col < in_width-1; in_col++)
	{
		int out_col = 2 * in_col + 1;

		// top
		OUT(0, out_col) =
				w4 * IN(0, in_col    ) +
				w5 * IN(0, in_col + 1);
		OUT(0, out_col+1) =
				w5 * IN(0, in_col    ) +
				w4 * IN(0, in_col + 1);

		// bottom
		OUT(out_height-1, out_col) =
				w4 * IN(in_height-1, in_col    ) +
				w5 * IN(in_height-1, in_col + 1);
		OUT(out_height-1, out_col+1) =
				w5 * IN(in_height-1, in_col    ) +
				w4 * IN(in_height-1, in_col + 1);
	}

	// vertical edges
	for(in_row = 0; in_row < in_height-1; in_row++)
	{
		int out_row = 2 * in_row + 1;

		// left
		OUT(out_row, 0) =
				w4 * IN(in_row  , 0) +
				w5 * IN(in_row+1, 0);
		OUT(out_row+1, 0) =
				w5 * IN(in_row  , 0) +
				w4 * IN(in_row+1, 0);

		// right
		OUT(out_row, out_width-1) =
				w4 * IN(in_row  , in_width-1) +
				w5 * IN(in_row+1, in_width-1);

		OUT(out_row+1, out_width-1) =
				w5 * IN(in_row  , in_width-1) +
				w4 * IN(in_row+1, in_width-1);
	}

	// corners
	OUT(0, 0) = IN(0,0);
	OUT(0, out_width-1) = IN(0, in_width-1);
	OUT(out_height-1, 0) = IN(in_height-1, 0);
	OUT(out_height-1, out_width-1) = IN(in_height-1, in_width-1);
}

void RGBtoYIQ ( const int length, 
				const unsigned char *in_r, const unsigned char *in_g, const unsigned char *in_b, 
				float *out_y, float *out_i, float *out_q )
{
  for(int i = 0; i < length; i++)
  {
	  RGB_to_YIQ( in_r[i], in_g[i], in_b[i], out_y + i, out_i + i, out_q + i );
  }
}

void YIQtoRGB( const int length, 
				const float *in_y, const float *in_i, const float *in_q, 
				unsigned char *out_r, unsigned char *out_g, unsigned char *out_b )
{
  for(int i = 0; i < length; i++)
  {
     float tmp_y = in_y[i];
     if(tmp_y > 255.0f)
     {
       tmp_y = 255.0f;
     }else
     if(tmp_y < 0.0f)
     {
       tmp_y = 0.0f;
     }

	 float tmp_r, tmp_g, tmp_b;
	 YIQ_to_RGB( tmp_y, in_i[i], in_q[i], &tmp_r, &tmp_g, &tmp_b );
	 tmp_r += .49999f;
	 tmp_g += .49999f;
	 tmp_b += .49999f;

     if(tmp_r > 255.0f)
     {
       tmp_r = 255.0f;
     }else
     if(tmp_r < 0.0f)
     {
       tmp_r = 0.0f;
     }

     if(tmp_g > 255.0f)
     {
       tmp_g = 255.0f;
     }else
     if(tmp_g < 0.0f)
     {
       tmp_g = 0.0f;
     }

     if(tmp_b > 255.0f)
     {
       tmp_b = 255.0f;
     }else
     if(tmp_b < 0.0f)
     {
       tmp_b = 0.0f;
     }

     out_r[i] = (unsigned char)tmp_r;
     out_g[i] = (unsigned char)tmp_g;
     out_b[i] = (unsigned char)tmp_b;
  }
}
