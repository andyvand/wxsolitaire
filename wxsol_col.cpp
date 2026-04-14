#include "wxsol.h"

bool fMegaDiscardHack = false;
MOVE g_move = {};

/* ---- Column class/instance creation ---- */
COLCLS* PcolclsCreate(int tcls, ColProc_t lpfnColProc,
                       DX dxUp, DY dyUp, DX dxDn, DY dyDn,
                       int dcrdUp, int dcrdDn)
{
    COLCLS* pcolcls = (COLCLS*)PAlloc(sizeof(COLCLS));
    if (!pcolcls) return nullptr;

    pcolcls->tcls = tcls;
    pcolcls->lpfnColProc = lpfnColProc;
    pcolcls->ccolDep = 0;
    pcolcls->dxUp = dxUp;
    pcolcls->dyUp = dyUp;
    pcolcls->dxDn = dxDn;
    pcolcls->dyDn = dyDn;
    pcolcls->dcrdUp = dcrdUp;
    pcolcls->dcrdDn = dcrdDn;
    return pcolcls;
}

COL* PcolCreate(COLCLS* pcolcls, X xLeft, Y yTop, X xRight, Y yBot, int icrdMax)
{
    COL* pcol = (COL*)PAlloc(sizeof(COL) + (icrdMax - 1) * sizeof(CRD));
    if (!pcol) return nullptr;

    if ((pcol->pcolcls = pcolcls) != nullptr)
        pcol->lpfnColProc = pcolcls->lpfnColProc;

    pcol->rc.xLeft = xLeft;
    pcol->rc.yTop = yTop;
    pcol->rc.xRight = xRight;
    pcol->rc.yBot = yBot;
    pcol->icrdMax = icrdMax;
    pcol->icrdMac = 0;
    pcol->pmove = nullptr;
    if (pcol->pcolcls)
        pcol->pcolcls->ccolDep++;
    return pcol;
}

/* ---- Default column procedures ---- */

static bool DefFreeCol(COL* pcol)
{
    COLCLS* pcolcls = pcol->pcolcls;
    if (pcolcls) {
        if (--pcolcls->ccolDep == 0)
            FreeP(pcolcls);
    }
    FreeP(pcol);
    return true;
}

static int DefHit(COL* pcol, PT* ppt, int icrdMin)
{
    if (!PtInRC(ppt, &pcol->rc) || pcol->icrdMac == 0)
        return icrdNil;

    for (int icrd = pcol->icrdMac - 1; icrd >= icrdMin; icrd--) {
        CRD* pcrd = &pcol->rgcrd[icrd];
        if (!pcrd->fUp)
            break;
        if (FPtInCrd(pcrd, *ppt)) {
            g_move.ccrdSel = pcol->icrdMac - icrd;
            PT ptCrd = pcol->rgcrd[icrd].pt;
            g_move.delHit.dx = ptCrd.x - ppt->x;
            g_move.delHit.dy = ptCrd.y - ppt->y;

            if (fOutlineDrag)
                goto Return;

            /* Full drag: capture card images and background */
            if (g_canvas) {
                g_move.dyCol = dyCrd + (g_move.ccrdSel - 1) * pcol->pcolcls->dyUp;

                /* Render the card column to a bitmap */
                wxBitmap bmpCol(dxCrd, g_move.dyCol);
                {
                    wxMemoryDC mdc(bmpCol);
                    /* Draw background then cards */
                    wxBrush br(wxColour(rgbTable & 0xFF, (rgbTable >> 8) & 0xFF, (rgbTable >> 16) & 0xFF));
                    mdc.SetBackground(br);
                    mdc.Clear();

                    wxDC* oldDc = g_dc;
                    int oldUse = usehdcCur;
                    usehdcCur = 1;
                    g_dc = &mdc;
                    xOrgCur = ptCrd.x;
                    yOrgCur = ptCrd.y;
                    for (int i = icrd; i < pcol->icrdMac; i++)
                        DrawCard(&pcol->rgcrd[i]);
                    g_dc = oldDc;
                    xOrgCur = 0;
                    yOrgCur = 0;
                    usehdcCur = oldUse;
                }
                delete g_move.bmpCol;
                g_move.bmpCol = new wxBitmap(bmpCol);

                /* Render the background (what's under the cards) */
                wxBitmap bmpSave(dxCrd, g_move.dyCol);
                {
                    wxMemoryDC mdc(bmpSave);
                    wxBrush br(wxColour(rgbTable & 0xFF, (rgbTable >> 8) & 0xFF, (rgbTable >> 16) & 0xFF));
                    mdc.SetBackground(br);
                    mdc.Clear();

                    wxDC* oldDc = g_dc;
                    int oldUse = usehdcCur;
                    usehdcCur = 1;
                    g_dc = &mdc;
                    xOrgCur = ptCrd.x;
                    yOrgCur = ptCrd.y;
                    DrawBackground(ptCrd.x, ptCrd.y, pcol->rc.xRight, pcol->rc.yBot);
                    SendColMsg(pcol, msgcRender, icrd - 1, WMax(0, icrd));
                    g_dc = oldDc;
                    xOrgCur = 0;
                    yOrgCur = 0;
                    usehdcCur = oldUse;
                }
                delete g_move.bmpScreenSave;
                g_move.bmpScreenSave = new wxBitmap(bmpSave);

                delete g_move.bmpT;
                g_move.bmpT = new wxBitmap(dxCrd, g_move.dyCol);
                g_move.fHdc = true;
            }

        Return:
            pcol->pmove = &g_move;
            g_move.icrdSel = icrd;
            return icrd;
        }
    }
    return icrdNil;
}

