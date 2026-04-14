#include "wxsol.h"

/* ---- Global state ---- */
GM*   pgmCur = nullptr;
int   dxCrd = 0, dyCrd = 0;
int   dxScreen = 0, dyScreen = 0;
unsigned long rgbTable = 0;
int   modeFaceDown = -1;
bool  fIconic = false;
bool  fStatusBar = true;
bool  fTimedGame = true;
bool  fKeepScore = false;
SMD   smd = smdStandard;
int   ccrdDeal = 3;
bool  fOutlineDrag = false;
bool  fHalfCards = false;
int   xCardMargin = 0;
int   igmCur = 0;
bool  fBW = false;
bool  fKlondWinner = false;
int   dyChar = 16;
int   dxChar = 8;
PT    ptNil = { 0x7fff, 0x7fff };
SolFrame*  g_frame = nullptr;
SolCanvas* g_canvas = nullptr;

/* ---- Undo ---- */
bool FInitUndo(UDR* pudr)
{
    pudr->fAvail = false;
    for (int icol = 0; icol < 2; icol++) {
        pudr->rgpcol[icol] = PcolCreate(nullptr, 0, 0, 0, 0, icrdUndoMax);
        if (!pudr->rgpcol[icol]) {
            if (icol != 0) FreeP(pudr->rgpcol[0]);
            return false;
        }
    }
    return true;
}

void FreeUndo(UDR* pudr)
{
    for (int icol = 0; icol < 2; icol++)
        FreeP(pudr->rgpcol[icol]);
}

/* ---- Game manager ---- */
void FreeGm(GM* pgm)
{
    if (pgm) {
        for (int icol = pgm->icolMac - 1; icol >= 0; icol--)
            if (pgm->rgpcol[icol])
                SendColMsg(pgm->rgpcol[icol], msgcEnd, 0, 0);
        if (pgm == pgmCur)
            pgmCur = nullptr;
        FreeUndo(&pgm->udr);
        FreeP(pgm);
    }
}

bool FSetDrag(bool fOutline)
{
    fOutlineDrag = fOutline;

    if (fOutline && g_move.fHdc) {
        delete g_move.bmpCol; g_move.bmpCol = nullptr;
        delete g_move.bmpScreenSave; g_move.bmpScreenSave = nullptr;
        delete g_move.bmpT; g_move.bmpT = nullptr;
        g_move.fHdc = false;
    }

    if (!fOutline && !g_move.fHdc && pgmCur) {
        g_move.bmpCol = new wxBitmap(dxCrd, pgmCur->dyDragMax > 0 ? pgmCur->dyDragMax : dyCrd);
        g_move.bmpScreenSave = new wxBitmap(dxCrd, pgmCur->dyDragMax > 0 ? pgmCur->dyDragMax : dyCrd);
        g_move.bmpT = new wxBitmap(dxCrd, pgmCur->dyDragMax > 0 ? pgmCur->dyDragMax : dyCrd);
        g_move.fHdc = true;
    }
    return true;
}

bool FInitGm()
{
    return FInitKlondGm();
}

/* ---- Default Game Procedures ---- */

static bool DefGmInit(GM* pgm, bool fResetScore)
{
    pgm->fDealt = false;
    if (fResetScore)
        pgm->sco = 0;
    pgm->iqsecScore = 0;
    pgm->irep = 0;
    pgm->icolHilight = pgm->icolSel = icolNil;
    pgm->icolKbd = 0;
    pgm->icrdKbd = 0;
    pgm->fInput = false;
    pgm->fWon = false;
    pgm->ccrdDeal = ccrdDeal;
    return true;
}

static bool DefGmMouseDown(GM* pgm, PT* ppt, int icolFirst)
{
    if (FSelOfGm(pgm)) return false;
    if (!pgm->fDealt) return false;
    pgm->fInput = true;
    pgm->fButtonDown = true;

    for (int icol = icolFirst; icol < pgm->icolMac; icol++) {
        if (SendColMsg(pgm->rgpcol[icol], msgcHit, (intptr_t)ppt, 0) != icrdNil) {
            pgm->icolSel = icol;
            pgm->ptMousePrev = ptNil;
            if (!fOutlineDrag)
                pgm->ptMousePrev = *ppt;
            return true;
        }
    }
    return false;
}

static bool DefGmMouseUp(GM* pgm, PT* pptBogus, bool fNoMove)
{
    pgm->fButtonDown = false;
    if (FSelOfGm(pgm)) {
        COL* pcolSel = pgm->rgpcol[pgm->icolSel];
        bool fResult = false;
        if (FHilightOfGm(pgm)) {
            COL* pcolHilight = pgm->rgpcol[pgm->icolHilight];
            SendGmMsg(pgm, msggSaveUndo, pgm->icolHilight, pgm->icolSel);
            SendColMsg(pcolHilight, msgcDragInvert, 0, 0);
            if (fNoMove) {
                SendColMsg(pcolSel, msgcMouseUp, (intptr_t)&pgm->ptMousePrev, fTrue);
                fResult = true;
                goto Return;
            }
            SendColMsg(pcolSel, msgcMouseUp, (intptr_t)&pgm->ptMousePrev, fFalse);
            fResult = SendColMsg(pcolHilight, msgcMove, (intptr_t)pcolSel, icrdToEnd) &&
                      SendGmMsg(pgm, msggScore, (intptr_t)pcolHilight, (intptr_t)pcolSel);
            pgm->icolHilight = icolNil;
            if (SendGmMsg(pgm, msggIsWinner, 0, 0))
                SendGmMsg(pgm, msggWinner, 0, 0);
        } else {
            SendColMsg(pcolSel, msgcMouseUp, (intptr_t)&pgm->ptMousePrev, fTrue);
        }
    Return:
        SendColMsg(pcolSel, msgcEndSel, fFalse, 0);
    }
    pgm->icolSel = icolNil;
    return false;
}

