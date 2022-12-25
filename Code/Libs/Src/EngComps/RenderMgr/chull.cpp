//
//
//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdlib.h>
#include <memory.h>
#include <math.h>

#include "chull.h"

//

class Stack
{
protected:

	int	top;

public:

	int	list[256];

	Stack(void)
	{
		top = 0;
	}

	int num(void) const
	{
		return top;
	}

	int push(int i)
	{
		list[top++] = i;
		return top;
	}

	int pop(void)
	{
		list[--top] = -1;
		top = __max(top, 0);
		return top;
	}
};

//

void FindLowest(ViewPoint * pts, int n)
{
	ViewPoint * m = pts;
	ViewPoint * p = pts + 1;
	for (int i = 1; i < n; i++, p++)
	{
		if ((p->y < m->y) || ((p->y == m->y) && (p->x < m->x)))
		{
			m = p;
		}
	}

	ViewPoint temp = *m;
	*m = *pts;
	*pts = temp;
}

//

ViewPoint * Points = NULL;

//

int Area2(const ViewPoint * p1, const ViewPoint * p2, const ViewPoint * p3)
{
	return	p1->x * p2->y - p1->y * p2->x +
			p1->y * p3->x - p1->x * p3->y +
			p2->x * p3->y - p3->x * p2->y;
}

//

int Left(const ViewPoint * p1, const ViewPoint * p2, const ViewPoint * p3)
{
	return Area2(p1, p2, p3) > 0;
}

//

int _cdecl Compare(const void * v1, const void * v2)
{
	const ViewPoint * p1 = (ViewPoint *) v1;
	const ViewPoint * p2 = (ViewPoint *) v2;

	int result;

	int a = Area2(Points, p1, p2);

	if (a > 0)
	{
		result = -1;
	}
	else if (a < 0)
	{
		result = 1;
	}
	else
	{
		ViewPoint r1, r2;
		r1.x = p1->x - Points->x;
		r1.y = p1->y - Points->y;

		r2.x = p2->x - Points->x;
		r2.y = p2->y - Points->y;

		float l1 = (float) sqrt( (float)(r1.x * r1.x + r1.y * r1.y));
		float l2 = (float) sqrt((float)(r2.x * r2.x + r2.y * r2.y));

		result = (l1 < l2) ? -1 : (l1 > l2) ? 1 : 0;
	}

	return result;
}

//

int Graham(ViewPoint * hull, int n_hull, const ViewPoint * pts, int n)
{
	int result;

	Stack stack;
	stack.push(n - 1);
	int top = stack.push(0);

	int timeout = n * 3;
	int counter = 0;

	int i = 1;
	while (i < n)
	{
		const ViewPoint * p1 = pts + stack.list[top - 2];
		const ViewPoint * p2 = pts + stack.list[top - 1];
		const ViewPoint * p3 = pts + i;

		if (Left(p1, p2, p3))
		{
			top = stack.push(i);
			i++;
		}
		else
		{
			top = stack.pop();
		}

		counter++;
		if (counter >= timeout)
		{
			OutputDebugString("Infinite loop in ComputeConvexHull().\n");
			return 0;
		}
	}

	stack.pop();

	if (n_hull > stack.num())
	{
		ViewPoint * h = hull;
		for (int i = 1; i < stack.num(); i++, h++)
		{
			*h = pts[stack.list[i]];
		}

		*h = pts[stack.list[0]];

		result = stack.num();
	}
	else
	{
		result = 0;
	}

	return result;
}

//

bool PointInArray(const ViewPoint & pt, const ViewPoint * list, int n)
{
	bool result = false;

// Unfortunate linear search. Convert to binary search of sorted list as necessary.
	for (int i = 0; i < n; i++, list++)
	{
		if ((pt.x == list->x) && (pt.y == list->y))
		{
			result = true;
			break;
		}
	}

	return result;
}

//

static ViewPoint* Buffer = NULL;
static unsigned int BufLen = 0;

void FreeConvexHullScratchBuffer (void)
{
	if (Buffer)
	{
		::free (Buffer);
		Buffer = NULL;
	}

	BufLen = 0;
}

int ComputeConvexHull(ViewPoint * dst, int dst_len, const ViewPoint * src, int src_len)
{
	if (BufLen < (src_len * sizeof (ViewPoint)))
	{
		BufLen = src_len * sizeof (ViewPoint);
		Buffer = (ViewPoint*)realloc (Buffer, BufLen);
	}

	Points = Buffer;
	int plen = 0;

	ViewPoint * dp = Points;
	const ViewPoint * sp = src;
	for (int i = 0; i < src_len; i++, sp++)
	{
		if (!PointInArray(*sp, Points, plen))
		{
			dp[plen++] = *sp;
		}
	}

	int result;
	if (plen >= 3)
	{
		FindLowest(Points, plen);

		qsort((void *) &Points[1], plen - 1, sizeof(ViewPoint), Compare);

		result = Graham(dst, dst_len, Points, plen);
	}
	else
	{
		result = 0;
	}

	return result;
}

//