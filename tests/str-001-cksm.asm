  TITLE 'str-001-cksm.asm: Test CKSM Instruction'
*
***********************************************************************
*
*Testcase str-001-cksm
*  Test cases for variations on the CKSM (Checksum) instruction.
*
***********************************************************************
*
*                        str-001-cksm.asm
*
* Created and placed into the public domain 2018-12-30 by Bob Polmanter
* Remove runtest *Compare dependency on 2022-03-08 by Fish
*
* The CKSM instruction is tested against the definition in the
* z/Architecture Principles of Operation, SA22-7832.
*
* Test data is assembled into this program, and some test data is
* generated by this program. The program itself verifies the resulting
* status of registers and condition codes via simple CLC comparison.
*
*
*
* Tests performed with CKSM (Checksum):
*
* 1.  Checksum; 2nd operand does not cross page boundary,
*               length is a multiple of 4.
* 2.  Checksum; 2nd operand does not cross page boundary,
*               length is NOT a multiple of 4.
* 3.  Checksum; 2nd operand fully crosses page boundary,
*               length is a multiple of 4.
* 4.  Checksum; 2nd operand fully crosses page boundary,
*               length is NOT a multiple of 4.
* 5.  Checksum; 2nd operand ends on page boundary,
*               length is a multiple of 4.
* 6.  Checksum; 2nd operand ends on page boundary,
*               length is NOT a multiple of 4.
* 7.  Checksum; 2nd operand ends on page boundary+2,
*               length is a multiple of 4.
* 8.  Checksum; 2nd operand ends on page boundary+2,
*               length is NOT a multiple of 4.
* 9.  Checksum; 2nd operand crosses multiple pages
*
* NOTE: the variation between lengths with a multiple of 4 and
*       not a multiple of 4 is to test the conceptual adding of
*       zero values to complete the checksum with 4-byte elements
*       as described in the Principles of Operation.
*
***********************************************************************
*
*
CKSM001  START 0
STRTLABL EQU   *
R0       EQU   0
R1       EQU   1
R2       EQU   2
R3       EQU   3
R4       EQU   4
R5       EQU   5
R6       EQU   6
R7       EQU   7
R8       EQU   8
R9       EQU   9
R10      EQU   10
R11      EQU   11
R12      EQU   12
R13      EQU   13
R14      EQU   14
R15      EQU   15
*
*
         USING *,R15
*
* Selected z/Arch low core layout
*
         ORG   STRTLABL+X'1A0'     z/Arch Restart PSW
         DC    X'0000000180000000',A(0,START)
*
         ORG   STRTLABL+X'1D0'     z/Arch Program check new PSW
         DC    X'0002000000000000',XL4'00',X'0000DEAD' Abnormal end
*
         EJECT
***********************************************************************
*
*  Main program.
*
         ORG   STRTLABL+X'200'
START    DS    0H
*
*
**********
* TEST 1 * No page boundary crossed; len=multiple of 4
**********
*
         LA    R2,TDATA1               -> buffer to checksum
         LA    R3,16                   Length
         BAS   R9,CHECKSUM             compute
         STM   R1,R3,RESULT1           Save test result regs
*
**********
* TEST 2 * No page boundary crossed; len=NOT multiple of 4
**********
*
         LA    R2,TDATA1               -> buffer to checksum
         LA    R3,13                   Length
         BAS   R9,CHECKSUM             compute
         STM   R1,R3,RESULT2           Save test result regs
*
**********
* TEST 3 * Page boundary crossed; len=multiple of 4
**********
*
         L     R2,BOUND1               -> where to place the buffer
         MVC   0(16,R2),TDATA1         Move data across boundary
         LA    R3,16                   Length
         BAS   R9,CHECKSUM             compute
         STM   R1,R3,RESULT3           Save test result regs
*
**********
* TEST 4 * Page boundary crossed; len=NOT multiple of 4
**********
*
         L     R2,BOUND1               -> where to place the buffer
         MVC   0(16,R2),TDATA1         Move data across boundary
         LA    R3,13                   Length
         BAS   R9,CHECKSUM             compute
         STM   R1,R3,RESULT4           Save test result regs
*
**********
* TEST 5 * Operand ends on a page boundary; len=multiple of 4
**********
*
         L     R2,BOUND2               -> where to place the buffer
         MVC   0(16,R2),TDATA1         Place the data
         LA    R3,16                   Length
         BAS   R9,CHECKSUM             compute
         STM   R1,R3,RESULT5           Save test result regs
*
*
**********
* TEST 6 * Operand ends on a page boundary; len=NOT multiple of 4
**********
*
         L     R2,BOUND3               -> where to place the buffer
         MVC   0(16,R2),TDATA1         Place the data
         LA    R3,13                   Length
         BAS   R9,CHECKSUM             compute
         STM   R1,R3,RESULT6           Save test result regs
