#include "wxsol.h"

/* ---- Global drawing state ---- */
wxDC* g_dc = nullptr;
X xOrgCur = 0;
Y yOrgCur = 0;
int usehdcCur = 0;
wxBitmap* g_backingStore = nullptr;

/* ---- Memory ---- */
void* PAlloc(int cb)
{
    void* p = calloc(1, cb + 200); /* +200 padding matches original */
    return p;
}

void FreeP(void* p)
{
    free(p);
}

/* ---- DC management ---- */
wxDC* DcSet(wxDC* dc, X xOrg, Y yOrg)
{
    wxDC* old = g_dc;
    g_dc = dc;
    xOrgCur = xOrg;
    yOrgCur = yOrg;
    return old;
}

/* ---- Backing store for immediate-mode drawing ---- */
/* On macOS Cocoa, wxClientDC drawing doesn't persist between paint
   cycles. We use an off-screen bitmap as a backing store: all
   immediate-mode drawing goes there, and OnPaint simply blits it. */
static wxMemoryDC* s_backingDC = nullptr;

void EnsureBackingStore()
{
    if (!g_canvas) return;
    wxSize sz = g_canvas->GetClientSize();
    if (sz.x <= 0 || sz.y <= 0) return;

    if (!g_backingStore ||
        g_backingStore->GetWidth() != sz.x ||
        g_backingStore->GetHeight() != sz.y)
    {
        delete g_backingStore;
        g_backingStore = new wxBitmap(sz.x, sz.y);
    }
}

bool FGetHdc()
{
    if (g_dc != nullptr) {
        usehdcCur++;
        return true;
    }
    if (!g_canvas)
        return false;

    EnsureBackingStore();
    if (!g_backingStore || !g_backingStore->IsOk())
        return false;

    s_backingDC = new wxMemoryDC(*g_backingStore);
    DcSet(s_backingDC, 0, 0);
    usehdcCur = 1;
    return true;
}

void ReleaseHdc()
{
    if (g_dc == nullptr)
        return;
    if (--usehdcCur == 0) {
        if (s_backingDC) {
            s_backingDC->SelectObject(wxNullBitmap);
            delete s_backingDC;
            s_backingDC = nullptr;
        }
        g_dc = nullptr;
        /* Push the backing store to screen */
        if (g_canvas) {
            g_canvas->Refresh(false);
            g_canvas->Update();
        }
    }
}

/* ---- Math helpers ---- */
int WMin(int w1, int w2) { return w1 < w2 ? w1 : w2; }
int WMax(int w1, int w2) { return w1 > w2 ? w1 : w2; }

bool FInRange(int w, int wFirst, int wLast)
{
    return w >= wFirst && w <= wLast;
}

int PegRange(int w, int wFirst, int wLast)
{
    if (w < wFirst) return wFirst;
    if (w > wLast) return wLast;
    return w;
}

/* ---- Geometry ---- */
void OffsetPt(PT* ppt, DEL* pdel, PT* pptDest)
{
    pptDest->x = ppt->x + pdel->dx;
    pptDest->y = ppt->y + pdel->dy;
}

void SwapCards(CRD* pcrd1, CRD* pcrd2)
{
    CRD t = *pcrd1;
    *pcrd1 = *pcrd2;
    *pcrd2 = t;
}

bool FPtInCrd(CRD* pcrd, PT pt)
{
    return pt.x >= pcrd->pt.x && pt.x < pcrd->pt.x + dxCrd &&
           pt.y >= pcrd->pt.y && pt.y < pcrd->pt.y + dyCrd;
}

bool PtInRC(PT* ppt, RC* prc)
{
    return ppt->x >= prc->xLeft && ppt->x < prc->xRight &&
           ppt->y >= prc->yTop  && ppt->y < prc->yBot;
}

bool FRectIsect(RC* prc1, RC* prc2)
{
    return !(prc1->xLeft >= prc2->xRight || prc2->xLeft >= prc1->xRight ||
             prc1->yTop >= prc2->yBot || prc2->yTop >= prc1->yBot);
}

void CrdRcFromPt(PT* ppt, RC* prc)
{
    prc->xRight = (prc->xLeft = ppt->x) + dxCrd;
    prc->yBot = (prc->yTop = ppt->y) + dyCrd;
}

bool FCrdRectIsect(CRD* pcrd, RC* prc)
{
    RC rcCrd;
    CrdRcFromPt(&pcrd->pt, &rcCrd);
    return FRectIsect(&rcCrd, prc);
}

