//
// <minbox.cpp>
//

#include <float.h>
#include "mingeom.h"
#include "mesh.h"
#include "eigen.h"

Matrix MakeRotation (float fRadians, float fX, float fY, float fZ)
{
    float fCos = cos(fRadians);
    float fSin = sin(fRadians);
    float fOneMinusCos = 1.0f-fCos;
    float fX2 = fX*fX;
    float fY2 = fY*fY;
    float fZ2 = fZ*fZ;
    float fXym = fX*fY*fOneMinusCos;
    float fXzm = fX*fZ*fOneMinusCos;
    float fYzm = fY*fZ*fOneMinusCos;
    float fXsin = fX*fSin;
    float fYsin = fY*fSin;
    float fZsin = fZ*fSin;
    
	Matrix m;

    m.d[0][0] = fX2*fOneMinusCos+fCos;
    m.d[0][1] = fXym+fZsin;
    m.d[0][2] = fXzm-fYsin;
    m.d[1][0] = fXym-fZsin;
    m.d[1][1] = fY2*fOneMinusCos+fCos;
    m.d[1][2] = fYzm+fXsin;
    m.d[2][0] = fXzm+fYsin;
    m.d[2][1] = fYzm-fXsin;
    m.d[2][2] = fZ2*fOneMinusCos+fCos;

	return m;
}

void ExtractAngleAndAxis (const Matrix & m, float & fRadians, float & fX, float & fY, float & fZ)
{
    // Let (x,y,z) be the unit-length axis and let A be an angle of rotation.
    // The rotation matrix is R = I + sin(A)*P + (1-cos(A))*P^2 where
    // I is the identity and
    //
    //       +-        -+
    //   P = |  0 +z -y |
    //       | -z  0 +x |
    //       | +y -x  0 |
    //       +-        -+
    //
    // Some algebra will show that
    //
    //   cos(A) = (trace(R)-1)/2  and  R - R^t = 2*sin(A)*P
    //
    // In the event that A = pi, R-R^t = 0 which prevents us from extracting
    // the axis through P.  Instead note that R = I+2*P^2 when A = pi, so
    // P^2 = (R-I)/2.  The diagonal entries of P^2 are x^2-1, y^2-1, and
    // z^2-1.  We can solve these for axis (x,y,z).  Because the angle is pi,
    // it does not matter which sign you choose on the square roots.

    float fTrace = m.d[0][0] + m.d[1][1] + m.d[2][2];
    float fCos = 0.5f*(fTrace-1.0f);
    fRadians = acos(fCos);

    fX = m.d[1][2]-m.d[2][1];
    fY = m.d[2][0]-m.d[0][2];
    fZ = m.d[0][1]-m.d[1][0];
    float fLength = sqrt(fX*fX+fY*fY+fZ*fZ);
    const float fEpsilon = 1e-06f;
    if ( fLength > fEpsilon )
    {
        float fInvLength = 1.0f/fLength;
        fX *= fInvLength;
        fY *= fInvLength;
        fZ *= fInvLength;
    }
    else  // angle is 0 or pi
    {
        if ( fRadians > 1.0f )  // any number strictly between 0 and pi works
        {
            // angle must be pi
            fX = sqrt(0.5f*(1.0f+m.d[0][0]));
            fY = sqrt(0.5f*(1.0f+m.d[1][1]));
            fZ = sqrt(0.5f*(1.0f+m.d[2][2]));

            // determine signs of axis components
            float afTest[3];
            afTest[0] = m.d[0][0]*fX + m.d[0][1]*fY +
                m.d[0][2]*fZ - fX;
            afTest[1] = m.d[1][0]*fX + m.d[1][1]*fY +
                m.d[1][2]*fZ - fY;
            afTest[2] = m.d[2][0]*fX + m.d[2][1]*fY +
                m.d[2][2]*fZ - fZ;
            fLength = afTest[0]*afTest[0] + afTest[1]*afTest[1] +
                afTest[2]*afTest[2];
            if ( fLength < fEpsilon )
                return;

            fY = -fY;
            afTest[0] = m.d[0][0]*fX + m.d[0][1]*fY +
                m.d[0][2]*fZ - fX;
            afTest[1] = m.d[1][0]*fX + m.d[1][1]*fY +
                m.d[1][2]*fZ - fY;
            afTest[2] = m.d[2][0]*fX + m.d[2][1]*fY +
                m.d[2][2]*fZ - fZ;
            fLength = afTest[0]*afTest[0] + afTest[1]*afTest[1] +
                afTest[2]*afTest[2];
            if ( fLength < fEpsilon )
                return;

            fZ = -fZ;
            afTest[0] = m.d[0][0]*fX + m.d[0][1]*fY +
                m.d[0][2]*fZ - fX;
            afTest[1] = m.d[1][0]*fX + m.d[1][1]*fY +
                m.d[1][2]*fZ - fY;
            afTest[2] = m.d[2][0]*fX + m.d[2][1]*fY +
                m.d[2][2]*fZ - fZ;
            fLength = afTest[0]*afTest[0] + afTest[1]*afTest[1] +
                afTest[2]*afTest[2];
            if ( fLength < fEpsilon )
                return;

            fY = -fY;
            afTest[0] = m.d[0][0]*fX + m.d[0][1]*fY +
                m.d[0][2]*fZ - fX;
            afTest[1] = m.d[1][0]*fX + m.d[1][1]*fY +
                m.d[1][2]*fZ - fY;
            afTest[2] = m.d[2][0]*fX + m.d[2][1]*fY +
                m.d[2][2]*fZ - fZ;
            fLength = afTest[0]*afTest[0] + afTest[1]*afTest[1] +
                afTest[2]*afTest[2];
            if ( fLength < fEpsilon )
                return;
        }
        else
        {
            // Angle is zero, matrix is the identity, no unique axis, so
            // return (1,0,0) for as good a guess as any.
            fX = 1.0f;
            fY = 0.0f;
            fZ = 0.0f;
        }
    }
}