*
**********
* TEST 7 * Operand ends on a page boundary+2; len=multiple of 4
**********
*
         L     R2,BOUND4               -> where to place the buffer
         MVC   0(16,R2),TDATA1         Place the data
         LA    R3,16                   Length
         BAS   R9,CHECKSUM             compute
         STM   R1,R3,RESULT7           Save test result regs
*
*
**********
* TEST 8 * Operand ends on a page boundary+2; len=NOT multiple of 4
**********
*
         L     R2,BOUND5               -> where to place the buffer
         MVC   0(16,R2),TDATA1         Place the data
         LA    R3,13                   Length
         BAS   R9,CHECKSUM             compute
         STM   R1,R3,RESULT8           Save test result regs
*
**********
* TEST 9 * Operand crosses multiple pages
**********
*
         LM    R2,R5,AREA              Load multi-page area ptrs
         MVCL  R2,R4                   Pad the buffer area
*
         L     R2,AREA                 -> multipage buffer
         L     R3,TEST9LEN             Length to checksum
         BAS   R9,CHECKSUM             compute
         STM   R1,R3,RESULT9           Save test result regs
*
**       Verify correct results...
*
         CLC   GRESULT1,RESULT1
         BNE   BAD99
         CLC   GRESULT2,RESULT2
         BNE   BAD99
         CLC   GRESULT3,RESULT3
         BNE   BAD99
         CLC   GRESULT4,RESULT4
         BNE   BAD99
         CLC   GRESULT5,RESULT5
         BNE   BAD99
         CLC   GRESULT6,RESULT6
         BNE   BAD99
         CLC   GRESULT7,RESULT7
         BNE   BAD99
         CLC   GRESULT8,RESULT8
         BNE   BAD99
         CLC   GRESULT9,RESULT9
         BNE   BAD99
*
         LPSWE GOODPSW                 load SUCCESS disabled wait PSW
*
*-- CKSM routine used by tests
*
CHECKSUM EQU   *
         SR    R1,R1                   Init checksum accum
*
INVOKE   EQU   *
         CKSM  R1,R2                   Compute checksum
         BC    4,BADCC                 CC=1 SHOULD NEVER HAPPEN
         BC    2,BADCC                 CC=2 SHOULD NEVER HAPPEN
         BC    1,INVOKE                Restart the checksum
         BR    R9                      Return if CC=0
*
BADCC    LPSWE BADCCPSW                Stop on invalid CKSUM CC
BAD99    LPSWE BAD99PSW                Stop on invalid CKSUM result
*
         DS    0D             Ensure correct alignment for psw
GOODPSW  DC    X'0002000000000000',A(0,0) Normal end - disabled wait
BADCCPSW DC    X'0002000000000000',XL4'00',X'000BADCC' Abnormal end
BAD99PSW DC    X'0002000000000000',XL4'00',X'00099BAD' Abnormal end
*
*
GRESULT1 DC    XL12'99DE22650000071000000000'
GRESULT2 DC    XL12'990033660000070D00000000'
GRESULT3 DC    XL12'99DE22650000300B00000000'
GRESULT4 DC    XL12'990033660000300800000000'
GRESULT5 DC    XL12'99DE22650000300000000000'
GRESULT6 DC    XL12'990033660000300000000000'
GRESULT7 DC    XL12'99DE22650000300200000000'
GRESULT8 DC    XL12'990033660000300200000000'
GRESULT9 DC    XL12'E1E1E1E10000BFF800000000'
*
*
*
         ORG   STRTLABL+X'700'
*
TDATA1   DC    X'00112233'             Buffer data to be checksummed
         DC    X'44556677'
         DC    X'8899AABB'
         DC    X'CCDDEEFF'
*
BOUND1   DC    X'00002FFB'             -> data crosses boundary
BOUND2   DC    X'00002FF0'             -> data ends at boundary
BOUND3   DC    X'00002FF3'             -> data ends at boundary
BOUND4   DC    X'00002FF2'             -> data ends at boundary+2
BOUND5   DC    X'00002FF5'             -> data ends at boundary+2
*
AREA     DC    X'00004000'             multi=page area
AREALEN  DC    A(4096*16)              Size of multi=page area
ZERO     DC    A(0)
PAD      DC    X'87000000'             MVCL pad char
TEST9LEN DC    F'32760'                Length to checksum test 9
*
*
*
*  Locations for results
*
* Result fields are kept on 16-byte boundaries to more easily
* track their assembled offsets for use in the .tst script.
*
*                              offset
         ORG   STRTLABL+X'800'   8xx
RESULT1  DS    4F                 00   Register results test 1
RESULT2  DS    4F                 10   Register results test 2
RESULT3  DS    4F                 20   Register results test 3
RESULT4  DS    4F                 30   Register results test 4
RESULT5  DS    4F                 40   Register results test 5
RESULT6  DS    4F                 50   Register results test 6
RESULT7  DS    4F                 60   Register results test 7
RESULT8  DS    4F                 70   Register results test 8
RESULT9  DS    4F                 80   Register results test 9
*
         END
