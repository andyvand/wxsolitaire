#include "wxsol.h"
#include "solitaire_icon.h"

/* ---- Implement the wxWidgets application ---- */
wxIMPLEMENT_APP(SolApp);

/* ---- Forward declarations ---- */
static void LoadSettings();
static void SaveSettings();

#if defined(__WXMAC__) || defined(__APPLE__)
extern "C" bool OpenMacHelpBook(const char* helpBundlePath);
#endif

/* ================================================================== */
/*  Options Dialog                                                     */
/* ================================================================== */

class OptionsDialog : public wxDialog {
public:
    OptionsDialog(wxWindow* parent);

    int  GetCcrdDeal() const { return m_ccrdDeal; }
    SMD  GetSmd() const { return m_smd; }
    bool GetStatusBar() const { return m_fStatusBar; }
    bool GetTimedGame() const { return m_fTimedGame; }
    bool GetOutlineDrag() const { return m_fOutlineDrag; }
    bool GetKeepScore() const { return m_fKeepScore; }

private:
    void OnOk(wxCommandEvent& evt);
    void OnScoringChanged(wxCommandEvent& evt);

    wxRadioButton* m_rbDrawOne;
    wxRadioButton* m_rbDrawThree;
    wxRadioButton* m_rbScoreStandard;
    wxRadioButton* m_rbScoreVegas;
    wxRadioButton* m_rbScoreNone;
    wxCheckBox*    m_cbStatusBar;
    wxCheckBox*    m_cbTimedGame;
    wxCheckBox*    m_cbOutlineDrag;
    wxCheckBox*    m_cbKeepScore;

    int  m_ccrdDeal;
    SMD  m_smd;
    bool m_fStatusBar;
    bool m_fTimedGame;
    bool m_fOutlineDrag;
    bool m_fKeepScore;
};

