/* HBYTESWP.C   Hercules Little <> Big Endian conversion             */
/*                                                                   */
/*  SPDX-FileCopyrightText: Copyright Roger Bowler                   */
/*  SPDX-License-Identifier: QPL-1.0                                 */

/*-------------------------------------------------------------------*/
/* These definitions are only nessesary when running on older        */
/* versions of linux that do not have /usr/include/byteswap.h.       */
/* Compile option -DNO_ASM_BYTESWAP will expand 'C' code.            */
/* Otherwise custom assember will be generated.  (Jan Jaeger)        */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _MACHDEP_C_
#define _HENGINE_DLL_

#include "hercules.h"

#if !defined( _MSVC_ )                  \
 && !defined( NO_ASM_BYTESWAP )         \
 && !defined( HAVE_SWAP_BUILTINS )

    #include "htypes.h"   // (need Hercules fixed-size data types)

    extern inline uint16_t ( ATTR_REGPARM(1) bswap_16 )( uint16_t  x );
    extern inline uint32_t ( ATTR_REGPARM(1) bswap_32 )( uint32_t  x );
    extern inline uint64_t ( ATTR_REGPARM(1) bswap_64 )( uint64_t  x );

#endif // !defined( NO_ASM_BYTESWAP )
