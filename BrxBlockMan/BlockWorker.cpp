#include "stdafx.h"
#include "BlockWorker.h"
#include "AcGsManager.h"

#include "Image.h"
#include "RgbModel.h"
#include "RgbGrayModel.h"
#include "RgbPaletteModel.h"
#include "codec_properties/FormatCodecPropertyInterface.h"
#include "format_codecs/BmpFormatCodec.h"
#include "RowProviderInterface.h"
#include "FileWriteDescriptor.h"
#include "DataBuffer.h"

#include <atlbase.h>
#include <atlsafe.h>
#include "BlockJig.h"

static int cvport()
{
    struct resbuf rb;
    acedGetVar(_T("CVPORT"), &rb);
    return rb.resval.rint;
}

static void setBackgroundColorFromArray(AcGsDevice* pDevice, const std::array<int, 3>& rgb)
{
    if (pDevice != nullptr)
    {
        if (rgb.size() == 3)
        {
            AcGsColor bkclr{};
            bkclr.m_red = rgb[0];
            bkclr.m_green = rgb[1];
            bkclr.m_blue = rgb[2];
            pDevice->setBackgroundColor(bkclr);
        }
    }
}

static AcDbExtents calcBlockExtents(AcDbBlockTableRecord& rec)
{
    AcDbExtents ex;
    auto [es, iter] = makeBlockTableRecordIterator(rec);
    if (es != eOk)
    {
        ex.addBlockExt(&rec);
        return ex;
    }
    AcDbObjectId id;
    for (iter->start(); !iter->done(); iter->step())
    {
        if (iter->getEntityId(id) == eOk)
        {
            AcDbExtents subex;
            AcDbEntityPointer pEnt(id);
            if (pEnt->visibility() == AcDb::kVisible)
            {
                if (pEnt->getGeomExtents(subex) == eOk)
                    ex.addExt(subex);
            }
        }
    }
    return ex;
}

void AcGsDeviceDeleter::operator()(AcGsDevice* ptr)
{
    if (ptr == nullptr)
        return;
    acgsGetGsManager()->destroyAutoCADDevice(ptr);
}

void AcGsViewDeleter::operator()(AcGsView* ptr)
{
    if (ptr == nullptr)
        return;
    ptr->eraseAll();
}

void AcGsModelDeleter::operator()(AcGsModel* ptr)
{
    if (ptr == nullptr)
        return;
    acgsGetGsManager()->destroyAutoCADModel(ptr);
}

BlockImageRenderer::BlockImageRenderer(int width, int height, const std::array<int, 3>& rgb)
    : m_pGraphicsKernel(nullptr)
    , m_width(width)
    , m_height(height)
    , m_isReady(false)
    , m_rgbModel(32)
    , m_initialColor(m_rgbModel.pixelType())
{
    AcGsManager* gsManager = acgsGetGsManager();
    AcGsKernelDescriptor descriptor;
    descriptor.addRequirement(AcGsKernelDescriptor::k3DDrawing);
    m_pGraphicsKernel = AcGsManager::acquireGraphicsKernel(descriptor);
    if (m_pGraphicsKernel == nullptr)
        return;

    m_pOffDevice.reset(gsManager->createAutoCADOffScreenDevice(*m_pGraphicsKernel));
    if (m_pOffDevice == nullptr)
        return;

    m_pView.reset(gsManager->createView(m_pOffDevice.get()));
    if (m_pView == nullptr)
        return;

    m_pModel.reset(gsManager->createAutoCADModel(*m_pGraphicsKernel));
    if (m_pModel == nullptr)
        return;

    m_pOffDevice->onSize(m_width, m_height);
    if (!m_pOffDevice->add(m_pView.get()))
        return;

    if (!acgsGetViewParameters(cvport(), m_pView.get()))
        acutPrintf(_T("\nFailed to copy view parameters. Using default fallback projection."));

    m_pView->setVisualStyle(acdbGetViewportVisualStyle());
    setBackgroundColorFromArray(m_pOffDevice.get(), rgb);
    m_isReady = true;
}

bool BlockImageRenderer::isValid() const
{
    return m_isReady;
}