static bool DefGmMouseDblClk(GM* pgm, PT* ppt)
{
    for (int icol = 0; icol < pgm->icolMac; icol++)
        if (SendColMsg(pgm->rgpcol[icol], msgcDblClk, (intptr_t)ppt, icol))
            return true;
    return false;
}

static bool DefGmMouseRightClk(GM* pgm, PT* ppt)
{
    bool fResult = false;
    int iContinue;
    do {
        iContinue = 0;
        for (int icol = 0; icol < pgm->icolMac; icol++) {
            if (icol >= icolFoundFirst && icol < icolFoundFirst + ccolFound)
                continue;
            COL* pcol = pgm->rgpcol[icol];
            CRD* pcrd;
            if (pcol->icrdMac > 0 && (pcrd = &pcol->rgcrd[pcol->icrdMac - 1])->fUp) {
                if (pcol->pmove == nullptr)
                    SendColMsg(pcol, msgcSel, icrdEnd, ccrdToEnd);
                for (int icolDest = icolFoundFirst; icolDest < icolFoundFirst + ccolFound; icolDest++) {
                    COL* pcolDest = pgmCur->rgpcol[icolDest];
                    if (SendColMsg(pcolDest, msgcValidMove, (intptr_t)pcol, 0)) {
                        SendGmMsg(pgmCur, msggSaveUndo, icolDest, icol);
                        fResult = SendColMsg(pcolDest, msgcMove, (intptr_t)pcol, icrdToEnd) &&
                                  (fOutlineDrag || SendColMsg(pcol, msgcRender, pcol->icrdMac - 1, icrdToEnd)) &&
                                  SendGmMsg(pgmCur, msggScore, (intptr_t)pcolDest, (intptr_t)pcol);
                        iContinue++;
                        if (SendGmMsg(pgmCur, msggIsWinner, 0, 0))
                            SendGmMsg(pgmCur, msggWinner, 0, 0);
                        break;
                    }
                }
            }
            SendColMsg(pcol, msgcEndSel, fFalse, 0);
        }
    } while (iContinue > 0);
    return fResult;
}

static bool DefGmMouseMove(GM* pgm, PT* ppt)
{
    if (FSelOfGm(pgm)) {
        COL* pcol = pgm->rgpcol[pgm->icolSel];
        SendColMsg(pcol, msgcDrawOutline, (intptr_t)ppt, (intptr_t)&pgm->ptMousePrev);
        pgm->ptMousePrev = *ppt;
        for (int icol = 0; icol < pgm->icolMac; icol++) {
            if (SendColMsg(pgm->rgpcol[icol], msgcValidMovePt, (intptr_t)pgm->rgpcol[pgm->icolSel], (intptr_t)ppt) != icrdNil) {
                if (icol != pgm->icolHilight) {
                    if (FHilightOfGm(pgm))
                        SendColMsg(pgm->rgpcol[pgm->icolHilight], msgcDragInvert, 0, 0);
                    pgm->icolHilight = icol;
                    return SendColMsg(pgm->rgpcol[icol], msgcDragInvert, 0, 0);
                }
                return true;
            }
        }
        if (FHilightOfGm(pgm)) {
            SendColMsg(pgm->rgpcol[pgm->icolHilight], msgcDragInvert, 0, 0);
            pgm->icolHilight = icolNil;
            return true;
        }
    }
    return false;
}

static bool DefGmPaint(GM* pgm, RC* ppaintRC)
{
    if (!pgm->fDealt) return true;
    for (int icol = 0; icol < pgm->icolMac; icol++)
        SendColMsg(pgm->rgpcol[icol], msgcPaint, (intptr_t)ppaintRC, 0);
    return true;
}

static bool DefGmUndo(GM* pgm)
{
    UDR* pudr = &pgm->udr;
    if (!pudr->fAvail) return false;
    pgm->sco = pudr->sco;
    pgm->irep = pudr->irep;
    SendGmMsg(pgm, msggChangeScore, 0, 0);
    SendColMsg(pgm->rgpcol[pudr->icol1], msgcCopy, (intptr_t)pudr->rgpcol[0], fTrue);
    SendColMsg(pgm->rgpcol[pudr->icol2], msgcCopy, (intptr_t)pudr->rgpcol[1], fTrue);
    SendColMsg(pgm->rgpcol[pudr->icol1], msgcEndSel, 0, 0);
    SendColMsg(pgm->rgpcol[pudr->icol2], msgcEndSel, 0, 0);
    SendGmMsg(pgm, msggKillUndo, 0, 0);
    return true;
}

static bool DefGmSaveUndo(GM* pgm, int icol1, int icol2)
{
    bltb(pgm->rgpcol[icol1], pgm->udr.rgpcol[0],
         sizeof(COL) + (pgm->rgpcol[icol1]->icrdMac - 1) * sizeof(CRD));
    bltb(pgm->rgpcol[icol2], pgm->udr.rgpcol[1],
         sizeof(COL) + (pgm->rgpcol[icol2]->icrdMac - 1) * sizeof(CRD));
    pgm->udr.icol1 = icol1;
    pgm->udr.icol2 = icol2;
    pgm->udr.fAvail = true;
    pgm->udr.sco = pgm->sco;
    pgm->udr.irep = pgm->irep;

    if (pgm->udr.fEndDeck) {
        pgm->udr.fEndDeck = false;
        pgm->udr.irep--;
    }
    return true;
}

void NewKbdColAbs(GM* pgm, int icol)
{
    if (!SendColMsg(pgm->rgpcol[icol], msgcValidKbdColSel, FSelOfGm(pgm), 0))
        return;
    pgm->icolKbd = icol;
    pgm->icrdKbd = SendColMsg(pgm->rgpcol[pgm->icolKbd], msgcNumCards, fFalse, 0) - 1;
    if (pgm->icrdKbd < 0)
        pgm->icrdKbd = 0;
}

