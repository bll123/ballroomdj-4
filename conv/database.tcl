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

set hsize 128
set rsize 2048

set infn [file join $bdj3dir musicdb.txt]
if { ! [file exists $infn] } {
  puts "   no database"
  exit 1
}

set fh [open $infn r]
gets $fh line
gets $fh line
gets $fh line
gets $fh line
regexp {^#RAMAX=(\d+)$} $line all racount
close $fh

set dbfn [file join $bdj3dir musicdb.txt]
set fh [open $dbfn r]
gets $fh line
if { [regexp {^# ?VERSION=(\d+)\s*$} $line all vers] } {
  if { $vers < 9 } {
    puts "Database version is too old to convert."
    exit 1
  }
} else {
  puts "Unable to locate database version"
  exit 1
}
close $fh

source $dbfn
if { [info exists masterList] } {
  # handle older database versions
  set musicdbList $masterList
  unset masterList
}

set ssdict [dict create]
set sscount 1

set c 0
dict for {fn data} $musicdbList {
  # build the ssdict now so that singletons can be identified
  if { [dict exists $data SAMESONG] } {
    set value [dict get $data SAMESONG]
    if { $value ne {} } {
      if { ! [dict exists $ssdict $value] } {
        dict set ssdict $value samesong $sscount
        dict set ssdict $value count 1
        set value $sscount
        incr sscount
      } else {
        set count [dict get $ssdict $value count]
        incr count
        dict set ssdict $value count $count
      }
    }
  }
  # get the total count
  incr c
}

set fh [open [file join $datatopdir data musicdb.dat] w]
puts $fh "#VERSION=10"
puts $fh "# Do not edit this file."
puts $fh "#RASIZE=2048"
puts $fh "#RACOUNT=$c"

set newrrn 1
dict for {fn data} $musicdbList {
  seek $fh [expr {([dict get $data rrn] - 1) * $rsize + $hsize}]
  puts $fh "FILE\n..$fn"

  # sort it now, make it easier
  foreach {tag} [lsort [dict keys $data]] {
    if { $tag eq "BDJSYNCID" } { continue }
    if { $tag eq "ALBART" } { continue }
    if { $tag eq "DURATION_HMS" } { continue }
    if { $tag eq "DURATION_STR" } { continue }
    if { $tag eq "DISPLAYIMG" } { continue }
    if { $tag eq "NOMAXPLAYTIME" } { continue }
    if { $tag eq "VARIOUSARTISTS" } { continue }
    if { $tag eq "UPDATEFLAG" } { continue }
    if { $tag eq "VOLUMEADJUSTMENT" } { continue }
    if { $tag eq "WRITETIME" } { continue }
    # not going to do the sort-of-odd sometimes re-org stuff
    if { $tag eq "AUTOORGFLAG" } { continue }
    # FILE is already handled
    if { $tag eq "FILE" } { continue }

    set value [dict get $data $tag]

    if { $tag eq "rrn" } {
      # make sure rrn is correct
      set tag RRN
      set value $newrrn
    }

    if { $tag eq "TRACKTOTAL" || $tag eq "DISCTOTAL" } {
      if { $value == 0 } {
        # no point in writing not-useful data
        continue;
      }
    }

    if { $tag eq "ADJUSTFLAGS" } {
      # adjustflags is saved as a string of NTS characters.
      # spaces are no longer used.
      regsub -all { } $value {} value
      regsub -all {A} $value {S} value
    }

    if { $tag eq "SAMESONG" } {
      if { $value eq {} } {
        continue
      }
      if { [dict get $ssdict $value count] == 1 } {
        # singleton, clear the samesong setting
        continue;
      } else {
        set value [dict get $ssdict $value samesong]
      }
    }

    if { $tag eq "VOLUMEADJUSTPERC" } {
      set value [expr {int ($value)}]
      set value [expr {double ($value) / 10.0}]
      # make sure the volume-adjust-perc has a decimal point in it.
      set value [format {%.2f} $value]
    }

    if { $tag eq "SONGSTART" || $tag eq "SONGEND" } {
      if { $value ne {} && $value ne "-1" } {
        regexp {(\d+):(\d+)} $value all min sec
        regsub {^0*} $min {} min
        if { $min eq {} } { set min 0 }
        regsub {^0*} $sec {} sec
        if { $sec eq {} } { set sec 0 }
        set value [expr {($min * 60 + $sec)*1000}]
      }
    }

    if { $tag eq "MUSICBRAINZ_TRACKID" } { set tag RECORDING_ID }
    if { $tag eq "DISCNUMBER" } { set tag DISC }
    if { $tag eq "UPDATETIME" } { set tag LASTUPDATE }
    if { $tag eq "DURATION" } {
      set value [expr {int($value * 1000)}]
    }
    puts $fh "$tag\n..$value"
  }
  incr newrrn
}

# need to make sure the last record is full-size
# write out a null at the end-1
seek $fh [expr {($newrrn - 1) * $rsize + $hsize - 1}]
puts -nonewline $fh "\0"

close $fh
exit 0