wxImage BlockImageRenderer::render(AcDbBlockTableRecord* pBlock, double zoomFactor)
{
    if (pBlock == nullptr || !isValid())
        return wxImage{};

    if (!m_pView->add(pBlock, m_pModel.get()))
        return wxImage{};

    m_pView->setView(m_pView->position(), m_pView->target(), m_pView->upVector(), m_width, m_height);

    AcDbExtents ex = calcBlockExtents(*pBlock);
    m_pView->zoomExtents(ex.minPoint(), ex.maxPoint());
    m_pView->zoom(zoomFactor);

    m_pOffDevice->update();

    Atil::Image image(Atil::Size(m_width, m_height), &m_rgbModel, m_initialColor);
    m_pView->getSnapShot(&image, AcGsDCPoint(0, 0));

    wxImage wximage;
    if (image.isValid())
    {
        Atil::Size imageSize = image.size();
        std::unique_ptr<Atil::ImageContext> imgContext(image.createContext(Atil::ImageContext::kRead, imageSize, Atil::Offset(0, 0)));
        if (imgContext)
        {
            Atil::DataModelAttributes::PixelType pixelType = imgContext->getPixelType();
            if (pixelType == Atil::DataModelAttributes::kRgba)
            {
                wximage = wxImage(wxSize(imageSize.width, imageSize.height));
                for (Atil::Int32 y = 0; y < imageSize.height; ++y)
                {
                    for (Atil::Int32 x = 0; x < imageSize.width; ++x)
                    {
                        const Atil::RgbColor pix(imgContext->get32(x, y));
                        wximage.SetRGB(x, y, pix.rgba.red, pix.rgba.green, pix.rgba.blue);
                    }
                }
            }
        }
    }

    m_pView->erase(pBlock);
    return wximage;
}

wxImage BlockWorker::getBlockImage(AcDbObjectId id, int width, int height, double zf, const std::array<int, 3>& rgb)
{
    AcAxDocLock lock;
    AcDbBlockTableRecordPointer pBlock(id);
    if (pBlock.openStatus() != eOk)
        return wxImage{};
    BlockImageRenderer renderer(width, height, rgb);
    return renderer.render(pBlock, zf);
}

Acad::ErrorStatus BlockWorker::getBlockImages(BlockInfoArray& info, int width, int height, double zf, const std::array<int, 3>& rgb)
{
    AcAxDocLock lock;
    BlockImageRenderer renderer(width, height, rgb);
    if (!renderer.isValid())
        return eInvalidInput;

    for (auto& item : info)
    {
        AcDbBlockTableRecordPointer pBlock(item.id);
        if (pBlock.openStatus() != eOk)
            continue;
        item.preview = renderer.render(pBlock, zf);
    }
    return eOk;
}

Acad::ErrorStatus BlockWorker::getBlockInfoFromdDb(AcDbDatabase* srcDb, BlockInfoArray& info)
{
    AcAxDocLock lock;
    AcDbBlockTablePointer pBlockTablePointer(srcDb->blockTableId(), AcDb::kForRead);
    if (auto es = pBlockTablePointer.openStatus(); es != Acad::eOk)
    {
        acutPrintf(L"\nError %ls - Failed to open blockTable", acadErrorStatusText(es));
        return es;
    }

    if (auto [res, pIter] = makeBlockTableIterator(*pBlockTablePointer); res == eOk)
    {
        AcDbObjectId btrId;
        for (pIter->start(); !pIter->done(); pIter->step())
        {
            if (pIter->getRecordId(btrId) == Acad::eOk)
            {
                AcDbBlockTableRecordPointer pBtr(btrId, AcDb::kForRead);
                if (pBtr.openStatus() == Acad::eOk)
                {
                    if (pBtr->isAnonymous() ||
                        pBtr->isLayout() ||
                        pBtr->isFromExternalReference() ||
                        pBtr->isFromOverlayReference())
                    {
                        continue;
                    }

                    TCHAR* name = 0;
                    if (pBtr->getName(name) == Acad::eOk)
                    {
                        BlockInfo blkinfo;
                        blkinfo.id = btrId;
                        blkinfo.name = name;
                        info.push_back(blkinfo);
                        acutDelString(name);
                    }
                }
            }
        }
        return getBlockImages(info, 64, 64, 0.95, { 25, 25, 25 });
    }
    else
    {
        return res;
    }
}

