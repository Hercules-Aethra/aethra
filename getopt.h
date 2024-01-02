/*  GETOPT.H    NetBSD getopt parsing function                       */
/*                                                                   */
/*  SPDX-FileCopyrightText: Copyright the following contributors:    */
/*  SPDX-FileContributor:   Roger Bowler                             */
/*  SPDX-FileContributor:   The NetBSD Foundation, Inc.              */
/*  SPDX-License-Identifier: QPL-1.0 AND BSD-3-Clause                */

/*  The QPL-1.0 license applies only to this specific version of     */
/*  this file, and only those changes made by contributors to        */
/*  Hercules. No claim is made to the file as distributed as part    */
/*  of NetBSD.                                                       */

#ifndef __GETOPT_H__
#define __GETOPT_H__
#endif /* __GETOPT_H__ */

#ifndef __UNISTD_GETOPT__
#ifndef __GETOPT_LONG_H__
#define __GETOPT_LONG_H__

struct option {
    const char *name;
    int  has_arg;
    int *flag;
    int val;
};

#ifndef HAVE_DECL_GETOPT
#define HAVE_DECL_GETOPT 1
#endif

#define no_argument             0
#define required_argument       1
#define optional_argument       2

#endif /* __GETOPT_LONG_H__ */
#endif /* __UNISTD_GETOPT__ */
