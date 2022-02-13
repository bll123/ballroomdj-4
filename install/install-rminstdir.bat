@echo off

set d=%1

taskkill /f /im gdbus.exe

if not "%d%" == "" (
  if exist "%d%" (
    rmdir /s/q "%d%"
  )
)

exit 0
