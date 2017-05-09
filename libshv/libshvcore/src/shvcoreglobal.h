#pragma once

#  ifdef Q_OS_WIN
#    define Q_DECL_EXPORT     __declspec(dllexport)
#    define Q_DECL_IMPORT     __declspec(dllimport)
#  else
#    define Q_DECL_EXPORT     __attribute__((visibility("default")))
#    define Q_DECL_IMPORT     __attribute__((visibility("default")))
#    define Q_DECL_HIDDEN     __attribute__((visibility("hidden")))
#  endif

/// Declaration of macros required for exporting symbols
/// into shared libraries
#if defined(SHVCORE_BUILD_DLL)
#  define SHVCORE_DECL_EXPORT Q_DECL_EXPORT
#else
#  define SHVCORE_DECL_EXPORT Q_DECL_IMPORT
#endif

