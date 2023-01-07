#!/usr/bin/tclsh

if { $argc != 2 } {
  puts "usage: $argv0 <bdj3dir> <datatopdir>"
  exit 1
}

set bdj3dir [lindex $argv 0]
if { ! [file exists $bdj3dir] || ! [file isdirectory $bdj3dir] } {
  puts "Invalid directory $bdj3dir"
  exit 1
}
set datatopdir [lindex $argv 1]

# the orgopt.txt file is always new
# audioadjust is new as of version 4.0.5
foreach fn [list audioadjust.txt autoselection.txt \
    favorites.txt orgopt.txt sortopt.txt] {
  set nfn [file join $datatopdir data $fn]

  if { ! [file exists templates/$fn] } {
    continue
  }
  file copy -force templates/$fn $nfn
}

# orgopt.txt

# get the user's ORGPATH
set ifh [open [file join $datatopdir data bdjconfig.txt] r]
set next 0
while { [gets $ifh line] >= 0 } {
  if { $next == 1 } {
    set orgpath $line
    break;
  }
  if { $line eq "ORGPATH" } {
    set next 1
  }
}
close $ifh
regsub {^\.\.} $orgpath {} orgpath

# now check and make sure that the user's ORGPATH is in orgopt.txt
set orgfn [file join $datatopdir data orgopt.txt]
set ifh [open $orgfn r]
set ok 0
while { [gets $ifh line] >= 0 } {
  if { $line eq $orgpath } {
    set ok 1
  }
}
close $ifh

# if not found, add it to orgopt.txt
if { $ok == 0 } {
  set fh [open $orgfn a]
  puts $fh "# converted organization path"
  puts $fh $orgpath
  close $fh
}

# finished with orgopt.txt

# ds-*

# ds-* is a known good match
set fnlist [glob -directory templates ds-*.txt]
try {
  set proflist [glob -directory [file join $datatopdir data] profile*]
} on error {err res} {
  set proflist {}
}
foreach prof $proflist {
  foreach fn $fnlist {
    set nfn [file join $prof [file tail $fn]]
    if { ! [file exists $fn] } {
      continue
    }
    file copy -force $fn $nfn
  }
}

# itunes-* is a known good match
set fnlist [glob -directory templates itunes-*.txt]
foreach fn $fnlist {
  set nfn [file join $datatopdir data [file tail $fn]]
  if { ! [file exists $fn] } {
    continue
  }
  file copy -force $fn $nfn
}

exit 0
