[1] GPlates 0.9.4 released
--------------------------

The GPlates development team is proud to release GPlates 0.9.4, the
fifth public release of GPlates.

GPlates 0.9.4 is the first release of GPlates which runs on all three
mainstream operating systems:  Windows, Linux and MacOS X.  That's right,
GPlates now finally officially runs on OS X!

As I'm sure we're all aware, this has been a long time coming.  The lion's
share of the work was performed by our new star programmer, JOHN CANNON,
with contributions from all the other GPlates developers.  Congratulations
to all!

More detailed information about this release may be found on the News page
of the GPlates website:
 http://www.gplates.org/news.html#gplates094

Screenshots of GPlates 0.9.4 are available:
 http://www.gplates.org/screenshots.html

You can download GPlates 0.9.4, and GPlates-compatible data:
 http://www.gplates.org/downloads.html

This release consists of three files:
 * a source tarball for Linux and MacOS X
 * a source zip-file for Windows
 * a ".MSI" Windows installer which contains a binary GPlates executable,
   the EarthByte coastline and rotation files, Bernhard Steinberger's
   time-dependent dynamic topography raster images, and the time-dependent
   ocean floor-age and spreading-rate raster images created by the
   EarthByte group.

The GPlates source code and binaries are distributed under the terms of
the GNU General Public License (GPL).

GPlates 0.9.4 compiles and runs on Windows Vista, Windows XP, Linux and
MacOS X.  The Windows installer works on Vista and XP.

GPlates is developed at the University of Sydney, the California Institute
of Technology and the Norwegian Geological Survey.


[2] Recent GPlates development
------------------------------

This is the first release which contains the work of new GPlates developer
John Cannon -- and he has done a LOT of work.  In addition to porting
GPlates 0.9.3 to OS X, John also:
 * completed the transition to the new CMake-based build system (which was
   commenced by HAMISH IVEY-LAW in February of this year and which, in
   addition to enabling GPlates to compile on OS X, will also enable the
   GPlates developers to work more efficiently in the future)
 * coordinated the OS X beta trial
 * implemented graphical functionality for the interactive manipulation of
   feature geometries and digitised geometries by dragging vertices
 * implemented GMT export of feature collection data files and digitised
   geometries

ROBIN WATSON has added to GPlates: the ability to specify surface extents
for raster images, to enable GPlates to correctly position raster images
which do not cover the entire globe; the ability to edit Shapefile
attributes; and a tabular (spreadsheet-like) display of all Shapefile
attributes in a file.  In addition, Robin continues to work on the epic
task of implementing multiple alternative map projections in GPlates,
which will be available in addition to the current 3-D orthographic
projection of the globe.

JAMES CLARK has applied a variety of small but useful improvements to the
GPlates user interface, most significantly the enhancements to the Manage
Feature Collections dialog which enable a user to reload a file from disk
with a single click, as well as the ability to enable or disable a feature
collection without unloading or reloading the file.

MARK TURNER continues to work on the epic task of porting the GPlates 0.8
interactive plate-boundary closure functionality to the GPlates 0.9 code
base.

In addition, the following members of the GPlates community took part in
the beta trial of GPlates on OS X:
 * Christian Heine
 * Bob Kopp
 * Patrice Rey
 * Maria Seton
 * Jo Whittaker
 * Florian Wobbe

Thank you to all participants for your enthusiasm and your generosity of
time and effort!


[3] GPlates download numbers
----------------------------

GPlates is distributed via the GPlates Project site on SourceForge:
 http://sourceforge.net/projects/gplates/

SourceForge tracks the downloads of the GPlates files, and presents us
with daily totals and longer-term statistics.

About a month ago, we were pleasantly surprised to discover that the
GPlates download totals seemed to be sky-rocketing.  As of right now,
GPlates 0.9.3.1, the most recent release of GPlates, has been downloaded a
total of 2663 times over the course of two months!  That's almost 300
downloads a week!

We think the cause of all this attention may be the mention of GPlates on
the NASA website:
 http://gcmd.nasa.gov/records/GPlates.html
but we have no evidence for this claim.

If anyone has any other explanations, please let us know!  :)


[4] Current and short-term future development
---------------------------------------------

GPlates development tasks which are currently in progress, were recently
completed, or will be commenced in the near future, include:
 * Map projections (Robin Watson)
 * Saving in Shapefile format (Robin Watson)
 * A user manual (Rhi McKeon and James Boyden)
 * Interactive graphical manipulation of feature geometries (John Cannon)
 * Interactive creation of dynamic closed plate boundaries (Mark Turner)
 * Various user-interface enhancements (James Clark)


Thanks for reading, and enjoy GPlates 0.9.4!


