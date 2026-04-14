#include "wxsol.h"
#include <wx/xrc/xmlres.h>
#include <wx/fs_mem.h>
#include <wx/fs_arc.h>
#include "cards_xrs.h"

/* Card face XRC resource names indexed by resource ID (1-52) */
static const char* s_cardNames[] = {
    nullptr,     /* 0: unused */
    "cla",       /* 1: Ace of Clubs */
    "cl2",       /* 2: 2 of Clubs */
    "cl3",       /* 3: 3 of Clubs */
    "cl4",       /* 4: 4 of Clubs */
    "cl5",       /* 5: 5 of Clubs */
    "cl6",       /* 6: 6 of Clubs */
    "cl7",       /* 7: 7 of Clubs */
    "cl8",       /* 8: 8 of Clubs */
    "cl9",       /* 9: 9 of Clubs */
    "cl10",      /* 10: 10 of Clubs */
    "clj",       /* 11: Jack of Clubs */
    "clq",       /* 12: Queen of Clubs */
    "clk",       /* 13: King of Clubs */
    "dma",       /* 14: Ace of Diamonds */
    "dm2",       /* 15: 2 of Diamonds */
    "dm3",       /* 16: 3 of Diamonds */
    "dm4",       /* 17: 4 of Diamonds */
    "dm5",       /* 18: 5 of Diamonds */
    "dm6",       /* 19: 6 of Diamonds */
    "dm7",       /* 20: 7 of Diamonds */
    "dm8",       /* 21: 8 of Diamonds */
    "dm9",       /* 22: 9 of Diamonds */
    "dm10",      /* 23: 10 of Diamonds */
    "dmj",       /* 24: Jack of Diamonds */
    "dmq",       /* 25: Queen of Diamonds */
    "dmk",       /* 26: King of Diamonds */
    "hta",       /* 27: Ace of Hearts */
    "ht2",       /* 28: 2 of Hearts */
    "ht3",       /* 29: 3 of Hearts */
    "ht4",       /* 30: 4 of Hearts */
    "ht5",       /* 31: 5 of Hearts */
    "ht6",       /* 32: 6 of Hearts */
    "ht7",       /* 33: 7 of Hearts */
    "ht8",       /* 34: 8 of Hearts */
    "ht9",       /* 35: 9 of Hearts */
    "ht10",      /* 36: 10 of Hearts */
    "htj",       /* 37: Jack of Hearts */
    "htq",       /* 38: Queen of Hearts */
    "htk",       /* 39: King of Hearts */
    "spa",       /* 40: Ace of Spades */
    "sp2",       /* 41: 2 of Spades */
    "sp3",       /* 42: 3 of Spades */
    "sp4",       /* 43: 4 of Spades */
    "sp5",       /* 44: 5 of Spades */
    "sp6",       /* 45: 6 of Spades */
    "sp7",       /* 46: 7 of Spades */
    "sp8",       /* 47: 8 of Spades */
    "sp9",       /* 48: 9 of Spades */
    "sp10",      /* 49: 10 of Spades */
    "spj",       /* 50: Jack of Spades */
    "spq",       /* 51: Queen of Spades */
    "spk",       /* 52: King of Spades */
};

/* Card back XRC resource names indexed from IDFACEDOWN1..IDFACEDOWN12 */
static const char* s_backNames[] = {
    "rbhatch",   /* IDFACEDOWN1 = 54 */
    "bghatch",   /* IDFACEDOWN2 = 55 */
    "brobot",    /* IDFACEDOWN3 = 56 */
    "rose",      /* IDFACEDOWN4 = 57 */
    "bkvine",    /* IDFACEDOWN5 = 58 */
    "blvine",    /* IDFACEDOWN6 = 59 */
    "cfish",     /* IDFACEDOWN7 = 60 */
    "bfish",     /* IDFACEDOWN8 = 61 */
    "shell",     /* IDFACEDOWN9 = 62 */
    "krazkstl",  /* IDFACEDOWN10 = 63 */
    "sanflipe",  /* IDFACEDOWN11 = 64 */
    "slimeup",   /* IDFACEDOWN12 = 65 */
};

