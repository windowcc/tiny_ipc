#ifndef _IPC_EXPORT_H_
#define _IPC_EXPORT_H_

/*
 * Compiler & system detection for DllExport & DllImport.
 * Not using QtCore cause it shouldn't depend on Qt.
*/

#if defined(_MSC_VER)
#   define DllExport      __declspec(dllexport)
#   define DllImport      __declspec(dllimport)
#elif defined(__ARMCC__) || defined(__CC_ARM)
#   if defined(ANDROID) || defined(__linux__) || defined(__linux)
#       define DllExport  __attribute__((visibility("default")))
#       define DllImport  __attribute__((visibility("default")))
#   else
#       define DllExport  __declspec(dllexport)
#       define DllImport  __declspec(dllimport)
#   endif
#elif defined(__GNUC__)
#   if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || \
       defined(WIN64) || defined(_WIN64) || defined(__WIN64__)
#       define DllExport  __declspec(dllexport)
#       define DllImport  __declspec(dllimport)
#   else
#       define DllExport  __attribute__((visibility("default")))
#       define DllImport  __attribute__((visibility("default")))
#   endif
#else
#   define DllExport      __attribute__((visibility("default")))
#   define DllImport      __attribute__((visibility("default")))
#endif
/*
 * Define IPC_EXPORT for exporting function & class.
*/

#ifndef IPC_EXPORT
#if defined(LIBIPC_LIBRARY_SHARED_BUILDING__)
#  define IPC_EXPORT DllExport
#elif defined(LIBIPC_LIBRARY_SHARED_USING__)
#  define IPC_EXPORT DllImport
#else
#  define IPC_EXPORT
#endif
#endif /*IPC_EXPORT*/

#endif // ! _IPC_EXPORT_H_