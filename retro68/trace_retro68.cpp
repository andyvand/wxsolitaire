/* trace_retro68.cpp -- startup tracing for the classic Mac OS build.

   Built only with -DSOLITAIRE_TRACE=ON. Writes "wxtest.log" next to the
   application, flushed line by line so it survives a crash: checkpoints,
   every wxLog message, exit() and std::terminate(). Once the main window
   has been up for a while it logs that, and if "wxtest.shutdown" is in the
   Preferences folder it shuts the Mac down, so an emulator run can be
   unattended (see retro68/run-basilisk.sh). */

#ifdef SOLITAIRE_TRACE

#include <wx/wx.h>
#include <wx/log.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <exception>
#include <Memory.h>
#include <Multiverse.h>
#include "trace_retro68.h"

static FILE *gs_trace;

void SolTrace(const char *fmt, ...)
{
    if (!gs_trace)
        gs_trace = fopen("wxtest.log", "w");
    if (!gs_trace)
        return;
    fprintf(gs_trace, "[%lu free=%ld] ", (unsigned long)TickCount(), (long)FreeMem());
    va_list args;
    va_start(args, fmt);
    vfprintf(gs_trace, fmt, args);
    va_end(args);
    fputc('\n', gs_trace);
    fflush(gs_trace);
}

namespace
{
class TraceLog : public wxLog
{
protected:
    void DoLogTextAtLevel(wxLogLevel level, const wxString &msg) override
    {
        SolTrace("wxLog(%d): %s", (int)level, (const char *)msg.utf8_str());
    }
};

void AtExit()
{
    SolTrace("exit() called");
}

void OnTerminate()
{
    SolTrace("std::terminate() called");
    abort();
}

class AliveTimer : public wxTimer
{
public:
    void Notify() override
    {
        SolTrace("ALIVE: main window up for %d s", ++m_ticks * 5);
        if (m_ticks < 5)
            return;
        Stop();
        const wxFileName marker(wxStandardPaths::Get().GetUserConfigDir(),
                                "wxtest.shutdown");
        if (marker.FileExists())
        {
            SolTrace("shutting down");
            fclose(gs_trace);
            gs_trace = nullptr;
            ShutDwnPower();
        }
    }
private:
    int m_ticks = 0;
};
}

void SolTraceInit()
{
    SolTrace("trace start");
    char here;
    Ptr applLimit = LMGetApplLimit();
    SolTrace("stack: StackSpace=%ld ApplLimit=%p CurStackBase=%p SP~%p HeapEnd=%p",
                (long)StackSpace(), (void *)applLimit, (void *)LMGetCurStackBase(),
                (void *)&here, (void *)LMGetHeapEnd());
    atexit(AtExit);
    std::set_terminate(OnTerminate);
    delete wxLog::SetActiveTarget(new TraceLog);
    wxLog::SetLogLevel(wxLOG_Max);
    wxLog::SetVerbose(true);
}

void SolTraceAlive(wxFrame *)
{
    static AliveTimer timer;
    timer.Start(5000);
}

#endif // SOLITAIRE_TRACE
