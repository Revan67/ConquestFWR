#ifndef ALGEBRA_H
#define ALGEBRA_H

class mgcAlgebraicRoots
{
public:
	mgcAlgebraicRoots (float _tolerance = 0)
		{ tolerance = _tolerance;}

	float tolerance;

	unsigned QuadraticRoots (float c[2], float root[2]);
	unsigned CubicRoots (float c[3], float root[3]);
	unsigned QuarticRoots (float c[4], float root[4]);

	int AllRealPartsNegative (int n, float c[]);
	int AllRealPartsPositive (int n, float c[]);
};

class mgcAlgebraicRootsD
{
public:
	mgcAlgebraicRootsD (double _tolerance = 0)
		{ tolerance = _tolerance;}

	double tolerance;

	unsigned QuadraticRoots (double c[2], double root[2]);
	unsigned CubicRoots (double c[3], double root[3]);
	unsigned QuarticRoots (double c[4], double root[4]);

	int AllRealPartsNegative (int n, double c[]);
	int AllRealPartsPositive (int n, double c[]);
};


#endif
