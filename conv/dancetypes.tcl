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

set fn dancetypes.tcl
set infn [file join $bdj3dir $fn]
set nfn [file join $datatopdir data dancetypes.txt]

if { ! [file exists $infn] } {
  puts "   dance types: copying template"
  file copy -force templates/dancetypes.txt $nfn
  exit 0
}

source $infn
set fh [open $nfn w]
puts $fh "# dance-types"
puts $fh "# [clock format [clock seconds] -gmt 1 -format {%Y-%m-%d %H:%M:%S}]"
puts $fh "# version 1"
foreach {item} $typevals {
  if { $item eq "rhythm" } { set item latin }
  if { $item eq "smooth" } { set item standard }
  puts $fh $item
}
close $fh
