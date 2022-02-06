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

set fn dancetypes.tcl
set nfn [file join data dancetypes.txt]
set infn [file join $dir $fn]
puts "Converting: $fn : $nfn"
if { [file exists $infn] } {
  source $infn
  set fh [open $nfn w]
  puts $fh "# BDJ4 dance types"
  puts $fh "# [clock format [clock seconds] -gmt 1]"
  puts $fh "# version 1"
  foreach {item} $typevals {
    puts $fh $item
  }
  close $fh
}
