/* PARSER.H     Hercules Simple parameter parser                     */
/*                                                                   */
/*  SPDX-FileCopyrightText: Copyright Roger Bowler                   */
/*  SPDX-License-Identifier: QPL-1.0                                 */

#if !defined( _PARSER_H_ )
#define _PARSER_H_

#include "hercules.h"

#define  MAX_PARSER_STRLEN           79
#define  _PARSER_STR_TYPE( len )     "%" QSTR( len ) "s"
#define  PARSER_STR_TYPE             _PARSER_STR_TYPE( MAX_PARSER_STRLEN )

typedef struct _parser
{
    char *key;
    char *fmt;
} PARSER;

PAR_DLL_IMPORT int parser( PARSER *, char *, void * );

#endif /* !defined( _PARSER_H_ ) */
