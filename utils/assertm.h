#ifndef __ASSERTM_INCLUDED__
#define __ASSERTM_INCLUDED__

#include <cassert>

#define assertm(exp, msg) assert(((void)msg, exp))

#endif