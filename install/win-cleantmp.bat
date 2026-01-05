@echo off
@rem Copyright 2023-2025 Brad Lanam Pleasant Hill CA
@rem BDJ4 clean

set bdj4dir=#BDJ4DIR#
if exist "%bdj4dir%" (
  start /MIN "BDJ4 Cleanup" "%bdj4dir%\bin\bdj4" --bdj4cleantmp
)
