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

set infn [file join $bdj3dir dancelevels.tcl]
if { ! [file exists $infn] } {
  puts "   no levels file"
  exit 1
}

set nfn [file join $datatopdir data levels.txt]
source $infn
set fh [open $nfn w]
puts $fh "# levels"
puts $fh "# [clock format [clock seconds] -gmt 1 -format {%Y-%m-%d %H:%M:%S}]"
puts $fh version
puts $fh "..1"
puts $fh count
puts $fh "..[expr {[llength $Levels]/2}]"
if { [dict exists $Levels 0 label] } {
  foreach {key data} $Levels {
    puts $fh "KEY\n..$key"
    foreach {k v} $data {
      if { $k eq "label" } { set k level }
      set k [string toupper $k]
      puts $fh "$k\n..$v"
    }
  }
} else {
  # an old format of the levels file.
  set key 0
  set w 50
  foreach {lvl dflt} $Levels {
    puts $fh "KEY\n..$key"
    puts $fh "LEVEL\n..$lvl"
    puts $fh "DEFAULT\n..$dflt"
    puts $fh "WEIGHT\n..$w"
    incr key
    if { $key == 1 } { set w 40 }
    if { $key == 2 } { set w 10 }
  }
}
close $fh