static bool DefMouseUp(COL* pcol, PT* pptPrev, bool fRender)
{
    if (fRender)
        SendColMsg(pcol, msgcZip, 0, 0);

    if (fOutlineDrag) {
        if (pptPrev->x != ptNil.x)
            SendColMsg(pcol, msgcDrawOutline, (intptr_t)pptPrev, (intptr_t)&ptNil);
        return true;
    }

    MOVE* pmove = pcol->pmove;
    if (pmove && pmove->fHdc && g_canvas) {
        if (pptPrev->x != ptNil.x) {
            /* Restore background at old position on backing store */
            EnsureBackingStore();
            if (g_backingStore && g_backingStore->IsOk() &&
                pmove->bmpScreenSave && pmove->bmpScreenSave->IsOk()) {
                wxMemoryDC cdc(*g_backingStore);
                wxMemoryDC mdc(*pmove->bmpScreenSave);
                cdc.Blit(pptPrev->x + pmove->delHit.dx,
                         pptPrev->y + pmove->delHit.dy,
                         dxCrd, pmove->dyCol, &mdc, 0, 0, wxCOPY);
                cdc.SelectObject(wxNullBitmap);
            }
        }

        if (fRender)
            SendColMsg(pcol, msgcRender, pmove->icrdSel - 1, icrdToEnd);
    }
    return true;
}

static bool DefRemove(COL* pcol, COL* pcolTemp)
{
    int icrdSel = pcol->pmove->icrdSel;
    int ccrdSel = pcol->pmove->ccrdSel;
    bltb(&pcol->rgcrd[icrdSel], &pcolTemp->rgcrd[0], sizeof(CRD) * ccrdSel);
    pcolTemp->icrdMac = ccrdSel;

    int ccrdShiftDown = pcol->icrdMac - (icrdSel + ccrdSel);
    if (ccrdShiftDown > 0)
        bltb(&pcol->rgcrd[icrdSel + ccrdSel], &pcol->rgcrd[icrdSel], sizeof(CRD) * ccrdShiftDown);
    pcol->icrdMac -= ccrdSel;
    return true;
}

static bool DefInsert(COL* pcol, COL* pcolTemp, int icrd)
{
    int icrdT = icrd == icrdToEnd ? pcol->icrdMac : icrd;

    if (icrd != icrdToEnd)
        bltb(&pcol->rgcrd[icrdT], &pcol->rgcrd[icrdT + pcolTemp->icrdMac],
             sizeof(CRD) * pcolTemp->icrdMac);
    else
        icrd = pcol->icrdMac;

    bltb(&pcolTemp->rgcrd[0], &pcol->rgcrd[icrdT], sizeof(CRD) * pcolTemp->icrdMac);
    pcol->icrdMac += pcolTemp->icrdMac;
    pcolTemp->icrdMac = 0;
    return true;
}

