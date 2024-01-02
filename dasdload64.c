/* DASDLOAD64.C Hercules DASD Utilities: DASD image loader           */
/*                                                                   */
/*  SPDX-FileCopyrightText: Copyright the following contributors:    */
/*  SPDX-FileContributor:   Roger Bowler                             */
/*  SPDX-FileContributor:   TurboHercules, SAS                       */
/*  SPDX-License-Identifier: QPL-1.0                                 */

/*-------------------------------------------------------------------*/
/* This program creates a virtual DASD volume from a list of         */
/* datasets previously unloaded using the TSO XMIT command.          */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Additional credits:                                               */
/*      Corrections to CVOL initialization logic by Jay Maynard      */
/*      IEBCOPY native dataset support by Ronen Tzur                 */
/*      Standardized Messages by P. Gorlinsky                        */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"
#include "hercules.h"
#include "dasdblks.h"
#include "ccwarn.h"
#include "cckddasd.h"   // (need cckdblk)

#define UTILITY_NAME    "dasdload64"
#define UTILITY_DESC    "Build 64-bit DASD from TSO XMIT files"

#define DASDLOAD64 1
#include "dasdload2.h"
