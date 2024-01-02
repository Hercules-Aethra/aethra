/* HISTORY.H     Hercules Command History Processes                  */
/*                                                                   */
/*  SPDX-FileCopyrightText: Copyright Roger Bowler                   */
/*  SPDX-License-Identifier: QPL-1.0                                 */

#ifndef _HISTORY_H_
#define _HISTORY_H_

/*-------------------------------------------------------------------*/
/*                     History constants                             */
/*-------------------------------------------------------------------*/
#define HISTORY_MAX     255         /* Maximum history depth         */
#define CMD_SIZE        256         /* 32767 is way toooooo much!    */

/*-------------------------------------------------------------------*/
/*                   Hisotry public variables                        */
/*-------------------------------------------------------------------*/
extern int    history_requested;
extern char*  historyCmdLine;

/*-------------------------------------------------------------------*/
/*                   Hisotry public function                         */
/*-------------------------------------------------------------------*/
int history_init();
int history_add( char* cmdline );
int history_remove();
int history_next();
int history_prev();
int history_relative_line( int x );
int history_absolute_line( int x );
int history_show();

#endif // _HISTORY_H_