static void NewKbdCol(GM* pgm, int dcol, bool fNextGroup)
{
    int icolNew = pgm->icolKbd;
    if (icolNew == icolNil) icolNew = 0;
    if (dcol != 0) {
        do {
            icolNew += dcol;
            if (icolNew < 0) icolNew = pgm->icolMac - 1;
            else if (icolNew >= pgm->icolMac) icolNew = 0;
            if (icolNew == pgm->icolKbd) break;
        } while (!SendColMsg(pgm->rgpcol[icolNew], msgcValidKbdColSel, FSelOfGm(pgm), 0) ||
                 (fNextGroup && pgm->rgpcol[icolNew]->pcolcls->tcls == pgm->rgpcol[pgm->icolKbd]->pcolcls->tcls));
    }
    NewKbdColAbs(pgm, icolNew);
}

static void NewKbdCrd(GM* pgm, int dcrd)
{
    int icrdUpMac = SendColMsg(pgm->rgpcol[pgm->icolKbd], msgcNumCards, fTrue, 0);
    int icrdMac = SendColMsg(pgm->rgpcol[pgm->icolKbd], msgcNumCards, fFalse, 0);
    int icrdKbdNew;
    if (icrdMac == 0)
        icrdKbdNew = 0;
    else {
        if (icrdUpMac == 0)
            icrdKbdNew = icrdMac - 1;
        else
            icrdKbdNew = PegRange(pgm->icrdKbd + dcrd, icrdMac - icrdUpMac, icrdMac - 1);
    }
    if (SendColMsg(pgm->rgpcol[pgm->icolKbd], msgcValidKbdCrdSel, icrdKbdNew, 0))
        pgm->icrdKbd = icrdKbdNew;
}

static bool DefGmKeyHit(GM* pgm, int vk)
{
    PT pt;

    switch (vk) {
    case WXK_SPACE:
    case WXK_RETURN:
        if (!FSelOfGm(pgm)) {
            NewKbdCrd(pgm, 0);
            SendColMsg(pgm->rgpcol[pgm->icolKbd], msgcGetPtInCrd, pgm->icrdKbd, (intptr_t)&pt);
            if (!SendGmMsg(pgm, msggMouseDown, (intptr_t)&pt, 0))
                return false;
            NewKbdCol(pgm, 0, false);
            return true;
        } else {
            SendGmMsg(pgm, msggMouseUp, 0, fFalse);
            NewKbdCol(pgm, 0, false);
            return true;
        }
    case WXK_ESCAPE:
        SendGmMsg(pgm, msggMouseUp, 0, fTrue);
        return true;
    case 'A':
        if (wxGetKeyState(WXK_CONTROL))
            SendGmMsg(pgm, msggMouseRightClk, 0, fTrue);
        return true;
    case WXK_LEFT:
        NewKbdCol(pgm, -1, wxGetKeyState(WXK_SHIFT));
        return true;
    case WXK_RIGHT:
        NewKbdCol(pgm, 1, wxGetKeyState(WXK_SHIFT));
        return true;
    case WXK_UP:
        NewKbdCrd(pgm, -1);
        return true;
    case WXK_DOWN:
        NewKbdCrd(pgm, 1);
        return true;
    case WXK_HOME:
        NewKbdColAbs(pgm, 0);
        return true;
    case WXK_END:
        NewKbdColAbs(pgm, pgm->icolMac - 1);
        return true;
    case WXK_TAB:
        NewKbdCol(pgm, wxGetKeyState(WXK_SHIFT) ? -1 : 1, true);
        return true;
    }
    return false;
}

static bool DefGmChangeScore(GM* pgm, int cs, int sco)
{
    if (smd == smdNone) return true;
    switch (cs) {
    default: return true;
    case csAbs: pgm->sco = sco; break;
    case csDel: pgm->sco += sco; break;
    case csDelPos: pgm->sco = WMax(pgm->sco + sco, 0); break;
    }
    StatUpdate();
    return true;
}

static bool DefGmWinner(GM* pgm)
{
    pgm->fWon = false;
    int ans = wxMessageBox(wxT("Deal Again?"), wxT("Solitaire"),
                           wxYES_NO | wxICON_QUESTION, g_frame);
    if (ans == wxYES && g_frame)
        g_frame->NewGame(true, false);
    return true;
}

int DefGmProc(GM* pgm, int msgg, intptr_t wp1, intptr_t wp2)
{
    switch (msgg) {
    case msggInit:    return DefGmInit(pgm, (bool)wp1);
    case msggEnd:     FreeGm(pgm); break;
    case msggKeyHit:  return DefGmKeyHit(pgm, (int)wp1);
    case msggMouseRightClk: return DefGmMouseRightClk(pgm, (PT*)wp1);
    case msggMouseDown:     return DefGmMouseDown(pgm, (PT*)wp1, (int)wp2);
    case msggMouseUp:       return DefGmMouseUp(pgm, (PT*)wp1, (bool)wp2);
    case msggMouseMove:     return DefGmMouseMove(pgm, (PT*)wp1);
    case msggMouseDblClk:   return DefGmMouseDblClk(pgm, (PT*)wp1);
    case msggPaint:         return DefGmPaint(pgm, (RC*)wp1);
    case msggDeal:          break;
    case msggUndo:          return DefGmUndo(pgm);
    case msggSaveUndo:      return DefGmSaveUndo(pgm, (int)wp1, (int)wp2);
    case msggKillUndo:      pgm->udr.fAvail = false; break;
    case msggIsWinner:      return fFalse;
    case msggWinner:        return DefGmWinner(pgm);
    case msggForceWin:      break;
    case msggTimer:         return fFalse;
    case msggScore:         return fTrue;
    case msggChangeScore:   return DefGmChangeScore(pgm, (int)wp1, (int)wp2);
    }
    return fFalse;
}

/* ==================================================================
 *   KLONDIKE IMPLEMENTATION
 * ================================================================== */

