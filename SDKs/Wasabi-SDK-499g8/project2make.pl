#!/usr/bin/perl

use strict;

if ( !$ARGV[0] || ! -f $ARGV[0] ) {
  die( "$0 <workspace file>\n" );
}

my $workspace = parseWorkspace( $ARGV[0] );

printMakefile( $workspace );

sub printMakefile {
  my ( $work ) = @_;

  my ( $project, $objdir, $i, @objs, $out );
  print <<EOF;
##########################################
#
# Include the controls
#
##########################################

EOF

  print "include Makefile.inc\n\n";

  print <<EOF;
##########################################
#
# Projects
#
##########################################

EOF

  foreach ( sort keys %{$work} ) {
    print "$_: $work->{$_}->{out}\n\n";
  }

  print <<EOF;
##########################################
#
# Directory Setup
#
##########################################

EOF

  print "\$(BUILDDIR)/setup:\n";
  foreach ( sort keys %{$work} ) {
    print "\tmkdir -p \$(BUILDDIR)/objs/$_\n";
  }
  print "\tmkdir -p \$(BUILDDIR)/Wacs\n";
  print "\tln -s \$(ROOT)/studio/Wacs/xml \$(BUILDDIR)/Wacs/xml\n";
  print "\tmkdir -p \$(BUILDDIR)/libs\n";
  print "\trm -f \$(BUILDDIR)/Skins\n";
  print "\tln -s \$(ROOT)/studio/Skins \$(BUILDDIR)/Skins\n";
  print "\trm -f \$(BUILDDIR)/Scripts\n";
  print "\tln -s \$(ROOT)/studio/Scripts \$(BUILDDIR)/Scripts\n";
  print "\trm -f \$(BUILDDIR)/Locales\n";
  print "\tln -s \$(ROOT)/studio/Locales \$(BUILDDIR)/Locales\n";
  print "\ttouch \$(BUILDDIR)/setup\n\n";

  print <<EOF;
##########################################
#
# Object rules
#
##########################################

EOF

  foreach ( sort keys %{$work} ) {
    $project = $work->{$_};
    print "${_}_objs=";
    $objdir = "\$(BUILDDIR)/objs/$_/";
    @objs = ();
    if ( $project->{rcobj} ) {
      $project->{rcobj} =~ s,.*/,$objdir,;
    }
    if ( ref( $project->{src} ) ) {
      @{$project->{src}} = uniq( @{$project->{src}} );
      @objs = @{$project->{src}};
      map { s,.*/,$objdir,; s/\.(c(pp)?|asm)$/.o/i } @objs;
      print join( " ", @objs );
    }

    print "\n\n";

    for ( $i = 0; $i < @objs; $i++ ) {
      print "$objs[$i]: $project->{src}->[$i]\n";
      if ( $project->{src}->[$i] =~ /cpp$/i ) {
	print "\t\$(CXX) -c \$(CXXFLAGS) \$(${_}_flags) -o \$@ \$<\n\n";
      } elsif ( $project->{src}->[$i] =~ /asm$/i ) {
	print "\t\$(NASM) \$(NASMFLAGS) \$(${_}_flags) -o \$@ \$<\n\n";
      } else {
	print "\t\$(CC) -c \$(CFLAGS) \$(${_}_flags) -o \$@ \$<\n\n";
      }
    }

    if ( $project->{rc} ) {
      print "$project->{rcobj}: $project->{rc}\n";
      print "\t./makerescode.pl $project->{rc} > $project->{rcc}\n";
      print "\t\$(CC) -c \$(CXXFLAGS) -o \$@ $project->{rcc}\n\n";
    }
  }

  print <<EOF;
##########################################
#
# Project rules
#
##########################################

EOF

  foreach ( sort keys %{$work} ) {
    my ( $deplibs );
    $project = $work->{$_};
    $out = $project->{out};
    $deplibs = "";
    if ( $project->{rcobj} ) {
      $deplibs = $project->{rcobj} . " ";
    }
    print "$out: \$(BUILDDIR)/setup \$(${_}_objs) ";
    if ( ref( $project->{dep} ) ) {
      $deplibs .= join( " ", map { $work->{$_}->{out} } grep {$work->{$_}->{out} !~ /\.wac$/} @{$project->{dep}} );
      print $deplibs;
    }
    print "\n";
    if ( $out =~ /\.wac/ ) {
      print "\t\$(ARSO) $out \$(${_}_objs) $deplibs \$(${_}_libs) \$(WACLIBARGS) \$(LIBARGS)\n";
    } elsif ( $out =~ /\.so/ ) {
      print "\t\$(ARSO) $out \$(${_}_objs) $deplibs \$(${_}_libs) \$(LIBARGS)\n";
    } elsif ( $out =~ /\.exe/ ) {
      print "\t\$(CXX) -o $out \$(${_}_objs) $deplibs \$(${_}_libs) \$(LIBARGS)\n";
    } else {
      print "\t\$(AR) $out \$(${_}_objs) \$(${_}_libs)\n";
      print "\t\$(RANLIB) $out\n";
	
    }
    print "\n";
  }
}

