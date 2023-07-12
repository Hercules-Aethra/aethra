/* CM3705II.C (c) 2023 Copyright Edwin Freekenhorst and Henk Stegeman

  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
  ROBERT M SUPNIK BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
  ------------------------------------------------------------------------------

  Hercules channel extender for the 3705 communications controller simulator

  This version used the COMM3705.C coding from Max H. Parke as the starting point.

  The purpose of this module is to extend the 37xx Channel Adapter module of
  the 37xx simulator, into hercules.
  The core function is to maintain a dual TCP/IP connection with the 37xx
  simulator, emulating a bus and tag channel connection.

  This coding is such, that it is (should be) independent from the Hercules
  version (as well as Linux as Windows).
*/

#include "hstdinc.h"
#include "hercules.h"
#include "devtype.h"
#include "opcode.h"
#include "parser.h"

#if defined(WIN32)
    typedef int bool;
    #define true 1
    #define false 0
#else
    #include "stdbool.h"
#endif

#if defined(WIN32) && defined(OPTION_DYNAMIC_LOAD) && !defined(HDL_USE_LIBTOOL) && !defined(_MSVC_)
    SYSBLK *psysblk;
    #define sysblk (*psysblk)
#endif

#if !defined(min)
    #define  min(a,b) (((a) <= (b)) ? (a) : (b))
#endif

#define BUFPD 0x1C

