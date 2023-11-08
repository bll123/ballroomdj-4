@echo off
: BDJ4 removal
: Copyright 2023 Brad Lanam Pleasant Hill CA
set ver=2

set bdj4dir="%USERPROFILE%\BDJ4"
set bdj4startup="%USERPROFILE%\Start Menu\Programs\Startup\bdj4.bat"
set bdj4conf="%USERPROFILE%\AppData\Roaming\BDJ4"
set bdj4instloc="%bdj4conf%\installdir.txt"
set bdj4cache="%TEMP%\BDJ4"

setlocal enabledelayedexpansion

if exist %bdj4instloc% (
  set /p x=<%bdj4instloc%
  set bdj4dir=!x!
  set "bdj4dir=!bdj4dir:/=\!"
)

: gdbus must be stopped
: may or may not exist as a process
taskkill /f /im gdbus.exe 2> NUL

if exist "%bdj4startup%" (
  del %bdj4startup%
)
if exist "%bdj4conf%" (
  rmdir /s/q %bdj4conf%
)
if exist "%bdj4cache%" (
  rmdir /s/q %bdj4conf%
)
if exist "%bdj4dir%" (
  rmdir /s/q %bdj4dir%
)

exit
