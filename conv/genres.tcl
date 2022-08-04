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

set infn [file join $bdj3dir genres.tcl]
if { ! [file exists $infn] } {
  puts "   no genres file"
  exit 1
}

set nfn [file join $datatopdir data genres.txt]
source $infn
set fh [open $nfn w]
puts $fh "# genres"
puts $fh "# [clock format [clock seconds] -gmt 1 -format {%Y-%m-%d %H:%M:%S}]"
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

