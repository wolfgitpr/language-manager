#ifndef LANGMGR_GLOBAL_H
#define LANGMGR_GLOBAL_H

#include <stdcorelib/stdc_global.h>

#ifndef LANGMGR_EXPORT
#ifdef LANGMGR_STATIC
#define LANGMGR_EXPORT
#else
#ifdef LANGMGR_LIBRARY
#define LANGMGR_EXPORT STDCORELIB_DECL_EXPORT
#else
#define LANGMGR_EXPORT STDCORELIB_DECL_IMPORT
#endif
#endif
#endif

#endif // LANGMGR_GLOBAL_H
