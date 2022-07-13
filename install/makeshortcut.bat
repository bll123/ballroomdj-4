@echo off

set sc="%1"
set target="%2"
set tgtpath="%3"

set VBS=makeshortcut.vbs
set SRC_LNK=%sc%
set APPLOC=%tgtpath%\%target%
set WRKDIR=%tgtpath%

echo cscript //Nologo %VBS% %SRC_LNK% %APPLOC% %WRKDIR%
cscript //Nologo %VBS% %SRC_LNK% %APPLOC% %WRKDIR%

c:\Windows\System32\ie4uinit.exe -ClearIconCache
c:\Windows\System32\ie4uinit.exe -show

exit
