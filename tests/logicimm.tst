sysclear 
archmode z 
*Testcase logicimm assembled 26 Sep 2015 12:41:34 by BLDHTC EXEC 
r    1A0=00000001800000000000000000000200
r    1D0=0002000180000000FFFFFFFFDEADDEAD
r    200=1B00B2B00880D2070888088041000008
r    210=4110000141200800441002464410024A
r    220=44100250441002544410025A4410025E
r    230=4120200189100001
r    238=46000218D40708800890B2B202689400
r    248=2000EB002800015496002008EB002808
r    258=015697002010EB0028100157
r    268=00020001800000000000000000000000
r    800=FFFFFFFFFFFFFFFF
r    808=0000000000000000
r    810=FFFFFFFFFFFFFFFF
r    890=0000000000000800
r   2000=FFFFFFFFFFFFFFFF
r   2008=0000000000000000
r   2010=FFFFFFFFFFFFFFFF
restart 
pause 1 
*Compare
r 800.8
*Want               01020408 10204080
*Compare
r 808.8
*Want               01020408 10204080
*Compare
r 810.8
*Want               FEFDFBF7 EFDFBF7F
*Compare
r 2000.8
*Want               01020408 10204080
*Compare
r 2008.8
*Want               01020408 10204080
*Compare
r 2010.8
*Want               FEFDFBF7 EFDFBF7F
*Compare
r 880.8
*Want               00000000 00000800
* First doubleword of facilities list
r 888.8
*Done