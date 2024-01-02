/* ARCHLVL.H        Architecture Level structures and functions      */
/*                                                                   */
/*  SPDX-FileCopyrightText: Copyright "Fish" (David B. Trout)        */
/*  SPDX-License-Identifier: QPL-1.0                                 */

#ifndef _ARCHLVL_H_
#define _ARCHLVL_H_

/*-------------------------------------------------------------------*/
/*                        ARCHTAB struct                             */
/*-------------------------------------------------------------------*/

struct ARCHTAB
{
    const char*  name;              /* Architecture name             */
    const int    num;               /* Architecture number           */
    const int    amask;             /* Architecture bit-mask         */
};
typedef struct ARCHTAB  ARCHTAB;

/*-------------------------------------------------------------------*/
/*                        ARCHTAB macro                              */
/*-------------------------------------------------------------------*/

#define AT( _name, _num, _amask )                                   \
{                                                                   \
    (_name),                                                        \
    (_num),                                                         \
    (_amask)                                                        \
},

/*-------------------------------------------------------------------*/
/*                        ARCHTAB functions                          */
/*-------------------------------------------------------------------*/

extern void init_default_archmode();
extern const char* get_arch_name( REGS* regs );
extern const char* get_arch_name_by_arch( int archnum );
extern const ARCHTAB* get_archtab_by_arch( int archnum );
extern const ARCHTAB* get_archtab_by_name( const char* name );

#endif // _ARCHLVL_H_