OptionsDialog::OptionsDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, wxT("Options"), wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE)
{
    m_ccrdDeal = ccrdDeal;
    m_smd = smd;
    m_fStatusBar = fStatusBar;
    m_fTimedGame = fTimedGame;
    m_fOutlineDrag = fOutlineDrag;
    m_fKeepScore = fKeepScore;

    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);

    /* Draw group */
    wxStaticBoxSizer* drawBox = new wxStaticBoxSizer(wxVERTICAL, this, wxT("Draw"));
    m_rbDrawOne = new wxRadioButton(drawBox->GetStaticBox(), wxID_ANY, wxT("Draw &One"),
                                     wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    m_rbDrawThree = new wxRadioButton(drawBox->GetStaticBox(), wxID_ANY, wxT("Draw &Three"));
    drawBox->Add(m_rbDrawOne, 0, wxALL, 4);
    drawBox->Add(m_rbDrawThree, 0, wxALL, 4);
    if (m_ccrdDeal == 1)
        m_rbDrawOne->SetValue(true);
    else
        m_rbDrawThree->SetValue(true);

    /* Scoring group */
    wxStaticBoxSizer* scoreBox = new wxStaticBoxSizer(wxVERTICAL, this, wxT("Scoring"));
    m_rbScoreStandard = new wxRadioButton(scoreBox->GetStaticBox(), wxID_ANY, wxT("&Standard"),
                                           wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    m_rbScoreVegas = new wxRadioButton(scoreBox->GetStaticBox(), wxID_ANY, wxT("&Vegas"));
    m_rbScoreNone = new wxRadioButton(scoreBox->GetStaticBox(), wxID_ANY, wxT("&None"));
    scoreBox->Add(m_rbScoreStandard, 0, wxALL, 4);
    scoreBox->Add(m_rbScoreVegas, 0, wxALL, 4);
    scoreBox->Add(m_rbScoreNone, 0, wxALL, 4);
    if (m_smd == smdStandard)
        m_rbScoreStandard->SetValue(true);
    else if (m_smd == smdVegas)
        m_rbScoreVegas->SetValue(true);
    else
        m_rbScoreNone->SetValue(true);

    /* Options checkboxes */
    wxStaticBoxSizer* optBox = new wxStaticBoxSizer(wxVERTICAL, this, wxT("Options"));
    m_cbStatusBar = new wxCheckBox(optBox->GetStaticBox(), wxID_ANY, wxT("Status &Bar"));
    m_cbTimedGame = new wxCheckBox(optBox->GetStaticBox(), wxID_ANY, wxT("&Timed Game"));
    m_cbOutlineDrag = new wxCheckBox(optBox->GetStaticBox(), wxID_ANY, wxT("&Outline Dragging"));
    m_cbKeepScore = new wxCheckBox(optBox->GetStaticBox(), wxID_ANY, wxT("&Keep Score"));
    optBox->Add(m_cbStatusBar, 0, wxALL, 4);
    optBox->Add(m_cbTimedGame, 0, wxALL, 4);
    optBox->Add(m_cbOutlineDrag, 0, wxALL, 4);
    optBox->Add(m_cbKeepScore, 0, wxALL, 4);
    m_cbStatusBar->SetValue(m_fStatusBar);
    m_cbTimedGame->SetValue(m_fTimedGame);
    m_cbOutlineDrag->SetValue(m_fOutlineDrag);
    m_cbKeepScore->SetValue(m_fKeepScore);
    m_cbKeepScore->Enable(m_smd == smdVegas);

    /* Layout */
    wxBoxSizer* rowSizer = new wxBoxSizer(wxHORIZONTAL);
    rowSizer->Add(drawBox, 0, wxALL | wxEXPAND, 5);
    rowSizer->Add(scoreBox, 0, wxALL | wxEXPAND, 5);
    topSizer->Add(rowSizer, 0, wxEXPAND);
    topSizer->Add(optBox, 0, wxALL | wxEXPAND, 5);

    wxSizer* btnSizer = CreateButtonSizer(wxOK | wxCANCEL);
    if (btnSizer)
        topSizer->Add(btnSizer, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 10);

    SetSizerAndFit(topSizer);
    CentreOnParent();

    /* Bind events */
    Bind(wxEVT_BUTTON, &OptionsDialog::OnOk, this, wxID_OK);
    m_rbScoreStandard->Bind(wxEVT_RADIOBUTTON, &OptionsDialog::OnScoringChanged, this);
    m_rbScoreVegas->Bind(wxEVT_RADIOBUTTON, &OptionsDialog::OnScoringChanged, this);
    m_rbScoreNone->Bind(wxEVT_RADIOBUTTON, &OptionsDialog::OnScoringChanged, this);
}

void OptionsDialog::OnScoringChanged(wxCommandEvent& evt)
{
    if (m_rbScoreVegas->GetValue())
        m_smd = smdVegas;
    else if (m_rbScoreNone->GetValue())
        m_smd = smdNone;
    else
        m_smd = smdStandard;
    m_cbKeepScore->Enable(m_smd == smdVegas);
}

void OptionsDialog::OnOk(wxCommandEvent& evt)
{
    m_ccrdDeal = m_rbDrawOne->GetValue() ? 1 : 3;
    if (m_rbScoreStandard->GetValue())
        m_smd = smdStandard;
    else if (m_rbScoreVegas->GetValue())
        m_smd = smdVegas;
    else
        m_smd = smdNone;
    m_fStatusBar = m_cbStatusBar->GetValue();
    m_fTimedGame = m_cbTimedGame->GetValue();
    m_fOutlineDrag = m_cbOutlineDrag->GetValue();
    m_fKeepScore = m_cbKeepScore->GetValue();
    EndModal(wxID_OK);
}

/* ================================================================== */
/*  Card Back Selection Dialog                                         */
/* ================================================================== */

class BackPanel : public wxPanel {
public:
    BackPanel(wxWindow* parent, int backId);
    void OnPaint(wxPaintEvent& evt);
    void OnLeftDown(wxMouseEvent& evt);
    void OnLeftDClick(wxMouseEvent& evt);
    int  GetBackId() const { return m_backId; }

    wxDECLARE_EVENT_TABLE();
private:
    int m_backId;
};

class BacksDialog : public wxDialog {
public:
    BacksDialog(wxWindow* parent);
    int GetSelectedBack() const { return m_selectedBack; }
    void SetSelectedBack(int backId);

private:
    void OnOk(wxCommandEvent& evt);
    int m_selectedBack;
    BackPanel* m_panels[cIDFACEDOWN];
};

wxBEGIN_EVENT_TABLE(BackPanel, wxPanel)
    EVT_PAINT(BackPanel::OnPaint)
    EVT_LEFT_DOWN(BackPanel::OnLeftDown)
    EVT_LEFT_DCLICK(BackPanel::OnLeftDClick)
wxEND_EVENT_TABLE()

BackPanel::BackPanel(wxWindow* parent, int backId)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(dxCrd + 8, dyCrd + 8),
              wxBORDER_SIMPLE)
    , m_backId(backId)
{
    SetMinSize(wxSize(dxCrd + 8, dyCrd + 8));
}

