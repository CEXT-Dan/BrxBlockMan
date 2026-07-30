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