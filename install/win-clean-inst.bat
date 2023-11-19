@echo off
@rem windows install dir clean (template)
@rem Copyright 2023 Brad Lanam Pleasant Hill CA
set ver=2

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
timeout /t 3 >NUL

rmdir /s/q "#UNPACKDIR#"

@rem tricky thing to delete the script
(goto) 2>nul & del "%~f0"
