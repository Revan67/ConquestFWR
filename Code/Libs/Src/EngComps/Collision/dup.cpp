//
// Remove duplicate vectors from a list, fill in remap table.
//

#include "3dmath.h"
#include "fdump.h"

//
// Returns number of unique vectors in list.
//
int RemapDuplicateVectors(int * remap, Vector * src, int n)
{
	ASSERT(remap);
	ASSERT(src);

// Locate duplicate vertices.
	memset(remap, 0xff, sizeof(int) * n);

// Assume they're all unique.
	int num_unique = n;

// Loop through all vertices, 
	Vector * v1 = src;
	for (int i = 0; i < n; i++, v1++)
	{
	// comparing each to all the remaining vertices.

		if (remap[i] == -1)	// don't count already remapped vectors multiple times.
		{
			Vector * v2 = v1 + 1;
			for (int j = i + 1; j < n; j++, v2++)
			{
				if (v1->equal(*v2, 1e-4))
				{
				// We've got a duplicate.
					int back = i;

				// Traverse duplicates back to first in case of multiple duplicates.
					while (remap[back] != -1)
					{
						back = remap[back];
					}

				// Record index that this vertex duplicates.
					remap[j] = back;

				// One less unique vertex...
					num_unique--;
				}
			}
		}
	}

	return num_unique;
}

//