#ifndef WXSOL_H
#define WXSOL_H

#include <wx/wx.h>
#include <wx/dcmemory.h>
#include <wx/dcbuffer.h>
#include <wx/config.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/mstream.h>
#include <wx/image.h>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cmath>

/* ---- Basic types (matching original) ---- */
typedef int X;
typedef int Y;
typedef int DX;
typedef int DY;
typedef int CD;
typedef int RA;
typedef int SU;
typedef int SCO;
typedef int SMD;

#define fTrue  1
#define fFalse 0

/* Point structure */
struct PT {
    X x;
    Y y;
};

/* Delta structure */
struct DEL {
    DX dx;
    DY dy;
};

/* Rectangle structure */
struct RC {
    X xLeft;
    Y yTop;
    X xRight;
    Y yBot;
};

/* ---- Card constants ---- */
#define suClub    0
#define suDiamond 1
#define suHeart   2
#define suSpade   3
#define suMax     4
#define suFirst   suClub

#define raAce    0
#define raDeuce  1
#define raTres   2
#define raFour   3
#define raFive   4
#define raSix    5
#define raSeven  6
#define raEight  7
#define raNine   8
#define raTen    9
#define raJack   10
#define raQueen  11
#define raKing   12
#define raMax    13
#define raNil    15
#define raFirst  raAce

#define cdNil 0x3c

#define SuFromCd(cd) ((cd) & 0x03)
#define RaFromCd(cd) ((cd) >> 2)
#define Cd(ra, su)   (((ra) << 2) | (su))

/* Card drawing modes */
#define FACEUP         0
#define FACEDOWN       1
#define HILITE         2
#define GHOST          3
#define REMOVE         4
#define INVISIBLEGHOST 5
#define DECKX          6
#define DECKO          7

/* Face down card IDs */
#define IDFACEDOWN1     54
#define IDFACEDOWN2     55
#define IDFACEDOWN3     56
#define IDFACEDOWN4     57
#define IDFACEDOWN5     58
#define IDFACEDOWN6     59
#define IDFACEDOWN7     60
#define IDFACEDOWN8     61
#define IDFACEDOWN9     62
#define IDFACEDOWN10    63
#define IDFACEDOWN11    64
#define IDFACEDOWN12    65
#define IDFACEDOWNFIRST IDFACEDOWN1
#define IDFACEDOWNLAST  IDFACEDOWN12
#define cIDFACEDOWN     (IDFACEDOWNLAST - IDFACEDOWNFIRST + 1)

/* Animation IDs */
#define IDASLIME1  678
#define IDASLIME2  679
#define IDAKASTL1  680
#define IDAFLIPE1  681
#define IDAFLIPE2  682
#define IDABROBOT1 683
#define IDABROBOT2 684

/* ---- Card structure ---- */
struct CRD {
    unsigned short cd  : 15;
    unsigned short fUp : 1;
    PT pt;
};

/* ---- Forward declarations ---- */
struct COL;
struct GM;

/* ---- Column class types ---- */
#define tclsDeck    1
#define tclsDiscard 2
#define tclsFound   3
#define tclsTab     4

/* Column procedure type */
typedef int (*ColProc_t)(COL*, int, intptr_t, intptr_t);

/* Column class structure */
struct COLCLS {
    int tcls;
    ColProc_t lpfnColProc;
    int ccolDep;
    DX dxUp;
    DY dyUp;
    DX dxDn;
    DY dyDn;
    int dcrdUp;
    int dcrdDn;
};

/* Move structure (during drag) */
struct MOVE {
    int  icrdSel;
    int  ccrdSel;
    DEL  delHit;
    bool fHdc;
    DY   dyCol;
    /* Bitmap buffers for smooth drag */
    wxBitmap* bmpCol;
    wxBitmap* bmpScreenSave;
    wxBitmap* bmpT;
    int  izip;
};

