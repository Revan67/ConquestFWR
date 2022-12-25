//
//
//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "spatial.h"
#include "Debugprint.h"

//
// Matrix inversion code shamelessly stolen from Numerical Recipes in C.
//

#define SWAP(a,b) { float temp=(a);(a)=(b);(b)=temp; }

static int temp1[6];
static int temp2[6];
static int temp3[6];


//

SpatialMatrix SpatialMatrix::get_inverse(void) const
{
	SpatialMatrix result = *this;
	SpatialMatrix id;
	id.set_identity();

	int *indxc,*indxr,*ipiv;
	int i,icol,irow,j,k,l,ll;
	float big,dum,pivinv;

	indxc = temp1; 
	indxr = temp2; 
	ipiv = temp3; 

	int n = 6;
	int m = 6;

	for (j = 0;j < n; j++) 
	{
		ipiv[j] = 0;
	}

	for (i = 0; i < n; i++) 
	{
		big	=0.0f;
		for (j = 0; j < n; j++)
			if (ipiv[j] != 1)
				for (k = 0; k < n; k++) 
				{
					if (ipiv[k] == 0) 
					{
					if (fabs(result.d[j][k]) >= big) 
{
	big = fabs(result.d[j][k]);
	irow = j;
	icol = k;
}
} 
else if (ipiv[k] > 1) 
{
	PHYTRACE10("GAUSSJ: Singular Matrix-1");
}
        }
    ++(ipiv[icol]);
    if (irow != icol) {
      for (l=0;l<n;l++) SWAP(result.d[irow][l],result.d[icol][l]);
      for (l=0;l<m;l++) SWAP(id.d[irow][l],id.d[icol][l]);
    }
    indxr[i]=irow;
    indxc[i]=icol;
    if (result.d[icol][icol] == 0.0f)
	{
		PHYTRACE10("GAUSSJ: Singular Matrix-2");
	}
    pivinv	=1.0f / result.d[icol][icol];
    result.d[icol][icol]	=1.0f;
    for (l=0;l<n;l++) result.d[icol][l] *= pivinv;
    for (l=0;l<m;l++) id.d[icol][l] *= pivinv;
    for (ll=0;ll<n;ll++)
      if (ll != icol) {
        dum=result.d[ll][icol];
        result.d[ll][icol]	=0.0f;
        for (l=0;l<n;l++) result.d[ll][l] -= result.d[icol][l]*dum;
        for (l=0;l<m;l++) id.d[ll][l] -= id.d[icol][l]*dum;
      }
  }
  for (l=n-1;l>=0;l--) {
    if (indxr[l] != indxc[l])
      for (k=0;k<n;k++)
        SWAP(result.d[k][indxr[l]],result.d[k][indxc[l]]);
  }

  return result;

}

//