static bool DefMove(COL* pcolDest, COL* pcolSrc, int icrd)
{
    bool fZip = icrd & bitFZip;
    icrd &= icrdMask;
    int icrdSelSav = WMax(pcolSrc->pmove->icrdSel - 1, 0);
    int icrdMacDestSav = (icrd == icrdToEnd) ? pcolDest->icrdMac : icrd;

    COL* pcolTemp = PcolCreate(nullptr, 0, 0, 0, 0, pcolSrc->pmove->ccrdSel);
    if (!pcolTemp)
        return false;

    bool fResult =
        SendColMsg(pcolSrc, msgcRemove, (intptr_t)pcolTemp, 0) &&
        SendColMsg(pcolDest, msgcInsert, (intptr_t)pcolTemp, icrd) &&
        SendColMsg(pcolDest, msgcComputeCrdPos, icrdMacDestSav, false) &&
        (!fZip || SendColMsg(pcolSrc, msgcZip, 0, 0)) &&
        (!fOutlineDrag || SendColMsg(pcolSrc, msgcRender, icrdSelSav, icrdToEnd)) &&
        SendColMsg(pcolDest, msgcRender, icrdMacDestSav, icrdToEnd) &&
        SendColMsg(pcolSrc, msgcEndSel, false, 0);
    FreeP(pcolTemp);
    return fResult;
}

static bool DefCopy(COL* pcolDest, COL* pcolSrc, bool fAll)
{
    if (fAll)
        bltb(pcolSrc, pcolDest, sizeof(COL) + (pcolSrc->icrdMac - 1) * sizeof(CRD));
    else {
        bltb(pcolSrc->rgcrd, pcolDest->rgcrd, pcolSrc->icrdMac * sizeof(CRD));
        pcolDest->icrdMac = pcolSrc->icrdMac;
    }
    return SendColMsg(pcolDest, msgcRender, 0, icrdToEnd);
}

static bool DefRender(COL* pcol, int icrdFirst, int icrdLast)
{
    icrdFirst = WMax(icrdFirst, 0);
    if (!FGetHdc())
        return false;

    if (pcol->icrdMac == 0 || icrdLast == 0) {
        DrawBackground(pcol->rc.xLeft, pcol->rc.yTop, pcol->rc.xRight, pcol->rc.yBot);
    } else {
        int icrdMac = WMin(pcol->icrdMac, icrdLast);
        CRD* pcrd = nullptr;
        CRD* pcrdPrev = nullptr;

        for (int icrd = icrdFirst; icrd < icrdMac; icrd++) {
            pcrd = &pcol->rgcrd[icrd];
            if (icrd == icrdFirst ||
                pcrd->pt.x != pcrdPrev->pt.x || pcrd->pt.y != pcrdPrev->pt.y ||
                pcrd->fUp)
                DrawCard(pcrd);
            pcrdPrev = pcrd;
        }
    }
    /* EraseExtra: erase background beyond last card */
    {
        if ((pgmCur->fDealt || pcol->pcolcls->tcls == tclsDeck) && pcol->icrdMac > 0) {
            CRD* pcrd2 = &pcol->rgcrd[icrdLast == 0 ? 0 : WMin(pcol->icrdMac, icrdLast) - 1];
            DrawBackExcl(pcol, &pcrd2->pt);
        }
    }
    ReleaseHdc();
    return true;
}

static bool DefPaint(COL* pcol, RC* ppaintRC)
{
    int icrd;
    if (ppaintRC == nullptr)
        icrd = 0;
    else {
        if (!FRectIsect(&pcol->rc, ppaintRC))
            return false;
        if (pcol->icrdMac == 0)
            icrd = 0;
        else {
            for (icrd = 0; icrd < pcol->icrdMac; icrd++)
                if (FCrdRectIsect(&pcol->rgcrd[icrd], ppaintRC))
                    break;
            if (icrd == pcol->icrdMac)
                return false;
        }
    }
    return SendColMsg(pcol, msgcRender, icrd, icrdToEnd);
}