void BackPanel::OnPaint(wxPaintEvent& evt)
{
    wxPaintDC dc(this);
    cdtDrawExt(&dc, 4, 4, dxCrd, dyCrd, m_backId, FACEDOWN, 0L);

    /* Draw highlight if this is the selected back */
    bool isSelected = false;
    BacksDialog* dlg = dynamic_cast<BacksDialog*>(GetParent());
    if (dlg)
        isSelected = (dlg->GetSelectedBack() == m_backId);
    if (isSelected) {
        wxPen pen(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT), 2);
        dc.SetPen(pen);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRectangle(1, 1, dxCrd + 6, dyCrd + 6);
    }
}

void BackPanel::OnLeftDown(wxMouseEvent& evt)
{
    SetFocus();
    /* Update explicit selection in the dialog */
    BacksDialog* dlg = dynamic_cast<BacksDialog*>(GetParent());
    if (dlg)
        dlg->SetSelectedBack(m_backId);
}

void BackPanel::OnLeftDClick(wxMouseEvent& evt)
{
    SetFocus();
    /* Double-click selects and closes */
    BacksDialog* dlg = dynamic_cast<BacksDialog*>(GetParent());
    if (dlg) {
        dlg->SetSelectedBack(m_backId);
        dlg->EndModal(wxID_OK);
    }
}

BacksDialog::BacksDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, wxT("Select Card Back"), wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE)
{
    m_selectedBack = modeFaceDown;

    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);

    /* Grid of card backs: 2 rows x 6 columns */
    wxGridSizer* grid = new wxGridSizer(2, 6, 4, 4);
    for (int i = 0; i < cIDFACEDOWN; i++) {
        int backId = IDFACEDOWNFIRST + i;
        m_panels[i] = new BackPanel(this, backId);
        grid->Add(m_panels[i], 0, wxALL, 2);
    }
    topSizer->Add(grid, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 10);

    wxSizer* btnSizer = CreateButtonSizer(wxOK | wxCANCEL);
    if (btnSizer)
        topSizer->Add(btnSizer, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 10);

    SetSizerAndFit(topSizer);
    CentreOnParent();

    /* Set initial focus to current back */
    for (int i = 0; i < cIDFACEDOWN; i++) {
        if (m_panels[i]->GetBackId() == modeFaceDown) {
            m_panels[i]->SetFocus();
            break;
        }
    }

    Bind(wxEVT_BUTTON, &BacksDialog::OnOk, this, wxID_OK);
}

void BacksDialog::SetSelectedBack(int backId)
{
    m_selectedBack = backId;
    /* Refresh all panels so the highlight follows the selection */
    for (int i = 0; i < cIDFACEDOWN; i++)
        m_panels[i]->Refresh();
}

void BacksDialog::OnOk(wxCommandEvent& evt)
{
    EndModal(wxID_OK);
}

/* ================================================================== */
/*  Settings persistence using wxConfig                                */
/* ================================================================== */

static void LoadSettings()
{
    wxConfig config(wxT("wxSolitaire"));

    long opts = 0;
    config.Read(wxT("Options"), &opts, -1);

    if (opts == -1) {
        /* Defaults */
        fStatusBar = true;
        fTimedGame = true;
        fOutlineDrag = false;
        ccrdDeal = 3;
        fKeepScore = false;
        smd = smdStandard;
    } else {
        fStatusBar   = (opts & 0x01) != 0;
        fTimedGame   = (opts & 0x02) != 0;
        fOutlineDrag = (opts & 0x04) != 0;
        ccrdDeal     = (opts & 0x08) ? 3 : 1;
        fKeepScore   = (opts & 0x10) != 0;
        int smdVal   = (opts >> 5) & 0x03;
        switch (smdVal) {
            default: smd = smdStandard; break;
            case 1:  smd = smdVegas;    break;
            case 2:  smd = smdNone;     break;
        }
    }

    long back = 0;
    config.Read(wxT("Back"), &back, 0);
    if (back == 0)
        modeFaceDown = IDFACEDOWNFIRST + (rand() % cIDFACEDOWN);
    else
        modeFaceDown = PegRange((int)back + IDFACEDOWNFIRST - 1,
                                IDFACEDOWNFIRST, IDFACEDOWN12);
}

