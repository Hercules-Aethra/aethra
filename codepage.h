/* CODEPAGE.H   Code Page conversion                                 */
/*                                                                   */
/*  SPDX-FileCopyrightText: Copyright the following contributors:    */
/*  SPDX-FileContributor:   Jan Jaeger                               */
/*  SPDX-FileContributor:   TurboHercules, SAS                       */
/*  SPDX-License-Identifier: QPL-1.0                                 */

#ifndef _HERCULES_CODEPAGE_H
#define _HERCULES_CODEPAGE_H

#include "hercules.h"

COD_DLL_IMPORT const char* query_codepage();
COD_DLL_IMPORT bool valid_codepage_name( const char* name );
COD_DLL_IMPORT void set_codepage( const char *name);
COD_DLL_IMPORT void set_codepage_no_msgs( const char* name );
COD_DLL_IMPORT int update_codepage(int argc, char *argv[], char *table );
COD_DLL_IMPORT unsigned char host_to_guest (unsigned char byte);
COD_DLL_IMPORT unsigned char guest_to_host (unsigned char byte);
COD_DLL_IMPORT unsigned char *h2g_tab();
COD_DLL_IMPORT unsigned char *g2h_tab();

/* helper functions for codepage */
COD_DLL_IMPORT BYTE* buf_guest_to_host( const BYTE *psinbuf, BYTE *psoutbuf, const u_int ilength );
COD_DLL_IMPORT BYTE* str_guest_to_host( const BYTE *psinbuf, BYTE *psoutbuf, const u_int ilength );
COD_DLL_IMPORT BYTE* buf_host_to_guest( const BYTE *psinbuf, BYTE *psoutbuf, const u_int ilength );
COD_DLL_IMPORT BYTE* str_host_to_guest( const BYTE *psinbuf, BYTE *psoutbuf, const u_int ilength );
COD_DLL_IMPORT BYTE* prt_guest_to_host( const BYTE *psinbuf, BYTE *psoutbuf, const u_int ilength );
COD_DLL_IMPORT BYTE* prt_host_to_guest( const BYTE *psinbuf, BYTE *psoutbuf, const u_int ilength );

#endif /* _HERCULES_CODEPAGE_H */
