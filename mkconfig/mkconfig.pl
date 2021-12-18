#!/usr/bin/perl
#
# Copyright 2006-2018 Brad Lanam Walnut Creek, CA USA
# Copyright 2020 Brad Lanam Pleasant Hill CA
#

# HP-UX doesn't have these installed.
# use strict;
# use Config;
require 5.005;

my $CONFH;
my $MKC_FILES = 'mkc_files';
if ( $ENV{'MKC_FILES'} ne "" ) {
  $MKC_FILES = $ENV{'MKC_FILES'};
}
mkdir $MKC_FILES, 0777;

my $LOG = "../../${MKC_FILES}/mkconfig.log";
my $_MKCONFIG_TMP = "${MKC_FILES}/_tmp_mkconfig";
my $OPTIONFILE = "../../options.dat";
my $VARSFILE = "../../${MKC_FILES}/mkc_none_c.vars";
my $CACHEFILE = "../../${MKC_FILES}/mkconfig.cache";
my $REQLIB = "../../${MKC_FILES}/mkconfig.reqlibs";
my $_MKCONFIG_DIR = "invalid";

my $precc = <<'_HERE_';
#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
# define _(x) x
#else
# define _(x) ()
# define void char
#endif
#if defined(__cplusplus) || defined (c_plusplus)
# define CPP_EXTERNS_BEG extern "C" {
# define CPP_EXTERNS_END }
CPP_EXTERNS_BEG
extern int printf (const char *, ...);
CPP_EXTERNS_END
#else
# define CPP_EXTERNS_BEG
# define CPP_EXTERNS_END
#endif
_HERE_

my $postcc = <<'_HERE_';
/* some gcc's (cygwin) redefine __restrict again */
#if defined (__restrict)
# undef __restrict
#endif
#define __restrict
_HERE_

my $optionsloaded = 0;
my %optionshash;
my $iflevels = '';

my $awkcmd = 'awk';
foreach my $p (split /[;:]/o, $ENV{'PATH'})
{
    if (-x "$p/awk" && $awkcmd eq "") {
        $awkcmd = "$p/awk";
    }
    if (-x "$p/nawk" && $awkcmd !~ /mawk$/o && $awkcmd !~ /gawk$/o) {
        $awkcmd = "$p/nawk";
    }
    if (-x "$p/mawk" && $awkcmd !~ /gawk$/o) {
        $awkcmd = "$p/mawk";
    }
    if (-x "$p/gawk") {
        $awkcmd = "$p/gawk";
    }
}

sub
exitmkconfig
{
    my $rc = shift;
    exit 1;
}

sub
printlabel
{
    my ($name, $label) = @_;
    print LOGFH "## ${iflevels}[$name] $label ... \n";
    print STDOUT "${iflevels}$label ... ";
}

sub
printyesno_actual
{
    my ($name, $val, $tag) = @_;

    print LOGFH "## [$name] $val $tag\n";
    print STDOUT "$val $tag\n";
}

sub
printyesno_val
{
  my ($name, $val, $tag) = @_;

  if ($val ne '0') {
    printyesno_actual ($name, $val, $tag);
  } else {
    printyesno_actual ($name, 'no', $tag);
  }
}

sub
printyesno
{
    my ($name, $val, $tag) = @_;

    if ($val ne "0")
    {
        $val = "yes";
    }
    printyesno_val $name, $val, $tag;
}

sub
savevars
{
    my ($r_clist) = @_;

    open (MKCV, ">$VARSFILE");
    foreach my $val (@{$r_clist->{'vars'}})
    {
      print MKCV $val, "\n";
    }
    close (MKCV);
}

sub
savecache
{
    my ($r_clist, $r_config) = @_;

    open (MKCC, ">$CACHEFILE");
    foreach my $val (@{$r_clist->{'clist'}})
    {
      if ($val =~ /^lib__lib_/o) {
        $tval = $val;
        $tval =~ s/^lib_//o;
        print MKCC "mkc_c_lib_${tval}='" . $r_config->{$val} . "'\n";
      } else {
        print MKCC "mkc_c_${val}='" . $r_config->{$val} . "'\n";
      }
    }
    close (MKCC);
}

sub
checkcache_val
{
  my ($name, $r_clist, $r_config) = @_;

  my $rc = 1;
  if (defined ($r_config->{$name}) && $r_config->{$name} ne "" )
  {
    push @{$r_clist->{'vars'}}, $name;
    my $r_hash = $r_clist->{'vhash'};
    $r_hash->{$name} = 1;
    printyesno_val $name, $r_config->{$name}, " (cached)";
    $rc = 0;
  }
  return $rc;
}

sub
checkcache
{
  my ($name, $r_clist, $r_config) = @_;

  my $rc = 1;
  if (defined ($r_config->{$name}) && $r_config->{$name} ne "" )
  {
    push @{$r_clist->{'vars'}}, $name;
    my $r_hash = $r_clist->{'vhash'};
    $r_hash->{$name} = 1;
    printyesno $name, $r_config->{$name}, " (cached)";
    $rc = 0;
  }
  return $rc;
}

sub
loadoptions
{
  if ($optionsloaded == 0 && open (OPTS, "<$OPTIONFILE")) {
    while (my $o = <OPTS>) {
      chomp $o;
      if ($o =~ /^$/o || $o =~ /^#/o) {
        next;
      }
      my ($onm, $val) = split (/=/, $o);
      printf LOGFH "## load: $onm = $val\n";
      $optionshash{$onm} = $val;
    }
    $optionsloaded = 1;
    close (OPTS);
  }
}

sub
setclist
{
    my ($r_clist, $name) = @_;
    $r_hash = $r_clist->{'chash'};
    if (! defined ($r_hash->{$name}))
    {
      push @{$r_clist->{'clist'}}, $name;
      $r_hash->{$name} = 1;
    }
}

sub
setlist
{
    my ($r_clist, $name) = @_;
    my $r_hash = $r_clist->{'vhash'};
    if (! defined ($r_hash->{$name}))
    {
      push @{$r_clist->{'vars'}}, $name;
      $r_hash->{$name} = 1;
    }
    setclist ($r_clist, $name);
}

sub
setcflags
{
  my $CFLAGS = '';
  if ( $ENV{'CFLAGS_ALL'} ne "" ) {
    $CFLAGS= $ENV{'CFLAGS_ALL'};
  } else {
    $CFLAGS .= ' ' . $ENV{'CFLAGS_OPTIMIZE'};
    $CFLAGS .= ' ' . $ENV{'CFLAGS_DEBUG'};
    $CFLAGS .= ' ' . $ENV{'CFLAGS_INCLUDE'};
    $CFLAGS .= ' ' . $ENV{'CFLAGS_USER'};
    $CFLAGS .= ' ' . $ENV{'CFLAGS_APPLICATION'};
    $CFLAGS .= ' ' . $ENV{'CFLAGS_COMPILER'};
    $CFLAGS .= ' ' . $ENV{'CFLAGS_SYSTEM'};
  }
  $ENV{'CFLAGS'} = $CFLAGS;
}

sub
setldflags
{
  my $LDFLAGS = '';
  if ( $ENV{'LDFLAGS_ALL'} ne "" ) {
    $LDFLAGS = $ENV{'LDFLAGS_ALL'}
  } else {
    $LDFLAGS .= ' ' . $ENV{'LDFLAGS_OPTIMIZE'};
    $LDFLAGS .= ' ' . $ENV{'LDFLAGS_DEBUG'};
    $LDFLAGS .= ' ' . $ENV{'LDFLAGS_USER'};
    $LDFLAGS .= ' ' . $ENV{'LDFLAGS_APPLICATION'};
    $LDFLAGS .= ' ' . $ENV{'LDFLAGS_COMPILER'};
    $LDFLAGS .= ' ' . $ENV{'LDFLAGS_SYSTEM'};
  }
  $ENV{'LDFLAGS'} = $LDFLAGS;
}