/* ---- Tableau ---- */
static bool FTabValidMove(COL* pcolDest, COL* pcolSrc)
{
    int icrdSel = pcolSrc->pmove->icrdSel;
    CD cd = pcolSrc->rgcrd[icrdSel].cd;
    RA raSrc = RaFromCd(cd);
    SU suSrc = SuFromCd(cd);
    if (raSrc == raKing) return pcolDest->icrdMac == 0;
    if (pcolDest->icrdMac == 0) return false;
    if (!pcolDest->rgcrd[pcolDest->icrdMac - 1].fUp) return false;
    CD cdDest = pcolDest->rgcrd[pcolDest->icrdMac - 1].cd;
    RA raDest = RaFromCd(cdDest);
    SU suDest = SuFromCd(cdDest);
    return ((suSrc ^ suDest) < 0x03) && suSrc != suDest && raSrc + 1 == raDest;
}

static int TabHit(COL* pcol, PT* ppt, int icrdMin)
{
    CRD* pcrd;
    if (pcol->icrdMac > 0 && !(pcrd = &pcol->rgcrd[pcol->icrdMac - 1])->fUp && FPtInCrd(pcrd, *ppt)) {
        SendGmMsg(pgmCur, msggKillUndo, 0, 0);
        SendColMsg(pcol, msgcSel, icrdEnd, 1);
        SendColMsg(pcol, msgcFlip, fTrue, 0);
        SendColMsg(pcol, msgcComputeCrdPos, pcol->icrdMac - 1, fFalse);
        SendColMsg(pcol, msgcRender, pcol->icrdMac - 1, icrdToEnd);
        SendGmMsg(pgmCur, msggChangeScore, csKlondTabFlip, 0);
        SendColMsg(pcol, msgcEndSel, fFalse, 0);
        return icrdNil;
    }
    return DefColProc(pcol, msgcHit, (intptr_t)ppt, icrdMin);
}

bool TabDiscardDblClk(COL* pcol, PT* ppt, int icol)
{
    CRD* pcrd;
    bool fResult = false;
    if (pcol->icrdMac > 0 && (pcrd = &pcol->rgcrd[pcol->icrdMac - 1])->fUp && FPtInCrd(pcrd, *ppt)) {
        if (pcol->pmove == nullptr)
            SendColMsg(pcol, msgcSel, icrdEnd, ccrdToEnd);
        for (int icolDest = icolFoundFirst; icolDest < icolFoundFirst + ccolFound; icolDest++) {
            COL* pcolDest = pgmCur->rgpcol[icolDest];
            if (SendColMsg(pcolDest, msgcValidMove, (intptr_t)pcol, 0)) {
                SendGmMsg(pgmCur, msggSaveUndo, icolDest, icol);
                fResult = SendColMsg(pcolDest, msgcMove, (intptr_t)pcol, icrdToEnd) &&
                          (fOutlineDrag || SendColMsg(pcol, msgcRender, pcol->icrdMac - 1, icrdToEnd)) &&
                          SendGmMsg(pgmCur, msggScore, (intptr_t)pcolDest, (intptr_t)pcol);
                if (SendGmMsg(pgmCur, msggIsWinner, 0, 0))
                    SendGmMsg(pgmCur, msggWinner, 0, 0);
                return fResult;
            }
        }
        SendColMsg(pcol, msgcEndSel, fFalse, 0);
    }
    return fResult;
}

intptr_t TabColProc(COL* pcol, int msgc, intptr_t wp1, intptr_t wp2)
{
    switch (msgc) {
    case msgcHit:       return TabHit(pcol, (PT*)wp1, (int)wp2);
    case msgcDblClk:    return TabDiscardDblClk(pcol, (PT*)wp1, (int)wp2);
    case msgcValidMove: return FTabValidMove(pcol, (COL*)wp1);
    }
    return DefColProc(pcol, msgc, wp1, wp2);
}

/* ---- Foundation ---- */
static bool FFoundRender(COL* pcol, int icrdFirst, int icrdLast)
{
    if (pcol->icrdMac == 0 || icrdLast == 0) {
        if (!FGetHdc()) return false;
        DrawCardExt((PT*)(&pcol->rc.xLeft), 0, GHOST);
        DrawBackExcl(pcol, (PT*)&pcol->rc);
        ReleaseHdc();
        return true;
    }
    return DefColProc(pcol, msgcRender, icrdFirst, icrdLast);
}

static bool FFoundValidMove(COL* pcolDest, COL* pcolSrc)
{
    int icrdSel = pcolSrc->pmove->icrdSel;
    if (pcolSrc->pmove->ccrdSel != 1) return false;
    RA raSrc = RaFromCd(pcolSrc->rgcrd[icrdSel].cd);
    SU suSrc = SuFromCd(pcolSrc->rgcrd[icrdSel].cd);
    if (pcolDest->icrdMac == 0) return raSrc == raAce;
    return raSrc == RaFromCd(pcolDest->rgcrd[pcolDest->icrdMac - 1].cd) + 1 &&
           suSrc == SuFromCd(pcolDest->rgcrd[pcolDest->icrdMac - 1].cd);
}

intptr_t FoundColProc(COL* pcol, int msgc, intptr_t wp1, intptr_t wp2)
{
    switch (msgc) {
    case msgcValidMove: return FFoundValidMove(pcol, (COL*)wp1);
    case msgcRender:    return FFoundRender(pcol, (int)wp1, (int)wp2);
    }
    return DefColProc(pcol, msgc, wp1, wp2);
}

/* ---- Deck ---- */
static bool DeckInit(COL* pcol)
{
    for (int icrd = 0; icrd < icrdDeckMax; icrd++) {
        CRD* pcrd = &pcol->rgcrd[icrd];
        pcrd->cd = (unsigned short)icrd;
        memcpy(&pcrd->pt, &pcol->rc, sizeof(PT));
        pcrd->fUp = 0;
    }
    pcol->icrdMac = icrdDeckMax;
    SendColMsg(pcol, msgcShuffle, 0, 0);
    SendColMsg(pcol, msgcComputeCrdPos, 0, fFalse);
    return true;
}

