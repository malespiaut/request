#include <signal.h>

#include "error.h"
#include "signals.h"

int siglist[] = {
#ifdef SIGHUP
  SIGHUP,
#endif
#ifdef SIGINT
  SIGINT,
#endif
#ifdef SIGQUIT
  SIGQUIT,
#endif
#ifdef SIGILL
  SIGILL,
#endif
#ifdef SIGABRT
  SIGABRT,
#endif
#ifdef SIGFPE
  SIGFPE,
#endif
#ifdef SIGSEGV
  SIGSEGV,
#endif
#ifdef SIGTERM
  SIGTERM,
#endif
#ifdef SIGBUS
  SIGBUS,
#endif
  -1};

static void
signalAbort(int signum)
{
  int* p;

  for (p = siglist; *p != -1; ++p)
    signal(*p, SIG_DFL);

  Abort("signalAbort", "Terminating on signal %i", signum);
  raise(signum);
}

void
InitSignals()
{
  /* Set up various signals to call signalAbort() */
  int* p;

  for (p = siglist; *p != -1; ++p)
    signal(*p, signalAbort);
}
