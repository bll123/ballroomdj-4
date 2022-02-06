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

source [file join $dir sortopt.tcl]
set nfn [file join data sortopt.txt]
puts "Converting: sortopt.tcl : $nfn"

set fh [open $nfn w]
puts $fh "# BDJ4 sort options"
puts $fh "# [clock format [clock seconds] -gmt 1]"
puts $fh "# version 1"
foreach {item} $sortOptionList {
  puts $fh $item
}
close $fh

