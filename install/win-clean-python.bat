@echo off
@rem windows clean all python installations and data
@rem Copyright 2023-2026 Brad Lanam Pleasant Hill CA
set ver=1

echo -- Cleaning up python.  Please wait...

set tdir="%USERPROFILE%/AppData/Local/Programs/Python"
if exist %tdir% (
  echo      Clean %tdir%
  rmdir /s/q %tdir%
)

set tdir="%USERPROFILE%/AppData/Roaming/Python"
if exist %tdir% (
  echo      Clean %tdir%
  rmdir /s/q %tdir%
)

setlocal enabledelayedexpansion
for %%v in (3.1 3.2 3.3 3.4 3.5 3.6 3.7 3.8 3.9 3.10 3.11) do (
  set tdir="%USERPROFILE%/Start Menu/Programs/Python %%v"
  if exist !tdir! (
    echo      Clean !tdir!
    rmdir /s/q !tdir!
  )
)

exit /b