//---------------------------------------------------------------------------
static void GetPrimitiveInfo (int iVertices, const Vector* akVertex, const bool* abValid, const Vector& kAngle, Matrix& kRot, Vector& kMin, Vector& kMax)
{
    float cs0 = float(cos(kAngle.x));
    float sn0 = float(sin(kAngle.x));
    float cs1 = float(cos(kAngle.y));
    float sn1 = float(sin(kAngle.y));
    kRot = MakeRotation(kAngle.z,cs0*sn1,sn0*sn1,cs1);

    kMin = Vector(FLT_MAX, FLT_MAX, FLT_MAX);
    kMax = Vector(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    for (int i = 0; i < iVertices; i++)
    {
        Vector kTest = akVertex[i]*kRot;

		if (kTest.x < kMin.x) kMin.x = kTest.x;
		if (kTest.x > kMax.x) kMax.x = kTest.x;
		if (kTest.y < kMin.y) kMin.y = kTest.y;
		if (kTest.y > kMax.y) kMax.y = kTest.y;
		if (kTest.z < kMin.z) kMin.z = kTest.z;
		if (kTest.z > kMax.z) kMax.z = kTest.z;
    }
}
//---------------------------------------------------------------------------
static float Volume (int iVertices, const Vector* akVertex,
    const bool* abValid, const Vector& kAngle)
{
    Vector kMin, kMax;
    Matrix kRot;
    GetPrimitiveInfo(iVertices,akVertex,abValid,kAngle,kRot,kMin,kMax);

	return (kMax.x - kMin.x)*(kMax.y - kMin.y)*(kMax.z - kMin.z);
}
//---------------------------------------------------------------------------
static void MinimalBoxForAngles (int iVertices, const Vector* akVertex,
    const bool* abValid, const Vector& kAngle, Vector& kCenter,
    Vector akAxis[3], Vector & afExtent)
{
    Vector kMin, kMax;
    Matrix kRot;
    GetPrimitiveInfo(iVertices,akVertex,abValid,kAngle,kRot,kMin,kMax);

    kCenter = kRot*(0.5f*(kMin+kMax));
	akAxis[0].x = kRot.d[0][0];
    akAxis[0].y = kRot.d[1][0];
    akAxis[0].z = kRot.d[2][0];
    akAxis[1].x = kRot.d[0][1];
    akAxis[1].y = kRot.d[1][1];
    akAxis[1].z = kRot.d[2][1];
    akAxis[2].x = kRot.d[0][2];
    akAxis[2].y = kRot.d[1][2];
    akAxis[2].z = kRot.d[2][2];

	afExtent = 0.5f * (kMax - kMin);
    
}
//---------------------------------------------------------------------------
static void GetInterval (const Vector& kA, const Vector& kD,
    float& fTmin, float& fTmax)
{
    static const Vector afAngleMin(-PI, 0.0f, 0.0f);
    static const Vector afAngleMax( PI,  PI,  PI);

    fTmin = -FLT_MAX;
    fTmax = +FLT_MAX;

    const float fEpsilon = 1e-08f;
	float fB0, fB1;
	float fInv, fTmp;
    
    fB0 = afAngleMin.x-kA.x; fB1 = afAngleMax.x-kA.x;

    if ( kD.x > fEpsilon )
    {
        fInv = 1.0f/kD.x;
        fTmp = fInv*fB0;
        if ( fTmp > fTmin ) fTmin = fTmp;
        fTmp = fInv*fB1;
        if ( fTmp < fTmax ) fTmax = fTmp;
    }
    else if ( kD.x < -fEpsilon )
    {
        fInv = 1.0f/kD.x;
        fTmp = fInv*fB0;
        if ( fTmp < fTmax ) fTmax = fTmp;
        fTmp = fInv*fB1;
        if ( fTmp > fTmin ) fTmin = fTmp;
    }

	fB0 = afAngleMin.y-kA.y; fB1 = afAngleMax.y-kA.y;

    if ( kD.y > fEpsilon )
    {
        fInv = 1.0f/kD.y;
        fTmp = fInv*fB0;
        if ( fTmp > fTmin ) fTmin = fTmp;
        fTmp = fInv*fB1;
        if ( fTmp < fTmax ) fTmax = fTmp;
    }
    else if ( kD.y < -fEpsilon )
    {
        fInv = 1.0f/kD.y;
        fTmp = fInv*fB0;
        if ( fTmp < fTmax ) fTmax = fTmp;
        fTmp = fInv*fB1;
        if ( fTmp > fTmin ) fTmin = fTmp;
    }

	fB0 = afAngleMin.z-kA.z; fB1 = afAngleMax.z-kA.z;

    if ( kD.z > fEpsilon )
    {
        fInv = 1.0f/kD.z;
        fTmp = fInv*fB0;
        if ( fTmp > fTmin ) fTmin = fTmp;
        fTmp = fInv*fB1;
        if ( fTmp < fTmax ) fTmax = fTmp;
    }
    else if ( kD.z < -fEpsilon )
    {
        fInv = 1.0f/kD.z;
        fTmp = fInv*fB0;
        if ( fTmp < fTmax ) fTmax = fTmp;
        fTmp = fInv*fB1;
        if ( fTmp > fTmin ) fTmin = fTmp;
    }
    
}
//---------------------------------------------------------------------------
static float MinimizeOnInterval (int iVertices, const Vector* akVertex,
    const bool* abValid, Vector& kA, Vector& kD)
{
    // compute intersection of line A+t*D with domain of function
    float fTmin, fTmax;
    GetInterval(kA,kD,fTmin,fTmax);
    float fTran = fTmax-fTmin;

    // bracket a minimum for angles in [A+tmin*D,A+tmax*D]
    Vector kAngle;
    float fT = 0.0f;
    float fVolumeMin = Volume(iVertices,akVertex,abValid,kA);
    float fVolume;

    const int iMaxPartition = 64;
    int i, iMin;
    for (i = 0, iMin = -1; i <= iMaxPartition; i++)
    {
        fT = fTmin+i*fTran/iMaxPartition;
        kAngle = kA+fT*kD;

        fVolume = Volume(iVertices,akVertex,abValid,kAngle);
        if ( fVolume < fVolumeMin )
        {
            iMin = i;
            fVolumeMin = fVolume;
        }
    }

    if ( iMin != -1 )
    {
        fT = fTmin+iMin*fTran/iMaxPartition;
    }
    else
    {
        fT = 0.0f;

        // interval in which t=0 lies
        iMin = int(-fTmin*iMaxPartition/fTran+0.5f);
    }
    fVolume = fVolumeMin;

    float fT0 = fTmin+(iMin-1)*fTran/iMaxPartition;
    kAngle = kA+fT0*kD;
    float fVolume0 = Volume(iVertices,akVertex,abValid,kAngle);

    float fT1 = fTmin+(iMin+1)*fTran/iMaxPartition;
    kAngle = kA+fT1*kD;
    float fVolume1 = Volume(iVertices,akVertex,abValid,kAngle);
    
    // use inverse parabolic interpolation to find the minimum
    const int iInvInterp = 64;
    for (i = 0; i <= iInvInterp; i++)
    {
        float fTmid, fVolumeMid;

        // test for convergence (do not change these parameters)
        const double dEpsilon = 1e-08;
        const double dTol = 1e-04;
        const double dOmTol = 1.0-dTol;
        if ( fabs(fT1-fT0) <= 2.0*dTol*fabs(fT)+dEpsilon )
            break;

        // compute vertex of interpolating parabola
        float fDt0 = fT0-fT, fDt1 = fT1-fT;
        float fDvolume0 = fVolume0-fVolume, fDvolume1 = fVolume1-fVolume;
        float fTemp0 = fDt0*fDvolume1, fTemp1 = fDt1*fDvolume0;
        float fDelta = fTemp1-fTemp0;
        if ( fabs(fDelta) < dEpsilon )
           break;

        fTmid = fT+0.5f*(fDt1*fTemp1-fDt0*fTemp0)/(fTemp1-fTemp0);

        // update bracket
        if ( fTmid < fT )
        {
            kAngle = kA+fTmid*kD;
            fVolumeMid = Volume(iVertices,akVertex,abValid,kAngle);
            if ( fVolumeMid <= fVolume )
            {
                fT1 = fT;
                fVolume1 = fVolume;
                fT = fTmid;
                fVolume = fVolumeMid;
            }
            else
            {
                fT0 = fTmid;
                fVolume0 = fVolumeMid;
            }
        }
        else if ( fTmid > fT )
        {
            kAngle = kA+fTmid*kD;
            fVolumeMid = Volume(iVertices,akVertex,abValid,kAngle);
            if ( fVolumeMid <= fVolume )
            {
                fT0 = fT;
                fVolume0 = fVolume;
                fT = fTmid;
                fVolume = fVolumeMid;
            }
            else
            {
                fT1 = fTmid;
                fVolume1 = fVolumeMid;
            }
        }
        else
        {
            // bracket middle already vertex of parabola
            break;
        }
    }

    kAngle += fT*kD;
    return fVolume;
}
//---------------------------------------------------------------------------
static float MinimizeOnLattice (int iVertices, const Vector* akVertex,
    const bool* abValid, Vector& kA, int iLayers, float fThickness)
{
    int iXmin = 0, iYmin = 0, iZmin = 0;
    float fVolumeMin = Volume(iVertices,akVertex,abValid,kA);

    Vector kAngle;
    for (int z = -iLayers; z <= iLayers; z++)
    {
        kAngle.z = kA.z + fThickness*z/iLayers;
        for (int y = -iLayers; y <= iLayers; y++)
        {
            kAngle.y = kA.y+fThickness*y/iLayers;
            for (int x = -iLayers; x <= iLayers; x++)
            {
                kAngle.x = kA.x+fThickness*x/iLayers;

                float fVolume = Volume(iVertices,akVertex,abValid,kAngle);
                if ( fVolume < fVolumeMin )
                {
                    iXmin = x;
                    iYmin = y;
                    iZmin = z;
                    fVolumeMin = fVolume;
                }
            }
        }
    }

	kA += Vector(1.0, 1.0, 1.0) * fThickness * iXmin / iLayers;
    
    return fVolumeMin;
}
//---------------------------------------------------------------------------
static void InitialGuess (int iVertices, const Vector* akVertex,
    const bool* abValid, Vector& kAngle)
{
    // compute mean of poitns
    Vector kMean(0.0f,0.0f,0.0f);
    int i;
    for (i = 0; i < iVertices; i++)
        kMean += akVertex[i];
    float fInvVertices = 1.0f/iVertices;
    kMean *= fInvVertices;

    // compute covariances of points
    float fXXCov = 0.0f, fXYCov = 0.0f, fXZCov = 0.0f;
    float fYYCov = 0.0f, fYZCov = 0.0f, fZZCov = 0.0f;
    for (i = 0; i < iVertices; i++)
    {
        Vector kDelta = akVertex[i] - kMean;
        fXXCov += kDelta.x * kDelta.x;
        fXYCov += kDelta.x * kDelta.y;
        fXZCov += kDelta.x * kDelta.z;
        fYYCov += kDelta.y * kDelta.y;
        fYZCov += kDelta.y * kDelta.z;
        fZZCov += kDelta.z * kDelta.z;
    }
    fXXCov *= fInvVertices;
    fXYCov *= fInvVertices;
    fXZCov *= fInvVertices;
    fYYCov *= fInvVertices;
    fYZCov *= fInvVertices;
    fZZCov *= fInvVertices;

    // compute eigenvectors for covariance matrix
    EigenSolve eig(3);
    eig.Matrix(0,0) = fXXCov;
    eig.Matrix(0,1) = fXYCov;
    eig.Matrix(0,2) = fXZCov;
    eig.Matrix(1,0) = fXYCov;
    eig.Matrix(1,1) = fYYCov;
    eig.Matrix(1,2) = fYZCov;
    eig.Matrix(2,0) = fXZCov;
    eig.Matrix(2,1) = fYZCov;
    eig.Matrix(2,2) = fZZCov;
    eig.EigenStuff3();

    // Use eigenvectors as the box axes.  Eigenmatrix must not have a
    // reflection component, thus the check for negative determinant.
    const float fEpsilon = 1e-06f;
    const float** aafTmp = eig.Eigenvector();
    Matrix kRot
    (
        aafTmp[0][0], aafTmp[0][1], aafTmp[0][2],
        aafTmp[1][0], aafTmp[1][1], aafTmp[1][2],
        aafTmp[2][0], aafTmp[2][1], aafTmp[2][2]
    );
    float fDet =
        +kRot.d[0][0]*kRot.d[1][1]*kRot.d[2][2]
        +kRot.d[0][1]*kRot.d[1][2]*kRot.d[2][0]
        +kRot.d[0][2]*kRot.d[1][0]*kRot.d[2][1]
        -kRot.d[0][2]*kRot.d[1][1]*kRot.d[2][0]
        -kRot.d[0][1]*kRot.d[1][0]*kRot.d[2][2]
        -kRot.d[0][0]*kRot.d[1][2]*kRot.d[2][1];
    if ( fDet < 0.0f )
    {
        kRot.d[0][0] = -kRot.d[0][0];
        kRot.d[1][0] = -kRot.d[1][0];
        kRot.d[2][0] = -kRot.d[2][0];
    }

    // extract angles from rotation axis = (cos(u)sin(v),sin(u)sin(v),cos(v))
    Vector kAxis;
    ExtractAngleAndAxis(kRot, kAngle.z, kAxis.x, kAxis.y, kAxis.z);
    if ( -1.0f+fEpsilon < kAxis.z )
    {
        if ( kAxis.z < 1.0f-fEpsilon )
        {
            kAngle.x = float(atan2(kAxis.y, kAxis.x));
            kAngle.y = float(acos(kAxis.z));
        }
        else
        {
            kAngle.x = 0.0f;
            kAngle.y = 0.0f;
        }
    }
    else
    {
        kAngle.x = 0.0f;
        kAngle.y = PI;
    }
}
//---------------------------------------------------------------------------
void MinimalBox3 (int iVertices, Vector* akVertex, bool* abValid,
    Vector& kCenter, Vector akAxis[3], Vector & afExtent)
{
    // compute a good initial guess for an oriented bounding box
    Vector kAngle;
    InitialGuess(iVertices,akVertex,abValid,kAngle);
    float fOldVolume = Volume(iVertices,akVertex,abValid,kAngle);
    Vector kSaveAngle = kAngle;

    // Powell's direction set method
    Matrix kU;
    float fVolume;
    const int iMaxIters = 3*32;
    for (int i = 0; i < iMaxIters; i++)
    {
        // reset directions to avoid linear dependence degeneration
        if ( i % 3 == 0 )
            kU.set_identity();

        // find minima in specified directions
        for (int d = 0; d < 3; d++)
        {
            Vector kDir(kU.d[d][0],kU.d[d][1],kU.d[d][2]);
            fVolume = MinimizeOnInterval(iVertices,akVertex,abValid,
                kAngle,kDir);
        }

        // estimate a conjugate direction
        Vector kConj = kAngle - kSaveAngle;
        float fLength = kConj.magnitude();
        if ( fLength >= 1e-06f )
        {
            float fInvLen = 1.0f/fLength;
            kConj *= fInvLen;
            
            // minimize in conjugate direction
            fVolume = MinimizeOnInterval(iVertices,akVertex,abValid,
                kAngle,kConj);
        }
        else
        {
            // Possible local, but not global, minimum.  Search nearby for
            // a smaller volume.
            fVolume = MinimizeOnLattice(iVertices,akVertex,abValid,
                kAngle,2,0.0001f);
            fVolume = MinimizeOnLattice(iVertices,akVertex,abValid,
                kAngle,2,0.0010f);
            fVolume = MinimizeOnLattice(iVertices,akVertex,abValid,
                kAngle,2,0.0100f);
            fVolume = MinimizeOnLattice(iVertices,akVertex,abValid,
                kAngle,2,0.1000f);
        }

        // test for convergence
        const float fEpsilon = 1e-04f;
        float fDiff = float(fabs(fVolume-fOldVolume));
        if ( fDiff <= fEpsilon )
        {
            // Possible local, but not global, minimum.  Search nearby for
            // a smaller volume.
            fVolume = MinimizeOnLattice(iVertices,akVertex,abValid,
                kAngle,2,0.0001f);
            fVolume = MinimizeOnLattice(iVertices,akVertex,abValid,
                kAngle,2,0.0010f);
            fVolume = MinimizeOnLattice(iVertices,akVertex,abValid,
                kAngle,2,0.0100f);
            fVolume = MinimizeOnLattice(iVertices,akVertex,abValid,
                kAngle,2,0.1000f);

            fDiff = float(fabs(fVolume-fOldVolume));
            if ( fDiff <= fEpsilon )
                break;
        }

        // cycle the directions and add conjugate direction to list
        kU.d[0][0] = kU.d[1][0];  kU.d[0][1] = kU.d[1][1];  kU.d[0][2] = kU.d[1][2];
        kU.d[1][0] = kU.d[2][0];  kU.d[1][1] = kU.d[2][1];  kU.d[1][2] = kU.d[2][2];
        kU.d[2][0] = kConj.x;  kU.d[2][1] = kConj.y;  kU.d[2][2] = kConj.z;

        // set parameters for next pass
        fOldVolume = fVolume;
        kSaveAngle = kAngle;
    }

    MinimalBoxForAngles(iVertices,akVertex,abValid,kAngle,kCenter,
        akAxis, afExtent);
}

void compute_optimal_box(BoxExtent * box, S32 vcount, Vector * vlist)
{
	bool valid = FALSE;
	
	Vector center, axis[3], extent;
	
	MinimalBox3(vcount, vlist, &valid, center, axis, extent);
    
    box->xform.set_position(center);
    box->xform.set_i(axis[0]);
    box->xform.set_j(axis[1]);
    box->xform.set_k(axis[2]);

    box->box.half_x = extent.x;
    box->box.half_y = extent.y;
    box->box.half_z = extent.z;
    
}

