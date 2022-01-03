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

puts "## Converting: dancedefaults.tcl"
source [file join $dir dancedefaults.tcl]
set nfn [file join data dances.txt]
set fh [open $nfn w]
puts $fh "# BDJ4 dances"
puts $fh "# [clock format [clock seconds] -gmt 1]"
puts $fh version
puts $fh "..2"
puts $fh count
puts $fh "..[expr {[llength $Dance]/2}]"
set ikey 0
foreach {key data} $Dance {
  puts $fh "KEY\n..$ikey"
  incr ikey;
  puts $fh "DANCE\n..$key"
  foreach {k v} $data {
    if { $k eq "ann" } { set k ANNOUNCE }
    puts $fh [string toupper $k]
    puts $fh "..$v"
  }
}
close $fh