/* Column structure (variable-length via rgcrd) */
struct COL {
    COLCLS* pcolcls;
    ColProc_t lpfnColProc;
    RC   rc;
    MOVE* pmove;
    int  icrdMac;
    int  icrdMax;
    CRD  rgcrd[1]; /* variable length array */
};

/* ---- Column messages ---- */
#define icolNil            -1
#define msgcNil             0
#define msgcInit            1
#define msgcEnd             2
#define msgcClearCol        3
#define msgcNumCards        4
#define msgcHit             5
#define msgcSel             6
#define msgcEndSel          7
#define msgcFlip            8
#define msgcInvert          9
#define msgcMouseUp        10
#define msgcDblClk         11
#define msgcRemove         12
#define msgcInsert         13
#define msgcMove           14
#define msgcCopy           15
#define msgcValidMove      16
#define msgcValidMovePt    17
#define msgcRender         18
#define msgcPaint          19
#define msgcDrawOutline    20
#define msgcComputeCrdPos  21
#define msgcDragInvert     22
#define msgcGetPtInCrd     23
#define msgcValidKbdColSel 24
#define msgcValidKbdCrdSel 25
#define msgcShuffle        26
#define msgcAnimate        27
#define msgcZip            28

#define icrdNil    0x1fff
#define icrdEmpty  0x1ffe
#define icrdToEnd  0x1ffd
#define ccrdToEnd  -2
#define icrdEnd    0x1ffc

#define bitFZip    0x2000
#define icrdMask   0x1fff

/* SendColMsg macro */
#define SendColMsg(pcol, msgc, wp1, wp2) \
    (*((pcol)->lpfnColProc))((pcol), (msgc), (wp1), (wp2))

/* ---- Undo ---- */
#define icrdUndoMax 52

struct UDR {
    bool fAvail;
    bool fEndDeck;
    int  sco;
    int  icol1, icol2;
    int  irep;
    COL* rgpcol[2];
};

/* ---- Game messages ---- */
#define msggInit          0
#define msggEnd           1
#define msggKeyHit        2
#define msggMouseDown     3
#define msggMouseUp       4
#define msggMouseMove     5
#define msggMouseDblClk   6
#define msggPaint         7
#define msggDeal          8
#define msggUndo          9
#define msggSaveUndo     10
#define msggKillUndo     11
#define msggIsWinner     12
#define msggWinner       13
#define msggScore        14
#define msggChangeScore  15
#define msggDrawStatus   16
#define msggTimer        17
#define msggForceWin     18
#define msggMouseRightClk 19

/* Game procedure type */
typedef intptr_t (*GmProc_t)(GM*, int, intptr_t, intptr_t);

/* Game structure (variable-length via rgpcol) */
struct GM {
    GmProc_t lpfnGmProc;
    UDR  udr;
    bool fDealt;
    bool fInput;
    bool fWon;
    int  sco;
    int  iqsecScore;
    int  dqsecScore;
    int  ccrdDeal;
    int  irep;
    PT   ptMousePrev;
    bool fButtonDown;
    int  icolKbd;
    int  icrdKbd;
    int  icolSel;
    int  icolHilight;
    DY   dyDragMax;
    int  icolMac;
    int  icolMax;
    COL* rgpcol[1]; /* variable length array */
};

/* SendGmMsg macro */
#define SendGmMsg(pgm, msgg, wp1, wp2) \
    (*((pgm)->lpfnGmProc))((pgm), (msgg), (wp1), (wp2))

#define FSelOfGm(pgm)     ((pgm)->icolSel != icolNil)
#define FHilightOfGm(pgm) ((pgm)->icolHilight != icolNil)

/* Score mode */
#define smdStandard 302
#define smdVegas    303
#define smdNone     304

/* Change score codes */
#define csNil    -1
#define csAbs    -2
#define csDel    -3
#define csDelPos -4

/* ---- Klondike constants ---- */
#define MENU_HEIGHT 0

