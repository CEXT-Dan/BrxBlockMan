#include "stdafx.h"
#include "BlockJig.h"

//---------------------------------------------------------------------
// BlockJig 
BlockJig::BlockJig(const AcDbObjectId& btrid, double scale, double rotation)
{
    m_pRef.reset(new AcDbBlockReference(AcGePoint3d::kOrigin, btrid));
    m_pRef->setDatabaseDefaults();

    AcGeMatrix3d matUcs;
    acedGetCurrentUCS(matUcs);

    AcGePoint3d ucsOrigin;
    AcGeVector3d xAxis, yAxis, zAxis;
    matUcs.getCoordSystem(ucsOrigin, xAxis, yAxis, zAxis);

    matUcs = matUcs * AcGeMatrix3d::rotation(rotation, AcGeVector3d::kZAxis);
    matUcs = matUcs * AcGeMatrix3d::scaling(scale);
    m_pRef->transformBy(matUcs);
}

AcEdJig::DragStatus BlockJig::sampler(void)
{
    setUserInputControls(AcEdJig::kAccept3dCoordinates);
    if (acquirePoint(m_curpnt) != AcEdJig::kNormal)
        return AcEdJig::kCancel;
    return AcEdJig::kNormal;
}

Adesk::Boolean BlockJig::update(void)
{
    if (m_pRef->setPosition(m_curpnt) == eOk)
        return kTrue;
    return kFalse;
}

AcDbEntity* BlockJig::entity(void) const
{
    return m_pRef.get();
}

Acad::PromptStatus BlockJig::doit(void)
{
    setDispPrompt(_T("\nInsertion Point: "));
    DragStatus stat = drag();
    if (stat == AcEdJig::kNormal)
        return Acad::eNormal;
    return Acad::eFailed;
}

const AcGePoint3d& BlockJig::getPoint() const
{
    return m_curpnt;
}
