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


puts "## Converting: status.tcl"
set infn [file join $dir status.tcl]
if { ! [file exists $infn] } {
  puts "   no status file"
  exit 1
}
source $infn
set nfn [file join data status.txt]
set fh [open $nfn w]
puts $fh "# BDJ4 status"
puts $fh "# [clock format [clock seconds] -gmt 1]"
puts $fh version
puts $fh "..1"
puts $fh count
puts $fh "..[expr {[llength $Status]/2}]"
foreach {key data} $Status {
  puts $fh "KEY\n..$key"
  foreach {k v} $data {
    if { $k eq "oktoplay" } { set k playflag }
    set k [string toupper $k]
    puts $fh $k
    puts $fh "..$v"
  }
}
close $fh

