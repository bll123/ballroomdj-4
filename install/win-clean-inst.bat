@echo off
: Copyright 2021-2023 Brad Lanam Pleasant Hill CA

: windows install dir clean

: arg0 : script
: arg1 : unpack dir (temporary installation folder)

set script=%0
set spath=%~dp0
: remove trailing backslash
set spath=%spath:~0,-1%
for %%i in (%spath%) do (
  set sbase=%%~ni
)
set unpackdir=%1
set unpackbase=%~n1

if not exist "%unpackdir%" (
  : folder does not exist
  goto scriptexit
)

if not %unpackbase% == bdj4-install (
  : not a bdj4-install folder
  goto scriptexit
)

: gdbus must be stopped
: may or may not exist as a process
taskkill /f /im gdbus.exe 2> NUL

if %sbase% == install (
  : copy the script elsewhere and re-start it
  copy /y %script% "%TEMP%\bdj4-clean-inst.bat"
  start "Installation Cleanup" /b cmd /c %TEMP%\bdj4-clean-inst.bat %unpackdir%
  goto scriptexit
)

: this section should only be executed by the copied script

echo -- Cleaning up installation.  Please wait...
: sleep for 3 seconds, wait for the installer and gdbus to exit
ping 127.0.0.1 -n 3 > nul
rmdir /s/q %unpackdir%
: tricky thing to delete the copied script
(goto) 2>nul & del "%0"

:scriptexit
exit
