// (C) Copyright 2002-2007 by Autodesk, Inc. 
//
// Permission to use, copy, modify, and distribute this software in
// object code form for any purpose and without fee is hereby granted, 
// provided that the above copyright notice appears in all copies and 
// that both that copyright notice and the limited warranty and
// restricted rights notice below appear in all supporting 
// documentation.
//
// AUTODESK PROVIDES THIS PROGRAM "AS IS" AND WITH ALL FAULTS. 
// AUTODESK SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTY OF
// MERCHANTABILITY OR FITNESS FOR A PARTICULAR USE.  AUTODESK, INC. 
// DOES NOT WARRANT THAT THE OPERATION OF THE PROGRAM WILL BE
// UNINTERRUPTED OR ERROR FREE.
//
// Use, duplication, or disclosure by the U.S. Government is subject to 
// restrictions set forth in FAR 52.227-19 (Commercial Computer
// Software - Restricted Rights) and DFAR 252.227-7013(c)(1)(ii)
// (Rights in Technical Data and Computer Software), as applicable.
//

//-----------------------------------------------------------------------------
//----- acrxEntryPoint.cpp
//-----------------------------------------------------------------------------
#include "StdAfx.h"
#include "resource.h"
#include "WxBrxApp.h"
#include "WxBlockPanelMFC.h"

//-----------------------------------------------------------------------------
#define szRDS _RXST("")

//-----------------------------------------------------------------------------
//----- ObjectARX EntryPoint
class ArxTemplate : public AcRxArxApp
{
    bool On_kLoadDwgMsgCallOnce = false;

    inline static WxBlockPanelMFC m_panel;

public:
    ArxTemplate() : AcRxArxApp()
    {
    }

    virtual AcRx::AppRetCode On_kInitAppMsg(void* pkt)
    {
        AcRx::AppRetCode retCode = AcRxArxApp::On_kInitAppMsg(pkt);
        acrxLockApplication(pkt);
        WxBrxApp::init();
        return (retCode);
    }

    virtual AcRx::AppRetCode On_kUnloadAppMsg(void* pkt)
    {
        AcRx::AppRetCode retCode = AcRxArxApp::On_kUnloadAppMsg(pkt);
        if (m_panel.m_hWnd)
            m_panel.DestroyWindow();
        WxBrxApp::uninit();
        return (retCode);
    }

    virtual AcRx::AppRetCode On_kLoadDwgMsg(void* pkt) override
    {
        AcRx::AppRetCode retCode = AcRxDbxApp::On_kLoadDwgMsg(pkt);
        if (!On_kLoadDwgMsgCallOnce)
        {
            PRINTVER();
            if (!m_panel.m_hWnd)
                m_panel.Create();
            On_kLoadDwgMsgCallOnce = true;
        }
        return retCode;
    }

    virtual void RegisterServerComponents()
    {
    }

    static void PRINTVER()
    {
        acutPrintf(_T("\nBlockman version <%ls> loaded:\n"), GETVER().constPtr());
    }

    static AcString GETVER()
    {
        constexpr TCHAR MAJOR1 = '1';
        constexpr TCHAR MINOR1 = '1';
        constexpr TCHAR REVISION1 = '0', REVISION2 = '0', REVISION3 = '6';

        constexpr unsigned int compileYear = (__DATE__[7] - '0') * 1000 + (__DATE__[8] - '0') * 100 + (__DATE__[9] - '0') * 10 + (__DATE__[10] - '0');
        constexpr unsigned int compileMonth = (__DATE__[0] == 'J') ? ((__DATE__[1] == 'a') ? 1 : ((__DATE__[2] == 'n') ? 6 : 7))    // Jan, Jun or Jul
            : (__DATE__[0] == 'F') ? 2                                                              // Feb
            : (__DATE__[0] == 'M') ? ((__DATE__[2] == 'r') ? 3 : 5)                                 // Mar or May
            : (__DATE__[0] == 'A') ? ((__DATE__[1] == 'p') ? 4 : 8)                                 // Apr or Aug
            : (__DATE__[0] == 'S') ? 9                                                              // Sep
            : (__DATE__[0] == 'O') ? 10                                                             // Oct
            : (__DATE__[0] == 'N') ? 11                                                             // Nov
            : (__DATE__[0] == 'D') ? 12                                                             // Dec
            : 0;
        constexpr unsigned int compileDay = (__DATE__[4] == ' ') ? (__DATE__[5] - '0') : (__DATE__[4] - '0') * 10 + (__DATE__[5] - '0');

        constexpr TCHAR IsoDate[] =
        {
           MAJOR1, '.' , MINOR1 , '.', REVISION1, REVISION2, REVISION3,
           '.', compileYear / 1000 + '0', (compileYear % 1000) / 100 + '0', (compileYear % 100) / 10 + '0', compileYear % 10 + '0',
           compileMonth / 10 + '0', compileMonth % 10 + '0',
           compileDay / 10 + '0', compileDay % 10 + '0', 0
        };
        const AcString ver = IsoDate;
        return ver;
    }

    static void ArxTemplate_blockman(void)
    {
        if (!m_panel.m_hWnd)
        {
            m_panel.Create();
            acutPrintf(L"\nPanel has been created");
        }
        else
        {
            m_panel.DestroyWindow();
            acutPrintf(L"\nPanel has been Destroyed");
        }
    }
};

//-----------------------------------------------------------------------------
#pragma warning( disable: 4838 ) //prevents a cast compiler warning, 
IMPLEMENT_ARX_ENTRYPOINT(ArxTemplate)
ACED_ARXCOMMAND_ENTRY_AUTO(ArxTemplate, ArxTemplate, _blockman, blockman, ACRX_CMD_TRANSPARENT, NULL)
#pragma warning( pop )