static bool DefDrawOutline(COL* pcol, PT* ppt, PT* pptPrev)
{
    MOVE* pmove = pcol->pmove;
    if (!pmove) return false;

    PT pt, ptPrev;
    OffsetPt(ppt, &pmove->delHit, &pt);
    if (pptPrev->x != ptNil.x)
        OffsetPt(pptPrev, &pmove->delHit, &ptPrev);

    if (fOutlineDrag) {
        DrawOutline(&pt, pmove->ccrdSel, 0, pcol->pcolcls->dyUp);
        if (pptPrev->x != ptNil.x)
            DrawOutline(&ptPrev, pmove->ccrdSel, 0, pcol->pcolcls->dyUp);
        return true;
    }

    /* Full drag with bitmaps - draw to backing store */
    if (!g_canvas || !pmove->bmpCol || !pmove->bmpScreenSave || !pmove->bmpT)
        return true;

    EnsureBackingStore();
    if (!g_backingStore || !g_backingStore->IsOk())
        return true;

    wxMemoryDC hdc(*g_backingStore);

    /* Save screen at new position to temp */
    wxMemoryDC mdcT(*pmove->bmpT);
    mdcT.Blit(0, 0, dxCrd, pmove->dyCol, &hdc, pt.x, pt.y, wxCOPY);

    if (pptPrev->x != ptNil.x) {
        DEL del;
        del.dx = ptPrev.x - pt.x;
        del.dy = ptPrev.y - pt.y;
        /* Composite old saved background into temp at offset */
        wxMemoryDC mdcSave(*pmove->bmpScreenSave);
        mdcT.Blit(del.dx, del.dy, dxCrd, pmove->dyCol, &mdcSave, 0, 0, wxCOPY);
        /* Draw cards onto old save buffer for compositing */
        wxMemoryDC mdcCol(*pmove->bmpCol);
        mdcSave.Blit(-del.dx, -del.dy, dxCrd, pmove->dyCol, &mdcCol, 0, 0, wxCOPY);
    }

    /* Draw cards at new position */
    {
        wxMemoryDC mdcCol(*pmove->bmpCol);
        hdc.Blit(pt.x, pt.y, dxCrd, pmove->dyCol, &mdcCol, 0, 0, wxCOPY);
    }

    /* Restore old position from saved background */
    if (pptPrev->x != ptNil.x) {
        wxMemoryDC mdcSave(*pmove->bmpScreenSave);
        hdc.Blit(ptPrev.x, ptPrev.y, dxCrd, pmove->dyCol, &mdcSave, 0, 0, wxCOPY);
    }

    hdc.SelectObject(wxNullBitmap);

    /* Swap temp and screenSave */
    wxBitmap* t = pmove->bmpScreenSave;
    pmove->bmpScreenSave = pmove->bmpT;
    pmove->bmpT = t;

    /* Show on screen */
    if (g_canvas) {
        g_canvas->Refresh(false);
        g_canvas->Update();
    }

    return true;
}

static bool DefComputeCrdPos(COL* pcol, int icrdFirst, bool fAssumeDown)
{
    PT pt;
    if (icrdFirst == 0) {
        pt.x = pcol->rc.xLeft;
        pt.y = pcol->rc.yTop;
    } else {
        --icrdFirst;
        pt = pcol->rgcrd[icrdFirst].pt;
        if (fMegaDiscardHack)
            icrdFirst++;
    }

    for (int icrd = icrdFirst; icrd < pcol->icrdMac; icrd++) {
        CRD* pcrd = &pcol->rgcrd[icrd];
        pcrd->pt = pt;
        if (pcrd->fUp && !fAssumeDown) {
            if (icrd % pcol->pcolcls->dcrdUp == pcol->pcolcls->dcrdUp - 1) {
                pt.x += pcol->pcolcls->dxUp;
                pt.y += pcol->pcolcls->dyUp;
            }
        } else if (icrd % pcol->pcolcls->dcrdDn == pcol->pcolcls->dcrdDn - 1) {
            pt.x += pcol->pcolcls->dxDn;
            pt.y += pcol->pcolcls->dyDn;
        }
    }
    return true;
}

