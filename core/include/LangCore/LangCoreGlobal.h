#ifndef LANGCORE_GLOBAL_H
#define LANGCORE_GLOBAL_H

#include <stdcorelib/stdc_global.h>

#ifndef LANGCORE_EXPORT
#ifdef LANGCORE_STATIC
#define LANGCORE_EXPORT
#else
#ifdef LANGCORE_LIBRARY
#define LANGCORE_EXPORT STDCORELIB_DECL_EXPORT
#else
#define LANGCORE_EXPORT STDCORELIB_DECL_IMPORT
#endif
#endif
#endif

#endif // LANGCORE_GLOBAL_H
