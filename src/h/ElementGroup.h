/*****************************************************************************/
/*  STAP++ : A C++ FEM code sharing the same input data file with STAP90     */
/*     Computational Dynamics Laboratory                                     */
/*     School of Aerospace Engineering, Tsinghua University                  */
/*                                                                           */
/*     Release 1.02, October 27, 2017                                        */
/*                                                                           */
/*     http://www.comdyn.cn/                                                 */
/*****************************************************************************/

#pragma once

#include <fstream>

#include "Element.h"
#include "Elements/Bar.h"
#include "Elements/Quadrilateral.h"
#include "Elements/Triangle.h"
#include "Elements/8H.h"
#include "Elements/Beam.h"
#include "Elements/TimoshenkoSRINT.h"
#include "Elements/TimoshenkoEBMOD.h"
#include "Elements/Plate.h"
#include "Material.h"
#include "Node.h"

using namespace std;

enum ElementTypes
{
    UNDEFINED = 0,
    Bar,
    Quadrilateral,
    Triangle,
    Hexahedron,
    Beam,
    Plate,
    Shell,
    TimoshenkoSRINT,
    TimoshenkoEBMOD
};

//! Element group class
class CElementGroup
{
private:
    //! List of all nodes in the domain, obtained from CDomain object
    static CNode* NodeList_;

    //! Element type of this group
    ElementTypes ElementType_;

    //! Element size of this group
    std::size_t ElementSize_;

    std::size_t MaterialSize_;

    //! Number of elements in this group
    unsigned int NUME_;

    //! Element List in this group
    CElement* ElementList_;

    //! Number of material/section property sets in this group
    unsigned int NUMMAT_;

    //! Material list in this group
    CMaterial* MaterialList_;

public:
    //! Constructor
    CElementGroup();

    //! Deconstructor
    ~CElementGroup();

    //! Read element group data from stream Input
    bool Read(ifstream& Input);

    void CalculateMemberSize();

    void AllocateElement(std::size_t size);

    void AllocateMaterial(std::size_t size);

    //! Read element data from the input data file
    bool ReadElementData(ifstream& Input);

    //! Read quadrilateral element data from the input data file
    bool ReadQuadrilateralElementData(ifstream& Input);

    //! Read plate element data from the input data file
    bool ReadPlateElementData(ifstream& Input);

    //! Return element type of this group
    ElementTypes GetElementType() { return ElementType_; }

    //! Return the number of elements in the group
    unsigned int GetNUME() { return NUME_; }

    CElement& GetElement(unsigned int index);

    CMaterial& GetMaterial(unsigned int index);

    //! Return the number of material/section property setss in this element group
    unsigned int GetNUMMAT() { return NUMMAT_; }
};
