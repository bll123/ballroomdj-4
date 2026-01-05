@echo off
@rem BDJ4 removal
@rem Copyright 2023-2026 Brad Lanam Pleasant Hill CA
set ver=5

set bdj4startup="%USERPROFILE%\Start Menu\Programs\Startup\bdj4.bat"
set bdj4conf="%USERPROFILE%\AppData\Roaming\BDJ4"
set bdj4instloc=%bdj4conf%\installdir.txt
set bdj4cache="%TEMP%\BDJ4"

setlocal enabledelayedexpansion

@rem gdbus must be stopped
@rem may or may not exist as a process
taskkill /f /im gdbus.exe 2> NUL

@rem these variables must be set for the if block to process properly
set bdj4dir="notset"
set bdj4sc="notset"

if exist %bdj4instloc% (
  set /p x=<%bdj4instloc%
  set bdj4dir="!x!"
  set bdj4dir=!bdj4dir:/=\!
  @rem version 4.4.4 used USERPROFILE
  @rem version 4.17.8 starts using USERPROFILE
  set bdj4dir=!bdj4dir:%%USERPROFILE%%=%USERPROFILE%!
  set bdj4dir=!bdj4dir:%%USERNAME%%=%USERNAME%!
  if exist !bdj4dir! (
    rmdir /s/q !bdj4dir!
  )
  set bdj4sc="%USERPROFILE%\Desktop\BDJ4.lnk"
  if exist !bdj4sc! (
    del /q !bdj4sc!
  )
)

set bdj4dir="%USERPROFILE%\BDJ4"
set bdj4sc="%USERPROFILE%\Desktop\BDJ4.lnk"

if exist "%bdj4dir%" (
  if exist "%bdj4dir%"\data (
    if exist "%bdj4dir%"\bin\bdj4.exe (
      rmdir /s/q "%bdj4dir%"
      if exist "%bdj4sc%" (
        del /q "%bdj4sc%"
      )
    )
  )
)

set bdj4dir="notset"
set bdj4sc="notset"
set bdj4altinstloc="notset"

for %%v in (01 02 03 04 05 06 07 08 09) do (
  set bdj4altinstloc=%bdj4conf%\altinstdir%%v.txt

  if exist !bdj4altinstloc! (
    set /p x=<!bdj4altinstloc!
    set bdj4dir="!x!"
    set bdj4dir=!bdj4dir:/=\!
    @rem version 4.4.4 used USERPROFILE
    @rem version 4.17.8 starts using USERPROFILE
    set bdj4dir=!bdj4dir:%%USERPROFILE%%=%USERPROFILE%!
    set bdj4dir=!bdj4dir:%%USERNAME%%=%USERNAME%!
    if exist !bdj4dir! (
      rmdir /s/q !bdj4dir!
    )
    for /f "delims=" %%i in ("!bdj4dir!") do (
      set nm=%%~ni
    )
    set bdj4sc="%USERPROFILE%\Desktop\!nm!.lnk"
    if exist !bdj4sc! (
      del /q !bdj4sc!
    )
  )
)

if exist %bdj4startup% (
  del /q %bdj4startup%
)

if exist %bdj4conf% (
  rmdir /s/q %bdj4conf%
)
if exist %bdj4cache% (
  rmdir /s/q %bdj4cache%
)

exit /b
