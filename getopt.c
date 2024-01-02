/*  GETOPT.C    NetBSD getopt parsing function                       */
/*                                                                   */
/*  SPDX-FileCopyrightText: Copyright the following contributors:    */
/*  SPDX-FileContributor:   Roger Bowler                             */
/*  SPDX-FileContributor:   The NetBSD Foundation, Inc.              */
/*  SPDX-License-Identifier: QPL-1.0 AND BSD-3-Clause                */

/*  The QPL-1.0 license applies only to this specific version of     */
/*  this file, and only those changes made by contributors to        */
/*  Hercules. No claim is made to the file as distributed as part    */
/*  of NetBSD.                                                       */

#include "hstdinc.h"

#define _GETOPT_C_
#define _HUTIL_DLL_

#include "hercules.h"
/*
#include <errno.h>
#include <stdlib.h>
#include <string.h>
*/
#include "getopt.h"
/*
#include <stdarg.h>
#include <stdio.h>
*/

#define REPLACE_GETOPT

#define _DIAGASSERT(x) do {} while (0)

#ifdef REPLACE_GETOPT
#ifdef __weak_alias
__weak_alias(getopt,_getopt)
#endif
DLL_EXPORT int     opterr = 1;             /* if error message should be printed */
DLL_EXPORT int     optind = 1;             /* index into parent argv vector */
DLL_EXPORT int     optopt = '?';           /* character checked for validity */
DLL_EXPORT int     optreset;               /* reset getopt */
DLL_EXPORT char    *optarg;                /* argument associated with option */
#endif

#ifdef __weak_alias
__weak_alias(getopt_long,_getopt_long)
#endif

#ifndef __CYGWIN__
#define __progname __argv[0]
#else
extern char __declspec(dllimport) *__progname;
#endif

#define IGNORE_FIRST    (*options == '-' || *options == '+')
#define PRINT_ERROR     ((opterr) && ((*options != ':') \
                                      || (IGNORE_FIRST && options[1] != ':')))

/* This differs from the cygwin implementation, which effectively defaults
   to PC, but is consistent with the NetBSD implementation and doc's. */
#ifndef IS_POSIXLY_CORRECT
#define IS_POSIXLY_CORRECT (get_symbol("POSIXLY_CORRECT") != NULL && *get_symbol("POSIXLY_CORRECT"))
#endif

#define PERMUTE         (!IS_POSIXLY_CORRECT && !IGNORE_FIRST)
/* XXX: GNU ignores PC if *options == '-' */
#define IN_ORDER        (!IS_POSIXLY_CORRECT && *options == '-')

/* return values */
#define BADCH   (int)'?'
#define BADARG          ((IGNORE_FIRST && options[1] == ':') \
                         || (*options == ':') ? (int)':' : (int)'?')
#define INORDER (int)1

static char EMSG[1];

static int getopt_internal (int, char * const *, const char *);
static int gcd (int, int);
static void permute_args (int, int, int, char * const *);

static char *place = EMSG; /* option letter processing */

/* XXX: set optreset to 1 rather than these two */
static int nonopt_start = -1; /* first non option argument (for permute) */
static int nonopt_end = -1;   /* first option after non options (for permute) */

/* Error messages */
static const char recargchar[] = "option requires an argument -- %c";
static const char recargstring[] = "option requires an argument -- %s";
static const char ambig[] = "ambiguous option -- %.*s";
static const char noarg[] = "option doesn't take an argument -- %.*s";
static const char illoptchar[] = "unknown option -- %c";
static const char illoptstring[] = "unknown option -- %s";

static void
_vwarnx(const char *fmt, va_list ap)
{
  (void)fprintf(stderr, "%s: ", __progname);
  if (fmt != NULL)
    (void)vfprintf(stderr, fmt, ap);
  (void)fprintf(stderr, "\n");
}

static void
warnx(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  _vwarnx(fmt, ap);
  va_end(ap);
}

/*
 * Compute the greatest common divisor of a and b.
 */
static int
gcd(a, b)
        int a;
        int b;
{
        int c;

        c = a % b;
        while (c != 0) {
                a = b;
                b = c;
                c = a % b;
        }

        return b;
}

/*
 * Exchange the block from nonopt_start to nonopt_end with the block
 * from nonopt_end to opt_end (keeping the same order of arguments
 * in each block).
 */
