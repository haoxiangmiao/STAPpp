#include "Elements/9Q.h"
#include "matrix.h"

#include <cmath>
#include <iomanip>
#include <iostream>

using namespace std;

//	Constructor
C9Q::C9Q()
{
    NEN = 9; // Each element has 4 nodes
    nodes = new CNode*[NEN];

    ND = 27; // 12 DOF in total
    LocationMatrix = new unsigned int[ND];

    ElementMaterial = NULL;
}

//	Desconstructor
C9Q::~C9Q() {}

//	Read element data from stream Input
bool C9Q::Read(ifstream& Input, unsigned int Ele, CMaterial* MaterialSets, CNode* NodeList)
{
    unsigned int N;

    Input >> N; // element number

    if (N != Ele + 1)
    {
        cerr << "*** Error *** Elements must be inputted in order !" << endl
             << "    Expected element : " << Ele + 1 << endl
             << "    Provided element : " << N << endl;

        return false;
    }

    for (unsigned n = 0; n < 9; n++)
    {
        unsigned int N; // node indexs
        Input >> N;
        nodes[n] = NodeList + N - 1;
    }

    unsigned int MSet; // Material property set number
    Input >> MSet;
    ElementMaterial = static_cast<C9QMaterial*>(MaterialSets) + MSet - 1;

    return true;
}

//	Write element data to stream
void C9Q::Write(COutputter& output, unsigned int Ele)
{
    output << setw(5) << Ele + 1                // element index
           << setw(11) << nodes[0]->NodeNumber; // node indexs
    for (unsigned i = 1; i < 9; i++)
    {
        output << setw(9) << nodes[i]->NodeNumber;
    }
    output << setw(12) << ElementMaterial->nset << endl;
}

//  Generate location matrix: the global equation number that corresponding to each DOF of the
//  element
//	Caution:  Equation number is numbered from 1 !
void C9Q::GenerateLocationMatrix()
{
    unsigned int i = 0;
    for (unsigned int N = 0; N < NEN; N++)
        for (unsigned int D = 0; D < 3; D++)
            LocationMatrix[i++] = nodes[N]->bcode[D];
}

//	Return the size of the element stiffness matrix (stored as an array column by column)
unsigned int C9Q::SizeOfStiffnessMatrix() { return ND * (ND + 1) / 2; }

// returns |Je|
// generate B
double GenerateB9Q(Matrix<double>& BB, const double xi, const double eta, const double xe[9],
                 const double ye[9])
{
    double GNData[18] = {((-1 + eta) * eta * (-1 + 2 * xi)) / 4.,
                         ((-1 + 2 * eta) * (-1 + xi) * xi) / 4.,
                         ((-1 + eta) * eta * (1 + 2 * xi)) / 4.,
                         ((-1 + 2 * eta) * xi * (1 + xi)) / 4.,
                         (eta * (1 + eta) * (1 + 2 * xi)) / 4.,
                         ((1 + 2 * eta) * xi * (1 + xi)) / 4.,
                         (eta * (1 + eta) * (-1 + 2 * xi)) / 4.,
                         ((1 + 2 * eta) * (-1 + xi) * xi) / 4.,
                         -((-1 + eta) * eta * xi),
                         -((-1 + 2 * eta) * (-1 + std::pow(xi, 2))) / 2.,
                         -((-1 + std::pow(eta, 2)) * (1 + 2 * xi)) / 2.,
                         -(eta * xi * (1 + xi)),
                         -(eta * (1 + eta) * xi),
                         -((1 + 2 * eta) * (-1 + std::pow(xi, 2))) / 2.,
                         -((-1 + std::pow(eta, 2)) * (-1 + 2 * xi)) / 2.,
                         -(eta * (-1 + xi) * xi),
                         2 * (-1 + std::pow(eta, 2)) * xi,
                         2 * eta * (-1 + std::pow(xi, 2))};

    Matrix<double> GN(2, 9, GNData);

    double dData[18];
    for (unsigned i = 0; i < 9; i++)
    {
        dData[i] = xe[i];
        dData[i + 9] = ye[i];
    }
    Matrix<double> d(9, 2, dData);

    // Je = GN * [xe ye]
    Matrix<double> Je = GN * d;

    BB = Je.inverse() * GN;

    double JeDet = -Je.c_at(1, 3) * Je.c_at(2, 2) * Je.c_at(3, 1) +
                   Je.c_at(1, 2) * Je.c_at(2, 3) * Je.c_at(3, 1) +
                   Je.c_at(1, 3) * Je.c_at(2, 1) * Je.c_at(3, 2) -
                   Je.c_at(1, 1) * Je.c_at(2, 3) * Je.c_at(3, 2) -
                   Je.c_at(1, 2) * Je.c_at(2, 1) * Je.c_at(3, 3) +
                   Je.c_at(1, 1) * Je.c_at(2, 2) * Je.c_at(3, 3);

    return JeDet;
}

