//
//
//

#include "xlist.h"

//


void XList::set_X_by_Y(int y, const int * x_list, int x_list_len)
{
// Remove all associations with y.
	XNode * ynode = &y_list[y];
	for (int i = 0; i < ynode->n; i++)
	{
		XNode * xnode = &x_list[ynode->list[i]];
		xnode->remove(y);
	}

// Clear y's list of assocations.
	ynode->clear();

// Add new associations.
	for (i = 0; i < x_len; i++)
	{
	// Add x to y's list.
		ynode->add(x[i]);

	// Add y to x's list.
		XNode * xnode = &x_list[x[i]];
		xnode->add(y);
	}
}

//

void XList::set_Y_by_X(int x, const int * y, int y_len)
{
// Remove all associations with x.
	XNode * xnode = &x_list[x];
	for (int i = 0; i < xnode->n; i++)
	{
		XNode * ynode = &y_list[xnode->list[i]];
		ynode->remove(x);
	}

// Clear x's list of assocations.
	xnode->clear();

// Add new associations.
	for (i = 0; i < y_len; i++)
	{
	// Add y to x's list.
		xnode->add(y[i]);

	// Add x to y's list.
		XNode * ynode = &y_list[y[i]];
		ynode->add(x);
	}
}

//

int XList::get_X_by_Y(int * x, int y) const
{
	XNode * ynode = &y_list[y];
	for (int i = 0; i < ynode->n; i++)
	{
		x[i] = ynode->list[i];
	}

	return ynode->n;
}

//

int XList::get_Y_by_X(int * y, int x) const
{
	XNode * xnode = &x_list[x];
	for (int i = 0; i < xnode->n; i++)
	{
		y[i] = xnode->list[i];
	}

	return xnode->n;
}

//