#define icolDeck       0
#define icolDiscard    1
#define icolFoundFirst 2
#define ccolFound      4
#define icolTabFirst   6
#define ccolTab        7

#define icrdDeckMax    52
#define icrdDiscardMax (icrdDeckMax - (1+2+3+4+5+6+7))
#define icrdFoundMax   13
#define icrdTabMax     19

/* Klondike score codes */
#define csKlondTime     0
#define csKlondDeckFlip 1
#define csKlondFound    2
#define csKlondTab      3
#define csKlondTabFlip  4
#define csKlondFoundTab 5
#define csKlondDeal     6
#define csKlondWin      7
#define csKlondMax      (csKlondWin + 1)

/* ---- Memory helpers ---- */
void* PAlloc(int cb);
void  FreeP(void* p);

/* ---- Utility functions ---- */
int  WMin(int w1, int w2);
int  WMax(int w1, int w2);
bool FInRange(int w, int wFirst, int wLast);
int  PegRange(int w, int wFirst, int wLast);
void OffsetPt(PT* ppt, DEL* pdel, PT* pptDest);
void SwapCards(CRD* pcrd1, CRD* pcrd2);
bool FPtInCrd(CRD* pcrd, PT pt);
bool FRectIsect(RC* prc1, RC* prc2);
bool FCrdRectIsect(CRD* pcrd, RC* prc);
void CrdRcFromPt(PT* ppt, RC* prc);
bool PtInRC(PT* ppt, RC* prc);

/* ---- Drawing ---- */
/* Global drawing context - set before draw operations */
extern wxDC* g_dc;
extern X xOrgCur;
extern Y yOrgCur;

wxDC* DcSet(wxDC* dc, X xOrg, Y yOrg);

/* Card drawing functions */
bool cdtInit(int* pdxCard, int* pdyCard);
void cdtTerm();
bool cdtDrawExt(wxDC* dc, int x, int y, int dx, int dy, int cd, int mode, unsigned long rgbBgnd);
bool cdtDraw(wxDC* dc, int x, int y, int cd, int mode, unsigned long rgbBgnd);
bool cdtAnimate(wxDC* dc, int cd, int x, int y, int ispr);

/* Drawing helpers */
void DrawCard(CRD* pcrd);
void DrawCardPt(CRD* pcrd, PT* ppt);
void DrawCardExt(PT* ppt, int cd, int mode);
void DrawBackground(X xLeft, Y yTop, X xRight, Y yBot);
void DrawBackExcl(COL* pcol, PT* ppt);
void DrawOutline(PT* ppt, int ccrd, DX dx, DY dy);
void EraseScreen();
void InvertRc(RC* prc);

/* HDC management (using wxClientDC) */
bool FGetHdc();
void ReleaseHdc();

/* ---- Column functions ---- */
COLCLS* PcolclsCreate(int tcls, ColProc_t lpfnColProc,
                       DX dxUp, DY dyUp, DX dxDn, DY dyDn,
                       int dcrdUp, int dcrdDn);
COL* PcolCreate(COLCLS* pcolcls, X xLeft, Y yTop, X xRight, Y yBot, int icrdMax);
int  DefColProc(COL* pcol, int msgc, intptr_t wp1, intptr_t wp2);

/* ---- Game functions ---- */
bool FInitGm();
bool FInitKlondGm();
void FreeGm(GM* pgm);
int  DefGmProc(GM* pgm, int msgg, intptr_t wp1, intptr_t wp2);
bool FSetDrag(bool fOutline);
void NewKbdColAbs(GM* pgm, int icol);

/* ---- Undo functions ---- */
bool FInitUndo(UDR* pudr);
void FreeUndo(UDR* pudr);