//Ax will handle attributes and fields, we don't have to : )
static HRESULT insertBlockViaActiveX(
    const TCHAR* blockName,
    const AcGePoint3d& insPoint,
    double scale,
    double rotationRad)
{
    AcAxDocLock lock;
    HRESULT hr = S_OK;

    if (acedGetAcadWinApp() == nullptr)
        return E_FAIL;

    CComPtr<IDispatch> pAcadApp = acedGetAcadWinApp()->GetIDispatch(FALSE);
    if (!pAcadApp) return E_FAIL;

    static DISPID dispidDoc = DISPID_UNKNOWN;
    static DISPID dispidModelSpace = DISPID_UNKNOWN;
    static DISPID dispidPaperSpace = DISPID_UNKNOWN;
    static DISPID dispidInsertBlock = DISPID_UNKNOWN;

    if (dispidDoc == DISPID_UNKNOWN)
    {
        OLECHAR* szActiveDoc = const_cast<OLECHAR*>(L"ActiveDocument");
        hr = pAcadApp->GetIDsOfNames(IID_NULL, &szActiveDoc, 1, LOCALE_USER_DEFAULT, &dispidDoc);
        if (FAILED(hr)) return hr;
    }

    CComVariant varDoc;
    DISPPARAMS noArgs = { NULL, NULL, 0, 0 };
    hr = pAcadApp->Invoke(dispidDoc, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, &noArgs, &varDoc, NULL, NULL);
    if (FAILED(hr) || varDoc.vt != VT_DISPATCH) return hr;
    CComPtr<IDispatch> pDoc = varDoc.pdispVal;

    AcDbDatabase* pDb = acdbHostApplicationServices()->workingDatabase();
    if (!pDb) return E_POINTER;

    bool bTilemode = pDb->tilemode();
    DISPID* pTargetSpaceId = bTilemode ? &dispidModelSpace : &dispidPaperSpace;

    if (*pTargetSpaceId == DISPID_UNKNOWN)
    {
        OLECHAR* szSpaceName = const_cast<OLECHAR*>(bTilemode ? L"ModelSpace" : L"PaperSpace");
        hr = pDoc->GetIDsOfNames(IID_NULL, &szSpaceName, 1, LOCALE_USER_DEFAULT, pTargetSpaceId);
        if (FAILED(hr)) return hr;
    }

    CComVariant varSpace;
    hr = pDoc->Invoke(*pTargetSpaceId, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, &noArgs, &varSpace, NULL, NULL);
    if (FAILED(hr) || varSpace.vt != VT_DISPATCH) return hr;
    CComPtr<IDispatch> pSpace = varSpace.pdispVal;

    if (dispidInsertBlock == DISPID_UNKNOWN)
    {
        OLECHAR* szInsertBlock = const_cast<OLECHAR*>(L"InsertBlock");
        hr = pSpace->GetIDsOfNames(IID_NULL, &szInsertBlock, 1, LOCALE_USER_DEFAULT, &dispidInsertBlock);
        if (FAILED(hr)) return hr;
    }

    CComSafeArray<double> zeroPoint(3);
    zeroPoint.SetAt(0, insPoint.x);
    zeroPoint.SetAt(1, insPoint.y);
    zeroPoint.SetAt(2, insPoint.z);

    CComVariant avArgs[6];
    avArgs[5].vt = VT_ARRAY | VT_R8;
    avArgs[5].parray = zeroPoint.Detach();
    avArgs[4] = CComVariant(blockName);
    avArgs[3] = CComVariant(scale);
    avArgs[2] = CComVariant(scale);
    avArgs[1] = CComVariant(scale);
    avArgs[0] = CComVariant(rotationRad);

    DISPPARAMS dpInsert = { avArgs, NULL, 6, 0 };
    CComVariant varResultBlockRef;

    hr = pSpace->Invoke(dispidInsertBlock, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dpInsert, &varResultBlockRef, NULL, NULL);
    if (FAILED(hr) || varResultBlockRef.vt != VT_DISPATCH || !varResultBlockRef.pdispVal)
        return E_FAIL;
    return hr;
}

