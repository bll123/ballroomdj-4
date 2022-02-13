@echo off

set d=%1

: don't care whether this exists or not.
taskkill /f /im gdbus.exe 2> NUL

if not "%d%" == "" (
  if exist "%d%" (
    rmdir /s/q "%d%"
  )
)

exit 0
