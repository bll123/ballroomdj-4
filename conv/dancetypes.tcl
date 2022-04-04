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

set nfn [file join $datatopdir data dancetypes.txt]

if { ! [file exists $nfn] } {
  file copy -force templates/dancetypes.txt $nfn
}

set fn dancetypes.tcl
set infn [file join $bdj3dir $fn]
if { ! [file exists $infn] } {
  puts "   no dance types file"
  exit 1
}

if { [file exists $infn] } {
  source $infn
  set fh [open $nfn w]
  puts $fh "# BDJ4 dance types"
  puts $fh "# [clock format [clock seconds] -gmt 1]"
  puts $fh "# version 1"
  foreach {item} $typevals {
    if { $item eq "rhythm" } { set item latin }
    puts $fh $item
  }
  close $fh
}