void AccumulateEtaPsi9Q(const double& eta, const double& psi, const double& weight, const double* xe,
                      const double* ye, Matrix<double>& ke, const double E, const double v)
{
    Matrix<double> BB(2, 9);
    double DetJe = GenerateB9Q(BB, eta, psi, xe, ye);

    Matrix<double> B(3, 18);
    for (unsigned i = 0; i < 9; i++)
    {
        double partialX = BB.c_at(1, 1 + i);
        double partialY = BB.c_at(2, 1 + i);
        B.at(1, 1 + 2 * i) = partialX;
        B.at(3, 1 + 2 * i + 1) = partialX;
        B.at(2, 1 + 2 * i + 1) = partialY;
        B.at(3, 1 + 2 * i) = partialY;
    }

    double DData[9] = {1, v, 0, v, 1, 0, 0, 0, (1 - v) / 2};
    Matrix<double> D(3, 3, DData);
    D = D * (E / (1 - v * v) * DetJe);

    // see 4Q.nb and 4Q-form-key.py
    ke = B * D * B;
}

// convert ke' to ke with R (input as i and j)
void Convert2d23d9Q(Matrix<double>& k, double* matrix, const double i[3], const double j[3])
{
    auto f = [i, j](Matrix<double>::Pos_t row, Matrix<double>::Pos_t column) -> double {
        if (row / 2 == column / 3)
        {
            return (row % 2 ? j : i)[column % 3];
        }
        else
        {
            return double(0);
        }
    };
    Matrix<double> R(2 * 9, 3 * 9, f);
    Matrix<double> K = R.transpose() * k * R;
    for (unsigned column = 1; column <= 27; column++)
    {
        for (unsigned row = column; row >= 1; row--)
        {
            unsigned index = column * (column - 1) / 2 + (column - row);
            matrix[index] = K.c_at(row, column);
        }
    }
}

// calculate n, i, j and xe, ye
inline void Convert3d22d(CNode* const nodes[9], double n[3], double i[3], double j[3], double xe[9],
                         double ye[9])
{
    const CNode& n1 = *nodes[0];
    const CNode& n2 = *nodes[1];
    const CNode& n3 = *nodes[2];

    // make p31 p21
    const double p31[3] = {n3.XYZ[0] - n1.XYZ[0], n3.XYZ[1] - n1.XYZ[1], n3.XYZ[2] - n1.XYZ[2]};
    const double p21[3] = {n2.XYZ[0] - n1.XYZ[0], n2.XYZ[1] - n1.XYZ[1], n2.XYZ[2] - n1.XYZ[2]};

    // n = p31 cross p21 (normalized)
    n[0] = p31[1] * p21[2] - p31[2] * p21[1];
    n[1] = p31[2] * p21[0] - p31[0] * p21[2];
    n[2] = p31[0] * p21[1] - p31[1] * p21[0];
    normalize(n);

    // i = normalized p21
    // i is manually set parallel to p21 so that y21 = 0
    i[0] = p21[0];
    i[1] = p21[1];
    i[2] = p21[2];
    normalize(i);
    // j = n cross i
    j[0] = n[1] * i[2] - n[2] * i[1];
    j[1] = n[2] * i[0] - n[0] * i[2];
    j[2] = n[0] * i[1] - n[1] * i[0];

    // by here, a conversion matrix is formed,
    // as (x', y') = ((i0, i1, i2), (j0, j1, j2)) . (x, y, z)

    // generate xe, ye
    for (unsigned n = 0; n < 9; n++)
    {
        xe[n] = i[0] * nodes[n]->XYZ[0] + i[1] * nodes[n]->XYZ[1] + i[2] * nodes[n]->XYZ[2];
        ye[n] = j[0] * nodes[n]->XYZ[0] + j[1] * nodes[n]->XYZ[1] + j[2] * nodes[n]->XYZ[2];
    }
}

