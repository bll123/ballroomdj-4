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

set nfn [file join data levels.txt]
puts "-- Converting: dancelevels.tcl : $nfn"
source [file join $dir dancelevels.tcl]
set fh [open $nfn w]
puts $fh "# BDJ4 levels"
puts $fh "# [clock format [clock seconds] -gmt 1]"
puts $fh version
puts $fh "..1"
puts $fh count
puts $fh "..[expr {[llength $Levels]/2}]"
foreach {key data} $Levels {
  puts $fh "KEY\n..$key"
  foreach {k v} $data {
    if { $k eq "label" } { set k level }
    set k [string toupper $k]
    puts $fh $k
    puts $fh "..$v"
  }
}
close $fh

