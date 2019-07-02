// add trace definitions for Dennis Bareis/AB pmprintf untility
// the following 2 defines controll verbosity level to pmprintf
#if (defined __DEBUG_PMPRINTF__) || (defined __DEBUG_PMPRINTF_LEVEL2__)
    #include "pmprintf.h"
    #include <STDARG.H>
    #define  __PMPRINTF__
// compilers which support VA_ARGS
    #define TRACE( ... )        PmpfF ( ( __VA_ARGS__ ) )       // doesn't work for VAC3.65 and ICCv4
#else
    #define TRACE( ... )
#endif  // __DEBUG_PMPRINTF__
#ifdef __DEBUG_PMPRINTF_LEVEL2__    // even more debug messages
    #define TRACE_L2( ... )     PmpfF ( ( __VA_ARGS__ ) )       // doesn't work for VAC3.65 and ICCv4
#else
    #define TRACE_L2( ... )
#endif  // __DEBUG_PMPRINTF__
// end trace definitions

