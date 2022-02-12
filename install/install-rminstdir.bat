@echo off

set d=%1

if not "%d%" == "" (
  if exist "%d%" (
    rmdir /s/q "%d%"
  )
)

exit 0
