/*****************************************************************************/
/*  STAP++ : A C++ FEM code sharing the same input data file with STAP90     */
/*     Computational Dynamics Laboratory                                     */
/*     School of Aerospace Engineering, Tsinghua University                  */
/*                                                                           */
/*     Release 1.02, October 27, 2017                                        */
/*                                                                           */
/*     http://www.comdyn.cn/                                                 */
/*****************************************************************************/

#include <algorithm>
#include <climits>
#include "Element.h"

//! Virtual deconstructor
CElement::~CElement()
{
    if (!nodes)
        delete [] nodes;
    
    // if (!ElementMaterial)
    //     delete [] ElementMaterial;

    if (!LocationMatrix)
        delete [] LocationMatrix;
}

//  Calculate the column height, used with the skyline storage scheme
void CElement::CalculateColumnHeight(unsigned int* ColumnHeight)
{
    
//  Generate location matrix
    GenerateLocationMatrix();

//  Look for the row number of the first non-zero element
    unsigned int nfirstrow = INT_MAX;
    for (unsigned int i = 0; i < ND; i++)
        if (LocationMatrix[i] && LocationMatrix[i] < nfirstrow)
            nfirstrow = LocationMatrix[i];

//	Calculate the column height contributed by this element
	for (unsigned int i = 0; i < ND; i++)
	{
		unsigned int column = LocationMatrix[i];
		if (!column)
			continue;

		const unsigned int Height = column - nfirstrow;
		if (ColumnHeight[column-1] < Height) ColumnHeight[column-1] = Height;
	}
}

//	Assemble the banded global stiffness matrix (skyline storage scheme)
void CElement::assembly(double* Matrix, CSkylineMatrix<double>* StiffnessMatrix)
{
//	Calculate element stiffness matrix
	ElementStiffness(Matrix);
	
//	Assemble global stiffness matrix
	for (unsigned int j = 0; j < ND; j++)
	{
		unsigned int Lj = LocationMatrix[j];	// Global equation number corresponding to jth DOF of the element
		if (!Lj) 
			continue;

//		Address of diagonal element of column j in the one dimensional element stiffness matrix
		unsigned int DiagjElement = (j+1)*j/2 + 1;

		for (unsigned int i = 0; i <= j; i++)
		{
			unsigned int Li = LocationMatrix[i];	// Global equation number corresponding to ith DOF of the element

			if (!Li)
				continue;
            
            (*StiffnessMatrix)(Li,Lj) += Matrix[DiagjElement + j - i - 1];
		}
	}

	return;
}