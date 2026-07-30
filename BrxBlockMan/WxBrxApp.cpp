#include "stdafx.h"
#include "WxBrxApp.h"
#include "wx/setup.h"
#include "wx/wx.h"
#include <wx/msw/darkmode.h>
#include <wx/config.h>

class WxArxDarkModeSettings : public wxDarkModeSettings
{
public:
    static COLORREF backgroundColor()
    {
        return RGB(45, 49, 53);
    }

    wxColour GetColour(wxSystemColour index) override
    {
        static wxColour clr = backgroundColor();
        switch (index)
        {
            case wxSYS_COLOUR_APPWORKSPACE:
            case wxSYS_COLOUR_INFOBK:
            case wxSYS_COLOUR_LISTBOX:
            case wxSYS_COLOUR_WINDOW:
            case wxSYS_COLOUR_BTNFACE:
                return clr;
            default:
                return wxDarkModeSettings::GetColour(index);
        }
    }
};

//------------------------------------------------------------------------------------------------
//  Attach to AutoCAD's main frame
BrxTopLevelWindow::BrxTopLevelWindow()
{
    this->SetHWND(adsw_acadMainWnd());
    this->AdoptAttributesFromHWND();
    this->m_isShown = true;
    wxTopLevelWindows.Append(this);
}

//------------------------------------------------------------------------------------------------
// the wxApp
bool WxApp::OnInit()
{
    resbuf rb;
    const auto rt = acedGetVar(_T("COLORTHEME"), &rb);
    if (rt == RTNORM && rb.restype == RTSHORT && rb.resval.rint == 0)
    {
        if (!wxTheApp->MSWEnableDarkMode(wxApp::DarkMode_Always, new WxArxDarkModeSettings()))
            acutPrintf(_T("MSWEnableDarkMode failed"));
    }
    wxTheApp->SetTopWindow(new BrxTopLevelWindow());
    if (wxTheApp->GetTopWindow() == nullptr)
        return false;
    wxTheApp->SetExitOnFrameDelete(false);
    return true;
}

int WxApp::OnExit()
{
    auto top = wxTheApp->GetTopWindow();
    if (top != nullptr)
    {
        top->DissociateHandle();
        top->SetHWND(0);
    }
    wxTopLevelWindows.Clear();
    return 0;
}

void WxApp::WakeUpIdle()
{
    const CWinApp* mfcApp = AfxGetApp();
    if (mfcApp != nullptr && mfcApp->m_pMainWnd)
    {
        ::PostMessage(mfcApp->m_pMainWnd->m_hWnd, WM_NULL, 0, 0);
    }
}

bool WxApp::initWxApp()
{
    wxApp::SetInstance(new WxApp());
    if (!wxEntryStart(_hdllInstance))
        return false;
    if (wxTheApp && wxTheApp->CallOnInit())
        return true;
    return false;
}

bool WxApp::uninitWxApp()
{
    wxTheApp->OnExit();
    wxEntryCleanup();
    return true;
}

//------------------------------------------------------------------------------------------------
//  wxArxApp
WxBrxApp& WxBrxApp::instance()
{
    static WxBrxApp mthis;
    return mthis;
}

bool WxBrxApp::init()
{
    if (auto res = WxApp::initWxApp(); !res)
    {
        acutPrintf(_T("\ninitWxApp Failed!: "));
        return false;
    }
    auto& app = WxBrxApp::instance();
    app.initResources();
    return true;
}

bool WxBrxApp::uninit()
{
    WxApp::uninitWxApp();
    return true;
}

const std::filesystem::path& WxBrxApp::modulePath()
{
    static std::filesystem::path path;
    if (path.empty())
    {
        path = WxBrxApp::moduleName();
        path.remove_filename();
    }
    return path;
}

const std::filesystem::path& WxBrxApp::moduleName()
{
    static std::filesystem::path path;
    if (path.empty())
    {
        std::wstring buffer(MAX_PATH, 0);
        GetModuleFileName(_hdllInstance, buffer.data(), buffer.size());
        path = buffer.c_str();
    }
    return path;
}

void WxBrxApp::initResources()
{
    initIcon();
    wxXmlResource::Get()->InitAllHandlers();
    auto resourcePath = WxBrxApp::modulePath() / L"WxBlockPanel.xrc";
    if (!std::filesystem::exists(resourcePath))
        resourcePath = "C:\\Users\\Dan\\Documents\\DialogBlocks Projects\\BlockMan\\WxBlockPanel.xrc";
    if (std::filesystem::exists(resourcePath))
    {
        if (!wxXmlResource::Get()->Load(wxString(resourcePath.wstring())))
            acutPrintf(_T("Failed to parse or load XRC file data.\n"));
    }
    else
    {
        acutPrintf(_T("Failed to load Resources: File does not exist.\n"));
    }
}

void WxBrxApp::initIcon()
{
    _wxIcon.CreateFromHICON(LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(31233)));
}

const wxIcon& WxBrxApp::getWxIcon() const
{
    return _wxIcon;
}

wxDECLARE_APP(WxApp);
wxIMPLEMENT_APP_NO_MAIN(WxApp);

