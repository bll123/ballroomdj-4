@echo off

cd bdj4-install
set checksumfn=%1
if "%checksumfn%" == "" (
  set checksumfn=install/checksum.txt
)

setlocal enabledelayedexpansion

rem *** this will not handle spaces in filenames correctly. ***
rem g - checksum, h - filename
rem the '*' binary indicator in front of the filename has already
rem been removed.
set exitcode=OK
for /f "tokens=1-2 delims= " %%g in (%checksumfn%) do (
  for /f "tokens=1 delims= " %%i in ('.\plocal\bin\openssl.exe sha512 -r %%h') do (
    if not %%g == %%i (
      set exitcode=NG
      goto exitloop
    )
  )
)

:exitloop
echo %exitcode%
