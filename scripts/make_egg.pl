#! /usr/bin/perl -w
#
#	Generate a tectonic plate-and-cutlery (in PLATES format)
#

my $num_points = 30;
my ($centre_x, $centre_y) = (-1, 75);
my $radius = 30;
my $PI = 3.14159265358979323846264338;

####################################################################
print "7777 1244 DINNER PLATE RIM\n";
print " 700  200.0 -999.0 CS   1 700   4   $num_points\n";

for (my $i = 0; $i < $num_points; ++$i) {
	my $x = $centre_x + $radius * cos (2 * $PI * $i / ($num_points - 1));
	my $y = $centre_y + $radius * sin (2 * $PI * $i / ($num_points - 1));
	printf "  %9.4f %9.4f %d\n", $x, $y, ($i == 0 ? 3 : 2);
}
print "  99 99 3\n";

####################################################################
print "7777 1244 DINNER PLATE BEVEL\n";
print " 701  200.0 -999.0 CS   1 701   4   $num_points\n";
$radius *= 0.8;
for (my $i = 0; $i < $num_points; ++$i) {
	my $x = $centre_x + $radius * cos (2 * $PI * $i / ($num_points - 1));
	my $y = $centre_y + $radius * sin (2 * $PI * $i / ($num_points - 1));
	printf "  %9.4f %9.4f %d\n", $x, $y, ($i == 0 ? 3 : 2);
}
print "  99 99 3\n";

print <<EOF;
7777 1244 DINNER FORK
 702  200.0 -999.0 CS   1 702   4   24
 -23.89 38.50 3
 -21.67 37.00 2
   6.00 37.00 2
   9.00 35.00 2
  14.50 35.00 2
  15.50 36.00 2
  11.00 36.00 2
  11.00 37.00 2
  14.50 37.00 2
  15.50 37.50 2
  14.50 38.00 2
  11.00 38.00 2
  11.00 39.00 2
  14.50 39.00 2
  15.50 39.50 2
  14.50 40.00 2
  11.00 40.00 2
  11.00 41.00 2
  15.50 41.00 2
  14.50 42.00 2
   9.00 42.00 2
   6.00 40.00 2
 -21.67 40.00 2
 -23.89 38.50 2
  99 99 3
EOF