/* Animation XRC resource names (indexed by resource ID) */
struct AniEntry {
    int id;
    const char* name;
};
static AniEntry s_aniEntries[] = {
    { IDASLIME1,  "aslime1" },
    { IDASLIME2,  "aslime2" },
    { IDAKASTL1,  "akstl1" },
    { IDAFLIPE1,  "aflipe1" },
    { IDAFLIPE2,  "aflipe2" },
    { IDABROBOT1, "abrobot1" },
    { IDABROBOT2, "abrobot2" },
};

/* ---- Animation data ---- */
struct SPR {
    int id;
    DX  dx;
    DY  dy;
};

#define isprMax 4

struct ANI {
    int cdBase;
    DX  dxspr;
    DY  dyspr;
    int isprMac;
    SPR rgspr[isprMax];
};

#define ianiMax 4
static ANI rgani[ianiMax] = {
    { IDFACEDOWN12, 32, 22, 4,
        { { IDASLIME1, 32, 32 }, { IDASLIME2, 32, 32 },
          { IDASLIME1, 32, 32 }, { IDFACEDOWN12, 32, 32 } }},
    { IDFACEDOWN10, 36, 12, 2,
        { { IDAKASTL1, 42, 12 }, { IDFACEDOWN10, 42, 12 },
          { 0, 0, 0 }, { 0, 0, 0 } }},
    { IDFACEDOWN11, 14, 12, 4,
        { { IDAFLIPE1, 47, 1 }, { IDAFLIPE2, 47, 1 },
          { IDAFLIPE1, 47, 1 }, { IDFACEDOWN11, 47, 1 } }},
    { IDFACEDOWN3, 24, 7, 4,
        { { IDABROBOT1, 24, 40 }, { IDABROBOT2, 24, 40 },
          { IDABROBOT1, 24, 40 }, { IDFACEDOWN3, 24, 40 } }}
};

/* ---- Loaded bitmaps ---- */
static wxBitmap* s_hbmCard[52] = {};
static wxBitmap* s_hbmGhost = nullptr;
static wxBitmap* s_hbmBack = nullptr;
static wxBitmap* s_hbmDeckX = nullptr;
static wxBitmap* s_hbmDeckO = nullptr;
static int       s_idback = 0;
static int       s_dxCard = 0;
static int       s_dyCard = 0;

/* ---- XRC resource initialization ---- */
static bool s_xrcInitialized = false;

static bool InitCardResources()
{
    if (s_xrcInitialized)
        return true;

    wxXmlResource::Get()->InitAllHandlers();
    wxFileSystem::AddHandler(new wxArchiveFSHandler);
    wxFileSystem::AddHandler(new wxMemoryFSHandler);
    wxMemoryFSHandler::AddFile(wxT("cards.xrs"), cards_xrs, sizeof(cards_xrs));

    if (!wxXmlResource::Get()->Load(wxT("memory:cards.xrs"))) {
        return false;
    }

    s_xrcInitialized = true;
    return true;
}

/* ---- Bitmap loading from XRC ---- */

static wxBitmap* LoadBmpByName(const char* name)
{
    wxBitmap bmp = wxXmlResource::Get()->LoadBitmap(wxString(name));
    if (bmp.IsOk())
        return new wxBitmap(bmp);
    return nullptr;
}

static wxBitmap* LoadBmpById(int id)
{
    /* Card face (1-52) */
    if (id >= 1 && id <= 52)
        return LoadBmpByName(s_cardNames[id]);
    /* Card backs */
    if (id >= IDFACEDOWNFIRST && id <= IDFACEDOWNLAST)
        return LoadBmpByName(s_backNames[id - IDFACEDOWNFIRST]);
    /* Animation frames */
    for (auto& ae : s_aniEntries) {
        if (ae.id == id)
            return LoadBmpByName(ae.name);
    }
    return nullptr;
}