static void SaveSettings()
{
    wxConfig config(wxT("wxSolitaire"));

    long opts = 0;
    if (fStatusBar)   opts |= 0x01;
    if (fTimedGame)   opts |= 0x02;
    if (fOutlineDrag) opts |= 0x04;
    if (ccrdDeal == 3) opts |= 0x08;
    if (fKeepScore)   opts |= 0x10;
    int smdVal = 0;
    switch (smd) {
        default:         smdVal = 0; break;
        case smdVegas:   smdVal = 1; break;
        case smdNone:    smdVal = 2; break;
    }
    opts |= (smdVal << 5);
    config.Write(wxT("Options"), opts);
    config.Write(wxT("Back"), (long)(modeFaceDown - IDFACEDOWNFIRST + 1));
}

/* ================================================================== */
/*  SolCanvas implementation                                           */
/* ================================================================== */

wxBEGIN_EVENT_TABLE(SolCanvas, wxPanel)
    EVT_PAINT(SolCanvas::OnPaint)
    EVT_LEFT_DOWN(SolCanvas::OnLeftDown)
    EVT_LEFT_UP(SolCanvas::OnLeftUp)
    EVT_LEFT_DCLICK(SolCanvas::OnLeftDClick)
    EVT_RIGHT_DOWN(SolCanvas::OnRightDown)
    EVT_MOTION(SolCanvas::OnMouseMove)
    EVT_KEY_DOWN(SolCanvas::OnKeyDown)
    EVT_SIZE(SolCanvas::OnSize)
wxEND_EVENT_TABLE()

SolCanvas::SolCanvas(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
              wxWANTS_CHARS | wxFULL_REPAINT_ON_RESIZE)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(wxColour(rgbTable & 0xFF,
                                  (rgbTable >> 8) & 0xFF,
                                  (rgbTable >> 16) & 0xFF));
}

void SolCanvas::OnPaint(wxPaintEvent& evt)
{
    wxPaintDC dc(this);
    wxSize sz = GetClientSize();

    if (g_backingStore && g_backingStore->IsOk() &&
        g_backingStore->GetWidth() >= sz.x &&
        g_backingStore->GetHeight() >= sz.y)
    {
        wxMemoryDC memDC(*g_backingStore);
        dc.Blit(0, 0, sz.x, sz.y, &memDC, 0, 0, wxCOPY);
        memDC.SelectObject(wxNullBitmap);
    } else {
        /* Backing store not ready - draw plain background */
        wxBrush br(wxColour(rgbTable & 0xFF,
                             (rgbTable >> 8) & 0xFF,
                             (rgbTable >> 16) & 0xFF));
        dc.SetBrush(br);
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(0, 0, sz.x, sz.y);
    }
}

void SolCanvas::OnLeftDown(wxMouseEvent& evt)
{
    if (!pgmCur) return;
    SetFocus();
    CaptureMouse();
    if (pgmCur->fButtonDown)
        return;
    PT pt;
    pt.x = evt.GetX();
    pt.y = evt.GetY();
    SendGmMsg(pgmCur, msggMouseDown, (intptr_t)&pt, 0);
}

void SolCanvas::OnLeftUp(wxMouseEvent& evt)
{
    if (!pgmCur) return;
    if (HasCapture())
        ReleaseMouse();
    if (!pgmCur->fButtonDown)
        return;
    PT pt;
    pt.x = evt.GetX();
    pt.y = evt.GetY();
    SendGmMsg(pgmCur, msggMouseUp, (intptr_t)&pt, 0);
}

