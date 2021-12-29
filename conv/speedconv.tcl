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

set fn dancespeeds.tcl
puts "## Converting: $fn"
source [file join $dir $fn]
set nfn [file join data dancespeeds.txt]
set fh [open $nfn w]
puts $fh "# BDJ4 speeds"
puts $fh "# [clock format [clock seconds] -gmt 1]"
puts $fh "# version 2"
foreach {item} $spdvals {
  puts $fh $item
}
close $fh

