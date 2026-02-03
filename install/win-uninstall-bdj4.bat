@echo off
@rem BDJ4 removal
@rem Copyright 2023-2026 Brad Lanam Pleasant Hill CA
set ver=7

set winstartup=%USERPROFILE%"\Start Menu\Programs\Startup"
set bdj4oldstartnm=bdj4.bat
set bdj4startupnm=bdj4winstartup.exe
set roaming=%USERPROFILE%\AppData\Roaming
set bdj4conf=BDJ4
set bdj4instloc=installdir.txt
set bdj4cache=BDJ4
set desktop=%USERPROFILE%\OneDrive\Desktop
if exist "%desktop%" (
  @rem do nothing
) else (
  set desktop=%USERPROFILE%\Desktop
)

setlocal enabledelayedexpansion

@rem gdbus must be stopped
@rem may or may not exist as a process
taskkill /f /im gdbus.exe 2> NUL

@rem these variables must be set for the if block to process properly
set bdj4dir=notset
set bdj4sc=notset

if exist "%roaming%\%bdj4conf%" (
  cd %roaming%\%bdj4conf%
  if exist "%bdj4instloc%" (
    set /p x=<%bdj4instloc%
    set bdj4dir=!x!
    set bdj4dir=!bdj4dir:/=\!
    @rem version 4.4.4 used USERPROFILE
    set bdj4dir=!bdj4dir:%%USERPROFILE%%=%USERPROFILE%!
    set bdj4dir=!bdj4dir:%%USERNAME%%=%USERNAME%!
    for /f "delims=" %%i in ("!bdj4dir!") do (
      set nm=%%~ni
      set path=%%~pi
    )
    set bdj4dir=!path!
    set bdj4dir=!bdj4dir:~0,-1!
    cd !bdj4dir!
    if exist "!nm!" (
      echo Removing configured installation at !bdj4dir!\!nm!
      rmdir /s/q !nm!
    )
    set bdj4sc=!nm!.lnk
    cd !desktop!
    if exist "!bdj4sc!" (
      echo Removing configured desktop shortcut
      del /q !bdj4sc!
    )
  )
)

cd %USERPROFILE%
set bdj4dir=BDJ4
if exist "%bdj4dir%" (
  echo Removing BDJ4 installation at %USERPROFILE%\BDJ4
  rmdir /s/q BDJ4
)

cd %USERPROFILE%
set bdj4dir=BDJ4dev
if exist "%bdj4dir%" (
  echo Removing BDJ4dev installation at %USERPROFILE%\BDJ4dev
  rmdir /s/q BDJ4dev
)

set bdj4sc=BDJ4.lnk
cd %desktop%
if exist "%bdj4sc%" (
  echo Removing desktop shortcut
  del /q %bdj4sc%
)
set bdj4sc=BDJ4dev.lnk
cd %desktop%
if exist "%bdj4sc%" (
  echo Removing dev desktop shortcut
  del /q %bdj4sc%
)

set bdj4dir=notset
set bdj4sc=notset
set bdj4altinstloc=notset

for %%v in (01 02 03 04 05 06 07 08 09) do (
  set bdj4altinstloc=%roaming%\%bdj4conf%\altinstdir%%v.txt

  if exist "!bdj4altinstloc!" (
    set /p x=<!bdj4altinstloc!
    set bdj4dir="!x!"
    set bdj4dir=!bdj4dir:/=\!
    @rem version 4.4.4 used USERPROFILE
    set bdj4dir=!bdj4dir:%%USERPROFILE%%=%USERPROFILE%!
    set bdj4dir=!bdj4dir:%%USERNAME%%=%USERNAME%!
    for /f "delims=" %%i in ("!bdj4dir!") do (
      set nm=%%~ni
      set path=%%~pi
    )
    set bdj4dir=!path!
    set bdj4dir=!bdj4dir:~0,-1!
    cd !bdj4dir!
    if exist "!nm!" (
      echo Removing Alternate installation at !bdj4dir!\!nm!
      rmdir /s/q !nm!
    )
    set bdj4sc=!nm!.lnk
    cd !desktop!
    if exist "!bdj4sc!" (
      echo Removing shortcut !desktop!\!bdj4sc!
      del /q !bdj4sc!
    )
  )
)

cd %winstartup%
if exist "%bdj4oldstartnm%" (
  echo Removing old startup script
  del /q %bdj4oldstartnm%
)
if exist "%bdj4startupnm%" (
  echo Removing startup program
  del /q %bdj4startupnm%
)

cd %roaming%
if exist "%bdj4conf%" (
  echo Removing configuration folder
  rmdir /s/q %bdj4conf%
)
cd %TEMP%
if exist "%bdj4cache%" (
  echo Removing cache folder
  rmdir /s/q %bdj4cache%
)

exit /b