//	Calculate element stiffness matrix
//	Upper triangular matrix, stored as an array column by colum starting from the diagonal element
void C9Q::ElementStiffness(double* matrix)
{
    clear(matrix, SizeOfStiffnessMatrix());

    // =========================== 3d to 2d ============================
    double n[3], i[3], j[3], xe[9], ye[9];
    Convert3d22d(nodes, n, i, j, xe, ye);

    // =========================== assembly Ke' =========================
    // generate GN4Q for eta, psi

    const double pos = 1 / std::sqrt(3.0f);
    const double etas[2] = {-pos, pos};
    const double psis[2] = {-pos, pos};
    const double weights[2][2] = {{1.0, 1.0}, {1.0, 1.0}};

    const C9QMaterial* material =
        static_cast<C9QMaterial*>(ElementMaterial); // Pointer to material of the element
    const double& E = material->E;
    const double& v = material->nu;
    
    Matrix<double> ke(18, 18);
    AccumulateEtaPsi9Q(etas[0], psis[0], weights[0][0], xe, ye, ke, E, v);
    AccumulateEtaPsi9Q(etas[0], psis[1], weights[0][1], xe, ye, ke, E, v);
    AccumulateEtaPsi9Q(etas[1], psis[0], weights[1][0], xe, ye, ke, E, v);
    AccumulateEtaPsi9Q(etas[1], psis[1], weights[1][1], xe, ye, ke, E, v);

    // ======================== assembly Ke (2d to 3d) ======================

    Convert2d23d9Q(ke, matrix, i, j);
    return;
}

void CalculateStressAt9Q(double xi, double eta, double xe[9], double ye[9], double E, double v,
                       const double de[18], double* stress)
{
    // generate B first
    // double B[8];
    // GenerateB9Q(B, xi, eta, xe, ye);

    // // see ../../memo/4Q/4Q.nb and ../../memo/4Q/4Q-calc-stress.py
    // double d33 = (1.f - v) / 2.0f;
    // const double cof = E / (1 - v * v);
    // stress[0] = cof * (B[0] * de[0] + B[1] * de[2] + B[2] * de[4] + B[3] * de[6] +
    //                    v * (B[4] * de[1] + B[5] * de[3] + B[6] * de[5] + B[7] * de[7]));
    // stress[1] =
    //     cof * (B[4] * de[1] + B[5] * de[3] + B[6] * de[5] +
    //            v * (B[0] * de[0] + B[1] * de[2] + B[2] * de[4] + B[3] * de[6]) + B[7] * de[7]);
    // stress[2] = cof * (d33 * (B[4] * de[0] + B[0] * de[1] + B[5] * de[2] + B[1] * de[3] +
    //                           B[6] * de[4] + B[2] * de[5] + B[7] * de[6] + B[3] * de[7]));
}

void CalculateN9Q(double eta, double psi, double N[4])
{
    N[0] = (1 - psi) * (1 - eta) / 4.;
    N[1] = (1 + psi) * (1 - eta) / 4.;
    N[2] = (1 + psi) * (1 + eta) / 4.;
    N[3] = (1 - psi) * (1 + eta) / 4.;
}

// generate 3d position and return weight
void CalculatePositionAt9Q(double eta, double psi, double xe[4], double ye[4], double i[3],
                         double j[3], double Positions[3])
{
    double N[4];
    CalculateN9Q(eta, psi, N);
    // generate local x
    double x2d = N[0] * xe[0] + N[1] * xe[1] + N[2] * xe[2] + N[3] * xe[3];
    double y2d = N[0] * ye[0] + N[1] * ye[1] + N[2] * ye[2] + N[3] * ye[3];

    // convert to 3d
    Positions[0] = i[0] * x2d + j[0] * y2d;
    Positions[1] = i[1] * x2d + j[1] * y2d;
    Positions[2] = i[2] * x2d + j[2] * y2d;
}

void CalculateDisplacementAt9Q(double eta, double psi, double de[8], double i[3], double j[3],
                             double Displacements[3])
{
    double N[4];
    CalculateN9Q(eta, psi, N);
    double ux = N[0] * de[0] + N[1] * de[2] + N[2] * de[4] + N[3] * de[6];
    double uy = N[0] * de[1] + N[1] * de[3] + N[2] * de[5] + N[3] * de[7];
    Displacements[0] = i[0] * ux + j[0] * uy;
    Displacements[1] = i[1] * ux + j[1] * uy;
    Displacements[2] = i[2] * ux + j[2] * uy;
}

