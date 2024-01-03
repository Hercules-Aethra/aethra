/*-------------------------------------------------------------------*/
/* X75.H   TCPIP instruction                                         */
/*-------------------------------------------------------------------*/
/*  SPDX-FileCopyrightText: Copyright the following contributors:    */
/*  SPDX-FileContributor:   Jason Paul Winter                        */
/*  SPDX-FileContributor:   Juergen Winkelmann                       */
/*  SPDX-License-Identifier: BSD-2-Clause                            */

extern int lar_tcpip (DW * regs); /* function in tcpip.c             */
extern U_LONG_PTR map32[Ccom];    /* map 64-bit host addresses       */
                                  /* to 32-bit guest registers       */
