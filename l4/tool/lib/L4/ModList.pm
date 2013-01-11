
package L4::ModList;
use Exporter;
use vars qw(@ISA @EXPORT);
@ISA    = qw(Exporter);
@EXPORT = qw(get_module_entry search_file get_entries);

my @internal_searchpaths;

my $arglen = 200;

sub get_command_and_cmdline($)
{
  my ($file, $args) = split /\s+/, $_[0], 2;

  my $full = $file;
  $full .= " $args" if defined $args;
  $full =~ s/"/\\"/g;

  if (length($full) > $arglen) {
    print "$line: \"$full\" too long...\n";
    exit 1;
  }

  ($file, $full);
}

# extract an entry with modules from a modules.list file
sub get_module_entry($$)
{
  my ($mod_file, $entry_to_pick) = @_;
  my @mods;
  my %groups;

  if ($entry_to_pick eq 'auto-build-entry') {
    # Automatic build entry is being built.
    # This image is useless but it always builds.

    $mods[0] = { command => 'Makefile', cmdline => 'Makefile', type => 'bin'};
    $mods[1] = { command => 'Makefile', cmdline => 'Makefile', type => 'bin'};
    $mods[2] = { command => 'Makefile', cmdline => 'Makefile', type => 'bin'};

    return (
      mods    => [ @mods ],
      modaddr => 0x200000,
    );
  }

  open(M, $mod_file) || die "Cannot open $mod_file!: $!";

  # preseed first 3 modules
  $mods[0] = { command => 'fiasco',   cmdline => 'fiasco',   type => 'bin'};
  $mods[1] = { command => 'sigma0',   cmdline => 'sigma0',   type => 'bin'};
  $mods[2] = { command => 'roottask', cmdline => 'moe',      type => 'bin'};

  my $line = 0;
  my $process_mode = undef;
  my $found_entry = 0;
  my $global = 1;
  my $modaddr_title;
  my $modaddr_global;
  my $bootstrap_command = "bootstrap";
  my $bootstrap_cmdline = "bootstrap";
  my $linux_initrd;
  my $is_mode_linux;
  while (<M>) {
    $line++;
    chomp;
    s/#.*$//;
    s/^\s*//;
    next if /^$/;

    if (/^modaddr\s+(\S+)/) {
      $modaddr_global = $1 if  $global;
      $modaddr_title  = $1 if !$global;
      next;
    }

    my ($type, $remaining) = split /\s+/, $_, 2;
    $type = lc($type);

    $type = 'bin'   if $type eq 'module';

    if ($type =~ /^(entry|title)$/) {
      if (lc($entry_to_pick) eq lc($remaining)) {
        $process_mode = 'entry';
        $found_entry = 1;
      } else {
	$process_mode = undef;
      }
      $global = 0;
      next;
    } elsif ($type eq 'searchpath') {
      push @internal_searchpaths, $remaining;
      next;
    } elsif ($type eq 'group') {
      $process_mode = 'group';
      $current_group_name = (split /\s+/, $remaining)[0];
      next;
    } elsif ($type eq 'default-bootstrap') {
      my ($file, $full) = get_command_and_cmdline($remaining);
      $bootstrap_command = $file;
      $bootstrap_cmdline = $full;
      next;
    } elsif ($type eq 'default-kernel') {
      my ($file, $full) = get_command_and_cmdline($remaining);
      $mods[0]{command}  = $file;
      $mods[0]{cmdline}  = $full;
      next;
    } elsif ($type eq 'default-sigma0') {
      my ($file, $full) = get_command_and_cmdline($remaining);
      $mods[1]{command}  = $file;
      $mods[1]{cmdline}  = $full;
      next;
    } elsif ($type eq 'default-roottask') {
      my ($file, $full) = get_command_and_cmdline($remaining);
      $mods[2]{command}  = $file;
      $mods[2]{cmdline}  = $full;
      next;
    }

    next unless $process_mode;

    my @valid_types = ( 'bin', 'data', 'bin-nostrip', 'data-nostrip',
	                'bootstrap', 'roottask', 'kernel', 'sigma0',
                        'module-perl', 'module-shell', 'module-glob',
			'module-group', 'moe', 'initrd', 'set');
    die "$line: Invalid type \"$type\""
      unless grep(/^$type$/, @valid_types);

    @m = ( $remaining );

    if ($type eq 'set') {
      my ($varname, $value) = split /\s+/, $remaining, 2;
      $is_mode_linux = 1 if $varname eq 'mode' and lc($value) eq 'linux';
    }

    if ($type eq 'module-perl') {
      @m = eval $remaining;
      die $@ if $@;
      $type = 'bin';
    } elsif ($type eq 'module-shell') {
      @m = split /\n/, `$remaining`;
      die "Shell command on line $line failed\n" if $?;
      $type = 'bin';
    } elsif ($type eq 'module-glob') {
      @m = glob $remaining;
      $type = 'bin';
    } elsif ($type eq 'module-group') {
      @m = ();
      foreach (split /\s+/, $remaining) {
	die "Unknown group '$_'" unless defined $groups{$_};
        push @m, @{$groups{$_}};
      }
      $type = 'bin';
    } elsif ($type eq 'moe') {
      $mods[2]{command}  = 'moe';
      $mods[2]{cmdline}  = "moe rom/$remaining";
      $type = 'bin';
      @m = ($remaining);
    }
    next if not defined $m[0] or $m[0] eq '';

    if ($process_mode eq 'entry') {
      foreach my $m (@m) {

        my ($file, $full) = get_command_and_cmdline($m);

	# special cases
	if ($type eq 'bootstrap') {
	  $bootstrap_command = $file;
	  $bootstrap_cmdline = $full;
	} elsif ($type =~ /(rmgr|roottask)/i) {
	  $mods[2]{command}  = $file;
	  $mods[2]{cmdline}  = $full;
	} elsif ($type eq 'kernel') {
	  $mods[0]{command}  = $file;
	  $mods[0]{cmdline}  = $full;
	} elsif ($type eq 'sigma0') {
	  $mods[1]{command}  = $file;
	  $mods[1]{cmdline}  = $full;
        } elsif ($type eq 'initrd') {
          $linux_initrd      = $file;
          $is_mode_linux     = 1;
        } else {
	  push @mods, {
			type    => $type,
			command => $file,
			cmdline => $full,
		      };
	}
      }
    } elsif ($process_mode eq 'group') {
      push @{$groups{$current_group_name}}, @m;
    } else {
      die "Invalid mode '$process_mode'";
    }
  }

  close M;

  die "Unknown entry \"$entry_to_pick\"!" unless $found_entry;
  die "'modaddr' not set" unless $modaddr_title || $modaddr_global;

  my $m = $modaddr_title || $modaddr_global;
  if (defined $is_mode_linux)
    {
      die "No Linux kernel image defined" unless defined $mods[0]{cmdline};
      print STDERR "Entry '$entry_to_pick' is a Linux type entry\n";
      my @files;
      # @files is actually redundant but eases file selection for entry
      # generators
      push @files, $mods[0]{command};
      push @files, $linux_initrd if defined $linux_initrd;
      my %r;
      %r = (
             # actually bootstrap is always the kernel in this
             # environment, for convenience we use $mods[0] because that
             # are the contents of 'kernel xxx' which sounds more
             # reasonable
             bootstrap => { command => $mods[0]{command},
                            cmdline => $mods[0]{cmdline}},
             type      => 'Linux',
             files     => [ @files ],
           );
      $r{initrd} = { cmdline => $linux_initrd } if defined $linux_initrd;
      return %r;
    }

  # now some implicit stuff
  if ($bootstrap_cmdline =~ /-modaddr\s+/) {
    $bootstrap_cmdline =~ s/(-modaddr\s+)%modaddr%/$1$m/;
  } else {
    $bootstrap_cmdline .= " -modaddr $m";
  }

  my @filse; # again, this is redundant but helps generator functions
  push @files, $bootstrap_command;
  push @files, $_->{command} foreach @mods;

  return (
           bootstrap => { command => $bootstrap_command,
                          cmdline => $bootstrap_cmdline },
           mods    => [ @mods ],
           modaddr => $modaddr_title || $modaddr_global,
           type    => 'MB',
           files   => [ @files ],
         );
}

