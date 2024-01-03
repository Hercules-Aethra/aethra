/* SCSIUTIL.H   (C) Copyright "Fish" (David B. Trout), 2016          */
/*              Hercules SCSI tape utility functions                 */
/*                                                                   */
/*  SPDX-FileCopyrightText: Copyright "Fish" (David B. Trout)        */
/*  SPDX-License-Identifier: QPL-1.0                                 */

#ifndef _SCSIUTIL_H_
#define _SCSIUTIL_H_

#if defined( OPTION_SCSI_TAPE )

extern char* gstat2str( U32 mt_gstat, char* buffer, size_t bufsz );

#endif // defined( OPTION_SCSI_TAPE )

#endif // _SCSIUTIL_H_