static void InvertCardPt(PT* ppt)
{
    RC rc;
    rc.xRight = (rc.xLeft = ppt->x) + dxCrd;
    rc.yBot = (rc.yTop = ppt->y) + dyCrd;
    InvertRc(&rc);
}

static int DefValidMovePt(COL* pcolDest, COL* pcolSrc, PT* ppt)
{
    RC rc;
    OffsetPt(ppt, &pcolSrc->pmove->delHit, (PT*)&rc);
    rc.xRight = rc.xLeft + dxCrd;
    rc.yBot = rc.yTop + dyCrd;
    if (pcolDest->icrdMac == 0) {
        if (!FRectIsect(&rc, &pcolDest->rc))
            return icrdNil;
    } else if (!FCrdRectIsect(&pcolDest->rgcrd[pcolDest->icrdMac - 1], &rc))
        return icrdNil;

    return SendColMsg(pcolDest, msgcValidMove, (intptr_t)pcolSrc, 0) ? pcolDest->icrdMac : icrdNil;
}

static bool DefSel(COL* pcol, int icrdFirst, int ccrd)
{
    g_move.delHit.dx = g_move.delHit.dy = 0;
    if (icrdFirst == icrdEnd) {
        if (pcol->icrdMac > 0) {
            g_move.icrdSel = pcol->icrdMac - 1;
            g_move.ccrdSel = 1;
            goto Return;
        } else
            return (bool)icrdNil;
    }
    if (ccrd == ccrdToEnd)
        ccrd = pcol->icrdMac - icrdFirst;
    g_move.icrdSel = icrdFirst;
    g_move.ccrdSel = ccrd;
Return:
    pcol->pmove = &g_move;
    return g_move.icrdSel;
}

static bool DefEndSel(COL* pcol, bool fReleaseDC)
{
    pcol->pmove = nullptr;
    return true;
}

static bool DefFlip(COL* pcol, bool fUp)
{
    MOVE* pmove = pcol->pmove;
    int icrdMac = pmove->icrdSel + pmove->ccrdSel;
    for (int icrd = pmove->icrdSel; icrd < icrdMac; icrd++)
        pcol->rgcrd[icrd].fUp = (unsigned short)fUp;
    return true;
}

static bool DefInvert(COL* pcol)
{
    int icrdSel = pcol->pmove->icrdSel;
    int ccrdSel = pcol->pmove->ccrdSel;
    int icrdMid = icrdSel + ccrdSel / 2;
    for (int icrd = icrdSel; icrd < icrdMid; icrd++)
        SwapCards(&pcol->rgcrd[icrd], &pcol->rgcrd[2 * icrdSel + ccrdSel - 1 - icrd]);
    return true;
}

static bool DefDragInvert(COL* pcol)
{
    if (fOutlineDrag) {
        if (!FGetHdc())
            return false;
        InvertCardPt(pcol->icrdMac > 0 ? &pcol->rgcrd[pcol->icrdMac - 1].pt : (PT*)&pcol->rc);
        ReleaseHdc();
    }
    return true;
}

static int DefNumCards(COL* pcol, bool fUpOnly)
{
    if (fUpOnly) {
        int icrd;
        for (icrd = pcol->icrdMac - 1; icrd >= 0 && pcol->rgcrd[icrd].fUp; icrd--)
            ;
        return pcol->icrdMac - 1 - icrd;
    }
    return pcol->icrdMac;
}

static bool DefGetPtInCrd(COL* pcol, int icrd, PT* ppt)
{
    PT* pptT;
    if (icrd == 0)
        pptT = (PT*)&pcol->rc;
    else
        pptT = &pcol->rgcrd[icrd].pt;
    ppt->x = pptT->x + dxCrd / 2;
    ppt->y = pptT->y;
    return true;
}

