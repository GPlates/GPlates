#! /usr/bin/perl -w

use strict;


# Process call graph
while (<>) {
	$_ =~ s/,\s+int/,int/g;
	print $_;
}
