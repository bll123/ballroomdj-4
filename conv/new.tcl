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

puts "cwd: [pwd]"
foreach fn [list optorg.tcl] {
  set nfn [file join $datatopdir data $fn]

  if { ! [file exists templates/$fn] } {
    continue
  }
  file copy -force templates/$fn $nfn
}

set fnlist [glob -directory templates ds*.txt]
set proflist [glob -directory [file join $datatopdir data] profile*]
foreach prof $proflist {
  foreach fn $fnlist {
    set nfn [file join $prof [file tail $fn]]
    if { ! [file exists $fn] } {
      continue
    }
    file copy -force $fn $nfn
  }
}

exit 0

