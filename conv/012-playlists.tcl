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

set rfn [file join $datatopdir data ratings.txt]
set ifh [open $rfn r]
set flag 0
while { [gets $ifh line] >= 0 } {
  if { $flag == 1 } {
    set unratedtxt [string range $line 2 end]
    break
  }
  if { $line eq "RATING" } {
    set flag 1
  }
}
close $ifh

try {
  set flist [glob -directory $bdj3dir *.playlist]
} on error { err res } {
  set flist {}
}

foreach {fn} $flist {
  set basename [file rootname [file tail $fn]]

  if { $basename eq "Raffle Songs" } {
    continue
  }
  if { $basename eq "Loterij liedjes" } {
    continue
  }

  set nfn [file join $datatopdir data ${basename}.pl]
  #  puts "   - [file tail $fn] : ${basename}.pl"
  set ifh [open $fn r]
  set dfn [file join $datatopdir data ${basename}.pldances]
  set ofh [open $nfn w]
  set dfh [open $dfn w]
  puts $ofh "# playlist"
  puts $ofh "# Converted from $fn"
  puts $ofh "# [clock format [clock seconds] -gmt 1 -format {%Y-%m-%d %H:%M:%S}]"
  puts $ofh "version"
  puts $ofh "..1"

  puts $dfh "# playlist-dances"
  puts $dfh "# Converted from $fn"
  puts $dfh "# [clock format [clock seconds] -gmt 1 -format {%Y-%m-%d %H:%M:%S}]"
  puts $dfh "version"
  puts $dfh "..1"
  # count is unknown

  set pltype automatic
  set keyidx 0
  set unratedval no
  set drvalue {}

  while { [gets $ifh line] >= 0 } {
    regexp {^([^:]*):(.*)$} $line all key value
    if { $key eq "list" } { continue }
    if { $key eq "version"} { continue }

    set tkey [string toupper $key]

    if { $tkey eq "MANUALLIST" && $value ne {} && $value ne {None} } {
      set pltype songlist
    }
    if { $tkey eq "SEQUENCE" && $value ne {} && $value ne {None} } {
      set pltype sequence
    }

    if { $tkey eq "REQUIREDKEYWORDS" } { continue }
    if { $tkey eq "STOPWAIT" } { continue }
    if { $tkey eq "STOPAFTERWAIT" } { continue }
    if { $tkey eq "STOPTYPE" } { continue }
    if { $tkey eq "RESUME" } { continue }
    if { $tkey eq "MANUALLIST" } { continue }
    if { $tkey eq "SEQUENCE" } { continue }
    if { $tkey eq "MQMESSAGE" } { continue }
    if { $tkey eq "PAUSEEACHSONG" } { continue }
    if { $tkey eq "STATUSOK" } { continue }

    if { $tkey eq "HIGHDANCELEVEL" } { set key DANCELEVELHIGH }
    if { $tkey eq "LOWDANCELEVEL" } { set key DANCELEVELLOW }

    if { $tkey eq "GAP" } {
      if { $value eq {} || $value eq "0" } {
        set value -100
      } else {
        set value [expr {int ($value * 1000)}]
      }
    }
    if { $tkey eq "VERSION" } { set key version }

    if { [regexp {:\d+:} $value] } {
      puts $dfh KEY
      puts $dfh "..$keyidx"
      puts $dfh DANCE
      puts $dfh "..$key"
      regexp {(\d):(\d*):(\d*):(\d*):(\d*):(\d*)} $value \
          all selected count maxmin maxsec lowbpm highbpm
      if { $lowbpm eq {} } { set lowbpm 0 }
      if { $highbpm eq {} } { set highbpm 0 }
      puts $dfh "SELECTED"
      puts $dfh "..$selected"
      puts $dfh "COUNT"
      puts $dfh "..$count"
      puts $dfh "MAXPLAYTIME"
      set tm 0
      if { $maxmin ne {} && $maxsec ne {} } {
        set tm [expr {($maxmin * 60 + $maxsec)*1000}]
      }
      puts $dfh "..$tm"
      puts $dfh "BPMLOW"
      puts $dfh "..$lowbpm"
      puts $dfh "BPMHIGH"
      puts $dfh "..$highbpm"
      incr keyidx
    } else {
      if { $tkey eq "STOPTIME" } {
        if { $value ne {} && $value ne "-1" } {
          # this will be relative to midnight
          regexp {(\d+):(\d+)} $value all hr min
          regsub {^0*} $hr {} hr
          if { $hr eq {} } { set hr 0 }
          regsub {^0*} $min {} min
          if { $min eq {} } { set min 0 }
          set value [expr {($hr * 3600 + $min * 60) * 1000}]
        } else {
          set value 0
        }
      }
      if { $tkey eq "MAXPLAYTIME" } {
        if { $value ne {} && $value ne "-1" } {
          regexp {(\d+):(\d+)} $value all min sec
          regsub {^0*} $min {} min
          if { $min eq {} } { set min 0 }
          regsub {^0*} $sec {} sec
          if { $sec eq {} } { set sec 0 }
          set value [expr {($min * 60 + $sec)*1000}]
        } else {
          set value 0
        }
      }

      if { $tkey eq "DANCERATING" } {
        if { $drvalue eq {} } {
          set drvalue $value
        }
        continue
      }
      if { $tkey eq "UNRATEDOK" } {
        if { $value eq "yes" } {
          set drvalue ${unratedtxt}
        }
        continue
      }

      set key [string toupper $key]
      puts $ofh $key
      puts $ofh "..$value"
    }
  }
  puts $ofh "DANCERATING"
  puts $ofh "..$drvalue"
  puts $ofh "TYPE"
  puts $ofh "..$pltype"
  close $ifh
  close $ofh

  if { $basename eq "Quick Play" } {
    set ofn [file join $datatopdir data ${basename}.pl]
    set nfn [file join $datatopdir data QueueDance.pl]
    file rename -force $ofn $nfn
    set ofn [file join $datatopdir data ${basename}.pldances]
    set nfn [file join $datatopdir data QueueDance.pldances]
    file rename -force $ofn $nfn
  }
}
