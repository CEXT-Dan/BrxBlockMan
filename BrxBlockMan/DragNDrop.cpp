#include "stdafx.h"
#include "DragNDrop.h"


BOOL CBlkFileDropTarget::OnDrop(CWnd* pWnd, COleDataObject* pDataObject,
    DROPEFFECT dropEffect, CPoint point)
{
    return TRUE;
}

DROPEFFECT  CBlkFileDropTarget::OnDropEx(CWnd* pWnd, COleDataObject* pDataObject,
    DROPEFFECT dropDefault, DROPEFFECT dropList, CPoint point)
{
    return DROPEFFECT_MOVE; 
}

DROPEFFECT CBlkFileDropTarget::OnDragEnter(CWnd* pWnd, COleDataObject* pDataObject,
    DWORD dwKeyState, CPoint point)
{
    return DROPEFFECT_MOVE;
}

DROPEFFECT CBlkFileDropTarget::OnDragOver(CWnd* pWnd, COleDataObject* pDataObject,
    DWORD dwKeyState, CPoint point)
{
    return DROPEFFECT_MOVE;
}

void  CBlkFileDropTarget::OnDragLeave(CWnd* pWnd)
{
}