#include "stdafx.h"
#include "WxBlockPanelMFC.h"
#include "wx/imaglist.h"
#include "BlockWorker.h"
#include "DragNDrop.h"
#include <wx/msw/regconf.h>

//---------------------------------------------------------------------
// WxBlockPanel
IMPLEMENT_DYNAMIC_CLASS(WxBlockPanel, wxPanel)

BEGIN_EVENT_TABLE(WxBlockPanel, wxPanel)
EVT_CHOICE(XRCID("ID_CHOICE"), WxBlockPanel::OnChoiceSelected)
EVT_BUTTON(XRCID("ID_ADD_BUTTON"), WxBlockPanel::OnAddButtonClick)
EVT_LIST_ITEM_SELECTED(XRCID("ID_LISTCTRL"), WxBlockPanel::OnListctrlSelected)
EVT_LIST_BEGIN_DRAG(XRCID("ID_LISTCTRL"), WxBlockPanel::OnListctrlBeginDrag)
EVT_DIRCTRL_SELECTIONCHANGED(XRCID("ID_DIRCTRL"), WxBlockPanel::OnDirCtrlSelectionChanged)
END_EVENT_TABLE()

WxBlockPanel::WxBlockPanel()
{
    Init();
}

WxBlockPanel::WxBlockPanel(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
{
    Init();
    Create(parent, id, pos, size, style);
    InitDialog();
}

bool WxBlockPanel::Create(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
{
    CAcModuleResourceOverride rsrc;
    SetExtraStyle(wxWS_EX_VALIDATE_RECURSIVELY);
    SetParent(parent);
    CreateControls();
    if (GetSizer())
    {
        GetSizer()->SetSizeHints(this);
    }
    Centre();
    return true;
}

WxBlockPanel::~WxBlockPanel()
{
}

void WxBlockPanel::Init()
{
    m_staticPreviewCtrl = NULL;
    m_choiceCtrl = NULL;
    m_addButtonCtrl = NULL;
    m_rotationTextCtrl = NULL;
    m_scaleTextCtrl = NULL;
    m_dirCtrl = NULL;
    m_listCtrl = NULL;
}

void WxBlockPanel::Uninit()
{
    m_selectedDb.reset();
}

bool WxBlockPanel::initDatabase(const wxString& selectedPath)
{
    m_selectedDb.reset(new AcDbDatabase(Adesk::kFalse, Adesk::kTrue));
    if (auto es = m_selectedDb->readDwgFile((const wchar_t*)selectedPath, _SH_DENYNO); es != Acad::eOk)
    {
        acutPrintf(L"\nError %ls - Failed to open drawing", acadErrorStatusText(es));
        return false;
    }
    if (auto es = m_selectedDb->closeInput(true); es != Acad::eOk)
    {
        acutPrintf(L"\nError %ls - Failed to close database", acadErrorStatusText(es));
        return false;
    }
    return true;
}

void WxBlockPanel::CreateControls()
{
    CAcModuleResourceOverride rsrc;
    if (!wxXmlResource::Get()->LoadPanel(this, GetParent(), wxT("ID_BLOCKMAN")))
        wxLogError(wxT("Missing wxXmlResource::Get()->Load() in OnInit()?"));

    m_staticPreviewCtrl = XRCCTRL(*this, "ID_STATIC_PREVIEW", wxStaticBitmap);
    m_choiceCtrl = XRCCTRL(*this, "ID_CHOICE", wxChoice);
    m_addButtonCtrl = XRCCTRL(*this, "ID_ADD_BUTTON", wxButton);
    m_rotationTextCtrl = XRCCTRL(*this, "ID_ROTATION_TEXTCTRL", wxTextCtrl);
    m_scaleTextCtrl = XRCCTRL(*this, "ID_SCALE_TEXTCTRL", wxTextCtrl);
    m_dirCtrl = XRCCTRL(*this, "ID_DIRCTRL", wxGenericDirCtrl);
    m_listCtrl = XRCCTRL(*this, "ID_LISTCTRL", wxListCtrl);

    // Bind
    if (m_dirCtrl)
    {
        wxTreeCtrl* internalTree = m_dirCtrl->GetTreeCtrl();
        if (internalTree)
            internalTree->Bind(wxEVT_TREE_ITEM_RIGHT_CLICK, &WxBlockPanel::OnDirCtrlRightClick, this);
    }
    m_choiceCtrl->Bind(wxEVT_RIGHT_DOWN, &WxBlockPanel::OnChoiceRightClick, this);
    m_listCtrl->Bind(wxEVT_LEFT_DCLICK, &WxBlockPanel::OnListCtrlLeftDClick, this);
    m_staticPreviewCtrl->Bind(wxEVT_LEFT_DCLICK, &WxBlockPanel::OnPreviewLeftDClick, this);
    m_choiceCtrl->Bind(wxEVT_CHOICE, &WxBlockPanel::OnChoiceChanged, this);

    LoadChoiceSetting();
}

void WxBlockPanel::InitDialog()
{
    CAcModuleResourceOverride rsrc;
    wxRect parentRect = GetParent()->GetRect();
    SetSize(parentRect);
    this->Update();
}

void WxBlockPanel::OnChoiceSelected(wxCommandEvent& event)
{
    CAcModuleResourceOverride rsrc;
    wxString folderPath = event.GetString();
    if (!folderPath.IsEmpty())
    {
        NavigateToFolder(folderPath);
    }
    event.Skip();
}

void WxBlockPanel::OnAddButtonClick(wxCommandEvent& event)
{
    CAcModuleResourceOverride rsrc;
    wxDirDialog dirDlg(this,
        "Choose a Folder",
        m_dirCtrl->GetPath(),
        wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);

    if (dirDlg.ShowModal() == wxID_OK)
    {
        wxString result = dirDlg.GetPath();
       
        if (int existingIndex = m_choiceCtrl->FindString(result); existingIndex == wxNOT_FOUND)
        {
            int newIndex = m_choiceCtrl->Append(result);
            m_choiceCtrl->SetSelection(newIndex);
        }
        else
        {
            m_choiceCtrl->SetSelection(existingIndex);
        }
        NavigateToFolder(result);
        SaveChoiceSetting();
    }
}

void WxBlockPanel::OnDirCtrlSelectionChanged(wxTreeEvent& event)
{
    struct CachedFileData {
        wxBitmap modelPreview;
        BlockInfoArray blockInfo;
    };
    static std::unordered_map<wxString, CachedFileData> sessionCache;

    AcAxDocLock lock;
    CAcModuleResourceOverride rsrc;
    wxGenericDirCtrl* dirCtrl = wxDynamicCast(event.GetEventObject(), wxGenericDirCtrl);

    if (!dirCtrl) 
    {
        m_selectedDb.reset();
        event.Skip();
        return;
    }

    wxString selectedPath = dirCtrl->GetPath();
    if (selectedPath.IsEmpty() || !selectedPath.EndsWith(".dwg")) 
    {
        m_selectedDb.reset();
        event.Skip();
        return;
    }

    if (!initDatabase(selectedPath)) 
    {
        m_selectedDb.reset();
        event.Skip();
        return;
    }

    bool inCache = sessionCache.contains(selectedPath);
    if (!inCache)
    {
        CachedFileData newData;
        AcDbObjectId msid = acdbSymUtil()->blockModelSpaceId(m_selectedDb.get());
       
        if (wxImage modelImage = BlockWorker::getBlockImage(msid, 400, 225, 1.0, { 25, 25, 25 }); modelImage.IsOk())
        {
            newData.modelPreview = wxBitmap(modelImage);
        }
        if (BlockWorker::getBlockInfoFromdDb(m_selectedDb.get(), newData.blockInfo) == eOk) 
        {
            sessionCache[selectedPath] = std::move(newData);
        }
        else 
        {
            m_selectedDb.reset();
            event.Skip();
            return;
        }
    }

    const auto& data = sessionCache.at(selectedPath);

    if (data.modelPreview.IsOk()) {
        m_staticPreviewCtrl->SetBitmap(data.modelPreview);
    }

    {// free the old wxImageList
        m_listCtrl->DeleteAllItems();
        m_listCtrl->AssignImageList(nullptr, wxIMAGE_LIST_NORMAL);
    }

    if (!data.blockInfo.empty()) {
        auto* imageList = new wxImageList(64, 64, false, static_cast<int>(data.blockInfo.size()));
        long itemIndex = 0;

        for (const auto& item : data.blockInfo) {
            int imgIdx = -1;
            if (item.preview.IsOk()) {
                imgIdx = imageList->Add(wxBitmap{ item.preview });
            }
            m_listCtrl->InsertItem(itemIndex, item.name, imgIdx);
            itemIndex++;
        }
        m_listCtrl->AssignImageList(imageList, wxIMAGE_LIST_NORMAL);
    }
    event.Skip();
}

void WxBlockPanel::OnListctrlSelected(wxListEvent& event)
{
    event.Skip();
}

void WxBlockPanel::OnListctrlBeginDrag(wxListEvent& event)
{
    CAcModuleResourceOverride rsrc;
    long itemIndex = event.GetIndex();
    if (itemIndex != wxNOT_FOUND)
    {
        COleDataSource source;
        CBlkFileDropTarget dropTarget;

        if (!acedStartOverrideDropTarget(&dropTarget))
            acutPrintf(_T("Error in overriding Custom drop target!\n"));

        DROPEFFECT dwEffect = source.DoDragDrop(DROPEFFECT_NONE | DROPEFFECT_MOVE);

        if (!acedEndOverrideDropTarget(&dropTarget))
            acutPrintf(_T("Error in ending override drop target\n"));

        if (dwEffect)
        {
            wxString blockName = m_listCtrl->GetItemText(itemIndex, 0);
            if (BlockWorker::insertBlockTableRecord(m_selectedDb.get(), blockName, getScaleValue(), getRotationValue()) == eOk)
                acutPrintf(_T("\n"));
            else
                acutPrintf(_T("\nOops, Something went wrong"));
        }
    }
    event.Skip();
}

void WxBlockPanel::NavigateToFolder(const wxString& folder)
{
    CAcModuleResourceOverride rsrc;
    if (!wxDirExists(folder))
    {
        acutPrintf(L"\nNavigateToFolder failed: Path does not exist -> %ls", (const wchar_t*)folder);
        return;
    }
    m_dirCtrl->CollapseTree();
    m_dirCtrl->SelectPath(folder);
    m_dirCtrl->ExpandPath(folder);
}

void WxBlockPanel::OnChoiceRightClick(wxMouseEvent& event)
{
    CAcModuleResourceOverride rsrc;
    if (!m_choiceCtrl || m_choiceCtrl->IsEmpty()) return;

    int response = wxMessageBox(
        L"Are you sure you want to clear all favorite folders?",
        L"Clear Favorites",
        wxYES_NO | wxICON_QUESTION,
        this
    );

    if (response == wxYES)
    {
        m_choiceCtrl->Clear();

        wxRegConfig config(wxT("Blockman"), wxT("CADExt"));

        config.DeleteEntry(wxT("/Settings/FavoritesCount"));
        config.DeleteEntry(wxT("/Settings/ActiveFavorite"));
        config.DeleteGroup(wxT("/Settings"));
        config.Flush();
    }
}

void WxBlockPanel::OnChoiceChanged(wxCommandEvent& event)
{
    CAcModuleResourceOverride rsrc;
    if (wxString activeFolder = event.GetString(); !activeFolder.IsEmpty())
    {
        NavigateToFolder(activeFolder);
    }
    SaveChoiceSetting();
}

void WxBlockPanel::OnListCtrlLeftDClick(wxMouseEvent& event)
{
    CAcModuleResourceOverride rsrc;
    wxPoint pos = event.GetPosition();
    int flags = 0;
    long itemIndex = m_listCtrl->HitTest(pos, flags);
    if (itemIndex != wxNOT_FOUND)
    {
        wxString blockName = m_listCtrl->GetItemText(itemIndex, 0);
        if(BlockWorker::insertBlockTableRecord(m_selectedDb.get(), blockName, getScaleValue(), getRotationValue()) == eOk)
            acutPrintf(_T("\n"));
        else
            acutPrintf(_T("\nOops, Something went wrong"));
    }
    event.Skip();
}

void WxBlockPanel::OnPreviewLeftDClick(wxMouseEvent& event)
{
    CAcModuleResourceOverride rsrc;
    if (BlockWorker::insertDwg(m_selectedDb.get(), getScaleValue(), getRotationValue()) == eOk)
        acutPrintf(_T("\n"));
    else
        acutPrintf(_T("\nOops, Something went wrong"));
    event.Skip();
}

void WxBlockPanel::OnDirCtrlRightClick(wxTreeEvent& event)
{
    CAcModuleResourceOverride rsrc;
    wxString path = m_dirCtrl->GetPath();
    if (path.IsEmpty()) return;
    wxString lowerPath = path.Lower();
    if (!lowerPath.EndsWith(".dwg"))
        return;
    m_selectedDb.reset();
    if (acDocManagerPtr()->isApplicationContext())
        acDocManagerPtr()->appContextOpenDocument(path.t_str());
    else
        acutPrintf(_T("\n[Error] Failed to acquire main Application Context.\n"));
}

void WxBlockPanel::SaveChoiceSetting()
{
    CAcModuleResourceOverride rsrc;
    if (!m_choiceCtrl) 
        return;

    wxRegConfig config(wxT("Blockman"), wxT("CADExt"));

    int itemCount = m_choiceCtrl->GetCount();
    config.Write(wxT("/Settings/FavoritesCount"), itemCount);

    for (int i = 0; i < itemCount; ++i)
    {
        wxString key = wxString::Format(wxT("/Settings/FavoriteFolder_%d"), i);
        config.Write(key, m_choiceCtrl->GetString(i));
    }
    if (int selectionIndex = m_choiceCtrl->GetSelection(); selectionIndex != wxNOT_FOUND)
    {
        config.Write(wxT("/Settings/ActiveFavorite"), m_choiceCtrl->GetString(selectionIndex));
    }
    else
    {
        config.Write(wxT("/Settings/ActiveFavorite"), wxEmptyString);
    }

    config.Flush(); // Commits into HKEY_CURRENT_USER\Software\CADExt\Blockman
}

void WxBlockPanel::LoadChoiceSetting()
{
    CAcModuleResourceOverride rsrc;
    if (!m_choiceCtrl) 
        return;

    m_choiceCtrl->Clear();
    wxRegConfig config(wxT("Blockman"), wxT("CADExt"));
    int itemCount = config.ReadLong(wxT("/Settings/FavoritesCount"), 0);

    for (int i = 0; i < itemCount; ++i)
    {
        wxString key = wxString::Format(wxT("/Settings/FavoriteFolder_%d"), i);
        wxString folderPath = config.Read(key, wxEmptyString);

        if (!folderPath.IsEmpty() && wxDirExists(folderPath))
        {
            m_choiceCtrl->Append(folderPath);
        }
    }

    if (wxString activeFolder = config.Read(wxT("/Settings/ActiveFavorite"), wxEmptyString); !activeFolder.IsEmpty())
    {
        int index = m_choiceCtrl->FindString(activeFolder);
        if (index != wxNOT_FOUND)
        {
            m_choiceCtrl->SetSelection(index);
            NavigateToFolder(activeFolder);
        }
    }
}

double WxBlockPanel::getScaleValue() const
{
    CAcModuleResourceOverride rsrc;
    double scale = 1.0;
    auto str = m_scaleTextCtrl->GetValue();
    if (!str.ToDouble(&scale) || std::fabs(scale) < 1e-9)
    {
        acutPrintf(L"\nConversion failed or scale is zero: defaulting to 1.0 -> getScaleValue");
        return 1.0;
    }
    return scale;
}

double WxBlockPanel::getRotationValue() const
{
    CAcModuleResourceOverride rsrc;
    double deg = 0.0;
    auto str = m_rotationTextCtrl->GetValue();
    if (!str.ToDouble(&deg))
    {
        acutPrintf(L"\nConversion failed: -> getRotationValue");
        return 0.0;
    }
    return deg * (M_PI / 180.0);
}

BEGIN_MESSAGE_MAP(WxBlockPanelMFC, BcUiPanelMFC)
    ON_WM_DESTROY()
END_MESSAGE_MAP()

//---------------------------------------------------------------------
// WxBlockPanelMFC
WxBlockPanelMFC::WxBlockPanelMFC()
    : BcUiPanelMFC(ACRX_T("BockPanel"), ACRX_T("WxBlockPanel"))
{
}

BOOL WxBlockPanelMFC::CreateControlBar(LPCREATESTRUCT lpCreateStruct)
{
    CAcModuleResourceOverride rsrc;
    if (!BcUiPanelMFC::CreateControlBar(lpCreateStruct))
        return FALSE;
    m_thisWin = new wxPanel();
    m_thisWin->SetHWND(this->m_hWnd);
    m_thisWin->AdoptAttributesFromHWND();
    m_blkPanel = new WxBlockPanel(m_thisWin);
    return TRUE;
}

void WxBlockPanelMFC::OnSizeChanged(int cx, int cy)
{
    CAcModuleResourceOverride rsrc;
    BcUiPanelMFC::OnSizeChanged(cx, cy);
    CRect rcClient;
    GetClientRect(&rcClient);
    wxRect wxR(rcClient.left, rcClient.top, rcClient.Width(), rcClient.Height());
    m_blkPanel->SetSize(wxR);
}

void WxBlockPanelMFC::OnDestroy()
{
    m_blkPanel->Uninit();
    CWnd::OnDestroy();
}