/* Get bitmap for card cd (0-51) */
static wxBitmap* HbmFromCd(int cd)
{
    if (s_hbmCard[cd] == nullptr) {
        int icd = RaFromCd(cd) % 13 + SuFromCd(cd) * 13;
        s_hbmCard[cd] = LoadBmpById(icd + 1);
    }
    return s_hbmCard[cd];
}

static bool FLoadBack(int idbackNew)
{
    if (idbackNew < IDFACEDOWNFIRST || idbackNew > IDFACEDOWNLAST)
        return false;
    if (s_idback != idbackNew) {
        delete s_hbmBack;
        s_hbmBack = LoadBmpById(idbackNew);
        if (s_hbmBack)
            s_idback = idbackNew;
        else
            s_idback = 0;
    }
    return s_idback != 0;
}

/* ---- Public API ---- */

bool cdtInit(int* pdxCard, int* pdyCard)
{
    if (!InitCardResources())
        return false;

    s_hbmGhost = LoadBmpByName("ghost");
    s_hbmDeckX = LoadBmpByName("x");
    s_hbmDeckO = LoadBmpByName("o");

    if (!s_hbmGhost || !s_hbmDeckX || !s_hbmDeckO) {
        cdtTerm();
        return false;
    }

    s_dxCard = s_hbmGhost->GetWidth();
    s_dyCard = s_hbmGhost->GetHeight();
    *pdxCard = s_dxCard;
    *pdyCard = s_dyCard;
    return true;
}

void cdtTerm()
{
    for (int i = 0; i < 52; i++) {
        delete s_hbmCard[i];
        s_hbmCard[i] = nullptr;
    }
    delete s_hbmGhost; s_hbmGhost = nullptr;
    delete s_hbmBack;  s_hbmBack = nullptr;
    delete s_hbmDeckX; s_hbmDeckX = nullptr;
    delete s_hbmDeckO; s_hbmDeckO = nullptr;
    s_idback = 0;
}

