#!/usr/bin/tclsh

if { $argc <= 0 } {
  puts "usage: $argv0 <directory>"
  exit 1
}

set dir [lindex $argv 0]
if { ! [file exists $dir] || ! [file isdirectory $dir] } {
  puts "Invalid directory $dir"
  exit 1
}

set flist [glob -directory $dir *.playlist]

foreach {fn} $flist {
  puts "## Converting: [file tail $fn]"
  set ifh [open $fn r]
  set nfn [file join data [file rootname [file tail $fn]].playlist4]
  set ofh [open $nfn w]
  puts $ofh "# BDJ4 playlist"
  puts $ofh "# Converted from $fn"
  puts $ofh "# [clock format [clock seconds] -gmt 1]"
  puts $ofh "datafiletype"
  puts $ofh "..playlist"
  while { [gets $ifh line] >= 0 } {
    regexp {^([^:]*):(.*)$} $line all key value
    if { $key eq "version" } {
      set value 10
    }
    puts $ofh $key
    puts $ofh "..$value"
  }
  close $ifh
  close $ofh
}
