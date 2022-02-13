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

set infn [file join $bdj3dir status.tcl]
set nfn [file join $datatopdir data status.txt]
if { ! [file exists $infn] } {
  puts "   no status file"
  exit 1
}
source $infn
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

