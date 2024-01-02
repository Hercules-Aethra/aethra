/* HERCIFC.H    Hercules Interface Control Program                   */
/*                                                                   */
/*  SPDX-FileCopyrightText: Copyright the following contributors:    */
/*  SPDX-FileContributor:   Roger Bowler                             */
/*  SPDX-FileContributor:   Janmes A. Pierson                        */
/*  SPDX-License-Identifier: QPL-1.0                                 */

#ifndef __HERCIFC_H_
#define __HERCIFC_H_

#include "hifr.h"                       /* need struct hifr */

#define    HERCIFC_CMD  "hercifc"

typedef struct _CTLREQ
{
    long                iType;
    int                 iProcID;
    unsigned long int   iCtlOp;
    char                szIFName[IFNAMSIZ];
    union
    {
        struct hifr     hifr;
#if !defined(__APPLE__) && !defined( FREEBSD_OR_NETBSD ) && !defined(__SOLARIS__)
        struct rtentry  rtentry;
#endif
    }
    iru;
}
CTLREQ, *PCTLREQ;
#define CTLREQ_SIZE     sizeof( CTLREQ )
#define CTLREQ_OP_DONE  0

#endif // __HERCIFC_H_
