@echo off

: https://stackoverflow.com/questions/346107/creating-a-shortcut-for-a-exe-from-a-batch-file
: thanks to VVS

set tgtpath=%1

: cd "%USERPROFILE%\Desktop"
set VBS=makeshortcut.vbs
set SRC_LNK="%USERPROFILE%\Desktop\BDJ4.lnk"
set APPLOC="%tgtpath%\bin\bdj4.exe"
set WRKDIR="%tgtpath%"

cscript //Nologo %VBS% %SRC_LNK% %APPLOC% %WRKDIR%