static bool DefShuffle(COL* pcol)
{
    for (int iSwitch = 0; iSwitch < 5; iSwitch++) {
        for (int icrd = 0; icrd < pcol->icrdMac; icrd++) {
            CRD* pcrdS = &pcol->rgcrd[rand() % pcol->icrdMac];
            CRD crdT = pcol->rgcrd[icrd];
            pcol->rgcrd[icrd] = *pcrdS;
            *pcrdS = crdT;
        }
    }
    return true;
}

static int DefZip(COL* pcol)
{
    if (pgmCur->ptMousePrev.x == ptNil.x)
        return fTrue;

    /* Simplified zip: just jump to destination (no DDA animation) */
    MOVE* pmove = pcol->pmove;
    PT ptDest;
    ptDest.x = pcol->rgcrd[pmove->icrdSel].pt.x - pmove->delHit.dx;
    ptDest.y = pcol->rgcrd[pmove->icrdSel].pt.y - pmove->delHit.dy;

    /* Move outline from current to destination in a few steps */
    int steps = 5;
    PT ptFrom = pgmCur->ptMousePrev;
    for (int i = 1; i <= steps; i++) {
        PT ptStep;
        ptStep.x = ptFrom.x + (ptDest.x - ptFrom.x) * i / steps;
        ptStep.y = ptFrom.y + (ptDest.y - ptFrom.y) * i / steps;
        SendColMsg(pcol, msgcDrawOutline, (intptr_t)&ptStep, (intptr_t)&pgmCur->ptMousePrev);
        pgmCur->ptMousePrev = ptStep;
    }

    return fTrue;
}

/* ---- Master default column procedure ---- */
int DefColProc(COL* pcol, int msgc, intptr_t wp1, intptr_t wp2)
{
    switch (msgc) {
    case msgcInit:
        return fTrue;
    case msgcEnd:
        return DefFreeCol(pcol);
    case msgcClearCol:
        pcol->pmove = nullptr;
        pcol->icrdMac = 0;
        return fTrue;
    case msgcHit:
        return DefHit(pcol, (PT*)wp1, (int)wp2);
    case msgcMouseUp:
        return DefMouseUp(pcol, (PT*)wp1, (int)wp2);
    case msgcDblClk:
        return fFalse;
    case msgcSel:
        return DefSel(pcol, (int)wp1, (int)wp2);
    case msgcEndSel:
        return DefEndSel(pcol, (bool)wp1);
    case msgcNumCards:
        return DefNumCards(pcol, (bool)wp1);
    case msgcFlip:
        return DefFlip(pcol, (bool)wp1);
    case msgcInvert:
        return DefInvert(pcol);
    case msgcRemove:
        return DefRemove(pcol, (COL*)wp1);
    case msgcInsert:
        return DefInsert(pcol, (COL*)wp1, (int)wp2);
    case msgcMove:
        return DefMove(pcol, (COL*)wp1, (int)wp2);
    case msgcCopy:
        return DefCopy(pcol, (COL*)wp1, (bool)wp2);
    case msgcValidMove:
        return fFalse;
    case msgcValidMovePt:
        return DefValidMovePt(pcol, (COL*)wp1, (PT*)wp2);
    case msgcRender:
        return DefRender(pcol, (int)wp1, (int)wp2);
    case msgcPaint:
        return DefPaint(pcol, (RC*)wp1);
    case msgcDrawOutline:
        return DefDrawOutline(pcol, (PT*)wp1, (PT*)wp2);
    case msgcComputeCrdPos:
        return DefComputeCrdPos(pcol, (int)wp1, (bool)wp2);
    case msgcDragInvert:
        return DefDragInvert(pcol);
    case msgcGetPtInCrd:
        return DefGetPtInCrd(pcol, (int)wp1, (PT*)wp2);
    case msgcValidKbdColSel:
        return fTrue;
    case msgcValidKbdCrdSel:
        return fTrue;
    case msgcShuffle:
        return DefShuffle(pcol);
    case msgcAnimate:
        return fFalse;
    case msgcZip:
        return DefZip(pcol);
    }
    return fFalse;
}