bool cdtDrawExt(wxDC* dc, int x, int y, int dx, int dy, int cd, int mode, unsigned long rgbBgnd)
{
    if (!dc)
        return false;

    wxBitmap* hbmSav = nullptr;
    bool fInvert = false;

    switch (mode & 0x7fffffff) {
    case FACEUP:
        hbmSav = HbmFromCd(cd);
        break;
    case FACEDOWN:
        if (!FLoadBack(cd))
            return false;
        hbmSav = s_hbmBack;
        break;
    case REMOVE:
    case GHOST: {
        wxBrush br(wxColour((rgbBgnd & 0xFF), (rgbBgnd >> 8) & 0xFF, (rgbBgnd >> 16) & 0xFF));
        dc->SetBrush(br);
        dc->SetPen(*wxTRANSPARENT_PEN);
        dc->DrawRectangle(x, y, dx, dy);
        if (mode == REMOVE)
            return true;
        hbmSav = s_hbmGhost;
        break;
    }
    case INVISIBLEGHOST:
        hbmSav = s_hbmGhost;
        break;
    case DECKX:
        hbmSav = s_hbmDeckX;
        break;
    case DECKO:
        hbmSav = s_hbmDeckO;
        break;
    case HILITE:
        hbmSav = HbmFromCd(cd);
        fInvert = true;
        break;
    default:
        return false;
    }

    if (!hbmSav || !hbmSav->IsOk())
        return false;

    wxMemoryDC memDC;
    memDC.SelectObject(*hbmSav);

    if ((mode & 0x7fffffff) == GHOST || (mode & 0x7fffffff) == INVISIBLEGHOST) {
        /* wxAND is not reliable on macOS Cocoa.
           Convert the ghost bitmap (black outline on white) to an
           image with transparency, then draw it.  Black pixels become
           a dark outline; white pixels are transparent. */
        wxImage img = hbmSav->ConvertToImage();
        if (dx != s_dxCard || dy != s_dyCard)
            img.Rescale(dx, dy);
        int iw = img.GetWidth();
        int ih = img.GetHeight();
        if (!img.HasAlpha())
            img.InitAlpha();
        unsigned char* alpha = img.GetAlpha();
        unsigned char* data  = img.GetData();
        /* Dark green for the outline */
        unsigned char outR = 0, outG = 64, outB = 0;
        for (int p = 0; p < iw * ih; p++) {
            unsigned char r = data[p * 3];
            unsigned char g = data[p * 3 + 1];
            unsigned char b = data[p * 3 + 2];
            if (r < 128 && g < 128 && b < 128) {
                /* Dark pixel = outline: make opaque dark green */
                data[p * 3]     = outR;
                data[p * 3 + 1] = outG;
                data[p * 3 + 2] = outB;
                alpha[p] = 255;
            } else {
                /* Light pixel = transparent */
                alpha[p] = 0;
            }
        }
        wxBitmap ghostBmp(img);
        dc->DrawBitmap(ghostBmp, x, y, true);
    } else if (fInvert) {
        /* Invert blit for highlight */
        dc->Blit(x, y, dx, dy, &memDC, 0, 0, wxSRC_INVERT);
    } else {
        if (dx != s_dxCard || dy != s_dyCard) {
            /* Stretch */
            wxImage img = hbmSav->ConvertToImage();
            img.Rescale(dx, dy);
            wxBitmap scaled(img);
            wxMemoryDC scaledDC;
            scaledDC.SelectObject(scaled);
            dc->Blit(x, y, dx, dy, &scaledDC, 0, 0, wxCOPY);
            scaledDC.SelectObject(wxNullBitmap);
        } else {
            dc->Blit(x, y, s_dxCard, s_dyCard, &memDC, 0, 0, wxCOPY);
        }
    }

    memDC.SelectObject(wxNullBitmap);

    /* Draw border for red cards (diamonds and hearts) */
    if ((mode & 0x7fffffff) == FACEUP && !(mode & 0x80000000)) {
        int icd = RaFromCd(cd) % 13 + SuFromCd(cd) * 13 + 1;
        if ((icd >= 14 && icd <= 23) || (icd >= 27 && icd <= 36)) {
            dc->SetPen(*wxBLACK_PEN);
            dc->DrawLine(x + 2, y, x + dx - 2, y);           /* top */
            dc->DrawLine(x + dx - 1, y + 2, x + dx - 1, y + dy - 2); /* right */
            dc->DrawLine(x + 2, y + dy - 1, x + dx - 2, y + dy - 1); /* bottom */
            dc->DrawLine(x, y + 2, x, y + dy - 2);            /* left */
        }
    }

    return true;
}

bool cdtDraw(wxDC* dc, int x, int y, int cd, int mode, unsigned long rgbBgnd)
{
    return cdtDrawExt(dc, x, y, s_dxCard, s_dyCard, cd, mode, rgbBgnd);
}

bool cdtAnimate(wxDC* dc, int cd, int x, int y, int ispr)
{
    if (ispr < 0 || !dc)
        return false;

    for (int iani = 0; iani < ianiMax; iani++) {
        if (cd == rgani[iani].cdBase) {
            ANI* pani = &rgani[iani];
            if (ispr < pani->isprMac) {
                SPR* pspr = &pani->rgspr[ispr];
                if (pspr->id == 0)
                    return false;

                X xSrc = 0, ySrc = 0;
                if (pspr->id == cd) {
                    xSrc = pspr->dx;
                    ySrc = pspr->dy;
                }

                wxBitmap* hbm = LoadBmpById(pspr->id);
                if (!hbm || !hbm->IsOk()) {
                    delete hbm;
                    return false;
                }

                wxMemoryDC memDC;
                memDC.SelectObject(*hbm);
                dc->Blit(x + pspr->dx, y + pspr->dy,
                         pani->dxspr, pani->dyspr,
                         &memDC, xSrc, ySrc, wxCOPY);
                memDC.SelectObject(wxNullBitmap);

                /* Animation bitmaps loaded on demand are not cached for backs;
                   but for non-back IDs we should delete */
                if (pspr->id < IDFACEDOWNFIRST || pspr->id > IDFACEDOWNLAST)
                    delete hbm;
                return true;
            }
        }
    }
    return false;
}