sub
setlibs
{
  my $LIBS = '';
  if ( $ENV{'LDFLAGS_LIBS_ALL'} ne "" ) {
    $LIBS = $ENV{'LDFLAGS_LIBS_ALL'}
  } else {
    $LIBS .= ' ' . $ENV{'LDFLAGS_LIBS_USER'};
    $LIBS .= ' ' . $ENV{'LDFLAGS_LIBS_APPLICATION'};
    $LIBS .= ' ' . $ENV{'LDFLAGS_LIBS_SYSTEM'};
  }
  $ENV{'LIBS'} = $LIBS;
}

sub
print_headers
{
    my ($r_a, $r_clist, $r_config, $cppchk) = @_;
    my $txt;

    $txt = '';

    if ($r_a->{'incheaders'} eq 'none') {
      return $txt;
    }
    if ($r_a->{'incheaders'} eq 'all' ||
        $r_a->{'incheaders'} eq 'std')
    {
        # always include these four if present ...
        foreach my $val ('_hdr_stdio', '_hdr_stdlib', '_sys_types', '_sys_param')
        {
            if (defined ($r_config->{$val}) &&
                 $r_config->{$val} ne '0')
            {
                $txt .= "#include <" . $r_config->{$val} . ">\n";
                # for cygwin/gcc
                if ($cppchk && $val eq '_hdr_stdio') {
                  $txt .= $postcc;
                }
            }
        }
    }

    if ($r_a->{'incheaders'} eq 'all')
    {
        foreach my $val (@{$r_clist->{'vars'}})
        {
            if ($val !~ m#^(_hdr_|_sys_)#o)
            {
                next;
            }
            if ($val eq '_hdr_stdio' ||
                $val eq '_hdr_stdlib' ||
                $val eq '_sys_types' ||
                $val eq '_sys_param')
            {
                next;
            }
            if ($val eq '_sys_time' &&
                $r_config->{'_sys_time'} ne '0' &&
                $r_config->{'_inc_conflict__hdr_time__sys_time'} eq '0')
            {
                next;
            }
            if ($val eq '_hdr_linux_quota' &&
                $r_config->{'_hdr_linux_quota'} ne '0' &&
                $r_config->{'_inc_conflict__sys_quota__hdr_linux_quota'} eq '0')
            {
                next;
            }
            if ($r_config->{$val} ne '0')
            {
                $txt .= "#include <" . $r_config->{$val} . ">\n";
            }
        }
        $txt .= "\n";
    }

    return $txt;
}

sub
_chk_run
{
    my ($name, $code, $r_val, $r_clist, $r_config, $r_a) = @_;

    my $rc = _chk_link ($name, $code, $r_clist, $r_config,
        { 'incheaders' => 'all', %$r_a, });
    print LOGFH "##  run test: link: $rc\n";
    $$r_val = 0;
    if ($rc == 0)
    {
        $rc = system ("./$name.exe > $name.out");
        if ($rc & 127) { exitmkconfig ($rc); }
        $rc >>= 8;
        print LOGFH "##  run test: run: $rc\n";
        if ($rc == 0)
        {
            open (CRFH, "<$name.out");
            $$r_val = <CRFH>;
            chomp $$r_val;
            close CRFH;
        }
    }
    return $rc;
}

sub
_chk_link
{
    my ($name, $code, $r_clist, $r_config, $r_a) = @_;

    my $otherlibs = '';
    if (defined ($r_a->{'otherlibs'}))
    {
        $otherlibs = $r_a->{'otherlibs'};
    }

    open (CLFH, ">$name.c");
    print CLFH $precc;

    my $hdrs = print_headers ($r_a, $r_clist, $r_config, 0);
    print CLFH $hdrs;
    print CLFH $code;
    close CLFH;

    my $rc = system ("cat $name.c >> $LOG");
    if ($rc & 127) { exitmkconfig ($rc); }

    my $dlibs = '';
    $rc = _chk_link_libs ($name, {} );
    if ($rc != 0)
    {
      if ($otherlibs ne '')
      {
        my @olibs = split (/,/, $otherlibs);
        my $oliblist = '';
        foreach my $olib (@olibs)
        {
          $oliblist = $oliblist . ' ' . $olib;
          $rc = _chk_link_libs ($name, { 'otherlibs' => $oliblist, } );
          if ($rc == 0)
          {

              my $r_hash = $r_config->{'reqlibs'};
              my @vals = split (/\s+/, $oliblist);
              $dlibs = '';
              foreach my $val (@vals)
              {
                  if ($val eq '') { next; }
                  if (! defined ($r_hash->{$val}))
                  {
                      print LOGFH "   reqlib: $val: new\n";
                      push @{$r_config->{'reqlibs_list'}}, $val;
                  } else {
                      print LOGFH "   reqlib: $val: already\n";
                  }
                  $r_hash->{$val} = 1;
                  $dlibs .= $val . ' ';
              }
              last;
          }
        }
      }
    }

    $dlibs =~ s/ *$//o;
    $r_a->{'dlibs'} = $dlibs;

    return $rc;
}

sub
_chk_link_libs
{
    my ($name, $r_a) = @_;

    setcflags
    setldflags
    setlibs

    my $cmd = "$ENV{'CC'} $ENV{'CFLAGS'} ";
    if (defined ($r_a->{'cflags'}))
    {
        $cmd .= ' ' . $r_a->{'cflags'} . ' ';
    }
    $cmd .= "-o $name.exe $name.c";
    $cmd .= " $ENV{'LDFLAGS'} $ENV{'LIBS'}";
    if (defined ($r_a->{'otherlibs'}) && $r_a->{'otherlibs'} ne undef)
    {
        $cmd .= ' ' . $r_a->{'otherlibs'} . ' ';
    }
    print LOGFH "##  link test: $cmd\n";
    my $rc = system ("$cmd >> $LOG 2>&1");
    if ($rc & 127) { exitmkconfig ($rc); }
    print LOGFH "##      link test: $rc\n";
    if ($rc == 0)
    {
        if (! -x "$name.exe")  # not executable.
        {
            $rc = 1;
        }
    }
    return $rc;
}

sub
_chk_cpp
{
    my ($name, $code, $r_clist, $r_config, $r_a) = @_;

    open (CCFH, ">$name.c");
    print CCFH $precc;
    my $hdrs = print_headers ($r_a, $r_clist, $r_config, 1);
    print CCFH $hdrs;
    print CCFH $code;
    close CCFH;

    setcflags

    my $cmd = "$ENV{'CC'} $ENV{'CFLAGS'} ";
    if (defined ($r_a->{'cflags'}))
    {
        $cmd .= ' ' . $r_a->{'cflags'} . ' ';
    }
    $cmd .= "-E $name.c > $name.out ";
    my $rc = system ("cat $name.c >> $LOG");
    if ($rc & 127) { exitmkconfig ($rc); }
    $rc = system ("$cmd 2>>$LOG");
    print LOGFH "##  cpp test: $cmd\n";
    if ($rc & 127) { exitmkconfig ($rc); }
    print LOGFH "##      cpp test: $rc\n";
    if ($rc == 0 && ! -f "$name.out") {
      print LOGFH "##      can't locate $name.out\n";
      $rc = -2;
    }
    return $rc;
}

sub
_chk_compile
{
    my ($name, $code, $r_clist, $r_config, $r_a) = @_;

    open (CCFH, ">$name.c");
    print CCFH $precc;
    my $hdrs = print_headers ($r_a, $r_clist, $r_config, 0);
    print CCFH $hdrs;
    print CCFH $code;
    close CCFH;

    setcflags

    my $cmd = "$ENV{'CC'} $ENV{'CFLAGS'} -c $name.c";
    print LOGFH "##  compile test: $cmd\n";
    my $rc = system ("cat $name.c >> $LOG");
    if ($rc & 127) { exitmkconfig ($rc); }
    $rc = system ("$cmd >> $LOG 2>&1");
    if ($rc & 127) { exitmkconfig ($rc); }
    print LOGFH "##  compile test: $rc\n";
    return $rc;
}

sub
do_chk_compile
{
    my ($name, $code, $inc, $r_clist, $r_config) = @_;

    my $rc = _chk_compile ($name, $code, $r_clist, $r_config,
        { 'incheaders' => $inc, });
    my $trc = 0;
    if ($rc == 0)
    {
        $trc = 1;
    }
    printyesno $name, $trc;
    setlist $r_clist, $name;
    $r_config->{$name} = $trc;
}

sub
check_header
{
    my ($name, $file, $r_clist, $r_config, $r_a) = @_;

    printlabel $name, "header: $file";
    if (checkcache ($name, $r_clist, $r_config) == 0)
    {
        return;
    }

    my $r_rh = $r_a->{'reqhdr'} || [];
    my $code = '';
    foreach my $reqhdr (@$r_rh)
    {
        $code .= <<"_HERE_";
#include <$reqhdr>
_HERE_
    }
    $code .= <<"_HERE_";
#include <${file}>
int main () { return (0); }
_HERE_
    my $rc = 1;
    $rc = _chk_compile ($name, $code, $r_clist, $r_config,
        { 'incheaders' => 'std', });
    my $val = 0;
    if ($rc == 0)
    {
        $val = $file;
    }
    printyesno $name, $val;
    setlist $r_clist, $name;
    $r_config->{$name} = $val;
}

sub
check_constant
{
    my ($name, $constant, $r_clist, $r_config, $r_a) = @_;

    printlabel $name, "constant: $constant";
    if (checkcache ($name, $r_clist, $r_config) == 0)
    {
        return;
    }

    my $r_rh = $r_a->{'reqhdr'} || [];
    my $code = '';
    foreach my $reqhdr (@$r_rh)
    {
        $code .= <<"_HERE_";
#include <$reqhdr>
_HERE_
    }
    $code .= <<"_HERE_";
int main () { if (${constant} == 0) { 1; } return (0); }
_HERE_
    do_chk_compile ($name, $code, 'all', $r_clist, $r_config);
}

# if the keyword is reserved, the compile will fail.
sub
check_keyword
{
    my ($name, $keyword, $r_clist, $r_config) = @_;

    printlabel $name, "keyword: $keyword";
    if (checkcache ($name, $r_clist, $r_config) == 0)
    {
        return;
    }

    $r_config->{$name} = 0;
    my $code = <<"_HERE_";
int main () { int ${keyword}; ${keyword} = 1; return (0); }
_HERE_
    my $rc = _chk_compile ($name, $code, $r_clist, $r_config,
        { 'incheaders' => 'std', });
    setlist $r_clist, $name;
    if ($rc != 0)  # failure means it is reserved...
    {
        $r_config->{$name} = 1;
    }
    printyesno $name, $r_config->{$name};
}

sub
check_proto
{
    my ($name, $r_clist, $r_config) = @_;

    printlabel $name, "supported: prototypes";
    if (checkcache ($name, $r_clist, $r_config) == 0)
    {
        return;
    }

    my $code = <<"_HERE_";
CPP_EXTERNS_BEG
extern int foo (int, int);
CPP_EXTERNS_END
int bar () { int rc; rc = foo (1,1); return 0; }
_HERE_
    my $rc = _chk_compile ($name, $code, $r_clist, $r_config,
        { 'incheaders' => 'all', });
    setlist $r_clist, $name;
    $r_config->{$name} = 0;
    if ($rc == 0)
    {
        $r_config->{$name} = 1;
    }
    printyesno $name, $r_config->{$name};
}

sub
check_command
{
    my ($name, $cmds, $r_clist, $r_config) = @_;

    my @cmdlist = split (/ +/o, $cmds);
    my $cmd = $cmdlist[0];
    $name = "_command_${cmd}";
    my $locnm = "_cmd_loc_${cmd}";

    printlabel $name, "command: $cmd";
    if (checkcache ($name, $r_clist, $r_config) == 0)
    {
        return;
    }

    setlist $r_clist, $name;
    $r_config->{$name} = 0;
    foreach my $cmd (@cmdlist)
    {
      foreach my $p (split /[;:]/o, $ENV{'PATH'})
      {
        if (-x "$p/$cmd")
        {
          setlist $r_clist, $locnm;
          $r_config->{$locnm} = "$p/$cmd";
          $r_config->{$name} = 1;
          last;
        }
      }
    }

    printyesno_val $name, $r_config->{$name};
}

sub
check_grep
{
    my ($name, $args, $r_clist, $r_config) = @_;

    my @arglist = split (/ +/o, $args);
    my $tag = $arglist[0];
    my $pat= $arglist[1];
    my $fn = $arglist[2];
    $name = "_grep_${tag}";
    my $locnm = "_grep_${tag}";

    printlabel $name, "grep: $tag";
    if (checkcache ($name, $r_clist, $r_config) == 0)
    {
        return;
    }

    setlist $r_clist, $name;
    $r_config->{$name} = 0;
    my $rc = system ("grep '${pat}' $fn > /dev/null 2>&1");
    if ($rc == 0) {
      $r_config->{$name} = 1;
    }

    printyesno_val $name, $r_config->{$name};
}

sub
check_ifoption
{
    my ($ifcount, $type, $name, $opt, $r_clist, $r_config) = @_;

    printlabel $name, "$type ($ifcount): $opt";

    loadoptions ();

    my $trc = 0;

    my $found = 'F';
    if ($optionsloaded && defined ($optionshash{$opt})) {
      $found = 'T';
      $trc = lc $optionshash{$opt};
      print LOGFH "##  found: $opt => $trc\n";
      if ($trc eq 't') { $trc = 1; }
      if ($trc eq 'enable') { $trc = 1; }
      if ($trc eq 'f') { $trc = 0; }
      if ($trc eq 'disable') { $trc = 0; }
      if ($trc eq 'true') { $trc = 1; }
      if ($trc eq 'false') { $trc = 0; }
      if ($trc eq 'yes') { $trc = 1; }
      if ($trc eq 'no') { $trc = 0; }
    }

    if (! $optionsloaded) {
      $trc = 0;
    } elsif ($found eq 'F') {
      $trc = 0;
    }

    if ($type eq 'ifnotoption') {
      $trc = $trc == 0 ? 1 : 0;
    }

    if (! $optionsloaded) {
      printyesno_actual $name, "no options file";
    } elsif ($found eq 'F') {
      printyesno_actual $name, "option not found";
    } else {
      printyesno $name, $trc;
    }
    return $trc;
}

sub
check_if
{
    my ($iflabel, $ifcount, $ifline, $r_clist, $r_config) = @_;

    my $name = "_if_$iflabel";
    my $trc = 0;
    # make parseable
    $ifline =~ s/!/ ! /og;
    $ifline =~ s/\(/ ( /og;
    $ifline =~ s/\)/ ) /og;
    $ifline =~ s/&&/ \&\& /og;
    $ifline =~ s/ and / && /og;
    $ifline =~ s/ -a / && /og;
    $ifline =~ s/\|\|/ || /og;
    $ifline =~ s/ or / || /og;
    $ifline =~ s/ -o / || /og;
    $ifline =~ s/ not / ! /og;
    $ifline =~ s/ +/ /og;
    $ifline =~ s/^ *//og;
    $ifline =~ s/ *$//og;

    printlabel $name, "if ($ifcount): $iflabel";
    my $nline = '';
    print LOGFH "## ifline: $ifline\n";
    my $ineq = 0;
    my $val;
    my $quoted = 0;
    my $qtoken;
    foreach my $token (split (/\s/, $ifline)) {
      if ($token =~ /^'.*'$/o) {
        print LOGFH "## start/end qtoken\n";
        $token =~ s/'//go;
      } elsif ($token =~ /^'/o) {
        $qtoken = $token;
        $quoted = 1;
        print LOGFH "## start qtoken\n";
        next;
      }
      if ($quoted) {
        if ($token =~ /'$/o) {
          $token = $qtoken . ' ' . $token;
          $token =~ s/'//go;
          print LOGFH "## end qtoken\n";
          $quoted = 0;
        } else {
          $qtoken = $qtoken . ' ' . $token;
          print LOGFH "## in qtoken\n";
          next;
        }
      }

      if ($ineq == 1) {
        $ineq = 2;
        print LOGFH "## value of $token(B): " . $r_config->{$token} . "\n";
        $val = $r_config->{$token};
      } elsif ($ineq == 2) {
        print LOGFH "## end ==\n";
        $ineq = 0;
        $nline .= "('$val' eq '$token') ? 1 : 0";
        $nline .= ' ';
      } elsif ($token eq '==') {
        print LOGFH "## begin ==\n";
        $ineq = 1;
      } elsif ($token eq '(' || $token eq ')' || $token eq '&&' ||
          $token eq '||' || $token eq '!') {
        $nline .= $token . ' ';
      } else {
        print LOGFH "## value of $token: " . $r_config->{$token} . "\n";
        $nline .= $r_config->{$token} ne '0' &&
              $r_config->{$token} ne '' ? 1 : 0;
        $nline .= ' ';
      }
    }
    print LOGFH "## nline: $nline\n";
    $trc = eval $nline;
    printyesno $name, $trc;
    return $trc;
}

sub
check_set
{
    my ($name, $type, $val, $r_clist, $r_config) = @_;

    my $tnm = $name;
    $tnm =~ s/^_setint_//;
    $tnm =~ s/^_setstr_//;
    printlabel $name, "${type}: $tnm";

    if ($type eq 'set') {
      if (defined ($r_config->{$name})) {
        setlist $r_clist, $name;
        $r_config->{$name} = $val;
        printyesno $name, $r_config->{$name};
      } else {
        printyesno_actual $name, 'no such variable';
      }
    } elsif ($type eq 'setint') {
      setlist $r_clist, $name;
      $r_config->{$name} = $val;
      printyesno_actual $name, $r_config->{$name};
    } else {
      setlist $r_clist, $name;
      $r_config->{$name} = $val;
      printyesno_actual $name, $r_config->{$name};
    }
}

sub
check_option
{
    my ($name, $onm, $def, $r_clist, $r_config) = @_;

    printlabel $name, "option: $onm";

    loadoptions ();
    my $oval = $def;

    if ($optionsloaded && defined ($optionshash{$onm})) {
      $oval = $optionshash{$onm};
      print LOGFH "##  found: $onm => $oval\n";
    }

    setlist $r_clist, $name;
    $r_config->{$name} = $oval;
    printyesno_actual $name, $r_config->{$name};
}

sub
check_include_conflict
{
    my ($name, $h1, $h2, $r_clist, $r_config) = @_;

    my $i1 = $h1;
    $i1 =~ s,/,_,go;
    $i1 =~ s,\.h$,,o;
    $i1 =~ s,:,,go;
    if ($i1 =~ m#^sys#o) {
      $i1 = "_${i1}";
    } else {
      $i1 = "_hdr_${i1}";
    }
    my $i2 = $h2;
    $i2 =~ s,/,_,go;
    $i2 =~ s,\.h$,,o;
    $i2 =~ s,:,,go;
    if ($i2 =~ m#^sys#o) {
      $i2 = "_${i2}";
    } else {
      $i2 = "_hdr_${i2}";
    }

    $name = "${name}_${i1}_${i2}";
    printlabel $name, "header: include both $h1 & $h2";

    if (defined ($r_config->{${i1}}) &&
        $r_config->{${i1}} ne '0' &&
        defined ($r_config->{${i2}}) &&
        $r_config->{${i2}} ne '0')
    {
        if (checkcache ($name, $r_clist, $r_config) == 0)
        {
            return;
        }

        my $code = <<"_HERE_";
#include <$h1>
#include <$h2>
int main () { return 0; }
_HERE_
        do_chk_compile ($name, $code, 'std', $r_clist, $r_config);
    } else {
        setlist $r_clist, $name;
        $r_config->{$name} = 1;
        printyesno $name, $r_config->{$name};
    }
}

sub
check_npt
{
    my ($name, $proto, $req, $r_clist, $r_config) = @_;

    printlabel $name, "need prototype: $proto";
    if (checkcache ($name, $r_clist, $r_config) == 0)
    {
        return;
    }

    if (defined ($r_config->{$req}) && $r_config->{$req} eq '0')
    {
      $r_config->{$name} = 0;
      setlist $r_clist, $name;
      printyesno $name, $r_config->{$name};
      return;
    }

    my $code = <<"_HERE_";
CPP_EXTERNS_BEG
struct _TEST_struct { int _TEST_member; };
extern struct _TEST_struct* $proto _((struct _TEST_struct*));
CPP_EXTERNS_END
_HERE_
    do_chk_compile ($name, $code, 'all', $r_clist, $r_config);
}

sub
check_type
{
    my ($name, $type, $r_clist, $r_config) = @_;

    $type =~ s/star/*/og;
    printlabel $name, "type: $type";
    if (checkcache ($name, $r_clist, $r_config) == 0)
    {
        return;
    }

    my $code = <<"_HERE_";
struct xxx { $type mem; };
static struct xxx v;
struct xxx* f() { return &v; }
int main () { struct xxx *tmp; tmp = f(); return (0); }
_HERE_
    do_chk_compile ($name, $code, 'all', $r_clist, $r_config);
}

sub
check_defined
{
    my ($name, $def, $r_clist, $r_config) = @_;

    printlabel $name, "defined: $def";
    if (checkcache ($name, $r_clist, $r_config) == 0)
    {
        return;
    }

    setlist $r_clist, $name;
    my $code = <<"_HERE_";
int main () {
#ifdef ${def}
return (0);
#else
return (1);
#endif
}
_HERE_
    my $val = 0;
    my $rc = _chk_run ($name, $code, \$val, $r_clist, $r_config, {});
    $r_config->{$name} = $rc == 0 ? 1 : 0;
    printyesno $name, $r_config->{$name};
}

sub
check_param_void_star
{
    my ($name, $r_clist, $r_config) = @_;

    printlabel $name, "parameter: void *";
    if (checkcache ($name, $r_clist, $r_config) == 0)
    {
        return;
    }

    my $code = <<"_HERE_";
char *
tparamvs (ptr)
  void *ptr;
{
  ptr = (void *) NULL;
  return (char *) ptr;
}
_HERE_
    do_chk_compile ($name, $code, 'all', $r_clist, $r_config);
}

sub
check_printf_long_double
{
  my ($name, $r_clist, $r_config) = @_;

  printlabel $name, "printf: long double printable";
  if (checkcache ($name, $r_clist, $r_config) == 0) {
    return;
  }

  setlist $r_clist, $name;
  my $code = << "_HERE_";
int main (int argc, char *argv[]) {
long double a;
long double b;
char t[40];
a = 1.0;
b = 2.0;
a = a / b;
sprintf (t, \"%.1Lf\", a);
if (strcmp(t,\"0.5\") == 0) {
return (0);
}
return (1);
}
_HERE_

  my $val = 0;
  my %a = (
       'incheaders' => 'all',
       );
  my $rc = _chk_run ($name, $code, \$val, $r_clist, $r_config, \%a);
  $r_config->{$name} = $rc == 0 ? 1 : 0;
  printyesno $name, $r_config->{$name};
}


sub
check_lib
{
    my ($name, $func, $r_clist, $r_config, $r_a) = @_;

    setlist $r_clist, $name;
    my $val = $r_a->{'otherlibs'} || '';

    $rfunc = $func;
    $rfunc =~ s/_dollar_/\$/g;
    if ($val ne '')
    {
        printlabel $name, "function: $rfunc [$val]";
    }
    else
    {
        printlabel $name, "function: $rfunc";
        if (checkcache ($name, $r_clist, $r_config) == 0)
        {
            return;
        }
    }

    $r_config->{$name} = 0;
    # unfortunately, this does not work if the function
    # is not declared.
    my $hinc = 'all';
    my $code = '';
    if ($ENV{'_MKCONFIG_USING_CPLUSPLUS'} eq 'Y') {
      $hinc = 'none';
      $code = <<"_HERE_";
CPP_EXTERNS_BEG
#undef $rfunc
typedef char (*_TEST_fun_)();
char $rfunc();
_TEST_fun_ f = $rfunc;
CPP_EXTERNS_END
int main () {
if (f == $rfunc) { return 0; }
return 1;
}
_HERE_
    } else {
      $hinc = 'all';
      $code = <<"_HERE_";
CPP_EXTERNS_BEG
typedef char (*_TEST_fun_)();
_TEST_fun_ f = $rfunc;
CPP_EXTERNS_END
int main () {
f(); if (f == $rfunc) { return 0; }
return 1;
}
_HERE_
    }

    my %a = (
         'incheaders' => $hinc,
         'otherlibs' => $val,
         );
    my $rc = _chk_link ($name, $code, $r_clist, $r_config, \%a);
    my $tag = '';
    if ($rc == 0)
    {
      $r_config->{$name} = 1;
      if ($a{'dlibs'} ne '')
      {
          $tag = " with $a{'dlibs'}";
          $r_config->{"lib_$name"} = $a{'dlibs'};
          setclist $r_clist, "lib_$name";
      }
    }
    printyesno $name, $r_config->{$name}, $tag;
}

sub
check_class
{
    my ($name, $class, $r_clist, $r_config, $r_a) = @_;

    setlist $r_clist, $name;
    my $val = $r_a->{'otherlibs'} || '';

    if ($val ne '')
    {
        printlabel $name, "class: $class [$val]";
    }
    else
    {
        printlabel $name, "class: $class";
        if (checkcache ($name, $r_clist, $r_config) == 0)
        {
            return;
        }
    }

    $r_config->{$name} = 0;
    my $code = <<"_HERE_";
int main () { $class testclass; }
_HERE_

    my %a = (
         'incheaders' => 'all',
         'otherlibs' => $val,
         );
    my $rc = _chk_link ($name, $code, $r_clist, $r_config, \%a);
    my $tag = '';
    if ($rc == 0)
    {
      $r_config->{$name} = 1;
      if ($a{'dlibs'} ne '')
      {
          $tag = " with $a{'dlibs'}";
          $r_config->{"lib_$name"} = $a{'dlibs'};
          setlist $r_clist, "lib_$name";
      }
    }
    printyesno $name, $r_config->{$name}, $tag;
}

sub
check_args
{
    my ($name, $funcnm, $r_a, $r_clist, $r_config) = @_;

    printlabel $name, "args: $funcnm";
    # no cache

    if ($ENV{'_MKCONFIG_USING_GCC'} eq 'N' &&
        $ENV{'_MKCONFIG_SYSTYPE'} eq 'HP-UX' ) {
      my $tcc = `$ENV{'CC'} -v 2>&1`;
      if ($tcc =~ /Bundled/o) {
        print " bundled cc; skipped";
        return;
      }
    }

    my $oldprecc = $precc;
    $precc .= "/* get rid of most gcc-isms */
#define __asm(a)
#define __asm__(a)
#define __attribute__(a)
#define __nonnull__(a,b)
#define __restrict
#define __restrict__
#if defined(__THROW)
# undef __THROW
#endif
#define __THROW
#define __const const
";

   my $rc = _chk_cpp ($name, "", $r_clist, $r_config,
        { 'incheaders' => 'all', });
   $precc = $oldprecc;
   my $ccount = 0;

   if ($rc == 0) {
     my $cmd = "egrep \"[	 *]" . ${funcnm} . "[	 ]*\\(\" $name.out >/dev/null 2>&1";
     $rc = system ($cmd);
     print LOGFH "##  args: $cmd $rc\n";
     if ($rc == 0) {
       my $dcl = `${awkcmd} -f ${_MKCONFIG_DIR}/util/mkcextdcl.awk ${name}.out ${funcnm}`;
       # $dcl may be multi-line...fix this now.
       $dcl =~ s/[ 	\n\r]/ /gos;
       $dcl =~ s/extern *//o;
       $dcl =~ s/;//o;
       print LOGFH "##  dcl(A): $dcl\n";
       $dcl =~ s/\( *void *\)/()/o;
       print LOGFH "##  dcl(C): $dcl\n";

       my $c = $dcl;
       $c =~ s/[^,]*//go;
       $ccount = length ($c);
       $ccount += 1;

       $c = $dcl;
       $c =~ s/^[^\(]*\(//o;
       $c =~ s/\)[^\)]*$//o;
       print LOGFH "## c(E): ${c}\n";
       my $val = 1;
       while (${c} ne "") {
         my $tc = $c;
         $tc =~ s/ *,.*$//o;
         $tc =~ s/[ 	]+/ /go;
         if ($r_a->{'noconst'} eq 'T') {
           $tc =~ s/const *//o;
         }
         $tc =~ s/^ *//;
         $tc =~ s/ *$//;
         # only do the following if the names of the variables are declared
         $tc =~ s/(struct|union|enum) /$1#/;
         if ($tc =~ / /o) {
           $tc =~ s/ *[A-Za-z0-9_]*$//o;
         }
         $tc =~ s/(struct|union|enum)#/$1 /;
         print LOGFH "## tc(F): ${tc}\n";
         my $nm = "_c_arg_${val}_${funcnm}";
         setlist $r_clist, $nm;
         $r_config->{$nm} = $tc;
         $val += 1;
         $c =~ s/^[^,]*//o;
         $c =~ s/^[	 ,]*//o;
         print LOGFH "## c(G): ${c}\n";
       }
       $c = $dcl;
       $c =~ s/[ 	]+/ /go;
       $c =~ s/([ \*])${funcnm}[ \(].*/$1/;
       $c =~ s/^ *//o;
       $c =~ s/ *$//o;
       if ($r_a->{'noconst'}) {
         $c =~ s/const *//;
       }
       $nm = "_c_type_${funcnm}";
       setlist $r_clist, $nm;
       $r_config->{$nm} = $c;
     }
   }

   setlist $r_clist, $name;
   $r_config->{$name} = $ccount;
   printyesno_val $name, $r_config->{$name};
}

sub
check_size
{
    my ($name, $type, $r_clist, $r_config) = @_;

    printlabel $name, "sizeof: $type";
    if (checkcache ($name, $r_clist, $r_config) == 0)
    {
        return;
    }

    setlist $r_clist, $name;
    $r_config->{$name} = 0;
    my $code = <<"_HERE_";
int main () {
	printf("%u\\n", sizeof($type));
    return (0);
    }
_HERE_
    my $val = 0;
    my $rc = _chk_run ($name, $code, \$val, $r_clist, $r_config, {});
    if ($rc == 0)
    {
        $val =~ s/\r//gos;
        $r_config->{$name} = $val;
    }
    printyesno_val $name, $r_config->{$name};
}

sub
check_member
{
    my ($name, $struct, $member, $r_clist, $r_config) = @_;

    printlabel $name, "exists: $struct.$member";
    if (checkcache ($name, $r_clist, $r_config) == 0)
    {
        return;
    }

    setlist $r_clist, $name;
    $r_config->{$name} = 0;
    my $code = <<"_HERE_";
int main () { $struct s; int i; i = sizeof (s.$member); }
_HERE_
    my $rc = _chk_compile ($name, $code, $r_clist, $r_config,
            { 'incheaders' => 'all', });
    if ($rc == 0)
    {
        $r_config->{$name} = 1;
    }
    printyesno $name, $r_config->{$name};
}

sub
check_memberxdr
{
    my ($name, $struct, $member, $r_clist, $r_config) = @_;

    printlabel $name, "member:xdr: $struct.$member";
    # no cache

    $r_config->{$name} = 0;
    my $rc = _chk_cpp ($name, "", $r_clist, $r_config,
        { 'incheaders' => 'all', });
    if ($rc == 0) {
      print LOGFH `pwd` . "\n";
      print LOGFH "## ${awkcmd} -f ${_MKCONFIG_DIR}/util/mkcextstruct.awk ${name}.out ${struct}\n";
      my $st = `${awkcmd} -f ${_MKCONFIG_DIR}/util/mkcextstruct.awk ${name}.out ${struct}`;
      print LOGFH "##  xdr(A): $st\n";
      if ($st =~ m/${member}\s*;/s) {
        my $mtype = $st;
        $mtype =~ s/\s*${member}\s*;.*//s;
        $mtype =~ s/.*\s//os;
        print LOGFH "##  xdr(B): $mtype\n";
        setlist $r_clist, "xdr_${member}";
        $r_config->{"xdr_${member}"} = "xdr_${mtype}";
        $r_config->{$name} = 1;
      }
    }
    setlist $r_clist, $name;
    printyesno $name, $r_config->{$name};
}

sub
check_int_declare
{
    my ($name, $function, $r_clist, $r_config) = @_;

    printlabel $name, "declared: $function";
    if (checkcache ($name, $r_clist, $r_config) == 0)
    {
        return;
    }

    setlist $r_clist, $name;
    $r_config->{$name} = 0;
    my $code = <<"_HERE_";
    int main () { int x; x = $function; }
_HERE_
    my $rc = _chk_compile ($name, $code, $r_clist, $r_config,
            { 'incheaders' => 'all', });
    if ($rc == 0)
    {
        $r_config->{$name} = 1;
    }
    printyesno $name, $r_config->{$name};
}

sub
check_ptr_declare
{
    my ($name, $function, $r_clist, $r_config) = @_;

    printlabel $name, "declared: $function";
    if (checkcache ($name, $r_clist, $r_config) == 0)
    {
        return;
    }

    setlist $r_clist, $name;
    $r_config->{$name} = 0;
    my $code = <<"_HERE_";
int main () { void *x; x = $function; }
_HERE_
    my $rc = _chk_compile ($name, $code, $r_clist, $r_config,
            { 'incheaders' => 'all', });
    if ($rc == 0)
    {
        $r_config->{$name} = 1;
    }
    printyesno $name, $r_config->{$name};
}

sub
check_standard
{
  my ($r_clist, $r_config) = @_;
  # FreeBSD has buggy headers, requires sys/param.h as a required include.
  # always check for these headers.
  my @headlist1 = (
      [ "_hdr_stdio", "stdio.h", ],
      [ "_hdr_stdlib", "stdlib.h", ],
      [ "_sys_types", "sys/types.h", ],
      [ "_sys_param", "sys/param.h", ],
      );

  foreach my $r_arr (@headlist1)
  {
      check_header ($$r_arr[0], $$r_arr[1], $r_clist, $r_config,
              { 'reqhdr' => [], });
  }
  check_keyword ('_key_void', 'void', $r_clist, $r_config);
  check_keyword ('_key_const', 'const', $r_clist, $r_config);
  check_param_void_star ('_param_void_star', $r_clist, $r_config);
  check_proto ('_proto_stdc', $r_clist, $r_config);
}


sub
create_output
{
  my ($r_clist, $r_config, $include, $configfile) = @_;

  if ($CONFH eq 'none') { return; }

  my $dt=`date`;
  chomp $dt;
  open (CCOFH, ">$CONFH");
  print CCOFH <<"_HERE_";
/* Created on: ${dt}
    From: ${configfile}
    Using: mkconfig-${_MKCONFIG_VERSION} (perl) */

#ifndef MKC_INC_${CONFHTAGUC}_H
#define MKC_INC_${CONFHTAGUC}_H 1

_HERE_

  foreach my $val (@{$r_clist->{'vars'}})
  {
    if ($val =~ m#^lib__lib_#o) {
      next;
    }
    my $tval = 0;
    if ($r_config->{$val} ne "0") {
        $tval = 1;
    }
    if ($val =~ m#^_setint_#o) {
      $tnm = $val;
      $tnm =~ s/^_setint_//;
      print CCOFH "#define $tnm " . $r_config->{$val} . "\n";
    } elsif ($val =~ m#^(_setstr_|_opt_|_cmd_loc_)#o) {
      $tnm = $val;
      $tnm =~ s/^_setstr_//;
      $tnm =~ s/^_opt_//;
      print CCOFH "#define $tnm \"" . $r_config->{$val} . "\"\n";
    } elsif ($val =~ m#^(_hdr|_sys|_command)#o) {
      print CCOFH "#define $val $tval\n";
    } else {
      print CCOFH "#define $val " . $r_config->{$val} . "\n";
    }
  }

  # standard tail -- always needed; non specific
  print CCOFH <<'_HERE_';

#ifndef MKC_STANDARD_DEFS
# define MKC_STANDARD_DEFS 1
# if ! _key_void
#  define void int
# endif
# if ! _key_void || ! _param_void_star
   typedef char *_pvoid;
# else
   typedef void *_pvoid;
# endif
# if ! _key_const
#  define const
# endif

# ifndef _
#  if _proto_stdc
#   define _(args) args
#  else
#   define _(args) ()
#  endif
# endif
#endif /* MKC_STANDARD_DEFS */

_HERE_

  print CCOFH $include;

  print CCOFH <<"_HERE_";

#endif /* MKC_INC_${CONFHTAGUC}_H */
_HERE_
  close CCOFH;
}

sub
main_process
{
    my ($configfile) = @_;
    my (%clist, %config);

    $clist{'clist'} = ();
    $clist{'chash'} = {};
    $clist{'vars'} = ();
    $clist{'vhash'} = {};
    $config{'reqlibs'} = {};
    $config{'reqlibs_list'} = ();

    if (-f $CACHEFILE) {
      open (MKCC, "<$CACHEFILE");
      while (my $line = <MKCC>) {
        chomp $line;
        if ($line =~ m/^mkc_c_(.*)=\$?'?(.*)'?/o) {
          my $name = $1;
          my $val = $2;
          $config{$name} = $val;
          push @{$clist{'clist'}}, $name;
          $clist{'chash'}->{$name} = 1;
        }
      }
      close (MKCC);
    }

    my $tconfigfile = $configfile;
    if ($configfile !~ m#^/#) {
      $tconfigfile = "../../$configfile";
    }
    if (! open (DATAIN, "<$tconfigfile"))
    {
        print STDOUT "$configfile: $!\n";
        exit 1;
    }

    my $linenumber = 0;
    my $inheaders = 1;
    my $ininclude = 0;
    my @doproclist;
    my $inproc = 0;
    my $doproc = 1;
    my $include = '';
    my $tline = '';
    my $ifstmtcount = 0;
    my $ifcurrlvl = 0;
    my $doif = $ifcurrlvl;
    while (my $line = <DATAIN>)
    {
      chomp $line;
      ++$linenumber;

      if ($ininclude == 0 && ($line =~ /^#/o || $line eq ''))
      {
          next;
      }

      if ($tline ne '') {
        $line = $tline . ' ' . $line;
        $tline = '';
      }
      if ($line =~ /[^\\]\\$/o) {
        $tline = $line;
        $tline =~ s/\\$//o;
        next;
      }

      if ($ininclude == 1 && $line =~ m#^\s*endinclude$#o)
      {
          print LOGFH "end include\n";
          $ininclude = 0;
          next;
      }
      elsif ($ininclude == 1)
      {
          $line =~ s,\\(.),$1,g;
          $include .= $line . "\n";
          next;
      }

      if ($inheaders && $line !~ m#^\s*(hdr|sys)#o)
      {
          $inheaders = 0;
      }

      print LOGFH "#### ${linenumber}: ${line}\n";

      if ($line =~ m#^\s*else#o)
      {
        if ($ifcurrlvl == $doif) {
          $doproc = $doproc == 0 ? 1 : 0;
          $iflevels =~ s/.\d+\s$//;
          $iflevels .= "-$ifstmtcount ";
        }
      }
      elsif ($line =~ m#^\s*if\s#o) {
        if ($doproc == 0) {
          $ifcurrlvl += 1;
        }
      }
      elsif ($line =~ m#^\s*endif#o)
      {
        if ($ifcurrlvl == $doif) {
          if ($#doproclist >= 0) {
            print LOGFH "## endif: doproclist: " . join (' ', @doproclist) . "\n";
            $doproc = shift @doproclist;
            print LOGFH "## endif: doproclist now : " . join (' ', @doproclist) . "\n";
            print LOGFH "## endif: doproc: $doproc\n";
            $iflevels =~ s/.\d+\s$//;
          } else {
            $doproc = 1;
            $iflevels = '';
          }
          $doif -= 1;
        }
        $ifcurrlvl -= 1;
      }

      if ($doproc == 1) {
        if ($line =~ m#^\s*output\s+([^\s]+)#o)
        {
            if ($inproc == 1) {
              savevars (\%clist);
              create_output (\%clist, \%config, $include, $configfile);
              $CONFH = 'none';
              $CONFHTAG = 'none';
              $CONFHTAGUC = 'NONE';
              $include = '';
            }

            print "output-file: $1\n";
            my $tconfh = $1;
            if ($tconfh =~ m#^/#o) {
              $CONFH = $tconfh;
            } else {
              $CONFH = "../../$tconfh";
            }
            $CONFHTAG = $tconfh;
            $CONFHTAG =~ s,.*/,,;
            $CONFHTAG =~ s,\..*$,,;
            $CONFHTAGUC = uc $CONFHTAG;
            print LOGFH "config file: $CONFH\n";
            $inproc = 1;
            $VARSFILE = "../../${MKC_FILES}/mkc_${CONFHTAG}_c.vars";
            $clist{'vars'} = ();
            $clist{'vhash'} = {};
        }
        elsif ($line =~ m#^\s*option\-file\s+([^\s]+)#o)
        {
            print "option-file: $1\n";
            my $tfile = $1;
            if ($tfile =~ m#^/#o) {
              $OPTIONFILE = $tfile;
            } else {
              $OPTIONFILE = "../../$tfile";
            }
            print LOGFH "options file: $OPTIONFILE\n";
        }
        elsif ($line =~ m#^\s*standard#o)
        {
            check_standard (\%clist, \%config);
        }
        elsif ($line =~ m#^\s*loadunit#o)
        {
            ;
        }
        elsif ($line =~ m#^\s*args\s+(.*)$#o)
        {
            my $funcnm = $1;

            %args = ( 'noconst' => 'F' );
            if ($funcnm =~ /^noconst */o) {
              $funcnm =~ s/^noconst *//o;
              $args{'noconst'} = 'T';
            }

            my $nm = "_args_${funcnm}";
            $nm =~ s,/,_,go;
            $nm =~ s,:,_,go;
            check_args ($nm, $funcnm, \%args, \%clist, \%config);
        }
        elsif ($line =~ m#^\s*include_conflict\s+([^\s]+)\s+([^\s]+)#o)
        {
            my $i1 = $1;
            my $i2 = $2;
            check_include_conflict ('_inc_conflict', $i1, $i2, \%clist, \%config);
        }
        elsif ($line =~ m#^\s*rquota_xdr$#o)
        {
            check_rquota_xdr ('_rquota_xdr', \%clist, \%config);
        }
        elsif ($line =~ m#^\s*gqa_uid_xdr$#o)
        {
            check_gqa_uid_xdr ('_gqa_uid_xdr', \%clist, \%config);
        }
        elsif ($line =~ m#^\s*include$#o)
        {
            print LOGFH "start include\n";
            $ininclude = 1;
        }
        elsif ($line =~ m#^\s*(hdr|sys)\s+([^\s]+)\s*(.*)#o)
        {
            my $typ = $1;
            my $hdr = $2;
            my $reqhdr = $3;
            my $nm = "_${typ}_";
            # create the standard header name for config.h
            $nm .= $hdr;
            $nm =~ s,/,_,go;
            $nm =~ s,\.h$,,o;
            $nm =~ s,:,_,go;
            if ($typ eq 'sys') {
              $hdr = 'sys/' . $hdr;
            }
            $reqhdr =~ s/^\s*//o;
            $reqhdr =~ s/\s*$//o;
            my @oh = split (/\s+/, $reqhdr);
            check_header ($nm, $hdr, \%clist, \%config, { 'reqhdr' => \@oh, });
        }
        elsif ($line =~ m#^\s*const\s+([^\s]+)\s*(.*)#o)
        {
            my $tnm = $1;
            my $reqhdr = $2;
            my $nm = "_const_" . $tnm;
            $reqhdr =~ s/^\s*//o;
            $reqhdr =~ s/\s*$//o;
            my @oh = split (/\s+/, $reqhdr);
            check_constant ($nm, $tnm, \%clist, \%config,
                { 'reqhdr' => \@oh, });
        }
        elsif ($line =~ m#^\s*command\s+(.*)#o)
        {
            my $cmds = $1;
            check_command ('', $cmds, \%clist, \%config);
        }
        elsif ($line =~ m#^\s*grep\s+(.*)#o)
        {
            check_grep ('', $1, \%clist, \%config);
        }
        elsif ($line =~ m#^\s*(if(not)?option)\s+([^\s]+)#o)
        {
            my $type = $1;
            my $opt = $3;
            my $nm = "_${type}_${opt}";
            ++$ifstmtcount;
            my $rc = check_ifoption ($ifstmtcount, $type, $nm, $opt, \%clist, \%config);
            $iflevels .= "+$ifstmtcount ";
            unshift @doproclist, $doproc;
            $doproc = $rc;
            print LOGFH "## ifoption: doproclist: " . join (' ', @doproclist) . "\n";
            print LOGFH "## ifoption: doproc: $doproc\n";
        }
        elsif ($line =~ m#^\s*if\s+([^\s]+)\s+(.*)#o)
        {
            my $iflabel = $1;
            my $ifline = $2;
            ++$ifcurrlvl;
            ++$ifstmtcount;
            print LOGFH "## if: label: $iflabel count $ifstmtcount line: $ifline\n";
            my $rc = check_if ($iflabel, $ifstmtcount, $ifline, \%clist, \%config);
            $iflevels .= "+$ifstmtcount ";
            unshift @doproclist, $doproc;
            $doproc = $rc;
            $doif = $ifcurrlvl;
            print LOGFH "## if: doproclist: " . join (' ', @doproclist) . "\n";
            print LOGFH "## if: doproc: $doproc\n";
        }
        elsif ($line =~ m#^\s*(endif|else)#o)
        {
            ;
        }
        elsif ($line =~ m#^\s*echo\s+(.*)$#o)
        {
            print $1;
        }
        elsif ($line =~ m#^\s*exit$#o)
        {
            print $1;
        }
        elsif ($line =~ m#^\s*(set(int|str)?)\s+([^\s]+)\s*(.*)#o)
        {
            my $type = $1;
            my $nm = $3;
            if ($type eq 'setint' || $type eq 'setstr') {
              $nm = "_${type}_$3";
            }
            my $val = $4;
            check_set ($nm, $type, $val, \%clist, \%config);
        }
        elsif ($line =~ m#^\s*option\s+([^\s]+)\s*(.*)#o)
        {
            my $nm = "_opt_$1";
            my $onm = $1;
            my $def = $2;
            check_option ($nm, $onm, $def, \%clist, \%config);
        }
        elsif ($line =~ m#^\s*npt\s+([^\s]*)\s*(.*)#o)
        {
            my $func = $1;
            my $req = $2;
            my $nm = "_npt_" . $func;
            check_npt ($nm, $func, $req, \%clist, \%config);
        }
        elsif ($line =~ m#^\s*key\s+(.*)#o)
        {
            my $tnm = $1;
            my $nm = "_key_" . $tnm;
            if (! defined ($config{$nm}) ||
                $config{$nm} eq '0')
            {
                check_keyword ($nm, $tnm, \%clist, \%config);
            }
        }
        elsif ($line =~ m#^\s*class\s+([^\s]+)\s*(.*)?#o)
        {
            my $class = $1;
            my $libs = $2 || '';
            my $nm = "_class_" . $class;
            $nm =~ s,:,_,go;
            if (! defined ($config{$nm}) ||
                $config{$nm} eq '0')
            {
                check_class ($nm, $class, \%clist, \%config,
                       { 'otherlibs' => $libs, });
            }
        }
        elsif ($line =~ m#^\s*typ\s+(.*)#o)
        {
            my $tnm = $1;
            my $nm = "_typ_" . $tnm;
            $nm =~ s, ,_,go;
            if (! defined ($config{$nm}) ||
                $config{$nm} eq '0')
            {
                check_type ($nm, $tnm, \%clist, \%config);
            }
        }
        elsif ($line =~ m#^\s*define\s+(.*)#o)
        {
            my $tnm = $1;
            my $nm = "_define_" . $tnm;
            if (! defined ($config{$nm}) ||
                $config{$nm} eq '0')
            {
                check_defined ($nm, $tnm, \%clist, \%config);
            }
        }
        elsif ($line =~ m#^\s*lib\s+([^\s]+)\s*(.*)?#o)
        {
            my $func = $1;
            my $libs = $2 || '';
            my $nm = "_lib_" . $func;
            if (! defined ($config{$nm}) ||
                $config{$nm} eq '0')
            {
                check_lib ($nm, $func, \%clist, \%config,
                       { 'otherlibs' => $libs, });
            }
        }
        elsif ($line =~ m#^\s*dcl\s+([^\s]*)\s+(.*)#o)
        {
            my $type = $1;
            my $var = $2;
            my $nm = "_dcl_" . $var;
            if (! defined ($config{$nm}) ||
                $config{$nm} eq '0')
            {
                if ($type eq 'int')
                {
                    check_int_declare ($nm, $var, \%clist, \%config);
                }
                elsif ($type eq 'ptr')
                {
                    check_ptr_declare ($nm, $var, \%clist, \%config);
                }
            }
        }
        elsif ($line =~ m#^\s*printf_long_double#o) {
          check_printf_long_double ('_printf_long_double', \%clist, \%config);
        }
        elsif ($line =~ m#^\s*member\s+(.*)\s+([^\s]+)#o)
        {
            my $struct = $1;
            my $member = $2;
            my $nm = "_mem_" . $struct . '_' . $member;
            $nm =~ s/ /_/go;
            if (! defined ($config{$nm}) ||
                $config{$nm} eq '0')
            {
                check_member ($nm, $struct, $member, \%clist, \%config);
            }
        }
        elsif ($line =~ m#^\s*memberxdr\s+(.*)\s+([^\s]+)#o)
        {
            my $struct = $1;
            my $member = $2;
            my $nm = "_memberxdr_" . $struct . '_' . $member;
            $nm =~ s/ /_/go;
            if (! defined ($config{$nm}) ||
                $config{$nm} eq '0')
            {
                check_memberxdr ($nm, $struct, $member, \%clist, \%config);
            }
        }
        elsif ($line =~ m#^\s*size\s+(.*)#o)
        {
            my $typ = $1;
            $typ =~ s/\s*$//o;
            my $nm = "_siz_" . $typ;
            $nm =~ s, ,_,go;
            if (! defined ($config{$nm}) ||
                $config{$nm} eq '0')
            {
                check_size ($nm, $typ, \%clist, \%config);
            }
        }
        else
        {
            print LOGFH "unknown command: $line\n";
            print STDOUT "unknown command: $line\n";
        }
      }
    }

    savevars (\%clist);
    savecache (\%clist, \%config);
    create_output (\%clist, \%config, $include, $configfile);

    # This is here for os/2 and other systems w/no usable bourne shell.
    # it is not up to date, as it will output libraries that are not
    # needed by the final configuration.
    open (RLIBFH, ">$REQLIB");
    my $r_list = $config{'reqlibs_list'};
    print RLIBFH join (' ', @{$r_list}) . "\n";
    close RLIBFH;
}

sub
usage
{
  print STDOUT "Usage: $0 [-c <cache-file>] [-L <log-file>]\n";
  print STDOUT "       [-o <option-file>] [-C] <config-file>\n";
  print STDOUT "  -C : clear cache-file\n";
  print STDOUT "defaults:\n";
  print STDOUT "  <cache-file> : mkconfig.cache\n";
  print STDOUT "  <log-file>   : mkconfig.log\n";
  print STDOUT "  <option-file>: options.dat\n";
}

# main

my $clearcache = 0;
while ($#ARGV > 0)
{
  if ($ARGV[0] eq "-C")
  {
      shift @ARGV;
      $clearcache = 1;
  }
  if ($ARGV[0] eq "-c")
  {
      shift @ARGV;
      $CACHEFILE = $ARGV[0];
      shift @ARGV;
  }
  if ($ARGV[0] eq "-L")
  {
      shift @ARGV;
      $LOG = $ARGV[0];
      shift @ARGV;
  }
  if ($ARGV[0] eq "-o")
  {
      shift @ARGV;
      $OPTIONFILE = $ARGV[0];
      shift @ARGV;
  }
}

my $configfile = $ARGV[0];
if (! defined ($configfile) || ! -f $configfile)
{
  usage;
  exit 1;
}
if (-d $_MKCONFIG_TMP && $_MKCONFIG_TMP ne "mkc_files/_tmp_mkconfig")
{
  usage;
  exit 1;
}

delete $ENV{'CDPATH'};
delete $ENV{'GREP_OPTIONS'};
delete $ENV{'ENV'};
$ENV{'LC_ALL'} = "C";

my $tpath = $0;
$tpath =~ s,/[^/]*$,,;
my $currdir =`pwd`;
chomp $currdir;
if (! chdir $tpath) {
  die ("Unable to cd to $tpath. $!\n");
}
$_MKCONFIG_VERSION=`cat $tpath/VERSION`;
chomp $_MKCONFIG_VERSION;
$_MKCONFIG_DIR = `pwd`;
chomp $_MKCONFIG_DIR;
if (! chdir $currdir) {
  die ("Unable to cd to $currdir. $!\n");
}

if (-d $_MKCONFIG_TMP) { system ("rm -rf $_MKCONFIG_TMP"); }
mkdir $_MKCONFIG_TMP, 0777;
if (! chdir $_MKCONFIG_TMP) {
  die ("Unable to cd to $_MKCONFIG_TMP. $!\n");
}

if ($clearcache)
{
  unlink $CACHEFILE;
  unlink $VARSFILE;
}

print STDOUT "$0 using $configfile\n";
unlink $LOG;
open (LOGFH, ">>$LOG");
$ENV{'CFLAGS'} = $ENV{'CFLAGS'};
foreach my $nm ('CC', 'CFLAGS_OPTIMIZE', 'CFLAGS_DEBUG', 'CFLAGS_INCLUDE',
      'CFLAGS_USER', 'CFLAGS_APPLICATION', 'CFLAGS_COMPILER', 'CFLAGS_SYSTEM',
      'CFLAGS_SHARED', 'CFLAGS_SHARED_USER', 'LDFLAGS_OPTIMIZE',
      'LDFLAGS_DEBUG', 'LDFLAGS_USER', 'LDFLAGS_APPLICATION',
      'LDFLAGS_COMPILER', 'LDFLAGS_SYSTEM', 'LDFLAGS_SHARED',
      'LDFLAGS_SHARED_LIBLINK', 'LDFLAGS_LIBS_USER',
      'LDFLAGS_LIBS_APPLICATION', 'LDFLAGS_LIBS_SYSTEM') {
  print LOGFH "$nm: $ENV{$nm}\n";
}
print LOGFH "awk: $awkcmd\n";

main_process $configfile;

close LOGFH;

if ($ENV{'MKC_KEEP_TMP'} eq "") {
  if (-d $_MKCONFIG_TMP) { system ("rm -rf $_MKCONFIG_TMP"); }
}
exit 0;