/* ---- Drawing functions ---- */
void DrawCard(CRD* pcrd)
{
    if (!g_dc) return;
    cdtDrawExt(g_dc,
               pcrd->pt.x - xOrgCur,
               pcrd->pt.y - yOrgCur,
               dxCrd, dyCrd,
               pcrd->fUp ? pcrd->cd : modeFaceDown,
               pcrd->fUp ? FACEUP : FACEDOWN,
               rgbTable);
}

void DrawCardPt(CRD* pcrd, PT* ppt)
{
    if (!g_dc) return;
    unsigned long dwModeExt = 0;
    if (fKlondWinner)
        dwModeExt = 0x80000000UL;

    cdtDrawExt(g_dc,
               ppt->x - xOrgCur,
               ppt->y - yOrgCur,
               dxCrd, dyCrd,
               pcrd->fUp ? pcrd->cd : modeFaceDown,
               (pcrd->fUp ? FACEUP : FACEDOWN) | dwModeExt,
               rgbTable);
}

void DrawCardExt(PT* ppt, int cd, int mode)
{
    if (!g_dc) return;
    cdtDrawExt(g_dc,
               ppt->x - xOrgCur,
               ppt->y - yOrgCur,
               dxCrd, dyCrd,
               cd, mode, rgbTable);
}

void DrawBackground(X xLeft, Y yTop, X xRight, Y yBot)
{
    if (!g_dc) return;

    wxBrush br(wxColour(rgbTable & 0xFF, (rgbTable >> 8) & 0xFF, (rgbTable >> 16) & 0xFF));
    g_dc->SetBrush(br);
    g_dc->SetPen(*wxTRANSPARENT_PEN);
    int w = xRight - xLeft;
    int h = yBot - yTop;
    if (w > 0 && h > 0)
        g_dc->DrawRectangle(xLeft - xOrgCur, yTop - yOrgCur, w, h);
}

void DrawBackExcl(COL* pcol, PT* ppt)
{
    COLCLS* pcolcls = pcol->pcolcls;
    if (pcolcls->dxUp != 0 || pcolcls->dxDn != 0)
        DrawBackground(ppt->x + dxCrd, pcol->rc.yTop, pcol->rc.xRight, pcol->rc.yBot);
    if (pcolcls->dyUp != 0 || pcolcls->dyDn != 0)
        DrawBackground(pcol->rc.xLeft, ppt->y + dyCrd, pcol->rc.xRight, pcol->rc.yBot);
}

void DrawOutline(PT* ppt, int ccrd, DX dx, DY dy)
{
    if (!FGetHdc())
        return;

    PT pt = *ppt;

    g_dc->SetLogicalFunction(wxINVERT);
    g_dc->SetPen(*wxBLACK_PEN);
    g_dc->SetBrush(*wxTRANSPARENT_BRUSH);

    Y yBottom = pt.y + dyCrd + (ccrd - 1) * dy;
    g_dc->DrawLine(pt.x, pt.y, pt.x + dxCrd, pt.y);
    g_dc->DrawLine(pt.x + dxCrd, pt.y, pt.x + dxCrd, yBottom);
    g_dc->DrawLine(pt.x + dxCrd, yBottom, pt.x, yBottom);
    g_dc->DrawLine(pt.x, yBottom, pt.x, pt.y);

    Y yy = pt.y;
    while (--ccrd) {
        yy += dy;
        g_dc->DrawLine(pt.x, yy, pt.x + dxCrd, yy);
    }
    g_dc->SetLogicalFunction(wxCOPY);

    ReleaseHdc();
}

void EraseScreen()
{
    if (!g_canvas) return;
    if (!FGetHdc())
        return;
    wxSize sz = g_canvas->GetClientSize();
    DrawBackground(0, 0, sz.x, sz.y);
    ReleaseHdc();
}

void InvertRc(RC* prc)
{
    if (!g_dc) return;
    g_dc->SetLogicalFunction(wxINVERT);
    g_dc->SetBrush(*wxBLACK_BRUSH);
    g_dc->SetPen(*wxTRANSPARENT_PEN);
    g_dc->DrawRectangle(prc->xLeft, prc->yTop,
                         prc->xRight - prc->xLeft,
                         prc->yBot - prc->yTop);
    g_dc->SetLogicalFunction(wxCOPY);
}

/* ---- Status bar ---- */
void StatUpdate()
{
    if (g_frame)
        g_frame->UpdateStatusText();
}

void StatStringSz(const wxString& sz)
{
    if (g_frame && g_frame->GetStatusBar())
        g_frame->SetStatusText(sz);
}

/* ---- Alert dialogs ---- */
bool FValidCol(COL* pcol)
{
    return pcol != nullptr;
}
