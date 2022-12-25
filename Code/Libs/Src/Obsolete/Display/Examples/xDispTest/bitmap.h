
struct Bitmap
{
	int width;
	int height;

	int bpp;

	int page_w;					// GL power of 2
	int page_h;

	char *pixels;

	int size;

	int num_colors;
	float palette_r[256];		// support OpenGL glPixelMap
	float palette_g[256];
	float palette_b[256];

	unsigned char color_table[3*256];

	int get_id (void)
	{
		return (int)pixels;		// unique identifier
	}

	Bitmap (void)
	{
		pixels = 0;
	}

	~Bitmap (void)
	{
		free();
	}

	void free (void)
	{
		if (pixels)
		{
			::free(pixels);
			pixels = 0;
		}
	}

	float x_ratio (void) const
	{
		return float(width)/page_w;
	}
	float y_ratio (void) const
	{
		return float(height)/page_h;
	}

	int line_size (void) const
	{
		return (page_w*bpp+7)/8;
	}

	int page_size (int d)
	// OpenGL wants 2^N size textures!
	{
		int shift;
		for (shift=6; d > (1<<shift); )
			shift++;
		return (1<<shift);
	}

	char *alloc (int w, int h, int b)
	{
		free();

		bpp = b;
		width = w;
		height = h;

		page_w = page_size(w);
		page_h = page_size(h);

		size = page_h * line_size();

		pixels = (char *)malloc(size);

		return pixels;
	}

	bool load (const char *filename);

	void set_rgb_palette (unsigned char *rgb, int colors)	// RGB
	{
		num_colors = colors;
		assert(num_colors <= 256);
		for (int i=0; i<num_colors; i++)
		{
			palette_r[i] = float(rgb[i*3+0])/255.0F;
			palette_g[i] = float(rgb[i*3+1])/255.0F;
			palette_b[i] = float(rgb[i*3+2])/255.0F;

			int c = i*3;
			color_table[c+0] = rgb[c+0];
			color_table[c+1] = rgb[c+1];
			color_table[c+2] = rgb[c+2];
		}
	}
};

