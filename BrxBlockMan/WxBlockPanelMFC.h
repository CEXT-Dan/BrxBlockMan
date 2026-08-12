#pragma once
#include "BrxSpecific/BcUiPanelMFC.h"
#include "wx/xrc/xmlres.h"
#include "wx/dirctrl.h"
#include "wx/listctrl.h"

class wxGenericDirCtrl;
class wxListCtrl;

//---------------------------------------------------------------------
// WxBlockPanel

#define ID_BLOCKMAN -1
#define SYMBOL_WXBLOCKPANEL_STYLE wxTAB_TRAVERSAL
#define SYMBOL_WXBLOCKPANEL_TITLE _("BlockMan")
#define SYMBOL_WXBLOCKPANEL_IDNAME ID_BLOCKMAN
#define SYMBOL_WXBLOCKPANEL_SIZE wxSize(400, -1)
#define SYMBOL_WXBLOCKPANEL_POSITION wxDefaultPosition

class WxBlockPanel : public wxPanel
{
    DECLARE_DYNAMIC_CLASS(WxBlockPanel)
    DECLARE_EVENT_TABLE()

    enum {
        ID_OPEN_AUTOCAD_DOC = wxID_HIGHEST + 2000
    };

public:
    // Constructors
    WxBlockPanel();
    WxBlockPanel(wxWindow* parent, 
        wxWindowID id = SYMBOL_WXBLOCKPANEL_IDNAME, 
        const wxPoint& pos = SYMBOL_WXBLOCKPANEL_POSITION, 
        const wxSize& size = SYMBOL_WXBLOCKPANEL_SIZE, 
        long style = SYMBOL_WXBLOCKPANEL_STYLE);

    // Creation
    bool Create(wxWindow* parent, 
        wxWindowID id = SYMBOL_WXBLOCKPANEL_IDNAME, 
        const wxPoint& pos = SYMBOL_WXBLOCKPANEL_POSITION, 
        const wxSize& size = SYMBOL_WXBLOCKPANEL_SIZE, 
        long style = SYMBOL_WXBLOCKPANEL_STYLE);

    ~WxBlockPanel();
    void Init();
    void Uninit();
    void CreateControls();

    // events
    void InitDialog() override;
    void OnChoiceSelected(wxCommandEvent& event);
    void OnAddButtonClick(wxCommandEvent& event);
    void OnDirCtrlSelectionChanged(wxTreeEvent& event);
    void OnListctrlSelected(wxListEvent& event);
    void OnListctrlBeginDrag(wxListEvent& event);
    void NavigateToFolder(const wxString& folder);
    void OnChoiceRightClick(wxMouseEvent& event);
    void OnChoiceChanged(wxCommandEvent& event);
    void OnListCtrlLeftDClick(wxMouseEvent& event);
    void OnPreviewLeftDClick(wxMouseEvent& event);
    void OnDirCtrlRightClick(wxTreeEvent& event);

    // WxBlockPanel
    bool initDatabase(const wxString& selectedPath);
    void SaveChoiceSetting();
    void LoadChoiceSetting();
    double getScaleValue() const;
    double getRotationValue() const;
    bool isRosChecked() const;
    bool isSosChecked() const;

private:
    wxStaticBitmap* m_staticPreviewCtrl = nullptr;
    wxChoice* m_choiceCtrl = nullptr;
    wxButton* m_addButtonCtrl = nullptr;
    wxTextCtrl* m_rotationTextCtrl = nullptr;
    wxTextCtrl* m_scaleTextCtrl = nullptr;
    wxGenericDirCtrl* m_dirCtrl = nullptr;
    wxListCtrl* m_listCtrl = nullptr;
    wxCheckBox* m_rosCheckBoxCtrl = nullptr;
    wxCheckBox* m_sosCheckBoxCtrl = nullptr;
    std::unique_ptr<AcDbDatabase> m_selectedDb;
};

//---------------------------------------------------------------------
// WxBlockPanelMFC
class WxBlockPanelMFC : public BcUiPanelMFC
{
public:
    WxBlockPanelMFC();
    virtual ~WxBlockPanelMFC() override = default;

protected:
    DECLARE_MESSAGE_MAP()
    virtual BOOL CreateControlBar(LPCREATESTRUCT lpCreateStruct) override;
    virtual void OnSizeChanged(int cx, int cy) override;
    afx_msg void OnDestroy();

private:
    wxPanel* m_thisWin;
    WxBlockPanel* m_blkPanel;
};
