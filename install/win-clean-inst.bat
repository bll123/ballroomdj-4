@echo off
@rem windows install dir clean (template)
@rem Copyright 2023 Brad Lanam Pleasant Hill CA
set ver=2

title "Installation cleanup."
echo -- Installation cleanup.
taskkill /f /im gdbus.exe 2>NUL

@rem sleep for 3 seconds, wait for the installer and gdbus to exit
@rem both timeout and ping show stuff on the screen despite redirects.
timeout /t 3 >NUL

rmdir /s/q "#UNPACKDIR#"

@rem tricky thing to delete the script
(goto) 2>nul & del "%~f0"
