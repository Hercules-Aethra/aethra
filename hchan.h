/* HCHAN.H      Generic channel device handler                       */
/*                                                                   */
/*  SPDX-FileCopyrightText: Copyright the following contributors:    */
/*  SPDX-FileContributor:   Roger Bowler                             */
/*  SPDX-FileContributor:   Jan Jaeger                               */
/*  SPDX-FileContributor:   Ivan Warren                              */
/*  SPDX-License-Identifier: QPL-1.0                                 */

#ifndef __HCHAN_H__
#define __HCHAN_H__
/*
 * Hercules Generic Channel internal definitions
 */

static  int     hchan_init_exec(DEVBLK *,int,char **);
static  int     hchan_init_connect(DEVBLK *,int,char **);
static  int     hchan_init_int(DEVBLK *,int,char **);

#endif
