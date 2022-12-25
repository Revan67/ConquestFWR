typedef unsigned short word;

#pragma warning(disable:4244)

//
//
//


void DrawLine(void * buffer, int x1, int y1, int x2, int y2, word color, unsigned int stride);

// Sutherland-Cohen line clip algorithm.

#define TOP		0x0001
#define BOTTOM	0x0002
#define RIGHT	0x0004
#define LEFT	0x0008

//

typedef unsigned int OutCode;

//

OutCode ComputeCode(int x, int y)
{
	OutCode code = 0;
	if (y > 479)
	{
		code |= BOTTOM;
	}
	else if (y < 0)
	{
		code |= TOP;
	}
	if (x > 639)
	{
		code |= RIGHT;
	}
	else if (x < 0)
	{
		code |= LEFT;
	}
	return code;
}


void ClipAndDrawLine(void * buffer, int x1, int y1, int x2, int y2, word color, unsigned int stride)
{
	bool accept = false;
	bool done = false;

	OutCode code1 = ComputeCode(x1, y1);
	OutCode code2 = ComputeCode(x2, y2);

	do
	{
		if (!(code1 | code2))
		{
			accept = true;
			done = true;
		}
		else if (code1 & code2)
		{
			done = true;
		}
		else
		{
			float x, y;
			OutCode out = code1 ? code1 : code2;

			if (out & TOP)
			{
				x = x1 + float(x2 - x1) * float(-y1) / float(y2 - y1);
				y = 0;
			}
			else if (out & BOTTOM)
			{
				x = x1 + float(x2 - x1) * float(479 - y1) / float(y2 - y1);
				y = 479;
			}
			else if (out & RIGHT)
			{
				x = 639;
				y = y1 + float(y2 - y1) * float(639 - x1) / float(x2 - x1);
			}
			else if (out & LEFT)
			{
				x = 0;
				y = y1 + float(y2 - y1) * float(-x1) / float(x2 - x1);
			}

			if (out == code1)
			{
				x1 = x;
				y1 = y;
				code1 = ComputeCode(x1, y1);
			}
			else
			{
				x2 = x;
				y2 = y;
				code2 = ComputeCode(x2, y2);
			}
		}

	} while (!done);

	if (accept)
	{
		DrawLine(buffer, x1, y1, x2, y2, color, stride);
	}
}

//

void DrawLine(void * buffer, int x1, int y1, int x2, int y2, word color, unsigned int stride)
{
	word * buff = (word *) buffer;

	stride /= sizeof (short);		// stride in words instead of bytes
	int dx = x2 - x1;
	int ax = ((dx > 0) ? dx : -dx) << 1;
	int sx = (dx > 0) ? 1 : -1;

	int dy = y2 - y1;
	int ay = ((dy > 0) ? dy : -dy) << 1;
	int sy = (dy > 0) ? 1 : -1;

	int x = x1;
	int y = y1;

	unsigned short * ptr = buff + x + y * stride;	// starting pixel.

	int x_inc = sx;
	int y_inc = sy * stride;

	if (ax > ay)
	{
		int d = ay - (ax >> 1);
		bool done = false;

		while (!done)
		{
			*ptr = color;
			if (x == x2)
			{
				done = true;
			}
			else
			{
				if (d >= 0)
				{
					y += sy;
					ptr += y_inc;
					d -= ax;
				}

				x += sx;
				ptr += x_inc;
				d += ay;
			}
		}
	}
	else
	{
		int d = ax - (ay >> 1);
		bool done = false;
		while (!done)
		{
			*ptr = color;
			if (y == y2)
			{
				done = true;
			}
			else
			{
				if (d >= 0)
				{
					x += sx;
					ptr += x_inc;
					d -= ay;
				}
				y += sy;
				ptr += y_inc;
				d += ax;
			}
		}
	}
}

//
