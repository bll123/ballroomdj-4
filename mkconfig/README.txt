mkconfig - configuration tool (version 2.6.3)

Website: https://mkconfig.sourceforge.io/
SourceForge: https://sourceforge.net/projects/mkconfig/

mkconfig is a build configuration utility. It creates an output file
intended to be included as a header file, run as a shell script, used
as a configuration file, or any other use. mkconfig is written in
portable shell script and designed to be extensible for any
configuration use.

It also includes tools that can be used to compile, link and build
libraries and executables using the shell configuration previously
built.

See: examples/helloworld for a simple example.

Another example of mkconfig in use can be seen in the
'di' program at https://sourceforge.net/projects/diskinfo-di/

mkconfig works with most any bourne shell compatible shell.  Modern
shells that are not bourne shell compatible are:
  posh, bosh, zsh (compatibility mode).
Very old shells do not have the memory capabilities needed in order to
save the large number of variables that mkconfig needs.

TESTING
    Version 2.4 has been tested on:
      Linux
	Debian 9 (gcc6) (bash4, dash)
        Fedora 7 (gcc) (bash3, sh/bash3)
        Fedora 27 (gcc7) (bash4, sh/bash4)
        MX Linux 19.2 (gcc8, clang) (ksh93 osh bash5 dash mksh yash)
      BSD
        DragonflyBSD 5.8.1 (gcc) (bash5, dash, pdksh, ksh93, mksh)
        FreeBSD 7.0 (gcc4) (sh)
        FreeBSD 11.0 (clang6) (sh, bash4, dash, pdksh, ksh93, mksh)
        FreeBSD 12.0 (clang6) (sh, bash4)
        NetBSD 9.0 (gcc) (pdksh, sh)
        OpenBSD 6.3 (clang5) (pdksh, sh)
      Windows
        Msys2 (gcc9, clang) (ash, bash4, dash, sh/bash4)
        Cygwin (gcc8, clang8) (ash, bash4, dash, mksh, sh/bash4)
      Other
        AIX 7.1 (xlc, gcc4) (bash4, ksh, ksh93, sh)
        AIX 7.2 (xlc, gcc7) (bash4, ksh, ksh93, sh)
        HP-UX 11.11 (gcc4) (sh, ksh, bash4)
        Mac OS X 10.15.5 (clang) (bash3, dash, ksh93, sh/bash3)
        QNX 6.5 (cc) (pdksh, bash4, sh/pdksh)
        SCO SV 6.0.0 (cc, gcc2) (sh, ksh, ksh93, bash3, bash2)
        Solaris 11/x86 (cc 12.3, gcc5) (bash4, ksh93, sh/ksh93)
        Solaris 10/sparc (cc 12.3, gcc5) (bash4, sh(1), sh, bash3, ksh)
        Solaris 9/x86 (cc 12, gcc4) (bash4, ksh93, sh(1), bash2, sh)
        Tru64 5.1b (cc, gcc) (sh, ksh, bash2)
        UnixWare 7.1.4 (cc, gcc2) (sh, ksh93, ksh88, bash2)
        (1) not a standard solaris shell

ISSUES
    - D Compiler bugs:
      ldc2 has structure size problems on 64bit (issue #28).
      gdc 4.6.3 (LinuxMint) has structure size problems on 64bit.
    - The D language portions have not been tested or used
      in a very long time.