static void
permute_args(panonopt_start, panonopt_end, opt_end, nargv)
        int panonopt_start;
        int panonopt_end;
        int opt_end;
        char * const *nargv;
{
        int cstart, cyclelen, i, j, ncycle, nnonopts, nopts, pos;
        char *swap;

        _DIAGASSERT(nargv != NULL);

        /*
         * compute lengths of blocks and number and size of cycles
         */
        nnonopts = panonopt_end - panonopt_start;
        nopts = opt_end - panonopt_end;
        ncycle = gcd(nnonopts, nopts);
        cyclelen = (opt_end - panonopt_start) / ncycle;

        for (i = 0; i < ncycle; i++) {
                cstart = panonopt_end+i;
                pos = cstart;
                for (j = 0; j < cyclelen; j++) {
                        if (pos >= panonopt_end)
                                pos -= nnonopts;
                        else
                                pos += nopts;
                        swap = nargv[pos];
                        /* LINTED const cast */
                        ((char **) nargv)[pos] = nargv[cstart];
                        /* LINTED const cast */
                        ((char **)nargv)[cstart] = swap;
                }
        }
}

/*
 * getopt_internal --
 *      Parse argc/argv argument vector.  Called by user level routines.
 *  Returns -2 if -- is found (can be long option or end of options marker).
 */
static int
getopt_internal(nargc, nargv, options)
        int nargc;
        char * const *nargv;
        const char *options;
{
        char *oli;                              /* option letter list index */
        int optchar;

        _DIAGASSERT(nargv != NULL);
        _DIAGASSERT(options != NULL);

        optarg = NULL;

        /*
         * XXX Some programs (like rsyncd) expect to be able to
         * XXX re-initialize optind to 0 and have getopt_long(3)
         * XXX properly function again.  Work around this braindamage.
         */
        if (optind == 0)
                optind = 1;

        if (optreset)
                nonopt_start = nonopt_end = -1;
start:
        if (optreset || !*place) {              /* update scanning pointer */
                optreset = 0;
                if (optind >= nargc) {          /* end of argument vector */
                        place = EMSG;
                        if (nonopt_end != -1) {
                                /* do permutation, if we have to */
                                permute_args(nonopt_start, nonopt_end,
                                    optind, nargv);
                                optind -= nonopt_end - nonopt_start;
                        }
                        else if (nonopt_start != -1) {
                                /*
                                 * If we skipped non-options, set optind
                                 * to the first of them.
                                 */
                                optind = nonopt_start;
                        }
                        nonopt_start = nonopt_end = -1;
                        return -1;
                }
                if ((*(place = nargv[optind]) != '-')
                    || (place[1] == '\0')) {    /* found non-option */
                        place = EMSG;
                        if (IN_ORDER) {
                                /*
                                 * GNU extension:
                                 * return non-option as argument to option 1
                                 */
                                optarg = nargv[optind++];
                                return INORDER;
                        }
                        if (!PERMUTE) {
                                /*
                                 * if no permutation wanted, stop parsing
                                 * at first non-option
                                 */
                                return -1;
                        }
                        /* do permutation */
                        if (nonopt_start == -1)
                                nonopt_start = optind;
                        else if (nonopt_end != -1) {
                                permute_args(nonopt_start, nonopt_end,
                                    optind, nargv);
                                nonopt_start = optind -
                                    (nonopt_end - nonopt_start);
                                nonopt_end = -1;
                        }
                        optind++;
                        /* process next argument */
                        goto start;
                }
                if (nonopt_start != -1 && nonopt_end == -1)
                        nonopt_end = optind;
                if (place[1] && *++place == '-') {      /* found "--" */
                        place++;
                        return -2;
                }
        }
        if ((optchar = (int)*place++) == (int)':' ||
            (oli = strchr(options + (IGNORE_FIRST ? 1 : 0), optchar)) == NULL) {
                /* option letter unknown or ':' */
                if (!*place)
                        ++optind;
                if (PRINT_ERROR)
                        warnx(illoptchar, optchar);
                optopt = optchar;
                return BADCH;
        }
        if (optchar == 'W' && oli[1] == ';') {          /* -W long-option */
                /* XXX: what if no long options provided (called by getopt)? */
                if (*place)
                        return -2;

                if (++optind >= nargc) {        /* no arg */
                        place = EMSG;
                        if (PRINT_ERROR)
                                warnx(recargchar, optchar);
                        optopt = optchar;
                        return BADARG;
                } else                          /* white space */
                        place = nargv[optind];
                /*
                 * Handle -W arg the same as --arg (which causes getopt to
                 * stop parsing).
                 */
                return -2;
        }
        if (*++oli != ':') {                    /* doesn't take argument */
                if (!*place)
                        ++optind;
        } else {                                /* takes (optional) argument */
                optarg = NULL;
                if (*place)                     /* no white space */
                        optarg = place;
                /* XXX: disable test for :: if PC? (GNU doesn't) */
                else if (oli[1] != ':') {       /* arg not optional */
                        if (++optind >= nargc) {        /* no arg */
                                place = EMSG;
                                if (PRINT_ERROR)
                                        warnx(recargchar, optchar);
                                optopt = optchar;
                                return BADARG;
                        } else
                                optarg = nargv[optind];
                }
                place = EMSG;
                ++optind;
        }
        /* dump back option letter */
        return optchar;
}

