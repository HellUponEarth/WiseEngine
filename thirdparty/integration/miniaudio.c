// Code Modification - Audio: Compile miniaudio once so consumers share one implementation.
// Scope: isolated SDK bridge. Performance: mixer costs arise only when used; no device starts here. Toggle: none.
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
// End of Code Modification - Audio
