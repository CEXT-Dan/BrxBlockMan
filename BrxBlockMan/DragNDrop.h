#pragma once
class CBlkFileDropTarget : public COleDropTarget
{
    virtual BOOL OnDrop(CWnd* pWnd, COleDataObject* pDataObject,
        DROPEFFECT dropEffect, CPoint point);
    virtual DROPEFFECT OnDropEx(CWnd* pWnd, COleDataObject* pDataObject,
        DROPEFFECT dropDefault, DROPEFFECT dropList, CPoint point);
    virtual DROPEFFECT OnDragEnter(CWnd* pWnd, COleDataObject* pDataObject,
        DWORD dwKeyState, CPoint point);
    virtual DROPEFFECT OnDragOver(CWnd* pWnd, COleDataObject* pDataObject,
        DWORD dwKeyState, CPoint point);
    virtual void OnDragLeave(CWnd* pWnd);

};