static BYTE commadpt_immed_command[256]=
{ 0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

/*-------------------------------------------------------------------*/
/* PARSER TABLES                                                     */
/*-------------------------------------------------------------------*/
static PARSER ptab[] = {
     {"port",    "%s"},
     {"adaptip", "%s"},
     {"debug",   "%s"},
     {"tracesna", "%s"},
     {NULL, NULL}
};


enum {
    COMMADPT_KW_PORT = 1,
    COMMADPT_KW_ADAPTIP,
    COMMADPT_KW_DEBUG,
    COMMADPT_KW_TRACESNA
} cm3705ii_kw;

struct COMMADPT {
    DEVBLK *dev;                /* the devblk to which this CA is attched   */
    TID  tthread;               /* ATTN thread                              */
    BYTE *unitstat;             /* pointer to channel unitstat              */
    in_addr_t adaptip;          /* 3705 CA listening address                */
    U16  port;                  /* 3705 CA listening port                   */
    BYTE carnstat;              /* Channel Adaptor return status            */
    BYTE carnstat2;             /* Channel Adaptor return status (tag)      */
    BYTE ccwactive;             /* indicate an active CCW                   */
    BYTE cswpending;            /* indicate a CSW is pending                */
    int  busfd;                 /* Communication socket for ccw/data        */
    int  tagfd;                 /* Communication socket for attn            */
    COND ipc;                   /* I/O <-> thread IPC condition EVB         */
    COND ipc_halt;              /* I/O <-> thread IPC HALT special EVB      */
    LOCK lock;                  /* COMMADPT lock                            */
    int  pipe[2];               /* pipe used for I/O to thread signaling    */
    U16  devnum;                /* devnum copy from DEVBLK                  */
    struct sockaddr_in servaddr;

    U32 have_cthread:1;         /* the comm thread is running               */
    U32 attn_run;               /* the ATTN thread is running               */
    U32 attn_halt;              /* signal halt to the ATTN thread           */
    U32 debug;                  /* 1 = write general  debug messages        */
    U32 tracesna;               /* 1 = write sna request codes/Commands     */

    int read_ccw_count;
    int write_ccw_count;
    int unack_attn_count;

    BYTE inpbuf[65536];
    int  inpbufl;

    void *freeq;
    void *sendq;
    BYTE *poolarea;

};

struct CSW {
    uint8_t  key;
    uint8_t  data_address[3];
    uint8_t  unit_conditions;
    uint8_t  channel_conditions;
    uint16_t count;
} csw;

/*-------------------------------------------------------------------*/
/* Internal macro definitions                                        */
/*-------------------------------------------------------------------*/
#define BUFLEN_3270     65536           /* 3270 Send/Receive buffer  */
#define BUFLEN_1052     150             /* 1052 Send/Receive buffer  */

#undef  FIX_QWS_BUG_FOR_MCS_CONSOLES

// ************************************************************
// This subroutine waits 1 usec
// ************************************************************
void wait1us() {
    usleep(1);
    return;
}

static int write_adpt(BYTE* bufferp, int len, COMMADPT* ca);

// ************************************************************
// Function to check if socket is (still) connected
// ************************************************************
static bool IsSocketConnected(int sockfd, DEVBLK* dev)
{
    int rc;
    struct sockaddr_in ccu_addr;
    socklen_t addrlen = sizeof(ccu_addr);
    if (sockfd < 1) {
        WRMSG(HHC05101, "E", LCSS_DEVNUM);
        return false;
    }
    rc = getpeername(sockfd, (struct sockaddr*)&ccu_addr, &addrlen);
    if (rc != 0) {
        WRMSG(HHC05102, "E" , LCSS_DEVNUM, sockfd, rc, strerror(errno));
        return false;
    }
    return true;
}

// ********************************************************************
// Function to enable TCP socket and connect to Remote channel adapter
// ********************************************************************
static int connect_adpt(COMMADPT *ca) {
    int rc;
    char cua[2];
#ifdef WIN32
    unsigned long bmode = 0;
#endif
    // Bus socket creation
    ca->busfd = socket(AF_INET, SOCK_STREAM, 0);
    if (ca->busfd <= 0 ) {
        WRMSG(HHC05103, "E", LCSS_DEVNUM_DEV(ca->dev));
        return(-1);
    }

    // ATTN (Tag) socket creation
    ca->tagfd = socket(AF_INET, SOCK_STREAM, 0);
    if (ca->tagfd <= 0) {
        WRMSG(HHC05153, "E", LCSS_DEVNUM_DEV(ca->dev));
        return(-1);
    }

#ifdef WIN32
    rc = ioctlsocket(ca->busfd, FIONBIO, &bmode);
    if (rc != 0) {
        WRMSG(HHC05104, "E", LCSS_DEVNUM_DEV(ca->dev));
        return(-1);
    }
    rc = ioctlsocket(ca->tagfd, FIONBIO, &bmode);
    if (rc != 0) {
        WRMSG(HHC05154, "E", LCSS_DEVNUM_DEV(ca->dev));
        return(-1);
    }
#endif

    // Remove if below works on Linux: bzero(&ca->servaddr, sizeof(ca->servaddr));
    memset(&ca->servaddr, 0x00, sizeof(ca->servaddr));

    // Assign IP addr and PORT number
    ca->servaddr.sin_family = AF_INET;
    ca->servaddr.sin_addr.s_addr = ca->adaptip;
    ca->servaddr.sin_port = htons(ca->port);


    // Connect to the bus socket
    WRMSG(HHC05105, "I",  LCSS_DEVNUM_DEV(ca->dev), ca->busfd);
    while (connect(ca->busfd, (struct sockaddr*)&ca->servaddr, sizeof(ca->servaddr)) != 0) {
        if (ca->attn_halt)
            return(-1);
        sleep(1);
    }

    // Connect to the tag (ATTN) socket
    WRMSG(HHC05155, "I", LCSS_DEVNUM_DEV(ca->dev), ca->tagfd);
    while (connect(ca->tagfd, (struct sockaddr*)&ca->servaddr, sizeof(ca->servaddr)) != 0) {
        if (ca->attn_halt)
            return(-1);
        sleep(1);
    }
    WRMSG(HHC05150, "I", LCSS_DEVNUM_DEV(ca->dev), ca->tagfd);

    cua[0] = (ca->devnum & 0x0000FF00) >> 8;
    cua[1] = (ca->devnum & 0x000000FF);
    if (ca->busfd  > 0) {
        rc = send (ca->busfd, cua, 2, 0);
        if (rc == 2)
            WRMSG(HHC05100, "I", LCSS_DEVNUM_DEV(ca->dev), ca->busfd);
    } else {
        WRMSG(HHC05106, "E",  LCSS_DEVNUM_DEV(ca->dev), strerror(HSO_errno));
        return(-1);
    }
    return(0);
}

// ********************************************************************
// Function to close the bus and tag TCP sockets
// ********************************************************************
static void close_adpt(COMMADPT *ca) {

    // Bus socket close
    if (ca->busfd > 0) {
        shutdown(ca->busfd, SHUT_RDWR);
        ca->busfd = -1;
        WRMSG(HHC05107, "I",  LCSS_DEVNUM_DEV(ca->dev));
    }
    // Tag socket close
    if (ca->tagfd > 0) {
        shutdown(ca->tagfd, SHUT_RDWR);
        ca->tagfd = -1;
        WRMSG(HHC05157, "I",  LCSS_DEVNUM_DEV(ca->dev));
    }
    return;
}

/*-------------------------------------------------------------------*/
/* Subroutine to send buffer to remote channel adapter               */
/*-------------------------------------------------------------------*/
static int
write_adpt(BYTE *bufferp, int len, COMMADPT *ca) {
    int rc;                              /* Return code               */

    if (ca->busfd > 0) {
        rc = send (ca->busfd, bufferp, len, 0);
        if (ca->debug)
            WRMSG(HHC05108, "I", LCSS_DEVNUM_DEV(ca->dev), ca->busfd, rc);
  }
    else
        rc = -1;
    if (rc < 0) {
        WRMSG(HHC05109, "E",  LCSS_DEVNUM_DEV(ca->dev), ca->busfd,
            strerror(HSO_errno));
        return -1;
    }
    return 0;
} /* End function write_adpt */

char EBCDIC2ASCII (char s) {
    static char etoa[] =
        "................................"
        "................................"
        " ...........<(+|&.........!$*); "  // first char here is real space !
        "-/.........,%_>?.........`:#@'=\""
        " abcdefghi.......jklmnopqr......"
        "  stuvwxyz......................"
        " ABCDEFGHI.......JKLMNOPQR......"
        "  STUVWXYZ......0123456789......";
    s = etoa[(unsigned char)s];
    return s;
}

static void logdump(char *txt, DEVBLK *dev, BYTE *bfr, size_t sz) {
    size_t i;
    char workbuf[50];
    char *workbuf_ptr = workbuf;
    if (!dev->ccwtrace) {
        return;
    }
    WRMSG(HHC05110, "D", LCSS_DEVNUM, txt);
    WRMSG(HHC05111, "D", LCSS_DEVNUM, txt, (long int)sz, (long int)sz);

    for (i = 0; i < sz; i++) {
        if (i%4 == 0) {
            *workbuf_ptr++ = ' ';
        }
        // We use snprintf directly here instead of MSGBUF because the latter uses
        // sizeof() on the first argument; normally, that works, but not if you're
        // passing an address directly
        snprintf(workbuf_ptr, 2, "%2.2X", bfr[i]);
        workbuf_ptr += 2;
        if (i%16 == 0) {
            WRMSG(HHC05112, "D", LCSS_DEVNUM, txt, (long int)i, workbuf);
            workbuf_ptr = workbuf;
        }
    }
    workbuf_ptr = workbuf;
        for (i = 0; i < sz; i++) {
            *workbuf_ptr++ = EBCDIC2ASCII(bfr[i]);
            if (i%16 == 0) {
                *workbuf_ptr = '\0';
                WRMSG(HHC05113, "D", LCSS_DEVNUM, txt, (long int)i, workbuf);
                workbuf_ptr = workbuf;
            }
        }
        if (i%16 != 0) {
            *workbuf_ptr = '\0';
            WRMSG(HHC05113, "D", LCSS_DEVNUM, txt, (long int)i, workbuf);
        }
}

// ********************************************************************
// * Function to "decode" the SNA traffic (tracesna=yes in hercules
// * config file).
// * It is not entirely watertight, but sufficent for debugging purposes.
// ********************************************************************
static void tracesna(DEVBLK *dev, BYTE *bfr, BYTE code) {
    int ru_len, fid4;
    uint8_t ru_cat;                                 // fmd = b00, nc = b01, dfc = b10, sc = b11
    char *ru_type = "";
    char *ru_code = "";

    if (!dev->commadpt->tracesna) {
        return;
    }
    if (bfr[0] == 0x41) return;                     // Skip if FID4 with priority network flow (FID1 will be empty)
    fid4 = 0;                                       // If no preceeding FID4, the offset to FID1 is 0 (earlier VTAM releases)
    ru_type = "Data";
    ru_code = "N/A";
    if (bfr[0] & 0x40) fid4 = 16;                   // If preceeded with a FID 4, then the FID1 is offset with 16 bytes
    else if (code == 0x02) fid4 = 28;               // (MVS3.8 - VTAM L2) A read is preceeded with a FID1.
        if ((bfr[fid4+0] & 0x0C) == 0x0C) {          // If Only Segment
            ru_len = (bfr[fid4+8] << 8) + bfr[fid4+9];
            if ((ru_len > 3) && ((bfr[fid4+10] & 0x03) == 0x03)) {   // RU length must be > 3 and single chain only
                ru_cat = (bfr[fid4+10] & 0x60) >> 5;
                ru_code = "UNKNOWN";
                switch (ru_cat) {
                    case 0x00: // FM Data
                        if (!memcmp(&bfr[fid4+13], "\x01\x02\x01", 3)) ru_code = "CONTACT";
                        if (!memcmp(&bfr[fid4+13], "\x01\x02\x02", 3)) ru_code = "DISCONTACT";
                        if (!memcmp(&bfr[fid4+13], "\x01\x02\x03", 3)) ru_code = "IPLINIT";
                        if (!memcmp(&bfr[fid4+13], "\x01\x02\x04", 3)) ru_code = "IPLTEXT";
                        if (!memcmp(&bfr[fid4+13], "\x01\x02\x05", 3)) ru_code = "IPLFINAL";
                        if (!memcmp(&bfr[fid4+13], "\x01\x02\x06", 3)) ru_code = "DUMPOINIT";
                        if (!memcmp(&bfr[fid4+13], "\x01\x02\x07", 3)) ru_code = "DUMPTEXT";
                        if (!memcmp(&bfr[fid4+13], "\x01\x02\x08", 3)) ru_code = "DUMPFINAL";
                        if (!memcmp(&bfr[fid4+13], "\x01\x02\x09", 3)) ru_code = "RPO";
                        if (!memcmp(&bfr[fid4+13], "\x01\x02\x0A", 3)) ru_code = "ACTLINK";
                        if (!memcmp(&bfr[fid4+13], "\x01\x02\x0B", 3)) ru_code = "DACTLINK";
                        if (!memcmp(&bfr[fid4+13], "\x01\x02\x0E", 3)) ru_code = "CONNOUT";
                        if (!memcmp(&bfr[fid4+13], "\x01\x02\x0F", 3)) ru_code = "ABCONN";
                        if (!memcmp(&bfr[fid4+13], "\x01\x02\x11", 3)) ru_code = "SETCV NS(c)";
                        if (!memcmp(&bfr[fid4+13], "\x01\x02\x14", 3)) ru_code = "ESLOW";
                        if (!memcmp(&bfr[fid4+13], "\x01\x02\x15", 3)) ru_code = "EXSLOW";
                        if (!memcmp(&bfr[fid4+13], "\x01\x02\x16", 3)) ru_code = "ACTCONNIN";
                        if (!memcmp(&bfr[fid4+13], "\x01\x02\x17", 3)) ru_code = "DACTCONNIN";
                        if (!memcmp(&bfr[fid4+13], "\x01\x02\x18", 3)) ru_code = "ABCONNOUT";
                        if (!memcmp(&bfr[fid4+13], "\x01\x02\x19", 3)) ru_code = "ANA";
                        if (!memcmp(&bfr[fid4+13], "\x01\x02\x1A", 3)) ru_code = "FNA";
                        if (!memcmp(&bfr[fid4+13], "\x01\x02\x1B", 3)) ru_code = "REQDISCONT";
                        if (!memcmp(&bfr[fid4+13], "\x01\x02\x80", 3)) ru_code = "CONTACTED";
                        if (!memcmp(&bfr[fid4+13], "\x01\x02\x81", 3)) ru_code = "INOP";
                        if (!memcmp(&bfr[fid4+13], "\x01\x02\x84", 3)) ru_code = "REQCONT";
                        if (!memcmp(&bfr[fid4+13], "\x01\x02\x85", 3)) ru_code = "NS-LSA";
                        if (!memcmp(&bfr[fid4+13], "\x01\x03\x01", 3)) ru_code = "EXECTEST";
                        if (!memcmp(&bfr[fid4+13], "\x01\x03\x02", 3)) ru_code = "ACTTRACE";
                        if (!memcmp(&bfr[fid4+13], "\x01\x03\x03", 3)) ru_code = "DACTTRACE";
                        if (!memcmp(&bfr[fid4+13], "\x01\x03\x11", 3)) ru_code = "SETCV NS(ma)";
                        if (!memcmp(&bfr[fid4+13], "\x41\x02\x10", 3)) ru_code = "RNAA";
                        if (!memcmp(&bfr[fid4+13], "\x81\x06\x88", 3)) ru_code = "SESSEND";
                    break;
                    case 0x01: // Network Control
                        if (bfr[fid4+13] == 0x02) ru_code = "NC-IPL-FINAL";
                        if (bfr[fid4+13] == 0x03) ru_code = "NC-IPL-INIT";
                        if (bfr[fid4+13] == 0x04) ru_code = "NC-IPL-TEXT";
                        if (bfr[fid4+13] == 0x05) ru_code = "LSA";
                        if (bfr[fid4+13] == 0x06) ru_code = "NC-ER-INOP";
                        if (bfr[fid4+13] == 0x09) ru_code = "NC-ER-TEST";
                        if (bfr[fid4+13] == 0x0A) ru_code = "NC-ER-TEST-REPLY";
                        if (bfr[fid4+13] == 0x0B) ru_code = "NC-ER-ACT";
                        if (bfr[fid4+13] == 0x0C) ru_code = "NC-ER-ACT-REPLY";
                        if (bfr[fid4+13] == 0x0D) ru_code = "NC-ACTVR";
                        if (bfr[fid4+13] == 0x0E) ru_code = "NC-DACTVR";
                        if (bfr[fid4+13] == 0x0F) ru_code = "NC-ER-OP";
                        if (bfr[fid4+13] == 0x46) ru_code = "NC-IPL-ABORT";
                    break;
                    case 0x02: // Data Flow Control
                        if (bfr[fid4+13] == 0x04) ru_code = "LUSTAT";
                        if (bfr[fid4+13] == 0x05) ru_code = "RTR";
                        if (bfr[fid4+13] == 0x07) ru_code = "ANSC";
                        if (bfr[fid4+13] == 0x70) ru_code = "BIS";
                        if (bfr[fid4+13] == 0x71) ru_code = "SBI";
                        if (bfr[fid4+13] == 0x80) ru_code = "QEC";
                        if (bfr[fid4+13] == 0x81) ru_code = "QC";
                        if (bfr[fid4+13] == 0x82) ru_code = "RELQ";
                        if (bfr[fid4+13] == 0x83) ru_code = "CANCEL";
                        if (bfr[fid4+13] == 0xC0) ru_code = "SHUTD";
                        if (bfr[fid4+13] == 0xC1) ru_code = "SHUTC";
                        if (bfr[fid4+13] == 0xC2) ru_code = "RSHUTD";
                        if (bfr[fid4+13] == 0xC8) ru_code = "BID";
                        if (bfr[fid4+13] == 0xC9) ru_code = "SIGNAL";
                    break;
                    case 0x03: // Session Control
                        if (bfr[fid4+13] == 0x0D) ru_code = "ACTLU";
                        if (bfr[fid4+13] == 0x0E) ru_code = "DACTLU";
                        if (bfr[fid4+13] == 0x11) ru_code = "ACTPU";
                        if (bfr[fid4+13] == 0x12) ru_code = "DACTPU";
                        if (bfr[fid4+13] == 0x14) ru_code = "ACTCDRM";
                        if (bfr[fid4+13] == 0x15) ru_code = "DACTCDRM";
                        if (bfr[fid4+13] == 0x31) ru_code = "BIND";
                        if (bfr[fid4+13] == 0x32) ru_code = "UNBIND";
                        if (bfr[fid4+13] == 0xA0) ru_code = "SDT";
                        if (bfr[fid4+13] == 0xA1) ru_code = "CLEAR";
                        if (bfr[fid4+13] == 0xC0) ru_code = "CRV";
                    break;
                } // End switch ru_cat
                if (bfr[fid4+10] & 0x04) ru_code = "Sense Data";
                if (bfr[fid4+10] & 0x80)
                    ru_type = "Response ";
                else ru_type = "Request";
            }  // End if ru_len
        }  // End if Only_segment
        WRMSG(HHC03711, "D", LCSS_DEVNUM, ru_type,  ru_code);
}

static void put_bufpool(void ** anchor, BYTE * ele) {
    void ** elep = anchor;
    for (;;) {
        if (!*elep) break;
            elep = *elep;
    }
    *elep = ele;
    *(void**)ele = 0;
}

static void init_bufpool(COMMADPT *ca) {
    BYTE * areap;
    int i1;
    int numbufs = 64;
    int bufsize = 256 + 16 + 4;
    ca->poolarea = (BYTE*)calloc (numbufs, bufsize);
    if (!ca->poolarea) {
        return;
    }
    areap = ca->poolarea;
    for (i1 = 0; i1 < numbufs; i1++) {
        put_bufpool(&ca->freeq, areap);
        areap += (bufsize);
    }
}

static void free_bufpool(COMMADPT *ca) {
    ca->sendq = 0;
    ca->freeq = 0;
    if (ca->poolarea) {
        free(ca->poolarea);
        ca->poolarea = 0;
    }
}

/*-------------------------------------------------------------------*/
/* Free all private structures and buffers                           */
/*-------------------------------------------------------------------*/
static void commadpt_clean_device(DEVBLK *dev) {
    if (dev->commadpt != NULL) {
        dev->commadpt->attn_halt = 1;
        close_adpt(dev->commadpt);
        while (dev->commadpt->attn_run == 1)
            usleep(20);

        free(dev->commadpt);
        dev->commadpt = NULL;
        WRMSG(HHC03709, "I", LCSS_DEVNUM);
    }
    else {
        WRMSG(HHC03710, "I", LCSS_DEVNUM);
    }
    return;
}

/*-------------------------------------------------------------------*/
/* Allocate initial private structures                               */
/*-------------------------------------------------------------------*/
static int commadpt_alloc_device(DEVBLK *dev) {
    dev->commadpt = malloc(sizeof(COMMADPT));
    if (dev->commadpt == NULL) {
        WRMSG(HHC03708, "E", LCSS_DEVNUM);
        return -1;
    }
    memset(dev->commadpt, 0, sizeof(COMMADPT));
    dev->commadpt->dev=dev;
    return 0;
}

/*-------------------------------------------------------------------*/
/* commadpt_getport : returns a port number or -1                    */
/*-------------------------------------------------------------------*/
static int commadpt_getport(char *txt) {
    int pno;
    struct servent *se;

    pno = atoi(txt);
    if (pno == 0) {
        se = getservbyname(txt, "tcp");
            if (se == NULL) {
                return -1;
            }
        pno = se->s_port;
    }
    return(pno);
}

/*-------------------------------------------------------------------*/
/* commadpt_getaddr : set an in_addr_t if ok, else return -1         */
/*-------------------------------------------------------------------*/
static int commadpt_getaddr(in_addr_t *ia, char *txt) {
    struct hostent *he;
    he = gethostbyname(txt);
    if (he == NULL) {
        return(-1);
    }
    memcpy(ia, he->h_addr_list[0], 4);
    return(0);
}

/*-------------------------------------------------------------------*/
/* CSW_adpt: Monitors sockets and handles Channel Status Words       */
/*            from the remote channel adapter                        */
/*-------------------------------------------------------------------*/
static void *CSW_adpt(void *vca) {
    COMMADPT *ca;

    int rc, rc_attn;           /* return code from various rtns       */
    int delay;

    ca = (COMMADPT*)vca;
    ca->dev->commadpt->attn_run = 1;

    while ((!IsSocketConnected(ca->busfd, ca->dev)) && !ca->dev->commadpt->attn_halt) {
        if (ca->dev->commadpt->debug)
            WRMSG(HHC05164, "D", LCSS_DEVNUM_DEV(ca->dev));

        rc = connect_adpt(ca);

        if ((rc == 0) && !ca->dev->commadpt->attn_halt) {
            if (ca->dev->commadpt->debug)
                WRMSG(HHC05165, "D", LCSS_DEVNUM_DEV(ca->dev), ca->port, ca->busfd, ca->tagfd);
        } else {
            if (!ca->dev->commadpt->attn_halt)
                *ca->unitstat |= CSW_UC;    // Signal unit check
        }

        while ((IsSocketConnected(ca->tagfd, ca->dev)) && !ca->dev->commadpt->attn_halt) {
            // Wait for Channel Adapter status byte
            rc = recv(ca->tagfd, &ca->carnstat, 1, 0);

            if (rc > 0) {
                if (ca->dev->commadpt->debug)
                    WRMSG(HHC05166, "D", LCSS_DEVNUM_DEV(ca->dev), ca->carnstat);

                if (ca->dev->commadpt->debug)
                    WRMSG(HHC05167, "D", LCSS_DEVNUM_DEV(ca->dev), ca->dev->busy, ca->ccwactive);

                /************************************************************************************************************/
                /* The code below injects the CSW into the channel subsys.                                                  */
                /************************************************************************************************************/
                delay = 0;

                while (ca->dev->busy || IOPENDING(ca->dev)) {
                    wait1us();
                    delay++;
                }
                obtain_lock(&ca->dev->commadpt->lock);
                rc_attn = device_attention(ca->dev, CSW_ATTN);  // Inject Attention
                if (ca->dev->commadpt->debug)
                    WRMSG(HHC05168, "D", LCSS_DEVNUM_DEV(ca->dev), delay, rc_attn);
                release_lock(&ca->dev->commadpt->lock);
                if (ca->dev->commadpt->debug)
                    WRMSG(HHC05169, "D", LCSS_DEVNUM_DEV(ca->dev));

            } else {  // If Error during receive of CA status, close both sockets and try to re-open
                if (ca->dev->commadpt->debug)
                    WRMSG(HHC05170, "D", LCSS_DEVNUM_DEV(ca->dev));
                close_adpt(ca);
                if (!ca->dev->commadpt->attn_halt) {
                    ca->dev->scsw.unitstat |= CSW_UC;       // Signal unit check
                    sleep(1);      /* Wait 1 secs, then try to re-connect */
                }
            }  // End if rc > 0

            if (ca->dev->commadpt->attn_halt == 1) {
                WRMSG(HHC05171, "I", LCSS_DEVNUM_DEV(ca->dev));
                close_adpt(ca);
            }
        }  // End while ca->tagfd
    }  // End while ca->busfd

    ca->dev->commadpt->attn_run = 0;
    return NULL;
}

/*-------------------------------------------------------------------*/
/* Halt currently executing I/O command                              */
/*-------------------------------------------------------------------*/
static void commadpt_halt(DEVBLK *dev) {
    if (!dev->busy) {
        return;
    }
    return;
}

/* The following two MSG functions ensure only one hardcoded         */
/* instance exist for the same numbered msg that is issued on        */
/* multiple situations                                               */
static void msg005i(DEVBLK *dev, char *kv) {
    WRMSG(HHC03705, "I", LCSS_DEVNUM, kv);
}
static void msg006e(DEVBLK *dev, char *kw, char *kv) {
    WRMSG(HHC03706, "E", LCSS_DEVNUM, kw, kv);
}

/*-------------------------------------------------------------------*/
/* Device Initialisation                                             */
/*-------------------------------------------------------------------*/
static int commadpt_init_handler (DEVBLK *dev, int argc, char *argv[]) {
    char thread_name[32];
    int i;
    int rc;
    int pc; /* Parse code */
    int errcnt;
    union {
        int num;
        char text[MAX_PARSER_STRLEN+1];
    } res;

    /* convert the device type string to hex */
    dev->devtype = (dev->typname[0] & 0x0F) << 12;
    dev->devtype = dev->devtype | ((dev->typname[1] & 0x0F) << 8);
    dev->devtype = dev->devtype | ((dev->typname[2] & 0x0F) << 4);
    dev->devtype = dev->devtype | (dev->typname[3] & 0x0F);
    WRMSG(HHC03700, "I", LCSS_DEVNUM, dev->devtype);

    /* For re-initialisation, close the existing file, if any */
    if (dev->fd >= 0)
        (dev->hnd->close)(dev);

    dev->excps = 0;
    WRMSG(HHC03701, "I", LCSS_DEVNUM);

    /* Legacy sense-id not supported */
    dev->numdevid = 0;

    if (dev->commadpt != NULL) {
        commadpt_clean_device(dev);
    }
    rc = commadpt_alloc_device(dev);

    if (rc < 0) {
        WRMSG(HHC03702, "E", LCSS_DEVNUM);
        return(-1);
    }
    errcnt = 0;

    /* Initialise ports & hosts */
    dev->commadpt->tracesna=0;
    dev->commadpt->debug=0;
    dev->commadpt->busfd = -1;
    dev->commadpt->tagfd = -1;
    dev->commadpt->port = 0;

    for (i = 0; i < argc; i++) {
        pc = parser(ptab, argv[i], &res);

        if (pc < 0) {
            WRMSG(HHC03703, "E", LCSS_DEVNUM, argv[i]);
            errcnt++;
            continue;
        }
        if (pc == 0) {
            WRMSG(HHC03704, "E", LCSS_DEVNUM, argv[i]);
            errcnt++;
            continue;
        }

        switch(pc) {
            case COMMADPT_KW_TRACESNA:
                if (res.text[0] == 'y' || res.text[0] == 'Y')  {
                    dev->commadpt->tracesna = 1;
                    msg005i(dev, argv[i]);
                }
                else
                    dev->commadpt->tracesna = 0;
                break;

            case COMMADPT_KW_DEBUG:
                if (res.text[0] == 'y' || res.text[0] == 'Y') {
                    dev->commadpt->debug = 1;
                    msg005i(dev, argv[i]);
                }
                else
                    dev->commadpt->debug = 0;
                break;

            case COMMADPT_KW_PORT:
                rc = commadpt_getport(res.text);
                if (rc < 0) {
                    errcnt++;
                    msg006e(dev, "PORT", res.text);
                    break;
                }
                dev->commadpt->port = rc;
                break;

            case COMMADPT_KW_ADAPTIP:
                if (strcmp(res.text, "*") == 0) {
                    dev->commadpt->adaptip = INADDR_ANY;
                    break;
                }
                rc = commadpt_getaddr(&dev->commadpt->adaptip, res.text);
                if (rc != 0) {
                    msg006e(dev, "ADAPTIP", res.text);
                    errcnt++;
                }
                break;

            default:
                break;

        }  // End of switch(pc)
    }  // End of for stmt.

    if (errcnt > 0) {
        WRMSG(HHC03707, "E", LCSS_DEVNUM);
        return -1;
    }

    dev->bufsize = 256;
    dev->numsense = 2;
    memset(dev->sense, 0, sizeof(dev->sense));

    init_bufpool(dev->commadpt);

    dev->commadpt->devnum = dev->devnum;

    /* Initialize the CA lock */
    initialize_lock(&dev->commadpt->lock);

    /* Initialise thread->I/O & halt initiation EVB */
    initialize_condition(&dev->commadpt->ipc);
    initialize_condition(&dev->commadpt->ipc_halt);

    /* Allocate I/O -> Thread signaling pipe */
    //VERIFY(!create_pipe(dev->commadpt->pipe));

    /* Obtain the CA lock */
    obtain_lock(&dev->commadpt->lock);

    /* Start the thread to establish client connections    */
    /* with the remote channel adapter                     */
    strcpy(thread_name, "CSW_adpt");

    /* Set thread-name for debugging purposes */
    if (dev->commadpt->debug)
        WRMSG(HHC03712, "D", LCSS_DEVNUM, thread_name);

    rc = create_thread(&dev->commadpt->tthread, &sysblk.detattr, CSW_adpt, dev->commadpt, thread_name);
    if (rc) {
        WRMSG(HHC03713, "E", LCSS_DEVNUM, strerror(rc));
        release_lock(&dev->commadpt->lock);
        return -1;
    }
    /* Indicate things need to be closed */
    dev->fd = 3705;     // too high an FD to be normally used
    /* Release the CA lock */
    release_lock(&dev->commadpt->lock);
    /* Indicate succesfull completion */
    return 0;
}

/*-------------------------------------------------------------------*/
/* Query the device definition                                       */
/*-------------------------------------------------------------------*/
static void commadpt_query_device (DEVBLK *dev, char **class,
                     int buflen, char *buffer) {
    *class = "LINE";
    snprintf(buffer, buflen, "Read count=%d, Write count=%d\n",
              dev->commadpt->read_ccw_count, dev->commadpt->write_ccw_count);
}

/*-------------------------------------------------------------------*/
/* Close the device                                                  */
/* Invoked by HERCULES shutdown & DEVINIT processing                 */
/*-------------------------------------------------------------------*/
static int commadpt_close_device (DEVBLK *dev) {
    if (dev->ccwtrace) {
        WRMSG(HHC03714, "D", LCSS_DEVNUM);
    }

    /* Obtain the CA lock */
    obtain_lock(&dev->commadpt->lock);

    /* Terminate current I/O thread if necessary */
    if (dev->busy) {
        commadpt_halt(dev);
    }
    free_bufpool(dev->commadpt);

    /* Release the CA lock   */
    release_lock(&dev->commadpt->lock);

    /* Free all work storage */
    commadpt_clean_device(dev);

    /* Indicate to hercules the device is no longer opened */
    dev->fd = -1;

    if (dev->ccwtrace) {
        WRMSG(HHC03715, "D", LCSS_DEVNUM);
    }
    return 0;
}

/*-------------------------------------------------------------------*/
/* xmit CCW to 3705/3720/3725 Channel Adapter for processing         */
/* Inject returned device status back into the channel               */
/*-------------------------------------------------------------------*/
static void commadpt_execute_ccw (DEVBLK *dev, BYTE code, BYTE flags,
          BYTE chained, uint32_t count, BYTE prevcode, int ccwseq,
          BYTE *iobuf, BYTE *more, BYTE *unitstat, uint32_t *residual) {

    U32      num;    /* Work : Actual CCW transfer count              */
    char     buf[80];
    uint32_t rc;

    struct CCW {
        BYTE code;
        BYTE data_address[3];
        BYTE flags;
        BYTE chain;
        BYTE count_ho;
        BYTE count_lo;
    } ccw;

    UNREFERENCED(prevcode);
    UNREFERENCED(ccwseq);

    ccw.code = code;
    ccw.flags = flags;
    ccw.chain = chained;
    ccw.count_ho = (count & 0x0000FF00) >> 8 ;
    ccw.count_lo = (count & 0x000000FF);

    if (dev->commadpt->debug)
        WRMSG(HHC03716, "D", LCSS_DEVNUM, code, count);

    *residual = 0;

    if (IsSocketConnected(dev->commadpt->busfd, dev)) {
        /* Obtain the COMMADPT lock */
        obtain_lock(&dev->commadpt->lock);
        dev->commadpt->unitstat = unitstat;

        if (dev->commadpt->debug)
            WRMSG(HHC03717, "D", LCSS_DEVNUM, code);

        rc = write_adpt((void*)&ccw, sizeof(ccw), dev->commadpt);
        /* Wait for ACK */

        switch (code) {
            /*----------------------------------------------------------*/
            /* TEST I/O                                                 */
            /*----------------------------------------------------------*/
            case 0x00:
                *residual = 0;
                /* Get Channel Return Status */
                rc = recv(dev->commadpt->busfd, &dev->commadpt->carnstat, 1, 0);
                break;

            /*----------------------------------------------------------*/
            /* I/O NO-OP & Discontact                                   */
            /*----------------------------------------------------------*/
            case 0x03:     // No_Op
                *residual = 0;
                /* Get Channel Return Status */
                rc = recv(dev->commadpt->busfd, &dev->commadpt->carnstat, 1, 0);
                *unitstat = dev->commadpt->carnstat;
                break;

            /*----------------------------------------------------------*/
            /* BASIC SENSE                                              */
            /*----------------------------------------------------------*/
            case 0x04:
                /* Wait for the sense data */
                rc = recv(dev->commadpt->busfd, dev->sense, 256, 0);
                dev->numsense = rc;
                dev->commadpt->unack_attn_count = 0;
                *more = 0;
                if (dev->commadpt->debug)
                    WRMSG(HHC03718, "D", LCSS_DEVNUM, dev->sense[0]);
                /* Copy device sense bytes to channel I/O buffer */
                memcpy (iobuf, dev->sense, rc);
                *residual = 0;

                /* Get Channel Return Status */
                rc = recv(dev->commadpt->busfd, &dev->commadpt->carnstat, 1, 0);
                *unitstat = dev->commadpt->carnstat;
                break;

            /*----------------------------------------------------------*/
            /* WRITE IPL                                                */
            /*----------------------------------------------------------*/
            case 0x05:
                dev->commadpt->unack_attn_count = 0;
                logdump("WRITE", dev, iobuf, count);
                tracesna(dev, iobuf, code);
                rc = write_adpt(iobuf, count, dev->commadpt);
                if (rc == 0) {
                    *residual = 0;
                    *more =  0;
                    /* Get Channel Return Status */
                    rc = recv(dev->commadpt->busfd, &dev->commadpt->carnstat, 1, 0);
                    *unitstat = dev->commadpt->carnstat;
                } else {
                    *unitstat |= CSW_ATTN;
                    *unitstat |= CSW_UX | CSW_ATTN;
                }
                break;

            /*----------------------------------------------------------*/
            /* SENSE-ID  (not for 3705)                                 */
            /*----------------------------------------------------------*/
            case 0xE4:
                if ((dev->devtype & 0x00FF) == 0x05) {   /* If this is a 3705....  */
                    ;  // NOP
                } else {
                    /* Wait for the sense-id data */
                    rc = recv(dev->commadpt->busfd, dev->commadpt->inpbuf, count, 0);
                    /* Calculate residual byte count */
                    num = (count < rc) ? count:rc;
                    *residual = count - num;
                    *more = count < rc ? 1 : 0;
                    /* Copy device sense-id bytes to channel I/O buffer */
                    memcpy (iobuf, dev->commadpt->inpbuf, num);
                    /* Initialize the device identifier bytes */
                    memcpy (dev->devid, dev->commadpt->inpbuf, num);
                    logdump("SENSE-ID", dev, iobuf, num);
                    dev->numdevid = 4;
                }  // End if dev->devtype

                /* Get Channel Return Status */
                rc = recv(dev->commadpt->busfd, &dev->commadpt->carnstat, 1, 0);
                *unitstat = dev->commadpt->carnstat;
                break;

            /*----------------------------------------------------------*/
            /* READ CCW                                                 */
            /*----------------------------------------------------------*/
            case 0x02:     /* READ */
                /* Wait for the remote channel adapter data */
                rc = recv(dev->commadpt->busfd, dev->commadpt->inpbuf, count, 0);

                dev->commadpt->read_ccw_count++;
                dev->commadpt->unack_attn_count = 0;
                *more = 0;
                /* Copy data to I/O buffer */
                memcpy (iobuf, dev->commadpt->inpbuf, rc);
                *residual = count - rc;
                logdump("READ", dev, iobuf, rc);
                tracesna(dev, iobuf, code);

                /* Get Channel Return Status */
                rc = recv(dev->commadpt->busfd, &dev->commadpt->carnstat, 1, 0);
                *unitstat = dev->commadpt->carnstat;
                break;

            /*----------------------------------------------------------*/
            /* WRITE CCW                                                */
            /*----------------------------------------------------------*/
            case 0x01:     /* WRITE */
            case 0x09:     /* WRITE BREAK */
                dev->commadpt->write_ccw_count++;
                dev->commadpt->unack_attn_count = 0;
                logdump("WRITE", dev, iobuf, count);
                tracesna(dev, iobuf, code);
                rc = write_adpt(iobuf, count, dev->commadpt);
                if (rc == 0) {
                    *residual = 0;
                    *more = 0;
                    /* Get Channel Return Status */
                    rc = recv(dev->commadpt->busfd, &dev->commadpt->carnstat, 1, 0);
                    *unitstat = dev->commadpt->carnstat;
                } else {
                    *unitstat |= CSW_ATTN;
                    *unitstat |= CSW_UX | CSW_ATTN;
                }
                break;

            /*----------------------------------------------------------*/
            /* Below set of CCW's require initial select only           */
            /*----------------------------------------------------------*/
            case 0x31:    // Write start 0
            case 0x32:    // Read start 0
            case 0x51:    // Write start 1
            case 0x52:    // Read start 1
            case 0x61:    // Write XID
            case 0x62:    // Read XID
            case 0x93:    // Restart Reset
            case 0xA3:    // Discontact
            case 0xC3:    // Contact
                dev->commadpt->unack_attn_count = 0;
                *residual = count;
                *more = 0;
                /* Get Channel Return Status */
                rc = recv(dev->commadpt->busfd, &dev->commadpt->carnstat, 1, 0);
                *unitstat = dev->commadpt->carnstat;
                break;
            /*----------------------------------------------------------*/
            /* Default: CCW not recognized, send unit check             */
            /*----------------------------------------------------------*/
            default:
                *unitstat = CSW_CE + CSW_DE + CSW_UC;
                dev->sense[0] = SENSE_CR;
                break;
        }  // End of switch (code)

        // Remove if below works under Linux: bzero(buf, 80);
        memset(buf, 0x00, 80);
        release_lock(&dev->commadpt->lock);
        //dev->commadpt->ccwactive = 0x00;
        if (dev->commadpt->debug)
            WRMSG(HHC03719, "D", LCSS_DEVNUM);
    } else {

        /*-------------------------------------------------------------*/
        /*  37x5 not online, set command reject sense                  */
        /*-------------------------------------------------------------*/
        /* Set command reject sense byte, and unit check status        */
        *unitstat = CSW_CE + CSW_DE + CSW_UC;
        dev->sense[0] = SENSE_CR;

    }  // End if IsSocketConnected...
}

/*-------------------------------------------------------------------*/
/* DEVICE FUNCTION POINTERS                                          */
/*-------------------------------------------------------------------*/
#if defined(OPTION_DYNAMIC_LOAD)
static
#endif

DEVHND c3705ii_device_hndinfo = {
          &commadpt_init_handler,        /* Device Initialisation      */
          &commadpt_execute_ccw,         /* Device CCW execute         */
          &commadpt_close_device,        /* Device Close               */
          &commadpt_query_device,        /* Device Query               */
          NULL,                          /* Device Extended Query      */
          NULL,                          /* Device Start channel pgm   */
          NULL,                          /* Device End channel pgm     */
          NULL,                          /* Device Resume channel pgm  */
          NULL,                          /* Device Suspend channel pgm */
          &commadpt_halt,                /* Device Halt channel pgm    */
          NULL,                          /* Device Read                */
          NULL,                          /* Device Write               */
          NULL,                          /* Device Query used          */
          NULL,                          /* Device Reserve             */
          NULL,                          /* Device Release             */
          NULL,                          /* Device Attention           */
          commadpt_immed_command,        /* Immediate CCW Codes        */
          NULL,                          /* Signal Adapter Input       */
          NULL,                          /* Signal Adapter Output      */
          NULL,                          /* Signal Adapter Sync        */
          NULL,                          /* Signal Adapter Output Mult */
          NULL,                          /* QDIO subsys desc           */
          NULL,                          /* QDIO set subchan ind       */
          NULL,                          /* Hercules suspend           */
          NULL                           /* Hercules resume            */
};

/* Libtool static name colision resolution                           */
/* note : lt_dlopen will look for symbol & modulename_LTX_symbol     */
#if !defined(HDL_BUILD_SHARED) && defined(HDL_USE_LIBTOOL)
#define hdl_ddev hdt3705ii_LTX_hdl_ddev
#define hdl_depc hdt3705ii_LTX_hdl_depc
#define hdl_reso hdt3705ii_LTX_hdl_reso
#define hdl_init hdt3705ii_LTX_hdl_init
#define hdl_fini hdt3705ii_LTX_hdl_fini
#endif

HDL_DEPENDENCY_SECTION;
{
    // HDL_DEPENDENCY(HERCULES);
    HDL_DEPENDENCY(DEVBLK);
    HDL_DEPENDENCY(SYSBLK);
}
END_DEPENDENCY_SECTION;


#if defined(WIN32) && !defined(HDL_USE_LIBTOOL) && !defined(_MSVC_)
    #undef sysblk
    HDL_RESOLVER_SECTION;
    {
        HDL_RESOLVE_PTRVAR( psysblk, sysblk );
    }
    END_RESOLVER_SECTION;
#endif

HDL_DEVICE_SECTION;
{
    HDL_DEVICE(3705II, c3705ii_device_hndinfo );
    HDL_DEVICE(3720, c3705ii_device_hndinfo );
    HDL_DEVICE(3725, c3705ii_device_hndinfo );
}
END_DEVICE_SECTION;