void SolCanvas::OnLeftDClick(wxMouseEvent& evt)
{
    if (!pgmCur) return;
    if (pgmCur->fButtonDown)
        return;
    PT pt;
    pt.x = evt.GetX();
    pt.y = evt.GetY();
    SendGmMsg(pgmCur, msggMouseDblClk, (intptr_t)&pt, 0);
}

void SolCanvas::OnRightDown(wxMouseEvent& evt)
{
    if (!pgmCur) return;
    /* Ignore right-click if left button is captured */
    if (HasCapture())
        return;
    PT pt;
    pt.x = evt.GetX();
    pt.y = evt.GetY();
    SendGmMsg(pgmCur, msggMouseRightClk, (intptr_t)&pt, 0);
}

void SolCanvas::OnMouseMove(wxMouseEvent& evt)
{
    if (!pgmCur) return;
    if (!pgmCur->fButtonDown)
        return;
    PT pt;
    pt.x = evt.GetX();
    pt.y = evt.GetY();
    SendGmMsg(pgmCur, msggMouseMove, (intptr_t)&pt, 0);
}

void SolCanvas::OnKeyDown(wxKeyEvent& evt)
{
    if (!pgmCur) return;
    SendGmMsg(pgmCur, msggKeyHit, (intptr_t)evt.GetKeyCode(), 0);
}

void SolCanvas::OnSize(wxSizeEvent& evt)
{
    wxSize sz = evt.GetSize();
    dxScreen = sz.GetWidth();
    dyScreen = sz.GetHeight();

    fIconic = g_frame && g_frame->IsIconized();

    /* Recompute card margin */
    int nNewMargin = (dxScreen - 7 * dxCrd) / 8;
    int nMinMargin = MIN_MARGIN;
    if (nNewMargin < nMinMargin)
        nNewMargin = nMinMargin;
    xCardMargin = nNewMargin;

    if (pgmCur && pgmCur->fDealt) {
        PositionCols();

        /* Recompute all card positions from the updated column rects */
        for (int icol = 0; icol < pgmCur->icolMac; icol++) {
            if (icol == icolDiscard) {
                /* Discard column: mirror DiscardMove's special positioning.
                   First stack ALL cards as face-down, then fan out only
                   the top 3 visible cards using fMegaDiscardHack. */
                COL* pcolDisc = pgmCur->rgpcol[icolDiscard];
                SendColMsg(pcolDisc, msgcComputeCrdPos, 0, fTrue);
                if (pcolDisc->icrdMac > 0) {
                    fMegaDiscardHack = true;
                    SendColMsg(pcolDisc, msgcComputeCrdPos,
                               WMax(0, pcolDisc->icrdMac - 3), fFalse);
                    fMegaDiscardHack = false;
                }
            } else {
                SendColMsg(pgmCur->rgpcol[icol], msgcComputeCrdPos, 0, fFalse);
            }
        }

        /* Invalidate and redraw backing store at new size */
        delete g_backingStore;
        g_backingStore = nullptr;

        if (FGetHdc()) {
            DrawBackground(0, 0, dxScreen, dyScreen);
            SendGmMsg(pgmCur, msggPaint, 0, 0);
            ReleaseHdc(); /* triggers Refresh + Update */
        }
    }
    evt.Skip();
}

/* ================================================================== */
/*  SolFrame implementation                                            */
/* ================================================================== */

wxBEGIN_EVENT_TABLE(SolFrame, wxFrame)
    EVT_MENU(SolFrame::ID_NEWGAME, SolFrame::OnNewGame)
    EVT_MENU(SolFrame::ID_UNDO,    SolFrame::OnUndo)
    EVT_MENU(SolFrame::ID_DECK,    SolFrame::OnDeck)
    EVT_MENU(SolFrame::ID_OPTIONS, SolFrame::OnOptions)
    EVT_MENU(wxID_EXIT,            SolFrame::OnExit)
    EVT_MENU(wxID_ABOUT,           SolFrame::OnAbout)
    EVT_MENU(wxID_HELP,            SolFrame::OnHelp)
    EVT_MENU(SolFrame::ID_FORCEWIN,SolFrame::OnNewGame) /* reuse for debug */
    EVT_TIMER(SolFrame::ID_TIMER,  SolFrame::OnTimer)
    EVT_MENU_OPEN(SolFrame::OnMenuOpen)
    EVT_CLOSE(SolFrame::OnClose)
    EVT_ACTIVATE(SolFrame::OnActivate)