/* ---- Klondike col procs ---- */
intptr_t DeckColProc(COL* pcol, int msgc, intptr_t wp1, intptr_t wp2);
intptr_t DiscardColProc(COL* pcol, int msgc, intptr_t wp1, intptr_t wp2);
intptr_t TabColProc(COL* pcol, int msgc, intptr_t wp1, intptr_t wp2);
intptr_t FoundColProc(COL* pcol, int msgc, intptr_t wp1, intptr_t wp2);
intptr_t KlondGmProc(GM* pgm, int msgg, intptr_t wp1, intptr_t wp2);
bool PositionCols();
bool TabDiscardDblClk(COL* pcol, PT* ppt, int icol);
bool FValidCol(COL* pcol);

/* ---- Global state ---- */
extern GM*   pgmCur;
extern int   dxCrd, dyCrd;
extern int   dxScreen, dyScreen;
extern unsigned long rgbTable;
extern int   modeFaceDown;
extern bool  fIconic;
extern bool  fStatusBar;
extern bool  fTimedGame;
extern bool  fKeepScore;
extern SMD   smd;
extern int   ccrdDeal;
extern bool  fOutlineDrag;
extern bool  fHalfCards;
extern int   xCardMargin;
extern int   igmCur;
extern bool  fBW;
extern bool  fKlondWinner;
extern bool  fMegaDiscardHack;
extern MOVE  g_move;
extern PT    ptNil;
extern int   dyChar;
extern int   dxChar;
extern int   usehdcCur;
extern wxBitmap* g_backingStore;

/* Ensure the backing store exists at canvas size */
void EnsureBackingStore();

#define MIN_MARGIN ((dxCrd / 8) + 3)
#define bltb(pb1, pb2, cb) memcpy(pb2, pb1, cb)

/* ---- wxWidgets application classes ---- */

class SolCanvas;
class SolFrame;

class SolApp : public wxApp {
public:
    bool OnInit() override;
    SolFrame* GetFrame() { return m_frame; }
private:
    SolFrame* m_frame = nullptr;
};

wxDECLARE_APP(SolApp);

class SolCanvas : public wxPanel {
public:
    SolCanvas(wxWindow* parent);

    void OnPaint(wxPaintEvent& evt);
    void OnLeftDown(wxMouseEvent& evt);
    void OnLeftUp(wxMouseEvent& evt);
    void OnLeftDClick(wxMouseEvent& evt);
    void OnRightDown(wxMouseEvent& evt);
    void OnMouseMove(wxMouseEvent& evt);
    void OnKeyDown(wxKeyEvent& evt);
    void OnSize(wxSizeEvent& evt);

    wxDECLARE_EVENT_TABLE();
};

class SolFrame : public wxFrame {
public:
    SolFrame();

    void OnNewGame(wxCommandEvent& evt);
    void OnUndo(wxCommandEvent& evt);
    void OnDeck(wxCommandEvent& evt);
    void OnOptions(wxCommandEvent& evt);
    void OnExit(wxCommandEvent& evt);
    void OnAbout(wxCommandEvent& evt);
    void OnHelp(wxCommandEvent& evt);
    void OnTimer(wxTimerEvent& evt);
    void OnMenuOpen(wxMenuEvent& evt);
    void OnClose(wxCloseEvent& evt);
    void OnActivate(wxActivateEvent& evt);

    void NewGame(bool fNewSeed, bool fZeroScore);
    void UpdateStatusText();
    SolCanvas* GetCanvas() { return m_canvas; }

    enum {
        ID_NEWGAME = wxID_HIGHEST + 1,
        ID_UNDO,
        ID_DECK,
        ID_OPTIONS,
        ID_TIMER,
        ID_FORCEWIN
    };

private:
    SolCanvas* m_canvas;
    wxTimer    m_timer;
    wxStatusBar* m_statusBar;

    wxDECLARE_EVENT_TABLE();
};

/* Global frame/canvas access */
extern SolFrame*  g_frame;
extern SolCanvas* g_canvas;

/* Status bar helpers */
void StatUpdate();
void StatStringSz(const wxString& sz);

#endif /* WXSOL_H */
