@echo off
@rem windows install dir clean (template)
@rem Copyright 2023-2026 Brad Lanam Pleasant Hill CA
set ver=3

if "%1" == "" (
  @rem Restart the script in the background.
  @rem This is needed for windows, even though the process is detached.
  @rem But not needed for msys2.
  start "Installation cleanup." cmd /c "%0" restart
)

title Installation cleanup.
echo -- Installation cleanup.  Please wait...
taskkill /f /im gdbus.exe 2>NUL

@rem sleep for 3 seconds, wait for the installer and gdbus to exit
ping 127.0.0.1 -n 3 >NUL

set roaming=%USERPROFILE%\AppData\Roaming
set bdj4inst=bdj4-install
set bdj4cab=bdj4-install.cab

cd %roaming%
if exist "%bdj4inst%" (
  echo Removing temporary installation folder
  rmdir /s/q %bdj4inst%
)
if exist "%bdj4cab%" (
  echo Removing temporary installation cabinet
  rmdir /s/q %bdj4cab%
)

@rem tricky thing to delete the script
(goto) 2>nul & del "%~f0"
