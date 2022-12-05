@echo off
: Copyright 2021-2023 Brad Lanam Pleasant Hill CA

set sc=%1
set target=%2
set tgtpath=%3
set profnum=%4

set VBS=winshortcut.vbs
set SRC_LNK=%sc%
set APPLOC=%target%
set WRKDIR=%tgtpath%
set PROFNUM=%profnum%

cscript //Nologo %VBS% %SRC_LNK% %APPLOC% %WRKDIR% %PROFNUM%

c:\Windows\System32\ie4uinit.exe -ClearIconCache
c:\Windows\System32\ie4uinit.exe -show

exit