sub entry_is_linux(%)
{
  my %e = @_;
  return defined $e{type} && $e{type} eq 'Linux';
}

sub entry_is_mb(%)
{
  my %e = @_;
  return defined $e{type} and $e{type} eq 'MB';
}

sub get_entries($)
{
  my ($mod_file) = @_;
  my @entry_list;

  open(M, $mod_file) || die "Cannot open $mod_file!: $!";

  while (<M>) {
    chomp;
    s/#.*$//;
    s/^\s*//;
    next if /^$/;
    push @entry_list, $2 if /^(entry|title)\s+(.+)/;
  }

  close M;

  return @entry_list;
}

# Search for a file by using a path list (single string, split with colons
# or spaces, see the split)
# return undef if it could not be found, the complete path otherwise
sub search_file($$)
{
  my $file = shift;
  my $paths = shift;

  foreach my $p (split(/[:\s]+/, $paths), @internal_searchpaths) {
    return "$p/$file" if -e "$p/$file" and ! -d "$p/$file";
  }

  return $file if $file =~ /^\// && -e $file;

  undef;
}

sub search_file_or_die($$)
{
  my $file = shift;
  my $paths = shift;
  my $f = search_file($file, $paths);
  die "Could not find '$file' with path '$paths'" unless defined $f;
  $f;
}

