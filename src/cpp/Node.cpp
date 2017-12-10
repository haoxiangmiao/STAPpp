/*****************************************************************************/
/*  STAP++ : A C++ FEM code sharing the same input data file with STAP90     */
/*     Computational Dynamics Laboratory                                     */
/*     School of Aerospace Engineering, Tsinghua University                  */
/*                                                                           */
/*     Release 1.02, October 27, 2017                                        */
/*                                                                           */
/*     http://www.comdyn.cn/                                                 */
/*****************************************************************************/

#include <iostream>
#include <iomanip>
#include <string>

#include "Node.h"

CNode::CNode(double X, double Y, double Z):NodeNumber(0)
{
    XYZ[0] = X;		// Coordinates of the node
    XYZ[1] = Y;
    XYZ[2] = Z;
    
    bcode[0] = 0;	// Boundary codes
    bcode[1] = 0;
    bcode[2] = 0;
    bcode[3] = 1;
    bcode[4] = 1;
    bcode[5] = 1;

	BcodeFlag = 0;	// Boundary code flag
};

//	Read element data from stream Input
bool CNode::Read(ifstream& Input, unsigned int np)
{
	unsigned int N;

	Input >> N;	// node number
	if (N != np + 1) 
	{
		cerr << "*** Error *** Nodes must be inputted in order !" << endl 
			 << "   Expected node number : " << np + 1 << endl
			 << "   Provided node number : " << N << endl;

		return false;
	}

	NodeNumber = N;

	Input >> bcode[0] >> bcode[1] >> bcode[2] >> XYZ[0] >> XYZ[1] >> XYZ[2];
	
	// Flags determing whether the istream is at the end of the line
	string EndFlag;
	getline(Input, EndFlag,'\n');
	if (!EndFlag.empty()) {
		// the last 3 bcodes are given
		BcodeFlag = 1;
		bcode[3] = static_cast<int>(XYZ[0]);
		bcode[4] = static_cast<int>(XYZ[1]);
		bcode[5] = static_cast<int>(XYZ[2]);
		int posTab = 0;
		string XYZtempstr;
		for (unsigned int i = 0; i < 2; i++) {
			int posNextTab = EndFlag.find('\t',posTab+1);
			posTab += posNextTab - posTab;
			XYZtempstr.assign(EndFlag, posTab, posNextTab - posTab);
			XYZ[i] = atof(XYZtempstr.c_str());
		}
		int LastStrLength = EndFlag.at(EndFlag.size() - 1) - posTab;
		XYZtempstr.assign(EndFlag, posTab, LastStrLength);
		XYZ[2] = atof(XYZtempstr.c_str());
	}
	
	return true;
}

//	Output nodal point data to stream
void CNode::Write(COutputter& output, unsigned int np)
{
	output << setw(9) << np + 1 
		<< setw(5) << bcode[0] << setw(5) << bcode[1] << setw(5) << bcode[2]
		<< setw(18) << XYZ[0] << setw(15) << XYZ[1] << setw(15) << XYZ[2] << endl;
}

//	Output equation numbers of nodal point to stream
void CNode::WriteEquationNo(COutputter& output, unsigned int np)
{
	output << setw(9) << np+1 << "       ";

	for (unsigned int dof = 0; dof < CNode::NDF; dof++)	// Loop over for DOFs of node np
	{
		output << setw(5) << bcode[dof];
	}

	output << endl;
}

//	Write nodal displacement
void CNode::WriteNodalDisplacement(COutputter& output, unsigned int np, double* Displacement)
{
	output << setw(5) << np + 1 << "        ";

	for (unsigned int j = 0; j < NDF; j++)
	{
		if (bcode[j] == 0)
		{
			output << setw(18) << 0.0;
		}
		else
		{
			output << setw(18) << Displacement[bcode[j] - 1];
		}
	}

	output << endl;
}