#ifdef REPLACE_GETOPT
/*
 * getopt --
 *      Parse argc/argv argument vector.
 *
 * [eventually this will replace the real getopt]
 */
DLL_EXPORT int
getopt(nargc, nargv, options)
        int nargc;
        char * const *nargv;
        const char *options;
{
        int retval;

        _DIAGASSERT(nargv != NULL);
        _DIAGASSERT(options != NULL);

        if ((retval = getopt_internal(nargc, nargv, options)) == -2) {
                ++optind;
                /*
                 * We found an option (--), so if we skipped non-options,
                 * we have to permute.
                 */
                if (nonopt_end != -1) {
                        permute_args(nonopt_start, nonopt_end, optind,
                                       nargv);
                        optind -= nonopt_end - nonopt_start;
                }
                nonopt_start = nonopt_end = -1;
                retval = -1;
        }
        return retval;
}
#endif

/*
 * getopt_long --
 *      Parse argc/argv argument vector.
 */
DLL_EXPORT int
getopt_long(nargc, nargv, options, long_options, idx)
        int nargc;
        char * const *nargv;
        const char *options;
        const struct option *long_options;
        int *idx;
{
        int retval;

        _DIAGASSERT(nargv != NULL);
        _DIAGASSERT(options != NULL);
        _DIAGASSERT(long_options != NULL);
        /* idx may be NULL */

        if ((retval = getopt_internal(nargc, nargv, options)) == -2) {
                char *current_argv, *has_equal;
                size_t current_argv_len;
                int i, match;

                current_argv = place;
                match = -1;

                optind++;
                place = EMSG;

                if (*current_argv == '\0') {            /* found "--" */
                        /*
                         * We found an option (--), so if we skipped
                         * non-options, we have to permute.
                         */
                        if (nonopt_end != -1) {
                                permute_args(nonopt_start, nonopt_end,
                                    optind, nargv);
                                optind -= nonopt_end - nonopt_start;
                        }
                        nonopt_start = nonopt_end = -1;
                        return -1;
                }
                if ((has_equal = strchr(current_argv, '=')) != NULL) {
                        /* argument found (--option=arg) */
                        current_argv_len = has_equal - current_argv;
                        has_equal++;
                } else
                        current_argv_len = strlen(current_argv);

                for (i = 0; long_options[i].name; i++) {
                        /* find matching long option */
                        if (strncmp(current_argv, long_options[i].name,
                            current_argv_len))
                                continue;

                        if (strlen(long_options[i].name) ==
                            (unsigned)current_argv_len) {
                                /* exact match */
                                match = i;
                                break;
                        }
                        if (match == -1)                /* partial match */
                                match = i;
                        else {
                                /* ambiguous abbreviation */
                                if (PRINT_ERROR)
                                        warnx(ambig, (int)current_argv_len,
                                             current_argv);
                                optopt = 0;
                                return BADCH;
                        }
                }
                if (match != -1) {                      /* option found */
                        if (long_options[match].has_arg == no_argument
                            && has_equal) {
                                if (PRINT_ERROR)
                                        warnx(noarg, (int)current_argv_len,
                                             current_argv);
                                /*
                                 * XXX: GNU sets optopt to val regardless of
                                 * flag
                                 */
                                if (long_options[match].flag == NULL)
                                        optopt = long_options[match].val;
                                else
                                        optopt = 0;
                                return BADARG;
                        }
                        if (long_options[match].has_arg == required_argument ||
                            long_options[match].has_arg == optional_argument) {
                                if (has_equal)
                                        optarg = has_equal;
                                else if (long_options[match].has_arg ==
                                    required_argument) {
                                        /*
                                         * optional argument doesn't use
                                         * next nargv
                                         */
                                        optarg = nargv[optind++];
                                }
                        }
                        if ((long_options[match].has_arg == required_argument)
                            && (optarg == NULL)) {
                                /*
                                 * Missing argument; leading ':'
                                 * indicates no error should be generated
                                 */
                                if (PRINT_ERROR)
                                        warnx(recargstring, current_argv);
                                /*
                                 * XXX: GNU sets optopt to val regardless
                                 * of flag
                                 */
                                if (long_options[match].flag == NULL)
                                        optopt = long_options[match].val;
                                else
                                        optopt = 0;
                                --optind;
                                return BADARG;
                        }
                } else {                        /* unknown option */
                        if (PRINT_ERROR)
                                warnx(illoptstring, current_argv);
                        optopt = 0;
                        return BADCH;
                }
                if (long_options[match].flag) {
                        *long_options[match].flag = long_options[match].val;
                        retval = 0;
                } else
                        retval = long_options[match].val;
                if (idx)
                        *idx = match;
        }
        return retval;
}