sub uniq {
  my %hash;
  for ( @_ ) { $hash{$_} = 1; }
  return keys %hash;
}

sub parseWorkspace {
  my ( $filename ) = @_;

  my ( $ret, $fh, $project, $file, $key );

  open TXT, "$filename";

  $ret = {};

  while ( <TXT> ) {
    if ( /^Project: "([^\"]+)"=("([^\"]+)"|[^ ]+)/ ) {
      $file = $2;
      $project = $1;
      $ret->{$project}->{file} = find_file( $file ) 
	or die "Cannot find file $file!\n";
    } elsif ( /Project_Dep_Name (\S+)/ ) {
      push @{$ret->{$project}->{dep}}, $1;
    }
  }

  foreach $key (keys %{$ret}) {
    $project = $ret->{$key};
    parseProject( $project );
    $project->{out} ||= "libs/$key.a";
    $project->{out} = "\$(BUILDDIR)/" . $project->{out};
  }

  return $ret;
}

sub parseProject {
  my ( $href ) = @_;

  my ( $path, $file, $fixed );

  ($path = $href->{file}) =~ s,/[^/]+$,,;

  open TXT, $href->{file};
  while ( <TXT> ) {
    if ( /^SOURCE=(.*\.(c(pp)?|asm))/i ) {
      $file = $path . "/" . $1;
      $fixed = find_file( $file )
	or die "Cannot find $file!\n";

      push @{$href->{src}}, $fixed;
    } elsif ( /^SOURCE=(.*\.rc)/i ) {
      $file = $path . "/" . $1;
      $fixed = find_file( $file )
	or die "Cannot find $file!\n";

      $href->{rc} = $fixed;
      ($href->{rcc} = $href->{rc}) =~ s/\.rc/-rc.c/i;
      ($href->{rcobj} = $href->{rc}) =~ s/\.rc/-rc.o/i;

    } elsif ( m,/out:"([^\"]+)", ) {
      $href->{out} = $1;
      $href->{out} =~ tr,\\,/,;
      $href->{out} =~ s,^\.\./studio/,,i;
      $href->{out} =~ s,^wacs/,Wacs/,i;
      $href->{out} =~ s,\.dll$,.so,i;
    }
  }
}

sub find_file {
  my ( $file ) = @_;

  my ( $found, $tofind, $try, $cur, $found );

  $file =~ tr,\\,/,;
  return $file if ( -f $file );

  return undef unless $file =~ s,^\./,,;

  $try = ".";
  $tofind = $file;
  $found = 1;
  while( $found && $tofind ) {
    $tofind =~ s,^([^/]+)/?,, or return undef;
    $cur = $1;
    opendir DIR, "$try";
    $found = 0;
    foreach ( readdir DIR ) {
      if ( /$cur/i ) {
	$try .= "/$_";
	$found = 1;
	last;
      }
    }
  }

  return $found?$try:undef;
}