wxEND_EVENT_TABLE()

SolFrame::SolFrame()
    : wxFrame(nullptr, wxID_ANY, wxT("Solitaire"),
              wxDefaultPosition, wxDefaultSize,
              wxDEFAULT_FRAME_STYLE | wxCLIP_CHILDREN)
    , m_timer(this, ID_TIMER)
    , m_statusBar(nullptr)
{
    /* --- Icon (embedded PNG loaded from compiled-in data) --- */
    {
        wxMemoryInputStream iconStream(solitaire_icon, sizeof(solitaire_icon));
        wxImage iconImage(iconStream, wxBITMAP_TYPE_PNG);
        if (iconImage.IsOk()) {
            wxBitmap iconBmp(iconImage);
            wxIcon appIcon;
            appIcon.CopyFromBitmap(iconBmp);
            SetIcon(appIcon);
        }
    }

    /* --- Menu bar --- */
    wxMenuBar* menuBar = new wxMenuBar;

    wxMenu* gameMenu = new wxMenu;
    gameMenu->Append(ID_NEWGAME, wxT("&Deal\tF2"), wxT("Start a new game"));
    gameMenu->AppendSeparator();
    gameMenu->Append(ID_UNDO, wxT("&Undo\tCtrl+Z"), wxT("Undo last move"));
    gameMenu->Append(ID_DECK, wxT("Card &Backs..."), wxT("Select card back design"));
    gameMenu->Append(ID_OPTIONS, wxT("&Options..."), wxT("Game options"));
    gameMenu->AppendSeparator();
    gameMenu->Append(wxID_EXIT, wxT("E&xit"), wxT("Exit the game"));
    menuBar->Append(gameMenu, wxT("&Game"));

    wxMenu* helpMenu = new wxMenu;
    helpMenu->Append(wxID_HELP, wxT("Solitaire &Help\tF1"), wxT("View Solitaire help"));
    helpMenu->AppendSeparator();
    helpMenu->Append(wxID_ABOUT, wxT("&About Solitaire..."), wxT("About this game"));
    menuBar->Append(helpMenu, wxT("&Help"));

    SetMenuBar(menuBar);

    /* --- Compute initial window size --- */
    xCardMargin = MIN_MARGIN;
    int initWidth = dxCrd * 7 + 8 * xCardMargin;
    int initHeight = dyCrd * 4;

    /* --- Canvas (main drawing area) --- */
    m_canvas = new SolCanvas(this);
    g_canvas = m_canvas;

    /* --- Status bar --- */
    if (fStatusBar) {
        m_statusBar = CreateStatusBar();
    }

    /* --- Set window size and minimum --- */
    int minWidth = dxCrd * 7 + 8 * MIN_MARGIN;
    int minHeight = dyCrd * 3;
    SetMinClientSize(wxSize(minWidth, minHeight));
    SetClientSize(initWidth, initHeight);

    /* --- Start timer (250ms like original) --- */
    m_timer.Start(250);
}

void SolFrame::OnNewGame(wxCommandEvent& evt)
{
    if (!pgmCur) return;
    if (FSelOfGm(pgmCur)) return; /* Don't start new game during selection */
    if (evt.GetId() == ID_FORCEWIN) {
        SendGmMsg(pgmCur, msggForceWin, 0, 0);
    } else {
        NewGame(true, false);
    }
}

void SolFrame::OnUndo(wxCommandEvent& evt)
{
    if (!pgmCur) return;
    SendGmMsg(pgmCur, msggUndo, 0, 0);
}

void SolFrame::OnDeck(wxCommandEvent& evt)
{
    if (!pgmCur) return;
    if (FSelOfGm(pgmCur)) return;

    BacksDialog dlg(this);
    if (dlg.ShowModal() == wxID_OK) {
        int newBack = dlg.GetSelectedBack();
        if (newBack != modeFaceDown) {
            modeFaceDown = newBack;
            SaveSettings();
            /* Redraw with new card backs */
            if (FGetHdc()) {
                wxSize sz = m_canvas->GetClientSize();
                DrawBackground(0, 0, sz.x, sz.y);
                if (pgmCur)
                    SendGmMsg(pgmCur, msggPaint, 0, 0);
                ReleaseHdc();
            }
        }
    }
}