static int DeckHit(COL* pcol, PT* ppt, int icrdMin)
{
    if (pcol->icrdMac == 0) {
        RC rc;
        CrdRcFromPt((PT*)&pcol->rc, &rc);
        if (PtInRC(ppt, &rc)) return icrdEmpty;
        return icrdNil;
    }
    if (!FPtInCrd(&pcol->rgcrd[pcol->icrdMac - 1], *ppt))
        return icrdNil;

    int ccrd = pgmCur->ccrdDeal;
    g_move.icrdSel = WMax(pcol->icrdMac - ccrd, 0);
    g_move.ccrdSel = pcol->icrdMac - g_move.icrdSel;
    pcol->pmove = &g_move;
    return g_move.icrdSel;
}

static bool FDeckRender(COL* pcol, int icrdFirst, int icrdLast)
{
    if (!pgmCur->fDealt && pcol->icrdMac % 10 != 9)
        return true;
    if (!FGetHdc()) return false;
    if (pcol->icrdMac == 0) {
        int mode = (smd == smdVegas && pgmCur->irep == ccrdDeal - 1) ? DECKX : DECKO;
        DrawCardExt((PT*)&pcol->rc, 0, mode);
        DrawBackExcl(pcol, (PT*)&pcol->rc);
    } else {
        DefColProc(pcol, msgcRender, icrdFirst, icrdLast);
    }
    ReleaseHdc();
    return true;
}

static void DrawAnimate(int cd, PT* ppt, int iani)
{
    if (!FGetHdc()) return;
    cdtAnimate(g_dc, cd, ppt->x, ppt->y, iani);
    ReleaseHdc();
}

static bool DeckAnimate(COL* pcol, int iqsec)
{
    if (pcol->icrdMac > 0 && !fHalfCards) {
        PT pt = pcol->rgcrd[pcol->icrdMac - 1].pt;
        int iani;
        switch (modeFaceDown) {
        case IDFACEDOWN3:
            DrawAnimate(IDFACEDOWN3, &pt, iqsec % 4);
            break;
        case IDFACEDOWN10:
            DrawAnimate(IDFACEDOWN10, &pt, iqsec % 2);
            break;
        case IDFACEDOWN11:
            if ((iani = (iqsec + 4) % (50 * 4)) < 4)
                DrawAnimate(IDFACEDOWN11, &pt, iani);
            else if (iani % 6 == 0)
                DrawAnimate(IDFACEDOWN11, &pt, 3);
            break;
        case IDFACEDOWN12:
            if ((iani = (iqsec + 4) % (15 * 4)) < 4)
                DrawAnimate(IDFACEDOWN12, &pt, iani);
            else if (iani % 6 == 0)
                DrawAnimate(IDFACEDOWN12, &pt, 3);
            break;
        }
    }
    return true;
}

intptr_t DeckColProc(COL* pcol, int msgc, intptr_t wp1, intptr_t wp2)
{
    switch (msgc) {
    case msgcInit:            return DeckInit(pcol);
    case msgcValidMove:
    case msgcDrawOutline:     return fFalse;
    case msgcValidMovePt:     return icrdNil;
    case msgcHit:             return DeckHit(pcol, (PT*)wp1, (int)wp2);
    case msgcRender:          return FDeckRender(pcol, (int)wp1, (int)wp2);
    case msgcValidKbdColSel:  return !wp1;
    case msgcValidKbdCrdSel:  return pcol->icrdMac == 0 || wp1 == (intptr_t)(pcol->icrdMac - 1);
    case msgcAnimate:         return DeckAnimate(pcol, (int)wp1);
    }
    return DefColProc(pcol, msgc, wp1, wp2);
}

/* ---- Discard ---- */
static bool DiscardRemove(COL* pcol, COL* pcolDest, intptr_t wp2)
{
    return DefColProc(pcol, msgcRemove, (intptr_t)pcolDest, wp2);
}

static bool DiscardMove(COL* pcolDest, COL* pcolSrc, int icrd)
{
    SendColMsg(pcolDest, msgcComputeCrdPos, WMax(0, pcolDest->icrdMac - 3), fTrue);
    fMegaDiscardHack = true;
    bool fResult = DefColProc(pcolDest, msgcMove, (intptr_t)pcolSrc, icrd);
    fMegaDiscardHack = false;
    return fResult;
}

static int DiscardHit(COL* pcol, PT* ppt, int icrdMin)
{
    return DefColProc(pcol, msgcHit, (intptr_t)ppt, WMax(0, pcol->icrdMac - 1));
}

static bool DiscardRender(COL* pcol, int icrdFirst, int icrdLast)
{
    if (DefColProc(pcol, msgcRender, icrdFirst, icrdLast)) {
        if (FGetHdc()) {
            COLCLS* pcolcls = pcol->pcolcls;
            for (int icrd = pcol->icrdMac - 1; icrd >= 0 && icrd >= pcol->icrdMac - 2; icrd--) {
                PT pt = pcol->rgcrd[icrd].pt;
                DrawBackground(pt.x + dxCrd - pcolcls->dxUp, pt.y - pcolcls->dyUp * 3,
                               pt.x + dxCrd, pt.y);
            }
            ReleaseHdc();
        }
        return true;
    }
    return false;
}

