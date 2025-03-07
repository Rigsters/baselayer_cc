#ifndef __PLATFORM_H__
#define __PLATFORM_H__

// platform-specific implementations of functions
// not currently distributed in separate files

#if defined __linux__ || defined __linux
    #define LINUX 1
    #define WINDOWS 0

    #ifdef __arm__
        #define RPI 1
    #endif
    #ifdef __aarch64__
        #define RPI 1
    #endif

    #ifndef RPI
    #define RPI 0
    #endif
    #include "plaf/plaf_linux.cpp"
#else 
    #define LINUX 0
    #define WINDOWS 1
    #define RPI 0

    #include "plaf/plaf_win.cpp"
#endif


#if LINUX
#ifndef DLL_EXPORT
#define DLL_EXPORT
#endif
#ifndef DLL_IMPORT
#define DLL_IMPORT
#endif
#elif WINDOWS
#ifndef DLL_EXPORT
#define DLL_EXPORT __declspec(dllexport)
#endif
#ifndef DLL_IMPORT
#define DLL_IMPORT __declspec(dllimport)
#endif
#endif


#ifndef DLL_CLIENT
#ifndef DLL_IMPORTEXPORT
#define DLL_IMPORTEXPORT DLL_EXPORT
#endif
#else
#ifndef DLL_IMPORTEXPORT
#define DLL_IMPORTEXPORT DLL_IMPORT
#endif
#endif


#endif
