/* Android-only link stubs for the prebuilt wxQt library.
 *
 * The wxQt static library shipped in the Qt Android kit was configured with
 * wxUSE_CONSOLE_EVENTLOOP == 0, so src/unix/evtloopunix.cpp (which defines
 * wxGUIAppTraits::GetEventLoopSourcesManager()) was never compiled. The GUI
 * app-traits vtable in that same library still references the symbol, however,
 * because the declaration is active under wxUSE_EVENTLOOP_SOURCE. That leaves
 * the symbol undefined at link time.
 *
 * The game never registers event-loop file-descriptor sources, so providing a
 * null-returning definition here satisfies the linker without changing
 * behaviour. This file is compiled only for Android.
 */
#ifdef __ANDROID__

#include <wx/apptrait.h>

#if wxUSE_EVENTLOOP_SOURCE

class wxEventLoopSourcesManagerBase;

wxEventLoopSourcesManagerBase* wxGUIAppTraits::GetEventLoopSourcesManager()
{
    return nullptr;
}

#endif /* wxUSE_EVENTLOOP_SOURCE */

#endif /* __ANDROID__ */
