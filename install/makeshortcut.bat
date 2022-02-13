@echo off

set tgtpath=%1

set VBS=makeshortcut.vbs
set SRC_LNK="%USERPROFILE%\Desktop\BDJ4.lnk"
set APPLOC=%tgtpath%\bin\bdj4.exe
set WRKDIR=%tgtpath%

cscript //Nologo %VBS% %SRC_LNK% %APPLOC% %WRKDIR%

c:\Windows\System32\ie4uinit.exe -ClearIconCache
c:\Windows\System32\ie4uinit.exe -show

exit
