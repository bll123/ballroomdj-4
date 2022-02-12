@echo off

dir=%1

if exist "%dir%" (
  rmdir /s/q "%dir%"
)

exit 0
