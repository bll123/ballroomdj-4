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

set flist [glob -directory $dir *.seq]

foreach {fn} $flist {
  set ifh [open $fn r]
  set nfn [file join data [file rootname [file tail $fn]].seq4]
  set ofh [open $nfn w]
  puts $ofh "# BDJ4 sequence"
  puts $ofh "# Converted from $fn"
  puts $ofh "# [clock format [clock seconds] -gmt 1]"
  while { [gets $ifh line] >= 0 } {
    puts $ofh $line
  }
  close $ifh
  close $ofh
}
