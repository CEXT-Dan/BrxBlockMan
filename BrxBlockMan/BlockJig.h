#pragma once

//---------------------------------------------------------------------
// BlockJig 
class BlockJig : public AcEdJig
{
private:
    AcGePoint3d m_curpnt;
    std::unique_ptr<AcDbBlockReference> m_pRef;
public:
    BlockJig(const AcDbObjectId& btrid, double scale, double rotation);
    virtual ~BlockJig(void) override = default;
    virtual AcEdJig::DragStatus sampler(void)override;
    virtual Adesk::Boolean update(void)override;
    virtual AcDbEntity* entity(void) const override;
    Acad::PromptStatus doit(void);
    const AcGePoint3d& getPoint() const;
};

//---------------------------------------------------------------------
// BlockJigScale
class BlockJigScale : public AcEdJig
{
private:
    double m_curScale;
    double m_refDist;
    AcGeMatrix3d m_baseMat;
    AcGePoint3d m_pos;
    std::unique_ptr<AcDbBlockReference> m_pRef;
public:
    BlockJigScale(const AcDbObjectId& btrid, const AcGePoint3d& pos, double scale);
    virtual ~BlockJigScale(void) override = default;
    virtual AcEdJig::DragStatus sampler(void)override;
    virtual Adesk::Boolean update(void)override;
    virtual AcDbEntity* entity(void) const override;
    Acad::PromptStatus doit(void);
    const double getScale() const;
};

//---------------------------------------------------------------------
// BlockJigRotate 
class BlockJigRotate : public AcEdJig
{
private:
    double m_curAng;
    double m_prevAng;
    AcGePoint3d m_pos;
    AcGeVector3d m_normal;
    AcGeMatrix3d m_baseMat;
    std::unique_ptr<AcDbBlockReference> m_pRef;
public:
    BlockJigRotate(const AcDbObjectId& btrid, const AcGePoint3d& pos, double rotation, double scale);
    virtual ~BlockJigRotate(void) override = default;
    virtual AcEdJig::DragStatus sampler(void)override;
    virtual Adesk::Boolean update(void)override;
    virtual AcDbEntity* entity(void) const override;
    Acad::PromptStatus doit(void);
    const double getRotation() const;
};
