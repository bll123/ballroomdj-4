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

source [file join $bdj3dir sortopt.tcl]
set nfn [file join $datatopdir data sortopt.txt]
puts "-- Converting: sortopt.tcl : sortopt.txt"

set fh [open $nfn w]
puts $fh "# BDJ4 sort options"
puts $fh "# [clock format [clock seconds] -gmt 1]"
puts $fh "# version 1"
foreach {item} $sortOptionList {
  puts $fh $item
}
close $fh

