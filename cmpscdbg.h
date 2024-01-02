/* CMPSCDBG.H   (C) Copyright "Fish" (David B. Trout), 2012-2014     */
/*              Compression Call Debugging Functions                 */
/*                                                                   */
/*  SPDX-FileCopyrightText: Copyright "Fish" (David B. Trout)        */
/*  SPDX-License-Identifier: QPL-1.0                                 */

#ifndef _CMPSCDBG_H_
#define _CMPSCDBG_H_    // Code to be compiled ONLY ONCE goes after here

// TODO: define any structures, etc, the debugging code might need...

#endif // _CMPSCDBG_H_    // Place all 'ARCH_DEP' code after this statement

///////////////////////////////////////////////////////////////////////////////
//
//  FILE: cmpscdbg.h
//
//      Header for the Compression Call debugging functions
//
//  FUNCTIONS:
//
//      cmpsc_Report() - Formatted dump of internal structures
//
//  PARAMETERS:
//
//      dbg     Pointer to debugging context
//
//  RETURNS:
//
//      int     0 if success, errno on failure.
//
//  COMMENTS:
//
///////////////////////////////////////////////////////////////////////////////

extern int (CMPSC_FASTCALL ARCH_DEP( cmpsc_Report ))( void* dbg );

///////////////////////////////////////////////////////////////////////////////
