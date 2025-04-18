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
if { ! [regexp {/data$} $bdj3dir] } {
  append bdj3dir /data
}
if { ! [file exists $bdj3dir] || ! [file isdirectory $bdj3dir] } {
  puts "Invalid directory $bdj3dir"
  exit 1
}
set datatopdir [lindex $argv 1]
if { ! [file exists $datatopdir/data] || ! [file isdirectory $datatopdir/data] } {
  puts "Invalid directory $datatopdir"
  exit 1
}

set infn [file join $bdj3dir dancedefaults.tcl]
if { ! [file exists $infn] } {
  puts "   no dances file"
  exit 1
}

set nfn [file join $datatopdir data dances.txt]
source $infn
set fh [open $nfn w]
puts $fh "# dances"
puts $fh "# [clock format [clock seconds] -gmt 1 -format {%Y-%m-%d %H:%M:%S}]"
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
    if { $k eq "lowbpm" } { set k LOWMPM }
    if { $k eq "highbpm" } { set k HIGHMPM }
    if { $k eq "select" } { continue }
    if { $k eq "count" } { continue }
    if { $k eq "ann" } { set k ANNOUNCE }
    if { $v eq "rhythm" } { set v latin }
    if { $v eq "smooth" } { set v standard }
    puts $fh [string toupper $k]
    puts $fh "..$v"
  }
}
close $fh

