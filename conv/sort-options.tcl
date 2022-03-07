#!/usr/bin/tclsh

if { $argc != 2 } {
  puts "usage: $argv0 <bdj3dir> <datatopdir>"
  exit 1
}

set bdj3dir [lindex $argv 1]
if { ! [file exists $bdj3dir] || ! [file isdirectory $bdj3dir] } {
  puts "Invalid directory $bdj3dir"
  exit 1
}
set datatopdir [lindex $argv 2]

set nfn [file join $datatopdir data sortopt.txt]
if { ! [file exists $nfn] } {
  file copy -force templates/sortopt.txt $nfn
}

if { [lindex $argv 0] != "--force" } {
  puts "skipping sort option conversion"
  exit 0
}

set infn [file join $bdj3dir sortopt.tcl]
if { ! [file exists $infn] } {
  puts "   no sort options file"
  exit 1
}

source $infn

set fh [open $nfn w]
puts $fh "# BDJ4 sort options"
puts $fh "# [clock format [clock seconds] -gmt 1]"
puts $fh "# version 1"
foreach {item} $sortOptionList {
  regsub -all {/} $item { } item
  regsub -all {S_} $item {} item
  regsub -all {ALBART} $item {ALBUMARTIST} item
  puts $fh $item
}
close $fh

