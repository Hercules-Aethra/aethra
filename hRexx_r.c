/* HREXX_R.C    Regina Rexx Interpreter Support                      */
/*                                                                   */
/*  SPDX-FileCopyrightText: Copyright the following contributors:    */
/*  SPDX-FileContributor:   Enrico Sorichetti                        */
/*  SPDX-FileContributor:   "Fish" (David B. Trout)                  */
/*  SPDX-License-Identifier: QPL-1.0                                 */

/*  inspired by the previous Rexx implementation by Jan Jaeger       */

#include "hstdinc.h"

#define _HREXX_R_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "hRexx.h"

#ifdef  REGINA_REXX         // Support for Regina Rexx
#define REXX_PKG            REGINA_PKG
#define REXX_PKGNUM         REGINA_PKGNUM

/*-------------------------------------------------------------------*/
/* Macro to build unique name similar to Hercules ARCH_DEP macro     */
/*-------------------------------------------------------------------*/
#define PASTEM( _p, _n )        _p ## _n
#define REXX_DEP2( _p, _n )     PASTEM( _p, _n )
#define REXX_DEP( _name )       REXX_DEP2( REXX_PKG, _name )

/*-------------------------------------------------------------------*/
/* Regina Rexx Support headers                                       */
/*-------------------------------------------------------------------*/
#define INCL_REXXSAA
#if defined(HAVE_REGINA_REXXSAA_H)
  #include "regina/rexxsaa.h"
#else
  #include "rexxsaa.h"
#endif

/*-------------------------------------------------------------------*/
/* Herculess Rexx implementation equivalents for Regina Rexx types   */
/*-------------------------------------------------------------------*/
#define HR_REXXRC_T         APIRET            // (API return code)
#define HR_ENTRY            APIENTRY          // (calling convention)
#define HR_PFN_T            PFN               // (func ptr type)

#define HR_EXITHAND_RC_T    LONG              // (exit handler rc)
#define HR_SUBCOM_RC_T      APIRET            // (subcom handler rc)
#define HR_EXTFUNC_RC_T     APIRET            // (extern func rc)

#define HR_ARGC_T           LONG              // (array count)
#define HR_ARGV_T           PRXSTRING         // (RXSTRING array)
#define HR_PCSZ_T           PCSZ              // (const RXSTRING)
#define HR_CALLTYPE_T       LONG              // (RexxStart call type)
#define HR_FCODE_T          LONG              // (func/subfunc code)
#define HR_MEMSIZE_T        ULONG             // (AllocateMemory size)
#define HR_PROCESS_ID_T     LONG              // (RexxSetHalt argument)
#define HR_THREAD_ID_T      LONG              // (RexxSetHalt argument)
#define HR_USERINFO_T       PUCHAR            // (RegisterExit argument)

#define RXAPI_OK            0                 // (missing from Regina)

/*-------------------------------------------------------------------*/
/* Regina Rexx PRIMARY library names                                 */
/*-------------------------------------------------------------------*/
#if defined( _MSVC_ )
  #define REXX_LIBNAME      "regina.dll"
  #define REXX_APILIBNAME   ""
#elif defined( __APPLE__ )
  #define REXX_LIBNAME      "libregina.dylib"
  #define REXX_APILIBNAME   ""
#else // *nix
  #define REXX_LIBNAME      "libregina.so"
  #define REXX_APILIBNAME   ""
#endif

/*-------------------------------------------------------------------*/
/* Regina Rexx EXTRA library names                                   */
/*-------------------------------------------------------------------*/
static char*  REXX_DEP( ExtraLibs )[]  =
{
  #if defined( _MSVC_ )
    "regutil.dll"           ,
  #elif defined( __APPLE__ )
    NULL                    ,
  #else
    NULL                    ,
  #endif
};

#define NUM_EXTRALIBS       _countof( REXX_DEP( ExtraLibs ))

/*-------------------------------------------------------------------*/
/* Include the remainder of Hercules generic Rexx support functions  */
/*-------------------------------------------------------------------*/
#include "hRexxapi.c"

#endif // REGINA_REXX
