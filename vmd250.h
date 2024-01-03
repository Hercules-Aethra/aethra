/* VMD250.H     z/VM 5.4 DIAGNOSE code X'250'                        */
/*                                                                   */
/*  SPDX-FileCopyrightText: Copyright Harold Grovesteen              */
/*  SPDX-License-Identifier: QPL-1.0                                 */

#if !defined(__VMD250_H__)
#define __VMD250_H__

/*-------------------------------------------------------------------*/
/*  DIAGNOSE X'250' Block I/O - Device Environment                   */
/*-------------------------------------------------------------------*/
struct VMBIOENV {
        DEVBLK *dev;     /* Device block pointer of device           */
        S32   blksiz;    /* Block size being used by the guest       */
        S64   offset;    /* Guest provided offset                    */
        S64   begblk;    /* BIO established beginning block number   */
        S64   endblk;    /* BIO established ending block number      */
        int   isCKD;     /* Count-Key-Data device                    */
        int   isRO;      /* Device is read-only                      */
        int   blkphys;   /* Block to physical relationship           */
                         /* For FBA: physical sectors per block      */
                         /* For CKD: physical blocks per track       */
        BYTE  sense[32]; /* Save area for any pending sense data     */
};

#endif /* !defined(__VMD250_H__) */
