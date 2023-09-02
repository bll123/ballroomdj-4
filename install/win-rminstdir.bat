@echo off

set d=%1
set g=%2

: restart this script so that it is completely detached from the
: starting process.  the /b doesn't seem to work...
if "%g%" == "" (
  start "BDJ4 Installation Cleanup" /b .\install\win-rminstdir.bat "%d%" 2
  exit 0
)

: run tasklist through find, as tasklist doesn't set the errorlevel.
echo Cleaning up BDJ4 installation.
:loop
tasklist /m libbdj4.dll | find "libbdj4" 2>NUL
if errorlevel 1 (
  goto done
) else (
  echo Waiting for installer to exit...
  timeout /t 1 2> NUL
)
:done

: don't care whether this exists or not.
taskkill /f /im gdbus.exe 2> NUL

echo Starting clean-up.
if not "%d%" == "" (
  if exist "%d%" (
    rmdir /s/q "%d%"
  )
)
echo Finished.

exit 0
