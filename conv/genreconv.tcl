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

set nfn [file join data genres.txt]
puts "Converting: genres.tcl : $nfn"
source [file join $dir genres.tcl]
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

