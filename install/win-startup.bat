@echo off
@rem BDJ4 clean
@rem Copyright 2023 Brad Lanam Pleasant Hill CA

set bdj4conf=%USERPROFILE%\AppData\Roaming\BDJ4
set bdj4clean=%bdj4conf%\bdj4clean.bat
if exist "%bdj4clean%" (
  call "%bdj4clean%"
)
exit /b