intptr_t DiscardColProc(COL* pcol, int msgc, intptr_t wp1, intptr_t wp2)
{
    switch (msgc) {
    case msgcDblClk:          return TabDiscardDblClk(pcol, (PT*)wp1, (int)wp2);
    case msgcHit:             return DiscardHit(pcol, (PT*)wp1, (int)wp2);
    case msgcMove:            return DiscardMove(pcol, (COL*)wp1, (int)wp2);
    case msgcRemove:          return DiscardRemove(pcol, (COL*)wp1, wp2);
    case msgcValidMovePt:     return icrdNil;
    case msgcValidKbdColSel:  return !wp1;
    case msgcRender:          return DiscardRender(pcol, (int)wp1, (int)wp2);
    case msgcValidKbdCrdSel:  return pcol->icrdMac == 0 || wp1 == (intptr_t)(pcol->icrdMac - 1);
    }
    return DefColProc(pcol, msgc, wp1, wp2);
}

/* ---- Klondike game proc ---- */

bool FInitKlondGm()
{
    COLCLS* pcolcls = nullptr;
    FreeGm(pgmCur);

    GM* pgm = pgmCur = (GM*)PAlloc(sizeof(GM) + (13 - 1) * sizeof(COL*));
    if (!pgm) return false;

    pgm->lpfnGmProc = KlondGmProc;
    SendGmMsg(pgm, msggInit, fTrue, 0);
    pgm->icolMax = 13;
    pgm->dqsecScore = 10 * 4;

    if (!FInitUndo(&pgm->udr)) {
        FreeGm(pgmCur);
        return false;
    }

    for (int icol = 0; icol < pgm->icolMax; icol++) {
        int icrdMax;
        switch (icol) {
        case icolDeck:
            pcolcls = PcolclsCreate(tclsDeck, (ColProc_t)DeckColProc, 0, 0, 2, 1, 1, 10);
            icrdMax = icrdDeckMax;
            break;
        case icolDiscard: {
            DX dxCrdOffUp = dxCrd / 5;
            pcolcls = PcolclsCreate(tclsDiscard, (ColProc_t)DiscardColProc, dxCrdOffUp, 1, 2, 1, 1, 10);
            icrdMax = icrdDiscardMax;
            break;
        }
        case icolFoundFirst:
            pcolcls = PcolclsCreate(tclsFound, (ColProc_t)FoundColProc, 2, 1, 0, 0, 4, 1);
            icrdMax = icrdFoundMax;
            break;
        case icolTabFirst: {
            DY dyCrdOffUp = dyCrd * 4 / 25 - fHalfCards;
            DY dyCrdOffDn = dyCrd / 25;
            pgm->dyDragMax = dyCrd + 12 * dyCrdOffUp;
            pcolcls = PcolclsCreate(tclsTab, (ColProc_t)TabColProc, 0, dyCrdOffUp, 0, dyCrdOffDn, 1, 1);
            icrdMax = icrdTabMax;
            break;
        }
        default:
            icrdMax = 0; /* use previous pcolcls */
            break;
        }

        if (!pcolcls) {
            FreeGm(pgmCur);
            return false;
        }

        pgm->rgpcol[icol] = PcolCreate(pcolcls, 0, 0, 0, 0, icrdMax);
        if (!pgm->rgpcol[icol]) {
            FreeP(pcolcls);
            FreeGm(pgmCur);
            return false;
        }
        pgm->icolMac++;
    }
    return true;
}

bool PositionCols()
{
    GM* pgm = pgmCur;
    if (!pgm) return false;

    /* Convert card X positions to offsets from column */
    for (int icol = 0; icol < 13; ++icol) {
        COL* pcol = pgm->rgpcol[icol];
        for (int i = 0; i < pcol->icrdMax; ++i)
            pcol->rgcrd[i].pt.x -= pcol->rc.xLeft;
    }

    DX dxMarg = xCardMargin;
    DY dyMarg = dyCrd * 5 / 100;
    DX dx = 0;
    X xLeft = 0, xRight = 0;
    Y yTop = 0, yBot = 0;

    for (int icol = 0; icol < 13; icol++) {
        switch (icol) {
        case icolDeck:
            xLeft = dxMarg;
            yTop = dyMarg + MENU_HEIGHT;
            xRight = xLeft + dxCrd + icrdDeckMax / 10 * 2;
            yBot = yTop + dyCrd + icrdDeckMax / 10;
            dx = 0;
            break;
        case icolDiscard:
            xLeft += dxMarg + dxCrd;
            xRight = xLeft + 7 * dxCrd / 5 + icrdDiscardMax / 10 * 2;
            break;
        case icolFoundFirst:
            xLeft = 4 * dxMarg + 3 * dxCrd;
            xRight = xLeft + dxCrd + icrdFoundMax / 4 * 2;
            dx = dxMarg + dxCrd;
            break;
        case icolTabFirst: {
            DY dyCrdOffUp = dyCrd * 4 / 25 - fHalfCards;
            DY dyCrdOffDn = dyCrd / 25;
            xLeft = dxMarg;
            xRight = xLeft + dxCrd;
            yTop = yBot + 1 + MENU_HEIGHT;
            yBot = yTop + 12 * dyCrdOffUp + dyCrd + 6 * dyCrdOffDn;
            break;
        }
        }

        COL* pcol = pgm->rgpcol[icol];
        pcol->rc.xLeft = xLeft;
        pcol->rc.yTop = yTop;
        pcol->rc.xRight = xRight;
        pcol->rc.yBot = yBot;
        xLeft += dx;
        xRight += dx;
    }

    /* Convert offsets back to absolute positions */
    for (int icol = 0; icol < 13; ++icol) {
        COL* pcol = pgm->rgpcol[icol];
        for (int i = 0; i < pcol->icrdMax; ++i)
            pcol->rgcrd[i].pt.x += pcol->rc.xLeft;
    }
    return true;
}