void SolFrame::OnOptions(wxCommandEvent& evt)
{
    OptionsDialog dlg(this);
    if (dlg.ShowModal() == wxID_OK) {
        bool fNewGame = false;

        /* Status bar change */
        bool newStatusBar = dlg.GetStatusBar();
        if (newStatusBar != fStatusBar) {
            fStatusBar = newStatusBar;
            if (fStatusBar) {
                if (!m_statusBar)
                    m_statusBar = CreateStatusBar();
            } else {
                if (m_statusBar) {
                    SetStatusBar(nullptr);
                    delete m_statusBar;
                    m_statusBar = nullptr;
                }
            }
        }

        /* Draw count change */
        int newCcrdDeal = dlg.GetCcrdDeal();
        if (newCcrdDeal != ccrdDeal) {
            ccrdDeal = newCcrdDeal;
            FInitGm();
            PositionCols();
            fNewGame = true;
        }

        /* Timed game change */
        bool newTimedGame = dlg.GetTimedGame();
        if (newTimedGame != fTimedGame) {
            fTimedGame = newTimedGame;
            fNewGame = true;
        }

        /* Scoring mode change */
        SMD newSmd = dlg.GetSmd();
        if (newSmd != smd) {
            smd = newSmd;
            fNewGame = true;
        }

        /* Outline drag change */
        bool newOutlineDrag = dlg.GetOutlineDrag();
        if (newOutlineDrag != fOutlineDrag) {
            FSetDrag(newOutlineDrag);
        }

        /* Keep score */
        fKeepScore = dlg.GetKeepScore();

        SaveSettings();

        if (fNewGame)
            NewGame(true, true);
    }
}

void SolFrame::OnExit(wxCommandEvent& evt)
{
    Close(true);
}

void SolFrame::OnAbout(wxCommandEvent& evt)
{
    wxMessageBox(wxT("Solitaire\n\n")
                 wxT("A port of the classic Windows Solitaire (Klondike) game\n")
                 wxT("using wxWidgets for cross-platform compatibility."),
                 wxT("About Solitaire"),
                 wxOK | wxICON_INFORMATION, this);
}

void SolFrame::OnHelp(wxCommandEvent& evt)
{
    const wxString resDir = wxStandardPaths::Get().GetResourcesDir();
    const wxString exeDir = wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath();

#if defined(__WXMAC__) || defined(__APPLE__)
    /* macOS: register with AppleHelp and open via Help Viewer. */
    wxFileName bundle(resDir, wxEmptyString);
    bundle.AppendDir(wxT("wxsolitaire.help"));
    if (!wxDirExists(bundle.GetPath())) {
        bundle.AssignDir(exeDir);
        bundle.AppendDir(wxT("wxsolitaire.help"));
    }
    if (wxDirExists(bundle.GetPath())) {
        if (OpenMacHelpBook(bundle.GetPath().utf8_str())) return;
        wxLaunchDefaultApplication(bundle.GetPath());
        return;
    }
#elif defined(__WXMSW__) || defined(_WIN32)
    /* Windows: hand the .chm to the registered viewer (hh.exe). */
    wxFileName chm(exeDir, wxT("wxsolitaire.chm"));
    if (!chm.FileExists()) chm.AssignDir(resDir), chm.SetFullName(wxT("wxsolitaire.chm"));
    if (chm.FileExists()) {
        wxLaunchDefaultApplication(chm.GetFullPath());
        return;
    }
#else
    /* Linux / other Unix: open the packaged HTML index in the default browser. */
    wxFileName html(resDir, wxT("index.html"));
    html.AppendDir(wxT("wxsolitaire.help"));
    html.AppendDir(wxT("Contents"));
    html.AppendDir(wxT("Resources"));
    html.AppendDir(wxT("English.lproj"));
    if (!html.FileExists()) {
        html.AssignDir(exeDir);
        html.AppendDir(wxT("wxsolitaire.help"));
        html.AppendDir(wxT("Contents"));
        html.AppendDir(wxT("Resources"));
        html.AppendDir(wxT("English.lproj"));
        html.SetFullName(wxT("index.html"));
    }
    if (html.FileExists()) {
        wxLaunchDefaultBrowser(wxT("file://") + html.GetFullPath());
        return;
    }
#endif

    wxMessageBox(wxT("Help content could not be located."),
                 wxT("Solitaire Help"), wxOK | wxICON_WARNING, this);
}

