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

set nfn [file join $datatopdir data genres.txt]
puts "-- Converting: genres.tcl : genres.txt"
source [file join $bdj3dir genres.tcl]
set fh [open $nfn w]
puts $fh "# BDJ4 genres"
puts $fh "# [clock format [clock seconds] -gmt 1]"
puts $fh version
puts $fh "..1"
puts $fh count
puts $fh "..[expr {[llength $Genres]/2}]"
foreach {key data} $Genres {
  puts $fh "KEY\n..$key"
  foreach {k v} $data {
    set k [string toupper $k]
    puts $fh $k
    puts $fh "..$v"
  }
}
close $fh