static bool KlondDeal(GM* pgm, bool fZeroScore)
{
    if (!FGetHdc()) return false;

    EraseScreen();
    for (int icol = 0; icol < pgm->icolMac; icol++)
        SendColMsg(pgm->rgpcol[icol], msgcClearCol, 0, 0);

    COL* pcolDeck = pgm->rgpcol[icolDeck];
    SendColMsg(pcolDeck, msgcInit, 0, 0);
    SendGmMsg(pgm, msggKillUndo, 0, 0);
    SendGmMsg(pgm, msggInit, !(smd == smdVegas && fKeepScore) || fZeroScore, 0);

    StatStringSz(wxT(""));
    pgm->fDealt = true;
    SendGmMsg(pgm, msggChangeScore, csKlondDeal, 0);
    SendColMsg(pcolDeck, msgcRender, 0, icrdToEnd);
    pgm->fDealt = false;

    for (int icol = icolFoundFirst; icol < icolFoundFirst + ccolFound; icol++)
        SendColMsg(pgm->rgpcol[icol], msgcRender, 0, icrdToEnd);

    for (int irw = 0; irw < ccolTab; irw++) {
        for (int icol = irw; icol < ccolTab; icol++) {
            int icrdSel = SendColMsg(pcolDeck, msgcSel, icrdEnd, 0);
            if (icol == irw)
                SendColMsg(pcolDeck, msgcFlip, fTrue, 0);
            SendColMsg(pgm->rgpcol[icol + icolTabFirst], msgcMove, (intptr_t)pcolDeck, icrdToEnd);
            SendColMsg(pcolDeck, msgcRender, icrdSel - 1, icrdToEnd);
        }
    }

    NewKbdColAbs(pgm, 0);
    pgm->fDealt = true;
    ReleaseHdc();
    return true;
}

static bool KlondMouseDown(GM* pgm, PT* ppt)
{
    if (FSelOfGm(pgm) || !pgm->fDealt) return false;

    int icrd = SendColMsg(pgm->rgpcol[icolDeck], msgcHit, (intptr_t)ppt, 0);
    if (icrd != icrdNil) {
        pgm->fInput = true;
        COL* pcolDeck = pgm->rgpcol[icolDeck];
        COL* pcolDiscard = pgm->rgpcol[icolDiscard];
        if (icrd == icrdEmpty) {
            if (SendColMsg(pcolDiscard, msgcNumCards, 0, 0) == 0)
                return false;
            if (smd == smdVegas && pgm->irep == ccrdDeal - 1)
                return false;
            pgm->irep++;
            pgm->udr.fEndDeck = true;
            return SendGmMsg(pgm, msggSaveUndo, icolDiscard, icolDeck) &&
                   SendColMsg(pcolDiscard, msgcSel, 0, ccrdToEnd) != icrdNil &&
                   SendColMsg(pcolDiscard, msgcFlip, fFalse, 0) &&
                   SendColMsg(pcolDiscard, msgcInvert, 0, 0) &&
                   SendGmMsg(pgm, msggScore, (intptr_t)pcolDeck, (intptr_t)pcolDiscard) &&
                   SendColMsg(pcolDeck, msgcMove, (intptr_t)pcolDiscard, icrdToEnd) &&
                   SendColMsg(pcolDiscard, msgcRender, 0, icrdToEnd);
        } else {
            int icrdSel = pcolDeck->pmove->icrdSel - 1;
            return SendGmMsg(pgm, msggSaveUndo, icolDiscard, icolDeck) &&
                   SendColMsg(pcolDeck, msgcFlip, fTrue, 0) &&
                   SendColMsg(pcolDeck, msgcInvert, 0, 0) &&
                   SendColMsg(pcolDiscard, msgcMove, (intptr_t)pcolDeck, icrdToEnd) &&
                   SendColMsg(pcolDeck, msgcRender, icrdSel, icrdToEnd);
        }
    }
    return DefGmProc(pgm, msggMouseDown, (intptr_t)ppt, icolDiscard);
}

static bool KlondIsWinner(GM* pgm)
{
    for (int icol = icolFoundFirst; icol < icolFoundFirst + ccolFound; icol++)
        if (pgm->rgpcol[icol]->icrdMac != icrdFoundMax)
            return false;
    return true;
}

static bool KlondWinner(GM* pgm)
{
    fKlondWinner = true;

    int dsco = (int)SendGmMsg(pgmCur, msggChangeScore, csKlondWin, 0);
    pgm->udr.fAvail = false;
    pgm->fDealt = false;
    pgm->fWon = true;

    wxString szBonus;
    if (smd == smdStandard) {
        szBonus = wxString::Format(wxT("Bonus: %d  "), dsco);
    }
    szBonus += wxT("Congratulations, you win!");
    StatStringSz(szBonus);

    if (!FGetHdc()) goto ByeNoRel;

    {
        wxSize sz = g_canvas ? g_canvas->GetClientSize() : wxSize(800, 600);
        int dxp = sz.x;
        int dyp = sz.y - dyCrd;

        for (int icrd = icrdFoundMax - 1; icrd >= 0; icrd--) {
            for (int icol = icolFoundFirst; icol < icolFoundFirst + ccolFound; icol++) {
                PT ptV;
                ptV.x = rand() % 110 - 65;
                if (abs(ptV.x) < 15) ptV.x = -20;
                ptV.y = rand() % 110 - 75;
                CRD* pcrd = &pgm->rgpcol[icol]->rgcrd[icrd];
                PT pt = pcrd->pt;

                while (pt.x > -dxCrd && pt.x < dxp) {
                    DrawCardPt(pcrd, &pt);
                    pt.x += ptV.x / 10;
                    pt.y += ptV.y / 10;
                    ptV.y += 3;
                    if (pt.y > dyp && ptV.y > 0)
                        ptV.y = -(ptV.y * 8) / 10;
                    /* Check for user input to abort */
                    if (wxGetApp().Pending()) {
                        wxGetApp().Dispatch();
                        goto ByeBye;
                    }
                }
            }
        }
    }

ByeBye:
    ReleaseHdc();
ByeNoRel:
    StatStringSz(wxT(""));
    EraseScreen();
    fKlondWinner = false;
    return DefGmProc(pgm, msggWinner, 0, 0);
}

