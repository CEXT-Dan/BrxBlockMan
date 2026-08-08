#pragma once

#include "AcGsManager.h"
#include "Image.h"
#include "RgbModel.h"

//---------------------------------------------------------------------
// BlockInfo
struct BlockInfo
{
    AcDbObjectId id;
    wxString name;
    wxImage preview;
};

typedef std::vector<BlockInfo> BlockInfoArray;

struct AcGsDeviceDeleter
{
    void operator()(AcGsDevice* ptr);
};
using AcGsDevicePtr = std::unique_ptr <AcGsDevice, AcGsDeviceDeleter>;

struct AcGsViewDeleter
{
    void operator()(AcGsView* ptr);
};
using AcGsViewPtr = std::unique_ptr <AcGsView, AcGsViewDeleter>;

struct AcGsModelDeleter
{
    void operator()(AcGsModel* ptr);
};
using AcGsModelPtr = std::unique_ptr <AcGsModel, AcGsModelDeleter>;

class BlockImageRenderer
{
public:
    BlockImageRenderer(int width, int height, const std::array<int, 3>& rgb);

    bool isValid() const;
    wxImage render(AcDbBlockTableRecord* pBlock, double zoomFactor);

private:
    AcGsGraphicsKernel* m_pGraphicsKernel;
    AcGsDevicePtr m_pOffDevice;
    AcGsViewPtr m_pView;
    AcGsModelPtr m_pModel;
    int m_width;
    int m_height;
    bool m_isReady;
    Atil::RgbModel m_rgbModel;
    Atil::ImagePixel m_initialColor;
};

class BlockWorker
{
public:
    static wxImage getBlockImage(AcDbObjectId id, int width, int height, double zf, const std::array<int, 3>& rgb);
    static Acad::ErrorStatus getBlockImages(BlockInfoArray& info, int width, int height, double zf, const std::array<int, 3>& rgb);
    static Acad::ErrorStatus getBlockInfoFromdDb(AcDbDatabase* srcDb, BlockInfoArray& info);
    static Acad::ErrorStatus insertBlockTableRecord(AcDbDatabase* srcDb, const wxString& blockName, double scale, double rotation, OnScreenFlags flags);
    static Acad::ErrorStatus insertDwg(AcDbDatabase* srcDb, double scale, double rotation, OnScreenFlags flags);
    static Acad::ErrorStatus xformBlockJig(const AcDbObjectId& id, AcGePoint3d& point, double& scale, double& rotation, OnScreenFlags flags);
};

