@ECHO OFF
REM windows conversion script

# the "ERROR:" ouput prefixes must exist.

set dir="%1"

if not exist "%dir%" (
  echo "ERROR: No BallroomDJ directory"
  exit /B 1
)

if not exist "%dir%/data" (
  echo "ERROR: No BallroomDJ 3 data directory"
  exit /B 1
)

set dir="%dir%/data"

set bits=32
if exist "C:\Program Files (x86)" (
  set bits=64
)

set tclsh="%dir%\..\windows\%bits%\tcl\bin\tclsh.exe"
if not exist %tclsh% (
  set tclsh="%dir%\..\..\windows\%bits%\tcl\bin\tclsh.exe"
  if not exist %tclsh% (
    echo "ERROR: Unable to locate tclsh.exe"
    exit /B 1
  )
)

echo "Located tclsh: %tclsh%"

if not exist "data" (
  echo "ERROR: No BDJ4 data directory"
  exit /B 1
)

if not exist "conv" (
  echo "ERROR: No BDJ4 conv directory"
  exit /B 1
)

%tclsh% ./conv/configconv.tcl %dir%
%tclsh% ./conv/danceconv.tcl %dir%
%tclsh% ./conv/dbconv.tcl %dir%
%tclsh% ./conv/genreconv.tcl %dir%
%tclsh% ./conv/levelsconv.tcl %dir%
%tclsh% ./conv/mlistconv.tcl %dir%
%tclsh% ./conv/playlistconv.tcl %dir%
%tclsh% ./conv/ratingconv.tcl %dir%
%tclsh% ./conv/seqconv.tcl %dir%
%tclsh% ./conv/sortoptconv.tcl %dir%
%tclsh% ./conv/autoselconv.tcl %dir%
%tclsh% ./conv/typeconv.tcl %dir%
%tclsh% ./conv/statusconv.tcl %dir%
echo "Conversion Complete"
echo "OK"
exit 0