static bool KlondForceWin(GM* pgm)
{
    for (int icol = 0; icol < pgm->icolMac; icol++)
        SendColMsg(pgm->rgpcol[icol], msgcClearCol, 0, 0);
    for (int su = suFirst, icol = icolFoundFirst; icol < icolFoundFirst + ccolFound; icol++, su++) {
        for (int ra = raFirst; ra < raMax; ra++) {
            COL* pcol = pgm->rgpcol[icol];
            CRD* pcrd = &pcol->rgcrd[ra];
            pcrd->cd = Cd(ra, su);
            pcrd->pt.x = pcol->rc.xLeft;
            pcrd->pt.y = pcol->rc.yTop;
            pcrd->fUp = 1;
        }
        pgm->rgpcol[icol]->icrdMac = icrdFoundMax;
    }
    return (bool)SendGmMsg(pgm, msggWinner, 0, 0);
}

static int mpcsdscoStd[]   = { -2, -20, 10, 5, 5, -15,   0,  0 };
static int mpcsdscoVegas[] = {  0,   0,  5, 0, 0,  -5, -52,  0 };

static bool KlondScore(GM* pgm, COL* pcolDest, COL* pcolSrc)
{
    if (smd == smdNone) return true;
    int cs = csNil;
    int tclsSrc = pcolSrc->pcolcls->tcls;
    int tclsDest = pcolDest->pcolcls->tcls;

    switch (tclsDest) {
    default: return true;
    case tclsDeck:
        if (tclsSrc == tclsDiscard) cs = csKlondDeckFlip;
        break;
    case tclsFound:
        if (tclsSrc == tclsDiscard || tclsSrc == tclsTab) cs = csKlondFound;
        else return true;
        break;
    case tclsTab:
        if (tclsSrc == tclsDiscard) cs = csKlondTab;
        else if (tclsSrc == tclsFound) cs = csKlondFoundTab;
        else return true;
        break;
    }
    SendGmMsg(pgm, msggChangeScore, cs, 0);
    return true;
}

static bool KlondChangeScore(GM* pgm, int cs, int sco)
{
    if (cs < 0) return DefGmProc(pgm, msggChangeScore, cs, sco);
    int dsco;
    int csNew;
    int* pmpcsdsco;

    switch (smd) {
    default: return true;
    case smdVegas:
        pmpcsdsco = mpcsdscoVegas;
        break;
    case smdStandard:
        pmpcsdsco = mpcsdscoStd;
        if (cs == csKlondWin && fTimedGame) {
            if (pgm->iqsecScore >= 120)
                dsco = (20000 / (pgm->iqsecScore >> 2)) * (350 / 10);
            else
                dsco = 0;
            goto DoScore;
        }
        if (cs == csKlondDeckFlip) {
            if (ccrdDeal == 1 && pgm->irep >= 1) {
                dsco = -100;
                goto DoScore;
            } else if (ccrdDeal == 3 && pgm->irep > 3)
                break;
            else
                return true;
        }
        break;
    }

    dsco = pmpcsdsco[cs];
DoScore:
    csNew = smd == smdVegas ? csDel : csDelPos;
    {
        int ret = DefGmProc(pgm, msggChangeScore, csNew, dsco);
        if (cs == csKlondWin) return dsco;
        return ret;
    }
}

static bool KlondTimer(GM* pgm, int wp1, int wp2)
{
    if (fTimedGame && pgm->fDealt && pgm->fInput && !fIconic) {
        pgm->iqsecScore = WMin(pgm->iqsecScore + 1, 0x7ffe);
        if (pgm->icolSel == icolNil)
            SendColMsg(pgm->rgpcol[icolDeck], msgcAnimate, pgm->iqsecScore, 0);
        if (pgm->dqsecScore != 0 && pgm->iqsecScore % pgm->dqsecScore == 0) {
            SendGmMsg(pgm, msggChangeScore, csKlondTime, 0);
        } else {
            if (~(pgm->iqsecScore & 0x03))
                StatUpdate();
            return true;
        }
    }
    return false;
}

static wxString KlondStatusText(GM* pgm)
{
    wxString status;
    if (fTimedGame) {
        status += wxString::Format(wxT("Time: %d  "), pgm->iqsecScore >> 2);
    }
    if (smd != smdNone) {
        status += wxT("Score: ");
        if (smd == smdVegas) {
            if (pgm->sco < 0)
                status += wxString::Format(wxT("-$%d"), -pgm->sco);
            else
                status += wxString::Format(wxT("$%d"), pgm->sco);
        } else {
            status += wxString::Format(wxT("%d"), pgm->sco);
        }
    }
    return status;
}

static bool KlondDrawStatus(GM* pgm, RC* prc)
{
    /* In wxWidgets, we update the status bar directly */
    if (g_frame && g_frame->GetStatusBar()) {
        g_frame->SetStatusText(KlondStatusText(pgm));
    }
    return true;
}

intptr_t KlondGmProc(GM* pgm, int msgg, intptr_t wp1, intptr_t wp2)
{
    switch (msgg) {
    case msggMouseDblClk:
        if (DefGmProc(pgm, msggMouseDblClk, wp1, wp2))
            return fTrue;
        /* fall thru for deck double clicks */
    case msggMouseDown:   return KlondMouseDown(pgm, (PT*)wp1);
    case msggDeal:        return KlondDeal(pgm, (bool)wp1);
    case msggIsWinner:    return KlondIsWinner(pgm);
    case msggWinner:      return KlondWinner(pgm);
    case msggForceWin:    return KlondForceWin(pgm);
    case msggScore:       return KlondScore(pgm, (COL*)wp1, (COL*)wp2);
    case msggChangeScore: return KlondChangeScore(pgm, (int)wp1, (int)wp2);
    case msggTimer:       return KlondTimer(pgm, (int)wp1, (int)wp2);
    case msggDrawStatus:  return KlondDrawStatus(pgm, (RC*)wp1);
    }
    return DefGmProc(pgm, msgg, wp1, wp2);
}