void SolFrame::OnTimer(wxTimerEvent& evt)
{
    if (pgmCur)
        SendGmMsg(pgmCur, msggTimer, 0, 0);
}

void SolFrame::OnMenuOpen(wxMenuEvent& evt)
{
    if (!pgmCur) return;
    wxMenuBar* mb = GetMenuBar();
    if (!mb) return;

    bool sel = FSelOfGm(pgmCur);
    mb->Enable(ID_UNDO, pgmCur->udr.fAvail && !sel);
    mb->Enable(ID_NEWGAME, !sel);
    mb->Enable(ID_DECK, !sel);
}

void SolFrame::OnClose(wxCloseEvent& evt)
{
    m_timer.Stop();

    if (pgmCur) {
        SendGmMsg(pgmCur, msggEnd, 0, 0);
        FSetDrag(true); /* Free drag bitmaps */
    }

    cdtTerm();
    SaveSettings();

    g_canvas = nullptr;
    g_frame = nullptr;

    Destroy();
}

void SolFrame::OnActivate(wxActivateEvent& evt)
{
    if (evt.GetActive() && !IsIconized()) {
        if (m_canvas && pgmCur) {
            /* Repaint backing store and refresh */
            if (FGetHdc()) {
                wxSize sz = m_canvas->GetClientSize();
                DrawBackground(0, 0, sz.x, sz.y);
                SendGmMsg(pgmCur, msggPaint, 0, 0);
                ReleaseHdc();
            }
        }
    }
    evt.Skip();
}

void SolFrame::NewGame(bool fNewSeed, bool fZeroScore)
{
    if (!pgmCur) return;

    if (fNewSeed) {
        igmCur = ((unsigned)time(nullptr)) & 0x7fff;
        srand(igmCur);
    }

    SendGmMsg(pgmCur, msggDeal, fZeroScore, 0);
}

void SolFrame::UpdateStatusText()
{
    if (pgmCur)
        SendGmMsg(pgmCur, msggDrawStatus, 0, 0);
}

/* ================================================================== */
/*  SolApp::OnInit                                                     */
/* ================================================================== */

bool SolApp::OnInit()
{
    /* Seed random number generator */
    srand((unsigned)time(nullptr));

    /* Enable PNG (and other) image handlers for embedded icon loading */
    wxInitAllImageHandlers();

    /* Initialize card library - get card dimensions */
    if (!cdtInit(&dxCrd, &dyCrd)) {
        wxMessageBox(wxT("Failed to initialize card bitmaps from embedded resources."),
                     wxT("Solitaire Error"), wxOK | wxICON_ERROR);
        return false;
    }

    /* Get screen resolution */
    dxScreen = wxSystemSettings::GetMetric(wxSYS_SCREEN_X);
    dyScreen = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y);

    /* Half cards for small screens */
    if ((fHalfCards = (dyScreen < 300))) {
        dyCrd /= 2;
    }

    /* Text metrics will be set after frame creation */
    dyChar = 16;
    dxChar = 8;

    /* Detect B&W (unlikely these days) */
    fBW = (wxSystemSettings::GetMetric(wxSYS_SCREEN_X) <= 0); /* effectively always false */

    /* Set table colour */
    rgbTable = fBW ? 0x00FFFFFFUL : 0x00008000UL; /* white or green */

    /* Load saved settings */
    LoadSettings();

    /* Create main frame */
    m_frame = new SolFrame();
    g_frame = m_frame;

    /* Initialize game engine */
    FInitGm();
    FSetDrag(fOutlineDrag);

    /* Position columns now that we have a game and a canvas */
    PositionCols();

    /* Show the window */
    m_frame->Show(true);
    SetTopWindow(m_frame);

    /* Deal initial game */
    m_frame->NewGame(true, false);

    return true;
}
