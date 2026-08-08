#include "stdafx.h"
#include "BlockJig.h"

//---------------------------------------------------------------------
// BlockJigScale
BlockJigScale::BlockJigScale(const AcDbObjectId& btrid, const AcGePoint3d& pos, double scale)
{
    m_pos = pos;
    m_curScale = scale;
    m_pRef.reset(new AcDbBlockReference(m_pos, btrid));
    m_pRef->setDatabaseDefaults();
    m_baseMat = m_pRef->blockTransform();
    m_refDist = m_curScale;
    if (m_refDist < 1e-4)
        m_refDist = 1.0;
}

AcEdJig::DragStatus BlockJigScale::sampler(void)
{
    setUserInputControls(AcEdJig::kNullResponseAccepted);
    if (auto result = acquireDist(m_curScale, m_pos); result == AcEdJig::kCancel)
        return AcEdJig::kCancel;
    return AcEdJig::kNormal;
}

Adesk::Boolean BlockJigScale::update(void)
{
    if (m_curScale < 1e-6)
        m_curScale = 1e-6;
    double totalScaleFactor = m_curScale / m_refDist;
    AcGeMatrix3d scaleMat = AcGeMatrix3d::scaling(totalScaleFactor, m_pos);
    m_pRef->setBlockTransform(scaleMat * m_baseMat);
    return kTrue;
}

AcDbEntity* BlockJigScale::entity(void) const
{
    return m_pRef.get();
}

Acad::PromptStatus BlockJigScale::doit(void)
{
    setDispPrompt(_T("\nSpecify scale factor: "));
    DragStatus stat = drag();
    if (stat == AcEdJig::kNormal)
        return Acad::eNormal;
    return Acad::eFailed;
}

const double BlockJigScale::getScale() const
{
    acutPrintf(L"\nScale = %f", m_curScale);
    return m_curScale;
}

//---------------------------------------------------------------------
// BlockJigRotate 
BlockJigRotate::BlockJigRotate(const AcDbObjectId& btrid, const AcGePoint3d& pos, double rotation, double scale)
{
    m_pos = pos;
    m_curAng = rotation;
    m_prevAng = 0.0;
    m_pRef.reset(new AcDbBlockReference(m_pos, btrid));
    m_pRef->setDatabaseDefaults();
    AcGeMatrix3d matUcs;
    acedGetCurrentUCS(matUcs);
    AcGePoint3d ucsOrigin;
    AcGeVector3d xAxis, yAxis, zAxis;
    matUcs.getCoordSystem(ucsOrigin, xAxis, yAxis, zAxis);
    m_normal = zAxis;
    AcGeMatrix3d scaleMat = AcGeMatrix3d::scaling(scale, m_pos);
    AcGeMatrix3d rotMat = AcGeMatrix3d::rotation(rotation, m_normal, m_pos);
    m_pRef->transformBy(rotMat * scaleMat);
    m_baseMat = m_pRef->blockTransform();
}

AcEdJig::DragStatus BlockJigRotate::sampler(void)
{
    setUserInputControls(AcEdJig::kNullResponseAccepted);
    if (auto result = acquireAngle(m_curAng, m_pos); result == AcEdJig::kCancel)
        return AcEdJig::kCancel;
    return AcEdJig::kNormal;
}

Adesk::Boolean BlockJigRotate::update(void)
{
    AcGeMatrix3d dynamicRot = AcGeMatrix3d::rotation(m_curAng, m_normal, m_pos);
    m_pRef->setBlockTransform(dynamicRot * m_baseMat);
    return kTrue;
}

AcDbEntity* BlockJigRotate::entity(void) const
{
    return m_pRef.get();
}

Acad::PromptStatus BlockJigRotate::doit(void)
{
    setDispPrompt(_T("\nSpecify rotation angle: "));
    DragStatus stat = drag();
    if (stat == AcEdJig::kNormal)
        return Acad::eNormal;
    return Acad::eFailed;
}

const double BlockJigRotate::getRotation() const
{
    return m_curAng;
}

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
    if (auto result = acquirePoint(m_curpnt); result == AcEdJig::kCancel)
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