Acad::ErrorStatus BlockWorker::insertBlockTableRecord(AcDbDatabase* srcDb, const wxString& blockName, double scale, double rotation, OnScreenFlags flags)
{
    AcAxDocLock lock;
    AcGePoint3d inspoint;
    AcDbObjectId srcBlockId;
    AcDbDatabase* pDestDb = acdbCurDwg();
    AcDbBlockTable* pDestBlockTable = nullptr;

    if (!pDestDb)
        return Acad::eNoDatabase;

    // check if the block is already inserted 
    if (pDestDb->getBlockTable(pDestBlockTable, AcDb::kForRead) == Acad::eOk)
    {
        bool bBlockExists = pDestBlockTable->has(blockName.c_str());
        pDestBlockTable->getAt(blockName.c_str(), srcBlockId);
        pDestBlockTable->close();
        if (bBlockExists)
        {
            if (xformBlockJig(srcBlockId, inspoint, scale, rotation, flags) == eOk)
            {
                HRESULT hr = insertBlockViaActiveX(blockName.c_str(), inspoint, scale, rotation);
                if (SUCCEEDED(hr))
                    return Acad::eOk;
                return Acad::eInvalidInput;
            }
        }
    }

    // else wblock
    if (!srcDb)
    {
        acutPrintf(_T("\nDrawing was closed: "));
        return Acad::eNoDatabase;
    }
    AcDbBlockTable* pSrcBlockTable;
    if (srcDb->getBlockTable(pSrcBlockTable, AcDb::kForRead) != Acad::eOk)
        return Acad::eInvalidInput;

    Acad::ErrorStatus es = pSrcBlockTable->getAt(blockName.c_str(), srcBlockId);
    if (es != Acad::eOk)
        return es;
    pSrcBlockTable->close();

    AcDbDatabase* pTmpDb = nullptr;
    if (auto es = srcDb->wblock(pTmpDb, srcBlockId); es != Acad::eOk)
        return es;
    std::unique_ptr<AcDbDatabase> pSafeTmpDb(pTmpDb);

    AcDbObjectId blkId;
    if (auto es = pDestDb->insert(blkId, blockName.c_str(), pTmpDb, Adesk::kTrue); es != Acad::eOk)
        return es;

    if (xformBlockJig(srcBlockId, inspoint, scale, rotation, flags) == eOk)
    {
        HRESULT hr = insertBlockViaActiveX(blockName.c_str(), inspoint, scale, rotation);
        if (!SUCCEEDED(hr))
            return Acad::eInvalidInput;
    }
    return Acad::eOk;
}

Acad::ErrorStatus BlockWorker::insertDwg(AcDbDatabase* srcDb, double scale, double rotation, OnScreenFlags flags)
{
    if (!srcDb)
        return Acad::eNoDatabase;

    const wchar_t* filename = nullptr;
    if (auto es = srcDb->getFilename(filename); es != eOk)
        return es;

    AcAxDocLock lock;
    AcGePoint3d inspoint;
    if (xformBlockJig(srcDb->currentSpaceId(), inspoint, scale, rotation, flags) == eOk)
    {
        HRESULT hr = insertBlockViaActiveX(filename, inspoint, scale, rotation);
        if (!SUCCEEDED(hr))
            return Acad::eInvalidInput;
    }
    return Acad::eOk;
}

Acad::ErrorStatus BlockWorker::xformBlockJig(const AcDbObjectId& id, AcGePoint3d& point, double& scale, double& rotation, OnScreenFlags flags)
{
    AcAxDocLock lock;

    BlockJig jig(id, scale, rotation);
    if (jig.doit() == Acad::eNormal)
        point = jig.getPoint();

    if (GETBIT(flags, OnScreenFlags::Scale))
    {
        BlockJigScale sjig(id, point, rotation);
        if (sjig.doit() != Acad::eNormal)
            return eInvalidInput;
        scale = sjig.getScale();
    }

    if (GETBIT(flags, OnScreenFlags::Rotate))
    {
        BlockJigRotate rjig(id, point, rotation, scale);
        if (rjig.doit() != Acad::eNormal)
            return eInvalidInput;
        rotation = rjig.getRotation();
    }
    return eOk;
}