sub get_or_copy_file_uncompressed_or_die($$$$)
{
  my $command   = shift;
  my $paths     = shift;
  my $targetdir = shift;
  my $copy      = shift;

  my $fp = L4::ModList::search_file_or_die($command, $paths);

  open F, $fp || die "connot open '$fp': $!";
  my $buf;
  read F, $buf, 2;
  close F;

  (my $tf = $fp) =~ s|.*/||;
  $tf = $targetdir.'/'.$tf;

  if (length($buf) >= 2 && unpack("n", $buf) == 0x1f8b) {
    print STDERR "'$fp' is a zipped file, uncompressing to '$tf'\n";
    system("zcat $fp >$tf");
    $fp = $tf;
  } elsif ($copy) {
    print("cp $fp $tf\n");
    system("cp $fp $tf");
    $fp = $tf;
  }

  $fp;
}

sub get_file_uncompressed_or_die($$$)
{
  return get_or_copy_file_uncompressed_or_die(shift, shift, shift, 0);
}

sub copy_file_uncompressed_or_die($$$)
{
  return get_or_copy_file_uncompressed_or_die(shift, shift, shift, 1);
}


sub generate_grub1_entry($$%)
{
  my $entryname = shift;
  my $prefix = shift;
  $prefix = '' unless defined $prefix;
  $prefix = "/$prefix" if $prefix ne '' and $prefix !~ /^\//;
  my %entry = @_;
  my $s = "title $entryname\n";
  my $c = $entry{bootstrap}{cmdline};
  $c =~ s/^\S*\/([^\/]+\s*)/$1/;
  $s .= "kernel $prefix/$c\n";

  if (entry_is_linux(%entry) and defined $entry{initrd})
    {
      $c = $entry{initrd}{cmdline};
      $c =~ s/^\S*\/([^\/]+\s*)/$1/;
      $s .= "initrd $prefix/$c\n";
      return $s;
    }

  foreach my $m (@{$entry{mods}})
    {
      $c = $m->{cmdline};
      $c =~ s/^\S*\/([^\/]+\s*)/$1/;
      $s .= "module $prefix/$c\n";
    }
  $s;
}

sub generate_grub2_entry($$%)
{
  my $entryname = shift;
  my $prefix = shift;
  $prefix = '' unless defined $prefix;
  $prefix = "/$prefix" if $prefix ne '' and $prefix !~ /^\//;
  my %entry = @_;
  # basename of first path
  my ($c, $args) = split(/\s+/, $entry{bootstrap}{cmdline}, 2);
  my $bn = (reverse split(/\/+/, $c))[0];
  my $s = "menuentry \"$entryname\" {\n";

  if (entry_is_linux(%entry))
    {
      $s .= "  echo Loading '$prefix/$bn $args'\n";
      $s .= "  linux $prefix/$bn $args\n";
      if (defined $entry{initrd})
        {
          $bn = (reverse split(/\/+/, $entry{initrd}{cmdline}))[0];
          my $c = $entry{initrd}{cmdline};
          $c =~ s/^\S*(\/[^\/]+\s*)/$1/;
          $s .= "  initrd $prefix$c\n";
        }
    }
  else
    {
      $s .= "  echo Loading '$prefix/$bn $prefix/$bn $args'\n";
      $s .= "  multiboot $prefix/$bn $prefix/$bn $args\n";
      foreach my $m (@{$entry{mods}})
        {
          # basename
          $bn = (reverse split(/\/+/, $m->{command}))[0];
          my $c = $m->{cmdline};
          $c =~ s/^\S*(\/[^\/]+\s*)/$1/;
          $s .= "  echo Loading '$prefix/$bn $c'\n";
          $s .= "  module $prefix/$bn $c\n";
        }
    }
  $s .= "  echo Done, booting...\n";
  $s .= "}\n";
}

return 1;
