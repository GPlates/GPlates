#! /usr/bin/perl -w

use strict;


# Filters are in three sections:
#	- prefix (all start with "^")
#	- suffix (all end with "$")
#	- other
# The first section that a regex falls into is where it belongs
my @filter_regexes = (
	"^char",
	"^std::",
	"^void",
	"^__do_global_"
);

# Process call graph
while (<>) {
	print;
	last if /^index/;
}
my $buffer = "";
my $skip = 0;
LINE: while (<>) {
	last if /^Index by function name/;
	if (/^\[/) {
		# Found "focus" line in this block
		my $ln = $_;
		$ln =~ s/^[^A-Za-z(_]*//;
		$ln =~ s/ \[\d+\]$//;
		foreach my $filter (@filter_regexes) {
			if ($ln =~ /$filter/) {
				$skip = 1;
				while (<>) {
					goto final_line if /^---/;
				}
			}
		}
	} elsif (/\[\d+\]$/) {
		my $ln = $_;
		$ln =~ s/^[^A-Za-z(_]*//;
		$ln =~ s/ \[\d+\]$//;
		foreach my $filter (@filter_regexes) {
			next LINE if $ln =~ /$filter/;
		}
	}

	# Some conditioning
	# 1: remove parameters entirely
	s/\([^)]*\)//g;
	# 2: remove const specifier
	s/ const//;

	$buffer .= $_;
	next unless /^---/;
final_line:
	print $buffer unless $skip;
	$buffer = "";
	$skip = 0;
}

# Munch remainder
while (<>) {
	# do nothing
}