double CalculateWeightAt9Q(double eta, double psi, double xe[4], double ye[4])
{
    double GN4Q[8] = {
        (eta - 1) / 4, (1 - eta) / 4,  (1 + eta) / 4, (-eta - 1) / 4, // first row
        (psi - 1) / 4, (-psi - 1) / 4, (1 + psi) / 4, (1 - psi) / 4   // second row
    };
    // Je = GN4Q * [xe ye]
    double Je[4] = {
        GN4Q[0] * xe[0] + GN4Q[1] * xe[1] + GN4Q[2] * xe[2] + GN4Q[3] * xe[3],
        GN4Q[0] * ye[0] + GN4Q[1] * ye[1] + GN4Q[2] * ye[2] + GN4Q[3] * ye[3], // first row
        GN4Q[4] * xe[0] + GN4Q[5] * xe[1] + GN4Q[6] * xe[2] + GN4Q[7] * xe[3],
        GN4Q[4] * ye[0] + GN4Q[5] * ye[1] + GN4Q[6] * ye[2] + GN4Q[7] * ye[3] // second row
    };
    return std::abs(Je[0] * Je[3] - Je[1] * Je[2]);
}

//	Calculate element stress
// stress:              double[12], represent 3 stress for 4 gauss points in C9Q element.
// Displacement:        double[NEQ], represent the Force vector.
// Positions:           double[12], represent 3d position for 4 gauss points.
// GaussDisplacements:  double[12], represent 3d displacements for 4 gauss points.
// Weights:             double[4], represent integrate weights.
void C9Q::ElementStress(double stress[12], double* Displacement, double Positions[12],
                        double GaussDisplacements[12], double weights[4])
{
    // =========================== 3d to 2d ============================
    double n[3], i[3], j[3], xe[4], ye[4];
    Convert3d22d(nodes, n, i, j, xe, ye);

    // form d first.
    // d represent 3d displacements at boundary nodes.
    double d[12];
    for (unsigned index = 0; index < 12; ++index)
    {
        if (LocationMatrix[index])
        {
            d[index] = Displacement[LocationMatrix[index] - 1];
        }
        else
        {
            d[index] = 0.0;
        }
    }

    // generate de, convert from 3d to 2d.
    double de[8] = {
        d[0] * i[0] + d[1] * i[1] + d[2] * i[2],   d[0] * j[0] + d[1] * j[1] + d[2] * j[2],
        d[3] * i[0] + d[4] * i[1] + d[5] * i[2],   d[3] * j[0] + d[4] * j[1] + d[5] * j[2],
        d[6] * i[0] + d[7] * i[1] + d[8] * i[2],   d[6] * j[0] + d[7] * j[1] + d[8] * j[2],
        d[9] * i[0] + d[10] * i[1] + d[11] * i[2], d[9] * j[0] + d[10] * j[1] + d[11] * j[2]};

    // ======================= calculate stress ========================
    C9QMaterial* material =
        static_cast<C9QMaterial*>(ElementMaterial); // Pointer to material of the element
    const double& E = material->E;
    const double& v = material->nu;

    double pos = 1 / std::sqrt(3.0f);
    double etas[2] = {-pos, pos};
    double psis[2] = {-pos, pos};

    // calculate Positions
    CalculatePositionAt9Q(etas[0], psis[0], xe, ye, i, j, Positions + 0);
    CalculatePositionAt9Q(etas[0], psis[1], xe, ye, i, j, Positions + 3);
    CalculatePositionAt9Q(etas[1], psis[0], xe, ye, i, j, Positions + 6);
    CalculatePositionAt9Q(etas[1], psis[1], xe, ye, i, j, Positions + 9);

    // calculate stresses
    CalculateStressAt9Q(etas[0], psis[0], xe, ye, E, v, de, stress + 0);
    CalculateStressAt9Q(etas[0], psis[1], xe, ye, E, v, de, stress + 3);
    CalculateStressAt9Q(etas[1], psis[0], xe, ye, E, v, de, stress + 6);
    CalculateStressAt9Q(etas[1], psis[1], xe, ye, E, v, de, stress + 9);

#ifdef _TEST_
    CalculateDisplacementAt9Q(etas[0], psis[0], de, i, j, GaussDisplacements + 0);
    CalculateDisplacementAt9Q(etas[0], psis[1], de, i, j, GaussDisplacements + 3);
    CalculateDisplacementAt9Q(etas[1], psis[0], de, i, j, GaussDisplacements + 6);
    CalculateDisplacementAt9Q(etas[1], psis[1], de, i, j, GaussDisplacements + 9);

    weights[0] = 1.0 * CalculateWeightAt9Q(etas[0], psis[0], xe, ye);
    weights[1] = 1.0 * CalculateWeightAt9Q(etas[0], psis[1], xe, ye);
    weights[2] = 1.0 * CalculateWeightAt9Q(etas[1], psis[0], xe, ye);
    weights[3] = 1.0 * CalculateWeightAt9Q(etas[1], psis[1], xe, ye);
#endif
}
