/* help_retro68.cpp -- Solitaire Help for classic Mac OS (Retro68).

   Classic Mac OS has no web browser to hand the HTML help to, so the help
   pages (zipped and embedded at build time, see CMakeLists.txt) are shown
   in a wxHtmlWindow of our own.  Links between the pages are relative, so
   they resolve inside the zip in the memory filesystem. */

#include "wxsol.h"
#include <wx/filesys.h>
#include <wx/fs_mem.h>
#include <wx/html/htmlwin.h>
#include <wx/sizer.h>

#include "help_data.h"

namespace
{

class HelpFrame : public wxFrame
{
public:
    explicit HelpFrame(wxWindow *parent)
        : wxFrame(parent, wxID_ANY, wxT("Solitaire Help"),
                  wxDefaultPosition, wxSize(480, 400))
    {
        m_html = new wxHtmlWindow(this, wxID_ANY);
        wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(m_html, 1, wxEXPAND);
        SetSizer(sizer);
        Bind(wxEVT_CLOSE_WINDOW, &HelpFrame::OnClose, this);
    }

    void ShowIndex()
    {
        m_html->LoadPage(wxT("memory:wxsolitaire_help.zip#zip:index.html"));
    }

private:
    void OnClose(wxCloseEvent &event);

    wxHtmlWindow *m_html;
};

HelpFrame *s_helpFrame = nullptr;

void HelpFrame::OnClose(wxCloseEvent &event)
{
    s_helpFrame = nullptr;
    event.Skip();
}

} // anonymous namespace


void ShowRetro68Help(wxWindow *parent)
{
    static bool registered = false;
    if (!registered)
    {
        /* The memory and archive handlers are added by cdtInit(). */
        wxMemoryFSHandler::AddFile(wxT("wxsolitaire_help.zip"), help_data, sizeof(help_data));
        registered = true;
    }

    if (!s_helpFrame)
        s_helpFrame = new HelpFrame(parent);
    s_helpFrame->ShowIndex();
    s_helpFrame->Show();
    s_helpFrame->Raise();
}
