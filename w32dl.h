/* W32DL.H                 dlopen compat                             */
/*                                                                   */
/*  SPDX-FileCopyrightText: Copyright Jan Jaeger                     */
/*  SPDX-License-Identifier: QPL-1.0                                 */

#ifndef _W32_DL_H
#define _W32_DL_H

#ifdef _WIN32

#define RTLD_NOW 0

#define dlopen(_name, _flags) \
        (void*) ((_name) ? LoadLibrary((_name)) : GetModuleHandle( NULL ) )

#define dlsym(_handle, _symbol) \
        (void*)GetProcAddress((HMODULE)(_handle), (_symbol))

#define dlclose(_handle) \
        FreeLibrary((HMODULE)(_handle))

#define dlerror() \
        strerror( GetLastError() )

#endif /* _WIN32 */

#endif /* _W32_DL_H */
