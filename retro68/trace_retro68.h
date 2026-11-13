/* trace_retro68.h -- startup tracing for testing the classic Mac OS build
   in an emulator. Only compiled in with -DSOLITAIRE_TRACE=ON (CMake); see
   retro68/trace_retro68.cpp. */

#ifndef TRACE_RETRO68_H
#define TRACE_RETRO68_H

#ifdef SOLITAIRE_TRACE
void SolTrace(const char *fmt, ...);
void SolTraceInit();
void SolTraceAlive(class wxFrame *frame);
#  define STRACE(...) SolTrace(__VA_ARGS__)
#else
#  define STRACE(...) ((void)0)
#endif

#endif
