@echo off
: BDJ4 clean

set bdj4conf="%USERPROFILE%\AppData\Roaming\BDJ4"
set bdj4clean=%bdj4conf%\bdj4clean.bat
if exist %bdj4clean% (
  call %bdj4clean%
)
exit
